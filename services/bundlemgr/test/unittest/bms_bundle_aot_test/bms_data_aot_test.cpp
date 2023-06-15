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

#define private public

#include <fstream>
#include <gtest/gtest.h>

#include "ability_manager_helper.h"
#include "aot/aot_executor.h"
#include "aot/aot_handler.h"
#include "app_log_wrapper.h"
#include "appexecfwk_errors.h"
#include "base_bundle_installer.h"
#include "bundle_constants.h"
#include "bundle_data_mgr.h"
#include "bundle_mgr_service.h"
#include "json_constants.h"
#include "json_serializer.h"
#include "parcel.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
using OHOS::Parcel;

namespace OHOS {
namespace {
const std::string HAP_PATH = "/system/etc/graphic/bootpic.zip";
const std::string NOHAP_PATH = "/data/test/resource/bms/aot_bundle/....right.hap";
const std::string OUT_PUT_PATH = "/data/test/resource/bms/aot_bundle/";
const std::string ABC_RELATIVE_PATH = "ets/modules.abc";
const std::string AOT_BUNDLE_NAME = "aotBundleName";
const std::string AOT_MODULE_NAME = "aotModuleName";

}  // namespace

class BmsAOTMgrTest : public testing::Test {
public:
    BmsAOTMgrTest();
    ~BmsAOTMgrTest();
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    const std::shared_ptr<BundleDataMgr> GetBundleDataMgr() const;

private:
    std::shared_ptr<BundleDataMgr> dataMgr_ = std::make_shared<BundleDataMgr>();
};

BmsAOTMgrTest::BmsAOTMgrTest()
{}

BmsAOTMgrTest::~BmsAOTMgrTest()
{}

void BmsAOTMgrTest::SetUpTestCase()
{}

void BmsAOTMgrTest::TearDownTestCase()
{}

void BmsAOTMgrTest::SetUp()
{
    DelayedSingleton<BundleMgrService>::GetInstance()->dataMgr_ = std::make_shared<BundleDataMgr>();
}

void BmsAOTMgrTest::TearDown()
{}

const std::shared_ptr<BundleDataMgr> BmsAOTMgrTest::GetBundleDataMgr() const
{
    return dataMgr_;
}

/**
 * @tc.number: AOTExecutor_0100
 * @tc.name: test AOTExecutor
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTExecutor_0100, Function | SmallTest | Level0)
{
    AOTArgs aotArgs;
    ErrCode ret;
    AOTExecutor::GetInstance().ExecuteAOT(aotArgs, ret);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: AOTExecutor_0200
 * @tc.name: test AOTExecutor
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTExecutor_0200, Function | SmallTest | Level0)
{
    AOTArgs aotArgs;
    ErrCode ret;
    aotArgs.compileMode = Constants::COMPILE_PARTIAL;
    AOTExecutor::GetInstance().ExecuteAOT(aotArgs, ret);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: AOTExecutor_0300
 * @tc.name: test AOTExecutor
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTExecutor_0300, Function | SmallTest | Level0)
{
    AOTArgs aotArgs;
    ErrCode ret;
    aotArgs.hapPath = HAP_PATH;
    AOTExecutor::GetInstance().ExecuteAOT(aotArgs, ret);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: AOTExecutor_0400
 * @tc.name: test AOTExecutor
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTExecutor_0400, Function | SmallTest | Level0)
{
    AOTArgs aotArgs;
    ErrCode ret;
    aotArgs.outputPath = OUT_PUT_PATH;
    AOTExecutor::GetInstance().ExecuteAOT(aotArgs, ret);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: AOTExecutor_0500
 * @tc.name: test AOTExecutor
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTExecutor_0500, Function | SmallTest | Level0)
{
    AOTArgs aotArgs;
    ErrCode ret;
    aotArgs.compileMode = Constants::COMPILE_PARTIAL;
    aotArgs.hapPath = ABC_RELATIVE_PATH;
    aotArgs.outputPath = OUT_PUT_PATH;
    AOTExecutor::GetInstance().ExecuteAOT(aotArgs, ret);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: AOTExecutor_0600
 * @tc.name: test AOTExecutor
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTExecutor_0600, Function | SmallTest | Level0)
{
    AOTArgs aotArgs;
    ErrCode ret;
    aotArgs.compileMode = Constants::COMPILE_FULL;
    aotArgs.hapPath = NOHAP_PATH;
    aotArgs.outputPath = OUT_PUT_PATH;
    aotArgs.arkProfilePath = OUT_PUT_PATH;
    AOTExecutor::GetInstance().ExecuteAOT(aotArgs, ret);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: AOTExecutor_0700
 * @tc.name: test AOTExecutor
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTExecutor_0700, Function | SmallTest | Level0)
{
    AOTArgs aotArgs;
    ErrCode ret;
    aotArgs.compileMode = Constants::COMPILE_FULL;
    aotArgs.hapPath = HAP_PATH;
    aotArgs.outputPath = OUT_PUT_PATH;
    aotArgs.arkProfilePath = OUT_PUT_PATH;

    AOTExecutor::GetInstance().ExecuteAOT(aotArgs, ret);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: AOTExecutor_0800
 * @tc.name: test AOTExecutor
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTExecutor_0800, Function | SmallTest | Level0)
{
    AOTArgs aotArgs;
    ErrCode ret;
    aotArgs.compileMode = Constants::COMPILE_FULL;
    aotArgs.hapPath = HAP_PATH;
    aotArgs.outputPath = OUT_PUT_PATH;
    aotArgs.arkProfilePath = OUT_PUT_PATH;

    AOTExecutor::GetInstance().ExecuteAOT(aotArgs, ret);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: AOTHandler_0100
 * @tc.name: test AOTHandler
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTHandler_0100, Function | SmallTest | Level0)
{
    BaseBundleInstaller installer;
    bool isOTA = true;
    std::unordered_map<std::string, InnerBundleInfo> infos;
    InnerBundleInfo newInfo;
    infos.emplace("", newInfo);
    installer.ProcessAOT(isOTA, infos);
    bool res = newInfo.GetIsNewVersion();
    EXPECT_EQ(res, false);
}

/**
 * @tc.number: AOTHandler_0100
 * @tc.name: test AOTHandler
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTHandler_0200, Function | SmallTest | Level0)
{
    BaseBundleInstaller installer;
    bool isOTA = false;
    std::unordered_map<std::string, InnerBundleInfo> infos;
    InnerBundleInfo newInfo;
    installer.ProcessAOT(isOTA, infos);

    newInfo.SetIsNewVersion(isOTA);
    infos.emplace("", newInfo);
    installer.ProcessAOT(isOTA, infos);
    bool res = newInfo.GetIsNewVersion();
    EXPECT_EQ(res, isOTA);
    AOTHandler::GetInstance().HandleIdle();
}

/**
 * @tc.number: AOTHandler_0100
 * @tc.name: test AOTHandler
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTHandler_0300, Function | SmallTest | Level0)
{
    BaseBundleInstaller installer;
    bool isOTA = false;
    std::unordered_map<std::string, InnerBundleInfo> infos;
    InnerBundleInfo newInfo;
    installer.ProcessAOT(isOTA, infos);

    newInfo.SetIsNewVersion(true);
    infos.emplace("", newInfo);
    installer.ProcessAOT(isOTA, infos);
    bool res = newInfo.GetIsNewVersion();
    EXPECT_EQ(res, true);
    AOTHandler::GetInstance().HandleOTA();
}

/**
 * @tc.number: AOTHandler_0100
 * @tc.name: test AOTHandler
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTHandler_0400, Function | SmallTest | Level0)
{
    InnerBundleInfo info;
    AOTHandler::GetInstance().HandleInstallWithSingleHap(info, "");
    std::string res = info.GetBundleName();
    EXPECT_EQ(res, Constants::EMPTY_STRING);
}

/**
 * @tc.number: AOTHandler_0100
 * @tc.name: test AOTHandler
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTHandler_0500, Function | SmallTest | Level0)
{
    InnerBundleInfo info;
    AOTHandler::GetInstance().HandleInstallWithSingleHap(info, Constants::COMPILE_PARTIAL);
    std::string res = info.GetBundleName();
    EXPECT_EQ(res, Constants::EMPTY_STRING);
}

/**
 * @tc.number: AOTHandler_0100
 * @tc.name: test AOTHandler
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTHandler_0600, Function | SmallTest | Level0)
{
    std::optional<AOTArgs> aotArgs;
    uint32_t versionCode = 1;
    AOTArgs aotArg;
    aotArg.bundleName = AOT_BUNDLE_NAME;
    aotArg.moduleName = AOT_MODULE_NAME;
    aotArgs = aotArg;
    AOTHandler::GetInstance().AOTInternal(aotArgs, versionCode);
    auto item = GetBundleDataMgr()->bundleInfos_.find(AOT_BUNDLE_NAME);
    EXPECT_EQ(item, GetBundleDataMgr()->bundleInfos_.end());
}

/**
 * @tc.number: AOTHandler_0100
 * @tc.name: test AOTHandler
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTHandler_0700, Function | SmallTest | Level0)
{
    std::optional<AOTArgs> aotArgs;
    uint32_t versionCode = 1;
    AOTArgs aotArg;
    aotArg.bundleName = AOT_BUNDLE_NAME;
    aotArg.moduleName = AOT_MODULE_NAME;
    aotArgs = aotArg;

    InnerBundleInfo innerBundleInfo;
    ApplicationInfo applicationInfo;
    applicationInfo.bundleName = AOT_BUNDLE_NAME;
    BundleInfo bundleInfo;
    bundleInfo.versionCode = versionCode;
    innerBundleInfo.SetBaseBundleInfo(bundleInfo);
    innerBundleInfo.SetBaseApplicationInfo(applicationInfo);

    DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr()->bundleInfos_.emplace(
        AOT_BUNDLE_NAME, innerBundleInfo);
    AOTHandler::GetInstance().AOTInternal(aotArgs, versionCode);
    auto item = DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr()->bundleInfos_.find(AOT_BUNDLE_NAME);
    EXPECT_NE(item, DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr()->bundleInfos_.end());

    versionCode = 2;
    AOTHandler::GetInstance().AOTInternal(aotArgs, versionCode);
    EXPECT_NE(item->second.GetVersionCode(), versionCode);
    DelayedSingleton<BundleMgrService>::GetInstance()->GetDataMgr()->bundleInfos_.clear();
}

/**
 * @tc.number: AOTHandler_0100
 * @tc.name: test AOTHandler
 * @tc.desc: 1. system running normally
 *           2. verify function return value
 */
HWTEST_F(BmsAOTMgrTest, AOTHandler_0800, Function | SmallTest | Level0)
{
    InnerBundleInfo info;
    InnerModuleInfo moduleInfo;
    moduleInfo.moduleName = AOT_MODULE_NAME;
    info.innerModuleInfos_.try_emplace(AOT_MODULE_NAME, moduleInfo);
    AOTHandler::GetInstance().HandleIdleWithSingleHap(info, AOT_MODULE_NAME, "");

    info.SetAOTCompileStatus(AOT_MODULE_NAME, AOTCompileStatus::COMPILE_SUCCESS);
    AOTHandler::GetInstance().HandleIdleWithSingleHap(info, AOT_MODULE_NAME, "");
    AOTCompileStatus ret = info.GetAOTCompileStatus(AOT_MODULE_NAME);
    EXPECT_EQ(ret, AOTCompileStatus::COMPILE_SUCCESS);
}

/**
 * @tc.number: AOTArgs_0001
 * Function: test AOTArgs
 * @tc.desc: 1. system running normally
 */
HWTEST_F(BmsAOTMgrTest, AOTArgs_0001, Function | SmallTest | Level0)
{
    AOTArgs info;
    info.bundleName = AOT_BUNDLE_NAME;
    Parcel parcel;
    auto result = info.Unmarshalling(parcel);
    EXPECT_NE(result->bundleName, AOT_BUNDLE_NAME);
    info.Marshalling(parcel);
    result = info.Unmarshalling(parcel);
    EXPECT_EQ(result->bundleName, AOT_BUNDLE_NAME);
}
} // OHOS