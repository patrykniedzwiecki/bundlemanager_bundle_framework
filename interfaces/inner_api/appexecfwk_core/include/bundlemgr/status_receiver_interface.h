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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_STATUS_RECEIVER_INTERFACE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_STATUS_RECEIVER_INTERFACE_H

#include "iremote_broker.h"

namespace OHOS {
namespace AppExecFwk {
class IStatusReceiver : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.StatusReceiver");

    /**
     * @brief Called when install status changed, with the percentage of installation progress
     * @param progress Indicates the percentage of the installation progress.
     */
    virtual void OnStatusNotify(const int progress) = 0;
    /**
     * @brief Called when an application is installed, updated, or uninstalled.
     * @param resultCode Indicates the status code returned for the application installation, update, or uninstallation
     *                   result.
     * @param resultMsg Indicates the result message returned with the status code.
     */
    virtual void OnFinished(const int32_t resultCode, const std::string &resultMsg) = 0;

    virtual void SetStreamInstallId(uint32_t installerId) = 0;

    enum class Message {
        ON_STATUS_NOTIFY,
        ON_FINISHED,
    };

    enum {
        SUCCESS = 0,
        ERR_INSTALL_INTERNAL_ERROR,
        ERR_INSTALL_HOST_INSTALLER_FAILED,
        ERR_INSTALL_PARSE_FAILED,
        ERR_INSTALL_VERSION_DOWNGRADE,
        ERR_INSTALL_VERIFICATION_FAILED,
        ERR_INSTALL_PARAM_ERROR,
        ERR_INSTALL_PERMISSION_DENIED,
        ERR_INSTALL_ENTRY_ALREADY_EXIST,
        ERR_INSTALL_STATE_ERROR,
        ERR_INSTALL_FILE_PATH_INVALID,
        ERR_INSTALL_INVALID_HAP_NAME,
        ERR_INSTALL_INVALID_BUNDLE_FILE,
        ERR_INSTALL_INVALID_HAP_SIZE,
        ERR_INSTALL_GENERATE_UID_ERROR,
        ERR_INSTALL_INSTALLD_SERVICE_ERROR,
        ERR_INSTALL_BUNDLE_MGR_SERVICE_ERROR,
        ERR_INSTALL_ALREADY_EXIST,
        ERR_INSTALL_BUNDLENAME_NOT_SAME,
        ERR_INSTALL_VERSIONCODE_NOT_SAME,
        ERR_INSTALL_VERSIONNAME_NOT_SAME,
        ERR_INSTALL_MINCOMPATIBLE_VERSIONCODE_NOT_SAME,
        ERR_INSTALL_VENDOR_NOT_SAME,
        ERR_INSTALL_RELEASETYPE_TARGET_NOT_SAME,
        ERR_INSTALL_RELEASETYPE_NOT_SAME,
        ERR_INSTALL_RELEASETYPE_COMPATIBLE_NOT_SAME,
        ERR_INSTALL_VERSION_NOT_COMPATIBLE,
        ERR_INSTALL_APP_DISTRIBUTION_TYPE_NOT_SAME,
        ERR_INSTALL_APP_PROVISION_TYPE_NOT_SAME,
        ERR_INSTALL_INVALID_NUMBER_OF_ENTRY_HAP,
        ERR_INSTALL_DISK_MEM_INSUFFICIENT,
        ERR_INSTALL_GRANT_REQUEST_PERMISSIONS_FAILED,
        ERR_INSTALL_UPDATE_HAP_TOKEN_FAILED,
        ERR_INSTALL_SINGLETON_NOT_SAME,
        ERR_INSTALL_ZERO_USER_WITH_NO_SINGLETON,
        ERR_INSTALL_CHECK_SYSCAP_FAILED,
        ERR_INSTALL_APPTYPE_NOT_SAME,
        ERR_INSTALL_URI_DUPLICATE,
        ERR_INSTALL_TYPE_ERROR,
        ERR_INSTALL_SDK_INCOMPATIBLE,
        ERR_INSTALL_SO_INCOMPATIBLE,
        ERR_INSTALL_AN_INCOMPATIBLE,
        ERR_INSTALL_NOT_UNIQUE_DISTRO_MODULE_NAME,
        ERR_INSTALL_INCONSISTENT_MODULE_NAME,
        ERR_INSTALL_SINGLETON_INCOMPATIBLE,
        ERR_INSTALL_DISALLOWED,
        ERR_INSTALL_DEVICE_TYPE_NOT_SUPPORTED,
        ERR_INSTALL_DEPENDENT_MODULE_NOT_EXIST,
        ERR_INSTALL_ASAN_ENABLED_NOT_SAME,
        ERR_INSTALL_ASAN_ENABLED_NOT_SUPPORT,
        ERR_INSTALL_BUNDLE_TYPE_NOT_SAME,
        ERR_INSTALL_SHARE_APP_LIBRARY_NOT_ALLOWED,
        ERR_INSTALL_COMPATIBLE_POLICY_NOT_SAME,
        ERR_APPEXECFWK_UNINSTALL_SHARE_APP_LIBRARY_IS_NOT_EXIST,
        ERR_APPEXECFWK_UNINSTALL_SHARE_APP_LIBRARY_IS_RELIED,
        ERR_APPEXECFWK_UNINSTALL_BUNDLE_IS_SHARED_LIBRARY,
        ERR_INSTALL_FILE_IS_SHARED_LIBRARY,
        ERR_INSATLL_CHECK_PROXY_DATA_URI_FAILED,
        ERR_INSATLL_CHECK_PROXY_DATA_PERMISSION_FAILED,

        // signature errcode
        ERR_INSTALL_FAILED_INVALID_SIGNATURE_FILE_PATH,
        ERR_INSTALL_FAILED_BAD_BUNDLE_SIGNATURE_FILE,
        ERR_INSTALL_FAILED_NO_BUNDLE_SIGNATURE,
        ERR_INSTALL_FAILED_VERIFY_APP_PKCS7_FAIL,
        ERR_INSTALL_FAILED_PROFILE_PARSE_FAIL,
        ERR_INSTALL_FAILED_APP_SOURCE_NOT_TRUESTED,
        ERR_INSTALL_FAILED_BAD_DIGEST,
        ERR_INSTALL_FAILED_BUNDLE_INTEGRITY_VERIFICATION_FAILURE,
        ERR_INSTALL_FAILED_FILE_SIZE_TOO_LARGE,
        ERR_INSTALL_FAILED_BAD_PUBLICKEY,
        ERR_INSTALL_FAILED_BAD_BUNDLE_SIGNATURE,
        ERR_INSTALL_FAILED_NO_PROFILE_BLOCK_FAIL,
        ERR_INSTALL_FAILED_BUNDLE_SIGNATURE_VERIFICATION_FAILURE,
        ERR_INSTALL_FAILED_VERIFY_SOURCE_INIT_FAIL,
        ERR_INSTALL_FAILED_INCOMPATIBLE_SIGNATURE,
        ERR_INSTALL_FAILED_INCONSISTENT_SIGNATURE,
        ERR_INSTALL_FAILED_MODULE_NAME_EMPTY,
        ERR_INSTALL_FAILED_MODULE_NAME_DUPLICATE,
        ERR_INSTALL_FAILED_CHECK_HAP_HASH_PARAM,

        ERR_INSTALL_PARSE_UNEXPECTED,
        ERR_INSTALL_PARSE_MISSING_BUNDLE,
        ERR_INSTALL_PARSE_MISSING_ABILITY,
        ERR_INSTALL_PARSE_NO_PROFILE,
        ERR_INSTALL_PARSE_BAD_PROFILE,
        ERR_INSTALL_PARSE_PROFILE_PROP_TYPE_ERROR,
        ERR_INSTALL_PARSE_PROFILE_MISSING_PROP,
        ERR_INSTALL_PARSE_PERMISSION_ERROR,
        ERR_INSTALL_PARSE_PROFILE_PROP_CHECK_ERROR,
        ERR_INSTALL_PARSE_PROFILE_PROP_SIZE_CHECK_ERROR,
        ERR_INSTALL_PARSE_RPCID_FAILED,
        ERR_INSTALL_PARSE_NATIVE_SO_FAILED,
        ERR_INSTALL_PARSE_AN_FAILED,

        ERR_INSTALLD_PARAM_ERROR,
        ERR_INSTALLD_GET_PROXY_ERROR,
        ERR_INSTALLD_CREATE_DIR_FAILED,
        ERR_INSTALLD_CREATE_DIR_EXIST,
        ERR_INSTALLD_CHOWN_FAILED,
        ERR_INSTALLD_REMOVE_DIR_FAILED,
        ERR_INSTALLD_EXTRACT_FILES_FAILED,
        ERR_INSTALLD_RNAME_DIR_FAILED,
        ERR_INSTALLD_CLEAN_DIR_FAILED,
        ERR_INSTALLD_PERMISSION_DENIED,
        ERR_INSTALLD_SET_SELINUX_LABEL_FAILED,

        // overlay installation
        ERR_OVERLAY_INSTALLATION_FAILED_INTERNAL_ERROR,
        ERR_OVERLAY_INSTALLATION_FAILED_INVALID_BUNDLE_NAME,
        ERR_OVERLAY_INSTALLATION_FAILED_INVALID_MODULE_NAME,
        ERR_OVERLAY_INSTALLATION_FAILED_ERROR_HAP_TYPE,
        ERR_OVERLAY_INSTALLATION_FAILED_ERROR_BUNDLE_TYPE,
        ERR_OVERLAY_INSTALLATION_FAILED_TARGET_BUNDLE_NAME_MISSED,
        ERR_OVERLAY_INSTALLATION_FAILED_TARGET_MODULE_NAME_MISSED,
        ERR_OVERLAY_INSTALLATION_FAILED_TARGET_BUNDLE_NAME_NOT_SAME,
        ERR_OVERLAY_INSTALLATION_FAILED_INTERNAL_EXTERNAL_OVERLAY_EXISTED_SIMULTANEOUSLY,
        ERR_OVERLAY_INSTALLATION_FAILED_TARGET_PRIORITY_NOT_SAME,
        ERR_OVERLAY_INSTALLATION_FAILED_INVALID_PRIORITY,
        ERR_OVERLAY_INSTALLATION_FAILED_INCONSISTENT_VERSION_CODE,
        ERR_OVERLAY_INSTALLATION_FAILED_SERVICE_EXCEPTION,
        ERR_OVERLAY_INSTALLATION_FAILED_BUNDLE_NAME_SAME_WITH_TARGET_BUNDLE_NAME,
        ERR_OVERLAY_INSTALLATION_FAILED_NO_SYSTEM_APPLICATION_FOR_EXTERNAL_OVERLAY,
        ERR_OVERLAY_INSTALLATION_FAILED_DIFFERENT_SIGNATURE_CERTIFICATE,
        ERR_OVERLAY_INSTALLATION_FAILED_TARGET_BUNDLE_IS_OVERLAY_BUNDLE,
        ERR_OVERLAY_INSTALLATION_FAILED_TARGET_MODULE_IS_OVERLAY_MODULE,
        ERR_OVERLAY_INSTALLATION_FAILED_OVERLAY_TYPE_NOT_SAME,
        ERR_OVERLAY_INSTALLATION_FAILED_INVALID_BUNDLE_DIR,

        ERR_UNINSTALL_SYSTEM_APP_ERROR,
        ERR_UNINSTALL_KILLING_APP_ERROR,
        ERR_UNINSTALL_INVALID_NAME,
        ERR_UNINSTALL_PARAM_ERROR,
        ERR_UNINSTALL_PERMISSION_DENIED,
        ERR_UNINSTALL_BUNDLE_MGR_SERVICE_ERROR,
        ERR_UNINSTALL_MISSING_INSTALLED_BUNDLE,
        ERR_UNINSTALL_MISSING_INSTALLED_MODULE,
        ERR_UNINSTALL_DISALLOWED,
        ERR_UNKNOWN,

        ERR_FAILED_GET_INSTALLER_PROXY,
        ERR_FAILED_SERVICE_DIED,
        ERR_OPERATION_TIME_OUT,

        ERR_RECOVER_GET_BUNDLEPATH_ERROR = 201,
        ERR_RECOVER_INVALID_BUNDLE_NAME,
        ERR_RECOVER_NOT_ALLOWED,

        ERR_USER_NOT_EXIST = 301,
        ERR_USER_CREATE_FAILED,
        ERR_USER_REMOVE_FAILED,
        ERR_USER_NOT_INSTALL_HAP,
    };
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_STATUS_RECEIVER_INTERFACE_H