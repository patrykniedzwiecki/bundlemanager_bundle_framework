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
#include "bundle_manager.h"

#include <string>

#include "app_log_wrapper.h"
#include "bundle_errors.h"
#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "business_error.h"
#include "common_func.h"
#include "ipc_skeleton.h"
#include "napi_arg.h"
#include "napi_constants.h"

namespace OHOS {
namespace AppExecFwk {
using namespace OHOS::AAFwk;

static ErrCode InnerQueryAbilityInfos(napi_env env, const Want &want,
    int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos)
{
    auto iBundleMgr = CommonFunc::GetBundleMgr();
    if (iBundleMgr == nullptr) {
        APP_LOGE("iBundleMgr is null");
        return INTERNAL_ERROR;
    }
    ErrCode ret = iBundleMgr->QueryAbilityInfosV9(want, flags, userId, abilityInfos);
    return CommonFunc::ConvertErrCode(ret);
}

void QueryAbilityInfosExec(napi_env env, void *data)
{
    AbilityCallbackInfo *asyncCallbackInfo = reinterpret_cast<AbilityCallbackInfo *>(data);
    if (asyncCallbackInfo == nullptr) {
        APP_LOGE("asyncCallbackInfo is null");
        return;
    }
    asyncCallbackInfo->err = InnerQueryAbilityInfos(env, asyncCallbackInfo->want, asyncCallbackInfo->flags,
        asyncCallbackInfo->userId, asyncCallbackInfo->abilityInfos);
}

void QueryAbilityInfosComplete(napi_env env, napi_status status, void *data)
{
    AbilityCallbackInfo *asyncCallbackInfo = reinterpret_cast<AbilityCallbackInfo *>(data);
    if (asyncCallbackInfo == nullptr) {
        APP_LOGE("asyncCallbackInfo is null in %{public}s", __func__);
        return;
    }
    std::unique_ptr<AbilityCallbackInfo> callbackPtr {asyncCallbackInfo};
    napi_value result[2] = {0};
    if (asyncCallbackInfo->err == NO_ERROR) {
        NAPI_CALL_RETURN_VOID(env, napi_get_null(env, &result[0]));
        NAPI_CALL_RETURN_VOID(env, napi_create_array(env, &result[1]));
        CommonFunc::ConvertAbilityInfos(env, asyncCallbackInfo->abilityInfos, result[1]);
    } else {
        result[0] = BusinessError::CreateError(env, asyncCallbackInfo->err, "");
    }
    if (asyncCallbackInfo->deferred) {
        if (asyncCallbackInfo->err == NO_ERROR) {
            NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, asyncCallbackInfo->deferred, result[1]));
        } else {
            NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, asyncCallbackInfo->deferred, result[0]));
        }
    } else {
        napi_value callback = nullptr;
        napi_value placeHolder = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, asyncCallbackInfo->callback, &callback));
        NAPI_CALL_RETURN_VOID(env, napi_call_function(env, nullptr, callback,
            sizeof(result) / sizeof(result[0]), result, &placeHolder));
    }
}

napi_value QueryAbilityInfos(napi_env env, napi_callback_info info)
{
    APP_LOGD("begin to QueryAbilityInfos");
    NapiArg args(env, info);
    AbilityCallbackInfo *asyncCallbackInfo = new (std::nothrow) AbilityCallbackInfo(env);
    if (asyncCallbackInfo == nullptr) {
        APP_LOGE("asyncCallbackInfo is null");
        BusinessError::ThrowError(env, OUT_OF_MEMORY_ERROR);
        return nullptr;
    }
    asyncCallbackInfo->userId = IPCSkeleton::GetCallingUid() / Constants::BASE_USER_RANGE;
    std::unique_ptr<AbilityCallbackInfo> callbackPtr {asyncCallbackInfo};
    if (!args.Init(ARGS_SIZE_TWO, ARGS_SIZE_FOUR)) {
        APP_LOGE("param count invalid.");
        BusinessError::ThrowError(env, PARAM_CHECK_ERROR);
        return nullptr;
    }
    for (size_t i = 0; i < args.GetMaxArgc(); ++i) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, args[i], &valueType);
        if ((i == ARGS_POS_ZERO) && (valueType == napi_object)) {
            if (!CommonFunc::ParseWant(env, args[i], asyncCallbackInfo->want)) {
                APP_LOGE("invalid want");
                BusinessError::ThrowError(env, PARAM_CHECK_ERROR);
                return nullptr;
            }
        } else if ((i == ARGS_POS_ONE) && (valueType == napi_number)) {
            CommonFunc::ParseInt(env, asyncCallbackInfo->flags, args[i]);
        } else if (i == ARGS_POS_TWO) {
            if (valueType == napi_number) {
                CommonFunc::ParseInt(env, asyncCallbackInfo->userId, args[i]);
            } else if (valueType == napi_function) {
                NAPI_CALL(env, napi_create_reference(env, args[i], NAPI_RETURN_ONE, &asyncCallbackInfo->callback));
                break;
            } else {
                APP_LOGE("param check error");
                BusinessError::ThrowError(env, PARAM_CHECK_ERROR);
                return nullptr;
            }
        } else if ((i == ARGS_POS_THREE) && (valueType == napi_function)) {
            NAPI_CALL(env, napi_create_reference(env, args[i], NAPI_RETURN_ONE, &asyncCallbackInfo->callback));
        } else {
            APP_LOGE("param check error");
            BusinessError::ThrowError(env, PARAM_CHECK_ERROR);
            return nullptr;
        }
    }
    auto promise = CommonFunc::AsyncCallNativeMethod<AbilityCallbackInfo>(
        env, asyncCallbackInfo, "QueryAbilityInfos", QueryAbilityInfosExec, QueryAbilityInfosComplete);
    callbackPtr.release();
    APP_LOGD("call QueryAbilityInfos done");
    return promise;
}
}
}