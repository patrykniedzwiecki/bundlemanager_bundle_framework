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

#include "default_app_mgr.h"

#include "bundle_data_mgr.h"
#include "bundle_mgr_service.h"
#include "bundle_permission_mgr.h"
#include "string_ex.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
    const std::string BROWSER = "BROWSER";
    const std::string IMAGE = "IMAGE";
    const std::string AUDIO = "AUDIO";
    const std::string VIDEO = "VIDEO";
    const std::string PERMISSION_GET_DEFAULT_APPLICATION = "ohos.permission.GET_DEFAULT_APPLICATION";
    const std::string PERMISSION_SET_DEFAULT_APPLICATION = "ohos.permission.SET_DEFAULT_APPLICATION";
}

DefaultAppMgr::DefaultAppMgr()
{
    APP_LOGI("create DefaultAppMgr.");
    InitSupportAppTypes();
}

DefaultAppMgr::~DefaultAppMgr()
{
    APP_LOGI("destroy DefaultAppMgr.");
}

bool DefaultAppMgr::IsDefaultApplication(int32_t userId, const std::string& type)
{
    return false;
}

bool DefaultAppMgr::GetDefaultApplication(int32_t userId, const std::string& type, BundleInfo& bundleInfo)
{
    return false;
}

bool DefaultAppMgr::SetDefaultApplication(int32_t userId, const std::string& type, const Element& element)
{
    return false;
}

bool DefaultAppMgr::ResetDefaultApplication(int32_t userId, const std::string& type)
{
    return false;
}

bool DefaultAppMgr::GetBundleInfo(int32_t userId, const std::string& type, const Element& element,
    BundleInfo& bundleInfo)
{
    return false;
}

bool DefaultAppMgr::IsMatch(const std::string& type, const std::vector<Skill>& skills)
{
    return false;
}

bool DefaultAppMgr::MatchAppType(const std::string& type, const std::vector<Skill>& skills)
{
    return false;
}

bool DefaultAppMgr::IsBrowserSkillsValid(const std::vector<Skill>& skills)
{
    return false;
}

bool DefaultAppMgr::IsImageSkillsValid(const std::vector<Skill>& skills)
{
    return false;
}

bool DefaultAppMgr::IsAudioSkillsValid(const std::vector<Skill>& skills)
{
    return false;
}

bool DefaultAppMgr::IsVideoSkillsValid(const std::vector<Skill>& skills)
{
    return false;
}

bool DefaultAppMgr::MatchFileType(const std::string& type, const std::vector<Skill>& skills)
{
    return false;
}

void DefaultAppMgr::InitSupportAppTypes()
{
    supportAppTypes.insert(BROWSER);
    supportAppTypes.insert(IMAGE);
    supportAppTypes.insert(AUDIO);
    supportAppTypes.insert(VIDEO);
}

bool DefaultAppMgr::IsAppType(const std::string& type)
{
    if (type.empty()) {
        return false;
    }
    return supportAppTypes.find(type) != supportAppTypes.end();
}

bool DefaultAppMgr::IsFileType(const std::string& type)
{
    return false;
}

bool DefaultAppMgr::IsUserIdExist(int32_t userId)
{
    std::shared_ptr<BundleDataMgr> dataMgr = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("get BundleDataMgr failed.");
        return false;
    }
    return dataMgr->HasUserId(userId);
}

bool DefaultAppMgr::IsElementValid(const Element& element)
{
    const std::string& bundleName = element.bundleName;
    const std::string& moduleName = element.moduleName;
    const std::string& abilityName = element.abilityName;
    const std::string& extensionName = element.extensionName;
    if (bundleName.empty()) {
        APP_LOGE("bundleName empty, Element invalid.");
        return false;
    }
    if (moduleName.empty()) {
        APP_LOGE("moduleName empty, Element invalid.");
        return false;
    }
    if (abilityName.empty() && extensionName.empty()) {
        APP_LOGE("abilityName and extensionName both empty, Element invalid.");
        return false;
    }
    if (!abilityName.empty() && !extensionName.empty()) {
        APP_LOGE("abilityName and extensionName both non-empty, Element invalid.");
        return false;
    }
    return true;
}
}
}