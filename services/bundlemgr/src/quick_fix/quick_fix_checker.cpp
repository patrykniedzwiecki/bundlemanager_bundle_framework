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

#include "quick_fix_checker.h"

#include "bundle_info.h"
#include "bundle_install_checker.h"
#include "bundle_mgr_service.h"
#include "bundle_parser.h"
#include "bundle_util.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
const std::unordered_map<Security::Verify::AppDistType, std::string> APP_DISTRIBUTION_TYPE_MAPS = {
    { Security::Verify::AppDistType::NONE_TYPE, Constants::APP_DISTRIBUTION_TYPE_NONE },
    { Security::Verify::AppDistType::APP_GALLERY, Constants::APP_DISTRIBUTION_TYPE_APP_GALLERY },
    { Security::Verify::AppDistType::ENTERPRISE, Constants::APP_DISTRIBUTION_TYPE_ENTERPRISE },
    { Security::Verify::AppDistType::OS_INTEGRATION, Constants::APP_DISTRIBUTION_TYPE_OS_INTEGRATION },
    { Security::Verify::AppDistType::CROWDTESTING, Constants::APP_DISTRIBUTION_TYPE_CROWDTESTING },
};
const size_t QUICK_FIX_MAP_SIZE = 1;

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

ErrCode QuickFixChecker::CheckMultipleHapsSignInfo(
    const std::vector<std::string> &bundlePaths,
    std::vector<Security::Verify::HapVerifyResult> &hapVerifyRes)
{
    APP_LOGD("Check multiple haps signInfo");
    BundleInstallChecker checker;
    return checker.CheckMultipleHapsSignInfo(bundlePaths, hapVerifyRes);
}

ErrCode QuickFixChecker::ParseAppQuickFix(const std::string &patchPath, AppQuickFix &appQuickFix)
{
    return ERR_OK;
}

ErrCode QuickFixChecker::ParseAppQuickFixFiles(
    const std::vector<std::string> &filePaths,
    std::unordered_map<std::string, AppQuickFix> &appQuickFixs)
{
    APP_LOGD("Parse quick fix files start.");
    ErrCode result = ERR_OK;
    for (size_t index = 0; index < filePaths.size(); ++index) {
        AppQuickFix appQuickFix;
        result = ParseAppQuickFix(filePaths[index], appQuickFix);
        if (result != ERR_OK) {
            APP_LOGE("quick fix parse failed %{public}d", result);
            return result;
        }
        appQuickFixs.emplace(filePaths[index], appQuickFix);
    }
    APP_LOGD("Parse quick fix files end.");
    return result;
}

ErrCode QuickFixChecker::CheckAppQuickFixInfos(const std::unordered_map<std::string, AppQuickFix> &infos)
{
    APP_LOGD("Check quick fix files start.");
    if (infos.size() <= QUICK_FIX_MAP_SIZE) {
        return ERR_OK;
    }
    const AppQuickFix &appQuickFix = infos.begin()->second;
    for (const auto &info : infos) {
        if (appQuickFix.bundleName != info.second.bundleName) {
            return ERR_APPEXECFWK_QUICK_FIX_BUNDLE_NAME_NOT_SAME;
        }
        if (appQuickFix.versionCode != info.second.versionCode) {
            return ERR_APPEXECFWK_QUICK_FIX_VERSION_CODE_NOT_SAME;
        }
        if (appQuickFix.versionName != info.second.versionName) {
            return ERR_APPEXECFWK_QUICK_FIX_VERSION_NAME_NOT_SAME;
        }
        if (appQuickFix.deployingAppqfInfo.versionCode != info.second.deployingAppqfInfo.versionCode) {
            return ERR_APPEXECFWK_QUICK_FIX_PATCH_VERSION_CODE_NOT_SAME;
        }
        if (appQuickFix.deployingAppqfInfo.versionName != info.second.deployingAppqfInfo.versionName) {
            return ERR_APPEXECFWK_QUICK_FIX_PATCH_VERSION_NAME_NOT_SAME;
        }
    }
    APP_LOGD("Check quick fix files end.");
    return ERR_OK;
}

ErrCode QuickFixChecker::CheckAppQuickFixInfosWithInstalledBundle(
    const std::unordered_map<std::string, AppQuickFix> &infos,
    const Security::Verify::ProvisionInfo &provisionInfo)
{
    std::shared_ptr<BundleDataMgr> dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if ((dataMgr == nullptr) || infos.empty()) {
        APP_LOGE("failed due to dataMgr is nullptr");
        return ERR_APPEXECFWK_QUICK_FIX_INTERNAL_ERROR;
    }
    // check bundleName is exists
    BundleInfo bundleInfo;
    if (!dataMgr->GetBundleInfo(infos.begin()->second.bundleName, BundleFlag::GET_BUNDLE_DEFAULT,
        bundleInfo, Constants::UNSPECIFIED_USERID)) {
        APP_LOGE("error: bundleName %{public}s does not exist!", infos.begin()->second.bundleName.c_str());
        return ERR_APPEXECFWK_QUICK_FIX_BUNDLE_NAME_NOT_EXIST;
    }
    // check versionCode and versionName
    if (bundleInfo.versionCode != (infos.begin()->second).versionCode) {
        return ERR_APPEXECFWK_QUICK_FIX_VERSION_CODE_NOT_SAME;
    }
    if (bundleInfo.versionName != (infos.begin()->second).versionName) {
        return ERR_APPEXECFWK_QUICK_FIX_VERSION_NAME_NOT_SAME;
    }
    // check moduleName is exists
    for (const auto &info : infos) {
        if (info.second.deployingAppqfInfo.hqfInfos.empty()) {
            return ERR_APPEXECFWK_QUICK_FIX_MODULE_NAME_NOT_EXIST;
        }
        auto iter = std::find(bundleInfo.moduleNames.begin(), bundleInfo.moduleNames.end(),
            info.second.deployingAppqfInfo.hqfInfos[0].moduleName);
        if (iter == bundleInfo.moduleNames.end()) {
            APP_LOGE("error: moduleName %{public}s does not exist",
                info.second.deployingAppqfInfo.hqfInfos[0].moduleName.c_str());
            return ERR_APPEXECFWK_QUICK_FIX_MODULE_NAME_NOT_EXIST;
        }
    }
    // check signature info
    if ((bundleInfo.appId != provisionInfo.appId) ||
        (bundleInfo.applicationInfo.appPrivilegeLevel != provisionInfo.bundleInfo.apl) ||
        (bundleInfo.applicationInfo.appDistributionType != GetAppDistributionType(provisionInfo.distributionType)) ||
        (bundleInfo.applicationInfo.appProvisionType != GetAppProvisionType(provisionInfo.type))) {
            APP_LOGE("Quick fix signature info is different with installed bundle : %{public}s",
                bundleInfo.name.c_str());
            return ERR_APPEXECFWK_QUICK_FIX_SIGNATURE_INFO_NOT_SAME;
        }
    return ERR_OK;
}

ErrCode QuickFixChecker::CheckMultiNativeSo(
    std::unordered_map<std::string, AppQuickFix> &infos)
{
    if (infos.size() <= QUICK_FIX_MAP_SIZE) {
        return ERR_OK;
    }
    const AppqfInfo &appqfInfo = (infos.begin()->second).deployingAppqfInfo;
    std::string nativeLibraryPath = appqfInfo.nativeLibraryPath;
    std::string cpuAbi = appqfInfo.deployedAppqfInfo.cpuAbi;
    for (const auto &info : infos) {
        const AppqfInfo &qfInfo = info.second.deployingAppqfInfo;
        if (qfInfo.nativeLibraryPath.empty()) {
            continue;
        }
        if (nativeLibraryPath.empty()) {
            nativeLibraryPath = qfInfo.nativeLibraryPath;
            cpuAbi = qfInfo.cpuAbi;
            continue;
        }
        if ((nativeLibraryPath != qfInfo.nativeLibraryPath)
            || (cpuAbi != qfInfo.cpuAbi)) {
            return ERR_APPEXECFWK_QUICK_FIX_SO_INCOMPATIBLE;
        }
    }

    // Ensure the so is consistent in multiple haps
    if (!nativeLibraryPath.empty()) {
        for (auto &info : infos) {
            info.second.deployingAppqfInfo.nativeLibraryPath = nativeLibraryPath;
            info.second.deployingAppqfInfo.cpuAbi = cpuAbi;
        }
    }

    return ERR_OK;
}
} // AppExecFwk
} // OHOS
