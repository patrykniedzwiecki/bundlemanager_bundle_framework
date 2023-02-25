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

#include "service_router_data_mgr.h"
#include "bundle_info_resolve_util.h"
#include "sr_constants.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "app_log_wrapper.h"
#include "sr_samgr_helper.h"


namespace OHOS {
namespace AppExecFwk {

ServiceRouterDataMgr::ServiceRouterDataMgr()
{
    APP_LOGD("SRDM instance is created");
}

ServiceRouterDataMgr::~ServiceRouterDataMgr()
{
    APP_LOGD("SRDM instance is destroyed");
    innerServiceInfos_.clear();
}

bool ServiceRouterDataMgr::LoadAllBundleInfos()
{
    APP_LOGI("SRDM LoadAllBundleInfos");
    auto bms = SrSamgrHelper::GetInstance().GetBundleMgr();
    if (bms == nullptr)
    {
        APP_LOGE("SRDM GetBundleMgr return null");
        return false;
    }
    auto flags = (BundleFlag::GET_BUNDLE_WITH_ABILITIES | BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO);
    std::vector<BundleInfo> bundleInfos;
    bool ret = bms->GetBundleInfos(flags, bundleInfos, Constants::ALL_USERID);
    if (!ret)
    {
        APP_LOGE("SRDM bms->GetBundleInfos return false");
    }
    for (const auto &bundleInfo : bundleInfos)
    {
        UpdateBundleInfo(bundleInfo);
    }
    return ret;
}

bool ServiceRouterDataMgr::LoadBundleInfo(const std::string &bundleName)
{
    APP_LOGD("SRDM LoadBundleInfo");
    auto bms = SrSamgrHelper::GetInstance().GetBundleMgr();
    if (bms == nullptr)
    {
        APP_LOGI("SRDM GetBundleMgr return null");
        return false;
    }
    BundleInfo bundleInfo;
    auto flags = (BundleFlag::GET_BUNDLE_WITH_ABILITIES | BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO);
    bool ret = bms->GetBundleInfo(bundleName, flags, bundleInfo, Constants::ALL_USERID);
    if (!ret)
    {
        APP_LOGE("SRDM bms->GetBundleInfos return false");
    }
    UpdateBundleInfo(bundleInfo);
    return ret;
}

bool ServiceRouterDataMgr::UpdateBundleInfo(const BundleInfo &bundleInfo)
{
    APP_LOGD("SRDM UpdateBundleInfo");
    std::vector<IntentInfo> intentInfos;
    std::vector<ServiceInfo> serviceInfos;
    if (BundleInfoResolveUtil::ResolveBundleInfo(bundleInfo, intentInfos, serviceInfos))
    {
        std::lock_guard<std::mutex> lock(bundleInfoMutex_);
        auto infoItem = innerServiceInfos_.find(bundleInfo.name);
        if (infoItem == innerServiceInfos_.end())
        {
            InnerServiceInfo innerServiceInfo;
            innerServiceInfo.UpdateInnerServiceInfo(bundleInfo, intentInfos, serviceInfos);
            innerServiceInfos_.try_emplace(bundleInfo.name, innerServiceInfo);
        }
        else
        {
            infoItem->second.UpdateInnerServiceInfo(bundleInfo, intentInfos, serviceInfos);
        }
        return true;
    }
    return false;
}

bool ServiceRouterDataMgr::DeleteBundleInfo(const std::string &bundleName)
{
    APP_LOGD("SRDM DeleteBundleInfo");
    std::lock_guard<std::mutex> lock(bundleInfoMutex_);
    auto infoItem = innerServiceInfos_.find(bundleName);
    if (infoItem == innerServiceInfos_.end())
    {
        APP_LOGE("SRDM inner service info not found by bundleName");
        return false;
    }
    innerServiceInfos_.erase(bundleName);
    return true;
}

int32_t ServiceRouterDataMgr::QueryServiceInfos(const Want &want, const ExtensionServiceType &serviceType, std::vector<ServiceInfo> &serviceInfos) const
{
    APP_LOGD("SRDM QueryServiceInfos");
    if (serviceType == ExtensionServiceType::UNSPECIFIED)
    {
        APP_LOGE("SRDM QueryServiceInfos, serviceType is empty");
        return ERR_BUNDLE_MANAGER_PARAM_ERROR;
    }
    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    if (bundleName.empty())
    {
        for (const auto &item : innerServiceInfos_)
        {
            item.second.FindServiceInfos(serviceType, serviceInfos);
        }
    }
    else
    {
        auto infoItem = innerServiceInfos_.find(bundleName);
        if (infoItem == innerServiceInfos_.end())
        {
            return ERR_OK;
        }
        infoItem->second.FindServiceInfos(serviceType, serviceInfos);
    }
    return ERR_OK;
}

int32_t ServiceRouterDataMgr::QueryIntentInfos(const Want &want, const std::string intentName, std::vector<IntentInfo> &intentInfos) const
{
    APP_LOGD("SRDM QueryIntentInfos");
    if (intentName.empty())
    {
        APP_LOGE("SRDM QueryIntentInfos, intentName is empty");
        return ERR_BUNDLE_MANAGER_PARAM_ERROR;
    }

    ElementName element = want.GetElement();
    std::string bundleName = element.GetBundleName();
    if (bundleName.empty())
    {
        for (const auto &item : innerServiceInfos_)
        {
            item.second.FindIntentInfos(intentName, intentInfos);
        }
    }
    else
    {
        auto infoItem = innerServiceInfos_.find(bundleName);
        if (infoItem == innerServiceInfos_.end())
        {
            return ERR_OK;
        }
        infoItem->second.FindIntentInfos(intentName, intentInfos);
    }
    return ERR_OK;
}

bool ServiceRouterDataMgr::IsContainsForm(const std::vector<IntentInfo> &intentInfos)
{
    bool isContainsForm = false;
    for (auto &intentInfo : intentInfos)
    {
        if (intentInfo.componentType == ComponentType::FORM)
        {
            isContainsForm = true;
            break;
        }
    }
    return isContainsForm;
}
}  // namespace AppExecFwk
}  // namespace OHOS
