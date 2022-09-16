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

#include <unordered_map>

#include "installer.h"

#include "appexecfwk_errors.h"
#include "app_log_wrapper.h"
#include "bundle_errors.h"
#include "bundle_death_recipient.h"
#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "business_error.h"
#include "common_func.h"
#include "if_system_ability_manager.h"
#include "installer_callback.h"
#include "napi_arg.h"
#include "napi_constants.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
// resource name
const std::string RESOURCE_NAME_OF_GET_BUNDLE_INSTALLER = "GetBundleInstaller";
const std::string RESOURCE_NAME_OF_INSTALL = "Install";
// install message
const std::string MSG_INSTALL_SUCCESSFULLY = "Install successfully";
const std::string MSG_ERROR_BUNDLE_SERVICE_EXCEPTION = "error bundle service exception";
const std::string MSG_ERROR_INSTALL_PARSE_FAILED = "error install parse failed";
const std::string MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED = "error install verify signature failed";
const std::string MSG_ERROR_INSTALL_HAP_FILEPATH_INVALID = "error install hap file path invalid";
const std::string MSG_ERROR_INSTALL_HAP_SIZE_TOO_LARGE = "error install hap size too large";
const std::string MSG_ERROR_INSTALL_INCORRECT_SUFFIX = "error install incorrect suffix";
const std::string MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT = "error install multiple hap info inconsistent";
const std::string MSG_ERROR_INSTALL_NO_DISK_SPACE_LEFT = "error install no disk space left";
const std::string MSG_ERROR_INSTALL_VERSION_DOWNGRADE = "error install version downgrade";
const std::string MSG_ERROR_UNINSTALL_PREINSTALL_APP_FAILED = "error uninstall pre-install app failed";
const std::string MSG_ERROR_SYSTEM_IO_OPERATION = "error system io operation";
const std::string MSG_ERROR_PERMISSION_DENIED_ERROR = "error bundle permission deny";
const std::string MSG_ERROR_PARAM_CHECK_ERROR = "error input parameter invalid";
const std::string MSG_ERROR_INVALID_USER_ID = "error user id not found";
// property name
const std::string USER_ID = "userId";
const std::string INSTALL_FLAG = "installFlag";
const std::string IS_KEEP_DATA = "isKeepData";
const std::string CROWD_TEST_DEADLINE = "crowdtestDeadline";
const std::string MODULE_NAME = "moduleName";
const std::string HASH_VALUE = "hashValue";
const std::string HASH_PARAMS = "hashParams";

constexpr int32_t FIRST_PARAM = 0;
constexpr int32_t SECOND_PARAM = 1;
constexpr int32_t THIRD_PARAM = 2;
constexpr int32_t INSTALLER_PARAMS_NUMBER = 3;
} // namespace
napi_ref thread_local g_classBundleInstaller;

AsyncInstallCallbackInfo::~AsyncInstallCallbackInfo()
{
    if (callback) {
        napi_delete_reference(env, callback);
        callback = nullptr;
    }
    if (asyncWork) {
        napi_delete_async_work(env, asyncWork);
        asyncWork = nullptr;
    }
}

AsyncGetBundleInstallerCallbackInfo::~AsyncGetBundleInstallerCallbackInfo()
{
    if (callback) {
        napi_delete_reference(env, callback);
        callback = nullptr;
    }
    if (asyncWork) {
        napi_delete_async_work(env, asyncWork);
        asyncWork = nullptr;
    }
}

void GetBundleInstallerCompleted(napi_env env, napi_status status, void *data)
{
    AsyncGetBundleInstallerCallbackInfo *asyncCallbackInfo = (AsyncGetBundleInstallerCallbackInfo *)data;
    std::unique_ptr<AsyncGetBundleInstallerCallbackInfo> callbackPtr {asyncCallbackInfo};

    napi_value m_classBundleInstaller = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, g_classBundleInstaller,
        &m_classBundleInstaller));
    napi_value result[CALLBACK_PARAM_SIZE] = {0};
    NAPI_CALL_RETURN_VOID(env, napi_new_instance(env, m_classBundleInstaller, 0, nullptr, &result[SECOND_PARAM]));

    if (callbackPtr->deferred) {
        NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, callbackPtr->deferred, result[SECOND_PARAM]));
    } else {
        napi_value callback = CommonFunc::WrapVoidToJS(env);
        NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, callbackPtr->callback, &callback));
        napi_value undefined = CommonFunc::WrapVoidToJS(env);
        NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
        napi_value callResult = CommonFunc::WrapVoidToJS(env);
        NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, CALLBACK_PARAM_SIZE,
            &result[FIRST_PARAM], &callResult));
    }
}

/**
 * Promise and async callback
 */
napi_value GetBundleInstaller(napi_env env, napi_callback_info info)
{
    APP_LOGD("GetBundleInstaller called");
    NapiArg args(env, info);
    if (!args.Init(FIRST_PARAM, SECOND_PARAM)) {
        APP_LOGE("GetBundleInstaller args init failed");
        BusinessError::ThrowError(env, ERROR_PARAM_CHECK_ERROR);
        return nullptr;
    }
    std::unique_ptr<AsyncGetBundleInstallerCallbackInfo> callbackPtr =
        std::make_unique<AsyncGetBundleInstallerCallbackInfo>(env);

    auto argc = args.GetMaxArgc();
    APP_LOGD("GetBundleInstaller argc = [%{public}zu]", argc);
    // check param
    if (argc == SECOND_PARAM) {
        napi_value arg = args.GetArgv(argc - SECOND_PARAM);
        if (arg == nullptr) {
            APP_LOGE("the param is nullptr");
            BusinessError::ThrowError(env, ERROR_OUT_OF_MEMORY_ERROR);
            return nullptr;
        }
        napi_valuetype valuetype = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, arg, &valuetype));
        if (valuetype != napi_function) {
            APP_LOGE("the param type is invalid");
            BusinessError::ThrowError(env, ERROR_PARAM_CHECK_ERROR);
            return nullptr;
        }
        NAPI_CALL(env, napi_create_reference(env, arg, NAPI_RETURN_ONE, &callbackPtr->callback));
    }

    auto executeFunc = [](napi_env env, void *data) {};
    napi_value promise = CommonFunc::AsyncCallNativeMethod(
        env,
        callbackPtr.get(),
        RESOURCE_NAME_OF_GET_BUNDLE_INSTALLER,
        executeFunc,
        GetBundleInstallerCompleted);
    callbackPtr.release();
    return promise;
}

static void CreateErrCodeMap(std::unordered_map<int32_t, struct InstallResult> &errCodeMap)
{
    errCodeMap = {
        { IStatusReceiver::SUCCESS,
            { SUCCESS, MSG_INSTALL_SUCCESSFULLY }},
        { IStatusReceiver::ERR_INSTALL_INTERNAL_ERROR,
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION }},
        { IStatusReceiver::ERR_INSTALL_HOST_INSTALLER_FAILED,
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_INSTALLD_PARAM_ERROR, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_INSTALLD_GET_PROXY_ERROR, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_INSTALL_INSTALLD_SERVICE_ERROR, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_UNINSTALL_BUNDLE_MGR_SERVICE_ERROR, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_FAILED_SERVICE_DIED, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_FAILED_GET_INSTALLER_PROXY, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_USER_CREATE_FAILED, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_USER_REMOVE_FAILED, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_UNINSTALL_KILLING_APP_ERROR, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_INSTALL_GENERATE_UID_ERROR, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_INSTALL_STATE_ERROR, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_RECOVER_NOT_ALLOWED, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_RECOVER_GET_BUNDLEPATH_ERROR, 
            { ERROR_BUNDLE_SERVICE_EXCEPTION, MSG_ERROR_BUNDLE_SERVICE_EXCEPTION}},
        { IStatusReceiver::ERR_UNINSTALL_SYSTEM_APP_ERROR,
            { ERROR_UNINSTALL_PREINSTALL_APP_FAILED, MSG_ERROR_UNINSTALL_PREINSTALL_APP_FAILED}},
        { IStatusReceiver::ERR_INSTALL_PARSE_FAILED,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_UNEXPECTED,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_MISSING_BUNDLE,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_NO_PROFILE,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_BAD_PROFILE,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_PROFILE_PROP_TYPE_ERROR,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_PROFILE_MISSING_PROP,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_PERMISSION_ERROR,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_RPCID_FAILED,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_NATIVE_SO_FAILED,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARSE_MISSING_ABILITY,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_PROFILE_PARSE_FAIL,
            { ERROR_INSTALL_PARSE_FAILED, MSG_ERROR_INSTALL_PARSE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_VERIFICATION_FAILED,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_INCOMPATIBLE_SIGNATURE,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_INVALID_SIGNATURE_FILE_PATH,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_BAD_BUNDLE_SIGNATURE_FILE,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_NO_BUNDLE_SIGNATURE,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_VERIFY_APP_PKCS7_FAIL,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_APP_SOURCE_NOT_TRUESTED,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_BAD_DIGEST,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_BUNDLE_INTEGRITY_VERIFICATION_FAILURE,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_BAD_PUBLICKEY,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_BAD_BUNDLE_SIGNATURE,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_NO_PROFILE_BLOCK_FAIL,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_BUNDLE_SIGNATURE_VERIFICATION_FAILURE,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_VERIFY_SOURCE_INIT_FAIL,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_SINGLETON_INCOMPATIBLE,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_FAILED_INCONSISTENT_SIGNATURE,
            { ERROR_INSTALL_VERIFY_SIGNATURE_FAILED, MSG_ERROR_INSTALL_VERIFY_SIGNATURE_FAILED }},
        { IStatusReceiver::ERR_INSTALL_PARAM_ERROR,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_UNINSTALL_PARAM_ERROR,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_RECOVER_INVALID_BUNDLE_NAME,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_UNINSTALL_INVALID_NAME,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_INSTALL_INVALID_BUNDLE_FILE,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_INSTALL_FAILED_MODULE_NAME_EMPTY,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_INSTALL_FAILED_MODULE_NAME_DUPLICATE,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_INSTALL_FAILED_CHECK_HAP_HASH_PARAM,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_UNINSTALL_MISSING_INSTALLED_BUNDLE,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_UNINSTALL_MISSING_INSTALLED_MODULE,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_USER_NOT_INSTALL_HAP,
            { ERROR_PARAM_CHECK_ERROR, MSG_ERROR_PARAM_CHECK_ERROR }},
        { IStatusReceiver::ERR_INSTALL_FILE_PATH_INVALID,
            { ERROR_INSTALL_HAP_FILEPATH_INVALID, MSG_ERROR_INSTALL_HAP_FILEPATH_INVALID }},
        { IStatusReceiver::ERR_INSTALL_INVALID_HAP_SIZE, 
            { ERROR_INSTALL_HAP_SIZE_TOO_LARGE, MSG_ERROR_INSTALL_HAP_SIZE_TOO_LARGE }},
        { IStatusReceiver::ERR_INSTALL_FAILED_FILE_SIZE_TOO_LARGE, 
            { ERROR_INSTALL_HAP_SIZE_TOO_LARGE, MSG_ERROR_INSTALL_HAP_SIZE_TOO_LARGE }},
        { IStatusReceiver::ERR_INSTALL_INVALID_HAP_NAME,
            { ERROR_INSTALL_INCORRECT_SUFFIX, MSG_ERROR_INSTALL_INCORRECT_SUFFIX }},
        { IStatusReceiver::ERR_INSTALL_PERMISSION_DENIED,
            { ERROR_PERMISSION_DENIED_ERROR, MSG_ERROR_PERMISSION_DENIED_ERROR }},
        { IStatusReceiver::ERR_UNINSTALL_PERMISSION_DENIED,
            { ERROR_PERMISSION_DENIED_ERROR, MSG_ERROR_PERMISSION_DENIED_ERROR }},
        { IStatusReceiver::ERR_INSTALL_GRANT_REQUEST_PERMISSIONS_FAILED,
            { ERROR_PERMISSION_DENIED_ERROR, MSG_ERROR_PERMISSION_DENIED_ERROR }},
        { IStatusReceiver::ERR_INSTALL_UPDATE_HAP_TOKEN_FAILED,
            { ERROR_PERMISSION_DENIED_ERROR, MSG_ERROR_PERMISSION_DENIED_ERROR }},
        { IStatusReceiver::ERR_INSTALLD_CREATE_DIR_FAILED,
            { ERROR_SYSTEM_IO_OPERATION, MSG_ERROR_SYSTEM_IO_OPERATION }},
        { IStatusReceiver::ERR_INSTALLD_CHOWN_FAILED,
            { ERROR_SYSTEM_IO_OPERATION, MSG_ERROR_SYSTEM_IO_OPERATION }},
        { IStatusReceiver::ERR_INSTALLD_CREATE_DIR_EXIST,
            { ERROR_SYSTEM_IO_OPERATION, MSG_ERROR_SYSTEM_IO_OPERATION }},
        { IStatusReceiver::ERR_INSTALLD_REMOVE_DIR_FAILED,
            { ERROR_SYSTEM_IO_OPERATION, MSG_ERROR_SYSTEM_IO_OPERATION }},
        { IStatusReceiver::ERR_INSTALLD_EXTRACT_FILES_FAILED,
            { ERROR_SYSTEM_IO_OPERATION, MSG_ERROR_SYSTEM_IO_OPERATION }},
        { IStatusReceiver::ERR_INSTALLD_RNAME_DIR_FAILED,
            { ERROR_SYSTEM_IO_OPERATION, MSG_ERROR_SYSTEM_IO_OPERATION }},
        { IStatusReceiver::ERR_INSTALLD_CLEAN_DIR_FAILED,
            { ERROR_SYSTEM_IO_OPERATION, MSG_ERROR_SYSTEM_IO_OPERATION }},
        { IStatusReceiver::ERR_INSTALL_ENTRY_ALREADY_EXIST,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_ALREADY_EXIST,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_BUNDLENAME_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_VERSIONCODE_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_VERSIONNAME_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_MINCOMPATIBLE_VERSIONCODE_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_VENDOR_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_RELEASETYPE_TARGET_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_RELEASETYPE_COMPATIBLE_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_SINGLETON_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_ZERO_USER_WITH_NO_SINGLETON,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_CHECK_SYSCAP_FAILED,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_APPTYPE_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_URI_DUPLICATE,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_VERSION_NOT_COMPATIBLE,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_APP_DISTRIBUTION_TYPE_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_APP_PROVISION_TYPE_NOT_SAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_SO_INCOMPATIBLE,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_TYPE_ERROR,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_TYPE_ERROR,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_NOT_UNIQUE_DISTRO_MODULE_NAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_INCONSISTENT_MODULE_NAME,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_INVALID_NUMBER_OF_ENTRY_HAP,
            { ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT, MSG_ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT }},
        { IStatusReceiver::ERR_INSTALL_DISK_MEM_INSUFFICIENT,
            { ERROR_INSTALL_NO_DISK_SPACE_LEFT, MSG_ERROR_INSTALL_NO_DISK_SPACE_LEFT }},
        { IStatusReceiver::ERR_USER_NOT_EXIST,
            { ERROR_INVALID_USER_ID, MSG_ERROR_INVALID_USER_ID }},
        { IStatusReceiver::ERR_INSTALL_VERSION_DOWNGRADE,
            { ERROR_INSTALL_VERSION_DOWNGRADE, MSG_ERROR_INSTALL_VERSION_DOWNGRADE }}
    };
}

static void ConvertInstallResult(InstallResult &installResult)
{
    APP_LOGD("ConvertInstallResult = %{public}s.", installResult.resultMsg.c_str());
    std::unordered_map<int32_t, struct InstallResult> errCodeMap;
    CreateErrCodeMap(errCodeMap);
    auto iter = errCodeMap.find(installResult.resultCode);
    if (iter != errCodeMap.end()) {
        installResult.resultCode = iter->second.resultCode;
        installResult.resultMsg = iter->second.resultMsg;
        return;
    }
    installResult.resultCode = ERROR_BUNDLE_SERVICE_EXCEPTION;
    installResult.resultMsg = MSG_ERROR_BUNDLE_SERVICE_EXCEPTION;
}

static bool ParseHashParam(napi_env env, napi_value args, std::string &key, std::string &value)
{
    APP_LOGD("start to parse moduleName");
    bool ret = CommonFunc::ParseStringPropertyFromObject(env, args, MODULE_NAME, true, key);
    if (!ret || key.empty()) {
        APP_LOGE("param string moduleName is empty.");
        return false;
    }
    APP_LOGD("ParseHashParam moduleName=%{public}s.", key.c_str());

    APP_LOGD("start to parse hashValue");
    ret = CommonFunc::ParseStringPropertyFromObject(env, args, HASH_VALUE, true, value);
    if (!ret || value.empty()) {
        APP_LOGE("param string hashValue is empty.");
        return false;
    }
    APP_LOGD("ParseHashParam hashValue=%{public}s.", value.c_str());
    return true;
}

static bool ParseHashParams(napi_env env, napi_value args, std::map<std::string, std::string> &hashParams)
{
    APP_LOGD("start to parse hashParams");
    std::vector<napi_value> valueVec;
    bool res = CommonFunc::ParsePropertyArray(env, args, HASH_PARAMS, valueVec);
    if (!res) {
        APP_LOGE("parse hashParams failed");
        return res;
    }
    if (valueVec.empty()) {
        APP_LOGE("value vec is empty");
        return true;
    }
    for (const auto &property : valueVec) {
        std::string key;
        std::string value;
        if (!ParseHashParam(env, property, key, value)) {
            APP_LOGE("parse hash param failed");
            return false;
        }
        if (hashParams.find(key) != hashParams.end()) {
            APP_LOGE("moduleName(%{public}s) is duplicate", key.c_str());
            return false;
        }
        hashParams.emplace(key, value);
    }
    return true;
}

static bool ParseUserId(napi_env env, napi_value args, int32_t &userId)
{
    APP_LOGD("start to parse userId");
    PropertyInfo propertyInfo = {
        .propertyName = USER_ID,
        .isNecessary = false,
        .propertyType = napi_number
    };
    napi_value property = nullptr;
    bool res = CommonFunc::ParsePropertyFromObject(env, args, propertyInfo, property);
    if (!res) {
        APP_LOGE("parse userId failed");
        return res;
    }
    if (property != nullptr) {
        PARSE_PROPERTY(env, property, int32, userId);
        if (userId < Constants::DEFAULT_USERID) {
            APP_LOGE("param userId(%{public}d) is invalid.", userId);
            return false;
        }
    }
    APP_LOGD("param userId is %{public}d", userId);
    return true;
}

static bool ParseInstallFlag(napi_env env, napi_value args, InstallFlag &installFlag)
{
    APP_LOGD("start to parse installFlag");
    PropertyInfo propertyInfo = {
        .propertyName = INSTALL_FLAG,
        .isNecessary = false,
        .propertyType = napi_number
    };
    napi_value property = nullptr;
    bool res = CommonFunc::ParsePropertyFromObject(env, args, propertyInfo, property);
    if (!res) {
        APP_LOGE("parse installFlag failed");
        return res;
    }

    if (property != nullptr) {
        int32_t flag = 0;
        PARSE_PROPERTY(env, property, int32, flag);
        APP_LOGD("param installFlag is %{public}d", flag);
        installFlag = static_cast<OHOS::AppExecFwk::InstallFlag>(flag);
    }
    return true;
}

static bool ParseIsKeepData(napi_env env, napi_value args, bool &isKeepData)
{
    APP_LOGD("start to parse isKeepData");
    PropertyInfo propertyInfo = {
        .propertyName = IS_KEEP_DATA,
        .isNecessary = false,
        .propertyType = napi_boolean
    };
    napi_value property = nullptr;
    bool res = CommonFunc::ParsePropertyFromObject(env, args, propertyInfo, property);
    if (!res) {
        APP_LOGE("parse isKeepData failed");
        return res;
    }
    if (property != nullptr) {
        PARSE_PROPERTY(env, property, bool, isKeepData);
    }
    APP_LOGD("param isKeepData is %{public}d", isKeepData);
    return true;
}

static bool ParseCrowdtestDeadline(napi_env env, napi_value args, int64_t &crowdtestDeadline)
{
    APP_LOGD("start to parse crowdtestDeadline");
    PropertyInfo propertyInfo = {
        .propertyName = CROWD_TEST_DEADLINE,
        .isNecessary = false,
        .propertyType = napi_number
    };
    napi_value property = nullptr;
    bool res = CommonFunc::ParsePropertyFromObject(env, args, propertyInfo, property);
    if (!res) {
        APP_LOGE("parse crowdtestDeadline failed");
        return res;
    }
    if (property != nullptr) {
        PARSE_PROPERTY(env, property, int64, crowdtestDeadline);
    }
    APP_LOGD("param crowdtestDeadline is %{public}lld", crowdtestDeadline);
    return true;
}

static bool ParseInstallParam(napi_env env, napi_value args, InstallParam &installParam)
{
    if (!ParseUserId(env, args, installParam.userId) || !ParseInstallFlag(env, args, installParam.installFlag) ||
        !ParseIsKeepData(env, args, installParam.isKeepData) ||
        !ParseCrowdtestDeadline(env, args, installParam.crowdtestDeadline) ||
        !ParseHashParams(env, args, installParam.hashParams)) {
        APP_LOGE("ParseInstallParam failed");
        return false;
    }
    return true;
}

void InstallExecute(napi_env env, void *data)
{
    AsyncInstallCallbackInfo *asyncCallbackInfo = (AsyncInstallCallbackInfo *)data;
    const std::vector<std::string> bundleFilePath = asyncCallbackInfo->hapFiles;
    InstallParam installParam = asyncCallbackInfo->installParam;
    InstallResult installResult = asyncCallbackInfo->installResult;
    if (bundleFilePath.empty()) {
        installResult.resultCode = static_cast<int32_t>(IStatusReceiver::ERR_INSTALL_FILE_PATH_INVALID);
        return;
    }
    auto iBundleInstaller = CommonFunc::GetBundleInstaller();
    if ((iBundleInstaller == nullptr) || (iBundleInstaller->AsObject() == nullptr)) {
        APP_LOGE("can not get iBundleInstaller");
        installResult.resultCode = static_cast<int32_t>(IStatusReceiver::ERR_INSTALL_INTERNAL_ERROR);
        return;
    }

    sptr<InstallerCallback> callback = new (std::nothrow) InstallerCallback();
    sptr<BundleDeathRecipient> recipient(new (std::nothrow) BundleDeathRecipient(callback));
    if (callback == nullptr || recipient == nullptr) {
        APP_LOGE("callback nullptr");
        installResult.resultCode = static_cast<int32_t>(IStatusReceiver::ERR_INSTALL_INTERNAL_ERROR);
        return;
    }
    iBundleInstaller->AsObject()->AddDeathRecipient(recipient);

    if (installParam.installFlag == InstallFlag::NORMAL) {
        installParam.installFlag = InstallFlag::REPLACE_EXISTING;
    }
    ErrCode res = iBundleInstaller->StreamInstall(bundleFilePath, installParam, callback);
    if (res == ERR_APPEXECFWK_INSTALL_PARAM_ERROR) {
        APP_LOGE("install param error");
        installResult.resultCode = IStatusReceiver::ERR_INSTALL_PARAM_ERROR;
    } else if (res == ERR_APPEXECFWK_INSTALL_FILE_PATH_INVALID) {
        APP_LOGE("install invalid path");
        installResult.resultCode = IStatusReceiver::ERR_INSTALL_FILE_PATH_INVALID;
    } else {
        installResult.resultCode = callback->GetResultCode();
        APP_LOGD("InnerInstall resultCode %{public}d", installResult.resultCode);
        installResult.resultMsg = callback->GetResultMsg();
        APP_LOGD("InnerInstall resultMsg %{public}s", installResult.resultMsg.c_str());
    }
}

void InstallCompleted(napi_env env, napi_status status, void *data)
{
    AsyncInstallCallbackInfo *asyncCallbackInfo = (AsyncInstallCallbackInfo *)data;
    std::unique_ptr<AsyncInstallCallbackInfo> callbackPtr {asyncCallbackInfo};
    napi_value result[CALLBACK_PARAM_SIZE] = {0};
    NAPI_CALL_RETURN_VOID(env, napi_create_object(env, &result[SECOND_PARAM]));
    ConvertInstallResult(callbackPtr->installResult);
    napi_value nResultMsg;
    if (callbackPtr->installResult.resultCode == SUCCESS) {
        NAPI_CALL_RETURN_VOID(env,
            napi_create_string_utf8(env, MSG_INSTALL_SUCCESSFULLY.c_str(), NAPI_AUTO_LENGTH, &nResultMsg));
        NAPI_CALL_RETURN_VOID(env,
            napi_set_named_property(env, result[SECOND_PARAM], "resultMessage", nResultMsg));
    } else {
        result[FIRST_PARAM] = BusinessError::CreateError(env, callbackPtr->installResult.resultCode,
            callbackPtr->installResult.resultMsg);
    }

    if (callbackPtr->deferred) {
        if (callbackPtr->installResult.resultCode == 0) {
            NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, callbackPtr->deferred, result[SECOND_PARAM]));
        } else {
            NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, callbackPtr->deferred, result[FIRST_PARAM]));
        }
    } else {
        napi_value callback = CommonFunc::WrapVoidToJS(env);
        NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, callbackPtr->callback, &callback));

        napi_value undefined = CommonFunc::WrapVoidToJS(env);
        NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));

        napi_value callResult = CommonFunc::WrapVoidToJS(env);
        NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, CALLBACK_PARAM_SIZE,
            &result[FIRST_PARAM], &callResult));
    }
}

/**
 * Promise and async callback
 */
napi_value Install(napi_env env, napi_callback_info info)
{
    APP_LOGD("Install called");
    // obtain arguments of install interface
    NapiArg args(env, info);
    if (!args.Init(INSTALLER_PARAMS_NUMBER, INSTALLER_PARAMS_NUMBER)) {
        APP_LOGE("init param failed");
        BusinessError::ThrowError(env, ERROR_PARAM_CHECK_ERROR);
        return nullptr;
    }
    auto argc = args.GetMaxArgc();
    APP_LOGD("the number of argc is  %{public}zu", argc);
    if (argc < INSTALLER_PARAMS_NUMBER) {
        APP_LOGE("the params number is incorrect");
        BusinessError::ThrowError(env, ERROR_PARAM_CHECK_ERROR);
        return nullptr;
    }
    std::unique_ptr<AsyncInstallCallbackInfo> callbackPtr = std::make_unique<AsyncInstallCallbackInfo>(env);

    napi_value firstArg = args.GetArgv(FIRST_PARAM);
    napi_value secondArg = args.GetArgv(SECOND_PARAM);
    napi_value thirdArg = args.GetArgv(THIRD_PARAM);
    if (firstArg == nullptr || secondArg == nullptr || thirdArg == nullptr) {
        APP_LOGE("the params is nullptr");
        BusinessError::ThrowError(env, ERROR_PARAM_CHECK_ERROR);
        return nullptr;
    }

    std::vector<std::string> bundleFilePaths;
    InstallParam installParam;
    if ((CommonFunc::ParseStringArray(env, bundleFilePaths, firstArg) == nullptr) ||
        (!ParseInstallParam(env, secondArg, installParam))) {
        APP_LOGE("the param parse failed");
        BusinessError::ThrowError(env, ERROR_PARAM_CHECK_ERROR);
        return nullptr;
    }
    callbackPtr->hapFiles = bundleFilePaths;
    callbackPtr->installParam = installParam;

    if (argc == INSTALLER_PARAMS_NUMBER) {
        napi_valuetype valuetype = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, thirdArg, &valuetype));
        if (valuetype != napi_function) {
            APP_LOGE("Wrong argument type. Function expected.");
            BusinessError::ThrowError(env, ERROR_PARAM_CHECK_ERROR);
            return nullptr;
        }
        NAPI_CALL(env, napi_create_reference(env, thirdArg, NAPI_RETURN_ONE, &callbackPtr->callback));
    }

    auto promise = CommonFunc::AsyncCallNativeMethod(env, callbackPtr.get(), RESOURCE_NAME_OF_INSTALL, InstallExecute,
        InstallCompleted);
    callbackPtr.release();
    return promise;
}

napi_value Recover(napi_env env, napi_callback_info info)
{
    return nullptr;
}

napi_value Uninstall(napi_env env, napi_callback_info info)
{
    return nullptr;
}

napi_value BundleInstallerConstructor(napi_env env, napi_callback_info info)
{
    napi_value jsthis = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr));
    return jsthis;
}
} // AppExecFwk
} // OHOS