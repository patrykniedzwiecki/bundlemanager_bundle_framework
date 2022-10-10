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

#include "zlib_callback_info.h"

#include "app_log_wrapper.h"
#include "common_func.h"
#include "business_error.h"

namespace OHOS {
namespace AppExecFwk {
namespace LIBZIP {
namespace {
    constexpr size_t ARGS_ONE = 1;
}

ZlibCallbackInfo::ZlibCallbackInfo(napi_env env, napi_ref callback, napi_deferred deferred, bool isCallback)
    : env_(env), callback_(callback), deferred_(deferred), isCallBack_(isCallback) {}

ZlibCallbackInfo::~ZlibCallbackInfo() {}

int32_t ZlibCallbackInfo::ExcuteWork(uv_loop_s* loop, uv_work_t* work)
{
    int32_t ret = uv_queue_work(
        loop, work, [](uv_work_t* work) {},
        [](uv_work_t* work, int status) {
            if (work == nullptr) {
                return;
            }
            AsyncCallbackInfo* asyncCallbackInfo = reinterpret_cast<AsyncCallbackInfo*>(work->data);
            if (asyncCallbackInfo == nullptr) {
                return;
            }
            std::unique_ptr<AsyncCallbackInfo> callbackPtr {asyncCallbackInfo};
            napi_value result[ARGS_ONE] = {0};
            if (asyncCallbackInfo->deliverErrcode) {
                if (asyncCallbackInfo->callbackResult == ERR_OK) {
                    NAPI_CALL_RETURN_VOID(asyncCallbackInfo->env, napi_get_null(asyncCallbackInfo->env, &result[0]));
                } else {
                    result[0] = BusinessError::CreateCommonError(asyncCallbackInfo->env,
                        asyncCallbackInfo->callbackResult, "");
                }
            } else {
                NAPI_CALL_RETURN_VOID(asyncCallbackInfo->env,
                    napi_create_int32(asyncCallbackInfo->env, asyncCallbackInfo->callbackResult, &result[0]));
            }
            if (asyncCallbackInfo->isCallBack) {
                napi_value callback = 0;
                napi_value placeHolder = nullptr;
                NAPI_CALL_RETURN_VOID(asyncCallbackInfo->env, napi_get_reference_value(asyncCallbackInfo->env,
                    asyncCallbackInfo->callback, &callback));
                NAPI_CALL_RETURN_VOID(asyncCallbackInfo->env, napi_call_function(asyncCallbackInfo->env, nullptr,
                    callback, sizeof(result) / sizeof(result[0]), result, &placeHolder));
                if (asyncCallbackInfo->callback != nullptr) {
                    napi_delete_reference(asyncCallbackInfo->env, asyncCallbackInfo->callback);
                }
            } else {
                if (asyncCallbackInfo->callbackResult == ERR_OK) {
                    NAPI_CALL_RETURN_VOID(asyncCallbackInfo->env, napi_resolve_deferred(asyncCallbackInfo->env,
                        asyncCallbackInfo->deferred, result[0]));
                } else {
                    NAPI_CALL_RETURN_VOID(asyncCallbackInfo->env, napi_reject_deferred(asyncCallbackInfo->env,
                        asyncCallbackInfo->deferred, result[0]));
                }
            }
            if (work != nullptr) {
                delete work;
                work = nullptr;
            }
        });
    return ret;
}

void ZlibCallbackInfo::OnZipUnZipFinish(ErrCode result)
{
    // do callback or promise
    uv_loop_s* loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        APP_LOGE("%{public}s, loop is nullptr.", __func__);
        return;
    }
    uv_work_t* work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        APP_LOGE("create work failed!");
        return;
    }
    ErrCode err = ERR_OK;
    if (!deliverErrcode_) {
        err = result == ERR_OK ? ERR_OK : ERROR_CODE_ERRNO;
    } else {
        err = CommonFunc::ConvertErrCode(result);
    }

    AsyncCallbackInfo* asyncCallbackInfo = new (std::nothrow)AsyncCallbackInfo {
        .env = env_,
        .callback = callback_,
        .deferred = deferred_,
        .isCallBack = isCallBack_,
        .callbackResult = err,
        .deliverErrcode = deliverErrcode_,
    };
    std::unique_ptr<AsyncCallbackInfo> callbackPtr {asyncCallbackInfo};
    if (asyncCallbackInfo == nullptr) {
        delete work;
        return;
    }
    work->data = (void*)asyncCallbackInfo;
    int32_t ret = ExcuteWork(loop, work);
    if (ret != 0) {
        if (asyncCallbackInfo != nullptr) {
            delete asyncCallbackInfo;
        }
        if (work != nullptr) {
            delete work;
        }
    }
    callbackPtr.release();
}

bool ZlibCallbackInfo::GetIsCallback() const
{
    return isCallBack_;
}

void ZlibCallbackInfo::SetIsCallback(bool isCallback)
{
    isCallBack_ = isCallback;
}

void ZlibCallbackInfo::SetCallback(napi_ref callback)
{
    callback_ = callback;
}

void ZlibCallbackInfo::SetDeferred(napi_deferred deferred)
{
    deferred_ = deferred;
}

void ZlibCallbackInfo::SetDeliverErrCode(bool isDeliverErrCode)
{
    deliverErrcode_ = isDeliverErrCode;
}
}  // namespace LIBZIP
}  // namespace AppExecFwk
}  // namespace OHOS
