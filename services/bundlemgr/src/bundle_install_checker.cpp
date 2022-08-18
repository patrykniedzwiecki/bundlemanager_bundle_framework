/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "bundle_install_checker.h"

#include "bundle_mgr_service_event_handler.h"
#include "bundle_parser.h"
#include "bundle_util.h"
#include "parameter.h"
#include "systemcapability.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
const std::string PRIVILEGE_ALLOW_APP_DATA_NOT_CLEARED = "AllowAppDataNotCleared";
const std::string PRIVILEGE_ALLOW_APP_MULTI_PROCESS = "AllowAppMultiProcess";
const std::string PRIVILEGE_ALLOW_APP_DESKTOP_ICON_HIDE = "AllowAppDesktopIconHide";
const std::string PRIVILEGE_ALLOW_ABILITY_PRIORITY_QUERIED = "AllowAbilityPriorityQueried";
const std::string PRIVILEGE_ALLOW_ABILITY_EXCLUDE_FROM_MISSIONS = "AllowAbilityExcludeFromMissions";
const std::string PRIVILEGE_ALLOW_APP_USE_PRIVILEGE_EXTENSION = "AllowAppUsePrivilegeExtension";
const std::string PRIVILEGE_ALLOW_FORM_VISIBLE_NOTIFY = "AllowFormVisibleNotify";

const std::unordered_map<Security::Verify::AppDistType, std::string> APP_DISTRIBUTION_TYPE_MAPS = {
    { Security::Verify::AppDistType::NONE_TYPE, Constants::APP_DISTRIBUTION_TYPE_NONE },
    { Security::Verify::AppDistType::APP_GALLERY, Constants::APP_DISTRIBUTION_TYPE_APP_GALLERY },
    { Security::Verify::AppDistType::ENTERPRISE, Constants::APP_DISTRIBUTION_TYPE_ENTERPRISE },
    { Security::Verify::AppDistType::OS_INTEGRATION, Constants::APP_DISTRIBUTION_TYPE_OS_INTEGRATION },
    { Security::Verify::AppDistType::CROWDTESTING, Constants::APP_DISTRIBUTION_TYPE_CROWDTESTING },
};

const std::unordered_map<std::string, void (*)(AppPrivilegeCapability &appPrivilegeCapability)>
        PRIVILEGE_MAP = {
            { PRIVILEGE_ALLOW_APP_DATA_NOT_CLEARED,
                [] (AppPrivilegeCapability &appPrivilegeCapability) {
                    appPrivilegeCapability.userDataClearable = false;
                } },
            { PRIVILEGE_ALLOW_APP_MULTI_PROCESS,
                [] (AppPrivilegeCapability &appPrivilegeCapability) {
                    appPrivilegeCapability.allowMultiProcess = true;
                } },
            { PRIVILEGE_ALLOW_APP_DESKTOP_ICON_HIDE,
                [] (AppPrivilegeCapability &appPrivilegeCapability) {
                    appPrivilegeCapability.hideDesktopIcon = true;
                } },
            { PRIVILEGE_ALLOW_ABILITY_PRIORITY_QUERIED,
                [] (AppPrivilegeCapability &appPrivilegeCapability) {
                    appPrivilegeCapability.allowQueryPriority = true;
                } },
            { PRIVILEGE_ALLOW_ABILITY_EXCLUDE_FROM_MISSIONS,
                [] (AppPrivilegeCapability &appPrivilegeCapability) {
                    appPrivilegeCapability.allowExcludeFromMissions = true;
                } },
            { PRIVILEGE_ALLOW_APP_USE_PRIVILEGE_EXTENSION,
                [] (AppPrivilegeCapability &appPrivilegeCapability) {
                    appPrivilegeCapability.allowUsePrivilegeExtension = true;
                } },
            { PRIVILEGE_ALLOW_FORM_VISIBLE_NOTIFY,
                [] (AppPrivilegeCapability &appPrivilegeCapability) {
                    appPrivilegeCapability.formVisibleNotify = true;
                } },
        };

std::string GetAppDistributionType(const Security::Verify::AppDistType &type)
{
    auto typeIter = APP_DISTRIBUTION_TYPE_MAPS.find(type);
    if (typeIter == APP_DISTRIBUTION_TYPE_MAPS.end()) {
        APP_LOGE("wrong AppDistType");
        return Constants::APP_DISTRIBUTION_TYPE_NONE;
    }

    return typeIter->second;
}

std::string GetAppProvisionType(const Security::Verify::ProvisionType &type)
{
    if (type == Security::Verify::ProvisionType::DEBUG) {
        return Constants::APP_PROVISION_TYPE_DEBUG;
    }

    return Constants::APP_PROVISION_TYPE_RELEASE;
}
}

ErrCode BundleInstallChecker::CheckSysCap(const std::vector<std::string> &bundlePaths)
{
    APP_LOGD("check hap syscaps start.");
    if (bundlePaths.empty()) {
        APP_LOGE("check hap syscaps failed due to empty bundlePaths!");
        return ERR_APPEXECFWK_INSTALL_PARAM_ERROR;
    }

    ErrCode result = ERR_OK;
    BundleParser bundleParser;
    for (const auto &bundlePath : bundlePaths) {
        std::vector<std::string> bundleSysCaps;
        result = bundleParser.ParseSysCap(bundlePath, bundleSysCaps);
        if (result != ERR_OK) {
            APP_LOGE("parse bundle syscap failed, error: %{public}d", result);
            return result;
        }

        for (const auto &bundleSysCapItem : bundleSysCaps) {
            APP_LOGD("check syscap(%{public}s)", bundleSysCapItem.c_str());
            if (!HasSystemCapability(bundleSysCapItem.c_str())) {
                APP_LOGE("check syscap failed which %{public}s is not exsit",
                    bundleSysCapItem.c_str());
                return ERR_APPEXECFWK_INSTALL_CHECK_SYSCAP_FAILED;
            }
        }
    }

    APP_LOGD("finish check hap syscaps");
    return result;
}

ErrCode BundleInstallChecker::CheckMultipleHapsSignInfo(
    const std::vector<std::string> &bundlePaths,
    std::vector<Security::Verify::HapVerifyResult>& hapVerifyRes)
{
    APP_LOGD("Check multiple haps signInfo");
    if (bundlePaths.empty()) {
        APP_LOGE("check hap sign info failed due to empty bundlePaths!");
        return ERR_APPEXECFWK_INSTALL_PARAM_ERROR;
    }

    for (const std::string &bundlePath : bundlePaths) {
        Security::Verify::HapVerifyResult hapVerifyResult;
        auto verifyRes = BundleVerifyMgr::HapVerify(bundlePath, hapVerifyResult);
        if (verifyRes != ERR_OK) {
            APP_LOGE("hap file verify failed");
            return verifyRes;
        }
        hapVerifyRes.emplace_back(hapVerifyResult);
    }

    if (hapVerifyRes.empty()) {
        APP_LOGE("no sign info in the all haps!");
        return ERR_APPEXECFWK_INSTALL_FAILED_INCOMPATIBLE_SIGNATURE;
    }

    auto appId = hapVerifyRes[0].GetProvisionInfo().appId;
    auto apl = hapVerifyRes[0].GetProvisionInfo().bundleInfo.apl;
    auto appDistributionType = hapVerifyRes[0].GetProvisionInfo().distributionType;
    auto appProvisionType = hapVerifyRes[0].GetProvisionInfo().type;
    bool isInvalid = std::any_of(hapVerifyRes.begin(), hapVerifyRes.end(),
        [appId, apl, appDistributionType, appProvisionType](const auto &hapVerifyResult) {
            if (appId != hapVerifyResult.GetProvisionInfo().appId) {
                APP_LOGE("error: hap files have different appId");
                return true;
            }
            if (apl != hapVerifyResult.GetProvisionInfo().bundleInfo.apl) {
                APP_LOGE("error: hap files have different apl");
                return true;
            }
            if (appDistributionType != hapVerifyResult.GetProvisionInfo().distributionType) {
                APP_LOGE("error: hap files have different appDistributionType");
                return true;
            }
            if (appProvisionType != hapVerifyResult.GetProvisionInfo().type) {
                APP_LOGE("error: hap files have different appProvisionType");
                return true;
            }
        return false;
    });
    if (isInvalid) {
        return ERR_APPEXECFWK_INSTALL_FAILED_INCOMPATIBLE_SIGNATURE;
    }
    APP_LOGD("finish check multiple haps signInfo");
    return ERR_OK;
}

ErrCode BundleInstallChecker::ParseHapFiles(
    const std::vector<std::string> &bundlePaths,
    const InstallCheckParam &checkParam,
    std::vector<Security::Verify::HapVerifyResult> &hapVerifyRes,
    std::unordered_map<std::string, InnerBundleInfo> &infos)
{
    APP_LOGD("Parse hap file");
    ErrCode result = ERR_OK;
    BundlePackInfo packInfo;
    for (uint32_t i = 0; i < bundlePaths.size(); ++i) {
        InnerBundleInfo newInfo;
        newInfo.SetAppType(checkParam.appType);
        Security::Verify::ProvisionInfo provisionInfo = hapVerifyRes[i].GetProvisionInfo();
        bool isSystemApp = (provisionInfo.bundleInfo.appFeature == Constants::HOS_SYSTEM_APP ||
            provisionInfo.bundleInfo.appFeature == Constants::OHOS_SYSTEM_APP);
        if (isSystemApp) {
            newInfo.SetAppType(Constants::AppType::SYSTEM_APP);
        }

        AppPrivilegeCapability appPrivilegeCapability;
        ParseAppPrivilegeCapability(provisionInfo, appPrivilegeCapability);
        newInfo.SetIsPreInstallApp(checkParam.isPreInstallApp);
        result = ParseBundleInfo(bundlePaths[i], appPrivilegeCapability, newInfo, packInfo);
        if (result != ERR_OK) {
            APP_LOGE("bundle parse failed %{public}d", result);
            return result;
        }

        if (newInfo.HasEntry()) {
            if (isContainEntry_) {
                APP_LOGE("more than one entry hap in the direction!");
                return ERR_APPEXECFWK_INSTALL_INVALID_NUMBER_OF_ENTRY_HAP;
            }
            isContainEntry_ = true;
        }

        SetEntryInstallationFree(packInfo, newInfo);
        result = CheckMainElement(newInfo);
        if (result != ERR_OK) {
            return result;
        }
        CollectProvisionInfo(provisionInfo, appPrivilegeCapability, newInfo);
#ifdef USE_PRE_BUNDLE_PROFILE
        GetPrivilegeCapability(checkParam, newInfo);
#endif
        if (provisionInfo.distributionType == Security::Verify::AppDistType::CROWDTESTING) {
            newInfo.SetAppCrowdtestDeadline(checkParam.crowdtestDeadline);
        } else {
            newInfo.SetAppCrowdtestDeadline(Constants::INVALID_CROWDTEST_DEADLINE);
        }
        if ((result = CheckSystemSize(bundlePaths[i], checkParam.appType)) != ERR_OK) {
            APP_LOGE("install failed due to insufficient disk memory");
            return result;
        }

        infos.emplace(bundlePaths[i], newInfo);
    }
    if ((result = CheckModuleNameForMulitHaps(infos)) != ERR_OK) {
        APP_LOGE("install failed due to duplicated moduleName");
        return result;
    }
    APP_LOGD("finish parse hap file");
    return result;
}

void BundleInstallChecker::CollectProvisionInfo(
    const Security::Verify::ProvisionInfo &provisionInfo,
    const AppPrivilegeCapability &appPrivilegeCapability,
    InnerBundleInfo &newInfo)
{
    newInfo.SetProvisionId(provisionInfo.appId);
    newInfo.SetAppFeature(provisionInfo.bundleInfo.appFeature);
    newInfo.SetAppPrivilegeLevel(provisionInfo.bundleInfo.apl);
    newInfo.SetAllowedAcls(provisionInfo.acls.allowedAcls);
    newInfo.SetCertificateFingerprint(provisionInfo.fingerprint);
    newInfo.SetAppDistributionType(GetAppDistributionType(provisionInfo.distributionType));
    newInfo.SetAppProvisionType(GetAppProvisionType(provisionInfo.type));
#ifdef USE_PRE_BUNDLE_PROFILE
    newInfo.SetUserDataClearable(appPrivilegeCapability.userDataClearable);
    newInfo.SetHideDesktopIcon(appPrivilegeCapability.hideDesktopIcon);
    newInfo.SetFormVisibleNotify(appPrivilegeCapability.formVisibleNotify);
#endif
}

void BundleInstallChecker::GetPrivilegeCapability(
    const InstallCheckParam &checkParam, InnerBundleInfo &newInfo)
{
    newInfo.SetRemovable(checkParam.removable);
    PreBundleConfigInfo preBundleConfigInfo;
    preBundleConfigInfo.bundleName = newInfo.GetBundleName();
    BMSEventHandler::GetPreInstallCapability(preBundleConfigInfo);
    bool ret = true;
    if (!preBundleConfigInfo.appSignature.empty()) {
        ret = std::find(
            preBundleConfigInfo.appSignature.begin(),
            preBundleConfigInfo.appSignature.end(),
            newInfo.GetCertificateFingerprint()) !=
            preBundleConfigInfo.appSignature.end();
    }

    if (!ret) {
        APP_LOGW("appSignature is incompatible");
        return;
    }

    newInfo.SetKeepAlive(preBundleConfigInfo.keepAlive);
    newInfo.SetSingleton(preBundleConfigInfo.singleton);
    newInfo.SetBootable(preBundleConfigInfo.bootable);
    newInfo.SetRunningResourcesApply(preBundleConfigInfo.runningResourcesApply);
    newInfo.SetAssociatedWakeUp(preBundleConfigInfo.associatedWakeUp);
    newInfo.SetAllowCommonEvent(preBundleConfigInfo.allowCommonEvent);
}

ErrCode BundleInstallChecker::ParseBundleInfo(
    const std::string &bundleFilePath,
    const AppPrivilegeCapability &appPrivilegeCapability,
    InnerBundleInfo &info,
    BundlePackInfo &packInfo) const
{
    BundleParser bundleParser;
    ErrCode result = bundleParser.Parse(bundleFilePath, appPrivilegeCapability, info);
    if (result != ERR_OK) {
        APP_LOGE("parse bundle info failed, error: %{public}d", result);
        return result;
    }

    if (!packInfo.GetValid()) {
        result = bundleParser.ParsePackInfo(bundleFilePath, packInfo);
        if (result != ERR_OK) {
            APP_LOGE("parse bundle pack info failed, error: %{public}d", result);
            return result;
        }

        info.SetBundlePackInfo(packInfo);
        packInfo.SetValid(true);
    }

    return ERR_OK;
}

void BundleInstallChecker::SetEntryInstallationFree(
    const BundlePackInfo &bundlePackInfo,
    InnerBundleInfo &innerBundleInfo)
{
    APP_LOGI("SetEntryInstallationFree start");
    if (!bundlePackInfo.GetValid()) {
        APP_LOGW("no pack.info in the hap file");
        return;
    }

    auto packageModule = bundlePackInfo.summary.modules;
    auto installationFree = std::any_of(packageModule.begin(), packageModule.end(), [&](auto &module) {
        return module.distro.moduleType == "entry" && module.distro.installationFree;
    });
    if (installationFree) {
        APP_LOGI("install or update hm service");
    }

    innerBundleInfo.SetEntryInstallationFree(installationFree);
    APP_LOGI("SetEntryInstallationFree end");
}

ErrCode BundleInstallChecker::CheckSystemSize(
    const std::string &bundlePath,
    const Constants::AppType appType) const
{
    if ((appType == Constants::AppType::SYSTEM_APP) &&
        (BundleUtil::CheckSystemSize(bundlePath, Constants::SYSTEM_APP_INSTALL_PATH))) {
        return ERR_OK;
    }

    if ((appType == Constants::AppType::THIRD_SYSTEM_APP) &&
        (BundleUtil::CheckSystemSize(bundlePath, Constants::THIRD_SYSTEM_APP_INSTALL_PATH))) {
        return ERR_OK;
    }

    if ((appType == Constants::AppType::THIRD_PARTY_APP) &&
        (BundleUtil::CheckSystemSize(bundlePath, Constants::THIRD_PARTY_APP_INSTALL_PATH))) {
        return ERR_OK;
    }

    APP_LOGE("install failed due to insufficient disk memory");
    return ERR_APPEXECFWK_INSTALL_DISK_MEM_INSUFFICIENT;
}

ErrCode BundleInstallChecker::CheckHapHashParams(
    std::unordered_map<std::string, InnerBundleInfo> &infos,
    std::map<std::string, std::string> hashParams)
{
    if (hashParams.empty()) {
        APP_LOGD("hashParams is empty");
        return ERR_OK;
    }

    std::vector<std::string> hapModuleNames;
    for (auto &info : infos) {
        std::vector<std::string> moduleNames;
        info.second.GetModuleNames(moduleNames);
        if (moduleNames.empty()) {
            APP_LOGE("hap(%{public}s) moduleName is empty", info.first.c_str());
            return ERR_APPEXECFWK_INSTALL_FAILED_MODULE_NAME_EMPTY;
        }

        if (std::find(hapModuleNames.begin(), hapModuleNames.end(), moduleNames[0]) != hapModuleNames.end()) {
            APP_LOGE("hap moduleName(%{public}s) duplicate", moduleNames[0].c_str());
            return ERR_APPEXECFWK_INSTALL_FAILED_MODULE_NAME_DUPLICATE;
        }

        hapModuleNames.emplace_back(moduleNames[0]);
        auto hashParamIter = hashParams.find(moduleNames[0]);
        if (hashParamIter != hashParams.end()) {
            info.second.SetModuleHashValue(hashParamIter->second);
            hashParams.erase(hashParamIter);
        }
    }

    if (!hashParams.empty()) {
        APP_LOGE("Some hashParam moduleName is not exist in hap moduleNames");
        return ERR_APPEXECFWK_INSTALL_FAILED_CHECK_HAP_HASH_PARAM;
    }

    return ERR_OK;
}

ErrCode BundleInstallChecker::CheckAppLabelInfo(
    const std::unordered_map<std::string, InnerBundleInfo> &infos)
{
    APP_LOGD("Check APP label");
    ErrCode ret = ERR_OK;
    std::string bundleName = (infos.begin()->second).GetBundleName();
    std::string vendor = (infos.begin()->second).GetVendor();
    uint32_t versionCode = (infos.begin()->second).GetVersionCode();
    std::string versionName = (infos.begin()->second).GetVersionName();
    uint32_t minCompatibleVersionCode = (infos.begin()->second).GetMinCompatibleVersionCode();
    uint32_t target = (infos.begin()->second).GetTargetVersion();
    std::string releaseType = (infos.begin()->second).GetReleaseType();
    uint32_t compatible = (infos.begin()->second).GetCompatibleVersion();
    bool singleton = (infos.begin()->second).IsSingleton();
    Constants::AppType appType = (infos.begin()->second).GetAppType();

    for (const auto &info : infos) {
        // check bundleName
        if (bundleName != info.second.GetBundleName()) {
            return ERR_APPEXECFWK_INSTALL_BUNDLENAME_NOT_SAME;
        }
        // check version
        if (versionCode != info.second.GetVersionCode()) {
            return ERR_APPEXECFWK_INSTALL_VERSIONCODE_NOT_SAME;
        }
        if (versionName != info.second.GetVersionName()) {
            return ERR_APPEXECFWK_INSTALL_VERSIONNAME_NOT_SAME;
        }
        if (minCompatibleVersionCode != info.second.GetMinCompatibleVersionCode()) {
            return ERR_APPEXECFWK_INSTALL_MINCOMPATIBLE_VERSIONCODE_NOT_SAME;
        }
        // check vendor
        if (vendor != info.second.GetVendor()) {
            return ERR_APPEXECFWK_INSTALL_VENDOR_NOT_SAME;
        }
        // check release type
        if (target != info.second.GetTargetVersion()) {
            return ERR_APPEXECFWK_INSTALL_RELEASETYPE_TARGET_NOT_SAME;
        }
        if (compatible != info.second.GetCompatibleVersion()) {
            return ERR_APPEXECFWK_INSTALL_RELEASETYPE_COMPATIBLE_NOT_SAME;
        }
        if (releaseType != info.second.GetReleaseType()) {
            return ERR_APPEXECFWK_INSTALL_RELEASETYPE_NOT_SAME;
        }
        if (singleton != info.second.IsSingleton()) {
            return ERR_APPEXECFWK_INSTALL_SINGLETON_NOT_SAME;
        }
        if (appType != info.second.GetAppType()) {
            return ERR_APPEXECFWK_INSTALL_APPTYPE_NOT_SAME;
        }
    }
    // check api sdk version
    if ((infos.begin()->second).GetCompatibleVersion() > static_cast<uint32_t>(GetSdkApiVersion())) {
        return ERR_APPEXECFWK_INSTALL_SDK_INCOMPATIBLE;
    }
    APP_LOGD("finish check APP label");
    return ret;
}

ErrCode BundleInstallChecker::CheckMultiNativeSo(
    std::unordered_map<std::string, InnerBundleInfo> &infos)
{
    std::string nativeLibraryPath = (infos.begin()->second).GetNativeLibraryPath();
    std::string cpuAbi = (infos.begin()->second).GetCpuAbi();
    for (const auto &info : infos) {
        if (info.second.GetNativeLibraryPath().empty()) {
            continue;
        }
        if (nativeLibraryPath.empty()) {
            nativeLibraryPath = info.second.GetNativeLibraryPath();
            cpuAbi = info.second.GetCpuAbi();
            continue;
        }
        if (nativeLibraryPath != info.second.GetNativeLibraryPath()
            || cpuAbi != info.second.GetCpuAbi()) {
            return ERR_APPEXECFWK_INSTALL_SO_INCOMPATIBLE;
        }
    }

    // Ensure the so is consistent in multiple haps
    if (!nativeLibraryPath.empty()) {
        for (auto &info : infos) {
            info.second.SetNativeLibraryPath(nativeLibraryPath);
            info.second.SetCpuAbi(cpuAbi);
        }
    }

    return ERR_OK;
}

void BundleInstallChecker::ResetProperties()
{
    isContainEntry_ = false;
}

void BundleInstallChecker::ParseAppPrivilegeCapability(
    const Security::Verify::ProvisionInfo &provisionInfo,
    AppPrivilegeCapability &appPrivilegeCapability)
{
    for (const auto &appPrivilege : provisionInfo.appPrivilegeCapabilities) {
        auto iter = PRIVILEGE_MAP.find(appPrivilege);
        if (iter != PRIVILEGE_MAP.end()) {
            iter->second(appPrivilegeCapability);
        }
    }

    APP_LOGD("AppPrivilegeCapability %{public}s",
        appPrivilegeCapability.ToString().c_str());
#ifndef USE_PRE_BUNDLE_PROFILE
    appPrivilegeCapability.allowMultiProcess = true;
    appPrivilegeCapability.allowUsePrivilegeExtension = true;
#endif
}

ErrCode BundleInstallChecker::CheckModuleNameForMulitHaps(
    const std::unordered_map<std::string, InnerBundleInfo> &infos) const
{
    std::set<std::string> moduleSet;
    for (const auto &info : infos) {
        std::vector<std::string> moduleVec = info.second.GetDistroModuleName();
        if (moduleVec.empty()) {
            APP_LOGE("moduleName vector is empty");
            return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
        }
        moduleSet.insert(moduleVec[0]);
    }

    if (moduleSet.size() != infos.size()) {
        APP_LOGE("someone moduleName is not unique in the haps");
        return ERR_APPEXECFWK_INSTALL_NOT_UNIQUE_DISTRO_MODULE_NAME;
    }
    return ERR_OK;
}

bool BundleInstallChecker::IsExistedDistroModule(const InnerBundleInfo &newInfo, const InnerBundleInfo &info) const
{
    std::string moduleName = newInfo.GetCurModuleName();
    std::string packageName = newInfo.GetCurrentModulePackage();
    if (packageName.empty() || moduleName.empty()) {
        APP_LOGE("IsExistedDistroModule failed due to invalid packageName or moduleName");
        return false;
    }
    std::string oldModuleName = info.GetModuleNameByPackage(packageName);
    // check consistency of module name
    if (moduleName.compare(oldModuleName) != 0) {
        APP_LOGE("no moduleName in the innerModuleInfo");
        return false;
    }
    // check consistency of module type
    std::string newModuleType = newInfo.GetModuleTypeByPackage(packageName);
    std::string oldModuleType = info.GetModuleTypeByPackage(packageName);
    if (newModuleType.compare(oldModuleType) != 0) {
        APP_LOGE("moduleType is different between the new hap and the original hap");
        return false;
    }

    return true;
}

bool BundleInstallChecker::IsContainModuleName(const InnerBundleInfo &newInfo, const InnerBundleInfo &info) const
{
    std::string moduleName = newInfo.GetCurModuleName();
    std::vector<std::string> moduleVec = info.GetDistroModuleName();
    if (moduleName.empty() || moduleVec.empty()) {
        APP_LOGE("IsContainModuleName failed due to invalid moduleName or modulevec");
        return false;
    }
    return (find(moduleVec.cbegin(), moduleVec.cend(), moduleName) == moduleVec.cend()) ? false : true;
}

ErrCode BundleInstallChecker::CheckMainElement(const InnerBundleInfo &info)
{
    const std::map<std::string, InnerModuleInfo> &innerModuleInfos = info.GetInnerModuleInfos();
    if (innerModuleInfos.empty()) {
        return ERR_OK;
    }
    if (info.GetEntryInstallationFree() && innerModuleInfos.cbegin()->second.mainAbility.empty()) {
        APP_LOGE("atomic service's mainElement can't be empty.");
        return ERR_APPEXECFWK_PARSE_PROFILE_PROP_CHECK_ERROR;
    }
    return ERR_OK;
}
}  // namespace AppExecFwk
}  // namespace OHOS