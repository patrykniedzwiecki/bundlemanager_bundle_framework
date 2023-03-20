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

#include "bundle_status_callback_proxy.h"

#include "ipc_types.h"
#include "parcel.h"
#include "string_ex.h"

#include "app_log_wrapper.h"
#include "appexecfwk_errors.h"

namespace OHOS {
namespace AppExecFwk {
void BundleStatusCallbackProxy::InitResultMap()
{
    resultMap_ = {
        {ERR_OK, "SUCCESS"},
        {ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR, "ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR"},
        {ERR_APPEXECFWK_INSTALL_HOST_INSTALLER_FAILED, "ERR_APPEXECFWK_INSTALL_HOST_INSTALLER_FAILED"},
        {ERR_APPEXECFWK_INSTALL_PARSE_FAILED, "ERR_APPEXECFWK_INSTALL_PARSE_FAILED"},
        {ERR_APPEXECFWK_INSTALL_VERSION_DOWNGRADE, "ERR_APPEXECFWK_INSTALL_VERSION_DOWNGRADE"},
        {ERR_APPEXECFWK_INSTALL_VERIFICATION_FAILED, "ERR_APPEXECFWK_INSTALL_VERIFICATION_FAILED"},
        {ERR_APPEXECFWK_INSTALL_FAILED_INVALID_SIGNATURE_FILE_PATH,
            "ERR_APPEXECFWK_INSTALL_FAILED_INVALID_SIGNATURE_FILE_PATH"},
        {ERR_APPEXECFWK_INSTALL_FAILED_BAD_BUNDLE_SIGNATURE_FILE,
            "ERR_APPEXECFWK_INSTALL_FAILED_BAD_BUNDLE_SIGNATURE_FILE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_NO_BUNDLE_SIGNATURE, "ERR_APPEXECFWK_INSTALL_FAILED_NO_BUNDLE_SIGNATURE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_VERIFY_APP_PKCS7_FAIL, "ERR_APPEXECFWK_INSTALL_FAILED_VERIFY_APP_PKCS7_FAIL"},
        {ERR_APPEXECFWK_INSTALL_FAILED_PROFILE_PARSE_FAIL, "ERR_APPEXECFWK_INSTALL_FAILED_PROFILE_PARSE_FAIL"},
        {ERR_APPEXECFWK_INSTALL_FAILED_APP_SOURCE_NOT_TRUESTED,
            "ERR_APPEXECFWK_INSTALL_FAILED_APP_SOURCE_NOT_TRUESTED"},
        {ERR_APPEXECFWK_INSTALL_FAILED_BAD_DIGEST, "ERR_APPEXECFWK_INSTALL_FAILED_BAD_DIGEST"},
        {ERR_APPEXECFWK_INSTALL_FAILED_BUNDLE_INTEGRITY_VERIFICATION_FAILURE,
            "ERR_APPEXECFWK_INSTALL_FAILED_BUNDLE_INTEGRITY_VERIFICATION_FAILURE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_FILE_SIZE_TOO_LARGE, "ERR_APPEXECFWK_INSTALL_FAILED_FILE_SIZE_TOO_LARGE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_BAD_PUBLICKEY, "ERR_APPEXECFWK_INSTALL_FAILED_BAD_PUBLICKEY"},
        {ERR_APPEXECFWK_INSTALL_FAILED_BAD_BUNDLE_SIGNATURE, "ERR_APPEXECFWK_INSTALL_FAILED_BAD_BUNDLE_SIGNATURE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_NO_PROFILE_BLOCK_FAIL, "ERR_APPEXECFWK_INSTALL_FAILED_NO_PROFILE_BLOCK_FAIL"},
        {ERR_APPEXECFWK_INSTALL_FAILED_BUNDLE_SIGNATURE_VERIFICATION_FAILURE,
            "ERR_APPEXECFWK_INSTALL_FAILED_BUNDLE_SIGNATURE_VERIFICATION_FAILURE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_VERIFY_SOURCE_INIT_FAIL,
            "ERR_APPEXECFWK_INSTALL_FAILED_VERIFY_SOURCE_INIT_FAIL"},
        {ERR_APPEXECFWK_INSTALL_FAILED_INCOMPATIBLE_SIGNATURE, "ERR_APPEXECFWK_INSTALL_FAILED_INCOMPATIBLE_SIGNATURE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_INCONSISTENT_SIGNATURE, "ERR_APPEXECFWK_INSTALL_FAILED_INCONSISTENT_SIGNATURE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_MODULE_NAME_EMPTY, "ERR_APPEXECFWK_INSTALL_FAILED_MODULE_NAME_EMPTY"},
        {ERR_APPEXECFWK_INSTALL_FAILED_MODULE_NAME_DUPLICATE, "ERR_APPEXECFWK_INSTALL_FAILED_MODULE_NAME_DUPLICATE"},
        {ERR_APPEXECFWK_INSTALL_FAILED_CHECK_HAP_HASH_PARAM, "ERR_APPEXECFWK_INSTALL_FAILED_CHECK_HAP_HASH_PARAM"},
        {ERR_APPEXECFWK_INSTALL_PARAM_ERROR, "ERR_APPEXECFWK_INSTALL_PARAM_ERROR"},
        {ERR_APPEXECFWK_INSTALL_PERMISSION_DENIED, "ERR_APPEXECFWK_INSTALL_PERMISSION_DENIED"},
        {ERR_APPEXECFWK_INSTALL_ENTRY_ALREADY_EXIST, "ERR_APPEXECFWK_INSTALL_ENTRY_ALREADY_EXIST"},
        {ERR_APPEXECFWK_INSTALL_STATE_ERROR, "ERR_APPEXECFWK_INSTALL_STATE_ERROR"},
        {ERR_APPEXECFWK_INSTALL_FILE_PATH_INVALID, "ERR_APPEXECFWK_INSTALL_FILE_PATH_INVALID"},
        {ERR_APPEXECFWK_INSTALL_INVALID_HAP_NAME, "ERR_APPEXECFWK_INSTALL_INVALID_HAP_NAME"},
        {ERR_APPEXECFWK_INSTALL_INVALID_BUNDLE_FILE, "ERR_APPEXECFWK_INSTALL_INVALID_BUNDLE_FILE"},
        {ERR_APPEXECFWK_INSTALL_INVALID_HAP_SIZE, "ERR_APPEXECFWK_INSTALL_INVALID_HAP_SIZE"},
        {ERR_APPEXECFWK_INSTALL_GENERATE_UID_ERROR, "ERR_APPEXECFWK_INSTALL_GENERATE_UID_ERROR"},
        {ERR_APPEXECFWK_INSTALL_INSTALLD_SERVICE_ERROR, "ERR_APPEXECFWK_INSTALL_INSTALLD_SERVICE_ERROR"},
        {ERR_APPEXECFWK_INSTALL_BUNDLE_MGR_SERVICE_ERROR, "ERR_APPEXECFWK_INSTALL_BUNDLE_MGR_SERVICE_ERROR"},
        {ERR_APPEXECFWK_INSTALL_ALREADY_EXIST, "ERR_APPEXECFWK_INSTALL_ALREADY_EXIST"},
        {ERR_APPEXECFWK_INSTALL_BUNDLENAME_NOT_SAME, "ERR_APPEXECFWK_INSTALL_BUNDLENAME_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_VERSIONCODE_NOT_SAME, "ERR_APPEXECFWK_INSTALL_VERSIONCODE_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_VERSIONNAME_NOT_SAME, "ERR_APPEXECFWK_INSTALL_VERSIONNAME_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_MINCOMPATIBLE_VERSIONCODE_NOT_SAME,
            "ERR_APPEXECFWK_INSTALL_MINCOMPATIBLE_VERSIONCODE_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_VENDOR_NOT_SAME, "ERR_APPEXECFWK_INSTALL_VENDOR_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_RELEASETYPE_TARGET_NOT_SAME, "ERR_APPEXECFWK_INSTALL_RELEASETYPE_TARGET_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_RELEASETYPE_NOT_SAME, "ERR_APPEXECFWK_INSTALL_RELEASETYPE_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_RELEASETYPE_COMPATIBLE_NOT_SAME,
            "ERR_APPEXECFWK_INSTALL_RELEASETYPE_COMPATIBLE_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_VERSION_NOT_COMPATIBLE, "ERR_APPEXECFWK_INSTALL_VERSION_NOT_COMPATIBLE"},
        {ERR_APPEXECFWK_INSTALL_INVALID_NUMBER_OF_ENTRY_HAP, "ERR_APPEXECFWK_INSTALL_INVALID_NUMBER_OF_ENTRY_HAP"},
        {ERR_APPEXECFWK_INSTALL_DISK_MEM_INSUFFICIENT, "ERR_APPEXECFWK_INSTALL_DISK_MEM_INSUFFICIENT"},
        {ERR_APPEXECFWK_INSTALL_GRANT_REQUEST_PERMISSIONS_FAILED,
            "ERR_APPEXECFWK_INSTALL_GRANT_REQUEST_PERMISSIONS_FAILED"},
        {ERR_APPEXECFWK_INSTALL_UPDATE_HAP_TOKEN_FAILED, "ERR_APPEXECFWK_INSTALL_UPDATE_HAP_TOKEN_FAILED"},
        {ERR_APPEXECFWK_INSTALL_CHECK_SYSCAP_FAILED, "ERR_APPEXECFWK_INSTALL_CHECK_SYSCAP_FAILED"},
        {ERR_APPEXECFWK_INSTALL_APPTYPE_NOT_SAME, "ERR_APPEXECFWK_INSTALL_APPTYPE_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_URI_DUPLICATE, "ERR_APPEXECFWK_INSTALL_URI_DUPLICATE"},
        {ERR_APPEXECFWK_INSTALL_TYPE_ERROR, "ERR_APPEXECFWK_INSTALL_TYPE_ERROR"},
        {ERR_APPEXECFWK_INSTALL_SDK_INCOMPATIBLE, "ERR_APPEXECFWK_INSTALL_SDK_INCOMPATIBLE"},
        {ERR_APPEXECFWK_INSTALL_SO_INCOMPATIBLE, "ERR_APPEXECFWK_INSTALL_SO_INCOMPATIBLE"},
        {ERR_APPEXECFWK_INSTALL_AN_INCOMPATIBLE, "ERR_APPEXECFWK_INSTALL_AN_INCOMPATIBLE"},
        {ERR_APPEXECFWK_FAILED_SERVICE_DIED, "ERR_APPEXECFWK_FAILED_SERVICE_DIED"},
        {ERR_APPEXECFWK_PARSE_UNEXPECTED, "ERR_APPEXECFWK_PARSE_UNEXPECTED"},
        {ERR_APPEXECFWK_PARSE_MISSING_BUNDLE, "ERR_APPEXECFWK_PARSE_MISSING_BUNDLE"},
        {ERR_APPEXECFWK_PARSE_MISSING_ABILITY, "ERR_APPEXECFWK_PARSE_MISSING_ABILITY"},
        {ERR_APPEXECFWK_PARSE_NO_PROFILE, "ERR_APPEXECFWK_PARSE_NO_PROFILE"},
        {ERR_APPEXECFWK_PARSE_BAD_PROFILE, "ERR_APPEXECFWK_PARSE_BAD_PROFILE"},
        {ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR, "ERR_APPEXECFWK_PARSE_PROFILE_PROP_TYPE_ERROR"},
        {ERR_APPEXECFWK_PARSE_PROFILE_MISSING_PROP, "ERR_APPEXECFWK_PARSE_PROFILE_MISSING_PROP"},
        {ERR_APPEXECFWK_PARSE_PERMISSION_ERROR, "ERR_APPEXECFWK_PARSE_PERMISSION_ERROR"},
        {ERR_APPEXECFWK_PARSE_PROFILE_PROP_CHECK_ERROR, "ERR_APPEXECFWK_PARSE_PROFILE_PROP_CHECK_ERROR"},
        {ERR_APPEXECFWK_PARSE_RPCID_FAILED, "ERR_APPEXECFWK_PARSE_RPCID_FAILED"},
        {ERR_APPEXECFWK_PARSE_NATIVE_SO_FAILED, "ERR_APPEXECFWK_PARSE_NATIVE_SO_FAILED"},
        {ERR_APPEXECFWK_PARSE_AN_FAILED, "ERR_APPEXECFWK_PARSE_AN_FAILED"},
        {ERR_APPEXECFWK_INSTALLD_PARAM_ERROR, "ERR_APPEXECFWK_INSTALLD_PARAM_ERROR"},
        {ERR_APPEXECFWK_INSTALLD_GET_PROXY_ERROR, "ERR_APPEXECFWK_INSTALLD_GET_PROXY_ERROR"},
        {ERR_APPEXECFWK_INSTALLD_CREATE_DIR_FAILED, "ERR_APPEXECFWK_INSTALLD_CREATE_DIR_FAILED"},
        {ERR_APPEXECFWK_INSTALLD_CREATE_DIR_EXIST, "ERR_APPEXECFWK_INSTALLD_CREATE_DIR_EXIST"},
        {ERR_APPEXECFWK_INSTALLD_CHOWN_FAILED, "ERR_APPEXECFWK_INSTALLD_CHOWN_FAILED"},
        {ERR_APPEXECFWK_INSTALLD_REMOVE_DIR_FAILED, "ERR_APPEXECFWK_INSTALLD_REMOVE_DIR_FAILED"},
        {ERR_APPEXECFWK_INSTALLD_EXTRACT_FILES_FAILED, "ERR_APPEXECFWK_INSTALLD_EXTRACT_FILES_FAILED"},
        {ERR_APPEXECFWK_INSTALLD_RNAME_DIR_FAILED, "ERR_APPEXECFWK_INSTALLD_RNAME_DIR_FAILED"},
        {ERR_APPEXECFWK_INSTALLD_CLEAN_DIR_FAILED, "ERR_APPEXECFWK_INSTALLD_CLEAN_DIR_FAILED"},
        {ERR_APPEXECFWK_INSTALLD_PERMISSION_DENIED, "ERR_APPEXECFWK_INSTALLD_PERMISSION_DENIED"},
        {ERR_APPEXECFWK_UNINSTALL_SYSTEM_APP_ERROR, "ERR_APPEXECFWK_UNINSTALL_SYSTEM_APP_ERROR"},
        {ERR_APPEXECFWK_UNINSTALL_KILLING_APP_ERROR, "ERR_APPEXECFWK_UNINSTALL_KILLING_APP_ERROR"},
        {ERR_APPEXECFWK_UNINSTALL_INVALID_NAME, "ERR_APPEXECFWK_UNINSTALL_INVALID_NAME"},
        {ERR_APPEXECFWK_UNINSTALL_PARAM_ERROR, "ERR_APPEXECFWK_UNINSTALL_PARAM_ERROR"},
        {ERR_APPEXECFWK_UNINSTALL_PERMISSION_DENIED, "ERR_APPEXECFWK_UNINSTALL_PERMISSION_DENIED"},
        {ERR_APPEXECFWK_UNINSTALL_BUNDLE_MGR_SERVICE_ERROR, "ERR_APPEXECFWK_UNINSTALL_BUNDLE_MGR_SERVICE_ERROR"},
        {ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_BUNDLE, "ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_BUNDLE"},
        {ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_MODULE, "ERR_APPEXECFWK_UNINSTALL_MISSING_INSTALLED_MODULE"},
        {ERR_APPEXECFWK_RECOVER_GET_BUNDLEPATH_ERROR, "ERR_APPEXECFWK_RECOVER_GET_BUNDLEPATH_ERROR"},
        {ERR_APPEXECFWK_RECOVER_INVALID_BUNDLE_NAME, "ERR_APPEXECFWK_RECOVER_INVALID_BUNDLE_NAME"},
        {ERR_APPEXECFWK_RECOVER_NOT_ALLOWED, "ERR_APPEXECFWK_RECOVER_NOT_ALLOWED"},
        {ERR_APPEXECFWK_USER_NOT_EXIST, "ERR_APPEXECFWK_USER_NOT_EXIST"},
        {ERR_APPEXECFWK_USER_CREATE_FAILED, "ERR_APPEXECFWK_USER_CREATE_FAILED"},
        {ERR_APPEXECFWK_USER_REMOVE_FAILED, "ERR_APPEXECFWK_USER_REMOVE_FAILED"},
        {ERR_APPEXECFWK_USER_NOT_INSTALL_HAP, "ERR_APPEXECFWK_USER_NOT_INSTALL_HAP"},
        {ERR_APPEXECFWK_INSTALL_APP_DISTRIBUTION_TYPE_NOT_SAME,
            "ERR_APPEXECFWK_INSTALL_APP_DISTRIBUTION_TYPE_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_APP_PROVISION_TYPE_NOT_SAME, "ERR_APPEXECFWK_INSTALL_APP_PROVISION_TYPE_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_NOT_UNIQUE_DISTRO_MODULE_NAME, "ERR_APPEXECFWK_INSTALL_NOT_UNIQUE_DISTRO_MODULE_NAME"},
        {ERR_APPEXECFWK_INSTALL_INCONSISTENT_MODULE_NAME, "ERR_APPEXECFWK_INSTALL_INCONSISTENT_MODULE_NAME"},
        {ERR_APPEXECFWK_INSTALL_SINGLETON_INCOMPATIBLE, "ERR_APPEXECFWK_INSTALL_SINGLETON_INCOMPATIBLE"},
        {ERR_BUNDLE_MANAGER_APP_CONTROL_DISALLOWED_INSTALL, "ERR_BUNDLE_MANAGER_APP_CONTROL_DISALLOWED_INSTALL"},
        {ERR_BUNDLE_MANAGER_APP_CONTROL_DISALLOWED_UNINSTALL, "ERR_BUNDLE_MANAGER_APP_CONTROL_DISALLOWED_UNINSTALL"},
        {ERR_APPEXECFWK_INSTALL_DEVICE_TYPE_NOT_SUPPORTED, "ERR_APPEXECFWK_INSTALL_DEVICE_TYPE_NOT_SUPPORTED"},
        {ERR_APPEXECFWK_INSTALL_DEPENDENT_MODULE_NOT_EXIST, "ERR_APPEXECFWK_INSTALL_DEPENDENT_MODULE_NOT_EXIST"},
        {ERR_APPEXECFWK_PARSE_PROFILE_PROP_SIZE_CHECK_ERROR, "ERR_APPEXECFWK_PARSE_PROFILE_PROP_SIZE_CHECK_ERROR"},
        {ERR_APPEXECFWK_INSTALL_ASAN_ENABLED_NOT_SAME, "ERR_APPEXECFWK_INSTALL_ASAN_ENABLED_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_ASAN_NOT_SUPPORT, "ERR_APPEXECFWK_INSTALL_ASAN_NOT_SUPPORT"},
        {ERR_APPEXECFWK_BUNDLE_TYPE_NOT_SAME, "ERR_APPEXECFWK_BUNDLE_TYPE_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_SHARE_APP_LIBRARY_NOT_ALLOWED, "ERR_APPEXECFWK_INSTALL_SHARE_APP_LIBRARY_NOT_ALLOWED"},
        {ERR_APPEXECFWK_INSTALL_COMPATIBLE_POLICY_NOT_SAME, "ERR_APPEXECFWK_INSTALL_COMPATIBLE_POLICY_NOT_SAME"},
        {ERR_APPEXECFWK_INSTALL_FILE_IS_SHARED_LIBRARY, "ERR_APPEXECFWK_INSTALL_FILE_IS_SHARED_LIBRARY"}
    };
}

BundleStatusCallbackProxy::BundleStatusCallbackProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<IBundleStatusCallback>(object)
{
    InitResultMap();
    APP_LOGI("create bundle status callback proxy instance");
}

BundleStatusCallbackProxy::~BundleStatusCallbackProxy()
{
    APP_LOGI("destroy bundle status callback proxy instance");
}

void BundleStatusCallbackProxy::OnBundleStateChanged(
    const uint8_t installType, const int32_t resultCode, const std::string &resultMsg, const std::string &bundleName)
{
    APP_LOGI("bundle state changed %{public}s", bundleName.c_str());
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    if (!data.WriteInterfaceToken(BundleStatusCallbackProxy::GetDescriptor())) {
        APP_LOGE("fail to OnBundleStateChanged due to write MessageParcel fail");
        return;
    }
    if (!data.WriteUint8(installType)) {
        APP_LOGE("fail to call OnBundleStateChanged, for write installType failed");
        return;
    }
    if (!data.WriteInt32(resultCode)) {
        APP_LOGE("fail to call OnBundleStateChanged, for write resultCode failed");
        return;
    }
    std::string msg = "";
    if (resultMap_.find(resultCode) != resultMap_.end()) {
        msg = resultMap_.at(resultCode);
    }
    if (!data.WriteString16(Str8ToStr16(msg))) {
        APP_LOGE("fail to call OnBundleStateChanged, for write resultMsg failed");
        return;
    }
    if (!data.WriteString16(Str8ToStr16(bundleName))) {
        APP_LOGE("fail to call OnBundleStateChanged, for write bundleName failed");
        return;
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        APP_LOGE("fail to call OnBundleStateChanged, for Remote() is nullptr");
        return;
    }

    int32_t ret = remote->SendRequest(
        static_cast<int32_t>(IBundleStatusCallback::Message::ON_BUNDLE_STATE_CHANGED), data, reply, option);
    if (ret != NO_ERROR) {
        APP_LOGW("fail to call OnBundleStateChanged, for transact is failed, error code is: %{public}d", ret);
    }
}
}  // namespace AppExecFwk
}  // namespace OHOS
