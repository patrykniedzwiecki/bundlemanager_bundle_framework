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

#define private public

#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "app_control_manager_host_impl.h"
#include "bundle_info.h"
#include "bundle_installer_host.h"
#include "bundle_mgr_service.h"
#include "bundle_permission_mgr.h"
#include "bundle_verify_mgr.h"
#include "inner_bundle_info.h"
#include "installd/installd_service.h"
#include "installd_client.h"
#include "mock_clean_cache.h"
#include "mock_bundle_status.h"
#include "mock_status_receiver.h"
#include "permission_define.h"

using namespace testing::ext;
using namespace std::chrono_literals;
using namespace OHOS;
using namespace OHOS::AppExecFwk;
using namespace OHOS::Security;

namespace OHOS {
namespace {
const std::string BUNDLE_NAME = "bundlName";
const std::string ABILITY_NAME = "abilityName";
const std::string MOUDLE_NAME = "moduleName";
const std::string APPID = "appId";
const std::string HAP_FILE_PATH = "/data/test/resource/bms/permission_bundle/";
const int32_t USERID = 100;
const int32_t FLAGS = 0;
const int32_t UID = 0;
const int32_t WAIT_TIME = 5; // init mocked bms
}  // namespace

class BmsBundlePermissionSyetemAppFalseTest : public testing::Test {
public:
    BmsBundlePermissionSyetemAppFalseTest();
    ~BmsBundlePermissionSyetemAppFalseTest();
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    const std::shared_ptr<BundleDataMgr> GetBundleDataMgr() const;
    void StartInstalldService() const;
    void StartBundleService();
private:
    std::shared_ptr<BundleMgrHostImpl> bundleMgrHostImpl_ = std::make_unique<BundleMgrHostImpl>();
    std::shared_ptr<BundleInstallerHost> bundleInstallerHost_ = std::make_unique<BundleInstallerHost>();
    std::shared_ptr<InstalldService> installdService_ = std::make_shared<InstalldService>();
    std::shared_ptr<BundleMgrService> bundleMgrService_ = DelayedSingleton<BundleMgrService>::GetInstance();
};

BmsBundlePermissionSyetemAppFalseTest::BmsBundlePermissionSyetemAppFalseTest()
{}

BmsBundlePermissionSyetemAppFalseTest::~BmsBundlePermissionSyetemAppFalseTest()
{}

void BmsBundlePermissionSyetemAppFalseTest::SetUpTestCase()
{}

void BmsBundlePermissionSyetemAppFalseTest::TearDownTestCase()
{}

void BmsBundlePermissionSyetemAppFalseTest::SetUp()
{
    StartInstalldService();
    StartBundleService();
}

void BmsBundlePermissionSyetemAppFalseTest::TearDown()
{}

void BmsBundlePermissionSyetemAppFalseTest::StartInstalldService() const
{
    if (!installdService_->IsServiceReady()) {
        installdService_->Start();
    }
}

void BmsBundlePermissionSyetemAppFalseTest::StartBundleService()
{
    if (!bundleMgrService_->IsServiceReady()) {
        bundleMgrService_->OnStart();
        std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
    }
}

const std::shared_ptr<BundleDataMgr> BmsBundlePermissionSyetemAppFalseTest::GetBundleDataMgr() const
{
    return bundleMgrService_->GetDataMgr();
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_0100
 * @tc.name: test GetApplicationInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetApplicationInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_0100, Function | SmallTest | Level0)
{
    ApplicationInfo appInfo;
    bool ret = bundleMgrHostImpl_->GetApplicationInfo(BUNDLE_NAME, FLAGS, USERID, appInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_0200
 * @tc.name: test GetApplicationInfoV9 of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetApplicationInfoV9 false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_0200, Function | SmallTest | Level0)
{
    ApplicationInfo appInfo;
    ErrCode ret = bundleMgrHostImpl_->GetApplicationInfoV9(BUNDLE_NAME, FLAGS, USERID, appInfo);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_0300
 * @tc.name: test GetApplicationInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetApplicationInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_0300, Function | SmallTest | Level0)
{
    std::vector<ApplicationInfo> appInfos;
    bool ret = bundleMgrHostImpl_->GetApplicationInfos(FLAGS, USERID, appInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_0400
 * @tc.name: test GetApplicationInfosV9 of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetApplicationInfosV9 false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_0400, Function | SmallTest | Level0)
{
    std::vector<ApplicationInfo> appInfos;
    ErrCode ret = bundleMgrHostImpl_->GetApplicationInfosV9(FLAGS, USERID, appInfos);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_0500
 * @tc.name: test GetBundleInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetBundleInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_0500, Function | SmallTest | Level0)
{
    BundleInfo bundleInfo;
    bool ret = bundleMgrHostImpl_->GetBundleInfo(BUNDLE_NAME, FLAGS, bundleInfo, USERID);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_0700
 * @tc.name: test GetBundleInfoV9 of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetBundleInfoV9 false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_0700, Function | SmallTest | Level0)
{
    BundleInfo bundleInfo;
    ErrCode ret = bundleMgrHostImpl_->GetBundleInfoV9(BUNDLE_NAME, FLAGS, bundleInfo, USERID);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_0800
 * @tc.name: test GetBundlePackInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetBundlePackInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_0800, Function | SmallTest | Level0)
{
    BundlePackInfo bundlePackInfo;
    ErrCode ret = bundleMgrHostImpl_->GetBundlePackInfo(BUNDLE_NAME, FLAGS, bundlePackInfo, USERID);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_0900
 * @tc.name: test GetBundleInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetBundleInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_0900, Function | SmallTest | Level0)
{
    std::vector<BundleInfo> bundleInfos;
    bool ret = bundleMgrHostImpl_->GetBundleInfos(FLAGS, bundleInfos, USERID);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1000
 * @tc.name: test GetBundleInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetBundleInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1000, Function | SmallTest | Level0)
{
    std::vector<BundleInfo> bundleInfos;
    ErrCode ret = bundleMgrHostImpl_->GetBundleInfosV9(FLAGS, bundleInfos, USERID);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1100
 * @tc.name: test GetNameForUid of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetNameForUid false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1100, Function | SmallTest | Level0)
{
    std::string name;
    ErrCode ret = bundleMgrHostImpl_->GetNameForUid(UID, name);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1200
 * @tc.name: test QueryAbilityInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryAbilityInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1200, Function | SmallTest | Level0)
{
    Want want;
    AbilityInfo abilityInfo;
    bool ret = bundleMgrHostImpl_->QueryAbilityInfo(want, FLAGS, USERID, abilityInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1300
 * @tc.name: test QueryAbilityInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryAbilityInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1300, Function | SmallTest | Level0)
{
    Want want;
    std::vector<AbilityInfo> abilityInfos;
    bool ret = bundleMgrHostImpl_->QueryAbilityInfos(want, FLAGS, USERID, abilityInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1400
 * @tc.name: test QueryAbilityInfosV9 of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryAbilityInfosV9 false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1400, Function | SmallTest | Level0)
{
    Want want;
    std::vector<AbilityInfo> abilityInfos;
    ErrCode ret = bundleMgrHostImpl_->QueryAbilityInfosV9(want, FLAGS, USERID, abilityInfos);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1500
 * @tc.name: test QueryAllAbilityInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryAllAbilityInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1500, Function | SmallTest | Level0)
{
    Want want;
    std::vector<AbilityInfo> abilityInfos;
    bool ret = bundleMgrHostImpl_->QueryAllAbilityInfos(want, USERID, abilityInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1600
 * @tc.name: test QueryAbilityInfoByUri of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryAbilityInfoByUri false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1600, Function | SmallTest | Level0)
{
    AbilityInfo abilityInfo;
    bool ret = bundleMgrHostImpl_->QueryAbilityInfoByUri(BUNDLE_NAME, abilityInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1700
 * @tc.name: test QueryAbilityInfoByUri of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryAbilityInfoByUri false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1700, Function | SmallTest | Level0)
{
    AbilityInfo abilityInfo;
    bool ret = bundleMgrHostImpl_->QueryAbilityInfoByUri(BUNDLE_NAME, USERID, abilityInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1800
 * @tc.name: test GetAbilityLabel of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetAbilityLabel false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1800, Function | SmallTest | Level0)
{
    std::string ret = bundleMgrHostImpl_->GetAbilityLabel(BUNDLE_NAME, ABILITY_NAME);
    EXPECT_EQ(ret, Constants::EMPTY_STRING);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_1900
 * @tc.name: test GetAbilityLabel of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetAbilityLabel false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_1900, Function | SmallTest | Level0)
{
    std::string label;
    ErrCode ret = bundleMgrHostImpl_->GetAbilityLabel(BUNDLE_NAME, MOUDLE_NAME, ABILITY_NAME, label);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2000
 * @tc.name: test GetBundleArchiveInfoV9 of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetBundleArchiveInfoV9 false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2000, Function | SmallTest | Level0)
{
    BundleInfo bundleInfo;
    ErrCode ret = bundleMgrHostImpl_->GetBundleArchiveInfoV9(HAP_FILE_PATH, FLAGS, bundleInfo);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2100
 * @tc.name: test GetLaunchWantForBundle of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetLaunchWantForBundle false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2100, Function | SmallTest | Level0)
{
    Want want;
    ErrCode ret = bundleMgrHostImpl_->GetLaunchWantForBundle(BUNDLE_NAME, want, USERID);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2200
 * @tc.name: test GetPermissionDef of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetPermissionDef false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2200, Function | SmallTest | Level0)
{
    PermissionDef permissionDef;
    ErrCode ret = bundleMgrHostImpl_->GetPermissionDef(BUNDLE_NAME, permissionDef);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2300
 * @tc.name: test CleanBundleCacheFiles of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. CleanBundleCacheFiles false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2300, Function | SmallTest | Level0)
{
    sptr<MockCleanCache> cleanCache = new (std::nothrow) MockCleanCache();
    ErrCode ret = bundleMgrHostImpl_->CleanBundleCacheFiles(BUNDLE_NAME, cleanCache, USERID);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2400
 * @tc.name: test IsModuleRemovable of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. IsModuleRemovable false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2400, Function | SmallTest | Level0)
{
    bool isRemovable = false;
    ErrCode ret = bundleMgrHostImpl_->IsModuleRemovable(BUNDLE_NAME, MOUDLE_NAME, isRemovable);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2500
 * @tc.name: test SetModuleUpgradeFlag of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. SetModuleUpgradeFlag false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2500, Function | SmallTest | Level0)
{
    bool upgradeFlag = false;
    ErrCode ret = bundleMgrHostImpl_->SetModuleUpgradeFlag(BUNDLE_NAME, MOUDLE_NAME, upgradeFlag);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2600
 * @tc.name: test SetApplicationEnabled of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. SetApplicationEnabled false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2600, Function | SmallTest | Level0)
{
    bool isEnable = false;
    ErrCode ret = bundleMgrHostImpl_->SetApplicationEnabled(BUNDLE_NAME, isEnable, USERID);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2700
 * @tc.name: test SetAbilityEnabled of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. SetAbilityEnabled false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2700, Function | SmallTest | Level0)
{
    bool isEnable = false;
    AbilityInfo abilityInfo;
    ErrCode ret = bundleMgrHostImpl_->SetAbilityEnabled(abilityInfo, isEnable, USERID);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2800
 * @tc.name: test GetShortcutInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetShortcutInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2800, Function | SmallTest | Level0)
{
    std::vector<ShortcutInfo> shortcutInfos;
    bool ret = bundleMgrHostImpl_->GetShortcutInfos(BUNDLE_NAME, USERID, shortcutInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_2900
 * @tc.name: test GetShortcutInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetShortcutInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_2900, Function | SmallTest | Level0)
{
    std::vector<ShortcutInfo> shortcutInfos;
    ErrCode ret = bundleMgrHostImpl_->GetShortcutInfoV9(BUNDLE_NAME, shortcutInfos);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3000
 * @tc.name: test QueryExtensionAbilityInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryExtensionAbilityInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3000, Function | SmallTest | Level0)
{
    Want want;
    std::vector<ExtensionAbilityInfo> extensionInfos;
    bool ret = bundleMgrHostImpl_->QueryExtensionAbilityInfos(want, FLAGS, USERID, extensionInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3100
 * @tc.name: test QueryExtensionAbilityInfosV9 of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryExtensionAbilityInfosV9 false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3100, Function | SmallTest | Level0)
{
    Want want;
    std::vector<ExtensionAbilityInfo> extensionInfos;
    ErrCode ret = bundleMgrHostImpl_->QueryExtensionAbilityInfosV9(want, FLAGS, USERID, extensionInfos);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3200
 * @tc.name: test QueryExtensionAbilityInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryExtensionAbilityInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3200, Function | SmallTest | Level0)
{
    Want want;
    std::vector<ExtensionAbilityInfo> extensionInfos;
    bool ret = bundleMgrHostImpl_->QueryExtensionAbilityInfos(
        want, ExtensionAbilityType::FORM, FLAGS, USERID, extensionInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3300
 * @tc.name: test QueryExtensionAbilityInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryExtensionAbilityInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3300, Function | SmallTest | Level0)
{
    Want want;
    std::vector<ExtensionAbilityInfo> extensionInfos;
    bool ret = bundleMgrHostImpl_->QueryExtensionAbilityInfos(
        want, ExtensionAbilityType::FORM, FLAGS, USERID, extensionInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3400
 * @tc.name: test QueryExtensionAbilityInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryExtensionAbilityInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3400, Function | SmallTest | Level0)
{
    std::vector<ExtensionAbilityInfo> extensionInfos;
    bool ret = bundleMgrHostImpl_->QueryExtensionAbilityInfos(ExtensionAbilityType::FORM, USERID, extensionInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3500
 * @tc.name: test QueryExtensionAbilityInfoByUri of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. QueryExtensionAbilityInfoByUri false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3500, Function | SmallTest | Level0)
{
    ExtensionAbilityInfo extensionAbilityInfo;
    bool ret = bundleMgrHostImpl_->QueryExtensionAbilityInfoByUri(HAP_FILE_PATH, USERID, extensionAbilityInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3600
 * @tc.name: test GetAppIdByBundleName of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetAppIdByBundleName false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3600, Function | SmallTest | Level0)
{
    std::string ret = bundleMgrHostImpl_->GetAppIdByBundleName(BUNDLE_NAME, USERID);
    EXPECT_EQ(ret, Constants::EMPTY_STRING);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3700
 * @tc.name: test GetAppType of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetAppType false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3700, Function | SmallTest | Level0)
{
    std::string ret = bundleMgrHostImpl_->GetAppType(BUNDLE_NAME);
    EXPECT_EQ(ret, Constants::EMPTY_STRING);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3800
 * @tc.name: test GetUidByBundleName of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetUidByBundleName false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3800, Function | SmallTest | Level0)
{
    int ret = bundleMgrHostImpl_->GetUidByBundleName(BUNDLE_NAME, USERID);
    EXPECT_EQ(ret, Constants::INVALID_UID);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_3900
 * @tc.name: test ImplicitQueryInfoByPriority of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. ImplicitQueryInfoByPriority false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_3900, Function | SmallTest | Level0)
{
    Want want;
    AbilityInfo abilityInfo;
    ExtensionAbilityInfo extensionInfo;
    bool ret = bundleMgrHostImpl_->ImplicitQueryInfoByPriority(want, FLAGS, USERID, abilityInfo, extensionInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4000
 * @tc.name: test ImplicitQueryInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. ImplicitQueryInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4000, Function | SmallTest | Level0)
{
    Want want;
    std::vector<AbilityInfo> abilityInfos;
    std::vector<ExtensionAbilityInfo> extensionInfos;
    bool ret = bundleMgrHostImpl_->ImplicitQueryInfos(want, FLAGS, USERID, false, abilityInfos, extensionInfos);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4100
 * @tc.name: test GetIconById of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetIconById false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4100, Function | SmallTest | Level0)
{
    uint32_t resId = 0;
    uint32_t density = 0;
    std::string ret = bundleMgrHostImpl_->GetIconById(BUNDLE_NAME, MOUDLE_NAME, resId, density, USERID);
    EXPECT_EQ(ret, Constants::EMPTY_STRING);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4200
 * @tc.name: test GetSandboxAbilityInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetSandboxAbilityInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4200, Function | SmallTest | Level0)
{
    int32_t appIndex = 1;
    Want want;
    AbilityInfo info;
    ErrCode ret = bundleMgrHostImpl_->GetSandboxAbilityInfo(want, appIndex, FLAGS, USERID, info);
    EXPECT_EQ(ret, ERR_APPEXECFWK_SANDBOX_QUERY_INTERNAL_ERROR);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4300
 * @tc.name: test GetSandboxExtAbilityInfos of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetSandboxExtAbilityInfos false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4300, Function | SmallTest | Level0)
{
    int32_t appIndex = 1;
    Want want;
    std::vector<ExtensionAbilityInfo> infos;
    ErrCode ret = bundleMgrHostImpl_->GetSandboxExtAbilityInfos(want, appIndex, FLAGS, USERID, infos);
    EXPECT_EQ(ret, ERR_APPEXECFWK_SANDBOX_QUERY_INTERNAL_ERROR);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4400
 * @tc.name: test GetAppProvisionInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetAppProvisionInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4400, Function | SmallTest | Level0)
{
    AppProvisionInfo appProvisionInfo;
    ErrCode ret = bundleMgrHostImpl_->GetAppProvisionInfo(BUNDLE_NAME, USERID, appProvisionInfo);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4500
 * @tc.name: test GetAllSharedBundleInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetAllSharedBundleInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4500, Function | SmallTest | Level0)
{
    std::vector<SharedBundleInfo> sharedBundles;
    ErrCode ret = bundleMgrHostImpl_->GetAllSharedBundleInfo(sharedBundles);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4600
 * @tc.name: test GetSharedBundleInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetSharedBundleInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4600, Function | SmallTest | Level0)
{
    std::vector<SharedBundleInfo> sharedBundles;
    ErrCode ret = bundleMgrHostImpl_->GetSharedBundleInfo(BUNDLE_NAME, MOUDLE_NAME, sharedBundles);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4700
 * @tc.name: test GetSandboxHapModuleInfo of BundleMgrHostImpl
 * @tc.desc: 1. system running normally
 *           2. GetSandboxHapModuleInfo false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4700, Function | SmallTest | Level0)
{
    std::unique_ptr<uint8_t[]> mediaDataPtr;
    size_t len = 0;
    ErrCode ret = bundleMgrHostImpl_->GetMediaData(BUNDLE_NAME, MOUDLE_NAME, ABILITY_NAME, mediaDataPtr, len, USERID);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4800
 * @tc.name: test Install of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. Install false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4800, Function | SmallTest | Level0)
{
    InstallParam installParam;
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    bool ret = bundleInstallerHost_->Install(HAP_FILE_PATH, installParam, receiver);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_4900
 * @tc.name: test Install of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. Install false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_4900, Function | SmallTest | Level0)
{
    InstallParam installParam;
    std::vector<std::string> bundleFilePaths;
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    bool ret = bundleInstallerHost_->Install(bundleFilePaths, installParam, receiver);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5000
 * @tc.name: test InstallByBundleName of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. InstallByBundleName false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5000, Function | SmallTest | Level0)
{
    InstallParam installParam;
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    bool ret = bundleInstallerHost_->InstallByBundleName(BUNDLE_NAME, installParam, receiver);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5100
 * @tc.name: test CreateStreamInstaller of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. CreateStreamInstaller false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5100, Function | SmallTest | Level0)
{
    InstallParam installParam;
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    sptr<IBundleStreamInstaller> ret = bundleInstallerHost_->CreateStreamInstaller(installParam, receiver);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5200
 * @tc.name: test DestoryBundleStreamInstaller of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. DestoryBundleStreamInstaller false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5200, Function | SmallTest | Level0)
{
    uint32_t streamInstallerId = 0;
    bool ret = bundleInstallerHost_->DestoryBundleStreamInstaller(streamInstallerId);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5300
 * @tc.name: test Uninstall of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. Uninstall false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5300, Function | SmallTest | Level0)
{
    UninstallParam uninstallParam;
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    bool ret = bundleInstallerHost_->Uninstall(uninstallParam, receiver);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5400
 * @tc.name: test Recover of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. Recover false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5400, Function | SmallTest | Level0)
{
    InstallParam installParam;
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    bool ret = bundleInstallerHost_->Recover(BUNDLE_NAME, installParam, receiver);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5500
 * @tc.name: test Uninstall of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. Uninstall false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5500, Function | SmallTest | Level0)
{
    InstallParam installParam;
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    bool ret = bundleInstallerHost_->Uninstall(BUNDLE_NAME, installParam, receiver);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5600
 * @tc.name: test Uninstall of BundleInstallerHost
 * @tc.desc: 1. system running normally
 *           2. Uninstall false by no permission
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5600, Function | SmallTest | Level0)
{
    InstallParam installParam;
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    bool ret = bundleInstallerHost_->Uninstall(BUNDLE_NAME, ABILITY_NAME, installParam, receiver);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5700
 * @tc.name: test QueryLauncherAbilityInfos
 * @tc.desc: 1.system run normally
 *           2.bundleInfos is empty
*/
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5700, Function | SmallTest | Level1)
{
    AAFwk::Want want;
    std::vector<AbilityInfo> abilityInfos;
    ErrCode testRet = bundleMgrHostImpl_->QueryLauncherAbilityInfos(want, USERID, abilityInfos);
    EXPECT_EQ(testRet, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5800
 * @tc.name: test AppControlManagerHostImpl
 * @tc.desc: 1.SetDisposedStatus test
 *           2.GetDisposedStatus test
 *           3.DeleteDisposedStatus test
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5800, Function | SmallTest | Level1)
{
    AppControlManagerHostImpl impl;
    Want want;
    ErrCode res = impl.SetDisposedStatus(APPID, want);
    EXPECT_EQ(res, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);

    res = impl.GetDisposedStatus(APPID, want);
    EXPECT_EQ(res, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);

    res = impl.DeleteDisposedStatus(APPID);
    EXPECT_EQ(res, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_5900
 * @tc.name: test GetSpecifiedDistributionType
 * @tc.desc: 1.system run normally
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_5900, Function | SmallTest | Level1)
{
    std::string specifiedDistributionType;
    ErrCode ret = bundleMgrHostImpl_->GetSpecifiedDistributionType("", specifiedDistributionType);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}

/**
 * @tc.number: BmsBundleSyetemAppFalseTest_6000
 * @tc.name: test GetAdditionalInfo
 * @tc.desc: 1.system run normally
 */
HWTEST_F(BmsBundlePermissionSyetemAppFalseTest, BmsBundleSyetemAppFalseTest_6000, Function | SmallTest | Level1)
{
    std::string additionalInfo;
    ErrCode ret = bundleMgrHostImpl_->GetAdditionalInfo("", additionalInfo);
    EXPECT_EQ(ret, ERR_BUNDLE_MANAGER_SYSTEM_API_DENIED);
}
} // OHOS