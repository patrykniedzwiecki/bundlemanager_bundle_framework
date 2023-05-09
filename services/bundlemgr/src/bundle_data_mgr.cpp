/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "bundle_data_mgr.h"

#include <chrono>
#include <cinttypes>

#ifdef BUNDLE_FRAMEWORK_FREE_INSTALL
#ifdef ACCOUNT_ENABLE
#include "os_account_info.h"
#endif
#endif
#include "account_helper.h"
#include "app_log_wrapper.h"
#include "app_provision_info_manager.h"
#include "bundle_constants.h"
#include "bundle_data_storage_rdb.h"
#include "preinstall_data_storage_rdb.h"
#include "bundle_event_callback_death_recipient.h"
#include "bundle_mgr_service.h"
#include "bundle_status_callback_death_recipient.h"
#include "bundle_util.h"
#ifdef BUNDLE_FRAMEWORK_DEFAULT_APP
#include "default_app_mgr.h"
#endif
#include "installd_client.h"
#include "ipc_skeleton.h"
#include "json_serializer.h"
#ifdef GLOBAL_I18_ENABLE
#include "locale_info.h"
#endif
#include "nlohmann/json.hpp"
#include "free_install_params.h"
#include "parameters.h"
#include "singleton.h"
#ifdef BUNDLE_FRAMEWORK_OVERLAY_INSTALLATION
#include "bundle_overlay_data_manager.h"
#endif

namespace OHOS {
namespace AppExecFwk {
namespace {
constexpr int MAX_EVENT_CALL_BACK_SIZE = 100;
constexpr const char* GLOBAL_RESOURCE_BUNDLE_NAME = "ohos.global.systemres";
}
BundleDataMgr::BundleDataMgr()
{
    InitStateTransferMap();
    dataStorage_ = std::make_shared<BundleDataStorageRdb>();
    preInstallDataStorage_ = std::make_shared<PreInstallDataStorageRdb>();
    sandboxAppHelper_ = DelayedSingleton<BundleSandboxAppHelper>::GetInstance();
    bundleStateStorage_ = std::make_shared<BundleStateStorage>();
    baseAppUid_ = system::GetIntParameter<int32_t>("const.product.baseappid", Constants::BASE_APP_UID);
    if (baseAppUid_ < Constants::BASE_APP_UID || baseAppUid_ >= Constants::MAX_APP_UID) {
        baseAppUid_ = Constants::BASE_APP_UID;
    }
    APP_LOGI("BundleDataMgr instance is created");
}

BundleDataMgr::~BundleDataMgr()
{
    APP_LOGI("BundleDataMgr instance is destroyed");
    installStates_.clear();
    transferStates_.clear();
    bundleInfos_.clear();
}

bool BundleDataMgr::LoadDataFromPersistentStorage()
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    LoadAllPreInstallBundleInfos(preInstallBundleInfos_);
    // Judge whether bundleState json db exists.
    // If it does not exist, create it and return the judgment result.
    bool bundleStateDbExist = bundleStateStorage_->HasBundleUserInfoJsonDb();
    if (!dataStorage_->LoadAllData(bundleInfos_)) {
        APP_LOGE("LoadAllData failed");
        return false;
    }

    if (bundleInfos_.empty()) {
        APP_LOGW("persistent data is empty");
        return false;
    }

    for (const auto &item : bundleInfos_) {
        std::lock_guard<std::mutex> stateLock(stateMutex_);
        installStates_.emplace(item.first, InstallState::INSTALL_SUCCESS);
    }

    RestoreUidAndGid();
    if (!bundleStateDbExist) {
        // Compatible old bundle status in kV db
        CompatibleOldBundleStateInKvDb();
    } else {
        ResetBundleStateData();
        // Load all bundle status from json db.
        LoadAllBundleStateDataFromJsonDb();
    }

    SetInitialUserFlag(true);
    return true;
}

void BundleDataMgr::CompatibleOldBundleStateInKvDb()
{
    for (auto& bundleInfoItem : bundleInfos_) {
        for (auto& innerBundleUserInfoItem : bundleInfoItem.second.GetInnerBundleUserInfos()) {
            auto& bundleUserInfo = innerBundleUserInfoItem.second.bundleUserInfo;
            if (bundleUserInfo.IsInitialState()) {
                continue;
            }

            // save old bundle state to json db
            bundleStateStorage_->SaveBundleStateStorage(
                bundleInfoItem.first, bundleUserInfo.userId, bundleUserInfo);
        }
    }
}

void BundleDataMgr::LoadAllBundleStateDataFromJsonDb()
{
    APP_LOGD("Load all bundle state start");
    std::map<std::string, std::map<int32_t, BundleUserInfo>> bundleStateInfos;
    if (!bundleStateStorage_->LoadAllBundleStateData(bundleStateInfos) || bundleStateInfos.empty()) {
        APP_LOGE("Load all bundle state failed");
        return;
    }

    for (auto& bundleState : bundleStateInfos) {
        auto infoItem = bundleInfos_.find(bundleState.first);
        if (infoItem == bundleInfos_.end()) {
            APP_LOGE("BundleName(%{public}s) not exist in cache", bundleState.first.c_str());
            continue;
        }

        InnerBundleInfo& newInfo = infoItem->second;
        for (auto& bundleUserState : bundleState.second) {
            auto& tempUserInfo = bundleUserState.second;
            newInfo.SetApplicationEnabled(tempUserInfo.enabled, bundleUserState.first);
            for (auto& disabledAbility : tempUserInfo.disabledAbilities) {
                newInfo.SetAbilityEnabled("", disabledAbility, false, bundleUserState.first);
            }
        }
    }

    APP_LOGD("Load all bundle state end");
}

void BundleDataMgr::ResetBundleStateData()
{
    for (auto& bundleInfoItem : bundleInfos_) {
        bundleInfoItem.second.ResetBundleState(Constants::ALL_USERID);
    }
}

bool BundleDataMgr::UpdateBundleInstallState(const std::string &bundleName, const InstallState state)
{
    if (bundleName.empty()) {
        APP_LOGW("update result:fail, reason:bundle name is empty");
        return false;
    }

    // always keep lock bundleInfoMutex_ before locking stateMutex_ to avoid deadlock
    std::lock_guard<std::mutex> lck(bundleInfoMutex_);
    std::lock_guard<std::mutex> lock(stateMutex_);
    auto item = installStates_.find(bundleName);
    if (item == installStates_.end()) {
        if (state == InstallState::INSTALL_START) {
            installStates_.emplace(bundleName, state);
            APP_LOGD("update result:success, state:INSTALL_START");
            return true;
        }
        APP_LOGW("update result:fail, reason:incorrect state");
        return false;
    }

    auto stateRange = transferStates_.equal_range(state);
    for (auto previousState = stateRange.first; previousState != stateRange.second; ++previousState) {
        if (item->second == previousState->second) {
            APP_LOGD("update result:success, current:%{public}d, state:%{public}d", previousState->second, state);
            if (IsDeleteDataState(state)) {
                installStates_.erase(item);
                DeleteBundleInfo(bundleName, state);
                return true;
            }
            item->second = state;
            return true;
        }
    }
    APP_LOGW("update result:fail, reason:incorrect current:%{public}d, state:%{public}d", item->second, state);
    return false;
}

bool BundleDataMgr::AddInnerBundleInfo(const std::string &bundleName, InnerBundleInfo &info)
{
    APP_LOGD("to save info:%{public}s", info.GetBundleName().c_str());
    if (bundleName.empty()) {
        APP_LOGW("save info fail, empty bundle name");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem != bundleInfos_.end()) {
        APP_LOGE("bundle info already exist");
        return false;
    }
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    auto statusItem = installStates_.find(bundleName);
    if (statusItem == installStates_.end()) {
        APP_LOGE("save info fail, app:%{public}s is not installed", bundleName.c_str());
        return false;
    }
    if (statusItem->second == InstallState::INSTALL_START) {
        APP_LOGD("save bundle:%{public}s info", bundleName.c_str());
        if (info.GetBaseApplicationInfo().needAppDetail) {
            AddAppDetailAbilityInfo(info);
        }
#ifdef BUNDLE_FRAMEWORK_OVERLAY_INSTALLATION
        if (info.GetOverlayType() == OVERLAY_EXTERNAL_BUNDLE) {
            InnerBundleInfo newInfo = info;
            OverlayDataMgr::GetInstance()->UpdateExternalOverlayInfo(newInfo, info);
        }
        if (info.GetOverlayType() == OVERLAY_INTERNAL_BUNDLE) {
            info.SetOverlayModuleState(info.GetCurrentModulePackage(), OverlayState::OVERLAY_INVALID,
                info.GetUserId());
        }
        if (info.GetOverlayType() == NON_OVERLAY_TYPE) {
            OverlayDataMgr::GetInstance()->BuildExternalOverlayConnection(info.GetCurrentModulePackage(), info,
                info.GetUserId());
        }
#endif
        if (dataStorage_->SaveStorageBundleInfo(info)) {
            APP_LOGI("write storage success bundle:%{public}s", bundleName.c_str());
            bundleInfos_.emplace(bundleName, info);
            return true;
        }
    }
    return false;
}

bool BundleDataMgr::AddNewModuleInfo(
    const std::string &bundleName, const InnerBundleInfo &newInfo, InnerBundleInfo &oldInfo)
{
    APP_LOGD("add new module info module name %{public}s ", newInfo.GetCurrentModulePackage().c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundle info not exist");
        return false;
    }
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    auto statusItem = installStates_.find(bundleName);
    if (statusItem == installStates_.end()) {
        APP_LOGE("save info fail, app:%{public}s is not updated", bundleName.c_str());
        return false;
    }
    if (statusItem->second == InstallState::UPDATING_SUCCESS) {
        APP_LOGD("save bundle:%{public}s info", bundleName.c_str());
        if (!oldInfo.HasEntry() || oldInfo.GetEntryInstallationFree()) {
            oldInfo.UpdateBaseBundleInfo(newInfo.GetBaseBundleInfo(), newInfo.HasEntry());
            oldInfo.UpdateBaseApplicationInfo(newInfo.GetBaseApplicationInfo());
            oldInfo.UpdateRemovable(
                newInfo.IsPreInstallApp(), newInfo.IsRemovable());
            oldInfo.SetAppPrivilegeLevel(newInfo.GetAppPrivilegeLevel());
            oldInfo.SetAllowedAcls(newInfo.GetAllowedAcls());
        }
        oldInfo.UpdateNativeLibAttrs(newInfo.GetBaseApplicationInfo());
        oldInfo.UpdateArkNativeAttrs(newInfo.GetBaseApplicationInfo());
        oldInfo.SetAsanLogPath(newInfo.GetAsanLogPath());
        oldInfo.SetAsanEnabled(newInfo.GetAsanEnabled());
        oldInfo.SetBundlePackInfo(newInfo.GetBundlePackInfo());
        oldInfo.AddModuleInfo(newInfo);
        oldInfo.UpdateAppDetailAbilityAttrs();
        oldInfo.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
        oldInfo.SetIsNewVersion(newInfo.GetIsNewVersion());
#ifdef BUNDLE_FRAMEWORK_OVERLAY_INSTALLATION
        if ((oldInfo.GetOverlayType() == NON_OVERLAY_TYPE) && (newInfo.GetOverlayType() != NON_OVERLAY_TYPE)) {
            oldInfo.SetOverlayType(newInfo.GetOverlayType());
        }
        if (OverlayDataMgr::GetInstance()->UpdateOverlayInfo(newInfo, oldInfo) != ERR_OK) {
            APP_LOGE("update overlay info failed");
            return false;
        }
#endif
        if (dataStorage_->SaveStorageBundleInfo(oldInfo)) {
            APP_LOGI("update storage success bundle:%{public}s", bundleName.c_str());
            bundleInfos_.at(bundleName) = oldInfo;
            return true;
        }
    }
    return false;
}

bool BundleDataMgr::RemoveModuleInfo(
    const std::string &bundleName, const std::string &modulePackage, InnerBundleInfo &oldInfo)
{
    APP_LOGD("remove module info:%{public}s/%{public}s", bundleName.c_str(), modulePackage.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundle info not exist");
        return false;
    }
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    auto statusItem = installStates_.find(bundleName);
    if (statusItem == installStates_.end()) {
        APP_LOGE("save info fail, app:%{public}s is not updated", bundleName.c_str());
        return false;
    }
    if (statusItem->second == InstallState::UNINSTALL_START || statusItem->second == InstallState::ROLL_BACK) {
        APP_LOGD("save bundle:%{public}s info", bundleName.c_str());
#ifdef BUNDLE_FRAMEWORK_OVERLAY_INSTALLATION
        OverlayDataMgr::GetInstance()->RemoveOverlayModuleInfo(bundleName, modulePackage, oldInfo);
#endif
        oldInfo.RemoveModuleInfo(modulePackage);
        oldInfo.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
        if (!oldInfo.isExistedOverlayModule()) {
            oldInfo.SetOverlayType(NON_OVERLAY_TYPE);
        }
        if (dataStorage_->SaveStorageBundleInfo(oldInfo)) {
            APP_LOGI("update storage success bundle:%{public}s", bundleName.c_str());
            bundleInfos_.at(bundleName) = oldInfo;
            return true;
        }
        APP_LOGD("after delete modulePackage:%{public}s info", modulePackage.c_str());
    }
    return true;
}

bool BundleDataMgr::RemoveHspModuleByVersionCode(int32_t versionCode, InnerBundleInfo &info)
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    std::string bundleName = info.GetBundleName();
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundle info not exist");
        return false;
    }
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    auto statusItem = installStates_.find(bundleName);
    if (statusItem == installStates_.end()) {
        APP_LOGE("save info fail, app:%{public}s is not updated", bundleName.c_str());
        return false;
    }
    if (statusItem->second == InstallState::UNINSTALL_START || statusItem->second == InstallState::ROLL_BACK) {
        info.DeleteHspModuleByVersion(versionCode);
        info.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
        if (dataStorage_->SaveStorageBundleInfo(info)) {
            APP_LOGI("update storage success bundle:%{public}s", bundleName.c_str());
            bundleInfos_.at(bundleName) = info;
            return true;
        }
    }
    return true;
}

bool BundleDataMgr::AddInnerBundleUserInfo(
    const std::string &bundleName, const InnerBundleUserInfo& newUserInfo)
{
    APP_LOGD("AddInnerBundleUserInfo:%{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundle info not exist");
        return false;
    }

    std::lock_guard<std::mutex> stateLock(stateMutex_);
    auto& info = bundleInfos_.at(bundleName);
    info.AddInnerBundleUserInfo(newUserInfo);
    info.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
    if (!dataStorage_->SaveStorageBundleInfo(info)) {
        APP_LOGE("update storage failed bundle:%{public}s", bundleName.c_str());
        return false;
    }
    return true;
}

bool BundleDataMgr::RemoveInnerBundleUserInfo(
    const std::string &bundleName, int32_t userId)
{
    APP_LOGD("RemoveInnerBundleUserInfo:%{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundle info not exist");
        return false;
    }

    std::lock_guard<std::mutex> stateLock(stateMutex_);
    auto& info = bundleInfos_.at(bundleName);
    info.RemoveInnerBundleUserInfo(userId);
    info.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
    if (!dataStorage_->SaveStorageBundleInfo(info)) {
        APP_LOGE("update storage failed bundle:%{public}s", bundleName.c_str());
        return false;
    }

    bundleStateStorage_->DeleteBundleState(bundleName, userId);
    return true;
}

bool BundleDataMgr::UpdateInnerBundleInfo(
    const std::string &bundleName, InnerBundleInfo &newInfo, InnerBundleInfo &oldInfo)
{
    APP_LOGD("UpdateInnerBundleInfo:%{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundle info not exist");
        return false;
    }
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    auto statusItem = installStates_.find(bundleName);
    if (statusItem == installStates_.end()) {
        APP_LOGE("save info fail, app:%{public}s is not updated", bundleName.c_str());
        return false;
    }
    // ROLL_BACK and USER_CHANGE should not be here
    if (statusItem->second == InstallState::UPDATING_SUCCESS
        || statusItem->second == InstallState::ROLL_BACK
        || statusItem->second == InstallState::USER_CHANGE) {
        APP_LOGD("begin to update, bundleName : %{public}s, moduleName : %{public}s",
            bundleName.c_str(), newInfo.GetCurrentModulePackage().c_str());
        bool needAppDetail = oldInfo.GetBaseApplicationInfo().needAppDetail;
        bool isOldInfoHasEntry = oldInfo.HasEntry();
        if (newInfo.GetOverlayType() == NON_OVERLAY_TYPE) {
            oldInfo.KeepOldOverlayConnection(newInfo);
        }
        oldInfo.UpdateModuleInfo(newInfo);
        // 1.exist entry, update entry.
        // 2.only exist feature, update feature.
        if (newInfo.HasEntry() || !isOldInfoHasEntry || oldInfo.GetEntryInstallationFree()) {
            oldInfo.UpdateBaseBundleInfo(newInfo.GetBaseBundleInfo(), newInfo.HasEntry());
            oldInfo.UpdateBaseApplicationInfo(newInfo.GetBaseApplicationInfo());
            oldInfo.UpdateRemovable(
                newInfo.IsPreInstallApp(), newInfo.IsRemovable());
            oldInfo.SetAppType(newInfo.GetAppType());
            oldInfo.SetAppFeature(newInfo.GetAppFeature());
            oldInfo.SetAppPrivilegeLevel(newInfo.GetAppPrivilegeLevel());
            oldInfo.SetAllowedAcls(newInfo.GetAllowedAcls());
        }
        oldInfo.UpdateAppDetailAbilityAttrs();
        if (!needAppDetail && oldInfo.GetBaseApplicationInfo().needAppDetail) {
            AddAppDetailAbilityInfo(oldInfo);
        }
        oldInfo.UpdateNativeLibAttrs(newInfo.GetBaseApplicationInfo());
        oldInfo.UpdateArkNativeAttrs(newInfo.GetBaseApplicationInfo());
        oldInfo.SetAsanLogPath(newInfo.GetAsanLogPath());
        oldInfo.SetAsanEnabled(newInfo.GetAsanEnabled());
        oldInfo.SetAppCrowdtestDeadline(newInfo.GetAppCrowdtestDeadline());
        oldInfo.SetBundlePackInfo(newInfo.GetBundlePackInfo());
        // clear apply quick fix frequency
        oldInfo.ResetApplyQuickFixFrequency();
        oldInfo.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
        oldInfo.SetIsNewVersion(newInfo.GetIsNewVersion());
        oldInfo.SetAppProvisionMetadata(newInfo.GetAppProvisionMetadata());
#ifdef BUNDLE_FRAMEWORK_OVERLAY_INSTALLATION
        if (newInfo.GetIsNewVersion() && newInfo.GetOverlayType() == NON_OVERLAY_TYPE) {
            if (OverlayDataMgr::GetInstance()->UpdateOverlayInfo(newInfo, oldInfo) != ERR_OK) {
                APP_LOGE("update overlay info failed");
                return false;
            }
        }

        if ((newInfo.GetOverlayType() != NON_OVERLAY_TYPE) &&
            (OverlayDataMgr::GetInstance()->UpdateOverlayInfo(newInfo, oldInfo) != ERR_OK)) {
            APP_LOGE("update overlay info failed");
            return false;
        }
#endif
        if (!dataStorage_->SaveStorageBundleInfo(oldInfo)) {
            APP_LOGE("update storage failed bundle:%{public}s", bundleName.c_str());
            return false;
        }
        APP_LOGI("update storage success bundle:%{public}s", bundleName.c_str());
        bundleInfos_.at(bundleName) = oldInfo;
        return true;
    }
    return false;
}

bool BundleDataMgr::QueryAbilityInfo(const Want &want, int32_t flags, int32_t userId, AbilityInfo &abilityInfo,
    int32_t appIndex) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string abilityName = element.GetAbilityName();
    APP_LOGD("QueryAbilityInfo bundle name:%{public}s, ability name:%{public}s",
        bundleName.c_str(), abilityName.c_str());
    // explicit query
    if (!bundleName.empty() && !abilityName.empty()) {
        bool ret = ExplicitQueryAbilityInfo(want, flags, requestUserId, abilityInfo, appIndex);
        if (!ret) {
            APP_LOGE("explicit queryAbilityInfo error");
            return false;
        }
        return true;
    }
    std::vector<AbilityInfo> abilityInfos;
    bool ret = ImplicitQueryAbilityInfos(want, flags, requestUserId, abilityInfos, appIndex);
    if (!ret) {
        APP_LOGE("implicit queryAbilityInfos error");
        return false;
    }
    if (abilityInfos.size() == 0) {
        APP_LOGE("no matching abilityInfo");
        return false;
    }
    abilityInfo = abilityInfos[0];
    return true;
}

bool BundleDataMgr::QueryAbilityInfos(
    const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string abilityName = element.GetAbilityName();
    APP_LOGD("QueryAbilityInfos bundle name:%{public}s, ability name:%{public}s",
        bundleName.c_str(), abilityName.c_str());
    // explicit query
    if (!bundleName.empty() && !abilityName.empty()) {
        AbilityInfo abilityInfo;
        bool ret = ExplicitQueryAbilityInfo(want, flags, requestUserId, abilityInfo);
        if (!ret) {
            APP_LOGE("explicit queryAbilityInfo error");
            return false;
        }
        abilityInfos.emplace_back(abilityInfo);
        return true;
    }
    // implicit query
    bool ret = ImplicitQueryAbilityInfos(want, flags, requestUserId, abilityInfos);
    if (!ret) {
        APP_LOGE("implicit queryAbilityInfos error");
        return false;
    }
    if (abilityInfos.size() == 0) {
        APP_LOGE("no matching abilityInfo");
        return false;
    }
    return true;
}

ErrCode BundleDataMgr::QueryAbilityInfosV9(
    const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string abilityName = element.GetAbilityName();
    APP_LOGD("QueryAbilityInfosV9 bundle name:%{public}s, ability name:%{public}s",
        bundleName.c_str(), abilityName.c_str());
    // explicit query
    if (!bundleName.empty() && !abilityName.empty()) {
        AbilityInfo abilityInfo;
        ErrCode ret = ExplicitQueryAbilityInfoV9(want, flags, requestUserId, abilityInfo);
        if (ret != ERR_OK) {
            APP_LOGE("explicit queryAbilityInfoV9 error");
            return ret;
        }
        abilityInfos.emplace_back(abilityInfo);
        return ERR_OK;
    }
    // implicit query
    ErrCode ret = ImplicitQueryAbilityInfosV9(want, flags, requestUserId, abilityInfos);
    if (ret != ERR_OK) {
        APP_LOGE("implicit queryAbilityInfosV9 error");
        return ret;
    }
    if (abilityInfos.empty()) {
        APP_LOGE("no matching abilityInfo");
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    }
    return ERR_OK;
}

bool BundleDataMgr::ExplicitQueryAbilityInfo(const Want &want, int32_t flags, int32_t userId,
    AbilityInfo &abilityInfo, int32_t appIndex) const
{
    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string abilityName = element.GetAbilityName();
    std::string moduleName = element.GetModuleName();
    APP_LOGD("ExplicitQueryAbilityInfo bundleName:%{public}s, moduleName:%{public}s, abilityName:%{public}s",
        bundleName.c_str(), moduleName.c_str(), abilityName.c_str());
    APP_LOGD("flags:%{public}d, userId:%{public}d", flags, userId);

    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    if ((appIndex == 0) && (!GetInnerBundleInfoWithFlags(bundleName, flags, innerBundleInfo, requestUserId))) {
        APP_LOGE("ExplicitQueryAbiliyInfo failed");
        return false;
    }
    // explict query from sandbox manager
    if (appIndex > 0) {
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return false;
        }
        auto ret = sandboxAppHelper_->GetSandboxAppInfo(bundleName, appIndex, requestUserId, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGW("obtain innerBundleInfo of sandbox app failed due to errCode %{public}d", ret);
            return false;
        }
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    auto ability = innerBundleInfo.FindAbilityInfo(moduleName, abilityName, responseUserId);
    if (!ability) {
        APP_LOGE("ability not found");
        return false;
    }
    return QueryAbilityInfoWithFlags(ability, flags, responseUserId, innerBundleInfo, abilityInfo);
}

ErrCode BundleDataMgr::ExplicitQueryAbilityInfoV9(const Want &want, int32_t flags, int32_t userId,
    AbilityInfo &abilityInfo, int32_t appIndex) const
{
    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string abilityName = element.GetAbilityName();
    std::string moduleName = element.GetModuleName();
    APP_LOGD("ExplicitQueryAbilityInfoV9 bundleName:%{public}s, moduleName:%{public}s, abilityName:%{public}s",
        bundleName.c_str(), moduleName.c_str(), abilityName.c_str());
    APP_LOGD("flags:%{public}d, userId:%{public}d", flags, userId);
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    if (appIndex == 0) {
        ErrCode ret = GetInnerBundleInfoWithFlagsV9(bundleName, flags, innerBundleInfo, requestUserId);
        if (ret != ERR_OK) {
            APP_LOGE("ExplicitQueryAbilityInfoV9 failed");
            return ret;
        }
    }
    // explict query from sandbox manager
    if (appIndex > 0) {
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
        auto ret = sandboxAppHelper_->GetSandboxAppInfo(bundleName, appIndex, requestUserId, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGW("obtain innerBundleInfo of sandbox app failed due to errCode %{public}d", ret);
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    auto ability = innerBundleInfo.FindAbilityInfoV9(moduleName, abilityName);
    if (!ability) {
        APP_LOGE("ability not found");
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    }

    return QueryAbilityInfoWithFlagsV9(ability, flags, responseUserId, innerBundleInfo, abilityInfo);
}

void BundleDataMgr::FilterAbilityInfosByModuleName(const std::string &moduleName,
    std::vector<AbilityInfo> &abilityInfos) const
{
    APP_LOGD("BundleDataMgr::FilterAbilityInfosByModuleName moduleName: %{public}s", moduleName.c_str());
    if (moduleName.empty()) {
        return;
    }
    for (auto iter = abilityInfos.begin(); iter != abilityInfos.end();) {
        if (iter->moduleName != moduleName) {
            iter = abilityInfos.erase(iter);
        } else {
            ++iter;
        }
    }
}

bool BundleDataMgr::ImplicitQueryAbilityInfos(
    const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    if (want.GetAction().empty() && want.GetEntities().empty()
        && want.GetUriString().empty() && want.GetType().empty()) {
        APP_LOGE("param invalid");
        return false;
    }
    APP_LOGD("action:%{public}s, uri:%{private}s, type:%{public}s",
        want.GetAction().c_str(), want.GetUriString().c_str(), want.GetType().c_str());
    APP_LOGD("flags:%{public}d, userId:%{public}d", flags, userId);
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ is empty");
        return false;
    }
    std::string bundleName = want.GetElement().GetBundleName();
    if (!bundleName.empty()) {
        // query in current bundleName
        if (!ImplicitQueryCurAbilityInfos(want, flags, requestUserId, abilityInfos, appIndex)) {
            APP_LOGE("ImplicitQueryCurAbilityInfos failed");
            return false;
        }
    } else {
        // query all
        ImplicitQueryAllAbilityInfos(want, flags, requestUserId, abilityInfos, appIndex);
    }
    // sort by priority, descending order.
    if (abilityInfos.size() > 1) {
        std::stable_sort(abilityInfos.begin(), abilityInfos.end(),
            [](AbilityInfo a, AbilityInfo b) { return a.priority > b.priority; });
    }
    return true;
}

ErrCode BundleDataMgr::ImplicitQueryAbilityInfosV9(
    const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    if (want.GetAction().empty() && want.GetEntities().empty()
        && want.GetUriString().empty() && want.GetType().empty()) {
        APP_LOGE("param invalid");
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    }
    APP_LOGD("action:%{public}s, uri:%{private}s, type:%{public}s",
        want.GetAction().c_str(), want.GetUriString().c_str(), want.GetType().c_str());
    APP_LOGD("flags:%{public}d, userId:%{public}d", flags, userId);
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ is empty");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    std::string bundleName = want.GetElement().GetBundleName();
    if (!bundleName.empty()) {
        // query in current bundleName
        ErrCode ret = ImplicitQueryCurAbilityInfosV9(want, flags, requestUserId, abilityInfos, appIndex);
        if (ret != ERR_OK) {
            APP_LOGE("ImplicitQueryCurAbilityInfosV9 failed");
            return ret;
        }
    } else {
        // query all
        ImplicitQueryAllAbilityInfosV9(want, flags, requestUserId, abilityInfos, appIndex);
    }
    // sort by priority, descending order.
    if (abilityInfos.size() > 1) {
        std::stable_sort(abilityInfos.begin(), abilityInfos.end(),
            [](AbilityInfo a, AbilityInfo b) { return a.priority > b.priority; });
    }
    return ERR_OK;
}

bool BundleDataMgr::QueryAbilityInfoWithFlags(const std::optional<AbilityInfo> &option, int32_t flags, int32_t userId,
    const InnerBundleInfo &innerBundleInfo, AbilityInfo &info) const
{
    APP_LOGD("begin to QueryAbilityInfoWithFlags.");
    if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_SYSTEMAPP_ONLY) == GET_ABILITY_INFO_SYSTEMAPP_ONLY &&
        !innerBundleInfo.IsSystemApp()) {
        APP_LOGE("no system app ability info for this calling");
        return false;
    }
    if (!(static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_DISABLE)) {
        if (!innerBundleInfo.IsAbilityEnabled((*option), userId)) {
            APP_LOGE("ability:%{public}s is disabled", option->name.c_str());
            return false;
        }
    }
    info = (*option);
    if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_PERMISSION) != GET_ABILITY_INFO_WITH_PERMISSION) {
        info.permissions.clear();
    }
    if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_METADATA) != GET_ABILITY_INFO_WITH_METADATA) {
        info.metaData.customizeData.clear();
        info.metadata.clear();
    }
    if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_APPLICATION) == GET_ABILITY_INFO_WITH_APPLICATION) {
        innerBundleInfo.GetApplicationInfo(
            ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, userId, info.applicationInfo);
    }
    // set uid for NAPI cache use
    InnerBundleUserInfo innerBundleUserInfo;
    if (innerBundleInfo.GetInnerBundleUserInfo(userId, innerBundleUserInfo)) {
        info.uid = innerBundleUserInfo.uid;
    }
    return true;
}

ErrCode BundleDataMgr::QueryAbilityInfoWithFlagsV9(const std::optional<AbilityInfo> &option,
    int32_t flags, int32_t userId, const InnerBundleInfo &innerBundleInfo, AbilityInfo &info) const
{
    APP_LOGD("begin to QueryAbilityInfoWithFlagsV9.");
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_ONLY_SYSTEM_APP)) ==
        static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_ONLY_SYSTEM_APP) &&
        !innerBundleInfo.IsSystemApp()) {
        APP_LOGE("target not system app");
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    }
    if (!(static_cast<uint32_t>(flags) & static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_DISABLE))) {
        if (!innerBundleInfo.IsAbilityEnabled((*option), userId)) {
            APP_LOGE("ability:%{public}s is disabled", option->name.c_str());
            return ERR_BUNDLE_MANAGER_ABILITY_DISABLED;
        }
    }
    info = (*option);
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_PERMISSION)) !=
        static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_PERMISSION)) {
        info.permissions.clear();
    }
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_METADATA)) !=
        static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_METADATA)) {
        info.metaData.customizeData.clear();
        info.metadata.clear();
    }
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_APPLICATION)) ==
        static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_APPLICATION)) {
        innerBundleInfo.GetApplicationInfoV9(static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_DEFAULT),
            userId, info.applicationInfo);
    }
    // set uid for NAPI cache use
    InnerBundleUserInfo innerBundleUserInfo;
    if (innerBundleInfo.GetInnerBundleUserInfo(userId, innerBundleUserInfo)) {
        info.uid = innerBundleUserInfo.uid;
    }
    return ERR_OK;
}

bool BundleDataMgr::ImplicitQueryCurAbilityInfos(const Want &want, int32_t flags, int32_t userId,
    std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const
{
    APP_LOGD("begin to ImplicitQueryCurAbilityInfos.");
    std::string bundleName = want.GetElement().GetBundleName();
    InnerBundleInfo innerBundleInfo;
    if ((appIndex == 0) && (!GetInnerBundleInfoWithFlags(bundleName, flags, innerBundleInfo, userId))) {
        APP_LOGE("ImplicitQueryCurAbilityInfos failed");
        return false;
    }
    if (appIndex > 0) {
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return false;
        }
        auto ret = sandboxAppHelper_->GetSandboxAppInfo(bundleName, appIndex, userId, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGW("obtain innerBundleInfo of sandbox app failed due to errCode %{public}d", ret);
            return false;
        }
    }
    int32_t responseUserId = innerBundleInfo.GetResponseUserId(userId);
    GetMatchAbilityInfos(want, flags, innerBundleInfo, responseUserId, abilityInfos);
    FilterAbilityInfosByModuleName(want.GetElement().GetModuleName(), abilityInfos);
    return true;
}

ErrCode BundleDataMgr::ImplicitQueryCurAbilityInfosV9(const Want &want, int32_t flags, int32_t userId,
    std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const
{
    APP_LOGD("begin to ImplicitQueryCurAbilityInfosV9.");
    std::string bundleName = want.GetElement().GetBundleName();
    InnerBundleInfo innerBundleInfo;
    if (appIndex == 0) {
        ErrCode ret = GetInnerBundleInfoWithFlagsV9(bundleName, flags, innerBundleInfo, userId);
        if (ret != ERR_OK) {
            APP_LOGE("ImplicitQueryCurAbilityInfosV9 failed");
            return ret;
        }
    }
    if (appIndex > 0) {
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
        auto ret = sandboxAppHelper_->GetSandboxAppInfo(bundleName, appIndex, userId, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGW("obtain innerBundleInfo of sandbox app failed due to errCode %{public}d", ret);
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
    }
    int32_t responseUserId = innerBundleInfo.GetResponseUserId(userId);
    GetMatchAbilityInfosV9(want, flags, innerBundleInfo, responseUserId, abilityInfos);
    FilterAbilityInfosByModuleName(want.GetElement().GetModuleName(), abilityInfos);
    return ERR_OK;
}

void BundleDataMgr::ImplicitQueryAllAbilityInfos(const Want &want, int32_t flags, int32_t userId,
    std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const
{
    APP_LOGD("begin to ImplicitQueryAllAbilityInfos.");
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        APP_LOGE("invalid userId");
        return;
    }

    // query from bundleInfos_
    if (appIndex == 0) {
        for (const auto &item : bundleInfos_) {
            const InnerBundleInfo &innerBundleInfo = item.second;
            int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
            if (CheckInnerBundleInfoWithFlags(innerBundleInfo, flags, responseUserId) != ERR_OK) {
                APP_LOGW("ImplicitQueryAllAbilityInfos failed");
                continue;
            }

            GetMatchAbilityInfos(want, flags, innerBundleInfo, responseUserId, abilityInfos);
        }
    } else {
        // query from sandbox manager for sandbox bundle
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return;
        }
        auto sandboxMap = sandboxAppHelper_->GetSandboxAppInfoMap();
        for (const auto &item : sandboxMap) {
            InnerBundleInfo info;
            size_t pos = item.first.rfind(Constants::FILE_UNDERLINE);
            if (pos == std::string::npos) {
                APP_LOGW("sandbox map contains invalid element");
                continue;
            }
            std::string innerBundleName = item.first.substr(0, pos);
            if (sandboxAppHelper_->GetSandboxAppInfo(innerBundleName, appIndex, userId, info) != ERR_OK) {
                APP_LOGW("obtain innerBundleInfo of sandbox app failed");
                continue;
            }

            int32_t responseUserId = info.GetResponseUserId(userId);
            GetMatchAbilityInfos(want, flags, info, responseUserId, abilityInfos);
        }
    }
    APP_LOGD("finish to ImplicitQueryAllAbilityInfos.");
}

void BundleDataMgr::ImplicitQueryAllAbilityInfosV9(const Want &want, int32_t flags, int32_t userId,
    std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const
{
    APP_LOGD("begin to ImplicitQueryAllAbilityInfosV9.");
    // query from bundleInfos_
    if (appIndex == 0) {
        for (const auto &item : bundleInfos_) {
            InnerBundleInfo innerBundleInfo;
            ErrCode ret = GetInnerBundleInfoWithFlagsV9(item.first, flags, innerBundleInfo, userId);
            if (ret != ERR_OK) {
                APP_LOGW("ImplicitQueryAllAbilityInfosV9 failed");
                continue;
            }

            int32_t responseUserId = innerBundleInfo.GetResponseUserId(userId);
            GetMatchAbilityInfosV9(want, flags, innerBundleInfo, responseUserId, abilityInfos);
        }
    } else {
        // query from sandbox manager for sandbox bundle
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return;
        }
        auto sandboxMap = sandboxAppHelper_->GetSandboxAppInfoMap();
        for (const auto &item : sandboxMap) {
            InnerBundleInfo info;
            size_t pos = item.first.rfind(Constants::FILE_UNDERLINE);
            if (pos == std::string::npos) {
                APP_LOGW("sandbox map contains invalid element");
                continue;
            }
            std::string innerBundleName = item.first.substr(0, pos);
            if (sandboxAppHelper_->GetSandboxAppInfo(innerBundleName, appIndex, userId, info) != ERR_OK) {
                APP_LOGW("obtain innerBundleInfo of sandbox app failed");
                continue;
            }

            int32_t responseUserId = info.GetResponseUserId(userId);
            GetMatchAbilityInfosV9(want, flags, info, responseUserId, abilityInfos);
        }
    }
    APP_LOGD("finish to ImplicitQueryAllAbilityInfosV9.");
}

void BundleDataMgr::GetMatchAbilityInfos(const Want &want, int32_t flags,
    const InnerBundleInfo &info, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const
{
    if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_SYSTEMAPP_ONLY) == GET_ABILITY_INFO_SYSTEMAPP_ONLY &&
        !info.IsSystemApp()) {
        return;
    }
    std::map<std::string, std::vector<Skill>> skillInfos = info.GetInnerSkillInfos();
    for (const auto &abilityInfoPair : info.GetInnerAbilityInfos()) {
        auto skillsPair = skillInfos.find(abilityInfoPair.first);
        if (skillsPair == skillInfos.end()) {
            continue;
        }
        for (const Skill &skill : skillsPair->second) {
            if (skill.Match(want)) {
                AbilityInfo abilityinfo = abilityInfoPair.second;
                if (abilityinfo.name == Constants::APP_DETAIL_ABILITY) {
                    continue;
                }
                if (!(static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_DISABLE)) {
                    if (!info.IsAbilityEnabled(abilityinfo, GetUserId(userId))) {
                        APP_LOGW("GetMatchAbilityInfos %{public}s is disabled", abilityinfo.name.c_str());
                        continue;
                    }
                }
                if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_APPLICATION) ==
                    GET_ABILITY_INFO_WITH_APPLICATION) {
                    info.GetApplicationInfo(
                        ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, userId,
                        abilityinfo.applicationInfo);
                }
                if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_PERMISSION) !=
                    GET_ABILITY_INFO_WITH_PERMISSION) {
                    abilityinfo.permissions.clear();
                }
                if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_METADATA) != GET_ABILITY_INFO_WITH_METADATA) {
                    abilityinfo.metaData.customizeData.clear();
                    abilityinfo.metadata.clear();
                }
                abilityInfos.emplace_back(abilityinfo);
                break;
            }
        }
    }
}

void BundleDataMgr::GetMatchAbilityInfosV9(const Want &want, int32_t flags,
    const InnerBundleInfo &info, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const
{
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_ONLY_SYSTEM_APP)) ==
        static_cast<int32_t>((GetAbilityInfoFlag::GET_ABILITY_INFO_ONLY_SYSTEM_APP)) &&
        !info.IsSystemApp()) {
        APP_LOGE("target not system app");
        return;
    }
    std::map<std::string, std::vector<Skill>> skillInfos = info.GetInnerSkillInfos();
    for (const auto &abilityInfoPair : info.GetInnerAbilityInfos()) {
        auto skillsPair = skillInfos.find(abilityInfoPair.first);
        if (skillsPair == skillInfos.end()) {
            continue;
        }
        for (const Skill &skill : skillsPair->second) {
            if (skill.Match(want)) {
                AbilityInfo abilityinfo = abilityInfoPair.second;
                if (abilityinfo.name == Constants::APP_DETAIL_ABILITY) {
                    continue;
                }
                if (!(static_cast<uint32_t>(flags) & static_cast<int32_t>(
                    GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_DISABLE))) {
                    if (!info.IsAbilityEnabled(abilityinfo, GetUserId(userId))) {
                        APP_LOGW("GetMatchAbilityInfos %{public}s is disabled", abilityinfo.name.c_str());
                        continue;
                    }
                }
                if ((static_cast<uint32_t>(flags) &
                    static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_APPLICATION)) ==
                    static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_APPLICATION)) {
                    info.GetApplicationInfoV9(static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_DEFAULT),
                        userId, abilityinfo.applicationInfo);
                }
                if ((static_cast<uint32_t>(flags) &
                    static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_PERMISSION)) !=
                    static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_PERMISSION)) {
                    abilityinfo.permissions.clear();
                }
                if ((static_cast<uint32_t>(flags) &
                    static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_METADATA)) !=
                    static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_METADATA)) {
                    abilityinfo.metaData.customizeData.clear();
                    abilityinfo.metadata.clear();
                }
                abilityInfos.emplace_back(abilityinfo);
                break;
            }
        }
    }
}

void BundleDataMgr::ModifyLauncherAbilityInfo(bool isStage, AbilityInfo &abilityInfo) const
{
    if (abilityInfo.labelId == 0) {
        if (isStage) {
            abilityInfo.labelId = abilityInfo.applicationInfo.labelId;
            abilityInfo.label = abilityInfo.applicationInfo.label;
        } else {
            abilityInfo.applicationInfo.label = abilityInfo.bundleName;
            abilityInfo.label = abilityInfo.bundleName;
        }
    }

    if (abilityInfo.iconId == 0) {
        if (isStage) {
            abilityInfo.iconId = abilityInfo.applicationInfo.iconId;
        } else {
            auto iter = bundleInfos_.find(GLOBAL_RESOURCE_BUNDLE_NAME);
            if (iter != bundleInfos_.end()) {
                abilityInfo.iconId = iter->second.GetBaseApplicationInfo().iconId;
            }
        }
    }
}

void BundleDataMgr::GetMatchLauncherAbilityInfos(const Want& want,
    const InnerBundleInfo& info, std::vector<AbilityInfo>& abilityInfos,
    int64_t installTime, int32_t userId) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return;
    }
    int32_t responseUserId = info.GetResponseUserId(requestUserId);
    bool isExist = false;
    bool isStage = info.GetIsNewVersion();
    std::map<std::string, std::vector<Skill>> skillInfos = info.GetInnerSkillInfos();
    for (const auto& abilityInfoPair : info.GetInnerAbilityInfos()) {
        auto skillsPair = skillInfos.find(abilityInfoPair.first);
        if (skillsPair == skillInfos.end()) {
            continue;
        }
        for (const Skill& skill : skillsPair->second) {
            if (skill.MatchLauncher(want) && (abilityInfoPair.second.type == AbilityType::PAGE)) {
                isExist = true;
                AbilityInfo abilityinfo = abilityInfoPair.second;
                info.GetApplicationInfo(ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT,
                    responseUserId, abilityinfo.applicationInfo);
                abilityinfo.installTime = installTime;
                // fix labelId or iconId is equal 0
                ModifyLauncherAbilityInfo(isStage, abilityinfo);
                abilityInfos.emplace_back(abilityinfo);
                break;
            }
        }
    }
    // add app detail ability
    if (!isExist && info.GetBaseApplicationInfo().needAppDetail) {
        APP_LOGD("bundleName: %{public}s add detail ability info.", info.GetBundleName().c_str());
        std::string moduleName = "";
        auto ability = info.FindAbilityInfo(moduleName, Constants::APP_DETAIL_ABILITY, responseUserId);
        if (!ability) {
            APP_LOGE("bundleName: %{public}s can not find app detail ability.", info.GetBundleName().c_str());
            return;
        }
        if (!info.GetIsNewVersion()) {
            ability->applicationInfo.label = info.GetBundleName();
        }
        ability->installTime = installTime;
        abilityInfos.emplace_back(*ability);
    }
}

void BundleDataMgr::AddAppDetailAbilityInfo(InnerBundleInfo &info) const
{
    AbilityInfo appDetailAbility;
    appDetailAbility.name = Constants::APP_DETAIL_ABILITY;
    appDetailAbility.bundleName = info.GetBundleName();
    std::vector<std::string> moduleNameVec;
    info.GetModuleNames(moduleNameVec);
    if (!moduleNameVec.empty()) {
        appDetailAbility.moduleName = moduleNameVec[0];
    } else {
        APP_LOGE("AddAppDetailAbilityInfo error: %{public}s has no module.", appDetailAbility.bundleName.c_str());
    }
    appDetailAbility.enabled = true;
    appDetailAbility.type = AbilityType::PAGE;
    appDetailAbility.package = info.GetCurrentModulePackage();
    appDetailAbility.isNativeAbility = true;

    ApplicationInfo applicationInfo = info.GetBaseApplicationInfo();
    appDetailAbility.applicationName = applicationInfo.name;
    appDetailAbility.labelId = applicationInfo.labelId;
    if (!info.GetIsNewVersion()) {
        appDetailAbility.labelId = 0;
    }
    appDetailAbility.iconId = applicationInfo.iconId;
    if ((appDetailAbility.iconId == 0) || !info.GetIsNewVersion()) {
        APP_LOGD("AddAppDetailAbilityInfo appDetailAbility.iconId is 0.");
        // get system resource icon Id
        auto iter = bundleInfos_.find(GLOBAL_RESOURCE_BUNDLE_NAME);
        if (iter != bundleInfos_.end()) {
            APP_LOGD("AddAppDetailAbilityInfo get system resource iconId");
            appDetailAbility.iconId = iter->second.GetBaseApplicationInfo().iconId;
        } else {
            APP_LOGE("AddAppDetailAbilityInfo error: ohos.global.systemres does not exist.");
        }
    }
    // not show in the mission list
    appDetailAbility.removeMissionAfterTerminate = true;

    std::string keyName;
    keyName.append(appDetailAbility.bundleName).append(".")
        .append(appDetailAbility.package).append(".").append(appDetailAbility.name);
    info.InsertAbilitiesInfo(keyName, appDetailAbility);
}

void BundleDataMgr::GetAllLauncherAbility(const Want &want, std::vector<AbilityInfo> &abilityInfos,
    const int32_t userId, const int32_t requestUserId) const
{
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGI("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        if (info.GetBaseApplicationInfo().hideDesktopIcon) {
            APP_LOGD("Bundle(%{public}s) hide desktop icon", info.GetBundleName().c_str());
            continue;
        }
        if (info.GetBaseBundleInfo().entryInstallationFree) {
            APP_LOGD("Bundle(%{public}s) is atomic service, hide desktop icon", info.GetBundleName().c_str());
            continue;
        }

        // get installTime from innerBundleUserInfo
        int64_t installTime = 0;
        std::string userIdKey = info.GetBundleName() + "_" + std::to_string(userId);
        std::string userZeroKey = info.GetBundleName() + "_" + std::to_string(0);
        for (const auto &userItem : info.GetInnerBundleUserInfos()) {
            if (userItem.first == userIdKey || userItem.first == userZeroKey) {
                installTime = userItem.second.installTime;
                break;
            }
        }
        GetMatchLauncherAbilityInfos(want, info, abilityInfos, installTime, userId);
    }
}

ErrCode BundleDataMgr::GetLauncherAbilityByBundleName(const Want &want, std::vector<AbilityInfo> &abilityInfos,
    const int32_t userId, const int32_t requestUserId) const
{
    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    const auto &item = bundleInfos_.find(bundleName);
    if (item == bundleInfos_.end()) {
        APP_LOGE("no bundleName %{public}s found", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    const InnerBundleInfo &info = item->second;
    if (info.IsDisabled()) {
        APP_LOGE("app %{public}s is disabled", info.GetBundleName().c_str());
        return ERR_BUNDLE_MANAGER_APPLICATION_DISABLED;
    }
    if (info.GetBaseApplicationInfo().hideDesktopIcon) {
        APP_LOGD("Bundle(%{public}s) hide desktop icon", bundleName.c_str());
        return ERR_OK;
    }
    if (info.GetBaseBundleInfo().entryInstallationFree) {
        APP_LOGD("Bundle(%{public}s) is atomic service, hide desktop icon", bundleName.c_str());
        return ERR_OK;
    }
    // get installTime from innerBundleUserInfo
    int64_t installTime = 0;
    std::string userIdKey = info.GetBundleName() + "_" + std::to_string(userId);
    std::string userZeroKey = info.GetBundleName() + "_" + std::to_string(0);
    for (const auto &userItem : info.GetInnerBundleUserInfos()) {
        if (userItem.first == userIdKey || userItem.first == userZeroKey) {
            installTime = userItem.second.installTime;
            break;
        }
    }
    GetMatchLauncherAbilityInfos(want, item->second, abilityInfos, installTime, userId);
    FilterAbilityInfosByModuleName(element.GetModuleName(), abilityInfos);
    return ERR_OK;
}

ErrCode BundleDataMgr::QueryLauncherAbilityInfos(
    const Want &want, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ is empty");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }

    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    if (bundleName.empty()) {
        // query all launcher ability
        GetAllLauncherAbility(want, abilityInfos, userId, requestUserId);
        return ERR_OK;
    } else {
        // query definite abilitys by bundle name
        return GetLauncherAbilityByBundleName(want, abilityInfos, userId, requestUserId);
    }
}

bool BundleDataMgr::QueryAbilityInfoByUri(
    const std::string &abilityUri, int32_t userId, AbilityInfo &abilityInfo) const
{
    APP_LOGD("abilityUri is %{private}s", abilityUri.c_str());
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    if (abilityUri.empty()) {
        return false;
    }
    if (abilityUri.find(Constants::DATA_ABILITY_URI_PREFIX) == std::string::npos) {
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }
    std::string noPpefixUri = abilityUri.substr(strlen(Constants::DATA_ABILITY_URI_PREFIX));
    auto posFirstSeparator = noPpefixUri.find(Constants::DATA_ABILITY_URI_SEPARATOR);
    if (posFirstSeparator == std::string::npos) {
        return false;
    }
    auto posSecondSeparator = noPpefixUri.find(Constants::DATA_ABILITY_URI_SEPARATOR, posFirstSeparator + 1);
    std::string uri;
    if (posSecondSeparator == std::string::npos) {
        uri = noPpefixUri.substr(posFirstSeparator + 1, noPpefixUri.size() - posFirstSeparator - 1);
    } else {
        uri = noPpefixUri.substr(posFirstSeparator + 1, posSecondSeparator - posFirstSeparator - 1);
    }
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGE("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }

        int32_t responseUserId = info.GetResponseUserId(requestUserId);
        if (!info.GetApplicationEnabled(responseUserId)) {
            continue;
        }

        auto ability = info.FindAbilityInfoByUri(uri);
        if (!ability) {
            continue;
        }

        abilityInfo = (*ability);
        info.GetApplicationInfo(
            ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, responseUserId,
            abilityInfo.applicationInfo);
        return true;
    }

    APP_LOGE("query abilityUri(%{private}s) failed.", abilityUri.c_str());
    return false;
}

bool BundleDataMgr::QueryAbilityInfosByUri(const std::string &abilityUri, std::vector<AbilityInfo> &abilityInfos)
{
    APP_LOGI("abilityUri is %{private}s", abilityUri.c_str());
    if (abilityUri.empty()) {
        return false;
    }
    if (abilityUri.find(Constants::DATA_ABILITY_URI_PREFIX) == std::string::npos) {
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGI("bundleInfos_ data is empty");
        return false;
    }
    std::string noPpefixUri = abilityUri.substr(strlen(Constants::DATA_ABILITY_URI_PREFIX));
    auto posFirstSeparator = noPpefixUri.find(Constants::DATA_ABILITY_URI_SEPARATOR);
    if (posFirstSeparator == std::string::npos) {
        return false;
    }
    auto posSecondSeparator = noPpefixUri.find(Constants::DATA_ABILITY_URI_SEPARATOR, posFirstSeparator + 1);
    std::string uri;
    if (posSecondSeparator == std::string::npos) {
        uri = noPpefixUri.substr(posFirstSeparator + 1, noPpefixUri.size() - posFirstSeparator - 1);
    } else {
        uri = noPpefixUri.substr(posFirstSeparator + 1, posSecondSeparator - posFirstSeparator - 1);
    }

    for (auto &item : bundleInfos_) {
        InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGI("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        info.FindAbilityInfosByUri(uri, abilityInfos, GetUserId());
    }
    if (abilityInfos.size() == 0) {
        return false;
    }

    return true;
}

bool BundleDataMgr::GetApplicationInfo(
    const std::string &appName, int32_t flags, const int userId, ApplicationInfo &appInfo) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    if (!GetInnerBundleInfoWithFlags(appName, flags, innerBundleInfo, requestUserId)) {
        APP_LOGE("GetApplicationInfo failed");
        return false;
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    innerBundleInfo.GetApplicationInfo(flags, responseUserId, appInfo);
    return true;
}

ErrCode BundleDataMgr::GetApplicationInfoV9(
    const std::string &appName, int32_t flags, int32_t userId, ApplicationInfo &appInfo) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    int32_t flag = 0;
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_WITH_DISABLE))
        == static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_WITH_DISABLE)) {
        flag = static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_DISABLE);
    }
    auto ret = GetInnerBundleInfoWithBundleFlagsV9(appName, flag, innerBundleInfo, requestUserId);
    if (ret != ERR_OK) {
        APP_LOGE("GetApplicationInfoV9 failed");
        return ret;
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    ret = innerBundleInfo.GetApplicationInfoV9(flags, responseUserId, appInfo);
    if (ret != ERR_OK) {
        APP_LOGE("GetApplicationInfoV9 failed");
        return ret;
    }
    return ret;
}

bool BundleDataMgr::GetApplicationInfos(
    int32_t flags, const int userId, std::vector<ApplicationInfo> &appInfos) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }

    bool find = false;
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGE("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        int32_t responseUserId = info.GetResponseUserId(requestUserId);
        if (!(static_cast<uint32_t>(flags) & GET_APPLICATION_INFO_WITH_DISABLE)
            && !info.GetApplicationEnabled(responseUserId)) {
            APP_LOGD("bundleName: %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        ApplicationInfo appInfo;
        info.GetApplicationInfo(flags, responseUserId, appInfo);
        appInfos.emplace_back(appInfo);
        find = true;
    }
    APP_LOGD("get installed bundles success");
    return find;
}

ErrCode BundleDataMgr::GetApplicationInfosV9(
    int32_t flags, int32_t userId, std::vector<ApplicationInfo> &appInfos) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGE("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        int32_t responseUserId = info.GetResponseUserId(requestUserId);
        if (!(static_cast<uint32_t>(flags) &
            static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_WITH_DISABLE))
            && !info.GetApplicationEnabled(responseUserId)) {
            APP_LOGD("bundleName: %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        ApplicationInfo appInfo;
        auto res = info.GetApplicationInfoV9(flags, responseUserId, appInfo);
        if (res != ERR_OK) {
            return res;
        }
        appInfos.emplace_back(appInfo);
    }
    APP_LOGD("get installed bundles success");
    return ERR_OK;
}

bool BundleDataMgr::GetBundleInfo(
    const std::string &bundleName, int32_t flags, BundleInfo &bundleInfo, int32_t userId) const
{
    std::vector<InnerBundleUserInfo> innerBundleUserInfos;
    if (userId == Constants::ANY_USERID) {
        if (!GetInnerBundleUserInfos(bundleName, innerBundleUserInfos)) {
            APP_LOGE("no userInfos for this bundle(%{public}s)", bundleName.c_str());
            return false;
        }
        userId = innerBundleUserInfos.begin()->bundleUserInfo.userId;
    }

    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    if (!GetInnerBundleInfoWithFlags(bundleName, flags, innerBundleInfo, requestUserId)) {
        APP_LOGE("GetBundleInfo failed");
        return false;
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    innerBundleInfo.GetBundleInfo(flags, bundleInfo, responseUserId);
    APP_LOGD("get bundleInfo(%{public}s) successfully in user(%{public}d)", bundleName.c_str(), userId);
    return true;
}

ErrCode BundleDataMgr::GetBundleInfoV9(
    const std::string &bundleName, int32_t flags, BundleInfo &bundleInfo, int32_t userId) const
{
    std::vector<InnerBundleUserInfo> innerBundleUserInfos;
    if (userId == Constants::ANY_USERID) {
        if (!GetInnerBundleUserInfos(bundleName, innerBundleUserInfos)) {
            APP_LOGE("no userInfos for this bundle(%{public}s)", bundleName.c_str());
            return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
        }
        userId = innerBundleUserInfos.begin()->bundleUserInfo.userId;
    }

    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;

    auto ret = GetInnerBundleInfoWithBundleFlagsV9(bundleName, flags, innerBundleInfo, requestUserId);
    if (ret != ERR_OK) {
        APP_LOGE("GetBundleInfoV9 failed, error code: %{public}d", ret);
        return ret;
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    innerBundleInfo.GetBundleInfoV9(flags, bundleInfo, responseUserId);
    APP_LOGD("get bundleInfo(%{public}s) successfully in user(%{public}d)", bundleName.c_str(), userId);
    return ERR_OK;
}

ErrCode BundleDataMgr::GetBaseSharedBundleInfos(const std::string &bundleName,
    std::vector<BaseSharedBundleInfo> &baseSharedBundleInfos) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("GetBaseSharedBundleInfos get bundleInfo failed");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    const InnerBundleInfo &innerBundleInfo = infoItem->second;
    std::vector<Dependency> dependencies = innerBundleInfo.GetDependencies();
    for (const auto &item : dependencies) {
        BaseSharedBundleInfo baseSharedBundleInfo;
        if (GetBaseSharedBundleInfo(item, baseSharedBundleInfo)) {
            baseSharedBundleInfos.emplace_back(baseSharedBundleInfo);
        }
    }
    APP_LOGD("GetBaseSharedBundleInfos(%{public}s) successfully", bundleName.c_str());
    return ERR_OK;
}

bool BundleDataMgr::GetBaseSharedBundleInfo(const Dependency &dependency,
    BaseSharedBundleInfo &baseSharedBundleInfo) const
{
    auto infoItem = bundleInfos_.find(dependency.bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("GetBaseSharedBundleInfo failed, can not find dependency bundle %{public}s",
            dependency.bundleName.c_str());
        return false;
    }
    const InnerBundleInfo &innerBundleInfo = infoItem->second;
    if (innerBundleInfo.GetApplicationBundleType() == BundleType::SHARED) {
        innerBundleInfo.GetMaxVerBaseSharedBundleInfo(dependency.moduleName, baseSharedBundleInfo);
    } else {
        APP_LOGE("GetBaseSharedBundleInfo failed, can not find bundleType %{public}d",
            innerBundleInfo.GetApplicationBundleType());
        return false;
    }
    APP_LOGD("GetBaseSharedBundleInfo(%{public}s) successfully)", dependency.bundleName.c_str());
    return true;
}

bool BundleDataMgr::DeleteSharedBundleInfo(const std::string &bundleName)
{
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem != bundleInfos_.end()) {
        APP_LOGD("del bundle name:%{public}s", bundleName.c_str());
        const InnerBundleInfo &innerBundleInfo = infoItem->second;
        bool ret = dataStorage_->DeleteStorageBundleInfo(innerBundleInfo);
        if (!ret) {
            APP_LOGW("delete storage error name:%{public}s", bundleName.c_str());
        }
        bundleInfos_.erase(bundleName);
        return ret;
    }
    return false;
}

ErrCode BundleDataMgr::GetBundlePackInfo(
    const std::string &bundleName, int32_t flags, BundlePackInfo &bundlePackInfo, int32_t userId) const
{
    APP_LOGD("Service BundleDataMgr GetBundlePackInfo start");
    int32_t requestUserId;
    if (userId == Constants::UNSPECIFIED_USERID) {
        requestUserId = GetUserIdByCallingUid();
    } else {
        requestUserId = userId;
    }

    if (requestUserId == Constants::INVALID_USERID) {
        APP_LOGE("getBundlePackInfo userId is invalid");
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    if (!GetInnerBundleInfoWithFlags(bundleName, flags, innerBundleInfo, requestUserId)) {
        APP_LOGE("GetBundlePackInfo failed");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    BundlePackInfo innerBundlePackInfo = innerBundleInfo.GetBundlePackInfo();
    if (static_cast<uint32_t>(flags) & GET_PACKAGES) {
        bundlePackInfo.packages = innerBundlePackInfo.packages;
        return ERR_OK;
    }
    if (static_cast<uint32_t>(flags) & GET_BUNDLE_SUMMARY) {
        bundlePackInfo.summary.app = innerBundlePackInfo.summary.app;
        bundlePackInfo.summary.modules = innerBundlePackInfo.summary.modules;
        return ERR_OK;
    }
    if (static_cast<uint32_t>(flags) & GET_MODULE_SUMMARY) {
        bundlePackInfo.summary.modules = innerBundlePackInfo.summary.modules;
        return ERR_OK;
    }
    bundlePackInfo = innerBundlePackInfo;
    return ERR_OK;
}

bool BundleDataMgr::GetBundleInfosByMetaData(
    const std::string &metaData, std::vector<BundleInfo> &bundleInfos) const
{
    if (metaData.empty()) {
        APP_LOGE("bundle name is empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }

    bool find = false;
    int32_t requestUserId = GetUserId();
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGW("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        if (info.CheckSpecialMetaData(metaData)) {
            BundleInfo bundleInfo;
            int32_t responseUserId = info.GetResponseUserId(requestUserId);
            info.GetBundleInfo(
                BundleFlag::GET_BUNDLE_WITH_ABILITIES, bundleInfo, responseUserId);
            bundleInfos.emplace_back(bundleInfo);
            find = true;
        }
    }
    return find;
}

bool BundleDataMgr::GetBundleList(
    std::vector<std::string> &bundleNames, int32_t userId) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }

    bool find = false;
    for (const auto &infoItem : bundleInfos_) {
        const InnerBundleInfo &innerBundleInfo = infoItem.second;
        int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
        if (CheckInnerBundleInfoWithFlags(
            innerBundleInfo, BundleFlag::GET_BUNDLE_DEFAULT, responseUserId) != ERR_OK) {
            continue;
        }

        bundleNames.emplace_back(infoItem.first);
        find = true;
    }
    APP_LOGD("user(%{public}d) get installed bundles list result(%{public}d)", userId, find);
    return find;
}

bool BundleDataMgr::GetBundleInfos(
    int32_t flags, std::vector<BundleInfo> &bundleInfos, int32_t userId) const
{
    if (userId == Constants::ALL_USERID) {
        return GetAllBundleInfos(flags, bundleInfos);
    }

    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }

    bool find = false;
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &innerBundleInfo = item.second;
        if (innerBundleInfo.GetApplicationBundleType() == BundleType::SHARED) {
            APP_LOGD("app %{public}s is cross-app shared bundle, ignore", innerBundleInfo.GetBundleName().c_str());
            continue;
        }

        int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
        if (CheckInnerBundleInfoWithFlags(innerBundleInfo, flags, responseUserId) != ERR_OK) {
            continue;
        }

        BundleInfo bundleInfo;
        if (!innerBundleInfo.GetBundleInfo(flags, bundleInfo, responseUserId)) {
            continue;
        }

        bundleInfos.emplace_back(bundleInfo);
        find = true;
    }
    APP_LOGD("get bundleInfos result(%{public}d) in user(%{public}d).", find, userId);
    return find;
}

ErrCode BundleDataMgr::CheckInnerBundleInfoWithFlags(
    const InnerBundleInfo &innerBundleInfo, const int32_t flags, int32_t userId) const
{
    if (userId == Constants::INVALID_USERID) {
        APP_LOGD("userId is invalid");
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    if (innerBundleInfo.IsDisabled()) {
        APP_LOGE("bundleName: %{public}s status is disabled", innerBundleInfo.GetBundleName().c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }

    if (!(static_cast<uint32_t>(flags) & GET_APPLICATION_INFO_WITH_DISABLE)
        && !innerBundleInfo.GetApplicationEnabled(userId)) {
        APP_LOGE("bundleName: %{public}s is disabled", innerBundleInfo.GetBundleName().c_str());
        return ERR_BUNDLE_MANAGER_APPLICATION_DISABLED;
    }

    return ERR_OK;
}

bool BundleDataMgr::GetAllBundleInfos(int32_t flags, std::vector<BundleInfo> &bundleInfos) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }

    bool find = false;
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGD("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        if (info.GetApplicationBundleType() == BundleType::SHARED) {
            APP_LOGD("app %{public}s is cross-app shared bundle, ignore", info.GetBundleName().c_str());
            continue;
        }
        BundleInfo bundleInfo;
        info.GetBundleInfo(flags, bundleInfo, Constants::ALL_USERID);
        bundleInfos.emplace_back(bundleInfo);
        find = true;
    }

    APP_LOGD("get all bundleInfos result(%{public}d).", find);
    return find;
}

ErrCode BundleDataMgr::GetBundleInfosV9(int32_t flags, std::vector<BundleInfo> &bundleInfos, int32_t userId) const
{
    if (userId == Constants::ALL_USERID) {
        return GetAllBundleInfosV9(flags, bundleInfos);
    }

    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }

    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &innerBundleInfo = item.second;
        if (innerBundleInfo.GetApplicationBundleType() == BundleType::SHARED) {
            APP_LOGD("app %{public}s is cross-app shared bundle, ignore", innerBundleInfo.GetBundleName().c_str());
            continue;
        }

        int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
        auto flag = GET_BASIC_APPLICATION_INFO;
        if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_DISABLE))
            == static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_DISABLE)) {
            flag = GET_APPLICATION_INFO_WITH_DISABLE;
        }
        if (CheckInnerBundleInfoWithFlags(innerBundleInfo, flag, responseUserId) != ERR_OK) {
            continue;
        }

        BundleInfo bundleInfo;
        if (innerBundleInfo.GetBundleInfoV9(flags, bundleInfo, responseUserId) != ERR_OK) {
            continue;
        }

        bundleInfos.emplace_back(bundleInfo);
    }
    return ERR_OK;
}

ErrCode BundleDataMgr::GetAllBundleInfosV9(int32_t flags, std::vector<BundleInfo> &bundleInfos) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }

    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGD("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        if (info.GetApplicationBundleType() == BundleType::SHARED) {
            APP_LOGD("app %{public}s is cross-app shared bundle, ignore", info.GetBundleName().c_str());
            continue;
        }
        BundleInfo bundleInfo;
        info.GetBundleInfoV9(flags, bundleInfo, Constants::ALL_USERID);
        bundleInfos.emplace_back(bundleInfo);
    }
    return ERR_OK;
}

bool BundleDataMgr::GetBundleNameForUid(const int uid, std::string &bundleName) const
{
    InnerBundleInfo innerBundleInfo;
    APP_LOGD("GetBundleNameForUid, uid %{public}d, bundleName %{public}s", uid, bundleName.c_str());
    if (GetInnerBundleInfoByUid(uid, innerBundleInfo) != ERR_OK) {
        if (sandboxAppHelper_ == nullptr) {
            return false;
        }
        if (sandboxAppHelper_->GetInnerBundleInfoByUid(uid, innerBundleInfo) != ERR_OK) {
            return false;
        }
    }

    bundleName = innerBundleInfo.GetBundleName();
    return true;
}

ErrCode BundleDataMgr::GetInnerBundleInfoByUid(const int uid, InnerBundleInfo &innerBundleInfo) const
{
    int32_t userId = GetUserIdByUid(uid);
    if (userId == Constants::UNSPECIFIED_USERID || userId == Constants::INVALID_USERID) {
        APP_LOGE("the uid %{public}d is illegal when get bundleName by uid.", uid);
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }

    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGW("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        if (info.GetUid(userId) == uid) {
            innerBundleInfo = info;
            return ERR_OK;
        }
    }

    APP_LOGD("the uid(%{public}d) is not exists.", uid);
    return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
}

bool BundleDataMgr::HasUserInstallInBundle(
    const std::string &bundleName, const int32_t userId) const
{
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        return false;
    }

    return infoItem->second.HasInnerBundleUserInfo(userId);
}

bool BundleDataMgr::GetBundleStats(
    const std::string &bundleName, const int32_t userId, std::vector<int64_t> &bundleStats) const
{
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        return false;
    }
    int32_t responseUserId = infoItem->second.GetResponseUserId(userId);
    if (InstalldClient::GetInstance()->GetBundleStats(bundleName, responseUserId, bundleStats) != ERR_OK) {
        APP_LOGE("bundle%{public}s GetBundleStats failed ", bundleName.c_str());
        return false;
    }
    if (infoItem->second.IsPreInstallApp() && !bundleStats.empty()) {
        for (const auto &innerModuleInfo : infoItem->second.GetInnerModuleInfos()) {
            bundleStats[0] += BundleUtil::GetFileSize(innerModuleInfo.second.hapPath);
        }
    }
    return true;
}

#ifdef BUNDLE_FRAMEWORK_FREE_INSTALL
int64_t BundleDataMgr::GetBundleSpaceSize(const std::string &bundleName) const
{
    return GetBundleSpaceSize(bundleName, AccountHelper::GetCurrentActiveUserId());
}

int64_t BundleDataMgr::GetBundleSpaceSize(const std::string &bundleName, int32_t userId) const
{
    int64_t spaceSize = 0;
    if (userId != Constants::ALL_USERID) {
        std::vector<int64_t> bundleStats;
        if (!GetBundleStats(bundleName, userId, bundleStats) || bundleStats.empty()) {
            APP_LOGE("GetBundleStats: bundleName: %{public}s failed", bundleName.c_str());
            return spaceSize;
        }

        spaceSize = std::accumulate(bundleStats.begin(), bundleStats.end(), spaceSize);
        return spaceSize;
    }

    for (const auto &iterUserId : GetAllUser()) {
        std::vector<int64_t> bundleStats;
        if (!GetBundleStats(bundleName, iterUserId, bundleStats) || bundleStats.empty()) {
            APP_LOGE("GetBundleStats: bundleName: %{public}s failed", bundleName.c_str());
            continue;
        }

        auto startIter = bundleStats.begin();
        auto endIter = bundleStats.end();
        if (spaceSize == 0) {
            spaceSize = std::accumulate(startIter, endIter, spaceSize);
        } else {
            spaceSize = std::accumulate(++startIter, endIter, spaceSize);
        }
    }

    return spaceSize;
}

int64_t BundleDataMgr::GetAllFreeInstallBundleSpaceSize() const
{
    int64_t allSize = 0;
    std::map<std::string, std::vector<std::string>> freeInstallModules;
    if (!GetFreeInstallModules(freeInstallModules)) {
        APP_LOGE("no removable bundles");
        return allSize;
    }

    for (const auto &iter : freeInstallModules) {
        APP_LOGD("%{public}s is freeInstall bundle", iter.first.c_str());
        allSize += GetBundleSpaceSize(iter.first, Constants::ALL_USERID);
    }

    APP_LOGI("All freeInstall app size:%{public}" PRId64, allSize);
    return allSize;
}

bool BundleDataMgr::GetFreeInstallModules(
    std::map<std::string, std::vector<std::string>> &freeInstallModules) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ is data is empty.");
        return false;
    }

    for (const auto &iter : bundleInfos_) {
        std::vector<std::string> modules;
        if (!iter.second.GetFreeInstallModules(modules)) {
            continue;
        }

        freeInstallModules.emplace(iter.first, modules);
    }

    return !freeInstallModules.empty();
}
#endif

bool BundleDataMgr::GetBundlesForUid(const int uid, std::vector<std::string> &bundleNames) const
{
    InnerBundleInfo innerBundleInfo;
    if (GetInnerBundleInfoByUid(uid, innerBundleInfo) != ERR_OK) {
        APP_LOGE("get innerBundleInfo by uid failed.");
        return false;
    }

    bundleNames.emplace_back(innerBundleInfo.GetBundleName());
    return true;
}

ErrCode BundleDataMgr::GetNameForUid(const int uid, std::string &name) const
{
    InnerBundleInfo innerBundleInfo;
    ErrCode ret = GetInnerBundleInfoByUid(uid, innerBundleInfo);
    if (ret != ERR_OK) {
        APP_LOGW("get innerBundleInfo from bundleInfo_ by uid failed.");
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return ERR_BUNDLE_MANAGER_INVALID_UID;
        }
        if (sandboxAppHelper_->GetInnerBundleInfoByUid(uid, innerBundleInfo) != ERR_OK) {
            return ERR_BUNDLE_MANAGER_INVALID_UID;
        }
    }

    name = innerBundleInfo.GetBundleName();
    return ERR_OK;
}

bool BundleDataMgr::GetBundleGids(const std::string &bundleName, std::vector<int> &gids) const
{
    int32_t requestUserId = GetUserId();
    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetInnerBundleUserInfoByUserId(bundleName, requestUserId, innerBundleUserInfo)) {
        APP_LOGE("the user(%{public}d) is not exists in bundleName(%{public}s) .",
            requestUserId, bundleName.c_str());
        return false;
    }

    gids = innerBundleUserInfo.gids;
    return true;
}

bool BundleDataMgr::GetBundleGidsByUid(
    const std::string &bundleName, const int &uid, std::vector<int> &gids) const
{
    return true;
}

bool BundleDataMgr::QueryKeepAliveBundleInfos(std::vector<BundleInfo> &bundleInfos) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }

    int32_t requestUserId = GetUserId();
    for (const auto &info : bundleInfos_) {
        if (info.second.IsDisabled()) {
            APP_LOGW("app %{public}s is disabled", info.second.GetBundleName().c_str());
            continue;
        }
        if (info.second.GetIsKeepAlive()) {
            BundleInfo bundleInfo;
            int32_t responseUserId = info.second.GetResponseUserId(requestUserId);
            info.second.GetBundleInfo(BundleFlag::GET_BUNDLE_WITH_ABILITIES, bundleInfo, responseUserId);
            if (bundleInfo.name == "") {
                continue;
            }
            bundleInfos.emplace_back(bundleInfo);
        }
    }
    return !(bundleInfos.empty());
}

ErrCode BundleDataMgr::GetAbilityLabel(const std::string &bundleName, const std::string &moduleName,
    const std::string &abilityName, std::string &label) const
{
#ifdef GLOBAL_RESMGR_ENABLE
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    ErrCode ret =
        GetInnerBundleInfoWithFlagsV9(bundleName, BundleFlag::GET_BUNDLE_DEFAULT, innerBundleInfo, GetUserId());
    if (ret != ERR_OK) {
        return ret;
    }
    AbilityInfo abilityInfo;
    if (moduleName.empty()) {
        auto ability = innerBundleInfo.FindAbilityInfoV9(moduleName, abilityName);
        if (!ability) {
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
        abilityInfo = *ability;
    } else {
        ret = innerBundleInfo.FindAbilityInfo(moduleName, abilityName, abilityInfo);
        if (ret != ERR_OK) {
            APP_LOGE("%{public}s:FindAbilityInfo failed: %{public}d", bundleName.c_str(), ret);
            return ret;
        }
    }
    bool isEnable = false;
    ret = innerBundleInfo.IsAbilityEnabledV9(abilityInfo, GetUserId(), isEnable);
    if (ret != ERR_OK) {
        return ret;
    }
    if (!isEnable) {
        APP_LOGE("%{public}s ability disabled: %{public}s", bundleName.c_str(), abilityName.c_str());
        return ERR_BUNDLE_MANAGER_ABILITY_DISABLED;
    }
    if (abilityInfo.labelId == 0) {
        label = abilityInfo.label;
        return ERR_OK;
    }
    std::shared_ptr<OHOS::Global::Resource::ResourceManager> resourceManager =
        GetResourceManager(bundleName, abilityInfo.moduleName, GetUserId());
    if (resourceManager == nullptr) {
        APP_LOGE("InitResourceManager failed");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    auto state = resourceManager->GetStringById(static_cast<uint32_t>(abilityInfo.labelId), label);
    if (state != OHOS::Global::Resource::RState::SUCCESS) {
        APP_LOGE("ResourceManager GetStringById failed");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    return ERR_OK;
#else
    APP_LOGW("GLOBAL_RES_MGR_ENABLE is false");
    return ERR_BUNDLE_MANAGER_GLOBAL_RES_MGR_ENABLE_DISABLED;
#endif
}

bool BundleDataMgr::GetHapModuleInfo(
    const AbilityInfo &abilityInfo, HapModuleInfo &hapModuleInfo, int32_t userId) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }

    APP_LOGD("GetHapModuleInfo %{public}s", abilityInfo.bundleName.c_str());
    auto infoItem = bundleInfos_.find(abilityInfo.bundleName);
    if (infoItem == bundleInfos_.end()) {
        return false;
    }

    const InnerBundleInfo &innerBundleInfo = infoItem->second;
    if (innerBundleInfo.IsDisabled()) {
        APP_LOGE("app %{public}s is disabled", innerBundleInfo.GetBundleName().c_str());
        return false;
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    auto module = innerBundleInfo.FindHapModuleInfo(abilityInfo.package, responseUserId);
    if (!module) {
        APP_LOGE("can not find module %{public}s", abilityInfo.package.c_str());
        return false;
    }
    hapModuleInfo = *module;
    return true;
}

ErrCode BundleDataMgr::GetLaunchWantForBundle(
    const std::string &bundleName, Want &want, int32_t userId) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    ErrCode ret = GetInnerBundleInfoWithFlagsV9(
        bundleName, BundleFlag::GET_BUNDLE_DEFAULT, innerBundleInfo, userId);
    if (ret != ERR_OK) {
        APP_LOGE("GetInnerBundleInfoWithFlagsV9 failed");
        return ret;
    }

    std::string mainAbility = innerBundleInfo.GetMainAbility();
    if (mainAbility.empty()) {
        APP_LOGE("no main ability in the bundle %{public}s", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }

    want.SetElementName("", bundleName, mainAbility);
    want.SetAction(Constants::ACTION_HOME);
    want.AddEntity(Constants::ENTITY_HOME);
    return ERR_OK;
}

bool BundleDataMgr::CheckIsSystemAppByUid(const int uid) const
{
    // If the value of uid is 0 (ROOT_UID) or 1000 (BMS_UID),
    // the uid should be the system uid.
    if (uid == Constants::ROOT_UID || uid == Constants::BMS_UID) {
        return true;
    }

    InnerBundleInfo innerBundleInfo;
    if (GetInnerBundleInfoByUid(uid, innerBundleInfo) != ERR_OK) {
        return false;
    }

    return innerBundleInfo.IsSystemApp();
}

void BundleDataMgr::InitStateTransferMap()
{
    transferStates_.emplace(InstallState::INSTALL_SUCCESS, InstallState::INSTALL_START);
    transferStates_.emplace(InstallState::INSTALL_FAIL, InstallState::INSTALL_START);
    transferStates_.emplace(InstallState::UNINSTALL_START, InstallState::INSTALL_SUCCESS);
    transferStates_.emplace(InstallState::UNINSTALL_START, InstallState::INSTALL_START);
    transferStates_.emplace(InstallState::UNINSTALL_START, InstallState::UPDATING_SUCCESS);
    transferStates_.emplace(InstallState::UNINSTALL_FAIL, InstallState::UNINSTALL_START);
    transferStates_.emplace(InstallState::UNINSTALL_SUCCESS, InstallState::UNINSTALL_START);
    transferStates_.emplace(InstallState::UPDATING_START, InstallState::INSTALL_SUCCESS);
    transferStates_.emplace(InstallState::UPDATING_SUCCESS, InstallState::UPDATING_START);
    transferStates_.emplace(InstallState::UPDATING_FAIL, InstallState::UPDATING_START);
    transferStates_.emplace(InstallState::UPDATING_FAIL, InstallState::INSTALL_START);
    transferStates_.emplace(InstallState::UPDATING_START, InstallState::INSTALL_START);
    transferStates_.emplace(InstallState::INSTALL_SUCCESS, InstallState::UPDATING_START);
    transferStates_.emplace(InstallState::INSTALL_SUCCESS, InstallState::UPDATING_SUCCESS);
    transferStates_.emplace(InstallState::INSTALL_SUCCESS, InstallState::UNINSTALL_START);
    transferStates_.emplace(InstallState::UPDATING_START, InstallState::UPDATING_SUCCESS);
    transferStates_.emplace(InstallState::ROLL_BACK, InstallState::UPDATING_START);
    transferStates_.emplace(InstallState::ROLL_BACK, InstallState::UPDATING_SUCCESS);
    transferStates_.emplace(InstallState::INSTALL_SUCCESS, InstallState::ROLL_BACK);
    transferStates_.emplace(InstallState::UNINSTALL_START, InstallState::USER_CHANGE);
    transferStates_.emplace(InstallState::UPDATING_START, InstallState::USER_CHANGE);
    transferStates_.emplace(InstallState::INSTALL_SUCCESS, InstallState::USER_CHANGE);
    transferStates_.emplace(InstallState::UPDATING_SUCCESS, InstallState::USER_CHANGE);
    transferStates_.emplace(InstallState::USER_CHANGE, InstallState::INSTALL_SUCCESS);
    transferStates_.emplace(InstallState::USER_CHANGE, InstallState::UPDATING_SUCCESS);
    transferStates_.emplace(InstallState::USER_CHANGE, InstallState::UPDATING_START);
}

bool BundleDataMgr::IsDeleteDataState(const InstallState state) const
{
    return (state == InstallState::INSTALL_FAIL || state == InstallState::UNINSTALL_FAIL ||
            state == InstallState::UNINSTALL_SUCCESS || state == InstallState::UPDATING_FAIL);
}

bool BundleDataMgr::IsDisableState(const InstallState state) const
{
    if (state == InstallState::UPDATING_START || state == InstallState::UNINSTALL_START) {
        return true;
    }
    return false;
}

void BundleDataMgr::DeleteBundleInfo(const std::string &bundleName, const InstallState state)
{
    if (InstallState::INSTALL_FAIL == state) {
        APP_LOGW("del fail, bundle:%{public}s has no installed info", bundleName.c_str());
        return;
    }

    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGW("create infoItem fail");
        return;
    }
#ifdef BUNDLE_FRAMEWORK_OVERLAY_INSTALLATION
    if (infoItem->second.GetOverlayType() == OVERLAY_EXTERNAL_BUNDLE) {
        OverlayDataMgr::GetInstance()->RemoveOverlayBundleInfo(infoItem->second.GetTargetBundleName(), bundleName);
    }

    if (infoItem->second.GetOverlayType() == NON_OVERLAY_TYPE) {
        OverlayDataMgr::GetInstance()->ResetExternalOverlayModuleState(bundleName);
    }
#endif
    APP_LOGD("del bundle name:%{public}s", bundleName.c_str());
    const InnerBundleInfo &innerBundleInfo = infoItem->second;
    RecycleUidAndGid(innerBundleInfo);
    bool ret = dataStorage_->DeleteStorageBundleInfo(innerBundleInfo);
    if (!ret) {
        APP_LOGW("delete storage error name:%{public}s", bundleName.c_str());
    }
    bundleInfos_.erase(bundleName);
}

bool BundleDataMgr::IsAppOrAbilityInstalled(const std::string &bundleName) const
{
    if (bundleName.empty()) {
        APP_LOGW("name:%{public}s empty", bundleName.c_str());
        return false;
    }

    std::lock_guard<std::mutex> lock(stateMutex_);
    auto statusItem = installStates_.find(bundleName);
    if (statusItem == installStates_.end()) {
        APP_LOGW("name:%{public}s not find", bundleName.c_str());
        return false;
    }

    if (statusItem->second == InstallState::INSTALL_SUCCESS) {
        return true;
    }

    APP_LOGW("name:%{public}s not install success", bundleName.c_str());
    return false;
}

bool BundleDataMgr::GetInnerBundleInfoWithFlags(const std::string &bundleName,
    const int32_t flags, InnerBundleInfo &info, int32_t userId) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }
    APP_LOGD("GetInnerBundleInfoWithFlags: %{public}s", bundleName.c_str());
    auto item = bundleInfos_.find(bundleName);
    if (item == bundleInfos_.end()) {
        APP_LOGE("GetInnerBundleInfoWithFlags: bundleName not find");
        return false;
    }
    const InnerBundleInfo &innerBundleInfo = item->second;
    if (innerBundleInfo.IsDisabled()) {
        APP_LOGE("bundleName: %{public}s status is disabled", innerBundleInfo.GetBundleName().c_str());
        return false;
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    if (!(static_cast<uint32_t>(flags) & GET_APPLICATION_INFO_WITH_DISABLE)
        && !innerBundleInfo.GetApplicationEnabled(responseUserId)) {
        APP_LOGE("bundleName: %{public}s is disabled", innerBundleInfo.GetBundleName().c_str());
        return false;
    }
    info = innerBundleInfo;
    return true;
}

ErrCode BundleDataMgr::GetInnerBundleInfoWithFlagsV9(const std::string &bundleName,
    const int32_t flags, InnerBundleInfo &info, int32_t userId) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    APP_LOGD("GetInnerBundleInfoWithFlagsV9: %{public}s", bundleName.c_str());
    auto item = bundleInfos_.find(bundleName);
    if (item == bundleInfos_.end()) {
        APP_LOGE("GetInnerBundleInfoWithFlagsV9: bundleName not find");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    const InnerBundleInfo &innerBundleInfo = item->second;
    if (innerBundleInfo.IsDisabled()) {
        APP_LOGE("bundleName: %{public}s status is disabled", innerBundleInfo.GetBundleName().c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    bool isEnabled;
    auto ret = innerBundleInfo.GetApplicationEnabledV9(responseUserId, isEnabled);
    if (ret != ERR_OK) {
        return ret;
    }
    if (!(static_cast<uint32_t>(flags) & static_cast<int32_t>(GetAbilityInfoFlag::GET_ABILITY_INFO_WITH_DISABLE))
        && !isEnabled) {
        APP_LOGE("bundleName: %{public}s is disabled", innerBundleInfo.GetBundleName().c_str());
        return ERR_BUNDLE_MANAGER_APPLICATION_DISABLED;
    }
    info = innerBundleInfo;
    return ERR_OK;
}

ErrCode BundleDataMgr::GetInnerBundleInfoWithBundleFlagsV9(const std::string &bundleName,
    const int32_t flags, InnerBundleInfo &info, int32_t userId) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    APP_LOGD("GetInnerBundleInfoWithFlagsV9: %{public}s", bundleName.c_str());
    auto item = bundleInfos_.find(bundleName);
    if (item == bundleInfos_.end()) {
        APP_LOGE("GetInnerBundleInfoWithFlagsV9: bundleName not find");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    const InnerBundleInfo &innerBundleInfo = item->second;
    if (innerBundleInfo.IsDisabled()) {
        APP_LOGE("bundleName: %{public}s status is disabled", innerBundleInfo.GetBundleName().c_str());
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }

    int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
    bool isEnabled;
    auto ret = innerBundleInfo.GetApplicationEnabledV9(responseUserId, isEnabled);
    if (ret != ERR_OK) {
        return ret;
    }
    if (!(static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_DISABLE))
        && !isEnabled) {
        APP_LOGE("bundleName: %{public}s is disabled", innerBundleInfo.GetBundleName().c_str());
        return ERR_BUNDLE_MANAGER_APPLICATION_DISABLED;
    }
    info = innerBundleInfo;
    return ERR_OK;
}

bool BundleDataMgr::GetInnerBundleInfo(const std::string &bundleName, InnerBundleInfo &info)
{
    APP_LOGD("GetInnerBundleInfo %{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("bundleName is empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return false;
    }
    infoItem->second.SetBundleStatus(InnerBundleInfo::BundleStatus::DISABLED);
    info = infoItem->second;
    info.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
    return true;
}

bool BundleDataMgr::DisableBundle(const std::string &bundleName)
{
    APP_LOGD("DisableBundle %{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("bundleName empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return false;
    }
    infoItem->second.SetBundleStatus(InnerBundleInfo::BundleStatus::DISABLED);
    return true;
}

bool BundleDataMgr::EnableBundle(const std::string &bundleName)
{
    APP_LOGD("EnableBundle %{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("bundleName empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return false;
    }
    infoItem->second.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
    return true;
}

ErrCode BundleDataMgr::IsApplicationEnabled(const std::string &bundleName, bool &isEnabled) const
{
    APP_LOGD("IsApplicationEnabled %{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    int32_t responseUserId = infoItem->second.GetResponseUserId(GetUserId());
    ErrCode ret = infoItem->second.GetApplicationEnabledV9(responseUserId, isEnabled);
    if (ret != ERR_OK) {
        APP_LOGE("GetApplicationEnabled failed: %{public}s", bundleName.c_str());
    }
    return ret;
}

ErrCode BundleDataMgr::SetApplicationEnabled(const std::string &bundleName, bool isEnable, int32_t userId)
{
    APP_LOGD("SetApplicationEnabled %{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        APP_LOGE("Request userId is invalid");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }

    InnerBundleInfo& newInfo = infoItem->second;
    auto ret = newInfo.SetApplicationEnabled(isEnable, requestUserId);
    if (ret != ERR_OK) {
        return ret;
    }
    InnerBundleUserInfo innerBundleUserInfo;
    if (!newInfo.GetInnerBundleUserInfo(requestUserId, innerBundleUserInfo)) {
        APP_LOGE("can not find bundleUserInfo in userId: %{public}d", requestUserId);
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }

    if (innerBundleUserInfo.bundleUserInfo.IsInitialState()) {
        bundleStateStorage_->DeleteBundleState(bundleName, requestUserId);
    } else {
        bundleStateStorage_->SaveBundleStateStorage(
            bundleName, requestUserId, innerBundleUserInfo.bundleUserInfo);
    }
    return ERR_OK;
}

bool BundleDataMgr::SetModuleRemovable(const std::string &bundleName, const std::string &moduleName, bool isEnable)
{
    if (bundleName.empty() || moduleName.empty()) {
        APP_LOGE("bundleName or moduleName is empty");
        return false;
    }
    int32_t userId = AccountHelper::GetCurrentActiveUserId();
    if (userId == Constants::INVALID_USERID) {
        APP_LOGE("get a invalid userid");
        return false;
    }
    APP_LOGD("bundleName:%{public}s, moduleName:%{public}s, userId:%{public}d",
        bundleName.c_str(), moduleName.c_str(), userId);
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return false;
    }
    InnerBundleInfo newInfo = infoItem->second;
    bool ret = newInfo.SetModuleRemovable(moduleName, isEnable, userId);
    if (ret && dataStorage_->SaveStorageBundleInfo(newInfo)) {
        ret = infoItem->second.SetModuleRemovable(moduleName, isEnable, userId);
#ifdef BUNDLE_FRAMEWORK_FREE_INSTALL
        if (isEnable) {
            // call clean task
            APP_LOGD("bundle:%{public}s isEnable:%{public}d ret:%{public}d call clean task",
                bundleName.c_str(), isEnable, ret);
            DelayedSingleton<BundleMgrService>::GetInstance()->GetAgingMgr()->Start(
                BundleAgingMgr::AgingTriggertype::UPDATE_REMOVABLE_FLAG);
        }
#endif
        return ret;
    } else {
        APP_LOGE("bundle:%{public}s SetModuleRemoved failed", bundleName.c_str());
        return false;
    }
}

ErrCode BundleDataMgr::IsModuleRemovable(const std::string &bundleName, const std::string &moduleName,
    bool &isRemovable) const
{
    if (bundleName.empty() || moduleName.empty()) {
        APP_LOGE("bundleName or moduleName is empty");
        return ERR_BUNDLE_MANAGER_PARAM_ERROR;
    }
    int32_t userId = AccountHelper::GetCurrentActiveUserId();
    if (userId == Constants::INVALID_USERID) {
        APP_LOGE("get a invalid userid");
        return ERR_BUNDLE_MANAGER_PARAM_ERROR;
    }
    APP_LOGD("bundleName:%{public}s, moduleName:%{public}s, userId:%{public}d",
        bundleName.c_str(), moduleName.c_str(), userId);
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    InnerBundleInfo newInfo = infoItem->second;
    return newInfo.IsModuleRemovable(moduleName, userId, isRemovable);
}

ErrCode BundleDataMgr::IsAbilityEnabled(const AbilityInfo &abilityInfo, bool &isEnable) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(abilityInfo.bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", abilityInfo.bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    InnerBundleInfo innerBundleInfo = infoItem->second;
    auto ability = innerBundleInfo.FindAbilityInfoV9(
        abilityInfo.moduleName, abilityInfo.name);
    if (!ability) {
        APP_LOGE("ability not found");
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    }
    int32_t responseUserId = innerBundleInfo.GetResponseUserId(GetUserId());
    return innerBundleInfo.IsAbilityEnabledV9((*ability), responseUserId, isEnable);
}

ErrCode BundleDataMgr::SetAbilityEnabled(const AbilityInfo &abilityInfo, bool isEnabled, int32_t userId)
{
    APP_LOGD("SetAbilityEnabled %{public}s", abilityInfo.name.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        APP_LOGE("Request userId is invalid");
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    auto infoItem = bundleInfos_.find(abilityInfo.bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", abilityInfo.bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    InnerBundleInfo& newInfo = infoItem->second;
    ErrCode ret = newInfo.SetAbilityEnabled(
        abilityInfo.moduleName, abilityInfo.name, isEnabled, userId);
    if (ret != ERR_OK) {
        return ret;
    }
    InnerBundleUserInfo innerBundleUserInfo;
    if (!newInfo.GetInnerBundleUserInfo(requestUserId, innerBundleUserInfo)) {
        APP_LOGE("can not find bundleUserInfo in userId: %{public}d", requestUserId);
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }

    if (innerBundleUserInfo.bundleUserInfo.IsInitialState()) {
        bundleStateStorage_->DeleteBundleState(abilityInfo.bundleName, requestUserId);
    } else {
        bundleStateStorage_->SaveBundleStateStorage(
            abilityInfo.bundleName, requestUserId, innerBundleUserInfo.bundleUserInfo);
    }
    return ERR_OK;
}

std::shared_ptr<BundleSandboxAppHelper> BundleDataMgr::GetSandboxAppHelper() const
{
    return sandboxAppHelper_;
}

bool BundleDataMgr::RegisterBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback)
{
    APP_LOGD("RegisterBundleStatusCallback %{public}s", bundleStatusCallback->GetBundleName().c_str());
    std::unique_lock<std::shared_mutex> lock(callbackMutex_);
    callbackList_.emplace_back(bundleStatusCallback);
    if (bundleStatusCallback->AsObject() != nullptr) {
        sptr<BundleStatusCallbackDeathRecipient> deathRecipient =
            new (std::nothrow) BundleStatusCallbackDeathRecipient();
        if (deathRecipient == nullptr) {
            APP_LOGE("deathRecipient is null");
            return false;
        }
        bundleStatusCallback->AsObject()->AddDeathRecipient(deathRecipient);
    }
    return true;
}

bool BundleDataMgr::RegisterBundleEventCallback(const sptr<IBundleEventCallback> &bundleEventCallback)
{
    if (bundleEventCallback == nullptr) {
        APP_LOGE("bundleEventCallback is null");
        return false;
    }
    std::lock_guard<std::mutex> lock(eventCallbackMutex_);
    if (eventCallbackList_.size() >= MAX_EVENT_CALL_BACK_SIZE) {
        APP_LOGE("eventCallbackList_ reach max size %{public}d", MAX_EVENT_CALL_BACK_SIZE);
        return false;
    }
    if (bundleEventCallback->AsObject() != nullptr) {
        sptr<BundleEventCallbackDeathRecipient> deathRecipient =
            new (std::nothrow) BundleEventCallbackDeathRecipient();
        if (deathRecipient == nullptr) {
            APP_LOGE("deathRecipient is null");
            return false;
        }
        bundleEventCallback->AsObject()->AddDeathRecipient(deathRecipient);
    }
    eventCallbackList_.emplace_back(bundleEventCallback);
    return true;
}

bool BundleDataMgr::UnregisterBundleEventCallback(const sptr<IBundleEventCallback> &bundleEventCallback)
{
    APP_LOGD("begin to UnregisterBundleEventCallback");
    if (bundleEventCallback == nullptr) {
        APP_LOGE("bundleEventCallback is null");
        return false;
    }
    std::lock_guard<std::mutex> lock(eventCallbackMutex_);
    eventCallbackList_.erase(std::remove_if(eventCallbackList_.begin(), eventCallbackList_.end(),
        [&bundleEventCallback](const sptr<IBundleEventCallback> &callback) {
            return callback->AsObject() == bundleEventCallback->AsObject();
        }), eventCallbackList_.end());
    return true;
}

void BundleDataMgr::NotifyBundleEventCallback(const EventFwk::CommonEventData &eventData) const
{
    APP_LOGD("begin to NotifyBundleEventCallback");
    std::lock_guard<std::mutex> lock(eventCallbackMutex_);
    for (const auto &callback : eventCallbackList_) {
        callback->OnReceiveEvent(eventData);
    }
    APP_LOGD("finish to NotifyBundleEventCallback");
}

bool BundleDataMgr::ClearBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback)
{
    APP_LOGD("ClearBundleStatusCallback %{public}s", bundleStatusCallback->GetBundleName().c_str());
    std::unique_lock<std::shared_mutex> lock(callbackMutex_);
    callbackList_.erase(std::remove_if(callbackList_.begin(),
        callbackList_.end(),
        [&](const sptr<IBundleStatusCallback> &callback) {
            return callback->AsObject() == bundleStatusCallback->AsObject();
        }),
        callbackList_.end());
    return true;
}

bool BundleDataMgr::UnregisterBundleStatusCallback()
{
    std::unique_lock<std::shared_mutex> lock(callbackMutex_);
    callbackList_.clear();
    return true;
}

bool BundleDataMgr::GenerateUidAndGid(InnerBundleUserInfo &innerBundleUserInfo)
{
    if (innerBundleUserInfo.bundleName.empty()) {
        APP_LOGE("bundleName is null.");
        return false;
    }

    int32_t bundleId = Constants::INVALID_BUNDLEID;
    if (!GenerateBundleId(innerBundleUserInfo.bundleName, bundleId)) {
        APP_LOGE("Generate bundleId failed.");
        return false;
    }

    innerBundleUserInfo.uid = innerBundleUserInfo.bundleUserInfo.userId * Constants::BASE_USER_RANGE
        + bundleId % Constants::BASE_USER_RANGE;
    innerBundleUserInfo.gids.emplace_back(innerBundleUserInfo.uid);
    return true;
}

bool BundleDataMgr::GenerateBundleId(const std::string &bundleName, int32_t &bundleId)
{
    std::lock_guard<std::mutex> lock(bundleIdMapMutex_);
    if (bundleIdMap_.empty()) {
        APP_LOGI("first app install");
        bundleId = baseAppUid_;
        bundleIdMap_.emplace(bundleId, bundleName);
        return true;
    }

    for (const auto &innerBundleId : bundleIdMap_) {
        if (innerBundleId.second == bundleName) {
            bundleId = innerBundleId.first;
            return true;
        }
    }

    for (int32_t i = baseAppUid_; i < bundleIdMap_.rbegin()->first; ++i) {
        if (bundleIdMap_.find(i) == bundleIdMap_.end()) {
            APP_LOGI("the %{public}d app install", i);
            bundleId = i;
            bundleIdMap_.emplace(bundleId, bundleName);
            BundleUtil::MakeFsConfig(bundleName, bundleId, Constants::HMDFS_CONFIG_PATH);
            BundleUtil::MakeFsConfig(bundleName, bundleId, Constants::SHAREFS_CONFIG_PATH);
            return true;
        }
    }

    if (bundleIdMap_.rbegin()->first == Constants::MAX_APP_UID) {
        APP_LOGE("the bundleId exceeding the maximum value.");
        return false;
    }

    bundleId = bundleIdMap_.rbegin()->first + 1;
    bundleIdMap_.emplace(bundleId, bundleName);
    BundleUtil::MakeFsConfig(bundleName, bundleId, Constants::HMDFS_CONFIG_PATH);
    BundleUtil::MakeFsConfig(bundleName, bundleId, Constants::SHAREFS_CONFIG_PATH);
    return true;
}

ErrCode BundleDataMgr::SetModuleUpgradeFlag(const std::string &bundleName,
    const std::string &moduleName, const int32_t upgradeFlag)
{
    APP_LOGD("SetModuleUpgradeFlag %{public}d", upgradeFlag);
    if (bundleName.empty() || moduleName.empty()) {
        APP_LOGE("bundleName or moduleName is empty");
        return ERR_BUNDLE_MANAGER_PARAM_ERROR;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    InnerBundleInfo &newInfo = infoItem->second;
    ErrCode setFlag = newInfo.SetModuleUpgradeFlag(moduleName, upgradeFlag);
    if (setFlag == ERR_OK) {
        if (dataStorage_->SaveStorageBundleInfo(newInfo)) {
            return ERR_OK;
        }
        return ERR_APPEXECFWK_SERVICE_INTERNAL_ERROR;
    }
    APP_LOGE("dataStorage SetModuleUpgradeFlag %{public}s failed", bundleName.c_str());
    return setFlag;
}

int32_t BundleDataMgr::GetModuleUpgradeFlag(const std::string &bundleName, const std::string &moduleName) const
{
    APP_LOGD("bundleName is bundleName:%{public}s, moduleName:%{public}s", bundleName.c_str(), moduleName.c_str());
    if (bundleName.empty() || moduleName.empty()) {
        APP_LOGE("bundleName or moduleName is empty");
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return false;
    }
    InnerBundleInfo newInfo = infoItem->second;
    return newInfo.GetModuleUpgradeFlag(moduleName);
}

void BundleDataMgr::StoreSandboxPersistentInfo(const std::string &bundleName, const SandboxAppPersistentInfo &info)
{
    if (bundleName.empty()) {
        APP_LOGW("StoreSandboxPersistentInfo bundleName is empty");
        return;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.find(bundleName) == bundleInfos_.end()) {
        APP_LOGW("can not find bundle %{public}s", bundleName.c_str());
        return;
    }
    bundleInfos_[bundleName].AddSandboxPersistentInfo(info);
    SaveInnerBundleInfo(bundleInfos_[bundleName]);
}

void BundleDataMgr::DeleteSandboxPersistentInfo(const std::string &bundleName, const SandboxAppPersistentInfo &info)
{
    if (bundleName.empty()) {
        APP_LOGW("DeleteSandboxPersistentInfo bundleName is empty");
        return;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.find(bundleName) == bundleInfos_.end()) {
        APP_LOGW("can not find bundle %{public}s", bundleName.c_str());
        return;
    }
    bundleInfos_[bundleName].RemoveSandboxPersistentInfo(info);
    SaveInnerBundleInfo(bundleInfos_[bundleName]);
}

void BundleDataMgr::RecycleUidAndGid(const InnerBundleInfo &info)
{
    auto userInfos = info.GetInnerBundleUserInfos();
    if (userInfos.empty()) {
        return;
    }

    auto innerBundleUserInfo = userInfos.begin()->second;
    int32_t bundleId = innerBundleUserInfo.uid -
        innerBundleUserInfo.bundleUserInfo.userId * Constants::BASE_USER_RANGE;
    std::lock_guard<std::mutex> lock(bundleIdMapMutex_);
    auto infoItem = bundleIdMap_.find(bundleId);
    if (infoItem == bundleIdMap_.end()) {
        return;
    }

    bundleIdMap_.erase(bundleId);
    BundleUtil::RemoveFsConfig(innerBundleUserInfo.bundleName, Constants::HMDFS_CONFIG_PATH);
    BundleUtil::RemoveFsConfig(innerBundleUserInfo.bundleName, Constants::SHAREFS_CONFIG_PATH);
}

bool BundleDataMgr::RestoreUidAndGid()
{
    for (const auto &info : bundleInfos_) {
        bool onlyInsertOne = false;
        for (auto infoItem : info.second.GetInnerBundleUserInfos()) {
            auto innerBundleUserInfo = infoItem.second;
            AddUserId(innerBundleUserInfo.bundleUserInfo.userId);
            if (!onlyInsertOne) {
                onlyInsertOne = true;
                int32_t bundleId = innerBundleUserInfo.uid -
                    innerBundleUserInfo.bundleUserInfo.userId * Constants::BASE_USER_RANGE;
                std::lock_guard<std::mutex> lock(bundleIdMapMutex_);
                auto item = bundleIdMap_.find(bundleId);
                if (item == bundleIdMap_.end()) {
                    bundleIdMap_.emplace(bundleId, innerBundleUserInfo.bundleName);
                } else {
                    bundleIdMap_[bundleId] = innerBundleUserInfo.bundleName;
                }
                BundleUtil::MakeFsConfig(innerBundleUserInfo.bundleName, bundleId, Constants::HMDFS_CONFIG_PATH);
                BundleUtil::MakeFsConfig(innerBundleUserInfo.bundleName, bundleId, Constants::SHAREFS_CONFIG_PATH);
            }
        }
    }
    return true;
}

std::mutex &BundleDataMgr::GetBundleMutex(const std::string &bundleName)
{
    bundleMutex_.lock_shared();
    auto it = bundleMutexMap_.find(bundleName);
    if (it == bundleMutexMap_.end()) {
        bundleMutex_.unlock_shared();
        std::unique_lock lock {bundleMutex_};
        return bundleMutexMap_[bundleName];
    }
    bundleMutex_.unlock_shared();
    return it->second;
}

bool BundleDataMgr::GetProvisionId(const std::string &bundleName, std::string &provisionId) const
{
    APP_LOGD("GetProvisionId %{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("bundleName empty");
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return false;
    }
    provisionId = infoItem->second.GetProvisionId();
    return true;
}

bool BundleDataMgr::GetAppFeature(const std::string &bundleName, std::string &appFeature) const
{
    APP_LOGD("GetAppFeature %{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("bundleName empty");
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return false;
    }
    appFeature = infoItem->second.GetAppFeature();
    return true;
}

void BundleDataMgr::SetInitialUserFlag(bool flag)
{
    APP_LOGD("SetInitialUserFlag %{public}d", flag);
    if (!initialUserFlag_ && flag && bundlePromise_ != nullptr) {
        bundlePromise_->NotifyAllTasksExecuteFinished();
    }

    initialUserFlag_ = flag;
}

std::shared_ptr<IBundleDataStorage> BundleDataMgr::GetDataStorage() const
{
    return dataStorage_;
}

bool BundleDataMgr::GetAllFormsInfo(std::vector<FormInfo> &formInfos) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }
    auto result = false;
    for (const auto &item : bundleInfos_) {
        if (item.second.IsDisabled()) {
            APP_LOGW("app %{public}s is disabled", item.second.GetBundleName().c_str());
            continue;
        }
        item.second.GetFormsInfoByApp(formInfos);
        result = true;
    }
    APP_LOGD("all the form infos find success");
    return result;
}

bool BundleDataMgr::GetFormsInfoByModule(
    const std::string &bundleName, const std::string &moduleName, std::vector<FormInfo> &formInfos) const
{
    if (bundleName.empty()) {
        APP_LOGW("bundle name is empty");
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        return false;
    }
    if (infoItem->second.IsDisabled()) {
        APP_LOGE("app %{public}s is disabled", infoItem->second.GetBundleName().c_str());
        return false;
    }
    infoItem->second.GetFormsInfoByModule(moduleName, formInfos);
    if (formInfos.empty()) {
        return false;
    }
    APP_LOGE("module forminfo find success");
    return true;
}

bool BundleDataMgr::GetFormsInfoByApp(const std::string &bundleName, std::vector<FormInfo> &formInfos) const
{
    if (bundleName.empty()) {
        APP_LOGW("bundle name is empty");
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        return false;
    }
    if (infoItem->second.IsDisabled()) {
        APP_LOGE("app %{public}s is disabled", infoItem->second.GetBundleName().c_str());
        return false;
    }
    infoItem->second.GetFormsInfoByApp(formInfos);
    APP_LOGE("App forminfo find success");
    return true;
}

bool BundleDataMgr::GetShortcutInfos(
    const std::string &bundleName, int32_t userId, std::vector<ShortcutInfo> &shortcutInfos) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    if (!GetInnerBundleInfoWithFlags(
        bundleName, BundleFlag::GET_BUNDLE_DEFAULT, innerBundleInfo, requestUserId)) {
        APP_LOGE("GetShortcutInfos failed");
        return false;
    }
    innerBundleInfo.GetShortcutInfos(shortcutInfos);
    return true;
}

ErrCode BundleDataMgr::GetShortcutInfoV9(
    const std::string &bundleName, int32_t userId, std::vector<ShortcutInfo> &shortcutInfos) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        APP_LOGE("input invalid userid");
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    ErrCode ret = GetInnerBundleInfoWithFlagsV9(bundleName,
        BundleFlag::GET_BUNDLE_DEFAULT, innerBundleInfo, requestUserId);
    if (ret != ERR_OK) {
        APP_LOGE("GetInnerBundleInfoWithFlagsV9 failed");
        return ret;
    }

    innerBundleInfo.GetShortcutInfos(shortcutInfos);
    return ERR_OK;
}

bool BundleDataMgr::GetAllCommonEventInfo(const std::string &eventKey,
    std::vector<CommonEventInfo> &commonEventInfos) const
{
    if (eventKey.empty()) {
        APP_LOGW("event key is empty");
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGI("bundleInfos_ data is empty");
        return false;
    }
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGI("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }
        info.GetCommonEvents(eventKey, commonEventInfos);
    }
    if (commonEventInfos.size() == 0) {
        APP_LOGE("commonEventInfos is empty");
        return false;
    }
    APP_LOGE("commonEventInfos find success");
    return true;
}

bool BundleDataMgr::SavePreInstallBundleInfo(
    const std::string &bundleName, const PreInstallBundleInfo &preInstallBundleInfo)
{
    std::lock_guard<std::mutex> lock(preInstallInfoMutex_);
    if (preInstallDataStorage_ == nullptr) {
        return false;
    }

    if (preInstallDataStorage_->SavePreInstallStorageBundleInfo(preInstallBundleInfo)) {
        auto info = std::find_if(
            preInstallBundleInfos_.begin(), preInstallBundleInfos_.end(), preInstallBundleInfo);
        if (info != preInstallBundleInfos_.end()) {
            *info = preInstallBundleInfo;
        } else {
            preInstallBundleInfos_.emplace_back(preInstallBundleInfo);
        }
        APP_LOGD("write storage success bundle:%{public}s", bundleName.c_str());
        return true;
    }

    return false;
}

bool BundleDataMgr::DeletePreInstallBundleInfo(
    const std::string &bundleName, const PreInstallBundleInfo &preInstallBundleInfo)
{
    std::lock_guard<std::mutex> lock(preInstallInfoMutex_);
    if (preInstallDataStorage_ == nullptr) {
        return false;
    }

    if (preInstallDataStorage_->DeletePreInstallStorageBundleInfo(preInstallBundleInfo)) {
        auto info = std::find_if(
            preInstallBundleInfos_.begin(), preInstallBundleInfos_.end(), preInstallBundleInfo);
        if (info != preInstallBundleInfos_.end()) {
            preInstallBundleInfos_.erase(info);
        }
        APP_LOGI("Delete PreInstall Storage success bundle:%{public}s", bundleName.c_str());
        return true;
    }

    return false;
}

bool BundleDataMgr::GetPreInstallBundleInfo(
    const std::string &bundleName, PreInstallBundleInfo &preInstallBundleInfo)
{
    std::lock_guard<std::mutex> lock(preInstallInfoMutex_);
    if (bundleName.empty()) {
        APP_LOGE("bundleName is empty");
        return false;
    }

    preInstallBundleInfo.SetBundleName(bundleName);
    auto info = std::find_if(
        preInstallBundleInfos_.begin(), preInstallBundleInfos_.end(), preInstallBundleInfo);
    if (info != preInstallBundleInfos_.end()) {
        preInstallBundleInfo = *info;
        return true;
    }

    APP_LOGE("get preInstall bundleInfo failed by bundle(%{public}s).", bundleName.c_str());
    return false;
}

bool BundleDataMgr::LoadAllPreInstallBundleInfos(std::vector<PreInstallBundleInfo> &preInstallBundleInfos)
{
    if (preInstallDataStorage_ == nullptr) {
        return false;
    }

    if (preInstallDataStorage_->LoadAllPreInstallBundleInfos(preInstallBundleInfos)) {
        APP_LOGD("load all storage success");
        return true;
    }

    return false;
}

bool BundleDataMgr::SaveInnerBundleInfo(const InnerBundleInfo &info) const
{
    APP_LOGD("write install InnerBundleInfo to storage with bundle:%{public}s", info.GetBundleName().c_str());
    if (dataStorage_->SaveStorageBundleInfo(info)) {
        APP_LOGD("save install InnerBundleInfo successfully");
        return true;
    }
    APP_LOGE("save install InnerBundleInfo failed!");
    return false;
}

bool BundleDataMgr::GetInnerBundleUserInfoByUserId(const std::string &bundleName,
    int32_t userId, InnerBundleUserInfo &innerBundleUserInfo) const
{
    APP_LOGD("get user info start: bundleName: (%{public}s)  userId: (%{public}d) ",
        bundleName.c_str(), userId);
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    if (bundleName.empty()) {
        APP_LOGW("bundle name is empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos data is empty");
        return false;
    }

    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        return false;
    }
    if (infoItem->second.IsDisabled()) {
        APP_LOGE("app %{public}s is disabled", infoItem->second.GetBundleName().c_str());
        return false;
    }

    return infoItem->second.GetInnerBundleUserInfo(requestUserId, innerBundleUserInfo);
}

int32_t BundleDataMgr::GetUserId(int32_t userId) const
{
    if (userId == Constants::ANY_USERID || userId == Constants::ALL_USERID) {
        return userId;
    }

    if (userId == Constants::UNSPECIFIED_USERID) {
        userId = GetUserIdByCallingUid();
    }

    if (!HasUserId(userId)) {
        APP_LOGE("user is not existed.");
        userId = Constants::INVALID_USERID;
    }

    return userId;
}

int32_t BundleDataMgr::GetUserIdByUid(int32_t uid) const
{
    return BundleUtil::GetUserIdByUid(uid);
}

void BundleDataMgr::AddUserId(int32_t userId)
{
    std::lock_guard<std::mutex> lock(multiUserIdSetMutex_);
    auto item = multiUserIdsSet_.find(userId);
    if (item != multiUserIdsSet_.end()) {
        return;
    }

    multiUserIdsSet_.insert(userId);
}

void BundleDataMgr::RemoveUserId(int32_t userId)
{
    std::lock_guard<std::mutex> lock(multiUserIdSetMutex_);
    auto item = multiUserIdsSet_.find(userId);
    if (item == multiUserIdsSet_.end()) {
        return;
    }

    multiUserIdsSet_.erase(item);
}

bool BundleDataMgr::HasUserId(int32_t userId) const
{
    std::lock_guard<std::mutex> lock(multiUserIdSetMutex_);
    return multiUserIdsSet_.find(userId) != multiUserIdsSet_.end();
}

int32_t BundleDataMgr::GetUserIdByCallingUid() const
{
    return BundleUtil::GetUserIdByCallingUid();
}

std::set<int32_t> BundleDataMgr::GetAllUser() const
{
    std::lock_guard<std::mutex> lock(multiUserIdSetMutex_);
    return multiUserIdsSet_;
}

bool BundleDataMgr::GetInnerBundleUserInfos(
    const std::string &bundleName, std::vector<InnerBundleUserInfo> &innerBundleUserInfos) const
{
    APP_LOGD("get all user info in bundle(%{public}s)", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGW("bundle name is empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos data is empty");
        return false;
    }

    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        return false;
    }
    if (infoItem->second.IsDisabled()) {
        APP_LOGE("app %{public}s is disabled", infoItem->second.GetBundleName().c_str());
        return false;
    }

    for (auto userInfo : infoItem->second.GetInnerBundleUserInfos()) {
        innerBundleUserInfos.emplace_back(userInfo.second);
    }

    return !innerBundleUserInfos.empty();
}

std::string BundleDataMgr::GetAppPrivilegeLevel(const std::string &bundleName, int32_t userId)
{
    APP_LOGD("GetAppPrivilegeLevel:%{public}s, userId:%{public}d", bundleName.c_str(), userId);
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo info;
    if (!GetInnerBundleInfoWithFlags(bundleName, 0, info, userId)) {
        return Constants::EMPTY_STRING;
    }

    return info.GetAppPrivilegeLevel();
}

bool BundleDataMgr::QueryExtensionAbilityInfos(const Want &want, int32_t flags, int32_t userId,
    std::vector<ExtensionAbilityInfo> &extensionInfos, int32_t appIndex) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }

    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string extensionName = element.GetAbilityName();
    APP_LOGD("bundle name:%{public}s, extension name:%{public}s",
        bundleName.c_str(), extensionName.c_str());
    // explicit query
    if (!bundleName.empty() && !extensionName.empty()) {
        ExtensionAbilityInfo info;
        bool ret = ExplicitQueryExtensionInfo(want, flags, requestUserId, info, appIndex);
        if (!ret) {
            APP_LOGE("explicit queryExtensionInfo error");
            return false;
        }
        extensionInfos.emplace_back(info);
        return true;
    }

    bool ret = ImplicitQueryExtensionInfos(want, flags, requestUserId, extensionInfos, appIndex);
    if (!ret) {
        APP_LOGE("implicit queryExtensionAbilityInfos error");
        return false;
    }
    if (extensionInfos.size() == 0) {
        APP_LOGE("no matching abilityInfo");
        return false;
    }
    APP_LOGD("query extensionAbilityInfo successfully");
    return true;
}

ErrCode BundleDataMgr::QueryExtensionAbilityInfosV9(const Want &want, int32_t flags, int32_t userId,
    std::vector<ExtensionAbilityInfo> &extensionInfos, int32_t appIndex) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }

    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string extensionName = element.GetAbilityName();
    APP_LOGD("bundle name:%{public}s, extension name:%{public}s",
        bundleName.c_str(), extensionName.c_str());
    // explicit query
    if (!bundleName.empty() && !extensionName.empty()) {
        ExtensionAbilityInfo info;
        ErrCode ret = ExplicitQueryExtensionInfoV9(want, flags, requestUserId, info, appIndex);
        if (ret != ERR_OK) {
            APP_LOGE("explicit queryExtensionInfo error");
            return ret;
        }
        extensionInfos.emplace_back(info);
        return ERR_OK;
    }

    ErrCode ret = ImplicitQueryExtensionInfosV9(want, flags, requestUserId, extensionInfos, appIndex);
    if (ret != ERR_OK) {
        APP_LOGE("ImplicitQueryExtensionInfosV9 error");
        return ret;
    }
    if (extensionInfos.empty()) {
        APP_LOGE("no matching abilityInfo");
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    }
    APP_LOGD("QueryExtensionAbilityInfosV9 success");
    return ERR_OK;
}

bool BundleDataMgr::ExplicitQueryExtensionInfo(const Want &want, int32_t flags, int32_t userId,
    ExtensionAbilityInfo &extensionInfo, int32_t appIndex) const
{
    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string moduleName = element.GetModuleName();
    std::string extensionName = element.GetAbilityName();
    APP_LOGD("bundleName:%{public}s, moduleName:%{public}s, abilityName:%{public}s",
        bundleName.c_str(), moduleName.c_str(), extensionName.c_str());
    APP_LOGD("flags:%{public}d, userId:%{public}d", flags, userId);
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    if ((appIndex == 0) && (!GetInnerBundleInfoWithFlags(bundleName, flags, innerBundleInfo, requestUserId))) {
        APP_LOGE("ExplicitQueryExtensionInfo failed");
        return false;
    }
    if (appIndex > 0) {
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return false;
        }
        auto ret = sandboxAppHelper_->GetSandboxAppInfo(bundleName, appIndex, requestUserId, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGW("obtain innerBundleInfo of sandbox app failed due to errCode %{public}d", ret);
            return false;
        }
    }
    auto extension = innerBundleInfo.FindExtensionInfo(moduleName, extensionName);
    if (!extension) {
        APP_LOGE("extensionAbility not found or disabled");
        return false;
    }
    if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_PERMISSION) != GET_ABILITY_INFO_WITH_PERMISSION) {
        extension->permissions.clear();
    }
    if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_METADATA) != GET_ABILITY_INFO_WITH_METADATA) {
        extension->metadata.clear();
    }
    extensionInfo = (*extension);
    if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_APPLICATION) == GET_ABILITY_INFO_WITH_APPLICATION) {
        int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
        innerBundleInfo.GetApplicationInfo(
            ApplicationFlag::GET_BASIC_APPLICATION_INFO |
            ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, responseUserId,
            extensionInfo.applicationInfo);
    }
    return true;
}

ErrCode BundleDataMgr::ExplicitQueryExtensionInfoV9(const Want &want, int32_t flags, int32_t userId,
    ExtensionAbilityInfo &extensionInfo, int32_t appIndex) const
{
    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    std::string moduleName = element.GetModuleName();
    std::string extensionName = element.GetAbilityName();
    APP_LOGD("bundleName:%{public}s, moduleName:%{public}s, abilityName:%{public}s",
        bundleName.c_str(), moduleName.c_str(), extensionName.c_str());
    APP_LOGD("flags:%{public}d, userId:%{public}d", flags, userId);
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    if (appIndex == 0) {
        ErrCode ret = GetInnerBundleInfoWithFlagsV9(bundleName, flags, innerBundleInfo, requestUserId);
        if (ret != ERR_OK) {
            APP_LOGE("ExplicitQueryExtensionInfoV9 failed");
            return ret;
        }
    }
    if (appIndex > 0) {
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
        auto ret = sandboxAppHelper_->GetSandboxAppInfo(bundleName, appIndex, requestUserId, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGW("obtain innerBundleInfo of sandbox app failed due to errCode %{public}d", ret);
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
    }
    auto extension = innerBundleInfo.FindExtensionInfo(moduleName, extensionName);
    if (!extension) {
        APP_LOGE("extensionAbility not found or disabled");
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    }
    if ((static_cast<uint32_t>(flags) &
        static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_PERMISSION)) !=
        static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_PERMISSION)) {
        extension->permissions.clear();
    }
    if ((static_cast<uint32_t>(flags) &
        static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_METADATA)) !=
        static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_METADATA)) {
        extension->metadata.clear();
    }
    extensionInfo = (*extension);
    if ((static_cast<uint32_t>(flags) &
        static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_APPLICATION)) ==
        static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_APPLICATION)) {
        int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
        innerBundleInfo.GetApplicationInfoV9(static_cast<int32_t>(
            GetApplicationFlag::GET_APPLICATION_INFO_DEFAULT), responseUserId, extensionInfo.applicationInfo);
    }
    // set uid for NAPI cache use
    InnerBundleUserInfo innerBundleUserInfo;
    if (innerBundleInfo.GetInnerBundleUserInfo(userId, innerBundleUserInfo)) {
        extensionInfo.uid = innerBundleUserInfo.uid;
    }
    return ERR_OK;
}

void BundleDataMgr::FilterExtensionAbilityInfosByModuleName(const std::string &moduleName,
    std::vector<ExtensionAbilityInfo> &extensionInfos) const
{
    APP_LOGD("BundleDataMgr::FilterExtensionAbilityInfosByModuleName moduleName: %{public}s", moduleName.c_str());
    if (moduleName.empty()) {
        return;
    }
    for (auto iter = extensionInfos.begin(); iter != extensionInfos.end();) {
        if (iter->moduleName != moduleName) {
            iter = extensionInfos.erase(iter);
        } else {
            ++iter;
        }
    }
}

bool BundleDataMgr::ImplicitQueryExtensionInfos(const Want &want, int32_t flags, int32_t userId,
    std::vector<ExtensionAbilityInfo> &extensionInfos, int32_t appIndex) const
{
    if (want.GetAction().empty() && want.GetEntities().empty()
        && want.GetUriString().empty() && want.GetType().empty()) {
        APP_LOGE("param invalid");
        return false;
    }
    APP_LOGD("action:%{public}s, uri:%{private}s, type:%{public}s",
        want.GetAction().c_str(), want.GetUriString().c_str(), want.GetType().c_str());
    APP_LOGD("flags:%{public}d, userId:%{public}d", flags, userId);

    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    std::string bundleName = want.GetElement().GetBundleName();
    if (!bundleName.empty()) {
        // query in current bundle
        if (!ImplicitQueryCurExtensionInfos(want, flags, requestUserId, extensionInfos, appIndex)) {
            APP_LOGE("ImplicitQueryCurExtensionInfos failed");
            return false;
        }
    } else {
        // query all
        ImplicitQueryAllExtensionInfos(want, flags, requestUserId, extensionInfos, appIndex);
    }
    // sort by priority, descending order.
    if (extensionInfos.size() > 1) {
        std::stable_sort(extensionInfos.begin(), extensionInfos.end(),
            [](ExtensionAbilityInfo a, ExtensionAbilityInfo b) { return a.priority > b.priority; });
    }
    return true;
}

ErrCode BundleDataMgr::ImplicitQueryExtensionInfosV9(const Want &want, int32_t flags, int32_t userId,
    std::vector<ExtensionAbilityInfo> &extensionInfos, int32_t appIndex) const
{
    if (want.GetAction().empty() && want.GetEntities().empty()
        && want.GetUriString().empty() && want.GetType().empty()) {
        APP_LOGE("param invalid");
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    }
    APP_LOGD("action:%{public}s, uri:%{private}s, type:%{public}s",
        want.GetAction().c_str(), want.GetUriString().c_str(), want.GetType().c_str());
    APP_LOGD("flags:%{public}d, userId:%{public}d", flags, userId);

    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    std::string bundleName = want.GetElement().GetBundleName();
    if (!bundleName.empty()) {
        // query in current bundle
        ErrCode ret = ImplicitQueryCurExtensionInfosV9(want, flags, requestUserId, extensionInfos, appIndex);
        if (ret != ERR_OK) {
            APP_LOGE("ImplicitQueryCurExtensionInfos failed");
            return ret;
        }
    } else {
        // query all
        ImplicitQueryAllExtensionInfosV9(want, flags, requestUserId, extensionInfos, appIndex);
    }
    // sort by priority, descending order.
    if (extensionInfos.size() > 1) {
        std::stable_sort(extensionInfos.begin(), extensionInfos.end(),
            [](ExtensionAbilityInfo a, ExtensionAbilityInfo b) { return a.priority > b.priority; });
    }
    return ERR_OK;
}

bool BundleDataMgr::ImplicitQueryCurExtensionInfos(const Want &want, int32_t flags, int32_t userId,
    std::vector<ExtensionAbilityInfo> &infos, int32_t appIndex) const
{
    APP_LOGD("begin to ImplicitQueryCurExtensionInfos");
    std::string bundleName = want.GetElement().GetBundleName();
    InnerBundleInfo innerBundleInfo;
    if ((appIndex == 0) && (!GetInnerBundleInfoWithFlags(bundleName, flags, innerBundleInfo, userId))) {
        APP_LOGE("ImplicitQueryExtensionAbilityInfos failed");
        return false;
    }
    if (appIndex > 0) {
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return false;
        }
        auto ret = sandboxAppHelper_->GetSandboxAppInfo(bundleName, appIndex, userId, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGW("obtain innerBundleInfo of sandbox app failed due to errCode %{public}d", ret);
            return false;
        }
    }
    int32_t responseUserId = innerBundleInfo.GetResponseUserId(userId);
    GetMatchExtensionInfos(want, flags, responseUserId, innerBundleInfo, infos);
    FilterExtensionAbilityInfosByModuleName(want.GetElement().GetModuleName(), infos);
    APP_LOGD("finish to ImplicitQueryCurExtensionInfos");
    return true;
}

ErrCode BundleDataMgr::ImplicitQueryCurExtensionInfosV9(const Want &want, int32_t flags, int32_t userId,
    std::vector<ExtensionAbilityInfo> &infos, int32_t appIndex) const
{
    APP_LOGD("begin to ImplicitQueryCurExtensionInfosV9");
    std::string bundleName = want.GetElement().GetBundleName();
    InnerBundleInfo innerBundleInfo;
    if (appIndex == 0) {
        ErrCode ret = GetInnerBundleInfoWithFlagsV9(bundleName, flags, innerBundleInfo, userId);
        if (ret != ERR_OK) {
            APP_LOGE("GetInnerBundleInfoWithFlagsV9 failed");
            return ret;
        }
    }
    if (appIndex > 0) {
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
        auto ret = sandboxAppHelper_->GetSandboxAppInfo(bundleName, appIndex, userId, innerBundleInfo);
        if (ret != ERR_OK) {
            APP_LOGW("obtain innerBundleInfo of sandbox app failed due to errCode %{public}d", ret);
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
    }
    int32_t responseUserId = innerBundleInfo.GetResponseUserId(userId);
    GetMatchExtensionInfosV9(want, flags, responseUserId, innerBundleInfo, infos);
    FilterExtensionAbilityInfosByModuleName(want.GetElement().GetModuleName(), infos);
    APP_LOGD("finish to ImplicitQueryCurExtensionInfosV9");
    return ERR_OK;
}

void BundleDataMgr::ImplicitQueryAllExtensionInfos(const Want &want, int32_t flags, int32_t userId,
    std::vector<ExtensionAbilityInfo> &infos, int32_t appIndex) const
{
    APP_LOGD("begin to ImplicitQueryAllExtensionInfos");
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        APP_LOGE("invalid userId");
        return;
    }

    // query from bundleInfos_
    if (appIndex == 0) {
        for (const auto &item : bundleInfos_) {
            const InnerBundleInfo &innerBundleInfo = item.second;
            int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
            if (CheckInnerBundleInfoWithFlags(innerBundleInfo, flags, responseUserId) != ERR_OK) {
                continue;
            }
            GetMatchExtensionInfos(want, flags, responseUserId, innerBundleInfo, infos);
        }
    } else {
        // query from sandbox manager for sandbox bundle
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return;
        }
        auto sandboxMap = sandboxAppHelper_->GetSandboxAppInfoMap();
        for (const auto &item : sandboxMap) {
            InnerBundleInfo info;
            size_t pos = item.first.rfind(Constants::FILE_UNDERLINE);
            if (pos == std::string::npos) {
                APP_LOGW("sandbox map contains invalid element");
                continue;
            }
            std::string innerBundleName = item.first.substr(0, pos);
            if (sandboxAppHelper_->GetSandboxAppInfo(innerBundleName, appIndex, userId, info) != ERR_OK) {
                APP_LOGW("obtain innerBundleInfo of sandbox app failed");
                continue;
            }

            int32_t responseUserId = info.GetResponseUserId(userId);
            GetMatchExtensionInfos(want, flags, responseUserId, info, infos);
        }
    }
    APP_LOGD("finish to ImplicitQueryAllExtensionInfos");
}

void BundleDataMgr::ImplicitQueryAllExtensionInfosV9(const Want &want, int32_t flags, int32_t userId,
    std::vector<ExtensionAbilityInfo> &infos, int32_t appIndex) const
{
    APP_LOGD("begin to ImplicitQueryAllExtensionInfosV9");
    // query from bundleInfos_
    if (appIndex == 0) {
        for (const auto &item : bundleInfos_) {
            InnerBundleInfo innerBundleInfo;
            ErrCode ret = GetInnerBundleInfoWithFlagsV9(item.first, flags, innerBundleInfo, userId);
            if (ret != ERR_OK) {
                APP_LOGE("ImplicitQueryExtensionAbilityInfos failed");
                continue;
            }
            int32_t responseUserId = innerBundleInfo.GetResponseUserId(userId);
            GetMatchExtensionInfosV9(want, flags, responseUserId, innerBundleInfo, infos);
        }
    } else {
        // query from sandbox manager for sandbox bundle
        if (sandboxAppHelper_ == nullptr) {
            APP_LOGE("sandboxAppHelper_ is nullptr");
            return;
        }
        auto sandboxMap = sandboxAppHelper_->GetSandboxAppInfoMap();
        for (const auto &item : sandboxMap) {
            InnerBundleInfo info;
            size_t pos = item.first.rfind(Constants::FILE_UNDERLINE);
            if (pos == std::string::npos) {
                APP_LOGW("sandbox map contains invalid element");
                continue;
            }
            std::string innerBundleName = item.first.substr(0, pos);
            if (sandboxAppHelper_->GetSandboxAppInfo(innerBundleName, appIndex, userId, info) != ERR_OK) {
                APP_LOGW("obtain innerBundleInfo of sandbox app failed");
                continue;
            }

            int32_t responseUserId = info.GetResponseUserId(userId);
            GetMatchExtensionInfosV9(want, flags, responseUserId, info, infos);
        }
    }
    APP_LOGD("finish to ImplicitQueryAllExtensionInfosV9");
}

void BundleDataMgr::GetMatchExtensionInfos(const Want &want, int32_t flags, const int32_t &userId,
    const InnerBundleInfo &info, std::vector<ExtensionAbilityInfo> &infos) const
{
    auto extensionSkillInfos = info.GetExtensionSkillInfos();
    auto extensionInfos = info.GetInnerExtensionInfos();
    for (const auto &skillInfos : extensionSkillInfos) {
        for (const auto &skill : skillInfos.second) {
            if (!skill.Match(want)) {
                continue;
            }
            if (extensionInfos.find(skillInfos.first) == extensionInfos.end()) {
                APP_LOGW("cannot find the extension info with %{public}s", skillInfos.first.c_str());
                break;
            }
            ExtensionAbilityInfo extensionInfo = extensionInfos[skillInfos.first];
            if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_APPLICATION) ==
                GET_ABILITY_INFO_WITH_APPLICATION) {
                info.GetApplicationInfo(
                    ApplicationFlag::GET_BASIC_APPLICATION_INFO |
                    ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, userId,
                    extensionInfo.applicationInfo);
            }
            if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_PERMISSION) !=
                GET_ABILITY_INFO_WITH_PERMISSION) {
                extensionInfo.permissions.clear();
            }
            if ((static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_METADATA) != GET_ABILITY_INFO_WITH_METADATA) {
                extensionInfo.metadata.clear();
            }
            infos.emplace_back(extensionInfo);
            break;
        }
    }
}

void BundleDataMgr::GetMatchExtensionInfosV9(const Want &want, int32_t flags, int32_t userId,
    const InnerBundleInfo &info, std::vector<ExtensionAbilityInfo> &infos) const
{
    auto extensionSkillInfos = info.GetExtensionSkillInfos();
    auto extensionInfos = info.GetInnerExtensionInfos();
    for (const auto &skillInfos : extensionSkillInfos) {
        for (const auto &skill : skillInfos.second) {
            if (!skill.Match(want)) {
                continue;
            }
            if (extensionInfos.find(skillInfos.first) == extensionInfos.end()) {
                APP_LOGW("cannot find the extension info with %{public}s", skillInfos.first.c_str());
                break;
            }
            ExtensionAbilityInfo extensionInfo = extensionInfos[skillInfos.first];
            if ((static_cast<uint32_t>(flags) &
                static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_APPLICATION)) ==
                static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_APPLICATION)) {
                info.GetApplicationInfoV9(static_cast<int32_t>(
                    GetApplicationFlag::GET_APPLICATION_INFO_DEFAULT), userId, extensionInfo.applicationInfo);
            }
            if ((static_cast<uint32_t>(flags) &
                static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_PERMISSION)) !=
                static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_PERMISSION)) {
                extensionInfo.permissions.clear();
            }
            if ((static_cast<uint32_t>(flags) &
                static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_METADATA)) !=
                static_cast<int32_t>(GetExtensionAbilityInfoFlag::GET_EXTENSION_ABILITY_INFO_WITH_METADATA)) {
                extensionInfo.metadata.clear();
            }
            infos.emplace_back(extensionInfo);
            break;
        }
    }
}

bool BundleDataMgr::QueryExtensionAbilityInfos(const ExtensionAbilityType &extensionType, const int32_t &userId,
    std::vector<ExtensionAbilityInfo> &extensionInfos) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        return false;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &innerBundleInfo = item.second;
        int32_t responseUserId = innerBundleInfo.GetResponseUserId(requestUserId);
        if (CheckInnerBundleInfoWithFlags(innerBundleInfo, 0, responseUserId) != ERR_OK) {
            continue;
        }
        auto innerExtensionInfos = innerBundleInfo.GetInnerExtensionInfos();
        for (const auto &info : innerExtensionInfos) {
            if (info.second.type == extensionType) {
                ExtensionAbilityInfo extensionAbilityInfo = info.second;
                innerBundleInfo.GetApplicationInfo(
                    ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, responseUserId,
                    extensionAbilityInfo.applicationInfo);
                extensionInfos.emplace_back(extensionAbilityInfo);
            }
        }
    }
    return true;
}

bool BundleDataMgr::QueryExtensionAbilityInfoByUri(const std::string &uri, int32_t userId,
    ExtensionAbilityInfo &extensionAbilityInfo) const
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        APP_LOGE("invalid userId -1");
        return false;
    }
    if (uri.empty()) {
        APP_LOGE("uri empty");
        return false;
    }
    // example of valid param uri : fileShare:///com.example.FileShare/person/10
    // example of convertUri : fileShare://com.example.FileShare
    size_t schemePos = uri.find(Constants::PARAM_URI_SEPARATOR);
    if (schemePos == uri.npos) {
        APP_LOGE("uri not include :///, invalid");
        return false;
    }
    size_t cutPos = uri.find(Constants::SEPARATOR, schemePos + Constants::PARAM_URI_SEPARATOR_LEN);
    // 1. cut string
    std::string convertUri = uri;
    if (cutPos != uri.npos) {
        convertUri = uri.substr(0, cutPos);
    }
    // 2. replace :/// with ://
    convertUri.replace(schemePos, Constants::PARAM_URI_SEPARATOR_LEN,
        Constants::URI_SEPARATOR);
    APP_LOGD("convertUri : %{private}s", convertUri.c_str());

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ data is empty");
        return false;
    }
    for (const auto &item : bundleInfos_) {
        const InnerBundleInfo &info = item.second;
        if (info.IsDisabled()) {
            APP_LOGE("app %{public}s is disabled", info.GetBundleName().c_str());
            continue;
        }

        int32_t responseUserId = info.GetResponseUserId(requestUserId);
        if (!info.GetApplicationEnabled(responseUserId)) {
            continue;
        }

        bool ret = info.FindExtensionAbilityInfoByUri(convertUri, extensionAbilityInfo);
        if (!ret) {
            continue;
        }
        info.GetApplicationInfo(
            ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, responseUserId,
            extensionAbilityInfo.applicationInfo);
        return true;
    }
    APP_LOGE("QueryExtensionAbilityInfoByUri (%{private}s) failed.", uri.c_str());
    return false;
}

void BundleDataMgr::GetAllUriPrefix(std::vector<std::string> &uriPrefixList, int32_t userId,
    const std::string &excludeModule) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    APP_LOGD("begin to GetAllUriPrefix, userId : %{public}d, excludeModule : %{public}s",
        userId, excludeModule.c_str());
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ is empty");
        return;
    }
    for (const auto &item : bundleInfos_) {
        item.second.GetUriPrefixList(uriPrefixList, userId, excludeModule);
        item.second.GetUriPrefixList(uriPrefixList, Constants::DEFAULT_USERID, excludeModule);
    }
}

std::string BundleDataMgr::GetStringById(const std::string &bundleName, const std::string &moduleName,
    uint32_t resId, int32_t userId, const std::string &localeInfo)
{
    APP_LOGD("GetStringById:%{public}s , %{public}s, %{public}d", bundleName.c_str(), moduleName.c_str(), resId);
#ifdef GLOBAL_RESMGR_ENABLE
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    std::shared_ptr<OHOS::Global::Resource::ResourceManager> resourceManager =
        GetResourceManager(bundleName, moduleName, userId);
    if (resourceManager == nullptr) {
        APP_LOGE("InitResourceManager failed");
        return Constants::EMPTY_STRING;
    }
    std::string label;
    OHOS::Global::Resource::RState errValue = resourceManager->GetStringById(resId, label);
    if (errValue != OHOS::Global::Resource::RState::SUCCESS) {
        APP_LOGE("GetStringById failed");
        return Constants::EMPTY_STRING;
    }
    return label;
#else
    APP_LOGW("GLOBAL_RESMGR_ENABLE is false");
    return Constants::EMPTY_STRING;
#endif
}

std::string BundleDataMgr::GetIconById(
    const std::string &bundleName, const std::string &moduleName, uint32_t resId, uint32_t density, int32_t userId)
{
    APP_LOGI("GetIconById bundleName:%{public}s, moduleName:%{public}s, resId:%{public}d, density:%{public}d",
        bundleName.c_str(), moduleName.c_str(), resId, density);
#ifdef GLOBAL_RESMGR_ENABLE
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    std::shared_ptr<OHOS::Global::Resource::ResourceManager> resourceManager =
        GetResourceManager(bundleName, moduleName, userId);
    if (resourceManager == nullptr) {
        APP_LOGE("InitResourceManager failed");
        return Constants::EMPTY_STRING;
    }
    std::string base64;
    OHOS::Global::Resource::RState errValue = resourceManager->GetMediaBase64DataById(resId, base64, density);
    if (errValue != OHOS::Global::Resource::RState::SUCCESS) {
        APP_LOGE("GetIconById failed");
        return Constants::EMPTY_STRING;
    }
    return base64;
#else
    APP_LOGW("GLOBAL_RESMGR_ENABLE is false");
    return Constants::EMPTY_STRING;
#endif
}

#ifdef GLOBAL_RESMGR_ENABLE
std::shared_ptr<Global::Resource::ResourceManager> BundleDataMgr::GetResourceManager(
    const std::string &bundleName, const std::string &moduleName, int32_t userId, const std::string &localeInfo) const
{
    InnerBundleInfo innerBundleInfo;
    if (!GetInnerBundleInfoWithFlags(bundleName, BundleFlag::GET_BUNDLE_DEFAULT, innerBundleInfo, userId)) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return nullptr;
    }
    int32_t responseUserId = innerBundleInfo.GetResponseUserId(userId);
    BundleInfo bundleInfo;
    innerBundleInfo.GetBundleInfo(BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo, responseUserId);
    std::shared_ptr<Global::Resource::ResourceManager> resourceManager(Global::Resource::CreateResourceManager());
    for (auto hapModuleInfo : bundleInfo.hapModuleInfos) {
        std::string moduleResPath;
        if (moduleName.empty() || moduleName == hapModuleInfo.moduleName) {
            moduleResPath = hapModuleInfo.hapPath.empty() ? hapModuleInfo.resourcePath : hapModuleInfo.hapPath;
        }
        if (!moduleResPath.empty()) {
            APP_LOGD("DistributedBms::InitResourceManager, moduleResPath: %{private}s", moduleResPath.c_str());
            if (!resourceManager->AddResource(moduleResPath.c_str())) {
                APP_LOGW("DistributedBms::InitResourceManager AddResource failed");
            }
        }
    }

    std::unique_ptr<Global::Resource::ResConfig> resConfig(Global::Resource::CreateResConfig());
#ifdef GLOBAL_I18_ENABLE
    std::map<std::string, std::string> configs;
    OHOS::Global::I18n::LocaleInfo locale(localeInfo, configs);
    resConfig->SetLocaleInfo(locale.GetLanguage().c_str(), locale.GetScript().c_str(), locale.GetRegion().c_str());
#endif
    resourceManager->UpdateResConfig(*resConfig);
    return resourceManager;
}
#endif

const std::vector<PreInstallBundleInfo>& BundleDataMgr::GetAllPreInstallBundleInfos()
{
    std::lock_guard<std::mutex> lock(preInstallInfoMutex_);
    return preInstallBundleInfos_;
}

bool BundleDataMgr::ImplicitQueryInfoByPriority(const Want &want, int32_t flags, int32_t userId,
    AbilityInfo &abilityInfo, ExtensionAbilityInfo &extensionInfo)
{
    int32_t requestUserId = GetUserId(userId);
    if (requestUserId == Constants::INVALID_USERID) {
        APP_LOGE("invalid userId");
        return false;
    }
    std::vector<AbilityInfo> abilityInfos;
    bool abilityValid =
        ImplicitQueryAbilityInfos(want, flags, requestUserId, abilityInfos) && (abilityInfos.size() > 0);
    std::vector<ExtensionAbilityInfo> extensionInfos;
    bool extensionValid =
        ImplicitQueryExtensionInfos(want, flags, requestUserId, extensionInfos) && (extensionInfos.size() > 0);
    if (!abilityValid && !extensionValid) {
        // both invalid
        APP_LOGE("can't find target AbilityInfo or ExtensionAbilityInfo");
        return false;
    }
    if (abilityValid && extensionValid) {
        // both valid
        if (abilityInfos[0].priority >= extensionInfos[0].priority) {
            APP_LOGD("find target AbilityInfo with higher priority, name : %{public}s", abilityInfos[0].name.c_str());
            abilityInfo = abilityInfos[0];
        } else {
            APP_LOGD("find target ExtensionAbilityInfo with higher priority, name : %{public}s",
                extensionInfos[0].name.c_str());
            extensionInfo = extensionInfos[0];
        }
    } else if (abilityValid) {
        // only ability valid
        APP_LOGD("find target AbilityInfo, name : %{public}s", abilityInfos[0].name.c_str());
        abilityInfo = abilityInfos[0];
    } else {
        // only extension valid
        APP_LOGD("find target ExtensionAbilityInfo, name : %{public}s", extensionInfos[0].name.c_str());
        extensionInfo = extensionInfos[0];
    }
    return true;
}

bool BundleDataMgr::ImplicitQueryInfos(const Want &want, int32_t flags, int32_t userId, bool isReturnDefaultSetting,
    std::vector<AbilityInfo> &abilityInfos, std::vector<ExtensionAbilityInfo> &extensionInfos)
{
    // step1 : find default infos, current only support default file types
#ifdef BUNDLE_FRAMEWORK_DEFAULT_APP
    if (isReturnDefaultSetting) {
        std::string action = want.GetAction();
        std::string uri = want.GetUriString();
        std::string type = want.GetType();
        APP_LOGD("action : %{public}s, uri : %{public}s, type : %{public}s", action.c_str(), uri.c_str(), type.c_str());
        if (action == Constants::ACTION_VIEW_DATA && !type.empty() && want.GetEntities().empty() && uri.empty()) {
            BundleInfo bundleInfo;
            ErrCode ret = DefaultAppMgr::GetInstance().GetDefaultApplication(userId, type, bundleInfo);
            if (ret == ERR_OK) {
                if (bundleInfo.abilityInfos.size() == 1) {
                    abilityInfos = bundleInfo.abilityInfos;
                    APP_LOGD("find default ability.");
                    return true;
                } else if (bundleInfo.extensionInfos.size() == 1) {
                    extensionInfos = bundleInfo.extensionInfos;
                    APP_LOGD("find default extension.");
                    return true;
                } else {
                    APP_LOGD("GetDefaultApplication failed.");
                }
            }
        }
    }
#endif
    // step2 : implicit query infos
    bool abilityRet =
        ImplicitQueryAbilityInfos(want, flags, userId, abilityInfos) && (abilityInfos.size() > 0);
    APP_LOGD("abilityRet: %{public}d, abilityInfos size: %{public}zu", abilityRet, abilityInfos.size());

    bool extensionRet =
        ImplicitQueryExtensionInfos(want, flags, userId, extensionInfos) && (extensionInfos.size() > 0);
    APP_LOGD("extensionRet: %{public}d, extensionInfos size: %{public}zu", extensionRet, extensionInfos.size());
    return abilityRet || extensionRet;
}

bool BundleDataMgr::GetAllDependentModuleNames(const std::string &bundleName, const std::string &moduleName,
    std::vector<std::string> &dependentModuleNames)
{
    APP_LOGD("GetAllDependentModuleNames bundleName: %{public}s, moduleName: %{public}s",
        bundleName.c_str(), moduleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto item = bundleInfos_.find(bundleName);
    if (item == bundleInfos_.end()) {
        APP_LOGE("GetAllDependentModuleNames: bundleName not find");
        return false;
    }
    const InnerBundleInfo &innerBundleInfo = item->second;
    return innerBundleInfo.GetAllDependentModuleNames(moduleName, dependentModuleNames);
}

void BundleDataMgr::UpdateRemovable(
    const std::string &bundleName, bool removable)
{
    APP_LOGD("UpdateRemovable %{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("bundleName is empty");
        return;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return;
    }

    if (infoItem->second.IsRemovable() != removable) {
        infoItem->second.UpdateRemovable(true, removable);
        SaveInnerBundleInfo(infoItem->second);
    }
}

void BundleDataMgr::UpdatePrivilegeCapability(
    const std::string &bundleName, const ApplicationInfo &appInfo)
{
    APP_LOGD("UpdatePrivilegeCapability %{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("bundleName is empty");
        return;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return;
    }

    infoItem->second.UpdatePrivilegeCapability(appInfo);
}

bool BundleDataMgr::FetchInnerBundleInfo(
    const std::string &bundleName, InnerBundleInfo &innerBundleInfo)
{
    APP_LOGD("FetchInnerBundleInfo %{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("bundleName is empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return false;
    }

    innerBundleInfo = infoItem->second;
    return true;
}

#ifdef BUNDLE_FRAMEWORK_DEFAULT_APP
bool BundleDataMgr::QueryInfoAndSkillsByElement(int32_t userId, const Element& element,
    AbilityInfo& abilityInfo, ExtensionAbilityInfo& extensionInfo, std::vector<Skill>& skills) const
{
    APP_LOGD("begin to QueryInfoAndSkillsByElement.");
    const std::string& bundleName = element.bundleName;
    const std::string& moduleName = element.moduleName;
    const std::string& abilityName = element.abilityName;
    const std::string& extensionName = element.extensionName;
    Want want;
    ElementName elementName("", bundleName, abilityName, moduleName);
    want.SetElement(elementName);
    bool isAbility = !element.abilityName.empty();
    bool ret = false;
    if (isAbility) {
        // get ability info
        ret = ExplicitQueryAbilityInfo(want, GET_ABILITY_INFO_DEFAULT, userId, abilityInfo);
        if (!ret) {
            APP_LOGE("ExplicitQueryAbilityInfo failed.");
            return false;
        }
    } else {
        // get extension info
        elementName.SetAbilityName(extensionName);
        want.SetElement(elementName);
        ret = ExplicitQueryExtensionInfo(want, GET_EXTENSION_INFO_DEFAULT, userId, extensionInfo);
        if (!ret) {
            APP_LOGE("ExplicitQueryExtensionInfo failed.");
            return false;
        }
    }

    // get skills info
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    if (bundleInfos_.empty()) {
        APP_LOGE("bundleInfos_ is empty.");
        return false;
    }
    auto item = bundleInfos_.find(bundleName);
    if (item == bundleInfos_.end()) {
        APP_LOGE("can't find bundleName : %{public}s.", bundleName.c_str());
        return false;
    }
    const InnerBundleInfo& innerBundleInfo = item->second;
    if (isAbility) {
        std::string key;
        key.append(bundleName).append(".").append(abilityInfo.package).append(".").append(abilityName);
        APP_LOGD("begin to find ability skills, key : %{public}s.", key.c_str());
        for (const auto& infoItem : innerBundleInfo.GetInnerSkillInfos()) {
            if (infoItem.first == key) {
                skills = infoItem.second;
                APP_LOGD("find ability skills success.");
                break;
            }
        }
    } else {
        std::string key;
        key.append(bundleName).append(".").append(moduleName).append(".").append(extensionName);
        APP_LOGD("begin to find extension skills, key : %{public}s.", key.c_str());
        for (const auto& infoItem : innerBundleInfo.GetExtensionSkillInfos()) {
            if (infoItem.first == key) {
                skills = infoItem.second;
                APP_LOGD("find extension skills success.");
                break;
            }
        }
    }
    APP_LOGD("QueryInfoAndSkillsByElement success.");
    return true;
}

bool BundleDataMgr::GetElement(int32_t userId, const ElementName& elementName, Element& element) const
{
    APP_LOGD("begin to GetElement.");
    const std::string& bundleName = elementName.GetBundleName();
    const std::string& moduleName = elementName.GetModuleName();
    const std::string& abilityName = elementName.GetAbilityName();
    if (bundleName.empty() || moduleName.empty() || abilityName.empty()) {
        APP_LOGE("bundleName or moduleName or abilityName is empty.");
        return false;
    }
    Want want;
    want.SetElement(elementName);
    AbilityInfo abilityInfo;
    bool ret = ExplicitQueryAbilityInfo(want, GET_ABILITY_INFO_DEFAULT, userId, abilityInfo);
    if (ret) {
        APP_LOGD("ElementName is ability.");
        element.bundleName = bundleName;
        element.moduleName = moduleName;
        element.abilityName = abilityName;
        return true;
    }

    ExtensionAbilityInfo extensionInfo;
    ret = ExplicitQueryExtensionInfo(want, GET_EXTENSION_INFO_DEFAULT, userId, extensionInfo);
    if (ret) {
        APP_LOGD("ElementName is extension.");
        element.bundleName = bundleName;
        element.moduleName = moduleName;
        element.extensionName = abilityName;
        return true;
    }

    APP_LOGE("ElementName doesn't exist.");
    return false;
}
#endif

ErrCode BundleDataMgr::GetMediaData(const std::string &bundleName, const std::string &moduleName,
    const std::string &abilityName, std::unique_ptr<uint8_t[]> &mediaDataPtr, size_t &len, int32_t userId) const
{
    APP_LOGI("begin to GetMediaData.");
#ifdef GLOBAL_RESMGR_ENABLE
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    InnerBundleInfo innerBundleInfo;
    ErrCode errCode = GetInnerBundleInfoWithFlagsV9(
        bundleName, BundleFlag::GET_BUNDLE_DEFAULT, innerBundleInfo, GetUserId(userId));
    if (errCode != ERR_OK) {
        return errCode;
    }
    AbilityInfo abilityInfo;
    if (moduleName.empty()) {
        auto ability = innerBundleInfo.FindAbilityInfoV9(moduleName, abilityName);
        if (!ability) {
            return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
        }
        abilityInfo = *ability;
    } else {
        errCode = innerBundleInfo.FindAbilityInfo(moduleName, abilityName, abilityInfo);
        if (errCode != ERR_OK) {
            APP_LOGE("%{public}s:FindAbilityInfo failed: %{public}d", bundleName.c_str(), errCode);
            return errCode;
        }
    }
    bool isEnable;
    errCode = innerBundleInfo.IsAbilityEnabledV9(abilityInfo, GetUserId(userId), isEnable);
    if (errCode != ERR_OK) {
        return errCode;
    }
    if (!isEnable) {
        APP_LOGE("%{public}s ability disabled: %{public}s", bundleName.c_str(), abilityName.c_str());
        return ERR_BUNDLE_MANAGER_ABILITY_DISABLED;
    }
    std::shared_ptr<Global::Resource::ResourceManager> resourceManager =
        GetResourceManager(bundleName, abilityInfo.moduleName, GetUserId(userId));
    if (resourceManager == nullptr) {
        APP_LOGE("InitResourceManager failed");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    OHOS::Global::Resource::RState ret =
        resourceManager->GetMediaDataById(static_cast<uint32_t>(abilityInfo.iconId), len, mediaDataPtr);
    if (ret != OHOS::Global::Resource::RState::SUCCESS || mediaDataPtr == nullptr || len == 0) {
        APP_LOGE("GetMediaDataById failed");
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    return ERR_OK;
#else
    APP_LOGW("GLOBAL_RES_MGR_ENABLE is false");
    return ERR_BUNDLE_MANAGER_GLOBAL_RES_MGR_ENABLE_DISABLED;
#endif
}

std::shared_mutex &BundleDataMgr::GetStatusCallbackMutex()
{
    return callbackMutex_;
}

std::vector<sptr<IBundleStatusCallback>> BundleDataMgr::GetCallBackList() const
{
    return callbackList_;
}

bool BundleDataMgr::UpdateQuickFixInnerBundleInfo(const std::string &bundleName,
    const InnerBundleInfo &innerBundleInfo)
{
    APP_LOGD("to update info:%{public}s", bundleName.c_str());
    if (bundleName.empty()) {
        APP_LOGE("update info fail, empty bundle name");
        return false;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundle info is not existed");
        return false;
    }

    if (dataStorage_->SaveStorageBundleInfo(innerBundleInfo)) {
        bundleInfos_.at(bundleName) = innerBundleInfo;
        return true;
    }
    APP_LOGE("to update info:%{public}s failed", bundleName.c_str());
    return false;
}

bool BundleDataMgr::UpdateInnerBundleInfo(const InnerBundleInfo &innerBundleInfo)
{
    std::string bundleName = innerBundleInfo.GetBundleName();
    if (bundleName.empty()) {
        APP_LOGE("UpdateInnerBundleInfo failed, empty bundle name");
        return false;
    }
    APP_LOGD("UpdateInnerBundleInfo:%{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundle info is not existed");
        return false;
    }

    if (dataStorage_->SaveStorageBundleInfo(innerBundleInfo)) {
        bundleInfos_.at(bundleName) = innerBundleInfo;
        return true;
    }
    APP_LOGE("to update InnerBundleInfo:%{public}s failed", bundleName.c_str());
    return false;
}

bool BundleDataMgr::GetOverlayInnerBundleInfo(const std::string &bundleName, InnerBundleInfo &info)
{
    APP_LOGI("start to get overlay innerBundleInfo");
    std::lock_guard<std::mutex> lock(overlayMutex_);
    if (bundleInfos_.find(bundleName) != bundleInfos_.end()) {
        bundleInfos_.at(bundleName).SetBundleStatus(InnerBundleInfo::BundleStatus::DISABLED);
        info = bundleInfos_.at(bundleName);
        return true;
    }

    APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
    return false;
}

bool BundleDataMgr::QueryOverlayInnerBundleInfo(const std::string &bundleName, InnerBundleInfo &info)
{
    APP_LOGI("start to query overlay innerBundleInfo");
    std::lock_guard<std::mutex> lock(overlayMutex_);
    if (bundleInfos_.find(bundleName) != bundleInfos_.end()) {
        info = bundleInfos_.at(bundleName);
        return true;
    }

    APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
    return false;
}

void BundleDataMgr::SaveOverlayInfo(const std::string &bundleName, InnerBundleInfo &innerBundleInfo)
{
    std::lock_guard<std::mutex> lock(overlayMutex_);
    innerBundleInfo.SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
    if (!dataStorage_->SaveStorageBundleInfo(innerBundleInfo)) {
        APP_LOGE("update storage failed bundle:%{public}s", bundleName.c_str());
        return;
    }
    bundleInfos_.at(bundleName) = innerBundleInfo;
}

void BundleDataMgr::EnableOverlayBundle(const std::string &bundleName)
{
    std::lock_guard<std::mutex> lock(overlayMutex_);
    if (bundleInfos_.find(bundleName) != bundleInfos_.end()) {
        bundleInfos_.at(bundleName).SetBundleStatus(InnerBundleInfo::BundleStatus::ENABLED);
        return;
    }
    APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
}

const std::map<std::string, InnerBundleInfo> &BundleDataMgr::GetAllOverlayInnerbundleInfos() const
{
    std::lock_guard<std::mutex> lock(overlayMutex_);
    return bundleInfos_;
}

ErrCode BundleDataMgr::GetAppProvisionInfo(const std::string &bundleName, int32_t userId,
    AppProvisionInfo &appProvisionInfo)
{
    if (!HasUserId(userId)) {
        APP_LOGE("GetAppProvisionInfo user is not existed.");
        return ERR_BUNDLE_MANAGER_INVALID_USER_ID;
    }
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    if (infoItem->second.GetApplicationBundleType() != BundleType::SHARED) {
        int32_t responseUserId = infoItem->second.GetResponseUserId(userId);
        if (responseUserId == Constants::INVALID_USERID) {
            return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
        }
    }
    if (!DelayedSingleton<AppProvisionInfoManager>::GetInstance()->GetAppProvisionInfo(bundleName, appProvisionInfo)) {
        APP_LOGE("bundleName:%{public}s GetAppProvisionInfo failed.", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    return ERR_OK;
}

ErrCode BundleDataMgr::GetProvisionMetadata(const std::string &bundleName, int32_t userId,
    std::vector<Metadata> &provisionMetadatas) const
{
    // Reserved interface
    return ERR_OK;
}

ErrCode BundleDataMgr::GetAllSharedBundleInfo(std::vector<SharedBundleInfo> &sharedBundles) const
{
    APP_LOGD("GetAllSharedBundleInfo");
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);

    for (const auto& [key, innerBundleInfo] : bundleInfos_) {
        if (innerBundleInfo.GetApplicationBundleType() != BundleType::SHARED) {
            continue;
        }
        SharedBundleInfo sharedBundleInfo;
        innerBundleInfo.GetSharedBundleInfo(sharedBundleInfo);
        sharedBundles.emplace_back(sharedBundleInfo);
    }

    return ERR_OK;
}

ErrCode BundleDataMgr::GetSharedBundleInfo(const std::string &bundleName, const std::string &moduleName,
    std::vector<SharedBundleInfo> &sharedBundles)
{
    APP_LOGD("GetSharedBundleInfo");
    if (bundleName.empty() || moduleName.empty()) {
        APP_LOGE("bundleName or moduleName is empty");
        return ERR_BUNDLE_MANAGER_PARAM_ERROR;
    }

    std::vector<Dependency> dependencies;
    ErrCode errCode = GetSharedDependencies(bundleName, moduleName, dependencies);
    if (errCode != ERR_OK) {
        APP_LOGE("GetSharedDependencies failed errCode is %{public}d", errCode);
        return errCode;
    }

    for (const auto& dep : dependencies) {
        SharedBundleInfo sharedBundleInfo;
        errCode = GetSharedBundleInfoBySelf(dep.bundleName, sharedBundleInfo);
        if (errCode != ERR_OK) {
            APP_LOGE("GetSharedBundleInfoBySelf failed errCode is %{public}d", errCode);
            return errCode;
        }
        sharedBundles.emplace_back(sharedBundleInfo);
    }

    return ERR_OK;
}

ErrCode BundleDataMgr::GetSharedBundleInfoBySelf(const std::string &bundleName, SharedBundleInfo &sharedBundleInfo)
{
    APP_LOGD("GetSharedBundleInfoBySelf bundleName: %{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("GetSharedBundleInfoBySelf failed, can not find bundle %{public}s",
            bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    const InnerBundleInfo &innerBundleInfo = infoItem->second;
    if (innerBundleInfo.GetApplicationBundleType() != BundleType::SHARED) {
        APP_LOGE("GetSharedBundleInfoBySelf failed, the bundle(%{public}s) is not shared library",
            bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    innerBundleInfo.GetSharedBundleInfo(sharedBundleInfo);
    APP_LOGD("GetSharedBundleInfoBySelf(%{public}s) successfully)", bundleName.c_str());
    return ERR_OK;
}

ErrCode BundleDataMgr::GetSharedDependencies(const std::string &bundleName, const std::string &moduleName,
    std::vector<Dependency> &dependencies)
{
    APP_LOGD("GetSharedDependencies bundleName: %{public}s, moduleName: %{public}s",
        bundleName.c_str(), moduleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto item = bundleInfos_.find(bundleName);
    if (item == bundleInfos_.end()) {
        APP_LOGE("GetSharedDependencies failed, can not find bundle %{public}s", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    const InnerBundleInfo &innerBundleInfo = item->second;
    if (!innerBundleInfo.GetAllSharedDependencies(moduleName, dependencies)) {
        APP_LOGE("GetSharedDependencies failed, can not find module %{public}s", moduleName.c_str());
        return ERR_BUNDLE_MANAGER_MODULE_NOT_EXIST;
    }
    APP_LOGD("GetSharedDependencies(bundle %{public}s, module %{public}s) successfully)",
        bundleName.c_str(), moduleName.c_str());
    return ERR_OK;
}

bool BundleDataMgr::CheckHspVersionIsRelied(int32_t versionCode, const InnerBundleInfo &info) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    std::string hspBundleName = info.GetBundleName();
    if (versionCode == Constants::ALL_VERSIONCODE) {
        // uninstall hsp bundle, check other bundle denpendency
        return CheckHspBundleIsRelied(hspBundleName);
    }
    std::vector<std::string> hspModules = info.GetAllHspModuleNamesForVersion(static_cast<uint32_t>(versionCode));
    // check whether has higher version
    std::vector<uint32_t> versionCodes = info.GetAllHspVersion();
    for (const auto &item : versionCodes) {
        if (item > static_cast<uint32_t>(versionCode)) {
            return false;
        }
    }
    // check other bundle denpendency
    for (const auto &[bundleName, innerBundleInfo] : bundleInfos_) {
        if (bundleName == hspBundleName) {
            continue;
        }
        std::vector<Dependency> dependencyList = innerBundleInfo.GetDependencies();
        for (const auto &dependencyItem : dependencyList) {
            if (dependencyItem.bundleName == hspBundleName &&
                std::find(hspModules.begin(), hspModules.end(), dependencyItem.moduleName) != hspModules.end()) {
                return true;
            }
        }
    }
    return false;
}

bool BundleDataMgr::CheckHspBundleIsRelied(const std::string &hspBundleName) const
{
    for (const auto &[bundleName, innerBundleInfo] : bundleInfos_) {
        if (bundleName == hspBundleName) {
            continue;
        }
        std::vector<Dependency> dependencyList = innerBundleInfo.GetDependencies();
        for (const auto &dependencyItem : dependencyList) {
            if (dependencyItem.bundleName == hspBundleName) {
                return true;
            }
        }
    }
    return false;
}

ErrCode BundleDataMgr::GetSharedBundleInfo(const std::string &bundleName, int32_t flags, BundleInfo &bundleInfo)
{
    if (bundleName.empty()) {
        APP_LOGE("bundleName is empty");
        return ERR_BUNDLE_MANAGER_PARAM_ERROR;
    }

    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("can not find bundle %{public}s", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    const InnerBundleInfo &innerBundleInfo = infoItem->second;
    innerBundleInfo.GetSharedBundleInfo(flags, bundleInfo);
    return ERR_OK;
}

bool BundleDataMgr::IsPreInstallApp(const std::string &bundleName)
{
    APP_LOGD("IsPreInstallApp bundleName: %{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto item = bundleInfos_.find(bundleName);
    if (item == bundleInfos_.end()) {
        APP_LOGE("IsPreInstallApp failed, can not find bundle %{public}s",
            bundleName.c_str());
        return false;
    }
    return item->second.IsPreInstallApp();
}

ErrCode BundleDataMgr::GetProxyDataInfos(const std::string &bundleName, const std::string &moduleName,
    int userId, std::vector<ProxyData> &proxyDatas) const
{
    InnerBundleInfo info;
    auto ret = GetInnerBundleInfoWithBundleFlagsV9(
        bundleName, static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE), info, userId);
    if (ret != ERR_OK) {
        APP_LOGE("GetProxyData failed for GetInnerBundleInfo failed");
        return ret;
    }
    return info.GetProxyDataInfos(moduleName, proxyDatas);
}

ErrCode BundleDataMgr::GetAllProxyDataInfos(int userId, std::vector<ProxyData> &proxyDatas) const
{
    std::vector<BundleInfo> bundleInfos;
    auto ret = GetBundleInfosV9(
        static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE), bundleInfos, userId);
    if (ret != ERR_OK) {
        APP_LOGE("GetAllProxyDataInfos failed for GetBundleInfos failed");
        return ret;
    }
    for (const auto &bundleInfo : bundleInfos) {
        for (const auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
            proxyDatas.insert(
                proxyDatas.end(), hapModuleInfo.proxyDatas.begin(), hapModuleInfo.proxyDatas.end());
        }
    }
    return ERR_OK;
}

std::string BundleDataMgr::GetBundleNameByAppId(const std::string &appId) const
{
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto it = std::find_if(bundleInfos_.cbegin(), bundleInfos_.cend(), [&appId](const auto &pair) {
        return appId == pair.second.GetAppId();
    });
    if (it == bundleInfos_.cend()) {
        APP_LOGW("invalid appId, can't find bundleName");
        return Constants::EMPTY_STRING;
    }
    return it->second.GetBundleName();
}

ErrCode BundleDataMgr::GetSpecifiedDistributionType(
    const std::string &bundleName, std::string &specifiedDistributionType)
{
    APP_LOGD("GetSpecifiedDistributionType bundleName: %{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundleName: %{public}s does not exist", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    if (infoItem->second.GetApplicationBundleType() != BundleType::SHARED) {
        int32_t userId = AccountHelper::GetCurrentActiveUserId();
        int32_t responseUserId = infoItem->second.GetResponseUserId(userId);
        if (responseUserId == Constants::INVALID_USERID) {
            APP_LOGE("bundleName: %{public}s does not exist in current userId", bundleName.c_str());
            return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
        }
    }
    if (!DelayedSingleton<AppProvisionInfoManager>::GetInstance()->GetSpecifiedDistributionType(bundleName,
        specifiedDistributionType)) {
        APP_LOGE("bundleName:%{public}s GetSpecifiedDistributionType failed.", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    return ERR_OK;
}

ErrCode BundleDataMgr::GetAdditionalInfo(
    const std::string &bundleName, std::string &additionalInfo)
{
    APP_LOGD("GetAdditionalInfo bundleName: %{public}s", bundleName.c_str());
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = bundleInfos_.find(bundleName);
    if (infoItem == bundleInfos_.end()) {
        APP_LOGE("bundleName: %{public}s does not exist", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    if (infoItem->second.GetApplicationBundleType() != BundleType::SHARED) {
        int32_t userId = AccountHelper::GetCurrentActiveUserId();
        int32_t responseUserId = infoItem->second.GetResponseUserId(userId);
        if (responseUserId == Constants::INVALID_USERID) {
            APP_LOGE("bundleName: %{public}s does not exist in current userId", bundleName.c_str());
            return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
        }
    }
    if (!DelayedSingleton<AppProvisionInfoManager>::GetInstance()->GetAdditionalInfo(bundleName,
        additionalInfo)) {
        APP_LOGE("bundleName:%{public}s GetAdditionalInfo failed.", bundleName.c_str());
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }
    return ERR_OK;
}
}  // namespace AppExecFwk
}  // namespace OHOS
