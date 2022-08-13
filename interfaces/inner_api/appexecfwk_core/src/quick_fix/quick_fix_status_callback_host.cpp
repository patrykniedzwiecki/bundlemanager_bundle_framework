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

#include "quick_fix_status_callback_host.h"

#include "app_log_wrapper.h"
#include "appexecfwk_errors.h"
#include "ipc_types.h"
#include "string_ex.h"

namespace OHOS {
namespace AppExecFwk {
QuickFixStatusCallbackHost::QuickFixStatusCallbackHost()
{
    APP_LOGI("create QuickFixStatusCallbackHost.");
    Init();
}

QuickFixStatusCallbackHost::~QuickFixStatusCallbackHost()
{
    APP_LOGI("destroy QuickFixStatusCallbackHost.");
}

void QuickFixStatusCallbackHost::Init()
{
    funcMap_.emplace(IQuickFixStatusCallback::Message::ON_PATCH_DEPLOYED,
        &QuickFixStatusCallbackHost::HandleOnPatchDeployed);
    funcMap_.emplace(IQuickFixStatusCallback::Message::ON_PATCH_SWITCHED,
        &QuickFixStatusCallbackHost::HandleOnPatchSwitched);
    funcMap_.emplace(IQuickFixStatusCallback::Message::ON_PATCH_DELETED,
        &QuickFixStatusCallbackHost::HandleOnPatchDeleted);
}

int QuickFixStatusCallbackHost::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    APP_LOGD("QuickFixStatusCallbackHost onReceived message, the message code is %{public}u", code);
    std::u16string descripter = QuickFixStatusCallbackHost::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    if (descripter != remoteDescripter) {
        APP_LOGE("fail to write reply message in clean cache host due to the reply is nullptr");
        return OBJECT_NULL;
    }
    if (funcMap_.find(code) != funcMap_.end() && funcMap_[code] != nullptr) {
        (this->*funcMap_[code])(data, reply);
    } else {
        APP_LOGW("quickfix callback host receives unknown code, code = %{public}u", code);
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    APP_LOGD("quickfix callback host finish to process message");
    return NO_ERROR;
}

void QuickFixStatusCallbackHost::HandleOnPatchDeployed(MessageParcel &data, MessageParcel &reply)
{
    APP_LOGI("start to process deployed patch callback message");
    DeployQuickFixResult deployQuickFixRes;
    std::unique_ptr<DeployQuickFixResult> resPtr(data.ReadParcelable<DeployQuickFixResult>());
    if (resPtr == nullptr) {
        APP_LOGE("read DeployQuickFixResult failed");
        deployQuickFixRes.resultCode = ERR_APPEXECFWK_QUICK_FIX_INTERNAL_ERROR;
        OnPatchDeployed(deployQuickFixRes);
        return;
    }
    deployQuickFixRes = *resPtr;

    OnPatchDeployed(deployQuickFixRes);
}

void QuickFixStatusCallbackHost::HandleOnPatchSwitched(MessageParcel &data, MessageParcel &reply)
{
    APP_LOGI("start to process switched patch callback message");
    SwitchQuickFixResult switchQuickFixRes;
    std::unique_ptr<SwitchQuickFixResult> resPtr(data.ReadParcelable<SwitchQuickFixResult>());
    if (resPtr == nullptr) {
        APP_LOGE("read SwitchQuickFixResult failed");
        switchQuickFixRes.resultCode = ERR_APPEXECFWK_QUICK_FIX_INTERNAL_ERROR;
        OnPatchSwitched(switchQuickFixRes);
        return;
    }
    switchQuickFixRes = *resPtr;

    OnPatchSwitched(switchQuickFixRes);
}

void QuickFixStatusCallbackHost::HandleOnPatchDeleted(MessageParcel &data, MessageParcel &reply)
{
    APP_LOGI("start to process deleted patch callback message");
    DeleteQuickFixResult deleteQuickFixRes;
    std::unique_ptr<DeleteQuickFixResult> resPtr(data.ReadParcelable<DeleteQuickFixResult>());
    if (resPtr == nullptr) {
        APP_LOGE("read DeleteQuickFixResult failed");
        deleteQuickFixRes.resultCode = ERR_APPEXECFWK_QUICK_FIX_INTERNAL_ERROR;
        OnPatchDeleted(deleteQuickFixRes);
        return;
    }
    deleteQuickFixRes = *resPtr;

    OnPatchDeleted(deleteQuickFixRes);
}
} // AppExecFwk
} // OHOS
