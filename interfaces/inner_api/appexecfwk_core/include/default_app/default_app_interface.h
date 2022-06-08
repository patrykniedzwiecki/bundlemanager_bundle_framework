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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_DEFAULT_APP_INTERFACE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_DEFAULT_APP_INTERFACE_H

#include "bundle_info.h"
#include "iremote_broker.h"
#include "want.h"

namespace OHOS {
namespace AppExecFwk {
class IDefaultApp : public IRemoteBroker {
public:
    using Want = OHOS::AAFwk::Want;

    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.DefaultApp");
    
    virtual bool IsDefaultApplication(const std::string& type) = 0;
    virtual bool GetDefaultApplication(int32_t userId, const std::string& type, BundleInfo& bundleInfo) = 0;
    virtual bool SetDefaultApplication(int32_t userId, const std::string& type, const Want& want) = 0;
    virtual bool ResetDefaultApplication(int32_t userId, const std::string& type) = 0;

    enum Message : uint32_t {
        IS_DEFAULT_APPLICATION = 0,
        GET_DEFAULT_APPLICATION = 1,
        SET_DEFAULT_APPLICATION = 2,
        RESET_DEFAULT_APPLICATION = 3,
    };
};
}
}
#endif
