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

#include "app_control_proxy.h"

#include "app_log_wrapper.h"
#include "appexecfwk_errors.h"
#include "ipc_types.h"

namespace OHOS {
namespace AppExecFwk {
AppControlProxy::AppControlProxy(const sptr<IRemoteObject> &object) : IRemoteProxy<IAppControlMgr>(object)
{
    APP_LOGD("create AppControlProxy.");
}

AppControlProxy::~AppControlProxy()
{
    APP_LOGD("destroy AppControlProxy.");
}

ErrCode AppControlProxy::AddAppInstallControlRule(const std::vector<std::string> &appIds,
    const AppInstallControlRuleType controlRuleType, int32_t userId)
{
    APP_LOGI("begin to call AddAppInstallControlRule.");
    if (appIds.size() == 0) {
        APP_LOGE("AddAppInstallControlRule failed due to params error.");
        return ERR_INVALID_VALUE;
    }
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        APP_LOGE("WriteInterfaceToken failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!WriteParcelableVector(appIds, data)) {
        APP_LOGE("write appIds failed");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!data.WriteInt32(static_cast<int32_t>(controlRuleType))) {
        APP_LOGE("write controlRuleType failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!data.WriteInt32(userId)) {
        APP_LOGE("write userId failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }

    MessageParcel reply;
    return SendRequest(IAppControlMgr::Message::ADD_APP_INSTALL_CONTROL_RULE, data, reply);
}

ErrCode AppControlProxy::DeleteAppInstallControlRule(const std::vector<std::string> &appIds, int32_t userId)
{
    APP_LOGI("begin to call DeleteAppInstallControlRule.");
    if (appIds.size() == 0) {
        APP_LOGE("DeleteAppInstallControlRule failed due to params error.");
        return ERR_INVALID_VALUE;
    }
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        APP_LOGE("WriteInterfaceToken failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!WriteParcelableVector(appIds, data)) {
        APP_LOGE("write appIds failed");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!data.WriteInt32(userId)) {
        APP_LOGE("write userId failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }

    MessageParcel reply;
    return SendRequest(IAppControlMgr::Message::DELETE_APP_INSTALL_CONTROL_RULE, data, reply);
}

ErrCode AppControlProxy::DeleteAppInstallControlRule(
    const AppInstallControlRuleType controlRuleType, int32_t userId)
{
    APP_LOGI("begin to call DeleteAppInstallControlRule.");
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        APP_LOGE("WriteInterfaceToken failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!data.WriteInt32(static_cast<int32_t>(controlRuleType))) {
        APP_LOGE("write controlRuleType failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!data.WriteInt32(userId)) {
        APP_LOGE("write userId failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }

    MessageParcel reply;
    return SendRequest(IAppControlMgr::Message::CLEAN_APP_INSTALL_CONTROL_RULE, data, reply);
}

ErrCode AppControlProxy::GetAppInstallControlRule(
    const AppInstallControlRuleType controlRuleType, int32_t userId, std::vector<std::string> &appIds)
{
    APP_LOGI("begin to call GetAppInstallControlRule.");
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        APP_LOGE("WriteInterfaceToken failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!data.WriteInt32(static_cast<int32_t>(controlRuleType))) {
        APP_LOGE("write controlRuleType failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    if (!data.WriteInt32(userId)) {
        APP_LOGE("write userId failed.");
        return ERR_APPEXECFWK_PARCEL_ERROR;
    }
    return GetParcelableInfos(IAppControlMgr::Message::GET_APP_INSTALL_CONTROL_RULE, data, appIds);
}

bool AppControlProxy::WriteParcelableVector(const std::vector<std::string> &stringVector, MessageParcel &data)
{
    if (!data.WriteInt32(stringVector.size())) {
        APP_LOGE("write ParcelableVector failed");
        return false;
    }

    for (auto &string : stringVector) {
        if (!data.WriteString(string)) {
            APP_LOGE("write string failed");
            return false;
        }
    }
    return true;
}

int32_t AppControlProxy::GetParcelableInfos(
    IAppControlMgr::Message code, MessageParcel &data, std::vector<std::string> &stringVector)
{
    MessageParcel reply;
    int32_t ret = SendRequest(code, data, reply);
    if (ret != NO_ERROR) {
        return ret;
    }

    int32_t infoSize = reply.ReadInt32();
    for (int32_t i = 0; i < infoSize; i++) {
        stringVector.emplace_back(reply.ReadString());
    }
    APP_LOGD("Read string vector success");
    return NO_ERROR;
}

int32_t AppControlProxy::SendRequest(IAppControlMgr::Message code, MessageParcel &data, MessageParcel &reply)
{
    MessageOption option(MessageOption::TF_SYNC);
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        APP_LOGE("failed to send request %{public}d due to remote object null.", code);
        return ERR_INVALID_OPERATION;
    }
    int32_t result = remote->SendRequest(code, data, reply, option);
    if (result != NO_ERROR) {
        APP_LOGE("receive error code %{public}d in transact %{public}d", result, code);
    }
    return result;
}
} // AppExecFwk
} // OHOS