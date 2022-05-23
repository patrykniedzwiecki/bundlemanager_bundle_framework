/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_APPEXECFWK_SERVICES_DBMS_INCLUDE_EVENT_REPORT_H
#define FOUNDATION_APPEXECFWK_SERVICES_DBMS_INCLUDE_EVENT_REPORT_H

#include <string>

#include "appexecfwk_errors.h"
#include "bundle_constants.h"

namespace OHOS {
namespace AppExecFwk {
enum class DBMSEventType {
    UNKNOW = 0,
    GET_REMOTE_ABILITY_INFO,
    GET_REMOTE_ABILITY_INFOS
};

struct DBMSEventInfo {
    std::string deviceID;
    std::string bundleName;
    std::string localeInfo;
    std::string abilityName;
    uint32_t resultCode = 0;
};

class EventReport {
public:
    /**
     * @brief Send dbms system events.
     * @param dbmsEventType Indicates the dbms eventType.
     * @param eventInfo Indicates the eventInfo.
     */
    static void SendSystemEvent(DBMSEventType dbmsEventType, const DBMSEventInfo& eventInfo);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_SERVICES_DBMS_INCLUDE_EVENT_REPORT_H
