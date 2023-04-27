/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bundle_mgr_service_event_handler.h"

#include <future>
#include <sys/stat.h>

#include "accesstoken_kit.h"
#include "access_token.h"
#include "app_log_wrapper.h"
#include "app_provision_info.h"
#include "app_provision_info_manager.h"
#include "app_privilege_capability.h"
#include "bundle_install_checker.h"
#include "bundle_mgr_service.h"
#include "bundle_parser.h"
#include "bundle_permission_mgr.h"
#include "bundle_scanner.h"
#include "bundle_util.h"
#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "common_event_subscriber.h"
#ifdef CONFIG_POLOCY_ENABLE
#include "config_policy_utils.h"
#endif
#include "event_report.h"
#include "installd_client.h"
#include "parameter.h"
#include "perf_profile.h"
#ifdef WINDOW_ENABLE
#include "scene_board_judgement.h"
#endif
#include "status_receiver_host.h"
#include "system_bundle_installer.h"
#ifdef BUNDLE_FRAMEWORK_QUICK_FIX
#include "quick_fix_boot_scanner.h"
#endif
#include "want.h"
#include "user_unlocked_event_subscriber.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
const std::string SYSTEM_UI_BUNDLE_NAME = "com.ohos.systemui";
const std::string LAUNCHER_BUNDLE_NAME = "com.ohos.launcher";
const std::string SCENE_BOARD_BUNDLE_NAME = "com.ohos.sceneboard";
const std::string APP_SUFFIX = "/app";
const std::string TEMP_PREFIX = "temp_";
const std::string MODULE_PREFIX = "module_";
const std::string PRE_INSTALL_HSP_PATH = "/shared_bundles/";
const std::string BMS_TEST_UPGRADE = "persist.bms.test-upgrade";
// this metadata used to indicate those system application update by hotpatch upgrade.
const std::string HOT_PATCH_METADATA = "ohos.app.quickfix";
const std::string FINGERPRINT = "fingerprint";
const std::string UNKNOWN = "";
const std::string VALUE_TRUE = "true";
const int32_t VERSION_LEN = 64;
const std::vector<std::string> FINGERPRINTS = {
    "const.product.software.version",
    "const.product.build.type",
    "const.product.brand",
    "const.product.name",
    "const.product.devicetype",
    "const.product.incremental.version",
    "const.comp.hl.product_base_version.real"
};
const std::string HSP_VERSION_PREFIX = "v";

std::set<PreScanInfo> installList_;
std::set<std::string> uninstallList_;
std::set<PreBundleConfigInfo> installListCapabilities_;
bool hasLoadPreInstallProFile_ = false;

void MoveTempPath(const std::vector<std::string> &fromPaths,
    const std::string &bundleName, std::vector<std::string> &toPaths)
{
    std::string tempDir =
        Constants::HAP_COPY_PATH + Constants::PATH_SEPARATOR + TEMP_PREFIX + bundleName;
    if (!BundleUtil::CreateDir(tempDir)) {
        APP_LOGE("create tempdir failed %{public}s", tempDir.c_str());
        return;
    }

    int32_t hapIndex = 0;
    for (const auto &path : fromPaths) {
        auto toPath = tempDir + Constants::PATH_SEPARATOR + MODULE_PREFIX
            + std::to_string(hapIndex) + Constants::INSTALL_FILE_SUFFIX;
        hapIndex++;
        if (InstalldClient::GetInstance()->MoveFile(path, toPath) != ERR_OK) {
            APP_LOGW("move from %{public}s to %{public}s failed", path.c_str(), toPath.c_str());
            continue;
        }

        toPaths.emplace_back(toPath);
    }
}

class InnerReceiverImpl : public StatusReceiverHost {
public:
    InnerReceiverImpl() = default;
    virtual ~InnerReceiverImpl() override = default;

    void SetBundleName(const std::string &bundleName)
    {
        bundleName_ = bundleName;
    }

    virtual void OnStatusNotify(const int progress) override {}
    virtual void OnFinished(
        const int32_t resultCode, const std::string &resultMsg) override
    {
        if (bundleName_.empty()) {
            return;
        }

        std::string tempDir = Constants::HAP_COPY_PATH
            + Constants::PATH_SEPARATOR + TEMP_PREFIX + bundleName_;
        APP_LOGD("delete tempDir %{public}s", tempDir.c_str());
        BundleUtil::DeleteDir(tempDir);
    }

private:
    std::string bundleName_;
};
}

BMSEventHandler::BMSEventHandler(const std::shared_ptr<EventRunner> &runner) : EventHandler(runner)
{
    APP_LOGD("instance is created");
}

BMSEventHandler::~BMSEventHandler()
{
    APP_LOGD("instance is destroyed");
}

void BMSEventHandler::ProcessEvent(const InnerEvent::Pointer &event)
{
    switch (event->GetInnerEventId()) {
        case BMS_START: {
            BmsStartEvent();
            break;
        }
        default:
            APP_LOGE("the eventId is not supported");
            break;
    }
}

void BMSEventHandler::BmsStartEvent()
{
    BeforeBmsStart();
    OnBmsStarting();
    AfterBmsStart();
}

void BMSEventHandler::BeforeBmsStart()
{
    needNotifyBundleScanStatus_ = false;
    if (!BundlePermissionMgr::Init()) {
        APP_LOGW("BundlePermissionMgr::Init failed");
    }

    EventReport::SendScanSysEvent(BMSEventType::BOOT_SCAN_START);
}

void BMSEventHandler::OnBmsStarting()
{
    // Judge whether there is install info in the persistent Db
    if (LoadInstallInfosFromDb()) {
        APP_LOGD("Load install info from db success");
        BundleRebootStartEvent();
        return;
    }

    // If the preInstall infos does not exist in preInstall db,
    // all preInstall directory applications will be reinstalled.
    if (!LoadAllPreInstallBundleInfos()) {
        APP_LOGE("Load all preInstall bundleInfos failed.");
        needRebootOta_ = true;
    }

    /* Guard against install infos lossed strategy.
     * 1. Scan user data dir
     *   1.1. If no data, first boot.
     *   1.2. If has data, but parse data to InnerBundleUserInfos failed,
     *        reInstall all app from install dir and preInstall dir
     *   1.3. If has data and parse data to InnerBundleUserInfos success, goto 2
     * 2. Scan installDir include common install dir and preInstall dir
     *    And the parse the hap to InnerBundleInfos
     * 3. Combine InnerBundleInfos and InnerBundleUserInfos to cache and db
     * 4. According to needRebootOta determine whether OTA detection is required
     */
    ResultCode resultCode = GuardAgainstInstallInfosLossedStrategy();
    switch (resultCode) {
        case ResultCode::RECOVER_OK: {
            APP_LOGD("Guard against install infos lossed strategy take effect.");
            if (needRebootOta_) {
                BundleRebootStartEvent();
            } else {
                needNotifyBundleScanStatus_ = true;
            }

            break;
        }
        case ResultCode::REINSTALL_OK: {
            APP_LOGD("ReInstall all haps.");
            needNotifyBundleScanStatus_ = true;
            break;
        }
        case ResultCode::NO_INSTALLED_DATA: {
            // First boot
            BundleBootStartEvent();
            break;
        }
        default:
            APP_LOGE("System internal error, install informations missing.");
            break;
    }

    SaveSystemFingerprint();
}

void BMSEventHandler::AfterBmsStart()
{
#ifdef BUNDLE_FRAMEWORK_QUICK_FIX
    DelayedSingleton<QuickFixBootScanner>::GetInstance()->ProcessQuickFixBootUp();
#endif
    DelayedSingleton<BundleMgrService>::GetInstance()->CheckAllUser();
    BundlePermissionMgr::UnInit();
    SetAllInstallFlag();
    HandleSceneBoard();
    DelayedSingleton<BundleMgrService>::GetInstance()->RegisterService();
    EventReport::SendScanSysEvent(BMSEventType::BOOT_SCAN_END);
    ClearCache();
    if (needNotifyBundleScanStatus_) {
        DelayedSingleton<BundleMgrService>::GetInstance()->NotifyBundleScanStatus();
    }
    ListeningUserUnlocked();
}

void BMSEventHandler::ClearCache()
{
    hapParseInfoMap_.clear();
    loadExistData_.clear();
    hasLoadAllPreInstallBundleInfosFromDb_ = false;
}

bool BMSEventHandler::LoadInstallInfosFromDb()
{
    APP_LOGD("Load install infos from db");
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }

    return dataMgr->LoadDataFromPersistentStorage();
}

void BMSEventHandler::BundleBootStartEvent()
{
    OnBundleBootStart(Constants::DEFAULT_USERID);
    PerfProfile::GetInstance().Dump();
}

void BMSEventHandler::BundleRebootStartEvent()
{
#ifdef USE_PRE_BUNDLE_PROFILE
    if (LoadPreInstallProFile()) {
        UpdateAllPrivilegeCapability();
    }
#endif

    if (IsSystemUpgrade()) {
        OnBundleRebootStart();
        SaveSystemFingerprint();
    }

    needNotifyBundleScanStatus_ = true;
}

ResultCode BMSEventHandler::GuardAgainstInstallInfosLossedStrategy()
{
    APP_LOGD("GuardAgainstInstallInfosLossedStrategy start");
    // Check user path, and parse userData to InnerBundleUserInfo
    std::map<std::string, std::vector<InnerBundleUserInfo>> innerBundleUserInfoMaps;
    ScanResultCode scanResultCode = ScanAndAnalyzeUserDatas(innerBundleUserInfoMaps);
    if (scanResultCode == ScanResultCode::SCAN_NO_DATA) {
        APP_LOGE("Scan the user data directory failed");
        return ResultCode::NO_INSTALLED_DATA;
    }

    // When data exist, but parse all userinfo fails, reinstall all app.
    // For example: the AT database is lost or others.
    if (scanResultCode == ScanResultCode::SCAN_HAS_DATA_PARSE_FAILED) {
        // Reinstall all app from install dir
        return ReInstallAllInstallDirApps();
    }

    // When data exist and parse all userinfo success,
    // it can be judged that some bundles has installed.
    // Check install dir, and parse the hap in install dir to InnerBundleInfo
    std::map<std::string, std::vector<InnerBundleInfo>> installInfos;
    ScanAndAnalyzeInstallInfos(installInfos);
    if (installInfos.empty()) {
        APP_LOGE("check bundle path failed due to hap lossd or parse failed");
        return ResultCode::SYSTEM_ERROR;
    }

    // Combine InnerBundleInfo and InnerBundleUserInfo
    if (!CombineBundleInfoAndUserInfo(installInfos, innerBundleUserInfoMaps)) {
        APP_LOGE("System internal error");
        return ResultCode::SYSTEM_ERROR;
    }

    return ResultCode::RECOVER_OK;
}

ScanResultCode BMSEventHandler::ScanAndAnalyzeUserDatas(
    std::map<std::string, std::vector<InnerBundleUserInfo>> &userMaps)
{
    ScanResultCode scanResultCode = ScanResultCode::SCAN_NO_DATA;
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("dataMgr is null");
        return scanResultCode;
    }

    std::string baseDataDir = Constants::BUNDLE_APP_DATA_BASE_DIR + Constants::BUNDLE_EL[0];
    std::vector<std::string> userIds;
    if (!ScanDir(baseDataDir, ScanMode::SUB_FILE_DIR, ResultMode::RELATIVE_PATH, userIds)) {
        APP_LOGD("Check the base user directory(%{public}s) failed", baseDataDir.c_str());
        return scanResultCode;
    }

    for (const auto &userId : userIds) {
        int32_t userIdInt = Constants::INVALID_USERID;
        if (!StrToInt(userId, userIdInt)) {
            APP_LOGE("UserId(%{public}s) strToInt failed", userId.c_str());
            continue;
        }

        dataMgr->AddUserId(userIdInt);
        std::vector<std::string> userDataBundleNames;
        std::string userDataDir = baseDataDir + Constants::PATH_SEPARATOR + userId + Constants::BASE;
        if (!ScanDir(userDataDir, ScanMode::SUB_FILE_DIR, ResultMode::RELATIVE_PATH, userDataBundleNames)) {
            APP_LOGD("Check the user installation directory(%{public}s) failed", userDataDir.c_str());
            continue;
        }

        for (const auto &userDataBundleName : userDataBundleNames) {
            if (scanResultCode == ScanResultCode::SCAN_NO_DATA) {
                scanResultCode = ScanResultCode::SCAN_HAS_DATA_PARSE_FAILED;
            }

            if (AnalyzeUserData(userIdInt, userDataDir, userDataBundleName, userMaps)) {
                scanResultCode = ScanResultCode::SCAN_HAS_DATA_PARSE_SUCCESS;
            }
        }
    }

    return scanResultCode;
}

bool BMSEventHandler::AnalyzeUserData(
    int32_t userId, const std::string &userDataDir, const std::string &userDataBundleName,
    std::map<std::string, std::vector<InnerBundleUserInfo>> &userMaps)
{
    if (userDataDir.empty() || userDataBundleName.empty()) {
        APP_LOGE("UserDataDir or UserDataBundleName is empty");
        return false;
    }

    std::string userDataBundlePath = userDataDir + userDataBundleName;
    APP_LOGD("Analyze user data path(%{public}s)", userDataBundlePath.c_str());
    FileStat fileStat;
    if (InstalldClient::GetInstance()->GetFileStat(userDataBundlePath, fileStat) != ERR_OK) {
        APP_LOGE("GetFileStat path(%{public}s) failed", userDataBundlePath.c_str());
        return false;
    }

    // It should be a bundleName dir
    if (!fileStat.isDir) {
        APP_LOGE("UserDataBundlePath(%{public}s) is not dir", userDataBundlePath.c_str());
        return false;
    }

    InnerBundleUserInfo innerBundleUserInfo;
    innerBundleUserInfo.bundleName = userDataBundleName;
    innerBundleUserInfo.bundleUserInfo.userId = userId;
    innerBundleUserInfo.uid = fileStat.uid;
    innerBundleUserInfo.gids.emplace_back(fileStat.gid);
    innerBundleUserInfo.installTime = fileStat.lastModifyTime;
    innerBundleUserInfo.updateTime = innerBundleUserInfo.installTime;
    auto accessTokenIdEx = OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenIDEx(
        innerBundleUserInfo.bundleUserInfo.userId, userDataBundleName, 0);
    if (accessTokenIdEx.tokenIdExStruct.tokenID == 0) {
        APP_LOGE("get tokenId failed.");
        return false;
    }

    innerBundleUserInfo.accessTokenId = accessTokenIdEx.tokenIdExStruct.tokenID;
    innerBundleUserInfo.accessTokenIdEx = accessTokenIdEx.tokenIDEx;
    auto userIter = userMaps.find(userDataBundleName);
    if (userIter == userMaps.end()) {
        std::vector<InnerBundleUserInfo> innerBundleUserInfos = { innerBundleUserInfo };
        userMaps.emplace(userDataBundleName, innerBundleUserInfos);
        return true;
    }

    userMaps.at(userDataBundleName).emplace_back(innerBundleUserInfo);
    return true;
}

ResultCode BMSEventHandler::ReInstallAllInstallDirApps()
{
    // First, reinstall all preInstall app from preInstall dir
    std::vector<std::string> preInstallDirs;
    GetPreInstallDir(preInstallDirs);
    for (const auto &preInstallDir : preInstallDirs) {
        std::vector<std::string> filePaths { preInstallDir };
        bool removable = IsPreInstallRemovable(preInstallDir);
        if (!OTAInstallSystemBundle(
            filePaths, Constants::AppType::SYSTEM_APP, removable)) {
            APP_LOGE("Reinstall bundle(%{public}s) error.", preInstallDir.c_str());
            continue;
        }
    }

    auto installer = DelayedSingleton<BundleMgrService>::GetInstance()->GetBundleInstaller();
    if (installer == nullptr) {
        APP_LOGE("installer is nullptr");
        return ResultCode::SYSTEM_ERROR;
    }

    // Second, reInstall all common install app from install dir
    std::map<std::string, std::vector<std::string>> hapPathsMap;
    ScanInstallDir(hapPathsMap);
    for (const auto &hapPaths : hapPathsMap) {
        InstallParam installParam;
        installParam.userId = Constants::ALL_USERID;
        installParam.installFlag = InstallFlag::REPLACE_EXISTING;
        installParam.streamInstallMode = true;
        sptr<InnerReceiverImpl> innerReceiverImpl(new (std::nothrow) InnerReceiverImpl());
        innerReceiverImpl->SetBundleName(hapPaths.first);
        std::vector<std::string> tempHaps;
        MoveTempPath(hapPaths.second, hapPaths.first, tempHaps);
        installer->Install(tempHaps, installParam, innerReceiverImpl);
    }

    return ResultCode::REINSTALL_OK;
}

void BMSEventHandler::ScanAndAnalyzeInstallInfos(
    std::map<std::string, std::vector<InnerBundleInfo>> &installInfos)
{
    // Scan the installed directory
    std::map<std::string, std::vector<std::string>> hapPathsMap;
    ScanInstallDir(hapPathsMap);
    AnalyzeHaps(false, hapPathsMap, installInfos);

    // Scan preBundle directory
    std::vector<std::string> preInstallDirs;
    GetPreInstallDir(preInstallDirs);
    AnalyzeHaps(true, preInstallDirs, installInfos);
}

void BMSEventHandler::ScanInstallDir(
    std::map<std::string, std::vector<std::string>> &hapPathsMap)
{
    APP_LOGD("Scan the installed directory start");
    std::vector<std::string> bundleNameList;
    if (!ScanDir(Constants::BUNDLE_CODE_DIR, ScanMode::SUB_FILE_DIR, ResultMode::RELATIVE_PATH, bundleNameList)) {
        APP_LOGE("Check the bundle directory(%{public}s) failed", Constants::BUNDLE_CODE_DIR);
        return;
    }

    for (const auto &bundleName : bundleNameList) {
        std::vector<std::string> hapPaths;
        auto appCodePath = Constants::BUNDLE_CODE_DIR + Constants::PATH_SEPARATOR + bundleName;
        if (!ScanDir(appCodePath, ScanMode::SUB_FILE_FILE, ResultMode::ABSOLUTE_PATH, hapPaths)) {
            APP_LOGE("Scan the appCodePath(%{public}s) failed", appCodePath.c_str());
            continue;
        }

        if (hapPaths.empty()) {
            APP_LOGD("The directory(%{public}s) scan result is empty", appCodePath.c_str());
            continue;
        }

        std::vector<std::string> checkHapPaths = CheckHapPaths(hapPaths);
        hapPathsMap.emplace(bundleName, checkHapPaths);
    }

    APP_LOGD("Scan the installed directory end");
}

std::vector<std::string> BMSEventHandler::CheckHapPaths(
    const std::vector<std::string> &hapPaths)
{
    std::vector<std::string> checkHapPaths;
    for (const auto &hapPath : hapPaths) {
        if (!BundleUtil::CheckFileType(hapPath, Constants::INSTALL_FILE_SUFFIX)) {
            APP_LOGE("Check hapPath(%{public}s) failed", hapPath.c_str());
            continue;
        }

        checkHapPaths.emplace_back(hapPath);
    }

    return checkHapPaths;
}

void BMSEventHandler::GetPreInstallRootDirList(std::vector<std::string> &rootDirList)
{
#ifdef CONFIG_POLOCY_ENABLE
    auto cfgDirList = GetCfgDirList();
    if (cfgDirList != nullptr) {
        for (const auto &cfgDir : cfgDirList->paths) {
            if (cfgDir == nullptr) {
                continue;
            }

            APP_LOGD("cfgDir: %{public}s ", cfgDir);
            rootDirList.emplace_back(cfgDir);
        }

        FreeCfgDirList(cfgDirList);
    }
#endif
    bool ret = std::find(
        rootDirList.begin(), rootDirList.end(), Constants::DEFAULT_PRE_BUNDLE_ROOT_DIR) != rootDirList.end();
    if (!ret) {
        rootDirList.emplace_back(Constants::DEFAULT_PRE_BUNDLE_ROOT_DIR);
    }
}

void BMSEventHandler::ClearPreInstallCache()
{
    if (!hasLoadPreInstallProFile_) {
        return;
    }

    installList_.clear();
    uninstallList_.clear();
    installListCapabilities_.clear();
    hasLoadPreInstallProFile_ = false;
}

bool BMSEventHandler::LoadPreInstallProFile()
{
    if (hasLoadPreInstallProFile_) {
        return !installList_.empty();
    }

    std::vector<std::string> rootDirList;
    GetPreInstallRootDirList(rootDirList);
    if (rootDirList.empty()) {
        APP_LOGE("dirList is empty");
        return false;
    }

    for (const auto &rootDir : rootDirList) {
        ParsePreBundleProFile(rootDir + Constants::PRODUCT_SUFFIX);
    }

    hasLoadPreInstallProFile_ = true;
    return !installList_.empty();
}

bool BMSEventHandler::HasPreInstallProfile()
{
    return !installList_.empty();
}

void BMSEventHandler::ParsePreBundleProFile(const std::string &dir)
{
    BundleParser bundleParser;
    bundleParser.ParsePreInstallConfig(
        dir + Constants::INSTALL_LIST_CONFIG, installList_);
    bundleParser.ParsePreUnInstallConfig(
        dir + Constants::UNINSTALL_LIST_CONFIG, uninstallList_);
    bundleParser.ParsePreInstallAbilityConfig(
        dir + Constants::INSTALL_LIST_CAPABILITY_CONFIG, installListCapabilities_);
}

void BMSEventHandler::GetPreInstallDir(std::vector<std::string> &bundleDirs)
{
#ifdef USE_PRE_BUNDLE_PROFILE
    if (LoadPreInstallProFile()) {
        GetPreInstallDirFromLoadProFile(bundleDirs);
        return;
    }
#endif

    GetPreInstallDirFromScan(bundleDirs);
}

void BMSEventHandler::GetPreInstallDirFromLoadProFile(std::vector<std::string> &bundleDirs)
{
    for (const auto &installInfo : installList_) {
        if (uninstallList_.find(installInfo.bundleDir) != uninstallList_.end()) {
            APP_LOGW("bundle(%{public}s) not allowed installed", installInfo.bundleDir.c_str());
            continue;
        }

        bundleDirs.emplace_back(installInfo.bundleDir);
    }
}

void BMSEventHandler::GetPreInstallDirFromScan(std::vector<std::string> &bundleDirs)
{
    std::list<std::string> scanbundleDirs;
    GetBundleDirFromScan(scanbundleDirs);
    std::copy(scanbundleDirs.begin(), scanbundleDirs.end(), std::back_inserter(bundleDirs));
}

void BMSEventHandler::AnalyzeHaps(
    bool isPreInstallApp,
    const std::map<std::string, std::vector<std::string>> &hapPathsMap,
    std::map<std::string, std::vector<InnerBundleInfo>> &installInfos)
{
    for (const auto &hapPaths : hapPathsMap) {
        AnalyzeHaps(isPreInstallApp, hapPaths.second, installInfos);
    }
}

void BMSEventHandler::AnalyzeHaps(
    bool isPreInstallApp,
    const std::vector<std::string> &bundleDirs,
    std::map<std::string, std::vector<InnerBundleInfo>> &installInfos)
{
    for (const auto &bundleDir : bundleDirs) {
        std::unordered_map<std::string, InnerBundleInfo> hapInfos;
        if (!CheckAndParseHapFiles(bundleDir, isPreInstallApp, hapInfos) || hapInfos.empty()) {
            APP_LOGE("Parse bundleDir(%{public}s) failed", bundleDir.c_str());
            continue;
        }

        CollectInstallInfos(hapInfos, installInfos);
    }
}

void BMSEventHandler::CollectInstallInfos(
    const std::unordered_map<std::string, InnerBundleInfo> &hapInfos,
    std::map<std::string, std::vector<InnerBundleInfo>> &installInfos)
{
    for (const auto &hapInfoIter : hapInfos) {
        auto bundleName = hapInfoIter.second.GetBundleName();
        if (installInfos.find(bundleName) == installInfos.end()) {
            std::vector<InnerBundleInfo> innerBundleInfos { hapInfoIter.second };
            installInfos.emplace(bundleName, innerBundleInfos);
            continue;
        }

        installInfos.at(bundleName).emplace_back(hapInfoIter.second);
    }
}

bool BMSEventHandler::CombineBundleInfoAndUserInfo(
    const std::map<std::string, std::vector<InnerBundleInfo>> &installInfos,
    const std::map<std::string, std::vector<InnerBundleUserInfo>> &userInfoMaps)
{
    APP_LOGD("Combine code information and user data start");
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("dataMgr is null");
        return false;
    }

    if (installInfos.empty() || userInfoMaps.empty()) {
        APP_LOGE("bundleInfos or userInfos is empty");
        return false;
    }

    for (auto hasInstallInfo : installInfos) {
        auto bundleName = hasInstallInfo.first;
        auto userIter = userInfoMaps.find(bundleName);
        if (userIter == userInfoMaps.end()) {
            APP_LOGE("User data directory missing with bundle %{public}s ", bundleName.c_str());
            needRebootOta_ = true;
            continue;
        }

        for (auto &info : hasInstallInfo.second) {
            SaveInstallInfoToCache(info);
        }

        for (const auto &userInfo : userIter->second) {
            dataMgr->AddInnerBundleUserInfo(bundleName, userInfo);
        }
    }

    // Parsing uid, gids and other user information
    dataMgr->RestoreUidAndGid();
    // Load all bundle state data from jsonDb
    dataMgr->LoadAllBundleStateDataFromJsonDb();
    APP_LOGD("Combine code information and user data end");
    return true;
}

void BMSEventHandler::SaveInstallInfoToCache(InnerBundleInfo &info)
{
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("dataMgr is null");
        return;
    }

    auto bundleName = info.GetBundleName();
    auto appCodePath = Constants::BUNDLE_CODE_DIR + Constants::PATH_SEPARATOR + bundleName;
    info.SetAppCodePath(appCodePath);

    std::string dataBaseDir = Constants::BUNDLE_APP_DATA_BASE_DIR + Constants::BUNDLE_EL[1]
        + Constants::DATABASE + bundleName;
    info.SetAppDataBaseDir(dataBaseDir);

    auto moduleDir = info.GetAppCodePath() + Constants::PATH_SEPARATOR + info.GetCurrentModulePackage();
    info.AddModuleSrcDir(moduleDir);
    info.AddModuleResPath(moduleDir);

    bool bundleExist = false;
    InnerBundleInfo dbInfo;
    {
        auto &mtx = dataMgr->GetBundleMutex(bundleName);
        std::lock_guard lock { mtx };
        bundleExist = dataMgr->GetInnerBundleInfo(bundleName, dbInfo);
        if (bundleExist) {
            dataMgr->EnableBundle(bundleName);
        }
    }

    if (!bundleExist) {
        dataMgr->UpdateBundleInstallState(bundleName, InstallState::INSTALL_START);
        dataMgr->AddInnerBundleInfo(bundleName, info);
        dataMgr->UpdateBundleInstallState(bundleName, InstallState::INSTALL_SUCCESS);
        return;
    }

    auto& hapModuleName = info.GetCurModuleName();
    std::vector<std::string> dbModuleNames;
    dbInfo.GetModuleNames(dbModuleNames);
    auto iter = std::find(dbModuleNames.begin(), dbModuleNames.end(), hapModuleName);
    if (iter != dbModuleNames.end()) {
        APP_LOGE("module(%{public}s) has install", hapModuleName.c_str());
        return;
    }

    dataMgr->UpdateBundleInstallState(bundleName, InstallState::UPDATING_START);
    dataMgr->UpdateBundleInstallState(bundleName, InstallState::UPDATING_SUCCESS);
    dataMgr->AddNewModuleInfo(bundleName, info, dbInfo);
}

bool BMSEventHandler::ScanDir(
    const std::string& dir, ScanMode scanMode, ResultMode resultMode, std::vector<std::string> &resultList)
{
    APP_LOGD("Scan the directory(%{public}s) start", dir.c_str());
    ErrCode result = InstalldClient::GetInstance()->ScanDir(dir, scanMode, resultMode, resultList);
    if (result != ERR_OK) {
        APP_LOGE("Scan the directory(%{public}s) failed", dir.c_str());
        return false;
    }

    return true;
}

void BMSEventHandler::OnBundleBootStart(int32_t userId)
{
#ifdef USE_PRE_BUNDLE_PROFILE
    if (LoadPreInstallProFile()) {
        APP_LOGD("Process boot bundle install from pre bundle proFile");
        InnerProcessBootPreBundleProFileInstall(userId);
        return;
    }
#else
    ProcessBootBundleInstallFromScan(userId);
#endif
}

void BMSEventHandler::ProcessBootBundleInstallFromScan(int32_t userId)
{
    APP_LOGD("Process boot bundle install from scan");
    std::list<std::string> bundleDirs;
    GetBundleDirFromScan(bundleDirs);
    for (auto item : bundleDirs) {
        ProcessSystemBundleInstall(item, Constants::AppType::SYSTEM_APP, userId);
    }
}

void BMSEventHandler::GetBundleDirFromScan(std::list<std::string> &bundleDirs)
{
    std::vector<std::string> rootDirList;
    GetPreInstallRootDirList(rootDirList);
    if (rootDirList.empty()) {
        APP_LOGE("rootDirList is empty");
        return;
    }

    for (const auto &rootDir : rootDirList) {
        ProcessScanDir(rootDir + APP_SUFFIX, bundleDirs);
    }

    auto iter = std::find(bundleDirs.begin(), bundleDirs.end(), Constants::SYSTEM_RESOURCES_APP_PATH);
    if (iter != bundleDirs.end()) {
        bundleDirs.erase(iter);
        bundleDirs.insert(bundleDirs.begin(), Constants::SYSTEM_RESOURCES_APP_PATH);
    }
}

void BMSEventHandler::ProcessScanDir(const std::string &dir, std::list<std::string> &bundleDirs)
{
    BundleScanner scanner;
    std::list<std::string> bundleList = scanner.Scan(dir);
    for (auto item : bundleList) {
        auto iter = std::find(bundleDirs.begin(), bundleDirs.end(), item);
        if (iter == bundleDirs.end()) {
            bundleDirs.push_back(item);
        }
    }
}

void BMSEventHandler::InnerProcessBootPreBundleProFileInstall(int32_t userId)
{
    std::list<std::string> hspDirs;
    std::list<PreScanInfo> normalSystemApps;
    for (const auto &installInfo : installList_) {
        APP_LOGD("Inner process boot preBundle proFile install %{public}s", installInfo.ToString().c_str());
        if (uninstallList_.find(installInfo.bundleDir) != uninstallList_.end()) {
            APP_LOGW("bundle(%{public}s) not allowed installed when boot", installInfo.bundleDir.c_str());
            continue;
        }
        if (installInfo.bundleDir.find(PRE_INSTALL_HSP_PATH) != std::string::npos) {
            APP_LOGD("found hsp path: %{public}s", installInfo.bundleDir.c_str());
            hspDirs.emplace_back(installInfo.bundleDir);
        } else {
            normalSystemApps.emplace_back(installInfo);
        }
    }

    for (const auto &hspDir : hspDirs) {
        ProcessSystemSharedBundleInstall(hspDir, Constants::AppType::SYSTEM_APP);
    }

    for (const auto &installInfo : normalSystemApps) {
        ProcessSystemBundleInstall(installInfo, Constants::AppType::SYSTEM_APP, userId);
    }
}

void BMSEventHandler::ProcessSystemBundleInstall(
    const PreScanInfo &preScanInfo, Constants::AppType appType, int32_t userId)
{
    APP_LOGD("Process system bundle install by bundleDir(%{public}s)", preScanInfo.bundleDir.c_str());
    InstallParam installParam;
    installParam.userId = userId;
    installParam.isPreInstallApp = true;
    installParam.noSkipsKill = false;
    installParam.needSendEvent = false;
    installParam.removable = preScanInfo.removable;
    installParam.needSavePreInstallInfo = true;
    installParam.copyHapToInstallPath = false;
    SystemBundleInstaller installer;
    if (!installer.InstallSystemBundle(preScanInfo.bundleDir, installParam, appType)) {
        APP_LOGW("Install System app:%{public}s error", preScanInfo.bundleDir.c_str());
    }
}

void BMSEventHandler::ProcessSystemBundleInstall(
    const std::string &bundleDir, Constants::AppType appType, int32_t userId)
{
    APP_LOGD("Process system bundle install by bundleDir(%{public}s)", bundleDir.c_str());
    InstallParam installParam;
    installParam.userId = userId;
    installParam.isPreInstallApp = true;
    installParam.noSkipsKill = false;
    installParam.needSendEvent = false;
    installParam.removable = false;
    installParam.needSavePreInstallInfo = true;
    installParam.copyHapToInstallPath = false;
    SystemBundleInstaller installer;
    if (!installer.InstallSystemBundle(bundleDir, installParam, appType)) {
        APP_LOGW("Install System app:%{public}s error", bundleDir.c_str());
    }
}

void BMSEventHandler::ProcessSystemSharedBundleInstall(const std::string &sharedBundlePath, Constants::AppType appType)
{
    APP_LOGD("Process system shared bundle by sharedBundlePath(%{public}s)", sharedBundlePath.c_str());
    InstallParam installParam;
    installParam.isPreInstallApp = true;
    installParam.noSkipsKill = false;
    installParam.needSendEvent = false;
    installParam.removable = false;
    installParam.needSavePreInstallInfo = true;
    installParam.sharedBundleDirPaths = {sharedBundlePath};
    SystemBundleInstaller installer;
    if (!installer.InstallSystemSharedBundle(installParam, false, appType)) {
        APP_LOGW("install system shared bundle: %{public}s error", sharedBundlePath.c_str());
    }
}

void BMSEventHandler::SetAllInstallFlag() const
{
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }

    dataMgr->SetInitialUserFlag(true);
}

void BMSEventHandler::OnBundleRebootStart()
{
    ProcessRebootBundle();
}

void BMSEventHandler::ProcessRebootBundle()
{
    APP_LOGD("Process reboot bundle start");
    LoadAllPreInstallBundleInfos();
    ProcessRebootBundleInstall();
    ProcessRebootBundleUninstall();
}

bool BMSEventHandler::LoadAllPreInstallBundleInfos()
{
    if (hasLoadAllPreInstallBundleInfosFromDb_) {
        APP_LOGD("Has load all preInstall bundleInfos from db");
        return true;
    }

    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }

    std::vector<PreInstallBundleInfo> preInstallBundleInfos = dataMgr->GetAllPreInstallBundleInfos();
    for (auto &iter : preInstallBundleInfos) {
        APP_LOGD("preInstallBundleInfos: %{public}s ", iter.GetBundleName().c_str());
        loadExistData_.emplace(iter.GetBundleName(), iter);
    }

    hasLoadAllPreInstallBundleInfosFromDb_ = true;
    return !preInstallBundleInfos.empty();
}

void BMSEventHandler::ProcessRebootBundleInstall()
{
    APP_LOGD("Process reboot bundle install start");
#ifdef USE_PRE_BUNDLE_PROFILE
    if (LoadPreInstallProFile()) {
        ProcessReBootPreBundleProFileInstall();
        return;
    }
#else
    ProcessRebootBundleInstallFromScan();
#endif
}

void BMSEventHandler::ProcessReBootPreBundleProFileInstall()
{
    std::list<std::string> bundleDirs;
    std::list<std::string> sharedBundleDirs;
    for (const auto &installInfo : installList_) {
        APP_LOGD("Process reboot preBundle proFile install %{public}s", installInfo.ToString().c_str());
        if (uninstallList_.find(installInfo.bundleDir) != uninstallList_.end()) {
            APP_LOGW("bundle(%{public}s) not allowed installed when reboot", installInfo.bundleDir.c_str());
            continue;
        }

        if (installInfo.bundleDir.find(PRE_INSTALL_HSP_PATH) != std::string::npos) {
            APP_LOGD("found shared bundle path: %{private}s", installInfo.bundleDir.c_str());
            sharedBundleDirs.emplace_back(installInfo.bundleDir);
        } else {
            bundleDirs.emplace_back(installInfo.bundleDir);
        }
    }

    InnerProcessRebootSharedBundleInstall(sharedBundleDirs, Constants::AppType::SYSTEM_APP);
    InnerProcessRebootBundleInstall(bundleDirs, Constants::AppType::SYSTEM_APP);
    InnerProcessStockBundleProvisionInfo();
}

void BMSEventHandler::ProcessRebootBundleInstallFromScan()
{
    APP_LOGD("Process reboot bundle install from scan");
    std::list<std::string> bundleDirs;
    GetBundleDirFromScan(bundleDirs);
    InnerProcessRebootBundleInstall(bundleDirs, Constants::AppType::SYSTEM_APP);
    InnerProcessStockBundleProvisionInfo();
}

void BMSEventHandler::InnerProcessRebootBundleInstall(
    const std::list<std::string> &scanPathList, Constants::AppType appType)
{
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }

    for (auto &scanPathIter : scanPathList) {
        APP_LOGD("reboot scan bundle path: %{public}s ", scanPathIter.c_str());
        bool removable = IsPreInstallRemovable(scanPathIter);
        std::unordered_map<std::string, InnerBundleInfo> infos;
        if (!ParseHapFiles(scanPathIter, infos) || infos.empty()) {
            APP_LOGE("obtain bundleinfo failed : %{public}s ", scanPathIter.c_str());
            continue;
        }

        auto bundleName = infos.begin()->second.GetBundleName();
        auto hapVersionCode = infos.begin()->second.GetVersionCode();
        AddParseInfosToMap(bundleName, infos);
        auto mapIter = loadExistData_.find(bundleName);
        if (mapIter == loadExistData_.end()) {
            APP_LOGD("OTA Install new bundle(%{public}s) by path(%{private}s).",
                bundleName.c_str(), scanPathIter.c_str());
            std::vector<std::string> filePaths { scanPathIter };
            if (!OTAInstallSystemBundle(filePaths, appType, removable)) {
                APP_LOGE("OTA Install new bundle(%{public}s) error.", bundleName.c_str());
            }

            continue;
        }

        APP_LOGD("OTA process bundle(%{public}s) by path(%{private}s).",
            bundleName.c_str(), scanPathIter.c_str());
        BundleInfo hasInstalledInfo;
        auto hasBundleInstalled = dataMgr->GetBundleInfo(
            bundleName, BundleFlag::GET_BUNDLE_DEFAULT, hasInstalledInfo, Constants::ANY_USERID);
        if (!hasBundleInstalled) {
            APP_LOGW("app(%{public}s) has been uninstalled and do not OTA install.",
                bundleName.c_str());
            continue;
        }

        if (HotPatchAppProcessing(bundleName)) {
            APP_LOGD("OTA Install prefab bundle(%{public}s) by path(%{private}s) for hotPath upgrade.",
                bundleName.c_str(), scanPathIter.c_str());
            std::vector<std::string> filePaths { scanPathIter };
            if (!OTAInstallSystemBundle(filePaths, appType, removable)) {
                APP_LOGE("OTA Install prefab bundle(%{public}s) error.", bundleName.c_str());
            }

            continue;
        }

        std::vector<std::string> filePaths;
        bool updateSelinuxLabel = false;
        for (auto item : infos) {
            auto parserModuleNames = item.second.GetModuleNameVec();
            if (parserModuleNames.empty()) {
                APP_LOGE("module is empty when parser path(%{public}s).", item.first.c_str());
                continue;
            }

            // Used to judge whether the module has been installed.
            bool hasModuleInstalled = std::find(
                hasInstalledInfo.hapModuleNames.begin(), hasInstalledInfo.hapModuleNames.end(),
                parserModuleNames[0]) != hasInstalledInfo.hapModuleNames.end();
            if (HasModuleSavedInPreInstalledDb(bundleName, item.first) && !hasModuleInstalled) {
                APP_LOGW("module(%{public}s) has been uninstalled and do not OTA install",
                    parserModuleNames[0].c_str());
                continue;
            }

            // When the accessTokenIdEx is equal to 0, the old application needs to be updated.
            if (hasInstalledInfo.applicationInfo.accessTokenIdEx == 0) {
                APP_LOGD("OTA update module(%{public}s) by path(%{private}s), accessTokenIdEx is equal to 0",
                    parserModuleNames[0].c_str(), item.first.c_str());
                filePaths.emplace_back(item.first);
                continue;
            }

            // Generally, when the versionCode of Hap is greater than the installed versionCode,
            // Except for the uninstalled app, they can be installed or upgraded directly by OTA.
            if (hasInstalledInfo.versionCode < hapVersionCode) {
                APP_LOGD("OTA update module(%{public}s) by path(%{private}s)",
                    parserModuleNames[0].c_str(), item.first.c_str());
                filePaths.emplace_back(item.first);
            }

            // The versionCode of Hap is equal to the installed versionCode.
            // You can only install new modules by OTA
            if (hasInstalledInfo.versionCode == hapVersionCode) {
                // update pre install app data dir selinux label
                if (!updateSelinuxLabel) {
                    UpdateAppDataSelinuxLabel(bundleName, hasInstalledInfo.applicationInfo.appPrivilegeLevel,
                        hasInstalledInfo.isPreInstallApp,
                        hasInstalledInfo.applicationInfo.debug);
                    updateSelinuxLabel = true;
                }
                if (hasModuleInstalled) {
                    APP_LOGD("module(%{public}s) has been installed and versionCode is same.",
                        parserModuleNames[0].c_str());
                    if (UpdateModuleByHash(hasInstalledInfo, item.second)) {
                        filePaths.emplace_back(item.first);
                    }
                    continue;
                }

                APP_LOGD("OTA install module(%{public}s) by path(%{private}s)",
                    parserModuleNames[0].c_str(), item.first.c_str());
                filePaths.emplace_back(item.first);
            }
        }

        if (filePaths.empty()) {
#ifdef USE_PRE_BUNDLE_PROFILE
            UpdateRemovable(bundleName, removable);
#endif
            continue;
        }

        if (!OTAInstallSystemBundle(filePaths, appType, removable)) {
            APP_LOGE("OTA bundle(%{public}s) failed", bundleName.c_str());
#ifdef USE_PRE_BUNDLE_PROFILE
            UpdateRemovable(bundleName, removable);
#endif
        }
    }
}

bool BMSEventHandler::UpdateModuleByHash(const BundleInfo &oldBundleInfo, const InnerBundleInfo &newInfo) const
{
    auto moduleName = newInfo.GetModuleNameVec().at(0);
    std::string existModuleHash;
    for (auto hapInfo : oldBundleInfo.hapModuleInfos) {
        if (hapInfo.package == moduleName) {
            existModuleHash = hapInfo.buildHash;
        }
    }
    std::string curModuleHash;
    if (!newInfo.GetModuleBuildHash(moduleName, curModuleHash)) {
        APP_LOGD("module(%{public}s) is not existed.", moduleName.c_str());
        return false;
    }
    if (existModuleHash != curModuleHash) {
        APP_LOGD("module(%{public}s) buildHash changed, so update corresponding hap or hsp.", moduleName.c_str());
        return true;
    }
    return false;
}

void BMSEventHandler::InnerProcessRebootSharedBundleInstall(
    const std::list<std::string> &scanPathList, Constants::AppType appType)
{
    APP_LOGD("InnerProcessRebootSharedBundleInstall");
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }
    for (const auto &scanPath : scanPathList) {
        bool removable = IsPreInstallRemovable(scanPath);
        std::unordered_map<std::string, InnerBundleInfo> infos;
        if (!ParseHapFiles(scanPath, infos) || infos.empty()) {
            APP_LOGE("obtain bundleinfo failed : %{public}s ", scanPath.c_str());
            continue;
        }

        auto bundleName = infos.begin()->second.GetBundleName();
        auto versionCode = infos.begin()->second.GetVersionCode();
        AddParseInfosToMap(bundleName, infos);
        auto mapIter = loadExistData_.find(bundleName);
        if (mapIter == loadExistData_.end()) {
            APP_LOGD("OTA Install new shared bundle(%{public}s) by path(%{private}s).",
                bundleName.c_str(), scanPath.c_str());
            if (!OTAInstallSystemSharedBundle({scanPath}, appType, removable)) {
                APP_LOGE("OTA Install new shared bundle(%{public}s) error.", bundleName.c_str());
            }
            continue;
        }

        InnerBundleInfo oldBundleInfo;
        bool hasInstalled = dataMgr->FetchInnerBundleInfo(bundleName, oldBundleInfo);
        if (!hasInstalled) {
            APP_LOGW("app(%{public}s) has been uninstalled and do not OTA install.", bundleName.c_str());
            continue;
        }

        if (oldBundleInfo.GetVersionCode() > versionCode) {
            APP_LOGD("the installed version is up-to-date");
            continue;
        }
        if (oldBundleInfo.GetVersionCode() == versionCode) {
            if (!UpdateSharedAppByHash(oldBundleInfo, infos.begin()->second)) {
                APP_LOGD("the installed version is up-to-date");
                continue;
            }
        }

        if (!OTAInstallSystemSharedBundle({scanPath}, appType, removable)) {
            APP_LOGE("OTA update shared bundle(%{public}s) error.", bundleName.c_str());
        }
    }
}

bool BMSEventHandler::UpdateSharedAppByHash(const InnerBundleInfo &oldInfo, const InnerBundleInfo &newInfo) const
{
    auto oldSharedModuleMap = oldInfo.GetInnerSharedModuleInfos();
    auto newSharedModuleMap = newInfo.GetInnerSharedModuleInfos();
    for (const auto &item : newSharedModuleMap) {
        auto newModuleName = item.first;
        auto search = newSharedModuleMap.find(newModuleName);
        if (search == newSharedModuleMap.end()) {
            APP_LOGD("module(%{public}s) is not existed.", newModuleName.c_str());
            return false;
        }
        // todo
        auto oldBuildHash = oldSharedModuleMap[newModuleName][0].buildHash;
        auto newBuildHash = item.second[0].buildHash;
        if (oldBuildHash != newBuildHash) {
            return true;
        }
    }
    return false;
}

bool BMSEventHandler::IsHotPatchApp(const std::string &bundleName)
{
    InnerBundleInfo innerBundleInfo;
    if (!FetchInnerBundleInfo(bundleName, innerBundleInfo)) {
        APP_LOGE("can not get InnerBundleInfo, bundleName=%{public}s", bundleName.c_str());
        return false;
    }

    return innerBundleInfo.CheckSpecialMetaData(HOT_PATCH_METADATA);
}

bool BMSEventHandler::HotPatchAppProcessing(const std::string &bundleName)
{
    if (bundleName.empty()) {
        APP_LOGW("bundleName:%{public}s empty", bundleName.c_str());
        return false;
    }

    if (IsHotPatchApp(bundleName)) {
        APP_LOGI("get hotpatch meta-data success, bundleName=%{public}s", bundleName.c_str());
        SystemBundleInstaller installer;
        if (!installer.UninstallSystemBundle(bundleName, true)) {
            APP_LOGE("keep data to uninstall app(%{public}s) error", bundleName.c_str());
            return false;
        }
        return true;
    }
    return false;
}

void BMSEventHandler::SaveSystemFingerprint()
{
    auto bmsPara = DelayedSingleton<BundleMgrService>::GetInstance()->GetBmsParam();
    if (bmsPara == nullptr) {
        APP_LOGE("bmsPara is nullptr");
        return;
    }

    std::string curSystemFingerprint = GetCurSystemFingerprint();
    APP_LOGD("curSystemFingerprint(%{public}s)", curSystemFingerprint.c_str());
    if (curSystemFingerprint.empty()) {
        return;
    }

    bmsPara->SaveBmsParam(FINGERPRINT, curSystemFingerprint);
}

bool BMSEventHandler::IsSystemUpgrade()
{
    return IsTestSystemUpgrade() || IsSystemFingerprintChanged();
}

bool BMSEventHandler::IsTestSystemUpgrade()
{
    std::string paramValue;
    if (!GetSystemParameter(BMS_TEST_UPGRADE, paramValue) || paramValue.empty()) {
        return false;
    }

    APP_LOGD("TestSystemUpgrade value is %{public}s", paramValue.c_str());
    return paramValue == VALUE_TRUE;
}

bool BMSEventHandler::IsSystemFingerprintChanged()
{
    std::string oldSystemFingerprint = GetOldSystemFingerprint();
    if (oldSystemFingerprint.empty()) {
        APP_LOGD("System should be upgraded due to oldSystemFingerprint is empty");
        return true;
    }

    std::string curSystemFingerprint = GetCurSystemFingerprint();
    APP_LOGD("oldSystemFingerprint(%{public}s), curSystemFingerprint(%{public}s)",
        oldSystemFingerprint.c_str(), curSystemFingerprint.c_str());
    return curSystemFingerprint != oldSystemFingerprint;
}

std::string BMSEventHandler::GetCurSystemFingerprint()
{
    std::string curSystemFingerprint;
    for (const auto &item : FINGERPRINTS) {
        std::string itemFingerprint;
        if (!GetSystemParameter(item, itemFingerprint) || itemFingerprint.empty()) {
            continue;
        }

        if (!curSystemFingerprint.empty()) {
            curSystemFingerprint.append(Constants::PATH_SEPARATOR);
        }

        curSystemFingerprint.append(itemFingerprint);
    }

    return curSystemFingerprint;
}

bool BMSEventHandler::GetSystemParameter(const std::string &key, std::string &value)
{
    char firmware[VERSION_LEN] = {0};
    int32_t ret = GetParameter(key.c_str(), UNKNOWN.c_str(), firmware, VERSION_LEN);
    if (ret <= 0) {
        APP_LOGE("GetParameter failed!");
        return false;
    }

    value = firmware;
    return true;
}

std::string BMSEventHandler::GetOldSystemFingerprint()
{
    std::string oldSystemFingerprint;
    auto bmsPara = DelayedSingleton<BundleMgrService>::GetInstance()->GetBmsParam();
    if (bmsPara != nullptr) {
        bmsPara->GetBmsParam(FINGERPRINT, oldSystemFingerprint);
    }

    return oldSystemFingerprint;
}

void BMSEventHandler::AddParseInfosToMap(
    const std::string &bundleName, const std::unordered_map<std::string, InnerBundleInfo> &infos)
{
    auto hapParseInfoMapIter = hapParseInfoMap_.find(bundleName);
    if (hapParseInfoMapIter == hapParseInfoMap_.end()) {
        hapParseInfoMap_.emplace(bundleName, infos);
        return;
    }

    auto iterMap = hapParseInfoMapIter->second;
    for (auto infoIter : infos) {
        iterMap.emplace(infoIter.first, infoIter.second);
    }

    hapParseInfoMap_.at(bundleName) = iterMap;
}

void BMSEventHandler::ProcessRebootBundleUninstall()
{
    APP_LOGD("Reboot scan and OTA uninstall start");
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }

    for (auto &loadIter : loadExistData_) {
        std::string bundleName = loadIter.first;
        auto listIter = hapParseInfoMap_.find(bundleName);
        if (listIter == hapParseInfoMap_.end()) {
            APP_LOGD("OTA uninstall app(%{public}s).", bundleName.c_str());
            SystemBundleInstaller installer;
            if (!installer.UninstallSystemBundle(bundleName)) {
                APP_LOGE("OTA uninstall app(%{public}s) error", bundleName.c_str());
            } else {
                std::string moduleName;
                DeletePreInfoInDb(bundleName, moduleName, true);
            }

            continue;
        }

        BundleInfo hasInstalledInfo;
        auto hasBundleInstalled = dataMgr->GetBundleInfo(
            bundleName, BundleFlag::GET_BUNDLE_DEFAULT, hasInstalledInfo, Constants::ANY_USERID);
        if (!hasBundleInstalled) {
            APP_LOGW("app(%{public}s) maybe has been uninstall.", bundleName.c_str());
            continue;
        }

        // Check the installed module.
        // If the corresponding Hap does not exist, it should be uninstalled.
        for (auto moduleName : hasInstalledInfo.hapModuleNames) {
            bool hasModuleHapExist = false;
            for (auto parserInfoIter : listIter->second) {
                auto parserModuleNames = parserInfoIter.second.GetModuleNameVec();
                if (!parserModuleNames.empty() && moduleName == parserModuleNames[0]) {
                    hasModuleHapExist = true;
                    break;
                }
            }

            if (!hasModuleHapExist) {
                APP_LOGD("OTA app(%{public}s) uninstall module(%{public}s).",
                    bundleName.c_str(), moduleName.c_str());
                SystemBundleInstaller installer;
                if (!installer.UninstallSystemBundle(bundleName, moduleName)) {
                    APP_LOGE("OTA app(%{public}s) uninstall module(%{public}s) error.",
                        bundleName.c_str(), moduleName.c_str());
                }
            }
        }

        // Check the preInstall path in Db.
        // If the corresponding Hap does not exist, it should be deleted.
        auto parserInfoMap = listIter->second;
        for (auto preBundlePath : loadIter.second.GetBundlePaths()) {
            auto parserInfoIter = parserInfoMap.find(preBundlePath);
            if (parserInfoIter != parserInfoMap.end()) {
                APP_LOGD("OTA uninstall app(%{public}s) module path(%{private}s) exits.",
                    bundleName.c_str(), preBundlePath.c_str());
                continue;
            }

            APP_LOGD("OTA app(%{public}s) delete path(%{private}s).",
                bundleName.c_str(), preBundlePath.c_str());
            DeletePreInfoInDb(bundleName, preBundlePath, false);
        }
    }

    APP_LOGD("Reboot scan and OTA uninstall success");
}

void BMSEventHandler::DeletePreInfoInDb(
    const std::string &bundleName, const std::string &bundlePath, bool bundleLevel)
{
    APP_LOGD("DeletePreInfoInDb bundle(%{public}s)", bundleName.c_str());
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }

    PreInstallBundleInfo preInstallBundleInfo;
    preInstallBundleInfo.SetBundleName(bundleName);
    if (bundleLevel) {
        APP_LOGD("DeletePreInfoInDb bundle bundleLevel");
        dataMgr->DeletePreInstallBundleInfo(bundleName, preInstallBundleInfo);
        return;
    }

    APP_LOGD("DeletePreInfoInDb bundle not bundleLevel with path(%{private}s)",
        bundlePath.c_str());
    dataMgr->GetPreInstallBundleInfo(bundleName, preInstallBundleInfo);
    preInstallBundleInfo.DeleteBundlePath(bundlePath);
    if (preInstallBundleInfo.GetBundlePaths().empty()) {
        dataMgr->DeletePreInstallBundleInfo(bundleName, preInstallBundleInfo);
    } else {
        dataMgr->SavePreInstallBundleInfo(bundleName, preInstallBundleInfo);
    }
}

bool BMSEventHandler::HasModuleSavedInPreInstalledDb(
    const std::string &bundleName, const std::string &bundlePath)
{
    auto preInstallIter = loadExistData_.find(bundleName);
    if (preInstallIter == loadExistData_.end()) {
        APP_LOGE("app(%{public}s) does not save in PreInstalledDb.", bundleName.c_str());
        return false;
    }

    return preInstallIter->second.HasBundlePath(bundlePath);
}

bool BMSEventHandler::OTAInstallSystemBundle(
    const std::vector<std::string> &filePaths,
    Constants::AppType appType,
    bool removable)
{
    if (filePaths.empty()) {
        APP_LOGE("File path is empty");
        return false;
    }

    InstallParam installParam;
    installParam.isPreInstallApp = true;
    installParam.noSkipsKill = false;
    installParam.needSendEvent = false;
    installParam.installFlag = InstallFlag::REPLACE_EXISTING;
    installParam.removable = removable;
    installParam.needSavePreInstallInfo = true;
    installParam.copyHapToInstallPath = false;
    SystemBundleInstaller installer;
    return installer.OTAInstallSystemBundle(filePaths, installParam, appType);
}

bool BMSEventHandler::OTAInstallSystemSharedBundle(
    const std::vector<std::string> &filePaths,
    Constants::AppType appType,
    bool removable)
{
    if (filePaths.empty()) {
        APP_LOGE("File path is empty");
        return false;
    }

    InstallParam installParam;
    installParam.isPreInstallApp = true;
    installParam.needSendEvent = false;
    installParam.installFlag = InstallFlag::REPLACE_EXISTING;
    installParam.removable = removable;
    installParam.needSavePreInstallInfo = true;
    installParam.sharedBundleDirPaths = filePaths;
    SystemBundleInstaller installer;
    return installer.InstallSystemSharedBundle(installParam, true, appType);
}

bool BMSEventHandler::CheckAndParseHapFiles(
    const std::string &hapFilePath,
    bool isPreInstallApp,
    std::unordered_map<std::string, InnerBundleInfo> &infos)
{
    std::unique_ptr<BundleInstallChecker> bundleInstallChecker =
        std::make_unique<BundleInstallChecker>();
    std::vector<std::string> hapFilePathVec { hapFilePath };
    std::vector<std::string> realPaths;
    auto ret = BundleUtil::CheckFilePath(hapFilePathVec, realPaths);
    if (ret != ERR_OK) {
        APP_LOGE("File path %{public}s invalid", hapFilePath.c_str());
        return false;
    }

    ret = bundleInstallChecker->CheckSysCap(realPaths);
    if (ret != ERR_OK) {
        APP_LOGE("hap(%{public}s) syscap check failed", hapFilePath.c_str());
        return false;
    }

    std::vector<Security::Verify::HapVerifyResult> hapVerifyResults;
    ret = bundleInstallChecker->CheckMultipleHapsSignInfo(realPaths, hapVerifyResults);
    if (ret != ERR_OK) {
        APP_LOGE("CheckMultipleHapsSignInfo %{public}s failed", hapFilePath.c_str());
        return false;
    }

    InstallCheckParam checkParam;
    checkParam.isPreInstallApp = isPreInstallApp;
    if (isPreInstallApp) {
        checkParam.appType = Constants::AppType::SYSTEM_APP;
    }

    ret = bundleInstallChecker->ParseHapFiles(
        realPaths, checkParam, hapVerifyResults, infos);
    if (ret != ERR_OK) {
        APP_LOGE("parse haps file(%{public}s) failed", hapFilePath.c_str());
        return false;
    }

    ret = bundleInstallChecker->CheckAppLabelInfo(infos);
    if (ret != ERR_OK) {
        APP_LOGE("Check APP label failed %{public}d", ret);
        return false;
    }

    // set hapPath
    std::for_each(infos.begin(), infos.end(), [](auto &item) {
        item.second.SetModuleHapPath(item.first);
    });

    return true;
}

bool BMSEventHandler::ParseHapFiles(
    const std::string &hapFilePath,
    std::unordered_map<std::string, InnerBundleInfo> &infos)
{
    std::vector<std::string> hapFilePathVec { hapFilePath };
    std::vector<std::string> realPaths;
    auto ret = BundleUtil::CheckFilePath(hapFilePathVec, realPaths);
    if (ret != ERR_OK) {
        APP_LOGE("File path %{public}s invalid", hapFilePath.c_str());
        return false;
    }

    BundleParser bundleParser;
    for (auto realPath : realPaths) {
        InnerBundleInfo innerBundleInfo;
        ret = bundleParser.Parse(realPath, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGE("Parse bundle info failed, error: %{public}d", ret);
            continue;
        }

        infos.emplace(realPath, innerBundleInfo);
    }

    if (infos.empty()) {
        APP_LOGE("Parse hap(%{public}s) empty ", hapFilePath.c_str());
        return false;
    }

    return true;
}

bool BMSEventHandler::IsPreInstallRemovable(const std::string &path)
{
#ifdef USE_PRE_BUNDLE_PROFILE
    if (!HasPreInstallProfile()) {
        return false;
    }

    if (!hasLoadPreInstallProFile_) {
        APP_LOGE("Not load preInstall proFile or release.");
        return false;
    }

    if (path.empty() || installList_.empty()) {
        APP_LOGE("path or installList is empty.");
        return false;
    }
    auto installInfo = std::find_if(installList_.begin(), installList_.end(),
        [path](const auto &installInfo) {
        return installInfo.bundleDir == path;
    });
    if (installInfo != installList_.end()) {
        return (*installInfo).removable;
    }
    return true;
#else
    return false;
#endif
}

bool BMSEventHandler::GetPreInstallCapability(PreBundleConfigInfo &preBundleConfigInfo)
{
    if (!hasLoadPreInstallProFile_) {
        APP_LOGE("Not load preInstall proFile or release.");
        return false;
    }

    if (preBundleConfigInfo.bundleName.empty() || installListCapabilities_.empty()) {
        APP_LOGE("BundleName or installListCapabilities is empty.");
        return false;
    }

    auto iter = installListCapabilities_.find(preBundleConfigInfo);
    if (iter == installListCapabilities_.end()) {
        APP_LOGD("BundleName(%{public}s) no has preinstall capability.",
            preBundleConfigInfo.bundleName.c_str());
        return false;
    }

    preBundleConfigInfo = *iter;
    return true;
}

#ifdef USE_PRE_BUNDLE_PROFILE
void BMSEventHandler::UpdateRemovable(const std::string &bundleName, bool removable)
{
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }

    dataMgr->UpdateRemovable(bundleName, removable);
}

void BMSEventHandler::UpdateAllPrivilegeCapability()
{
    for (const auto &preBundleConfigInfo : installListCapabilities_) {
        UpdatePrivilegeCapability(preBundleConfigInfo);
    }
}

void BMSEventHandler::UpdatePrivilegeCapability(
    const PreBundleConfigInfo &preBundleConfigInfo)
{
    auto &bundleName = preBundleConfigInfo.bundleName;
    InnerBundleInfo innerBundleInfo;
    if (!FetchInnerBundleInfo(bundleName, innerBundleInfo)) {
        APP_LOGW("App(%{public}s) is not installed.", bundleName.c_str());
        return;
    }

    if (!MatchSignature(preBundleConfigInfo, innerBundleInfo.GetCertificateFingerprint())) {
        APP_LOGW("App(%{public}s) signature verify failed", bundleName.c_str());
        return;
    }

    UpdateTrustedPrivilegeCapability(preBundleConfigInfo);
}

bool BMSEventHandler::MatchSignature(
    const PreBundleConfigInfo &configInfo, const std::string &signature)
{
    if (configInfo.appSignature.empty()) {
        APP_LOGW("appSignature is empty");
        return false;
    }

    return std::find(configInfo.appSignature.begin(),
        configInfo.appSignature.end(), signature) != configInfo.appSignature.end();
}

void BMSEventHandler::UpdateTrustedPrivilegeCapability(
    const PreBundleConfigInfo &preBundleConfigInfo)
{
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }

    ApplicationInfo appInfo;
    appInfo.keepAlive = preBundleConfigInfo.keepAlive;
    appInfo.singleton = preBundleConfigInfo.singleton;
    appInfo.runningResourcesApply = preBundleConfigInfo.runningResourcesApply;
    appInfo.associatedWakeUp = preBundleConfigInfo.associatedWakeUp;
    for (const auto &event : preBundleConfigInfo.allowCommonEvent) {
        appInfo.allowCommonEvent.emplace_back(event);
    }

    dataMgr->UpdatePrivilegeCapability(preBundleConfigInfo.bundleName, appInfo);
}
#endif

bool BMSEventHandler::FetchInnerBundleInfo(
    const std::string &bundleName, InnerBundleInfo &innerBundleInfo)
{
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }

    return dataMgr->FetchInnerBundleInfo(bundleName, innerBundleInfo);
}

void BMSEventHandler::ListeningUserUnlocked() const
{
    APP_LOGI("BMSEventHandler listen the unlock of someone user start.");
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_UNLOCKED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    auto subscriberPtr = std::make_shared<UserUnlockedEventSubscriber>(subscribeInfo);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(subscriberPtr)) {
        APP_LOGW("BMSEventHandler subscribe common event %{public}s failed",
            EventFwk::CommonEventSupport::COMMON_EVENT_USER_UNLOCKED.c_str());
    }
}

void BMSEventHandler::AddStockAppProvisionInfoByOTA(const std::string &bundleName, const std::string &filePath)
{
    APP_LOGD("AddStockAppProvisionInfoByOTA bundleName: %{public}s", bundleName.c_str());
    // parse profile info
    Security::Verify::HapVerifyResult hapVerifyResult;
    auto ret = BundleVerifyMgr::ParseHapProfile(filePath, hapVerifyResult);
    if (ret != ERR_OK) {
        APP_LOGE("BundleVerifyMgr::HapVerify failed, bundleName: %{public}s, errCode: %{public}d",
            bundleName.c_str(), ret);
        return;
    }

    std::unique_ptr<BundleInstallChecker> bundleInstallChecker =
        std::make_unique<BundleInstallChecker>();
    AppProvisionInfo appProvisionInfo = bundleInstallChecker->ConvertToAppProvisionInfo(
        hapVerifyResult.GetProvisionInfo());
    if (!DelayedSingleton<AppProvisionInfoManager>::GetInstance()->AddAppProvisionInfo(bundleName, appProvisionInfo)) {
        APP_LOGE("AddAppProvisionInfo failed, bundleName:%{public}s", bundleName.c_str());
    }
}

void BMSEventHandler::UpdateAppDataSelinuxLabel(const std::string &bundleName, const std::string &apl,
    bool isPreInstall, bool debug)
{
    APP_LOGD("UpdateAppDataSelinuxLabel bundleName: %{public}s start.", bundleName.c_str());
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }
    std::set<int32_t> userIds = dataMgr->GetAllUser();
    for (const auto &userId : userIds) {
        for (const auto &el : Constants::BUNDLE_EL) {
            std::string baseBundleDataDir = Constants::BUNDLE_APP_DATA_BASE_DIR +
                                            el +
                                            Constants::PATH_SEPARATOR +
                                            std::to_string(userId);
            std::string baseDataDir = baseBundleDataDir + Constants::BASE + bundleName;
            bool isExist = true;
            ErrCode result = InstalldClient::GetInstance()->IsExistDir(baseDataDir, isExist);
            if (result != ERR_OK) {
                APP_LOGE("IsExistDir failed, error is %{public}d", result);
                continue;
            }
            if (!isExist) {
                // Update only accessible directories when OTA,
                // and other directories need to be set after the device is unlocked.
                // Can see UserUnlockedEventSubscriber::UpdateAppDataDirSelinuxLabel
                continue;
            }
            result = InstalldClient::GetInstance()->SetDirApl(baseDataDir, bundleName, apl, isPreInstall, debug);
            if (result != ERR_OK) {
                APP_LOGW("bundleName: %{public}s, fail to SetDirApl baseDataDir dir, error is %{public}d",
                    bundleName.c_str(), result);
            }
            std::string databaseDataDir = baseBundleDataDir + Constants::DATABASE + bundleName;
            result = InstalldClient::GetInstance()->SetDirApl(databaseDataDir, bundleName, apl, isPreInstall, debug);
            if (result != ERR_OK) {
                APP_LOGW("bundleName: %{public}s, fail to SetDirApl databaseDir dir, error is %{public}d",
                    bundleName.c_str(), result);
            }
        }
    }
    APP_LOGD("UpdateAppDataSelinuxLabel bundleName: %{public}s end.", bundleName.c_str());
}

void BMSEventHandler::HandleSceneBoard() const
{
#ifdef WINDOW_ENABLE
    APP_LOGD("begin to HandleSceneBoard");
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("dataMgr is null");
        return;
    }
    bool sceneBoardEnable = Rosen::SceneBoardJudgement::IsSceneBoardEnabled();
    APP_LOGD("sceneBoardEnable : %{public}d", sceneBoardEnable);
    dataMgr->SetApplicationEnabled(SCENE_BOARD_BUNDLE_NAME, sceneBoardEnable, Constants::DEFAULT_USERID);
    dataMgr->SetApplicationEnabled(SYSTEM_UI_BUNDLE_NAME, !sceneBoardEnable, Constants::DEFAULT_USERID);
    std::set<int32_t> userIds = dataMgr->GetAllUser();
    std::for_each(userIds.cbegin(), userIds.cend(), [dataMgr, sceneBoardEnable](const int32_t userId) {
        dataMgr->SetApplicationEnabled(LAUNCHER_BUNDLE_NAME, !sceneBoardEnable, userId);
    });
    APP_LOGD("HandleSceneBoard finish");
#endif
}

void BMSEventHandler::InnerProcessStockBundleProvisionInfo()
{
    APP_LOGD("InnerProcessStockBundleProvisionInfo start.");
    std::unordered_set<std::string> allBundleNames;
    if (!DelayedSingleton<AppProvisionInfoManager>::GetInstance()->GetAllAppProvisionInfoBundleName(allBundleNames)) {
        APP_LOGE("GetAllAppProvisionInfoBundleName failed");
        return;
    }
    // process normal bundle
    ProcessBundleProvisionInfo(allBundleNames);
    // process shared bundle
    ProcessSharedBundleProvisionInfo(allBundleNames);
    APP_LOGD("InnerProcessStockBundleProvisionInfo end.");
}

void BMSEventHandler::ProcessBundleProvisionInfo(const std::unordered_set<std::string> &allBundleNames)
{
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }
    std::vector<BundleInfo> bundleInfos;
    if (dataMgr->GetBundleInfosV9(static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE),
        bundleInfos, Constants::ALL_USERID) != ERR_OK) {
        APP_LOGE("GetBundleInfos failed");
        return;
    }
    for (const auto &bundleInfo : bundleInfos) {
        // not exist in appProvisionInfo table, then parse profile info and save it
        if ((allBundleNames.find(bundleInfo.name) == allBundleNames.end()) &&
            !bundleInfo.hapModuleInfos.empty()) {
            AddStockAppProvisionInfoByOTA(bundleInfo.name, bundleInfo.hapModuleInfos[0].hapPath);
        }
    }
}

void BMSEventHandler::ProcessSharedBundleProvisionInfo(const std::unordered_set<std::string> &allBundleNames)
{
    auto dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return;
    }
    std::vector<SharedBundleInfo> shareBundleInfos;
    if (dataMgr->GetAllSharedBundleInfo(shareBundleInfos) != ERR_OK) {
        APP_LOGE("GetAllSharedBundleInfo failed");
        return;
    }
    for (const auto &sharedBundleInfo : shareBundleInfos) {
        // not exist in appProvisionInfo table, then parse profile info and save it
        if ((allBundleNames.find(sharedBundleInfo.name) == allBundleNames.end()) &&
            !sharedBundleInfo.sharedModuleInfos.empty()) {
            std::string hspPath = Constants::BUNDLE_CODE_DIR + Constants::PATH_SEPARATOR + sharedBundleInfo.name +
                Constants::PATH_SEPARATOR + HSP_VERSION_PREFIX +
                std::to_string(sharedBundleInfo.sharedModuleInfos[0].versionCode) + Constants::PATH_SEPARATOR +
                sharedBundleInfo.sharedModuleInfos[0].name + Constants::PATH_SEPARATOR +
                sharedBundleInfo.sharedModuleInfos[0].name + Constants::INSTALL_SHARED_FILE_SUFFIX;
            AddStockAppProvisionInfoByOTA(sharedBundleInfo.name, hspPath);
        }
    }
}
}  // namespace AppExecFwk
}  // namespace OHOS
