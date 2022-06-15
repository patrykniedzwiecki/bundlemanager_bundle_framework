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
#include "aging/bundle_aging_mgr.h"

#include "account_helper.h"
#include "battery_srv_client.h"
#ifdef DEVICE_USAGE_STATISTICS_ENABLE
#include "bundle_active_period_stats.h"
#endif
#include "bundle_mgr_service.h"
#include "bundle_util.h"
#include "display_power_mgr_client.h"
#include "parameter.h"

namespace OHOS {
namespace AppExecFwk {
BundleAgingMgr::BundleAgingMgr()
{
    InitAgingHandlerChain();
    APP_LOGI("BundleAgingMgr is created.");
}

BundleAgingMgr::~BundleAgingMgr()
{
    APP_LOGI("BundleAgingMgr is destroyed");
}

void BundleAgingMgr::InitAgingRunner()
{
    auto agingRunner = EventRunner::Create(AgingConstants::AGING_THREAD);
    if (agingRunner == nullptr) {
        APP_LOGE("create aging runner failed");
        return;
    }
    SetEventRunner(agingRunner);
}
void BundleAgingMgr::InitAgingTimerInterval()
{
    char szTimerThresold[AgingConstants::THRESHOLD_VAL_LEN] = {0};
    int32_t ret = GetParameter(AgingConstants::SYSTEM_PARAM_AGING_TIMER_INTERVAL.c_str(), "", szTimerThresold,
        AgingConstants::THRESHOLD_VAL_LEN);
    APP_LOGD("ret is %{public}d, szTimerThresold is %{public}d", ret, atoi(szTimerThresold));
    if (ret <= 0) {
        APP_LOGD("GetParameter failed");
        return;
    }
    if (strcmp(szTimerThresold, "") != 0) {
        agingTimerInterval = atoi(szTimerThresold);
        APP_LOGD("BundleAgingMgr init aging timer success");
    }
}

void BundleAgingMgr::InitAgingBatteryThresold()
{
    char szBatteryThresold[AgingConstants::THRESHOLD_VAL_LEN] = {0};
    int32_t ret = GetParameter(AgingConstants::SYSTEM_PARAM_AGING_BATTER_THRESHOLD.c_str(), "", szBatteryThresold,
        AgingConstants::THRESHOLD_VAL_LEN);
    APP_LOGD("ret is %{public}d, szBatteryThresold is %{public}d", ret, atoi(szBatteryThresold));
    if (ret <= 0) {
        APP_LOGD("GetParameter failed");
        return;
    }
    if (strcmp(szBatteryThresold, "") != 0) {
        agingBatteryThresold = atoi(szBatteryThresold);
        APP_LOGD("BundleAgingMgr init battery threshold success");
    }
}

void BundleAgingMgr::InitAgingtTimer()
{
    InitAgingBatteryThresold();
    InitAgingTimerInterval();
    bool isEventStarted = SendEvent(InnerEvent::Get(EVENT_AGING_NOW), agingTimerInterval);
    if (!isEventStarted) {
        APP_LOGE("faild to send event is not started");
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running = false;
        }
    }
}

#ifdef DEVICE_USAGE_STATISTICS_ENABLE
int BundleAgingMgr::AgingQueryFormStatistics(std::vector<DeviceUsageStats::BundleActiveModuleRecord>& results,
    const std::shared_ptr<BundleDataMgr> &dataMgr)
{
    int32_t userId = AccountHelper::GetCurrentActiveUserId();
    int ret = DeviceUsageStats::BundleActiveClient::GetInstance().QueryFormStatistics(
        AgingConstants::COUNT_MODULE_RECODES_GET, results, userId);
    APP_LOGD("activeModuleRecord size %{public}zu, ret:%{public}d", results.size(), ret);
    return ret;
}
#endif

bool BundleAgingMgr::ReInitAgingRequest(const std::shared_ptr<BundleDataMgr> &dataMgr)
{
    if (dataMgr == nullptr) {
        APP_LOGE("ReInitAgingRequest: dataMgr is null");
        return false;
    }
    request.RequestReset();
    std::map<std::string, int> bundleNamesAndUid;
    dataMgr->GetRemovableBundleNameVec(bundleNamesAndUid);
    if (bundleNamesAndUid.empty()) {
        APP_LOGE("ReInitAgingRequest: no removable bundles");
        return false;
    }
    APP_LOGD("ReInitAgingRequest: removable bundles size %{public}zu", bundleNamesAndUid.size());

#ifdef DEVICE_USAGE_STATISTICS_ENABLE
    std::vector<DeviceUsageStats::BundleActiveModuleRecord> activeModuleRecord;
    int ret = AgingQueryFormStatistics(activeModuleRecord, dataMgr);
    if (ret != 0) {
        APP_LOGE("ReInitAgingRequest: can not get bundle active module record");
        return false;
    }
    int64_t lastBundleUsedTime = 0;
    int64_t lastLaunchTimesMs = AgingUtil::GetNowSysTimeMs();
    APP_LOGD("now: %{public}" PRId64, lastLaunchTimesMs);
    for (auto iter : bundleNamesAndUid) {
        int64_t dataBytes = dataMgr->GetBundleSpaceSize(iter.first);
        // the value of lastLaunchTimesMs get from lastLaunchTimesMs interface
        lastBundleUsedTime = 0;
        for (const auto &moduleRecord : activeModuleRecord) {
            APP_LOGD("%{public}s: %{public}" PRId64, moduleRecord.bundleName_.c_str(),
                moduleRecord.lastModuleUsedTime_);
            if (moduleRecord.bundleName_ == iter.first && lastBundleUsedTime < moduleRecord.lastModuleUsedTime_) {
                lastBundleUsedTime = moduleRecord.lastModuleUsedTime_;
            }
        }
        if (lastBundleUsedTime) {
            APP_LOGD("%{public}s: %{public}" PRId64, iter.first.c_str(), lastBundleUsedTime);
            AgingBundleInfo agingBundleInfo(iter.first, lastBundleUsedTime, dataBytes, iter.second);
            request.AddAgingBundle(agingBundleInfo);
        } else {
            APP_LOGD("%{public}s: %{public}" PRId64, iter.first.c_str(), lastLaunchTimesMs);
            AgingBundleInfo agingBundleInfo(iter.first, lastLaunchTimesMs, dataBytes, iter.second);
            request.AddAgingBundle(agingBundleInfo);
        }
    }
    request.SetTotalDataBytes(dataMgr->GetAllFreeInstallBundleSpaceSize());
#endif
    return request.SortAgingBundles() > 0;
}

void BundleAgingMgr::Process(const std::shared_ptr<BundleDataMgr> &dataMgr)
{
    APP_LOGD("BundleAging begin to process.");
    if (ReInitAgingRequest(dataMgr)) {
        if (request.IsReachStartAgingThreshold()) {
            chain.Process(request);
        }
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running = false;
    }
    APP_LOGD("BundleAgingMgr Process done");
}

void BundleAgingMgr::Start(AgingTriggertype type)
{
    APP_LOGD("aging start, AgingTriggertype: %{public}d", type);
    if (!CheckPrerequisite(type)) {
        APP_LOGE("BundleAgingMgr aging Prerequisite is not satisfied");
        return;
    }

    auto dataMgr = OHOS::DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr();
    if (dataMgr == nullptr) {
        APP_LOGE("dataMgr is null");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running) {
            APP_LOGD("BundleAgingMgr is running, no need to start is again");
            return;
        }
        running = true;
    }

    auto task = [&, dataMgr]() { Process(dataMgr); };
    bool isEventStarted = SendEvent(InnerEvent::Get(task));
    if (!isEventStarted) {
        APP_LOGE("BundleAgingMgr event is not started.");
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running = false;
        }
    } else {
        APP_LOGD("BundleAgingMgr schedule process done");
    }
}

bool BundleAgingMgr::CheckPrerequisite(AgingTriggertype type) const
{
    if (type != AgingTriggertype::PREIOD) {
        return true;
    }
    DisplayPowerMgr::DisplayState state = DisplayPowerMgr::DisplayPowerMgrClient::GetInstance().GetDisplayState();
    if (state == DisplayPowerMgr::DisplayState::DISPLAY_ON) {
        APP_LOGD("current Displaystate is DisplayState::DISPLAY_ON");
        return false;
    }
    int32_t currentBatteryCap = OHOS::PowerMgr::BatterySrvClient::GetInstance().GetCapacity();
    APP_LOGD("current GetCapacity is %{public}d agingBatteryThresold: %{public}" PRId64,
        currentBatteryCap, agingBatteryThresold);
    return currentBatteryCap > agingBatteryThresold;
}

void BundleAgingMgr::ProcessEvent(const InnerEvent::Pointer &event)
{
    uint32_t eventId = event->GetInnerEventId();
    APP_LOGD("BundleAgingMgr process event : %{public}u", eventId);
    switch (eventId) {
        case EVENT_AGING_NOW:
            APP_LOGD("BundleAgingMgr timer expire, run aging now.");
            Start(AgingTriggertype::PREIOD);
            SendEvent(eventId, 0, agingTimerInterval);
            APP_LOGD("BundleAginMgr reschedule time.");
            break;

        default:
            APP_LOGD("BundleAgingMgr invalid Event %{public}d.", eventId);
            break;
    }
}

void BundleAgingMgr::InitAgingHandlerChain()
{
    chain = AgingHandlerChain();
    chain.AddHandler(std::make_shared<Over30DaysUnusedBundleAgingHandler>());
    chain.AddHandler(std::make_shared<Over20DaysUnusedBundleAgingHandler>());
    chain.AddHandler(std::make_shared<Over10DaysUnusedBundleAgingHandler>());
    chain.AddHandler(std::make_shared<BundleDataSizeAgingHandler>());
    APP_LOGD("InitAgingHandleChain is finished.");
}
}  //  namespace AppExecFwk
}  //  namespace OHOS
