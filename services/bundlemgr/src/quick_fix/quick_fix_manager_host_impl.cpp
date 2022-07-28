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

#include "quick_fix_manager_host_impl.h"

#include "app_log_wrapper.h"
#include "quick_fix_data_mgr.h"

namespace OHOS {
namespace AppExecFwk {
bool QuickFixManagerHostImpl::DeployQuickFix(const std::vector<std::string> &bundleFilePaths,
    const sptr<IQuickFixStatusCallback> &statusCallback)
{
    APP_LOGI("QuickFixManagerHostImpl::DeployQuickFix start");
    if (bundleFilePaths.empty() || (statusCallback == nullptr)) {
        APP_LOGE("QuickFixManagerHostImpl::DeployQuickFix wrong parms");
        return false;
    }
    return DelayedSingleton<QuickFixDataMgr>::GetInstance()->DeployQuickFix(bundleFilePaths, statusCallback);
}

bool QuickFixManagerHostImpl::SwitchQuickFix(const std::string &bundleName,
    const sptr<IQuickFixStatusCallback> &statusCallback)
{
    APP_LOGI("QuickFixManagerHostImpl::SwitchQuickFix start");
    if (bundleName.empty() || (statusCallback == nullptr)) {
        APP_LOGE("QuickFixManagerHostImpl::SwitchQuickFix wrong parms");
        return false;
    }
    return DelayedSingleton<QuickFixDataMgr>::GetInstance()->SwitchQuickFix(bundleName, statusCallback);
}

bool QuickFixManagerHostImpl::DeleteQuickFix(const std::string &bundleName,
    const sptr<IQuickFixStatusCallback> &statusCallback)
{
    APP_LOGI("QuickFixManagerHostImpl::DeleteQuickFix start");
    if (bundleName.empty() || (statusCallback == nullptr)) {
        APP_LOGE("QuickFixManagerHostImpl::DeleteQuickFix wrong parms");
        return false;
    }
    return DelayedSingleton<QuickFixDataMgr>::GetInstance()->DeleteQuickFix(bundleName, statusCallback);
}
}
} // namespace OHOS