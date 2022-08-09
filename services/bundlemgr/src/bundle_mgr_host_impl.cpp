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

#include "bundle_mgr_host_impl.h"

#include <dirent.h>
#include <future>
#include <string>

#include "app_log_wrapper.h"
#include "app_privilege_capability.h"
#include "bundle_mgr_service.h"
#include "bundle_parser.h"
#include "bundle_permission_mgr.h"
#include "bundle_sandbox_app_helper.h"
#include "bundle_util.h"
#include "directory_ex.h"
#include "distributed_bms_proxy.h"
#include "element_name.h"
#include "if_system_ability_manager.h"
#include "installd_client.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "json_serializer.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AppExecFwk {
bool BundleMgrHostImpl::GetApplicationInfo(
    const std::string &appName, const ApplicationFlag flag, const int userId, ApplicationInfo &appInfo)
{
    return GetApplicationInfo(appName, static_cast<int32_t>(flag), userId, appInfo);
}

bool BundleMgrHostImpl::GetApplicationInfo(
    const std::string &appName, int32_t flags, int32_t userId, ApplicationInfo &appInfo)
{
    APP_LOGD("start GetApplicationInfo, bundleName : %{public}s, flags : %{public}d, userId : %{public}d",
        appName.c_str(), flags, userId);
    if (!VerifyQueryPermission(appName)) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to GetApplicationInfo");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetApplicationInfo(appName, flags, userId, appInfo);
}

bool BundleMgrHostImpl::GetApplicationInfos(
    const ApplicationFlag flag, const int userId, std::vector<ApplicationInfo> &appInfos)
{
    return GetApplicationInfos(static_cast<int32_t>(flag), userId, appInfos);
}

bool BundleMgrHostImpl::GetApplicationInfos(
    int32_t flags, int32_t userId, std::vector<ApplicationInfo> &appInfos)
{
    APP_LOGD("start GetApplicationInfos, flags : %{public}d, userId : %{public}d", flags, userId);
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_GET_BUNDLE_INFO_PRIVILEGED)) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to GetApplicationInfos");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetApplicationInfos(flags, userId, appInfos);
}

bool BundleMgrHostImpl::GetBundleInfo(
    const std::string &bundleName, const BundleFlag flag, BundleInfo &bundleInfo, int32_t userId)
{
    return GetBundleInfo(bundleName, static_cast<int32_t>(flag), bundleInfo, userId);
}

bool BundleMgrHostImpl::GetBundleInfo(
    const std::string &bundleName, int32_t flags, BundleInfo &bundleInfo, int32_t userId)
{
    APP_LOGD("start GetBundleInfo, bundleName : %{public}s, flags : %{public}d, userId : %{public}d",
        bundleName.c_str(), flags, userId);
    if (!VerifyQueryPermission(bundleName)) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to GetBundleInfo");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetBundleInfo(bundleName, flags, bundleInfo, userId);
}

bool BundleMgrHostImpl::GetBundlePackInfo(
    const std::string &bundleName, const BundlePackFlag flag, BundlePackInfo &bundlePackInfo, int32_t userId)
{
    return GetBundlePackInfo(bundleName, static_cast<int32_t>(flag), bundlePackInfo, userId);
}

bool BundleMgrHostImpl::GetBundlePackInfo(
    const std::string &bundleName, int32_t flags, BundlePackInfo &bundlePackInfo, int32_t userId)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetBundlePackInfo(bundleName, flags, bundlePackInfo, userId);
}

bool BundleMgrHostImpl::GetBundleUserInfo(
    const std::string &bundleName, int32_t userId, InnerBundleUserInfo &innerBundleUserInfo)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetInnerBundleUserInfoByUserId(bundleName, userId, innerBundleUserInfo);
}

bool BundleMgrHostImpl::GetBundleUserInfos(
    const std::string &bundleName, std::vector<InnerBundleUserInfo> &innerBundleUserInfos)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetInnerBundleUserInfos(bundleName, innerBundleUserInfos);
}

bool BundleMgrHostImpl::GetBundleInfos(const BundleFlag flag, std::vector<BundleInfo> &bundleInfos, int32_t userId)
{
    return GetBundleInfos(static_cast<int32_t>(flag), bundleInfos, userId);
}

bool BundleMgrHostImpl::GetBundleInfos(int32_t flags, std::vector<BundleInfo> &bundleInfos, int32_t userId)
{
    APP_LOGD("start GetBundleInfos, flags : %{public}d, userId : %{public}d", flags, userId);
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_GET_BUNDLE_INFO_PRIVILEGED)) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to GetBundleInfos");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetBundleInfos(flags, bundleInfos, userId);
}

bool BundleMgrHostImpl::GetBundleNameForUid(const int uid, std::string &bundleName)
{
    APP_LOGD("start GetBundleNameForUid, uid : %{public}d", uid);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetBundleNameForUid(uid, bundleName);
}

bool BundleMgrHostImpl::GetBundlesForUid(const int uid, std::vector<std::string> &bundleNames)
{
    APP_LOGD("start GetBundlesForUid, uid : %{public}d", uid);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetBundlesForUid(uid, bundleNames);
}

bool BundleMgrHostImpl::GetNameForUid(const int uid, std::string &name)
{
    APP_LOGD("start GetNameForUid, uid : %{public}d", uid);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetNameForUid(uid, name);
}

bool BundleMgrHostImpl::GetBundleGids(const std::string &bundleName, std::vector<int> &gids)
{
    APP_LOGD("start GetBundleGids, bundleName : %{public}s", bundleName.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetBundleGids(bundleName, gids);
}

bool BundleMgrHostImpl::GetBundleGidsByUid(const std::string &bundleName, const int &uid, std::vector<int> &gids)
{
    APP_LOGD("start GetBundleGidsByUid, bundleName : %{public}s, uid : %{public}d", bundleName.c_str(), uid);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetBundleGidsByUid(bundleName, uid, gids);
}

bool BundleMgrHostImpl::CheckIsSystemAppByUid(const int uid)
{
    APP_LOGD("start CheckIsSystemAppByUid, uid : %{public}d", uid);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->CheckIsSystemAppByUid(uid);
}

bool BundleMgrHostImpl::GetBundleInfosByMetaData(const std::string &metaData, std::vector<BundleInfo> &bundleInfos)
{
    APP_LOGD("start GetBundleInfosByMetaData, metaData : %{public}s", metaData.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetBundleInfosByMetaData(metaData, bundleInfos);
}

bool BundleMgrHostImpl::QueryAbilityInfo(const Want &want, AbilityInfo &abilityInfo)
{
    return QueryAbilityInfo(want, GET_ABILITY_INFO_WITH_APPLICATION, Constants::UNSPECIFIED_USERID, abilityInfo);
}

#ifdef BUNDLE_FRAMEWORK_FREE_INSTALL
bool BundleMgrHostImpl::QueryAbilityInfo(const Want &want, int32_t flags, int32_t userId,
    AbilityInfo &abilityInfo, const sptr<IRemoteObject> &callBack)
{
    auto connectAbilityMgr = GetConnectAbilityMgrFromService();
    if (connectAbilityMgr == nullptr) {
        APP_LOGE("connectAbilityMgr is nullptr");
        return false;
    }
    return connectAbilityMgr->QueryAbilityInfo(want, flags, userId, abilityInfo, callBack);
}

void BundleMgrHostImpl::UpgradeAtomicService(const Want &want, int32_t userId)
{
    auto connectAbilityMgr = GetConnectAbilityMgrFromService();
    if (connectAbilityMgr == nullptr) {
        APP_LOGE("connectAbilityMgr is nullptr");
    }
    connectAbilityMgr->UpgradeAtomicService(want, userId);
}

bool BundleMgrHostImpl::CheckAbilityEnableInstall(
    const Want &want, int32_t missionId, int32_t userId, const sptr<IRemoteObject> &callback)
{
    auto elementName = want.GetElement();
    if (elementName.GetDeviceID().empty() || elementName.GetBundleName().empty() ||
        elementName.GetAbilityName().empty()) {
        APP_LOGE("check ability install parameter is invalid");
        return false;
    }
    auto bundleDistributedManager = DelayedSingleton<BundleMgrService>::GetInstance()->GetBundleDistributedManager();
    if (bundleDistributedManager == nullptr) {
        APP_LOGE("bundleDistributedManager failed");
        return false;
    }
    return bundleDistributedManager->CheckAbilityEnableInstall(want, missionId, userId, callback);
}
#endif

bool BundleMgrHostImpl::QueryAbilityInfo(const Want &want, int32_t flags, int32_t userId, AbilityInfo &abilityInfo)
{
    APP_LOGD("start QueryAbilityInfo, flags : %{public}d, userId : %{public}d", flags, userId);
    if (!VerifyQueryPermission(want.GetElement().GetBundleName())) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to QueryAbilityInfo");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->QueryAbilityInfo(want, flags, userId, abilityInfo);
}

bool BundleMgrHostImpl::QueryAbilityInfos(const Want &want, std::vector<AbilityInfo> &abilityInfos)
{
    return QueryAbilityInfos(
        want, GET_ABILITY_INFO_WITH_APPLICATION, Constants::UNSPECIFIED_USERID, abilityInfos);
}

bool BundleMgrHostImpl::QueryAbilityInfos(
    const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos)
{
    APP_LOGD("start QueryAbilityInfos, flags : %{public}d, userId : %{public}d", flags, userId);
    if (!VerifyQueryPermission(want.GetElement().GetBundleName())) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to QueryAbilityInfos");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->QueryAbilityInfos(want, flags, userId, abilityInfos);
}

bool BundleMgrHostImpl::QueryAllAbilityInfos(const Want &want, int32_t userId, std::vector<AbilityInfo> &abilityInfos)
{
    APP_LOGD("start QueryAllAbilityInfos, userId : %{public}d", userId);
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_GET_BUNDLE_INFO_PRIVILEGED)) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to QueryAllAbilityInfos");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->QueryLauncherAbilityInfos(want, userId, abilityInfos);
}

bool BundleMgrHostImpl::QueryAbilityInfoByUri(const std::string &abilityUri, AbilityInfo &abilityInfo)
{
    APP_LOGD("start QueryAbilityInfoByUri, uri : %{private}s", abilityUri.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->QueryAbilityInfoByUri(abilityUri, Constants::UNSPECIFIED_USERID, abilityInfo);
}

bool BundleMgrHostImpl::QueryAbilityInfosByUri(const std::string &abilityUri, std::vector<AbilityInfo> &abilityInfos)
{
    APP_LOGD("start QueryAbilityInfosByUri, uri : %{private}s", abilityUri.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->QueryAbilityInfosByUri(abilityUri, abilityInfos);
}

bool BundleMgrHostImpl::QueryAbilityInfoByUri(
    const std::string &abilityUri, int32_t userId, AbilityInfo &abilityInfo)
{
    APP_LOGD("start QueryAbilityInfoByUri, uri : %{private}s, userId : %{public}d", abilityUri.c_str(), userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->QueryAbilityInfoByUri(abilityUri, userId, abilityInfo);
}

bool BundleMgrHostImpl::QueryKeepAliveBundleInfos(std::vector<BundleInfo> &bundleInfos)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->QueryKeepAliveBundleInfos(bundleInfos);
}

std::string BundleMgrHostImpl::GetAbilityLabel(const std::string &bundleName, const std::string &abilityName)
{
    APP_LOGD("start GetAbilityLabel, bundleName : %{public}s, abilityName : %{public}s",
        bundleName.c_str(), abilityName.c_str());
    if (!VerifyQueryPermission(bundleName)) {
        APP_LOGE("verify permission failed");
        return Constants::EMPTY_STRING;
    }
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::EMPTY_STRING;
    }
    return dataMgr->GetAbilityLabel(bundleName, Constants::EMPTY_STRING, abilityName);
}

std::string BundleMgrHostImpl::GetAbilityLabel(const std::string &bundleName, const std::string &moduleName,
    const std::string &abilityName)
{
    APP_LOGD("start GetAbilityLabel, bundleName : %{public}s, moduleName : %{public}s, abilityName : %{public}s",
        bundleName.c_str(), moduleName.c_str(), abilityName.c_str());
    if (!VerifyQueryPermission(bundleName)) {
        APP_LOGE("verify permission failed");
        return Constants::EMPTY_STRING;
    }
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::EMPTY_STRING;
    }
    return dataMgr->GetAbilityLabel(bundleName, moduleName, abilityName);
}

bool BundleMgrHostImpl::GetBundleArchiveInfo(
    const std::string &hapFilePath, const BundleFlag flag, BundleInfo &bundleInfo)
{
    return GetBundleArchiveInfo(hapFilePath, static_cast<int32_t>(flag), bundleInfo);
}

bool BundleMgrHostImpl::GetBundleArchiveInfo(
    const std::string &hapFilePath, int32_t flags, BundleInfo &bundleInfo)
{
    APP_LOGD("start GetBundleArchiveInfo, hapFilePath : %{public}s, flags : %{public}d", hapFilePath.c_str(), flags);
    if (hapFilePath.find(Constants::SANDBOX_DATA_PATH) == std::string::npos) {
        std::string realPath;
        auto ret = BundleUtil::CheckFilePath(hapFilePath, realPath);
        if (ret != ERR_OK) {
            APP_LOGE("GetBundleArchiveInfo file path %{private}s invalid", hapFilePath.c_str());
            return false;
        }

        InnerBundleInfo info;
        BundleParser bundleParser;
        AppPrivilegeCapability appPrivilegeCapability;
        appPrivilegeCapability.allowMultiProcess = true;
        appPrivilegeCapability.allowUsePrivilegeExtension = true;
        ret = bundleParser.Parse(realPath, appPrivilegeCapability, info);
        if (ret != ERR_OK) {
            APP_LOGE("parse bundle info failed, error: %{public}d", ret);
            return false;
        }
        APP_LOGD("verify permission success, bgein to GetBundleArchiveInfo");
        info.GetBundleInfo(flags, bundleInfo, Constants::NOT_EXIST_USERID);
        return true;
    } else {
        return GetBundleArchiveInfoBySandBoxPath(hapFilePath, flags, bundleInfo);
    }
}

bool BundleMgrHostImpl::GetBundleArchiveInfoBySandBoxPath(const std::string &hapFilePath,
    int32_t flags, BundleInfo &bundleInfo)
{
    std::string bundleName;
    if (!ObtainCallingBundleName(bundleName)) {
        APP_LOGE("get calling bundleName failed");
        return false;
    }
    std::string hapRealPath;
    if (!BundleUtil::RevertToRealPath(hapFilePath, bundleName, hapRealPath)) {
        APP_LOGE("GetBundleArchiveInfo RevertToRealPath failed");
        return false;
    }
    std::string tempHapPath = Constants::BUNDLE_MANAGER_SERVICE_PATH +
        Constants::PATH_SEPARATOR + std::to_string(BundleUtil::GetCurrentTime());
    if (!BundleUtil::CreateDir(tempHapPath)) {
        APP_LOGE("GetBundleArchiveInfo make temp dir failed");
        return false;
    }
    std::string hapName = hapFilePath.substr(hapFilePath.find_last_of("//") + 1);
    std::string tempHapFile = tempHapPath + Constants::PATH_SEPARATOR + hapName;
    if (InstalldClient::GetInstance()->CopyFile(hapRealPath, tempHapFile) != ERR_OK) {
        APP_LOGE("GetBundleArchiveInfo copy hap file failed");
        return false;
    }
    std::string realPath;
    auto ret = BundleUtil::CheckFilePath(tempHapFile, realPath);
    if (ret != ERR_OK) {
        APP_LOGE("CheckFilePath failed");
        return false;
    }
    InnerBundleInfo info;
    BundleParser bundleParser;
    AppPrivilegeCapability appPrivilegeCapability;
    appPrivilegeCapability.allowMultiProcess = true;
    appPrivilegeCapability.allowUsePrivilegeExtension = true;
    ret = bundleParser.Parse(realPath, appPrivilegeCapability, info);
    if (ret != ERR_OK) {
        APP_LOGE("parse bundle info failed, error: %{public}d", ret);
        BundleUtil::DeleteDir(tempHapPath);
        return false;
    }
    BundleUtil::DeleteDir(tempHapPath);
    APP_LOGD("verify permission success, bgein to GetBundleArchiveInfo");
    info.GetBundleInfo(flags, bundleInfo, Constants::NOT_EXIST_USERID);
    return true;
}

bool BundleMgrHostImpl::GetHapModuleInfo(const AbilityInfo &abilityInfo, HapModuleInfo &hapModuleInfo)
{
    APP_LOGD("start GetHapModuleInfo");
    return GetHapModuleInfo(abilityInfo, Constants::UNSPECIFIED_USERID, hapModuleInfo);
}

bool BundleMgrHostImpl::GetHapModuleInfo(const AbilityInfo &abilityInfo, int32_t userId, HapModuleInfo &hapModuleInfo)
{
    APP_LOGD("start GetHapModuleInfo with userId: %{public}d", userId);
    if (abilityInfo.bundleName.empty() || abilityInfo.package.empty()) {
        APP_LOGE("fail to GetHapModuleInfo due to params empty");
        return false;
    }
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetHapModuleInfo(abilityInfo, hapModuleInfo, userId);
}

bool BundleMgrHostImpl::GetLaunchWantForBundle(const std::string &bundleName, Want &want)
{
    APP_LOGD("start GetLaunchWantForBundle, bundleName : %{public}s", bundleName.c_str());
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_GET_BUNDLE_INFO_PRIVILEGED)) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to GetLaunchWantForBundle");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetLaunchWantForBundle(bundleName, want);
}

int BundleMgrHostImpl::CheckPublicKeys(const std::string &firstBundleName, const std::string &secondBundleName)
{
    APP_LOGD("start CheckPublicKeys, firstBundleName : %{public}s, secondBundleName : %{public}s",
        firstBundleName.c_str(), secondBundleName.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->CheckPublicKeys(firstBundleName, secondBundleName);
}

bool BundleMgrHostImpl::GetPermissionDef(const std::string &permissionName, PermissionDef &permissionDef)
{
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_GET_BUNDLE_INFO_PRIVILEGED)) {
        APP_LOGE("verify GET_BUNDLE_INFO_PRIVILEGED failed");
        return false;
    }
    if (permissionName.empty()) {
        APP_LOGE("fail to GetPermissionDef due to params empty");
        return false;
    }
    return BundlePermissionMgr::GetPermissionDef(permissionName, permissionDef);
}

bool BundleMgrHostImpl::HasSystemCapability(const std::string &capName)
{
    return true;
}

bool BundleMgrHostImpl::GetSystemAvailableCapabilities(std::vector<std::string> &systemCaps)
{
    return true;
}

bool BundleMgrHostImpl::IsSafeMode()
{
    return true;
}

bool BundleMgrHostImpl::CleanBundleCacheFiles(
    const std::string &bundleName, const sptr<ICleanCacheCallback> &cleanCacheCallback,
    int32_t userId)
{
    if (userId == Constants::UNSPECIFIED_USERID) {
        userId = BundleUtil::GetUserIdByCallingUid();
    }

    APP_LOGD("start CleanBundleCacheFiles, bundleName : %{public}s, userId : %{public}d", bundleName.c_str(), userId);
    if (userId < 0) {
        APP_LOGE("userId is invalid");
        EventReport::SendCleanCacheSysEvent(bundleName, userId, true, true);
        return false;
    }

    if (bundleName.empty() || !cleanCacheCallback) {
        APP_LOGE("the cleanCacheCallback is nullptr or bundleName empty");
        EventReport::SendCleanCacheSysEvent(bundleName, userId, true, true);
        return false;
    }

    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_REMOVECACHEFILE)) {
        APP_LOGE("ohos.permission.REMOVE_CACHE_FILES permission denied");
        EventReport::SendCleanCacheSysEvent(bundleName, userId, true, true);
        return false;
    }

    ApplicationInfo applicationInfo;
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        EventReport::SendCleanCacheSysEvent(bundleName, userId, true, true);
        return false;
    }

    if (!dataMgr->GetApplicationInfo(bundleName, ApplicationFlag::GET_BASIC_APPLICATION_INFO,
        userId, applicationInfo)) {
        APP_LOGE("can not get application info of %{public}s", bundleName.c_str());
        EventReport::SendCleanCacheSysEvent(bundleName, userId, true, true);
        return false;
    }

    if (applicationInfo.isSystemApp && !applicationInfo.userDataClearable) {
        APP_LOGE("can not clean cacheFiles of %{public}s due to userDataClearable is false", bundleName.c_str());
        EventReport::SendCleanCacheSysEvent(bundleName, userId, true, true);
        return false;
    }

    CleanBundleCacheTask(bundleName, cleanCacheCallback, dataMgr, userId);
    return true;
}

void BundleMgrHostImpl::CleanBundleCacheTask(const std::string &bundleName,
    const sptr<ICleanCacheCallback> &cleanCacheCallback,
    const std::shared_ptr<BundleDataMgr> &dataMgr,
    int32_t userId)
{
    std::vector<std::string> rootDir;
    for (const auto &el : Constants::BUNDLE_EL) {
        std::string dataDir = Constants::BUNDLE_APP_DATA_BASE_DIR + el +
            Constants::PATH_SEPARATOR + std::to_string(userId) + Constants::BASE + bundleName;
        rootDir.emplace_back(dataDir);
    }

    auto cleanCache = [bundleName, userId, rootDir, dataMgr, cleanCacheCallback, this]() {
        std::vector<std::string> caches;
        for (const auto &st : rootDir) {
            std::vector<std::string> cache;
            if (InstalldClient::GetInstance()->GetBundleCachePath(st, cache) != ERR_OK) {
                APP_LOGW("GetBundleCachePath failed, path: %{public}s", st.c_str());
            }
            for (const auto &item : cache) {
                caches.emplace_back(item);
            }
        }

        bool error = false;
        if (!caches.empty()) {
            for (const auto& cache : caches) {
                error = InstalldClient::GetInstance()->CleanBundleDataDir(cache);
                if (error) {
                    break;
                }
            }
        }
        EventReport::SendCleanCacheSysEvent(bundleName, userId, true, error);
        APP_LOGD("CleanBundleCacheFiles with error %{public}d", error);
        cleanCacheCallback->OnCleanCacheFinished(error);
        InnerBundleUserInfo innerBundleUserInfo;
        if (!this->GetBundleUserInfo(bundleName, userId, innerBundleUserInfo)) {
            APP_LOGW("Get calling userInfo in bundle(%{public}s) failed", bundleName.c_str());
            return;
        }
        NotifyBundleEvents installRes = {
            .bundleName = bundleName,
            .resultCode = ERR_OK,
            .type = NotifyType::BUNDLE_CACHE_CLEARED,
            .uid = innerBundleUserInfo.uid,
            .accessTokenId = innerBundleUserInfo.accessTokenId
        };
        NotifyBundleStatus(installRes);
    };
    handler_->PostTask(cleanCache);
}

bool BundleMgrHostImpl::CleanBundleDataFiles(const std::string &bundleName, const int userId)
{
    APP_LOGD("start CleanBundleDataFiles, bundleName : %{public}s, userId : %{public}d", bundleName.c_str(), userId);
    if (bundleName.empty() || userId < 0) {
        APP_LOGE("the  bundleName empty or invalid userid");
        EventReport::SendCleanCacheSysEvent(bundleName, userId, false, true);
        return false;
    }

    ApplicationInfo applicationInfo;
    if (!GetApplicationInfo(bundleName, ApplicationFlag::GET_BASIC_APPLICATION_INFO, userId, applicationInfo)) {
        APP_LOGE("can not get application info of %{public}s", bundleName.c_str());
        EventReport::SendCleanCacheSysEvent(bundleName, userId, false, true);
        return false;
    }

    if (applicationInfo.isSystemApp && !applicationInfo.userDataClearable) {
        APP_LOGE("can not clean dataFiles of %{public}s due to userDataClearable is false", bundleName.c_str());
        EventReport::SendCleanCacheSysEvent(bundleName, userId, false, true);
        return false;
    }

    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetBundleUserInfo(bundleName, userId, innerBundleUserInfo)) {
        APP_LOGE("%{public}s, userId:%{public}d, GetBundleUserInfo failed", bundleName.c_str(), userId);
        EventReport::SendCleanCacheSysEvent(bundleName, userId, false, true);
        return false;
    }

    if (BundlePermissionMgr::ClearUserGrantedPermissionState(applicationInfo.accessTokenId)) {
        APP_LOGE("%{public}s, ClearUserGrantedPermissionState failed", bundleName.c_str());
        EventReport::SendCleanCacheSysEvent(bundleName, userId, false, true);
        return false;
    }

    if (InstalldClient::GetInstance()->RemoveBundleDataDir(bundleName, userId) != ERR_OK) {
        APP_LOGE("%{public}s, RemoveBundleDataDir failed", bundleName.c_str());
        EventReport::SendCleanCacheSysEvent(bundleName, userId, false, true);
        return false;
    }

    if (InstalldClient::GetInstance()->CreateBundleDataDir(bundleName, userId, innerBundleUserInfo.uid,
        innerBundleUserInfo.uid, GetAppPrivilegeLevel(bundleName))) {
        APP_LOGE("%{public}s, CreateBundleDataDir failed", bundleName.c_str());
        EventReport::SendCleanCacheSysEvent(bundleName, userId, false, true);
        return false;
    }

    EventReport::SendCleanCacheSysEvent(bundleName, userId, false, false);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        EventReport::SendCleanCacheSysEvent(bundleName, userId, true, true);
        return false;
    }
    NotifyBundleEvents installRes = {
        .bundleName = bundleName,
        .resultCode = ERR_OK,
        .type = NotifyType::BUNDLE_DATA_CLEARED,
        .uid = innerBundleUserInfo.uid,
        .accessTokenId = innerBundleUserInfo.accessTokenId
    };
    NotifyBundleStatus(installRes);
    return true;
}

bool BundleMgrHostImpl::RegisterBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback)
{
    APP_LOGD("start RegisterBundleStatusCallback");
    if ((!bundleStatusCallback) || (bundleStatusCallback->GetBundleName().empty())) {
        APP_LOGE("the bundleStatusCallback is nullptr or bundleName empty");
        return false;
    }
    // check permission
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::LISTEN_BUNDLE_CHANGE)) {
        APP_LOGE("register bundle status callback failed due to lack of permission");
        return false;
    }

    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->RegisterBundleStatusCallback(bundleStatusCallback);
}

bool BundleMgrHostImpl::ClearBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback)
{
    APP_LOGD("start ClearBundleStatusCallback");
    if (!bundleStatusCallback) {
        APP_LOGE("the bundleStatusCallback is nullptr");
        return false;
    }

    // check permission
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::LISTEN_BUNDLE_CHANGE)) {
        APP_LOGE("register bundle status callback failed due to lack of permission");
        return false;
    }

    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->ClearBundleStatusCallback(bundleStatusCallback);
}

bool BundleMgrHostImpl::UnregisterBundleStatusCallback()
{
    APP_LOGD("start UnregisterBundleStatusCallback");
    // check permission
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::LISTEN_BUNDLE_CHANGE)) {
        APP_LOGE("register bundle status callback failed due to lack of permission");
        return false;
    }

    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->UnregisterBundleStatusCallback();
}

bool BundleMgrHostImpl::DumpInfos(
    const DumpFlag flag, const std::string &bundleName, int32_t userId, std::string &result)
{
    bool ret = false;
    switch (flag) {
        case DumpFlag::DUMP_BUNDLE_LIST: {
            ret = DumpAllBundleInfoNames(userId, result);
            break;
        }
        case DumpFlag::DUMP_BUNDLE_INFO: {
            ret = DumpBundleInfo(bundleName, userId, result);
            break;
        }
        case DumpFlag::DUMP_SHORTCUT_INFO: {
            ret = DumpShortcutInfo(bundleName, userId, result);
            break;
        }
        default:
            APP_LOGE("dump flag error");
            return false;
    }
    return ret;
}

bool BundleMgrHostImpl::DumpAllBundleInfoNames(int32_t userId, std::string &result)
{
    APP_LOGD("DumpAllBundleInfoNames begin");
    if (userId != Constants::ALL_USERID) {
        return DumpAllBundleInfoNamesByUserId(userId, result);
    }

    auto userIds = GetExistsCommonUserIs();
    for (auto userId : userIds) {
        DumpAllBundleInfoNamesByUserId(userId, result);
    }

    APP_LOGD("DumpAllBundleInfoNames success");
    return true;
}

bool BundleMgrHostImpl::DumpAllBundleInfoNamesByUserId(int32_t userId, std::string &result)
{
    APP_LOGI("DumpAllBundleInfoNamesByUserId begin");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }

    std::vector<std::string> bundleNames;
    if (!dataMgr->GetBundleList(bundleNames, userId)) {
        APP_LOGE("get bundle list failed by userId(%{public}d)", userId);
        return false;
    }

    result.append("ID: ");
    result.append(std::to_string(userId));
    result.append(":\n");
    for (const auto &name : bundleNames) {
        result.append("\t");
        result.append(name);
        result.append("\n");
    }
    APP_LOGI("DumpAllBundleInfoNamesByUserId successfully");
    return true;
}

bool BundleMgrHostImpl::DumpBundleInfo(
    const std::string &bundleName, int32_t userId, std::string &result)
{
    APP_LOGD("DumpBundleInfo begin");
    std::vector<InnerBundleUserInfo> innerBundleUserInfos;
    if (userId == Constants::ALL_USERID) {
        if (!GetBundleUserInfos(bundleName, innerBundleUserInfos)) {
            APP_LOGE("get all userInfos in bundle(%{public}s) failed", bundleName.c_str());
            return false;
        }
        userId = innerBundleUserInfos.begin()->bundleUserInfo.userId;
    } else {
        InnerBundleUserInfo innerBundleUserInfo;
        if (!GetBundleUserInfo(bundleName, userId, innerBundleUserInfo)) {
            APP_LOGI("get userInfo in bundle(%{public}s) failed", bundleName.c_str());
        }
        innerBundleUserInfos.emplace_back(innerBundleUserInfo);
    }

    BundleInfo bundleInfo;
    if (!GetBundleInfo(bundleName,
        BundleFlag::GET_BUNDLE_WITH_ABILITIES |
        BundleFlag::GET_BUNDLE_WITH_REQUESTED_PERMISSION |
        BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO |
        BundleFlag::GET_BUNDLE_WITH_HASH_VALUE, bundleInfo, userId)) {
        APP_LOGE("get bundleInfo(%{public}s) failed", bundleName.c_str());
        return false;
    }

    result.append(bundleName);
    result.append(":\n");
    nlohmann::json jsonObject = bundleInfo;
    jsonObject["hapModuleInfos"] = bundleInfo.hapModuleInfos;
    jsonObject["userInfo"] = innerBundleUserInfos;
    result.append(jsonObject.dump(Constants::DUMP_INDENT));
    result.append("\n");
    APP_LOGI("DumpBundleInfo success with bundleName %{public}s", bundleName.c_str());
    return true;
}

bool BundleMgrHostImpl::DumpShortcutInfo(
    const std::string &bundleName, int32_t userId, std::string &result)
{
    APP_LOGD("DumpShortcutInfo begin");
    std::vector<ShortcutInfo> shortcutInfos;
    if (userId == Constants::ALL_USERID) {
        std::vector<InnerBundleUserInfo> innerBundleUserInfos;
        if (!GetBundleUserInfos(bundleName, innerBundleUserInfos)) {
            APP_LOGE("get all userInfos in bundle(%{public}s) failed", bundleName.c_str());
            return false;
        }
        userId = innerBundleUserInfos.begin()->bundleUserInfo.userId;
    }

    if (!GetShortcutInfos(bundleName, userId, shortcutInfos)) {
        APP_LOGE("get all shortcut info by bundle(%{public}s) failed", bundleName.c_str());
        return false;
    }

    result.append("shortcuts");
    result.append(":\n");
    for (const auto &info : shortcutInfos) {
        result.append("\"shortcut\"");
        result.append(":\n");
        nlohmann::json jsonObject = info;
        result.append(jsonObject.dump(Constants::DUMP_INDENT));
        result.append("\n");
    }
    APP_LOGD("DumpShortcutInfo success with bundleName %{public}s", bundleName.c_str());
    return true;
}

bool BundleMgrHostImpl::IsApplicationEnabled(const std::string &bundleName)
{
    APP_LOGD("start IsApplicationEnabled, bundleName : %{public}s", bundleName.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->IsApplicationEnabled(bundleName);
}

bool BundleMgrHostImpl::IsModuleRemovable(const std::string &bundleName, const std::string &moduleName)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->IsModuleRemovable(bundleName, moduleName);
}

bool BundleMgrHostImpl::SetModuleRemovable(const std::string &bundleName, const std::string &moduleName, bool isEnable)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->SetModuleRemovable(bundleName, moduleName, isEnable);
}

bool BundleMgrHostImpl::GetModuleUpgradeFlag(const std::string &bundleName, const std::string &moduleName)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetModuleUpgradeFlag(bundleName, moduleName);
}

bool BundleMgrHostImpl::SetModuleUpgradeFlag(const std::string &bundleName,
    const std::string &moduleName, int32_t upgradeFlag)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->SetModuleUpgradeFlag(bundleName, moduleName, upgradeFlag);
}

bool BundleMgrHostImpl::SetApplicationEnabled(const std::string &bundleName, bool isEnable, int32_t userId)
{
    APP_LOGD("SetApplicationEnabled begin");
    if (userId == Constants::UNSPECIFIED_USERID) {
        userId = BundleUtil::GetUserIdByCallingUid();
    }

    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_CHANGE_ABILITY_ENABLED_STATE)) {
        APP_LOGE("verify permission failed");
        EventReport::SendComponentStateSysEvent(bundleName, "", userId, isEnable, true);
        return false;
    }
    APP_LOGD("verify permission success, bgein to SetApplicationEnabled");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        EventReport::SendComponentStateSysEvent(bundleName, "", userId, isEnable, true);
        return false;
    }

    if (!dataMgr->SetApplicationEnabled(bundleName, isEnable, userId)) {
        APP_LOGE("Set application(%{public}s) enabled value faile.", bundleName.c_str());
        EventReport::SendComponentStateSysEvent(bundleName, "", userId, isEnable, true);
        return false;
    }

    EventReport::SendComponentStateSysEvent(bundleName, "", userId, isEnable, false);
    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetBundleUserInfo(bundleName, userId, innerBundleUserInfo)) {
        APP_LOGE("Get calling userInfo in bundle(%{public}s) failed", bundleName.c_str());
        return false;
    }

    NotifyBundleEvents installRes = {
        .bundleName = bundleName,
        .resultCode = ERR_OK,
        .type = NotifyType::APPLICATION_ENABLE,
        .uid = innerBundleUserInfo.uid,
        .accessTokenId = innerBundleUserInfo.accessTokenId
    };
    NotifyBundleStatus(installRes);
    APP_LOGD("SetApplicationEnabled finish");
    return true;
}

bool BundleMgrHostImpl::IsAbilityEnabled(const AbilityInfo &abilityInfo)
{
    APP_LOGD("start IsAbilityEnabled");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->IsAbilityEnabled(abilityInfo);
}

bool BundleMgrHostImpl::SetAbilityEnabled(const AbilityInfo &abilityInfo, bool isEnabled, int32_t userId)
{
    APP_LOGD("start SetAbilityEnabled");
    if (userId == Constants::UNSPECIFIED_USERID) {
        userId = BundleUtil::GetUserIdByCallingUid();
    }

    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_CHANGE_ABILITY_ENABLED_STATE)) {
        APP_LOGE("verify permission failed");
        EventReport::SendComponentStateSysEvent(
            abilityInfo.bundleName, abilityInfo.name, userId, isEnabled, true);
        return false;
    }

    APP_LOGD("verify permission success, bgein to SetAbilityEnabled");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        EventReport::SendComponentStateSysEvent(
            abilityInfo.bundleName, abilityInfo.name, userId, isEnabled, true);
        return false;
    }

    if (!dataMgr->SetAbilityEnabled(abilityInfo, isEnabled, userId)) {
        APP_LOGE("Set ability(%{public}s) enabled value failed.", abilityInfo.bundleName.c_str());
        EventReport::SendComponentStateSysEvent(
            abilityInfo.bundleName, abilityInfo.name, userId, isEnabled, true);
        return false;
    }

    EventReport::SendComponentStateSysEvent(
        abilityInfo.bundleName, abilityInfo.name, userId, isEnabled, false);
    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetBundleUserInfo(abilityInfo.bundleName, userId, innerBundleUserInfo)) {
        APP_LOGE("Get calling userInfo in bundle(%{public}s) failed", abilityInfo.bundleName.c_str());
        return false;
    }

    NotifyBundleEvents installRes = {
        .bundleName = abilityInfo.bundleName,
        .abilityName = abilityInfo.name,
        .resultCode = ERR_OK,
        .type = NotifyType::APPLICATION_ENABLE,
        .uid = innerBundleUserInfo.uid,
        .accessTokenId = innerBundleUserInfo.accessTokenId,
    };
    NotifyBundleStatus(installRes);
    return true;
}

sptr<IBundleInstaller> BundleMgrHostImpl::GetBundleInstaller()
{
    APP_LOGD("start GetBundleInstaller");
    return DelayedSingleton<BundleMgrService>::GetInstance()->GetBundleInstaller();
}

sptr<IBundleUserMgr> BundleMgrHostImpl::GetBundleUserMgr()
{
    return DelayedSingleton<BundleMgrService>::GetInstance()->GetBundleUserMgr();
}

bool BundleMgrHostImpl::GetAllFormsInfo(std::vector<FormInfo> &formInfos)
{
    APP_LOGD("start GetAllFormsInfo");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetAllFormsInfo(formInfos);
}

bool BundleMgrHostImpl::GetFormsInfoByApp(const std::string &bundleName, std::vector<FormInfo> &formInfos)
{
    APP_LOGD("start GetFormsInfoByApp, bundleName : %{public}s", bundleName.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetFormsInfoByApp(bundleName, formInfos);
}

bool BundleMgrHostImpl::GetFormsInfoByModule(
    const std::string &bundleName, const std::string &moduleName, std::vector<FormInfo> &formInfos)
{
    APP_LOGD("start GetFormsInfoByModule, bundleName : %{public}s, moduleName : %{public}s",
        bundleName.c_str(), moduleName.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetFormsInfoByModule(bundleName, moduleName, formInfos);
}

bool BundleMgrHostImpl::GetShortcutInfos(
    const std::string &bundleName, std::vector<ShortcutInfo> &shortcutInfos)
{
    return GetShortcutInfos(bundleName, Constants::UNSPECIFIED_USERID, shortcutInfos);
}

bool BundleMgrHostImpl::GetShortcutInfos(
    const std::string &bundleName, int32_t userId, std::vector<ShortcutInfo> &shortcutInfos)
{
    APP_LOGD("start GetShortcutInfos, bundleName : %{public}s, userId : %{public}d", bundleName.c_str(), userId);
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_GET_BUNDLE_INFO_PRIVILEGED)) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("verify permission success, bgein to GetShortcutInfos");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetShortcutInfos(bundleName, userId, shortcutInfos);
}

bool BundleMgrHostImpl::GetAllCommonEventInfo(const std::string &eventKey,
    std::vector<CommonEventInfo> &commonEventInfos)
{
    APP_LOGD("start GetAllCommonEventInfo, eventKey : %{public}s", eventKey.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetAllCommonEventInfo(eventKey, commonEventInfos);
}

bool BundleMgrHostImpl::GetDistributedBundleInfo(const std::string &networkId, const std::string &bundleName,
    DistributedBundleInfo &distributedBundleInfo)
{
    APP_LOGD("start GetDistributedBundleInfo, networkId : %{public}s, bundleName : %{public}s",
        networkId.c_str(), bundleName.c_str());
    auto distributedBundleMgr = GetDistributedBundleMgrService();
    if (distributedBundleMgr == nullptr) {
        APP_LOGE("DistributedBundleMgrService is nullptr");
        return false;
    }
    return distributedBundleMgr->GetDistributedBundleInfo(networkId, bundleName, distributedBundleInfo);
}

bool BundleMgrHostImpl::QueryExtensionAbilityInfos(const Want &want, const int32_t &flag, const int32_t &userId,
    std::vector<ExtensionAbilityInfo> &extensionInfos)
{
    APP_LOGD("QueryExtensionAbilityInfos without type begin");
    if (!VerifyQueryPermission(want.GetElement().GetBundleName())) {
        APP_LOGE("verify permission failed");
        return false;
    }
    APP_LOGD("want uri is %{private}s", want.GetUriString().c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    bool ret = dataMgr->QueryExtensionAbilityInfos(want, flag, userId, extensionInfos);
    if (!ret) {
        APP_LOGE("QueryExtensionAbilityInfos is failed");
        return false;
    }
    if (extensionInfos.empty()) {
        APP_LOGE("no valid extension info can be inquired");
        return false;
    }
    return true;
}

bool BundleMgrHostImpl::QueryExtensionAbilityInfos(const Want &want, const ExtensionAbilityType &extensionType,
    const int32_t &flag, const int32_t &userId, std::vector<ExtensionAbilityInfo> &extensionInfos)
{
    APP_LOGD("QueryExtensionAbilityInfos begin");
    if (!VerifyQueryPermission(want.GetElement().GetBundleName())) {
        APP_LOGE("verify permission failed");
        return false;
    }
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    std::vector<ExtensionAbilityInfo> infos;
    bool ret = dataMgr->QueryExtensionAbilityInfos(want, flag, userId, infos);
    if (!ret) {
        APP_LOGE("QueryExtensionAbilityInfos is failed");
        return false;
    }
    for_each(infos.begin(), infos.end(), [&extensionType, &extensionInfos](const auto &info)->decltype(auto) {
        APP_LOGD("QueryExtensionAbilityInfos extensionType is %{public}d, info.type is %{public}d",
            static_cast<int32_t>(extensionType), static_cast<int32_t>(info.type));
        if (extensionType == info.type) {
            extensionInfos.emplace_back(info);
        }
    });
    if (extensionInfos.empty()) {
        APP_LOGE("no valid extension info can be inquired");
        return false;
    }
    return true;
}

bool BundleMgrHostImpl::QueryExtensionAbilityInfos(const ExtensionAbilityType &extensionType, const int32_t &userId,
    std::vector<ExtensionAbilityInfo> &extensionInfos)
{
    APP_LOGD("QueryExtensionAbilityInfos with type begin");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    bool ret = dataMgr->QueryExtensionAbilityInfos(extensionType, userId, extensionInfos);
    if (!ret) {
        APP_LOGE("QueryExtensionAbilityInfos is failed");
        return false;
    }

    if (extensionInfos.empty()) {
        APP_LOGE("no valid extension info can be inquired");
        return false;
    }
    return true;
}

const std::shared_ptr<BundleDataMgr> BundleMgrHostImpl::GetDataMgrFromService()
{
    return DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
}

const OHOS::sptr<IDistributedBms> BundleMgrHostImpl::GetDistributedBundleMgrService()
{
    auto saMgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saMgr == nullptr) {
        APP_LOGE("saMgr is nullptr");
        return nullptr;
    }
    OHOS::sptr<OHOS::IRemoteObject> remoteObject =
        saMgr->CheckSystemAbility(OHOS::DISTRIBUTED_BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    return OHOS::iface_cast<IDistributedBms>(remoteObject);
}

#ifdef BUNDLE_FRAMEWORK_FREE_INSTALL
const std::shared_ptr<BundleConnectAbilityMgr> BundleMgrHostImpl::GetConnectAbilityMgrFromService()
{
    return DelayedSingleton<BundleMgrService>::GetInstance()->GetConnectAbility();
}
#endif

std::set<int32_t> BundleMgrHostImpl::GetExistsCommonUserIs()
{
    std::set<int32_t> userIds;
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("Get dataMgr shared_ptr nullptr");
        return userIds;
    }

    for (auto userId : dataMgr->GetAllUser()) {
        if (userId >= Constants::START_USERID) {
            userIds.insert(userId);
        }
    }
    return userIds;
}

bool BundleMgrHostImpl::VerifyQueryPermission(const std::string &queryBundleName)
{
    std::string callingBundleName;
    bool ret = GetBundleNameForUid(IPCSkeleton::GetCallingUid(), callingBundleName);
    APP_LOGD("callingBundleName : %{public}s", callingBundleName.c_str());
    if (ret && (queryBundleName == callingBundleName)) {
        APP_LOGD("query own info, verify success");
        return true;
    }
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_GET_BUNDLE_INFO) &&
        !BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_GET_BUNDLE_INFO_PRIVILEGED)) {
        APP_LOGE("verify query permission failed");
        return false;
    }
    APP_LOGD("verify query permission successfully");
    return true;
}

std::string BundleMgrHostImpl::GetAppPrivilegeLevel(const std::string &bundleName, int32_t userId)
{
    APP_LOGD("start GetAppPrivilegeLevel");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::EMPTY_STRING;
    }
    return dataMgr->GetAppPrivilegeLevel(bundleName, userId);
}

bool BundleMgrHostImpl::VerifyCallingPermission(const std::string &permission)
{
    APP_LOGD("VerifyCallingPermission begin");
    return BundlePermissionMgr::VerifyCallingPermission(permission);
}

std::vector<std::string> BundleMgrHostImpl::GetAccessibleAppCodePaths(int32_t userId)
{
    APP_LOGD("GetAccessibleAppCodePaths begin");
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        std::vector<std::string> vec;
        return vec;
    }

    return dataMgr->GetAccessibleAppCodePaths(userId);
}

bool BundleMgrHostImpl::QueryExtensionAbilityInfoByUri(const std::string &uri, int32_t userId,
    ExtensionAbilityInfo &extensionAbilityInfo)
{
    APP_LOGD("uri : %{private}s, userId : %{public}d", uri.c_str(), userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->QueryExtensionAbilityInfoByUri(uri, userId, extensionAbilityInfo);
}

std::string BundleMgrHostImpl::GetAppIdByBundleName(const std::string &bundleName, const int userId)
{
    APP_LOGD("bundleName : %{public}s, userId : %{public}d", bundleName.c_str(), userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::EMPTY_STRING;
    }
    BundleInfo bundleInfo;
    bool ret = dataMgr->GetBundleInfo(bundleName, GET_BUNDLE_DEFAULT, bundleInfo, userId);
    if (!ret) {
        APP_LOGE("get bundleInfo failed");
        return Constants::EMPTY_STRING;
    }
    APP_LOGD("appId is %{private}s", bundleInfo.appId.c_str());
    return bundleInfo.appId;
}

std::string BundleMgrHostImpl::GetAppType(const std::string &bundleName)
{
    APP_LOGD("bundleName : %{public}s", bundleName.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::EMPTY_STRING;
    }
    BundleInfo bundleInfo;
    bool ret = dataMgr->GetBundleInfo(bundleName, GET_BUNDLE_DEFAULT, bundleInfo, Constants::UNSPECIFIED_USERID);
    if (!ret) {
        APP_LOGE("get bundleInfo failed");
        return Constants::EMPTY_STRING;
    }
    bool isSystemApp = bundleInfo.applicationInfo.isSystemApp;
    std::string appType = isSystemApp ? Constants::SYSTEM_APP : Constants::THIRD_PARTY_APP;
    APP_LOGD("appType is %{public}s", appType.c_str());
    return appType;
}

int BundleMgrHostImpl::GetUidByBundleName(const std::string &bundleName, const int userId)
{
    APP_LOGD("bundleName : %{public}s, userId : %{public}d", bundleName.c_str(), userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::INVALID_UID;
    }
    std::vector<BundleInfo> bundleInfos;
    int32_t uid = Constants::INVALID_UID;
    bool ret = dataMgr->GetBundleInfos(GET_BUNDLE_DEFAULT, bundleInfos, userId);
    if (ret) {
        for (auto bundleInfo : bundleInfos) {
            if (bundleInfo.name == bundleName) {
                uid = bundleInfo.uid;
                break;
            }
        }
        APP_LOGD("get bundle uid success");
    } else {
        APP_LOGE("can not get bundleInfo's uid");
    }
    APP_LOGD("uid is %{public}d", uid);
    return uid;
}

bool BundleMgrHostImpl::GetAbilityInfo(
    const std::string &bundleName, const std::string &abilityName, AbilityInfo &abilityInfo)
{
    APP_LOGD("start GetAbilityInfo, bundleName : %{public}s, abilityName : %{public}s",
        bundleName.c_str(), abilityName.c_str());
    ElementName elementName("", bundleName, abilityName);
    Want want;
    want.SetElement(elementName);
    return QueryAbilityInfo(want, abilityInfo);
}

bool BundleMgrHostImpl::GetAbilityInfo(
    const std::string &bundleName, const std::string &moduleName,
    const std::string &abilityName, AbilityInfo &abilityInfo)
{
    APP_LOGD("start GetAbilityInfo, bundleName : %{public}s, moduleName : %{public}s, abilityName : %{public}s",
        bundleName.c_str(), moduleName.c_str(), abilityName.c_str());
    ElementName elementName("", bundleName, abilityName, moduleName);
    Want want;
    want.SetElement(elementName);
    return QueryAbilityInfo(want, abilityInfo);
}

bool BundleMgrHostImpl::ImplicitQueryInfoByPriority(const Want &want, int32_t flags, int32_t userId,
    AbilityInfo &abilityInfo, ExtensionAbilityInfo &extensionInfo)
{
    APP_LOGD("start ImplicitQueryInfoByPriority, flags : %{public}d, userId : %{public}d", flags, userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->ImplicitQueryInfoByPriority(want, flags, userId, abilityInfo, extensionInfo);
}

bool BundleMgrHostImpl::ImplicitQueryInfos(const Want &want, int32_t flags, int32_t userId,
    std::vector<AbilityInfo> &abilityInfos, std::vector<ExtensionAbilityInfo> &extensionInfos)
{
    APP_LOGD("begin to ImplicitQueryInfos, flags : %{public}d, userId : %{public}d", flags, userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->ImplicitQueryInfos(want, flags, userId, abilityInfos, extensionInfos);
}

int BundleMgrHostImpl::Dump(int fd, const std::vector<std::u16string> &args)
{
    std::string result;
    std::vector<std::string> argsStr;
    for (auto item : args) {
        argsStr.emplace_back(Str16ToStr8(item));
    }

    if (!DelayedSingleton<BundleMgrService>::GetInstance()->Hidump(argsStr, result)) {
        APP_LOGE("Hidump error");
        return ERR_APPEXECFWK_HIDUMP_ERROR;
    }

    int ret = dprintf(fd, "%s\n", result.c_str());
    if (ret < 0) {
        APP_LOGE("dprintf error");
        return ERR_APPEXECFWK_HIDUMP_ERROR;
    }

    return ERR_OK;
}

bool BundleMgrHostImpl::GetAllDependentModuleNames(const std::string &bundleName, const std::string &moduleName,
    std::vector<std::string> &dependentModuleNames)
{
    APP_LOGD("GetAllDependentModuleNames: bundleName: %{public}s, moduleName: %{public}s",
        bundleName.c_str(), moduleName.c_str());
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->GetAllDependentModuleNames(bundleName, moduleName, dependentModuleNames);
}

ErrCode BundleMgrHostImpl::GetSandboxBundleInfo(
    const std::string &bundleName, int32_t appIndex, int32_t userId, BundleInfo &info)
{
    APP_LOGD("start GetSandboxBundleInfo, bundleName : %{public}s, appindex : %{public}d, userId : %{public}d",
        bundleName.c_str(), appIndex, userId);
    // check bundle name
    if (bundleName.empty()) {
        APP_LOGE("GetSandboxBundleInfo failed due to empty bundleName");
        return ERR_APPEXECFWK_SANDBOX_INSTALL_PARAM_ERROR;
    }
    // check appIndex
    if (appIndex <= Constants::INITIAL_APP_INDEX || appIndex > Constants::MAX_APP_INDEX) {
        APP_LOGE("the appIndex %{public}d is invalid", appIndex);
        return ERR_APPEXECFWK_SANDBOX_INSTALL_PARAM_ERROR;
    }
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return ERR_APPEXECFWK_SANDBOX_INSTALL_INTERNAL_ERROR;
    }
    auto sandboxAppHelper = dataMgr->GetSandboxAppHelper();
    if (sandboxAppHelper == nullptr) {
        APP_LOGE("sandboxAppHelper is nullptr");
        return ERR_APPEXECFWK_SANDBOX_INSTALL_INTERNAL_ERROR;
    }
    int32_t requestUserId = dataMgr->GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_APPEXECFWK_SANDBOX_QUERY_INVALID_USER_ID;
    }
    return sandboxAppHelper->GetSandboxAppBundleInfo(bundleName, appIndex, requestUserId, info);
}

bool BundleMgrHostImpl::SetDisposedStatus(const std::string &bundleName, int32_t status)
{
    APP_LOGD("SetDisposedStatus: bundleName: %{public}s, status: %{public}d", bundleName.c_str(), status);
    // check permission
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_MANAGE_DISPOSED_APP_STATUS)) {
        APP_LOGE("SetDisposedStatus bundleName: %{public}s failed due to lack of permission", bundleName.c_str());
        return false;
    }
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return false;
    }
    return dataMgr->SetDisposedStatus(bundleName, status);
}

int32_t BundleMgrHostImpl::GetDisposedStatus(const std::string &bundleName)
{
    APP_LOGD("GetDisposedStatus: bundleName: %{public}s", bundleName.c_str());
    // check permission
    if (!BundlePermissionMgr::VerifyCallingPermission(Constants::PERMISSION_MANAGE_DISPOSED_APP_STATUS)) {
        APP_LOGE("GetDisposedStatus bundleName: %{public}s failed due to lack of permission", bundleName.c_str());
        return Constants::DEFAULT_DISPOSED_STATUS;
    }
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::DEFAULT_DISPOSED_STATUS;
    }
    return dataMgr->GetDisposedStatus(bundleName);
}

bool BundleMgrHostImpl::ObtainCallingBundleName(std::string &bundleName)
{
    bool ret = GetBundleNameForUid(IPCSkeleton::GetCallingUid(), bundleName);
    if (!ret) {
        APP_LOGE("query calling bundle name failed");
        return false;
    }
    APP_LOGD("calling bundleName is : %{public}s", bundleName.c_str());
    return ret;
}

bool BundleMgrHostImpl::GetBundleStats(const std::string &bundleName, int32_t userId,
    std::vector<int64_t> &bundleStats)
{
    if (InstalldClient::GetInstance()->GetBundleStats(bundleName, userId, bundleStats) != ERR_OK) {
        APP_LOGE("GetBundleStats: bundleName: %{public}s failed", bundleName.c_str());
        return false;
    }
    return true;
}

std::string BundleMgrHostImpl::GetStringById(
    const std::string &bundleName, const std::string &moduleName, uint32_t resId, int32_t userId)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::EMPTY_STRING;
    }
    return dataMgr->GetStringById(bundleName, moduleName, resId, userId);
}

std::string BundleMgrHostImpl::GetIconById(
    const std::string &bundleName, const std::string &moduleName, uint32_t resId, uint32_t density, int32_t userId)
{
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return Constants::EMPTY_STRING;
    }
    return dataMgr->GetIconById(bundleName, moduleName, resId, density, userId);
}

int32_t BundleMgrHostImpl::GetUdidByNetworkId(const std::string &networkId, std::string &udid)
{
#ifdef DEVICE_MANAGER_ENABLE
    auto deviceManager = DelayedSingleton<BundleMgrService>::GetInstance()->GetDeviceManager();
    if (deviceManager == nullptr) {
        APP_LOGE("deviceManager is nullptr");
        return -1;
    }
    return deviceManager->GetUdidByNetworkId(networkId, udid);
#else
    APP_LOGW("deviceManager is unable");
    return -1;
#endif
}

#ifdef BUNDLE_FRAMEWORK_DEFAULT_APP
sptr<IDefaultApp> BundleMgrHostImpl::GetDefaultAppProxy()
{
    return DelayedSingleton<BundleMgrService>::GetInstance()->GetDefaultAppProxy();
}
#endif

sptr<IQuickFixManager> BundleMgrHostImpl::GetQuickFixManagerProxy()
{
#ifdef BUNDLE_FRAMEWORK_QUICK_FIX
    return DelayedSingleton<BundleMgrService>::GetInstance()->GetQuickFixManagerProxy();
#else
    return nullptr;
#endif
}

ErrCode BundleMgrHostImpl::GetSandboxAbilityInfo(const Want &want, int32_t appIndex, int32_t flags, int32_t userId,
    AbilityInfo &info)
{
    APP_LOGD("start GetSandboxAbilityInfo appIndex : %{public}d, userId : %{public}d", appIndex, userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return ERR_APPEXECFWK_SANDBOX_QUERY_INTERNAL_ERROR;
    }

    if (!dataMgr->QueryAbilityInfo(want, flags, userId, info, appIndex)) {
        APP_LOGE("query ability info failed");
        return ERR_APPEXECFWK_SANDBOX_QUERY_INTERNAL_ERROR;
    }
    return ERR_OK;
}

ErrCode BundleMgrHostImpl::GetSandboxExtAbilityInfos(const Want &want, int32_t appIndex, int32_t flags,
    int32_t userId, std::vector<ExtensionAbilityInfo> &infos)
{
    APP_LOGD("start GetSandboxExtAbilityInfos appIndex : %{public}d, userId : %{public}d", appIndex, userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return ERR_APPEXECFWK_SANDBOX_QUERY_INTERNAL_ERROR;
    }

    if (!dataMgr->QueryExtensionAbilityInfos(want, flags, userId, infos, appIndex)) {
        APP_LOGE("query extension ability info failed");
        return ERR_APPEXECFWK_SANDBOX_QUERY_INTERNAL_ERROR;
    }
    return ERR_OK;
}

ErrCode BundleMgrHostImpl::GetSandboxHapModuleInfo(const AbilityInfo &abilityInfo, int32_t appIndex, int32_t userId,
    HapModuleInfo &info)
{
    APP_LOGD("start GetSandboxHapModuleInfo appIndex : %{public}d, userId : %{public}d", appIndex, userId);
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return ERR_APPEXECFWK_SANDBOX_QUERY_INTERNAL_ERROR;
    }
    auto sandboxAppHelper = dataMgr->GetSandboxAppHelper();
    if (sandboxAppHelper == nullptr) {
        APP_LOGE("sandboxAppHelper is nullptr");
        return ERR_APPEXECFWK_SANDBOX_QUERY_INTERNAL_ERROR;
    }
    int32_t requestUserId = dataMgr->GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_APPEXECFWK_SANDBOX_QUERY_INVALID_USER_ID;
    }
    return sandboxAppHelper->GetSandboxHapModuleInfo(abilityInfo, appIndex, requestUserId, info);
}

int32_t BundleMgrHostImpl::GetMediaFileDescriptor(const std::string &bundleName, const std::string &moduleName,
    const std::string &abilityName)
{
    APP_LOGI("start to get file fd");
    int32_t fd = -1;
    if (!VerifyQueryPermission(bundleName)) {
        APP_LOGE("verify permission failed");
        return fd;
    }
    auto dataMgr = GetDataMgrFromService();
    if (dataMgr == nullptr) {
        APP_LOGE("DataMgr is nullptr");
        return fd;
    }
    fd = dataMgr->GetMediaFileDescriptor(bundleName, moduleName, abilityName);
    return fd;
}

void BundleMgrHostImpl::NotifyBundleStatus(const NotifyBundleEvents &installRes)
{
    std::shared_ptr<BundleCommonEventMgr> commonEventMgr = std::make_shared<BundleCommonEventMgr>();
    if (commonEventMgr == nullptr) {
        APP_LOGE("commonEventMgr is nullptr");
        return;
    }
    commonEventMgr->NotifyBundleStatus(installRes, nullptr);
}
}  // namespace AppExecFwk
}  // namespace OHOS
