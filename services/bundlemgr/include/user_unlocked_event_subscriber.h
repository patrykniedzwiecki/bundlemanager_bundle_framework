/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_USER_LOCKED_EVENT_SUBSCRIBER_H
#define FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_USER_LOCKED_EVENT_SUBSCRIBER_H

#include "bundle_info.h"
#include "common_event_data.h"
#include "common_event_subscriber.h"

namespace OHOS {
namespace AppExecFwk {
class UserUnLockedEventSubscriber final : public EventFwk::CommonEventSubscriber {
public:
    explicit UserUnLockedEventSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo);
    virtual ~UserUnLockedEventSubscriber();
    void OnReceiveEvent(const EventFwk::CommonEventData &data) override;

private:
    static bool JudgeBundleDataDir(const BundleInfo &bundleInfo, int32_t userId);
    static void UpdateAppDataDirSelinuxLabel(int32_t userId);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif // FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_USER_LOCKED_EVENT_SUBSCRIBER_H
