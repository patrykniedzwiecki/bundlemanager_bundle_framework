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

#include "default_app_proxy.h"

#include "app_log_wrapper.h"
#include "hitrace_meter.h"
#include "ipc_types.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
DefaultAppProxy::DefaultAppProxy(const sptr<IRemoteObject>& object)
    : IRemoteProxy<IDefaultApp>(object)
{
    APP_LOGD("create DefaultAppProxy.");
}

DefaultAppProxy::~DefaultAppProxy()
{
    APP_LOGD("destroy DefaultAppProxy.");
}

bool DefaultAppProxy::IsDefaultApplication(const std::string& type)
{
    APP_LOGI("begin to call IsDefaultApplication.");
    HITRACE_METER_NAME(HITRACE_TAG_APP, __PRETTY_FUNCTION__);

    if (type.empty()) {
        APP_LOGE("type is empty.");
        return false;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        APP_LOGE("WriteInterfaceToken failed.");
        return false;
    }
    if (!data.WriteString(type)) {
        APP_LOGE("write type failed.");
        return false;
    }

    MessageParcel reply;
    if (!SendRequest(IDefaultApp::Message::IS_DEFAULT_APPLICATION, data, reply)) {
        APP_LOGE("SendRequest failed.");
        return false;
    }

    return reply.ReadBool();
}

bool DefaultAppProxy::GetDefaultApplication(int32_t userId, const std::string& type, BundleInfo& bundleInfo)
{
    APP_LOGI("begin to GetDefaultApplication.");
    HITRACE_METER_NAME(HITRACE_TAG_APP, __PRETTY_FUNCTION__);

    if (type.empty()) {
        APP_LOGE("type is empty.");
        return false;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        APP_LOGE("WriteInterfaceToken failed.");
        return false;
    }
    if (!data.WriteInt32(userId)) {
        APP_LOGE("write userId failed.");
        return false;
    }
    if (!data.WriteString(type)) {
        APP_LOGE("write type failed.");
        return false;
    }

    if (!GetParcelableInfo<BundleInfo>(IDefaultApp::Message::GET_DEFAULT_APPLICATION, data, bundleInfo)) {
        APP_LOGE("failed to GetDefaultApplication from server.");
        return false;
    }

    APP_LOGI("GetDefaultApplication success.");
    return true;
}

bool DefaultAppProxy::SetDefaultApplication(int32_t userId, const std::string& type, const Want& want)
{
    APP_LOGI("begin to SetDefaultApplication.");
    HITRACE_METER_NAME(HITRACE_TAG_APP, __PRETTY_FUNCTION__);

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        APP_LOGE("WriteInterfaceToken failed.");
        return false;
    }
    if (!data.WriteInt32(userId)) {
        APP_LOGE("write userId failed.");
        return false;
    }
    if (!data.WriteString(type)) {
        APP_LOGE("write type failed.");
        return false;
    }
    if (!data.WriteParcelable(&want)) {
        APP_LOGE("write want failed.");
        return false;
    }

    MessageParcel reply;
    if (!SendRequest(IDefaultApp::Message::SET_DEFAULT_APPLICATION, data, reply)) {
        APP_LOGE("SendRequest failed.");
        return false;
    }

    return reply.ReadBool();
}

bool DefaultAppProxy::ResetDefaultApplication(int32_t userId, const std::string& type)
{
    APP_LOGI("begin to ResetDefaultApplication.");
    HITRACE_METER_NAME(HITRACE_TAG_APP, __PRETTY_FUNCTION__);

    if (type.empty()) {
        APP_LOGE("type is empty.");
        return false;
    }

    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        APP_LOGE("WriteInterfaceToken failed.");
        return false;
    }
    if (!data.WriteInt32(userId)) {
        APP_LOGE("write userId failed.");
        return false;
    }
    if (!data.WriteString(type)) {
        APP_LOGE("write type failed.");
        return false;
    }

    MessageParcel reply;
    if (!SendRequest(IDefaultApp::Message::RESET_DEFAULT_APPLICATION, data, reply)) {
        APP_LOGE("SendRequest failed.");
        return false;
    }

    return reply.ReadBool();
}

template<typename T>
bool DefaultAppProxy::GetParcelableInfo(IDefaultApp::Message code, MessageParcel& data, T& parcelableInfo)
{
    MessageParcel reply;
    if (!SendRequest(code, data, reply)) {
        return false;
    }

    if (!reply.ReadBool()) {
        APP_LOGE("reply false.");
        return false;
    }

    std::unique_ptr<T> info(reply.ReadParcelable<T>());
    if (info == nullptr) {
        APP_LOGE("ReadParcelable failed.");
        return false;
    }
    parcelableInfo = *info;
    APP_LOGI("GetParcelableInfo success.");
    return true;
}

bool DefaultAppProxy::SendRequest(IDefaultApp::Message code, MessageParcel& data, MessageParcel& reply)
{
    MessageOption option(MessageOption::TF_SYNC);
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        APP_LOGE("failed to send request %{public}d due to remote object null.", code);
        return false;
    }
    int32_t result = remote->SendRequest(code, data, reply, option);
    if (result != NO_ERROR) {
        APP_LOGE("receive error code %{public}d in transact %{public}d", result, code);
        return false;
    }
    return true;
}
}
}
