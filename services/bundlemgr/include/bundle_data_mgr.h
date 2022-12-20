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

#ifndef FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_BUNDLE_DATA_MGR_H
#define FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_BUNDLE_DATA_MGR_H

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>

#include "want.h"

#include "ability_info.h"
#include "application_info.h"
#include "bundle_data_storage_interface.h"
#include "bundle_event_callback_interface.h"
#include "bundle_promise.h"
#include "bundle_sandbox_app_helper.h"
#include "bundle_state_storage.h"
#include "bundle_status_callback_interface.h"
#include "common_event_data.h"
#include "inner_bundle_info.h"
#include "inner_bundle_user_info.h"
#include "module_usage_record.h"
#include "preinstall_data_storage_interface.h"
#ifdef GLOBAL_RESMGR_ENABLE
#include "resource_manager.h"
#endif
#ifdef BUNDLE_FRAMEWORK_DEFAULT_APP
#include "element.h"
#endif

namespace OHOS {
namespace AppExecFwk {
enum class InstallState {
    INSTALL_START = 1,
    INSTALL_SUCCESS,
    INSTALL_FAIL,
    UNINSTALL_START,
    UNINSTALL_SUCCESS,
    UNINSTALL_FAIL,
    UPDATING_START,
    UPDATING_SUCCESS,
    UPDATING_FAIL,
    ROLL_BACK,
    USER_CHANGE,
};

class BundleDataMgr {
public:
    using Want = OHOS::AAFwk::Want;

    // init state transfer map data.
    BundleDataMgr();
    virtual ~BundleDataMgr();

    /**
     * @brief Boot query persistent storage.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool LoadDataFromPersistentStorage();
    /**
     * @brief Update internal state for whole bundle.
     * @param bundleName Indicates the bundle name.
     * @param state Indicates the install state to be set.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool UpdateBundleInstallState(const std::string &bundleName, const InstallState state);
    /**
     * @brief Add new InnerBundleInfo.
     * @param bundleName Indicates the bundle name.
     * @param info Indicates the InnerBundleInfo object to be save.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool AddInnerBundleInfo(const std::string &bundleName, InnerBundleInfo &info);
    /**
     * @brief Add new module info to an exist InnerBundleInfo.
     * @param bundleName Indicates the bundle name.
     * @param newInfo Indicates the new InnerBundleInfo object.
     * @param oldInfo Indicates the old InnerBundleInfo object.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool AddNewModuleInfo(const std::string &bundleName, const InnerBundleInfo &newInfo, InnerBundleInfo &oldInfo);
    /**
     * @brief Remove module info from an exist InnerBundleInfo.
     * @param bundleName Indicates the bundle name.
     * @param modulePackage Indicates the module Package.
     * @param oldInfo Indicates the old InnerBundleInfo object.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool RemoveModuleInfo(const std::string &bundleName, const std::string &modulePackage, InnerBundleInfo &oldInfo);
    /**
     * @brief Update module info of an exist module.
     * @param bundleName Indicates the bundle name.
     * @param newInfo Indicates the new InnerBundleInfo object.
     * @param oldInfo Indicates the old InnerBundleInfo object.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool UpdateInnerBundleInfo(const std::string &bundleName, const InnerBundleInfo &newInfo, InnerBundleInfo &oldInfo);
    /**
     * @brief Get an InnerBundleInfo if exist (will change the status to DISABLED).
     * @param bundleName Indicates the bundle name.
     * @param info Indicates the obtained InnerBundleInfo object.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool GetInnerBundleInfo(const std::string &bundleName, InnerBundleInfo &info);
    /**
     * @brief Generate UID and GID for a bundle.
     * @param innerBundleUserInfo Indicates the InnerBundleUserInfo object.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool GenerateUidAndGid(InnerBundleUserInfo &innerBundleUserInfo);
    /**
     * @brief Recycle uid and gid .
     * @param info Indicates the InnerBundleInfo object.
     */
    void RecycleUidAndGid(const InnerBundleInfo &info);
    /**
     * @brief Query the AbilityInfo by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    bool QueryAbilityInfo(const Want &want, int32_t flags, int32_t userId, AbilityInfo &abilityInfo,
        int32_t appIndex = 0) const;
    /**
     * @brief Query a AbilityInfo of list by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfos Indicates the obtained AbilityInfo of list.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    bool QueryAbilityInfos(
        const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const;
    /**
     * @brief Query a AbilityInfo of list by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfos Indicates the obtained AbilityInfo of list.
     * @return Returns ERR_OK if the AbilityInfo is successfully obtained; returns errCode otherwise.
     */
    ErrCode QueryAbilityInfosV9(
        const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const;
    /**
     * @brief Query all match launcher ability infos by given wants.
     * @param want Indicates the match infomation for abilities.
     * @param info Indicates the bundleInfo.
     * @param abilityInfo Indicates the obtained AbilityInfo of list.
     * @param userId Indicates the user ID.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    void GetMatchLauncherAbilityInfos(const Want& want, const InnerBundleInfo& info,
        std::vector<AbilityInfo>& abilityInfos, int32_t userId = Constants::UNSPECIFIED_USERID) const;
    /**
     * @brief Query a AbilityInfo of list by the given userId.
     * @param want Indicates the match infomation for abilities.
     * @param flags Indicates weather to get application Info.
     * @param userId Indicates the id of the user.
     * @param abilityInfo Indicates the obtained AbilityInfo of list.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    bool QueryLauncherAbilityInfos(
        const Want& want, uint32_t userId, std::vector<AbilityInfo>& abilityInfos) const;
    /**
     * @brief Query the AbilityInfo by ability.uri in config.json.
     * @param abilityUri Indicates the uri of the ability.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    bool QueryAbilityInfosByUri(const std::string &abilityUri, std::vector<AbilityInfo> &abilityInfos);
    /**
     * @brief Query the AbilityInfo by ability.uri in config.json.
     * @param abilityUri Indicates the uri of the ability.
     * @param userId Indicates the user ID.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    bool QueryAbilityInfoByUri(
        const std::string &abilityUri, int32_t userId, AbilityInfo &abilityInfo) const;
    /**
     * @brief Obtains the ApplicationInfo based on a given bundle name.
     * @param appName Indicates the application bundle name to be queried.
     * @param flags Indicates the flag used to specify information contained
     *             in the ApplicationInfo object that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfo Indicates the obtained ApplicationInfo object.
     * @return Returns true if the application is successfully obtained; returns false otherwise.
     */
    bool GetApplicationInfo(
        const std::string &appName, int32_t flags, const int userId, ApplicationInfo &appInfo) const;
    /**
     * @brief Obtains the ApplicationInfo based on a given bundle name.
     * @param appName Indicates the application bundle name to be queried.
     * @param flags Indicates the flag used to specify information contained
     *             in the ApplicationInfo object that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfo Indicates the obtained ApplicationInfo object.
     * @return Returns ERR_OK if the application is successfully obtained; returns error code otherwise.
     */
    ErrCode GetApplicationInfoV9(
        const std::string &appName, int32_t flags, int32_t userId, ApplicationInfo &appInfo) const;
    /**
     * @brief Obtains information about all installed applications of a specified user.
     * @param flags Indicates the flag used to specify information contained
     *             in the ApplicationInfo objects that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfos Indicates all of the obtained ApplicationInfo objects.
     * @return Returns true if the application is successfully obtained; returns false otherwise.
     */
    bool GetApplicationInfos(
        int32_t flags, const int userId, std::vector<ApplicationInfo> &appInfos) const;
    /**
     * @brief Obtains information about all installed applications of a specified user.
     * @param flags Indicates the flag used to specify information contained
     *             in the ApplicationInfo objects that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfos Indicates all of the obtained ApplicationInfo objects.
     * @return Returns ERR_OK if the application is successfully obtained; returns error code otherwise.
     */
    ErrCode GetApplicationInfosV9(
        int32_t flags, int32_t userId, std::vector<ApplicationInfo> &appInfos) const;
    /**
     * @brief Obtains BundleInfo of all bundles available in the system.
     * @param flags Indicates the flag used to specify information contained in the BundleInfo that will be returned.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @param userId Indicates the user ID.
     * @return Returns true if the BundleInfos is successfully obtained; returns false otherwise.
     */
    bool GetBundleInfos(int32_t flags,
        std::vector<BundleInfo> &bundleInfos, int32_t userId = Constants::UNSPECIFIED_USERID) const;
    /**
     * @brief Obtains BundleInfo of all bundles available in the system.
     * @param flags Indicates the flag used to specify information contained in the BundleInfo that will be returned.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundleInfos is successfully obtained; returns error code otherwise.
     */
    ErrCode GetBundleInfosV9(int32_t flags, std::vector<BundleInfo> &bundleInfos, int32_t userId) const;
    /**
     * @brief Obtains the BundleInfo based on a given bundle name.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @param userId Indicates the user ID.
     * @return Returns true if the BundleInfo is successfully obtained; returns false otherwise.
     */
    bool GetBundleInfo(const std::string &bundleName, int32_t flags, BundleInfo &bundleInfo,
        int32_t userId = Constants::UNSPECIFIED_USERID) const;
    /**
     * @brief Obtains the BundleInfo based on a given bundle name.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    ErrCode GetBundleInfoV9(const std::string &bundleName, int32_t flags, BundleInfo &bundleInfo,
        int32_t userId = Constants::UNSPECIFIED_USERID) const;
    /**
     * @brief Obtains the BundlePackInfo based on a given bundle name.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param BundlePackInfo Indicates the obtained BundlePackInfo object.
     * @return Returns ERR_OK if the BundlePackInfo is successfully obtained; returns other ErrCode otherwise.
     */
    ErrCode GetBundlePackInfo(const std::string &bundleName, int32_t flags, BundlePackInfo &bundleInfo,
        int32_t userId = Constants::UNSPECIFIED_USERID) const;
    /**
     * @brief Obtains the BundleInfo of application bundles based on the specified metaData.
     * @param metaData Indicates the metadata to get in the bundle.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @return Returns true if the BundleInfos is successfully obtained; returns false otherwise.
     */
    bool GetBundleInfosByMetaData(const std::string &metaData, std::vector<BundleInfo> &bundleInfos) const;
    /**
     * @brief Obtains the bundle name of a specified application based on the given UID.
     * @param uid Indicates the uid.
     * @param bundleName Indicates the obtained bundle name.
     * @return Returns true if the bundle name is successfully obtained; returns false otherwise.
     */
    bool GetBundleNameForUid(const int uid, std::string &bundleName) const;
    /**
     * @brief Obtains all bundle names of a specified application based on the given application UID.
     * @param uid Indicates the uid.
     * @param bundleNames Indicates the obtained bundle names.
     * @return Returns true if the bundle names is successfully obtained; returns false otherwise.
     */
    bool GetBundlesForUid(const int uid, std::vector<std::string> &bundleNames) const;
    /**
     * @brief Obtains the formal name associated with the given UID.
     * @param uid Indicates the uid.
     * @param name Indicates the obtained formal name.
     * @return Returns ERR_OK if called successfully; returns error code otherwise.
     */
    ErrCode GetNameForUid(const int uid, std::string &name) const;
    /**
     * @brief Obtains an array of all group IDs associated with a specified bundle.
     * @param bundleName Indicates the bundle name.
     * @param gids Indicates the group IDs associated with the specified bundle.
     * @return Returns true if the gids is successfully obtained; returns false otherwise.
     */
    bool GetBundleGids(const std::string &bundleName, std::vector<int> &gids) const;
    /**
     * @brief Obtains an array of all group IDs associated with the given bundle name and UID.
     * @param bundleName Indicates the bundle name.
     * @param uid Indicates the uid.
     * @param gids Indicates the group IDs associated with the specified bundle.
     * @return Returns true if the gids is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleGidsByUid(const std::string &bundleName, const int &uid, std::vector<int> &gids) const;
    /**
     * @brief Obtains the BundleInfo of all keep-alive applications in the system.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @return Returns true if the BundleInfos is successfully obtained; returns false otherwise.
     */
    bool QueryKeepAliveBundleInfos(std::vector<BundleInfo> &bundleInfos) const;
    /**
     * @brief Obtains the label of a specified ability.
     * @param bundleName Indicates the bundle name.
     * @param moduleName Indicates the module name.
     * @param abilityName Indicates the ability name.
     * @param label Indicates the obtained label.
     * @return Returns ERR_OK if the ability label is successfully obtained; returns errCode otherwise.
     */
    ErrCode GetAbilityLabel(const std::string &bundleName, const std::string &moduleName,
        const std::string &abilityName, std::string &label) const;
    /**
     * @brief Obtains the Want for starting the main ability of an application based on the given bundle name.
     * @param bundleName Indicates the bundle name.
     * @param want Indicates the obtained launch Want object.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if this function is successfully called; returns errCode otherwise.
     */
    ErrCode GetLaunchWantForBundle(
        const std::string &bundleName, Want &want, int32_t userId = Constants::UNSPECIFIED_USERID) const;
    /**
     * @brief Obtain the HAP module info of a specific ability.
     * @param abilityInfo Indicates the ability.
     * @param userId Indicates the user ID.
     * @param hapModuleInfo Indicates the obtained HapModuleInfo object.
     * @return Returns true if the HapModuleInfo is successfully obtained; returns false otherwise.
     */
    bool GetHapModuleInfo(const AbilityInfo &abilityInfo,
        HapModuleInfo &hapModuleInfo, int32_t userId = Constants::UNSPECIFIED_USERID) const;
    /**
     * @brief Check whether the app is system app by it's UID.
     * @param uid Indicates the uid.
     * @return Returns true if the bundle is a system application; returns false otherwise.
     */
    bool CheckIsSystemAppByUid(const int uid) const;
    /**
     * @brief Obtains all bundle names installed.
     * @param bundleNames Indicates the bundle Names.
     * @param userId Indicates the user ID.
     * @return Returns true if have bundle installed; returns false otherwise.
     */
    bool GetBundleList(
        std::vector<std::string> &bundleNames, int32_t userId = Constants::UNSPECIFIED_USERID) const;
    /**
     * @brief Set the bundle status disable.
     * @param bundleName Indicates the bundle name.
     * @return Returns true if the bundle status successfully set; returns false otherwise.
     */
    bool DisableBundle(const std::string &bundleName);
    /**
     * @brief Set the bundle status enable.
     * @param bundleName Indicates the bundle name.
     * @return Returns true if the bundle status successfully set; returns false otherwise.
     */
    bool EnableBundle(const std::string &bundleName);
    /**
     * @brief Get whether the application status is enabled.
     * @param bundleName Indicates the bundle name.
     * @param isEnable Indicates the application status is enabled.
     * @return Returns result of the operation.
     */
    ErrCode IsApplicationEnabled(const std::string &bundleName, bool &isEnable) const;
    /**
     * @brief Set the application status.
     * @param bundleName Indicates the bundle name.
     * @param isEnable Indicates the status to set.
     * @param userId Indicates the user id.
     * @return Returns result of the operation.
     */
    ErrCode SetApplicationEnabled(const std::string &bundleName, bool isEnable,
        int32_t userId = Constants::UNSPECIFIED_USERID);
    /**
     * @brief Sets whether to enable a specified ability through the proxy object.
     * @param abilityInfo Indicates information about the ability to check.
     * @param isEnable Indicates the ability status is enabled.
     * @return Returns result of the operation.
     */
    ErrCode IsAbilityEnabled(const AbilityInfo &abilityInfo, bool &isEnable) const;
    /**
     * @brief Sets whether to enable a specified ability through the proxy object.
     * @param abilityInfo Indicates information about the ability.
     * @param isEnabled Specifies whether to enable the ability.
     *                 The value true means to enable it, and the value false means to disable it.
     * @param userId Indicates the user id.
     * @return Returns result of the operation.
     */
    ErrCode SetAbilityEnabled(const AbilityInfo &abilityInfo, bool isEnabled,
        int32_t userId = Constants::UNSPECIFIED_USERID);
    /**
     * @brief Register the bundle status callback function.
     * @param bundleStatusCallback Indicates the callback object that using for notifing the bundle status.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool RegisterBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback);

    bool RegisterBundleEventCallback(const sptr<IBundleEventCallback> &bundleEventCallback);

    bool UnregisterBundleEventCallback(const sptr<IBundleEventCallback> &bundleEventCallback);
    /**
     * @brief Clear the specific bundle status callback.
     * @param bundleStatusCallback Indicates the callback to be cleared.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool ClearBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback);
    /**
     * @brief Unregister all the callbacks of status changed.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool UnregisterBundleStatusCallback();
    /**
     * @brief Get a mutex for locking by bundle name.
     * @param bundleName Indicates the bundle name.
     * @return Returns a reference of mutex that for locing by bundle name.
     */
    std::mutex &GetBundleMutex(const std::string &bundleName);
    /**
     * @brief Obtains the provision Id based on a given bundle name.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param provisionId Indicates the provision Id to be returned.
     * @return Returns true if the provision Id is successfully obtained; returns false otherwise.
     */
    bool GetProvisionId(const std::string &bundleName, std::string &provisionId) const;
    /**
     * @brief Obtains the app feature based on a given bundle name.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param provisionId Indicates the app feature to be returned.
     * @return Returns true if the app feature is successfully obtained; returns false otherwise.
     */
    bool GetAppFeature(const std::string &bundleName, std::string &appFeature) const;
    /**
     * @brief Set the flag that indicates whether initial user create successfully.
     * @param flag Indicates the flag to be set.
     * @return
     */
    void SetInitialUserFlag(bool flag);
    /**
     * @brief Checks whether the publickeys of two bundles are the same.
     * @param firstBundleName Indicates the first bundle name.
     * @param secondBundleName Indicates the second bundle name.
     * @return Returns SIGNATURE_UNKNOWN_BUNDLE if at least one of the given bundles is not found;
     *         returns SIGNATURE_NOT_MATCHED if their publickeys are different;
     *         returns SIGNATURE_MATCHED if their publickeys are the same.
     */
    int CheckPublicKeys(const std::string &firstBundleName, const std::string &secondBundleName) const;
    /**
     * @brief Get a shared pointer to the IBundleDataStorage object.
     * @return Returns the pointer of IBundleDataStorage object.
     */
    std::shared_ptr<IBundleDataStorage> GetDataStorage() const;
    /**
     * @brief Obtains the FormInfo objects provided by all applications on the device.
     * @param formInfos List of FormInfo objects if obtained;
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool GetAllFormsInfo(std::vector<FormInfo> &formInfos) const;
    /**
     * @brief Obtains the FormInfo objects provided by a specified application on the device.
     * @param bundleName Indicates the bundle name of the  application.
     * @param formInfos List of FormInfo objects if obtained;
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool GetFormsInfoByApp(const std::string &bundleName, std::vector<FormInfo> &formInfos) const;
    /**
     * @brief Obtains the FormInfo objects provided by a specified module name.
     * @param formInfos List of FormInfo objects if obtained;
     * @param moduleName Indicates the module name of the application.
     * @param bundleName Indicates the bundle name of the application.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool GetFormsInfoByModule(
        const std::string &bundleName, const std::string &moduleName, std::vector<FormInfo> &formInfos) const;
    /**
     * @brief Obtains the ShortcutInfo objects provided by a specified application on the device.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @param shortcutInfos List of ShortcutInfo objects if obtained.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool GetShortcutInfos(
        const std::string &bundleName, int32_t userId, std::vector<ShortcutInfo> &shortcutInfos) const;
    /**
     * @brief Obtains the ShortcutInfo objects provided by a specified application on the device.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @param shortcutInfos List of ShortcutInfo objects if obtained.
     * @return Returns errcode of the result.
     */
    ErrCode GetShortcutInfoV9(
        const std::string &bundleName, int32_t userId, std::vector<ShortcutInfo> &shortcutInfos) const;
    /**
     * @brief Obtains the CommonEventInfo objects provided by an event key on the device.
     * @param eventKey Indicates the event of the subscribe.
     * @param commonEventInfos List of CommonEventInfo objects if obtained.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool GetAllCommonEventInfo(const std::string &eventKey, std::vector<CommonEventInfo> &commonEventInfos) const;
    /**
     * @brief Update bundle usage record on module removed.
     * @param keepUsage Indicates the flag record is remove on module removed.
     * @param userId Indicates the user Id of the application.
     * @param bundleName Indicates the bundle name of the application.
     * @param moduleName Indicates the module name of the  application.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool UpdateUsageRecordOnModuleRemoved(
        bool keepUsage, const int userId, const std::string &bundleName, const std::string &moduleName) const;
    /**
     * @brief Obtains the PreInstallBundleInfo objects provided by bundleName.
     * @param bundleName Indicates the bundle name of the application.
     * @param preInstallBundleInfo Indicates information about the PreInstallBundleInfo.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool GetPreInstallBundleInfo(const std::string &bundleName, PreInstallBundleInfo &preInstallBundleInfo);
    /**
     * @brief Save new PreInstallBundleInfo.
     * @param bundleName Indicates the bundle name.
     * @param preInstallBundleInfo Indicates the PreInstallBundleInfo object to be save.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool SavePreInstallBundleInfo(const std::string &bundleName, const PreInstallBundleInfo &preInstallBundleInfo);
    /**
     * @brief Obtains the PreInstallBundleInfo objects provided by bundleName.
     * @param preInstallBundleInfo Indicates information about the PreInstallBundleInfo.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool LoadAllPreInstallBundleInfos(std::vector<PreInstallBundleInfo> &preInstallBundleInfos);
    /**
     * @brief Save new PreInstallBundleInfo.
     * @param bundleName Indicates the bundle name.
     * @param preInstallBundleInfo Indicates the PreInstallBundleInfo object to be save.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool DeletePreInstallBundleInfo(
        const std::string &bundleName, const PreInstallBundleInfo &preInstallBundleInfo);
    /**
     * @brief Save installation mark to datebase storage.
     * @param info Indicates the innerBundleInfo of the bundle which needs to save installation mark.
     * @param isAppExisted Indicates whether the application exists in the database storage or not.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool SaveInnerBundleInfo(const InnerBundleInfo &info) const;
    /**
     * @brief GetInnerBundleUserInfoByUserId.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param userId Indicates the user ID.
     * @param innerBundleUserInfo Indicates the obtained InnerBundleUserInfo object.
     * @return Returns true if the application is successfully obtained; returns false otherwise.
     */
    bool GetInnerBundleUserInfoByUserId(
        const std::string &bundleName, int32_t userId, InnerBundleUserInfo &innerBundleUserInfo) const;
    /**
     * @brief save all created users.
     * @param userId Indicates the user ID.
     */
    void AddUserId(int32_t userId);
    /**
     * @brief remove userId.
     * @param userId Indicates the user ID.
     */
    void RemoveUserId(int32_t userId);
    /**
     * @brief query users.
     * @param userId Indicates the user ID.
     * @return Returns true when query user success; returns false otherwise.
     */
    bool HasUserId(int32_t userId) const;
    /**
     * @brief Get userId by calling uid.
     * @return Returns userId.
     */
    int32_t GetUserIdByCallingUid() const;
    /**
     * @brief Get all user.
     * @return Returns all userId.
     */
    std::set<int32_t> GetAllUser() const;
    /**
     * @brief Has initial user created.
     * @return Returns initial user flag.
     */
    bool HasInitialUserCreated() const
    {
        return initialUserFlag_;
    }
    /**
     * @brief Set bundlePromise.
     * @param bundlePromise Indicates the bundlePromise.
     */
    void SetBundlePromise(const std::shared_ptr<BundlePromise>& bundlePromise)
    {
        bundlePromise_ = bundlePromise;
    }
    /**
     * @brief Get bundleUserInfos by bundleName.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param innerBundleUserInfo Indicates the obtained InnerBundleUserInfo object.
     * @return Returns true if the application is successfully obtained; returns false otherwise.
     */
    bool GetInnerBundleUserInfos(
        const std::string &bundleName, std::vector<InnerBundleUserInfo> &innerBundleUserInfos) const;
    /**
     * @brief Get app privilege level.
     * @param bundleName Indicates the bundle name of the app privilege level.
     * @param userId Indicates the user id.
     * @return Returns app privilege level.
     */
    std::string GetAppPrivilegeLevel(
        const std::string &bundleName, int32_t userId = Constants::UNSPECIFIED_USERID);
    /**
     * @brief Query a ExtensionAbilityInfo of list by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param extensionInfos Indicates the obtained ExtensionAbilityInfo of list.
     * @return Returns true if the ExtensionAbilityInfo is successfully obtained; returns false otherwise.
     */
    bool QueryExtensionAbilityInfos(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos, int32_t appIndex = 0) const;
    /**
     * @brief Query a ExtensionAbilityInfo of list by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param extensionInfos Indicates the obtained ExtensionAbilityInfo of list.
     * @return Returns ERR_OK if the ExtensionAbilityInfo is successfully obtained; returns errCode otherwise.
     */
    ErrCode QueryExtensionAbilityInfosV9(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos, int32_t appIndex = 0) const;
    /**
     * @brief Obtains the PreInstallBundleInfo objects in Cache.
     * @return Returns PreInstallBundleInfos.
     */
    const std::vector<PreInstallBundleInfo>& GetAllPreInstallBundleInfos();
    /**
     * @brief Restore uid and gid .
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    bool RestoreUidAndGid();
    /**
     * @brief Load all bundle state data from jsonDb .
     * @return
     */
    void LoadAllBundleStateDataFromJsonDb();

    bool QueryExtensionAbilityInfos(const ExtensionAbilityType &extensionType, const int32_t &userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos) const;

    std::vector<std::string> GetAccessibleAppCodePaths(int32_t userId) const;

    bool QueryExtensionAbilityInfoByUri(const std::string &uri, int32_t userId,
        ExtensionAbilityInfo &extensionAbilityInfo) const;

    bool QueryAllDeviceIds(std::vector<std::string> &deviceIds);

    void GetAllUriPrefix(std::vector<std::string> &uriPrefixList, int32_t userId,
        const std::string &excludeModule = "") const;

    bool AddInnerBundleUserInfo(const std::string &bundleName, const InnerBundleUserInfo& newUserInfo);

    bool RemoveInnerBundleUserInfo(const std::string &bundleName, int32_t userId);

#ifdef BUNDLE_FRAMEWORK_FREE_INSTALL
    bool GetRemovableBundleNameVec(std::map<std::string, int>& bundlenameAndUids);
#endif
    bool ImplicitQueryInfoByPriority(const Want &want, int32_t flags, int32_t userId,
        AbilityInfo &abilityInfo, ExtensionAbilityInfo &extensionInfo);

    bool ImplicitQueryInfos(const Want &want, int32_t flags, int32_t userId,
        std::vector<AbilityInfo> &abilityInfos, std::vector<ExtensionAbilityInfo> &extensionInfos);

    /**
     * @brief Sets whether to enable isRemovable based on given bundle name, module name and isEnable.
     * @param bundleName Indicates the bundleName.
     * @param moduleName Indicates the moduleName.
     * @param isEnable Set module isRemovable is enable.
     * @return Returns true if the module isRemovable is set success; returns false otherwise.
     */
    bool SetModuleRemovable(const std::string &bundleName, const std::string &moduleName, bool isEnable);
    /**
     * @brief Get Module isRemovable by bundleName and moduleName.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param moduleName Indicates the moduleName.
     * @param isRemovable Indicates the module whether is removable.
     * @return Returns ERR_OK if the module isRemovable is successfully obtained; returns other ErrCode otherwise.
     */
    ErrCode IsModuleRemovable(const std::string &bundleName, const std::string &moduleName, bool &isRemovable) const;

#ifdef BUNDLE_FRAMEWORK_FREE_INSTALL
    /**
     * @brief Get bundle space size (Bytes) by bundleName.
     * @param bundleName Indicates the application bundle name to be queried.
     * @return Returns the space size of a bundle by bundleName.
     */
    int64_t GetBundleSpaceSize(const std::string &bundleName) const;
    /**
     * @brief Get all free install bundle space size (Bytes).
     * @return Returns the space size of all free install bundles.
     */
    int64_t GetAllFreeInstallBundleSpaceSize() const;
#endif
    bool GetAllDependentModuleNames(const std::string &bundleName, const std::string &moduleName,
        std::vector<std::string> &dependentModuleNames);
    ErrCode SetModuleUpgradeFlag(const std::string &bundleName, const std::string &moduleName, int32_t upgradeFlag);
    int32_t GetModuleUpgradeFlag(const std::string &bundleName, const std::string &moduleName) const;
    /**
     * @brief Get the Inner Bundle Info With Flags object
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param info Indicates the innerBundleInfo of the bundle.
     * @param userId Indicates the user ID.
     * @return Returns true if get inner bundle info is successfully obtained; returns false otherwise.
     */
    bool GetInnerBundleInfoWithFlags(const std::string &bundleName, const int32_t flags,
        InnerBundleInfo &info, int32_t userId = Constants::UNSPECIFIED_USERID) const;
    ErrCode GetInnerBundleInfoWithFlagsV9(const std::string &bundleName, int32_t flags,
        InnerBundleInfo &info, int32_t userId = Constants::UNSPECIFIED_USERID) const;
    ErrCode GetInnerBundleInfoWithBundleFlagsV9(const std::string &bundleName, int32_t flags,
        InnerBundleInfo &info, int32_t userId = Constants::UNSPECIFIED_USERID) const;
    std::shared_ptr<BundleSandboxAppHelper> GetSandboxAppHelper() const;
    void StoreSandboxPersistentInfo(const std::string &bundleName, const SandboxAppPersistentInfo &info);
    void DeleteSandboxPersistentInfo(const std::string &bundleName, const SandboxAppPersistentInfo &info);

    bool SetDisposedStatus(const std::string &bundleName, int32_t status);

    int32_t GetDisposedStatus(const std::string &bundleName);

#ifdef BUNDLE_FRAMEWORK_DEFAULT_APP
    bool QueryInfoAndSkillsByElement(int32_t userId, const Element& element,
        AbilityInfo& abilityInfo, ExtensionAbilityInfo& extensionInfo, std::vector<Skill>& skills) const;

    bool GetElement(int32_t userId, const ElementName& elementName, Element& element) const;
#endif

    int32_t GetUserId(int32_t userId = Constants::UNSPECIFIED_USERID) const;

    ErrCode GetMediaData(const std::string &bundleName, const std::string &moduleName, const std::string &abilityName,
        std::unique_ptr<uint8_t[]> &mediaDataPtr, size_t &len, int32_t userId) const;

    std::shared_mutex &GetStatusCallbackMutex();

    std::vector<sptr<IBundleStatusCallback>> GetCallBackList() const;

    std::string GetStringById(const std::string &bundleName, const std::string &moduleName,
        uint32_t resId, int32_t userId, const std::string &localeInfo);

    std::string GetIconById(
        const std::string &bundleName, const std::string &moduleName, uint32_t resId, uint32_t density, int32_t userId);
    void UpdateRemovable(const std::string &bundleName, bool removable);
    void UpdatePrivilegeCapability(
        const std::string &bundleName, const ApplicationInfo &appInfo);
    bool FetchInnerBundleInfo(
        const std::string &bundleName, InnerBundleInfo &innerBundleInfo);

    bool UpdateQuickFixInnerBundleInfo(const std::string &bundleName, const InnerBundleInfo &innerBundleInfo);

    void NotifyBundleEventCallback(const EventFwk::CommonEventData &eventData) const;
private:
    /**
     * @brief Init transferStates.
     * @return
     */
    void InitStateTransferMap();
    /**
     * @brief Determine whether to delete the data status.
     * @param state Indicates the InstallState object.
     * @return Returns true if state is INSTALL_FAIL，UNINSTALL_FAIL，UNINSTALL_SUCCESS，or UPDATING_FAIL; returns false
     * otherwise.
     */
    bool IsDeleteDataState(const InstallState state) const;
    /**
     * @brief Determine whether it is disable.
     * @param state Indicates the InstallState object.
     * @return Returns true if install state is UPDATING_START or UNINSTALL_START; returns false otherwise.
     */
    bool IsDisableState(const InstallState state) const;
    /**
     * @brief Delete bundle info if InstallState is not INSTALL_FAIL.
     * @param bundleName Indicates the bundle Names.
     * @param state Indicates the InstallState object.
     * @return Returns true if install state is UPDATING_START or UNINSTALL_START; returns false otherwise.
     */
    void DeleteBundleInfo(const std::string &bundleName, const InstallState state);
    /**
     * @brief Determine whether app is installed.
     * @param bundleName Indicates the bundle Names.
     * @return Returns true if install state is INSTALL_SUCCESS; returns false otherwise.
     */
    bool IsAppOrAbilityInstalled(const std::string &bundleName) const;
    /**
     * @brief Implicit query abilityInfos by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfos Indicates the obtained AbilityInfo of list.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    bool ImplicitQueryAbilityInfos(const Want &want, int32_t flags, int32_t userId,
        std::vector<AbilityInfo> &abilityInfos, int32_t appIndex = 0) const;
    ErrCode ImplicitQueryAbilityInfosV9(const Want &want, int32_t flags, int32_t userId,
        std::vector<AbilityInfo> &abilityInfos, int32_t appIndex = 0) const;
    void GetMatchAbilityInfos(const Want &want, int32_t flags,
        const InnerBundleInfo &info, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const;
    void GetMatchAbilityInfosV9(const Want &want, int32_t flags,
        const InnerBundleInfo &info, int32_t userId, std::vector<AbilityInfo> &abilityInfos) const;
    bool ExplicitQueryAbilityInfo(const Want &want, int32_t flags, int32_t userId, AbilityInfo &abilityInfo,
        int32_t appIndex = 0) const;
    ErrCode ExplicitQueryAbilityInfoV9(const Want &want, int32_t flags, int32_t userId, AbilityInfo &abilityInfo,
        int32_t appIndex = 0) const;
    bool GenerateBundleId(const std::string &bundleName, int32_t &bundleId);
    int32_t GetUserIdByUid(int32_t uid) const;
    ErrCode GetInnerBundleInfoByUid(const int uid, InnerBundleInfo &innerBundleInfo) const;
    bool GetAllBundleInfos(int32_t flags, std::vector<BundleInfo> &bundleInfos) const;
    ErrCode GetAllBundleInfosV9(int32_t flags, std::vector<BundleInfo> &bundleInfos) const;
    bool ExplicitQueryExtensionInfo(const Want &want, int32_t flags, int32_t userId,
        ExtensionAbilityInfo &extensionInfo, int32_t appIndex = 0) const;
    ErrCode ExplicitQueryExtensionInfoV9(const Want &want, int32_t flags, int32_t userId,
        ExtensionAbilityInfo &extensionInfo, int32_t appIndex = 0) const;
    bool ImplicitQueryExtensionInfos(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos, int32_t appIndex = 0) const;
    ErrCode ImplicitQueryExtensionInfosV9(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos, int32_t appIndex = 0) const;
    void GetMatchExtensionInfos(const Want &want, int32_t flags, const int32_t &userId, const InnerBundleInfo &info,
        std::vector<ExtensionAbilityInfo> &einfos) const;
    void GetMatchExtensionInfosV9(const Want &want, int32_t flags, int32_t userId, const InnerBundleInfo &info,
        std::vector<ExtensionAbilityInfo> &infos) const;
#ifdef GLOBAL_RESMGR_ENABLE
    std::shared_ptr<Global::Resource::ResourceManager> GetResourceManager(const std::string &bundleName,
        const std::string &moduleName, int32_t userId, const std::string &localeInfo = Constants::EMPTY_STRING) const;
#endif

    void FilterAbilityInfosByModuleName(const std::string &moduleName, std::vector<AbilityInfo> &abilityInfos) const;

    void FilterExtensionAbilityInfosByModuleName(const std::string &moduleName,
        std::vector<ExtensionAbilityInfo> &extensionInfos) const;
    void CompatibleOldBundleStateInKvDb();
    void ResetBundleStateData();
    bool QueryAbilityInfoWithFlags(const std::optional<AbilityInfo> &option, int32_t flags, int32_t userId,
        const InnerBundleInfo &innerBundleInfo, AbilityInfo &info) const;
    ErrCode QueryAbilityInfoWithFlagsV9(const std::optional<AbilityInfo> &option, int32_t flags, int32_t userId,
        const InnerBundleInfo &innerBundleInfo, AbilityInfo &info) const;
    bool ImplicitQueryCurAbilityInfos(const Want &want, int32_t flags, int32_t userId,
        std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const;
    ErrCode ImplicitQueryCurAbilityInfosV9(const Want &want, int32_t flags, int32_t userId,
        std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const;
    void ImplicitQueryAllAbilityInfos(const Want &want, int32_t flags, int32_t userId,
        std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const;
    void ImplicitQueryAllAbilityInfosV9(const Want &want, int32_t flags, int32_t userId,
        std::vector<AbilityInfo> &abilityInfos, int32_t appIndex) const;
    bool ImplicitQueryCurExtensionInfos(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &infos, int32_t appIndex) const;
    ErrCode ImplicitQueryCurExtensionInfosV9(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &infos, int32_t appIndex) const;
    void ImplicitQueryAllExtensionInfos(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &infos, int32_t appIndex) const;
    void ImplicitQueryAllExtensionInfosV9(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &infos, int32_t appIndex) const;
    bool CheckAppInstallControl(const std::string &appId, int32_t userId) const;
    ErrCode CheckInnerBundleInfoWithFlags(
        const InnerBundleInfo &innerBundleInfo, int32_t flags, int32_t userId) const;

private:
    mutable std::mutex bundleInfoMutex_;
    mutable std::mutex stateMutex_;
    mutable std::mutex bundleIdMapMutex_;
    mutable std::shared_mutex callbackMutex_;
    mutable std::mutex eventCallbackMutex_;
    mutable std::shared_mutex bundleMutex_;
    mutable std::mutex multiUserIdSetMutex_;
    mutable std::mutex preInstallInfoMutex_;
    bool initialUserFlag_ = false;
    // using for locking by bundleName
    std::unordered_map<std::string, std::mutex> bundleMutexMap_;
    // using for generating bundleId
    // key:bundleId
    // value:bundleName
    std::map<int32_t, std::string> bundleIdMap_;
    // save all created users.
    std::set<int32_t> multiUserIdsSet_;
    // use vector because these functions using for IPC, the bundleName may duplicate
    std::vector<sptr<IBundleStatusCallback>> callbackList_;
    // common event callback
    std::vector<sptr<IBundleEventCallback>> eventCallbackList_;
    // all installed bundles
    // key:bundleName
    // value:innerbundleInfo
    std::map<std::string, InnerBundleInfo> bundleInfos_;
    // key:bundle name
    std::map<std::string, InstallState> installStates_;
    // current-status:previous-statue pair
    std::multimap<InstallState, InstallState> transferStates_;
    std::shared_ptr<IBundleDataStorage> dataStorage_;
    std::shared_ptr<IPreInstallDataStorage> preInstallDataStorage_;
    std::shared_ptr<BundleStateStorage> bundleStateStorage_;
    std::vector<PreInstallBundleInfo> preInstallBundleInfos_;
    std::shared_ptr<BundlePromise> bundlePromise_ = nullptr;
    std::shared_ptr<BundleSandboxAppHelper> sandboxAppHelper_;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_BUNDLE_DATA_MGR_H
