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

#include "appexecfwk_errors.h"
#include "bundle_info.h"
#include "bundle_mgr_service.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "inner_app_quick_fix.h"
#include "inner_bundle_info.h"
#include "installd/installd_service.h"
#include "installd_client.h"
#include "mock_quick_fix_callback.h"
#include "mock_status_receiver.h"
#include "quick_fix_data_mgr.h"
#include "quick_fix_deleter.h"
#include "quick_fix_deployer.h"
#include "quick_fix_switcher.h"
#include "quick_fix_checker.h"

using namespace testing::ext;
using namespace std::chrono_literals;
using namespace OHOS;
using namespace OHOS::AppExecFwk;
using namespace OHOS::Security;

namespace OHOS {
namespace {
const std::string BUNDLE_NAME = "com.example.bmsaccesstoken1";
const std::string BUNDLE_NAME_DEMO = "com.example.demo.bmsaccesstoken1";
const std::string HAP_FILE_PATH1 = "/data/test/resource/bms/quick_fix/bmsAccessTokentest1.hap";
const std::string HAP_FILE_PATH2 = "/data/test/resource/bms/quick_fix/bmsAccessTokentest3.hap";
const std::string HQF_FILE_PATH1 = "/data/test/resource/bms/quick_fix/bmsAccessTokentest1.hqf";
const int32_t USERID = 100;
const int32_t WAIT_TIME = 5; // init mocked bms
const std::string QUICK_FIX_ABI = "arms";
const std::string QUICK_FIX_SO_PATH = "libs/arms";
const uint32_t QUICK_FIX_VERSION_CODE = 1;
const uint32_t BUNDLE_VERSION_CODE = 1;
const std::string QUICK_FIX_VERSION_NAME = "1.0";
const std::string BUNDLE_VERSION_NAME = "1.0";
const std::string PROVISION_TYPE_DEBUG = "debug";
const std::string PROVISION_TYPE_RELEASE = "release";
const std::string RESULT_CODE = "resultCode";
const std::string PATCH_PATH = "patch_1";
const std::string MODULE_NAME = "entry";
const std::string FILE1_PATH = "/data/test/hello.hqf";
const std::string FILE2_PATH = "/data/test/world.hqf";
const std::string FILE3_PATH = "/data/test/world.hap";
const std::string INVALID_FILE_SUFFIX_PATH = "/data/test/invalidSuffix.txt";
}  // namespace

class BmsBundleQuickFixTest : public testing::Test {
public:
    BmsBundleQuickFixTest();
    ~BmsBundleQuickFixTest();
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    ErrCode InstallBundle(const std::string &bundlePath) const;
    ErrCode UpdateBundle(const std::string &bundlePath) const;
    ErrCode UnInstallBundle(const std::string &bundleName) const;
    void AddInnerBundleInfo(const std::string bundleName, const std::string provisionType = PROVISION_TYPE_RELEASE,
        QuickFixType type = QuickFixType::PATCH);
    void UninstallBundleInfo(const std::string bundleName);
    const std::shared_ptr<BundleDataMgr> GetBundleDataMgr() const;
    const std::shared_ptr<QuickFixDeployer> GetQuickFixDeployer();
    const std::shared_ptr<QuickFixDeleter> GetQuickFixDeleter();
    const std::shared_ptr<QuickFixSwitcher> GetQuickFixSwitcher();
    const std::shared_ptr<QuickFixDataMgr> GetQuickFixDataMgr() const;
    AppQuickFix CreateAppQuickFix();
    void StartInstalldService() const;
    void StartBundleService();
    sptr<IQuickFixManager> GetQuickFixManagerProxy();
    void CreateFiles(const std::vector<std::string>& sourceFiles);
    void DeleteFiles(const std::vector<std::string>& destFiles);

private:
    std::shared_ptr<InstalldService> installdService_ = std::make_shared<InstalldService>();
    std::shared_ptr<BundleMgrService> bundleMgrService_ = DelayedSingleton<BundleMgrService>::GetInstance();
    std::shared_ptr<QuickFixDeployer> deployer_ = nullptr;
    std::shared_ptr<QuickFixDeleter> deleter_ = nullptr;
    std::shared_ptr<QuickFixSwitcher> switcher_ = nullptr;
    std::shared_ptr<QuickFixDataMgr> quickFixDataMgr_ = DelayedSingleton<QuickFixDataMgr>::GetInstance();
};

BmsBundleQuickFixTest::BmsBundleQuickFixTest()
{}

BmsBundleQuickFixTest::~BmsBundleQuickFixTest()
{}

void BmsBundleQuickFixTest::SetUpTestCase()
{}

void BmsBundleQuickFixTest::TearDownTestCase()
{}

void BmsBundleQuickFixTest::SetUp()
{
    StartInstalldService();
    StartBundleService();
    auto dataMgr = GetBundleDataMgr();
    if (dataMgr != nullptr) {
        dataMgr->AddUserId(USERID);
    }
}

void BmsBundleQuickFixTest::TearDown()
{}

ErrCode BmsBundleQuickFixTest::InstallBundle(const std::string &bundlePath) const
{
    if (!bundleMgrService_) {
        return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
    }
    auto installer = bundleMgrService_->GetBundleInstaller();
    if (!installer) {
        EXPECT_FALSE(true) << "the installer is nullptr";
        return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
    }
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    if (!receiver) {
        EXPECT_FALSE(true) << "the receiver is nullptr";
        return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
    }
    InstallParam installParam;
    installParam.installFlag = InstallFlag::NORMAL;
    installParam.userId = USERID;
    bool result = installer->Install(bundlePath, installParam, receiver);
    EXPECT_TRUE(result);
    return receiver->GetResultCode();
}

ErrCode BmsBundleQuickFixTest::UpdateBundle(const std::string &bundlePath) const
{
    if (!bundleMgrService_) {
        return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
    }
    auto installer = bundleMgrService_->GetBundleInstaller();
    if (!installer) {
        EXPECT_FALSE(true) << "the installer is nullptr";
        return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
    }
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    if (!receiver) {
        EXPECT_FALSE(true) << "the receiver is nullptr";
        return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
    }
    InstallParam installParam;
    installParam.installFlag = InstallFlag::REPLACE_EXISTING;
    installParam.userId = USERID;
    bool result = installer->Install(bundlePath, installParam, receiver);
    EXPECT_TRUE(result);
    return receiver->GetResultCode();
}

ErrCode BmsBundleQuickFixTest::UnInstallBundle(const std::string &bundleName) const
{
    if (!bundleMgrService_) {
        return ERR_APPEXECFWK_UNINSTALL_BUNDLE_MGR_SERVICE_ERROR;
    }
    auto installer = bundleMgrService_->GetBundleInstaller();
    if (!installer) {
        EXPECT_FALSE(true) << "the installer is nullptr";
        return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
    }
    sptr<MockStatusReceiver> receiver = new (std::nothrow) MockStatusReceiver();
    if (!receiver) {
        EXPECT_FALSE(true) << "the receiver is nullptr";
        return ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR;
    }
    InstallParam installParam;
    installParam.installFlag = InstallFlag::NORMAL;
    installParam.userId = USERID;
    bool result = installer->Uninstall(bundleName, installParam, receiver);
    EXPECT_TRUE(result);
    return receiver->GetResultCode();
}

void BmsBundleQuickFixTest::AddInnerBundleInfo(const std::string bundleName,
    const std::string provisionType,
    QuickFixType type)
{
    BundleInfo bundleInfo;
    bundleInfo.name = bundleName;
    AppqfInfo deployedAppqfInfo;
    deployedAppqfInfo.versionCode = QUICK_FIX_VERSION_CODE;
    deployedAppqfInfo.versionName =  QUICK_FIX_VERSION_NAME;
    deployedAppqfInfo.cpuAbi = QUICK_FIX_ABI;
    deployedAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
    deployedAppqfInfo.type = type;
    bundleInfo.versionCode = BUNDLE_VERSION_CODE;
    bundleInfo.versionName = BUNDLE_VERSION_NAME;

    ApplicationInfo applicationInfo;
    applicationInfo.name = bundleName;
    if (provisionType == PROVISION_TYPE_DEBUG) {
        applicationInfo.debug = true;
        applicationInfo.appProvisionType = PROVISION_TYPE_DEBUG;
        deployedAppqfInfo.hqfInfos.push_back(HqfInfo());
    }
    applicationInfo.appQuickFix.deployedAppqfInfo = deployedAppqfInfo;
    applicationInfo.appQuickFix.deployingAppqfInfo = deployedAppqfInfo;
    applicationInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
    InnerBundleUserInfo userInfo;
    userInfo.bundleName = bundleName;
    userInfo.bundleUserInfo.userId = USERID;

    InnerBundleInfo innerBundleInfo;
    innerBundleInfo.SetBaseBundleInfo(bundleInfo);
    innerBundleInfo.SetBaseApplicationInfo(applicationInfo);
    innerBundleInfo.AddInnerBundleUserInfo(userInfo);

    auto dataMgr = GetBundleDataMgr();
    EXPECT_NE(dataMgr, nullptr);

    bool startRet = dataMgr->UpdateBundleInstallState(bundleName, InstallState::INSTALL_START);
    bool addRet = dataMgr->AddInnerBundleInfo(bundleName, innerBundleInfo);
    bool endRet = dataMgr->UpdateBundleInstallState(bundleName, InstallState::INSTALL_SUCCESS);

    EXPECT_TRUE(startRet);
    EXPECT_TRUE(addRet);
    EXPECT_TRUE(endRet);
}

void BmsBundleQuickFixTest::UninstallBundleInfo(const std::string bundleName)
{
    auto dataMgr = GetBundleDataMgr();
    EXPECT_NE(dataMgr, nullptr);
    bool startRet = dataMgr->UpdateBundleInstallState(bundleName, InstallState::UNINSTALL_START);
    bool finishRet = dataMgr->UpdateBundleInstallState(bundleName, InstallState::UNINSTALL_SUCCESS);

    EXPECT_TRUE(startRet);
    EXPECT_TRUE(finishRet);
}

void BmsBundleQuickFixTest::StartInstalldService() const
{
    if (!installdService_->IsServiceReady()) {
        installdService_->Start();
    }
}

void BmsBundleQuickFixTest::StartBundleService()
{
    if (!bundleMgrService_->IsServiceReady()) {
        bundleMgrService_->OnStart();
        std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
    }
}

const std::shared_ptr<BundleDataMgr> BmsBundleQuickFixTest::GetBundleDataMgr() const
{
    return bundleMgrService_->GetDataMgr();
}

const std::shared_ptr<QuickFixDeployer> BmsBundleQuickFixTest::GetQuickFixDeployer()
{
    if (deployer_ == nullptr) {
        std::vector<std::string> path;
        deployer_ = std::make_shared<QuickFixDeployer>(path);
    }
    return deployer_;
}

const std::shared_ptr<QuickFixDeleter> BmsBundleQuickFixTest::GetQuickFixDeleter()
{
    if (deleter_ == nullptr) {
        deleter_ = std::make_shared<QuickFixDeleter>(BUNDLE_NAME);
    }
    return deleter_;
}

const std::shared_ptr<QuickFixSwitcher> BmsBundleQuickFixTest::GetQuickFixSwitcher()
{
    if (switcher_ == nullptr) {
        switcher_ = std::make_shared<QuickFixSwitcher>(BUNDLE_NAME, true);
    }
    return switcher_;
}

const std::shared_ptr<QuickFixDataMgr> BmsBundleQuickFixTest::GetQuickFixDataMgr() const
{
    return quickFixDataMgr_;
}

AppQuickFix BmsBundleQuickFixTest::CreateAppQuickFix()
{
    AppqfInfo appInfo;
    appInfo.versionCode = QUICK_FIX_VERSION_CODE;
    appInfo.versionName = QUICK_FIX_VERSION_NAME;
    appInfo.type = QuickFixType::PATCH;
    HqfInfo hqfInfo;
    hqfInfo.moduleName = "entry";
    hqfInfo.type = QuickFixType::PATCH;
    appInfo.hqfInfos.push_back(hqfInfo);
    AppQuickFix appQuickFix;
    appQuickFix.bundleName = BUNDLE_NAME;
    appQuickFix.versionCode = BUNDLE_VERSION_CODE;
    appQuickFix.versionName = BUNDLE_VERSION_NAME;
    appQuickFix.deployingAppqfInfo = appInfo;
    return appQuickFix;
}

sptr<IQuickFixManager> BmsBundleQuickFixTest::GetQuickFixManagerProxy()
{
    auto systemAbilityManager = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        APP_LOGE("GetSystemAbilityManager failed.");
        return nullptr;
    }
    auto bundleMgrSa = systemAbilityManager->GetSystemAbility(OHOS::BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleMgrSa == nullptr) {
        APP_LOGE("GetSystemAbility failed.");
        return nullptr;
    }
    auto bundleMgr = OHOS::iface_cast<IBundleMgr>(bundleMgrSa);
    if (bundleMgr == nullptr) {
        APP_LOGE("iface_cast failed.");
        return nullptr;
    }
    return bundleMgr->GetQuickFixManagerProxy();
}

void BmsBundleQuickFixTest::CreateFiles(const std::vector<std::string>& sourceFiles)
{
    for (const auto& path : sourceFiles) {
        SaveStringToFile(path, path);
    }
}

void BmsBundleQuickFixTest::DeleteFiles(const std::vector<std::string>& destFiles)
{
    for (const auto& path : destFiles) {
        RemoveFile(path);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0001
 * Function: CheckAppQuickFixInfos
 * @tc.name: test QuickFixChecker
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check bundle name not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0001, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    appQuickFix.bundleName = "wrong_name";
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_BUNDLE_NAME_NOT_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0002
 * Function: CheckAppQuickFixInfos
 * @tc.name: test QuickFixChecker
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check bundle version code not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0002, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    appQuickFix.deployingAppqfInfo.hqfInfos[0].moduleName = "feature";
    appQuickFix.versionCode = 20000;
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_CODE_NOT_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0003
 * Function: CheckAppQuickFixInfos
 * @tc.name: test QuickFixChecker
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check bundle version name not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0003, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    appQuickFix.versionName = "2.0.0";
    appQuickFix.deployingAppqfInfo.hqfInfos[0].moduleName = "feature";
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_NAME_NOT_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0004
 * Function: CheckAppQuickFixInfos
 * @tc.name: test QuickFixChecker
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check patch version code not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0004, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    appQuickFix.deployingAppqfInfo.versionCode = 20000;
    appQuickFix.deployingAppqfInfo.hqfInfos[0].moduleName = "feature";
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PATCH_VERSION_CODE_NOT_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0005
 * Function: CheckAppQuickFixInfos
 * @tc.name: test QuickFixChecker
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check patch version code not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0005, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    appQuickFix.deployingAppqfInfo.versionName = "2.0.0";
    appQuickFix.deployingAppqfInfo.hqfInfos[0].moduleName = "feature";
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PATCH_VERSION_NAME_NOT_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0006
 * Function: CheckAppQuickFixInfos
 * @tc.name: test QuickFixChecker
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check patch version code not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0006, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
    appQuickFix.deployingAppqfInfo.hqfInfos[0].type = QuickFixType::HOT_RELOAD;
    appQuickFix.deployingAppqfInfo.hqfInfos[0].moduleName = "feature";
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PATCH_TYPE_NOT_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0007
 * Function: CheckAppQuickFixInfos
 * @tc.name: test QuickFixChecker
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check module not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0007, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_MODULE_NAME_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0008
 * Function: CheckAppQuickFixInfos
 * @tc.name: test QuickFixChecker
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check module not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0008, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    appQuickFix.deployingAppqfInfo.hqfInfos[0].moduleName = "feature";
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0009
 * Function: Query inner app quick fix
 * @tc.name: test QuickFixDataMgr
 * @tc.require: issueI5N7AD
 * @tc.desc: 1. check module not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0009, Function | SmallTest | Level0)
{
    AppQuickFix appQuickFix = CreateAppQuickFix();
    QuickFixMark mark;
    mark.bundleName = appQuickFix.bundleName;
    mark.status = QuickFixStatus::DEPLOY_START;
    InnerAppQuickFix innerAppQuickFix(appQuickFix, mark);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    ErrCode ret = ERR_OK;
    if (deployer != nullptr) {
        ret = deployer->SaveAppQuickFix(innerAppQuickFix);
        EXPECT_EQ(ret, ERR_OK);
        auto quickFixMgr = GetQuickFixDataMgr();
        EXPECT_FALSE(quickFixMgr == nullptr);
        InnerAppQuickFix tempInnerAppQuickFix;
        bool query = quickFixMgr->QueryInnerAppQuickFix(appQuickFix.bundleName, tempInnerAppQuickFix);
        EXPECT_EQ(query, true);
        EXPECT_EQ(tempInnerAppQuickFix.GetAppQuickFix().bundleName, appQuickFix.bundleName);
        EXPECT_EQ(tempInnerAppQuickFix.GetAppQuickFix().versionCode, appQuickFix.versionCode);
        query = quickFixMgr->DeleteInnerAppQuickFix(appQuickFix.bundleName);
        EXPECT_EQ(query, true);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0010
 * Function: Query DeployQuickFixResult
 * @tc.name: test ToDeployQuickFixResult
 * @tc.require: issueI5N7AD
 * @tc.desc: parse and check ToDeployQuickFixResult
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0010, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        deployer->ToDeployQuickFixResult(appQuickFix);
        DeployQuickFixResult result = deployer->GetDeployQuickFixResult();
        EXPECT_EQ(result.bundleName, appQuickFix.bundleName);
        EXPECT_EQ(result.bundleVersionCode, appQuickFix.versionCode);
        EXPECT_EQ(result.patchVersionCode, appQuickFix.deployingAppqfInfo.versionCode);
        EXPECT_EQ(result.type, appQuickFix.deployingAppqfInfo.type);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0011
 * Function: GetBundleInfo
 * @tc.name: test GetBundleInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: GetBundleInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0011, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_BUNDLE_NAME_NOT_EXIST);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0012
 * Function: GetBundleInfo
 * @tc.name: test GetBundleInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: GetBundleInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0012, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
    }

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0013
 * Function: GetBundleInfo
 * @tc.name: test GetBundleInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: GetBundleInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0013, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
    }

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0014
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckCommonWithInstalledBundle
 * @tc.require: issueI5MZ5L
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0014, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = checker.CheckCommonWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
    }

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0015
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckCommonWithInstalledBundle
 * @tc.require: issueI5MZ5L
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0015, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.versionCode = 200005;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = checker.CheckCommonWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_CODE_NOT_SAME);
    }

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0016
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckCommonWithInstalledBundle
 * @tc.require: issueI5MZ5D
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0016, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        bundleInfo.applicationInfo.appQuickFix.deployedAppqfInfo.versionCode = 2;
        bundleInfo.applicationInfo.appQuickFix.deployedAppqfInfo.hqfInfos.emplace_back(HqfInfo());
        QuickFixChecker checker;
        ret = checker.CheckCommonWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_CODE_ERROR);
    }

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0017
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckHotReloadWithInstalledBundle
 * @tc.require: issueI5MZ5D
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0017, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.hqfInfos[0].type = QuickFixType::HOT_RELOAD;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = checker.CheckHotReloadWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_HOT_RELOAD_NOT_SUPPORT_RELEASE_BUNDLE);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0018
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckHotReloadWithInstalledBundle
 * @tc.require: issueI5MZ7R
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0018, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.hqfInfos[0].type = QuickFixType::HOT_RELOAD;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = checker.CheckHotReloadWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PATCH_ALREADY_EXISTED);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0019
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckHotReloadWithInstalledBundle
 * @tc.require: issueI5MZ7R
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0019, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG, QuickFixType::HOT_RELOAD);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.hqfInfos[0].type = QuickFixType::HOT_RELOAD;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = checker.CheckHotReloadWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0020
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckPatchWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0020, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG, QuickFixType::HOT_RELOAD);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        Security::Verify::ProvisionInfo provisionInfo;
        QuickFixChecker checker;
        ret = checker.CheckPatchWithInstalledBundle(appQuickFix, bundleInfo, provisionInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_HOT_RELOAD_ALREADY_EXISTED);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0021
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckPatchWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0021, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = "libs/armeabi-v7a";
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        Security::Verify::ProvisionInfo provisionInfo;
        QuickFixChecker checker;
        ret = checker.CheckPatchWithInstalledBundle(appQuickFix, bundleInfo, provisionInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SO_INCOMPATIBLE);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0022
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckPatchWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckPatchWithInstalledBundle, signature info not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0022, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        Security::Verify::ProvisionInfo provisionInfo;
        QuickFixChecker checker;
        ret = checker.CheckPatchWithInstalledBundle(appQuickFix, bundleInfo, provisionInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SIGNATURE_INFO_NOT_SAME);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0023
 * Function: SaveInnerAppQuickFix
 * @tc.name: test update
 * @tc.require: issueI5N7AD
 * @tc.desc: update hap, app quick fix info will be deleted
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0023, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto quickFixMgr = GetQuickFixDataMgr();
    EXPECT_FALSE(quickFixMgr == nullptr);
    if (quickFixMgr != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        QuickFixMark mark;
        mark.bundleName = appQuickFix.bundleName;
        mark.status = QuickFixStatus::DEPLOY_END;
        InnerAppQuickFix innerAppQuickFix(appQuickFix, mark);
        bool ret = quickFixMgr->SaveInnerAppQuickFix(innerAppQuickFix);
        EXPECT_TRUE(ret);
        ErrCode updateResult = UpdateBundle(HAP_FILE_PATH1);
        EXPECT_EQ(updateResult, ERR_OK);
        ret = quickFixMgr->QueryInnerAppQuickFix(appQuickFix.bundleName, innerAppQuickFix);
        EXPECT_FALSE(ret);
    }

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0024
 * Function: SaveInnerAppQuickFix
 * @tc.name: test add new module
 * @tc.require: issueI5N7AD
 * @tc.desc: update hap, app quick fix info will not be deleted
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0024, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto quickFixMgr = GetQuickFixDataMgr();
    EXPECT_FALSE(quickFixMgr == nullptr);
    if (quickFixMgr != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        QuickFixMark mark;
        mark.bundleName = appQuickFix.bundleName;
        mark.status = QuickFixStatus::DEPLOY_END;
        InnerAppQuickFix innerAppQuickFix(appQuickFix, mark);
        bool ret = quickFixMgr->SaveInnerAppQuickFix(innerAppQuickFix);
        EXPECT_TRUE(ret);
        ErrCode installResult = InstallBundle(HAP_FILE_PATH2);
        EXPECT_EQ(installResult, ERR_OK);
        ret = quickFixMgr->QueryInnerAppQuickFix(appQuickFix.bundleName, innerAppQuickFix);
        EXPECT_TRUE(ret);
    }

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0025
 * Function: SaveInnerAppQuickFix
 * @tc.name: test uninstall
 * @tc.require: issueI5N7AD
 * @tc.desc: uninstall hap, app quick fix info will be deleted
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0025, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto quickFixMgr = GetQuickFixDataMgr();
    EXPECT_FALSE(quickFixMgr == nullptr);
    if (quickFixMgr != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        QuickFixMark mark;
        mark.bundleName = appQuickFix.bundleName;
        mark.status = QuickFixStatus::DEPLOY_END;
        InnerAppQuickFix innerAppQuickFix(appQuickFix, mark);
        bool ret = quickFixMgr->SaveInnerAppQuickFix(innerAppQuickFix);
        EXPECT_TRUE(ret);
    }
    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
    if (quickFixMgr != nullptr) {
        InnerAppQuickFix innerAppQuickFix;
        bool ret = quickFixMgr->QueryInnerAppQuickFix(BUNDLE_NAME, innerAppQuickFix);
        EXPECT_FALSE(ret);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0026
 * Function: GetDeployQuickFixResult
 * @tc.name: test GetDeployQuickFixResult
 * @tc.require: issueI5N7AD
 * @tc.desc: deploy hqf, GetDeployQuickFixResult.
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0026, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        deployer->ToDeployQuickFixResult(appQuickFix);
        DeployQuickFixResult result = deployer->GetDeployQuickFixResult();
        EXPECT_EQ(result.bundleName, appQuickFix.bundleName);
        EXPECT_EQ(result.bundleVersionCode, appQuickFix.versionCode);
        EXPECT_EQ(result.patchVersionCode, appQuickFix.deployingAppqfInfo.versionCode);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0027
 * Function: DeployQuickFix
 * @tc.name: test DeployQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: empty path, DeployQuickFix.
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0027, Function | SmallTest | Level0)
{
    auto quickFixProxy= GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    sptr<MockQuickFixCallback> callback = new (std::nothrow) MockQuickFixCallback();
    EXPECT_NE(callback, nullptr) << "the callback is nullptr";
    std::vector<std::string> path;
    ErrCode ret = quickFixProxy->DeployQuickFix(path, callback);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0028
 * Function: DeployQuickFix
 * @tc.name: test DeployQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: not hqf file, DeployQuickFix.
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0028, Function | SmallTest | Level0)
{
    auto quickFixProxy= GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    sptr<MockQuickFixCallback> callback = new (std::nothrow) MockQuickFixCallback();
    EXPECT_NE(callback, nullptr) << "the callback is nullptr";
    std::vector<std::string> path {HAP_FILE_PATH1};
    ErrCode ret = quickFixProxy->DeployQuickFix(path, callback);
    EXPECT_EQ(ret, ERR_OK);
    auto callbackRes = callback->GetResCode();
    EXPECT_TRUE(callbackRes != nullptr);
    if (callbackRes != nullptr) {
        auto jsonObject = nlohmann::json::parse(callbackRes->ToString());
        const int32_t resultCode = jsonObject[RESULT_CODE];
        EXPECT_EQ(resultCode, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0029
 * Function: DeployQuickFix
 * @tc.name: test DeployQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: not exist hqf file, DeployQuickFix.
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0029, Function | SmallTest | Level0)
{
    auto quickFixProxy= GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    sptr<MockQuickFixCallback> callback = new (std::nothrow) MockQuickFixCallback();
    EXPECT_NE(callback, nullptr) << "the callback is nullptr";
    std::vector<std::string> path {HQF_FILE_PATH1};
    ErrCode ret = quickFixProxy->DeployQuickFix(path, callback);
    EXPECT_EQ(ret, ERR_OK);
    auto callbackRes = callback->GetResCode();
    if (callbackRes != nullptr) {
        auto jsonObject = nlohmann::json::parse(callbackRes->ToString());
        const int32_t resultCode = jsonObject[RESULT_CODE];
        EXPECT_EQ(resultCode, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0030
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckCommonWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckCommonWithInstalledBundle, bundleInfo has no patch info
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0030, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = checker.CheckCommonWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0031
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckCommonWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckCommonWithInstalledBundle, bundleInfo patchVersionCode is
 *           equal to new patchVersionCode
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0031, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        bundleInfo.applicationInfo.appQuickFix.deployedAppqfInfo.versionCode = 1;
        bundleInfo.applicationInfo.appQuickFix.deployedAppqfInfo.hqfInfos.push_back(HqfInfo());
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = checker.CheckCommonWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_CODE_ERROR);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0032
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckCommonWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0032, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        bundleInfo.applicationInfo.appQuickFix.deployedAppqfInfo.versionCode = 0;
        bundleInfo.applicationInfo.appQuickFix.deployedAppqfInfo.hqfInfos.push_back(HqfInfo());
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = checker.CheckCommonWithInstalledBundle(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0033
 * Function: ProcessNativeLibraryPath
 * @tc.name: test ProcessNativeLibraryPath
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessNativeLibraryPath, bundleInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0033, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
        InnerAppQuickFix innerAppQuickFix;
        innerAppQuickFix.SetAppQuickFix(appQuickFix);
        deployer->ProcessNativeLibraryPath(PATCH_PATH, innerAppQuickFix);

        appQuickFix = innerAppQuickFix.GetAppQuickFix();
        deployer->ToDeployQuickFixResult(appQuickFix);
        DeployQuickFixResult result = deployer->GetDeployQuickFixResult();
        EXPECT_EQ(result.bundleName, appQuickFix.bundleName);
        EXPECT_FALSE(result.isSoContained);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0034
 * Function: ProcessNativeLibraryPath
 * @tc.name: test ProcessNativeLibraryPath
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessNativeLibraryPath, bundleInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0034, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
        appQuickFix.deployingAppqfInfo.hqfInfos[0].nativeLibraryPath = QUICK_FIX_SO_PATH;
        InnerAppQuickFix innerAppQuickFix;
        innerAppQuickFix.SetAppQuickFix(appQuickFix);
        deployer->ProcessNativeLibraryPath(PATCH_PATH, innerAppQuickFix);

        appQuickFix = innerAppQuickFix.GetAppQuickFix();
        deployer->ToDeployQuickFixResult(appQuickFix);
        DeployQuickFixResult result = deployer->GetDeployQuickFixResult();
        EXPECT_EQ(result.bundleName, appQuickFix.bundleName);
        EXPECT_FALSE(result.isSoContained);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0035
 * Function: ProcessNativeLibraryPath
 * @tc.name: test ProcessNativeLibraryPath
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessNativeLibraryPath, both empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0035, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        InnerAppQuickFix innerAppQuickFix;
        innerAppQuickFix.SetAppQuickFix(appQuickFix);
        deployer->ProcessNativeLibraryPath(PATCH_PATH, innerAppQuickFix);

        appQuickFix = innerAppQuickFix.GetAppQuickFix();
        deployer->ToDeployQuickFixResult(appQuickFix);
        DeployQuickFixResult result = deployer->GetDeployQuickFixResult();
        EXPECT_EQ(result.bundleName, appQuickFix.bundleName);
        EXPECT_FALSE(result.isSoContained);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0036
 * Function: SaveToInnerBundleInfo
 * @tc.name: test SaveToInnerBundleInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: SaveToInnerBundleInfo, bundleName is not exist in bundleInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0036, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        InnerAppQuickFix innerAppQuickFix;
        innerAppQuickFix.SetAppQuickFix(appQuickFix);
        ErrCode ret = deployer->SaveToInnerBundleInfo(innerAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_NOT_EXISTED_BUNDLE_INFO);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0037
 * Function: SaveToInnerBundleInfo
 * @tc.name: test SaveToInnerBundleInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: SaveToInnerBundleInfo, bundleName exists in bundleInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0037, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        InnerAppQuickFix innerAppQuickFix;
        innerAppQuickFix.SetAppQuickFix(appQuickFix);
        ErrCode ret = deployer->SaveToInnerBundleInfo(innerAppQuickFix);
        EXPECT_EQ(ret, ERR_OK);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0038
 * Function: ProcessBundleFilePaths
 * @tc.name: test ProcessBundleFilePaths
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessBundleFilePaths, .hap
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0038, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> paths {HAP_FILE_PATH1};
        std::vector<std::string> realPaths;
        ErrCode ret = deployer->ProcessBundleFilePaths(paths, realPaths);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0039
 * Function: ProcessBundleFilePaths
 * @tc.name: test ProcessBundleFilePaths
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessBundleFilePaths, empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0039, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> paths;
        std::vector<std::string> realPaths;
        ErrCode ret = deployer->ProcessBundleFilePaths(paths, realPaths);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0040
 * Function: ProcessBundleFilePaths
 * @tc.name: test ProcessBundleFilePaths
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessBundleFilePaths, path not exit
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0040, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> paths {HQF_FILE_PATH1};
        std::vector<std::string> realPaths;
        ErrCode ret = deployer->ProcessBundleFilePaths(paths, realPaths);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0041
 * Function: RemoveDeployingInfo
 * @tc.name: test RemoveDeployingInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: RemoveDeployingInfo, bundleName is not existed
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0041, Function | SmallTest | Level0)
{
    auto deleter = GetQuickFixDeleter();
    EXPECT_FALSE(deleter == nullptr);
    if (deleter != nullptr) {
        ErrCode ret = deleter->RemoveDeployingInfo(BUNDLE_NAME);
        EXPECT_EQ(ret, ERR_OK);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0042
 * Function: RemoveDeployingInfo
 * @tc.name: test RemoveDeployingInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: RemoveDeployingInfo, bundleName exists, hqfInfos empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0042, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);
    auto deleter = GetQuickFixDeleter();
    EXPECT_FALSE(deleter == nullptr);
    if (deleter != nullptr) {
        ErrCode ret = deleter->RemoveDeployingInfo(BUNDLE_NAME);
        EXPECT_EQ(ret, ERR_OK);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0043
 * Function: RemoveDeployingInfo
 * @tc.name: test RemoveDeployingInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: RemoveDeployingInfo, bundleName exists, hqfInfos not empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0043, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG);
    auto deleter = GetQuickFixDeleter();
    EXPECT_FALSE(deleter == nullptr);
    if (deleter != nullptr) {
        ErrCode ret = deleter->RemoveDeployingInfo(BUNDLE_NAME);
        EXPECT_EQ(ret, ERR_OK);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0044
 * Function: InnerSwitchQuickFix
 * @tc.name: test InnerSwitchQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: InnerSwitchQuickFix, bundleName not exists
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0044, Function | SmallTest | Level0)
{
    auto switcher = GetQuickFixSwitcher();
    EXPECT_FALSE(switcher == nullptr);
    if (switcher != nullptr) {
        InnerAppQuickFix innerAppQuickFix;
        ErrCode ret = switcher->InnerSwitchQuickFix(BUNDLE_NAME, innerAppQuickFix, true);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_NOT_EXISTED_BUNDLE_INFO);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0045
 * Function: InnerSwitchQuickFix
 * @tc.name: test InnerSwitchQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: InnerSwitchQuickFix, bundleName exists, hqfInfos empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0045, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME);

    auto switcher = GetQuickFixSwitcher();
    EXPECT_FALSE(switcher == nullptr);
    if (switcher != nullptr) {
        InnerAppQuickFix innerAppQuickFix;
        ErrCode ret = switcher->InnerSwitchQuickFix(BUNDLE_NAME, innerAppQuickFix, false);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_NO_PATCH_INFO_IN_BUNDLE_INFO);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0046
 * Function: ToDeployEndStatus
 * @tc.name: test ToDeployEndStatus
 * @tc.require: issueI5N7AD
 * @tc.desc: ToDeployEndStatus nativeLibraryPath empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0046, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.hqfInfos[0].type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = "";
        InnerAppQuickFix newInnerAppQuickFix;
        newInnerAppQuickFix.SetAppQuickFix(appQuickFix);
        InnerAppQuickFix oldInnerAppQuickFix;
        ErrCode ret = deployer->ToDeployEndStatus(newInnerAppQuickFix, oldInnerAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0047
 * Function: ToDeployEndStatus
 * @tc.name: test ToDeployEndStatus
 * @tc.require: issueI5N7AD
 * @tc.desc: ToDeployEndStatus nativeLibraryPath not empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0047, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.hqfInfos[0].type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
        InnerAppQuickFix newInnerAppQuickFix;
        newInnerAppQuickFix.SetAppQuickFix(appQuickFix);
        InnerAppQuickFix oldInnerAppQuickFix;
        ErrCode ret = deployer->ToDeployEndStatus(newInnerAppQuickFix, oldInnerAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0048
 * Function: ToDeployEndStatus
 * @tc.name: test ToDeployEndStatus
 * @tc.require: issueI5N7AD
 * @tc.desc: ToDeployEndStatus nativeLibraryPath not empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0048, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.hqfInfos[0].type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
        InnerAppQuickFix newInnerAppQuickFix;
        newInnerAppQuickFix.SetAppQuickFix(appQuickFix);
        InnerAppQuickFix oldInnerAppQuickFix;
        ErrCode ret = deployer->ToDeployEndStatus(newInnerAppQuickFix, oldInnerAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0049
 * Function: ProcessBundleFilePaths
 * @tc.name: test ProcessBundleFilePaths
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessBundleFilePaths
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0049, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> sourceFiles {FILE1_PATH, FILE2_PATH};
        CreateFiles(sourceFiles);
        std::vector<std::string> realFilePaths;
        ErrCode ret = deployer->ProcessBundleFilePaths(sourceFiles, realFilePaths);
        EXPECT_EQ(ret, ERR_OK);
        DeleteFiles(sourceFiles);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0050
 * Function: ProcessBundleFilePaths
 * @tc.name: test ProcessBundleFilePaths
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessBundleFilePaths
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0050, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> sourceFiles {FILE1_PATH, FILE2_PATH, INVALID_FILE_SUFFIX_PATH};
        CreateFiles(sourceFiles);
        std::vector<std::string> realFilePaths;
        ErrCode ret = deployer->ProcessBundleFilePaths(sourceFiles, realFilePaths);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
        DeleteFiles(sourceFiles);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0051
 * Function: ProcessBundleFilePaths
 * @tc.name: test ProcessBundleFilePaths
 * @tc.require: issueI5N7AD
 * @tc.desc: ProcessBundleFilePaths
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0051, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> sourceFiles {FILE1_PATH, FILE2_PATH, FILE3_PATH};
        CreateFiles(sourceFiles);
        std::vector<std::string> realFilePaths;
        ErrCode ret = deployer->ProcessBundleFilePaths(sourceFiles, realFilePaths);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
        DeleteFiles(sourceFiles);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0052
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckPatchWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckPatchWithInstalledBundle, signature info not same
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0052, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        Security::Verify::ProvisionInfo provisionInfo;
        QuickFixChecker checker;
        ret = checker.CheckPatchWithInstalledBundle(appQuickFix, bundleInfo, provisionInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SIGNATURE_INFO_NOT_SAME);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0053
 * Function: ToDeployEndStatus
 * @tc.name: test ToDeployEndStatus
 * @tc.require: issueI5N7AD
 * @tc.desc: ToDeployEndStatus
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0053, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> sourcePath {FILE1_PATH};
        CreateFiles(sourcePath);
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        std::vector<HqfInfo> hqfInfo;
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = "";
        appQuickFix.deployingAppqfInfo.hqfInfos= hqfInfo;
        InnerAppQuickFix newInnerAppQuickFix;
        newInnerAppQuickFix.SetAppQuickFix(appQuickFix);
        InnerAppQuickFix oldInnerAppQuickFix;
        ErrCode ret = deployer->ToDeployEndStatus(newInnerAppQuickFix, oldInnerAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_NOT_EXISTED_BUNDLE_INFO);
        DeleteFiles(sourcePath);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0054
 * Function: ToDeployEndStatus
 * @tc.name: test ToDeployEndStatus
 * @tc.require: issueI5N7AD
 * @tc.desc: ToDeployEndStatus
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0054, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG, QuickFixType::HOT_RELOAD);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> sourcePath {FILE1_PATH};
        CreateFiles(sourcePath);
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        std::vector<HqfInfo> hqfInfo;
        appQuickFix.deployingAppqfInfo.hqfInfos= hqfInfo;
        InnerAppQuickFix newInnerAppQuickFix;
        newInnerAppQuickFix.SetAppQuickFix(appQuickFix);
        InnerAppQuickFix oldInnerAppQuickFix;
        ErrCode ret = deployer->ToDeployEndStatus(newInnerAppQuickFix, oldInnerAppQuickFix);
        EXPECT_EQ(ret, ERR_OK);
        DeleteFiles(sourcePath);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0055
 * Function: RemoveDeployingInfo
 * @tc.name: test RemoveDeployingInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: RemoveDeployingInfo, bundleName exists, hqfInfos not empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0055, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG);
    auto deleter = GetQuickFixDeleter();
    EXPECT_FALSE(deleter == nullptr);
    if (deleter != nullptr) {
        ErrCode ret = deleter->RemoveDeployingInfo(BUNDLE_NAME);
        EXPECT_EQ(ret, ERR_OK);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0056
 * Function: GetAppProvisionType
 * @tc.name: test GetAppProvisionType
 * @tc.require: issueI5N7AD
 * @tc.desc: GetAppProvisionType, debug
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0056, Function | SmallTest | Level0)
{
    QuickFixChecker checker;
    std::string type = checker.GetAppProvisionType(Security::Verify::ProvisionType::DEBUG);
    EXPECT_EQ(type, PROVISION_TYPE_DEBUG);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0057
 * Function: GetAppProvisionType
 * @tc.name: test GetAppProvisionType
 * @tc.require: issueI5N7AD
 * @tc.desc: GetAppProvisionType release
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0057, Function | SmallTest | Level0)
{
    QuickFixChecker checker;
    std::string type = checker.GetAppProvisionType(Security::Verify::ProvisionType::RELEASE);
    EXPECT_EQ(type, PROVISION_TYPE_RELEASE);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0058
 * Function: GetAppDistributionType
 * @tc.name: test GetAppDistributionType
 * @tc.require: issueI5N7AD
 * @tc.desc: GetAppDistributionType app_gallery
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0058, Function | SmallTest | Level0)
{
    QuickFixChecker checker;
    std::string type = checker.GetAppDistributionType(Security::Verify::AppDistType::APP_GALLERY);
    EXPECT_EQ(type, "app_gallery");
}

/**
 * @tc.number: BmsBundleQuickFixTest_0058
 * Function: GetAppDistributionType
 * @tc.name: test GetAppDistributionType
 * @tc.require: issueI5N7AD
 * @tc.desc: GetAppDistributionType none
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0058, Function | SmallTest | Level0)
{
    QuickFixChecker checker;
    std::string type = checker.GetAppDistributionType(static_cast<Security::Verify::AppDistType>(7));
    EXPECT_EQ(type, "none");
}

/**
 * @tc.number: BmsBundleQuickFixTest_0059
 * Function: CheckMultiNativeSo
 * @tc.name: test CheckMultiNativeSo
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckMultiNativeSo empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0059, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    QuickFixChecker checker;
    auto ret = checker.CheckMultiNativeSo(infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0060
 * Function: CheckMultiNativeSo
 * @tc.name: test CheckMultiNativeSo
 * @tc.require: issueI5N7AD
 * @tc.desc: nativeLibraryPath empty
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0060, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    QuickFixChecker checker;
    auto ret = checker.CheckMultiNativeSo(infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0061
 * Function: CheckMultiNativeSo
 * @tc.name: test CheckMultiNativeSo
 * @tc.require: issueI5N7AD
 * @tc.desc: nativeLibraryPath
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0061, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
    infos.emplace("appQuickFix_2", appQuickFix);
    infos.emplace("appQuickFix_3", appQuickFix);

    QuickFixChecker checker;
    auto ret = checker.CheckMultiNativeSo(infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0061
 * Function: CheckMultiNativeSo
 * @tc.name: test CheckMultiNativeSo
 * @tc.require: issueI5N7AD
 * @tc.desc: nativeLibraryPath
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0061, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
    infos.emplace("appQuickFix_1", appQuickFix);
    appQuickFix.deployingAppqfInfo.nativeLibraryPath = "wrong"
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    auto ret = checker.CheckMultiNativeSo(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SO_INCOMPATIBLE);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0062
 * Function: CheckMultiNativeSo
 * @tc.name: test CheckMultiNativeSo
 * @tc.require: issueI5N7AD
 * @tc.desc: nativeLibraryPath
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0062, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
    infos.emplace("appQuickFix_1", appQuickFix);
    appQuickFix.deployingAppqfInfo.nativeLibraryPath = "wrong";
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    auto ret = checker.CheckMultiNativeSo(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SO_INCOMPATIBLE);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0063
 * Function: CheckMultiNativeSo
 * @tc.name: test CheckMultiNativeSo
 * @tc.require: issueI5N7AD
 * @tc.desc: nativeLibraryPath
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0063, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
    infos.emplace("appQuickFix_1", appQuickFix);
    appQuickFix.deployingAppqfInfo.cpuAbi = "wrong";
    infos.emplace("appQuickFix_2", appQuickFix);

    QuickFixChecker checker;
    auto ret = checker.CheckMultiNativeSo(infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SO_INCOMPATIBLE);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0064
 * Function: CheckSignatureInfo
 * @tc.name: test CheckSignatureInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckSignatureInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0064, Function | SmallTest | Level0)
{
    BundleInfo bundleInfo;
    Security::Verify::ProvisionInfo provisionInfo;
    QuickFixChecker checker;
    auto ret = checker.CheckSignatureInfo(bundleInfo, provisionInfo);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0065
 * Function: CheckSignatureInfo
 * @tc.name: test CheckSignatureInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckSignatureInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0065, Function | SmallTest | Level0)
{
    BundleInfo bundleInfo;
    bundleInfo.appId = "_";
    bundleInfo.applicationInfo.appPrivilegeLevel = "normal";
    Security::Verify::ProvisionInfo provisionInfo;
    provisionInfo.bundleInfo.apl = "system_basic";
    QuickFixChecker checker;
    auto ret = checker.CheckSignatureInfo(bundleInfo, provisionInfo);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SIGNATURE_INFO_NOT_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0066
 * Function: CheckSignatureInfo
 * @tc.name: test CheckSignatureInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckSignatureInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0066, Function | SmallTest | Level0)
{
    BundleInfo bundleInfo;
    bundleInfo.appId = "_";
    bundleInfo.applicationInfo.appPrivilegeLevel = "normal";
    Security::Verify::ProvisionInfo provisionInfo;
    provisionInfo.bundleInfo.apl = "system_basic";
    QuickFixChecker checker;
    auto ret = checker.CheckSignatureInfo(bundleInfo, provisionInfo);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SIGNATURE_INFO_NOT_SAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0067
 * Function: CheckModuleNameExist
 * @tc.name: test CheckModuleNameExist
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckModuleNameExist
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0067, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix;
    infos.emplace("appQuickFix_1", appQuickFix);

    BundleInfo bundleInfo;
    QuickFixChecker checker;
    auto ret = checker.CheckModuleNameExist(bundleInfo, infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_MODULE_NAME_NOT_EXIST);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0068
 * Function: CheckModuleNameExist
 * @tc.name: test CheckModuleNameExist
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckModuleNameExist
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0068, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    BundleInfo bundleInfo;
    QuickFixChecker checker;
    auto ret = checker.CheckModuleNameExist(bundleInfo, infos);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_MODULE_NAME_NOT_EXIST);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0069
 * Function: CheckModuleNameExist
 * @tc.name: test CheckModuleNameExist
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckModuleNameExist
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0069, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    infos.emplace("appQuickFix_1", appQuickFix);

    BundleInfo bundleInfo;
    bundleInfo.moduleNames.emplace_back("entry");
    QuickFixChecker checker;
    auto ret = checker.CheckModuleNameExist(bundleInfo, infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0070
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckCommonWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0070, Function | SmallTest | Level0)
{
    AppQuickFix appQuickFix = CreateAppQuickFix();
    BundleInfo bundleInfo;
    QuickFixChecker checker;
    auto ret = checker.CheckCommonWithInstalledBundle(appQuickFix, bundleInfo);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_BUNDLE_NAME_NOT_EXIST);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0071
 * Function: CheckCommonWithInstalledBundle
 * @tc.name: test CheckCommonWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckCommonWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0071, Function | SmallTest | Level0)
{
    AppQuickFix appQuickFix = CreateAppQuickFix();
    BundleInfo bundleInfo;
    QuickFixChecker checker;
    auto ret = checker.CheckCommonWithInstalledBundle(appQuickFix, bundleInfo);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_BUNDLE_NAME_NOT_EXIST);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0072
 * Function: CheckHotReloadWithInstalledBundle
 * @tc.name: test CheckHotReloadWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckHotReloadWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0072, Function | SmallTest | Level0)
{
    AppQuickFix appQuickFix = CreateAppQuickFix();
    BundleInfo bundleInfo;
    QuickFixChecker checker;
    auto ret = checker.CheckHotReloadWithInstalledBundle(appQuickFix, bundleInfo);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_BUNDLE_NAME_NOT_EXIST);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0073
 * Function: CheckPatchWithInstalledBundle
 * @tc.name: test CheckPatchWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckPatchWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0073, Function | SmallTest | Level0)
{
    AppQuickFix appQuickFix = CreateAppQuickFix();
    BundleInfo bundleInfo;
    Security::Verify::ProvisionInfo provisionInfo;
    QuickFixChecker checker;
    auto ret = checker.CheckPatchWithInstalledBundle(appQuickFix, bundleInfo, provisionInfo);
    EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_BUNDLE_NAME_NOT_EXIST);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0074
 * Function: CheckPatchWithInstalledBundle
 * @tc.name: test CheckPatchWithInstalledBundle
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckPatchWithInstalledBundle
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0074, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG);

    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        Security::Verify::ProvisionInfo provisionInfo;
        bundleInfo.versionName = "3.0";
        QuickFixChecker checker;
        ret = checker.CheckPatchWithInstalledBundle(appQuickFix, bundleInfo, provisionInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_NAME_NOT_SAME);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0075
 * Function: CheckAppQuickFixInfos
 * @tc.name: test CheckAppQuickFixInfos
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckAppQuickFixInfos
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0075, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0076
 * Function: CheckAppQuickFixInfos
 * @tc.name: test CheckAppQuickFixInfos
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckAppQuickFixInfos
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0076, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, AppQuickFix> infos;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    std::vector<HqfInfo> hqfInfo;
    appQuickFix.deployingAppqfInfo.hqfInfos = hqfInfo;
    infos.emplace("appQuickFix_1", appQuickFix);

    QuickFixChecker checker;
    ErrCode ret = checker.CheckAppQuickFixInfos(infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0077
 * Function: CheckMultipleHqfsSignInfo
 * @tc.name: test CheckMultipleHqfsSignInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: CheckMultipleHqfsSignInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0077, Function | SmallTest | Level0)
{
    std::vector<std::string> bundlePaths,
    std::vector<Security::Verify::HapVerifyResult> hapVerifyRes;
    QuickFixChecker checker;
    ErrCode ret = checker.CheckMultipleHqfsSignInfo(bundlePaths, hapVerifyRes);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_PARAM_ERROR);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0078
 * Function: ParseAndCheckAppQuickFixInfos
 * @tc.name: test ParseAndCheckAppQuickFixInfos
 * @tc.require: issueI5N7AD
 * @tc.desc: ParseAndCheckAppQuickFixInfos
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0078, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> bundleFilePaths;
        std::unordered_map<std::string, AppQuickFix> infos;
        ErrCode ret = deployer->ParseAndCheckAppQuickFixInfos(bundleFilePaths, infos);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PROFILE_PARSE_FAILED);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0079
 * Function: AddHqfInfo
 * @tc.name: test AddHqfInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: AddHqfInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0079, Function | SmallTest | Level0)
{
    InnerAppQuickFix innerAppQuickFix;
    AppQuickFix appQuickFix;
    auto ret = innerAppQuickFix.AddHqfInfo(appQuickFix);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0080
 * Function: AddHqfInfo
 * @tc.name: test AddHqfInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: AddHqfInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0080, Function | SmallTest | Level0)
{
    InnerAppQuickFix innerAppQuickFix;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    auto ret = innerAppQuickFix.AddHqfInfo(appQuickFix);
    EXPECT_TRUE(ret);
    ret = innerAppQuickFix.AddHqfInfo(appQuickFix);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0080
 * Function: AddHqfInfo
 * @tc.name: test AddHqfInfo
 * @tc.require: issueI5N7AD
 * @tc.desc: AddHqfInfo
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0080, Function | SmallTest | Level0)
{
    InnerAppQuickFix innerAppQuickFix;
    AppQuickFix appQuickFix = CreateAppQuickFix();
    auto ret = innerAppQuickFix.AddHqfInfo(appQuickFix);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0081
 * Function: ProcessPatchDeployStart
 * @tc.name: test ProcessPatchDeployStart
 * @tc.require: issueI5MZ5D
 * @tc.desc: ProcessPatchDeployStart
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0081, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> bundleFilePaths;
        BundleInfo bundleInfo;
        std::unordered_map<std::string, AppQuickFix> infos;
        ErrCode ret = deployer->ProcessPatchDeployStart(bundleFilePaths, bundleInfo, infos);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PROFILE_PARSE_FAILED);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0082
 * Function: ProcessPatchDeployStart
 * @tc.name: test ProcessPatchDeployStart
 * @tc.require: issueI5MZ5D
 * @tc.desc: ProcessPatchDeployStart
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0082, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> bundleFilePaths;
        BundleInfo bundleInfo;
        std::unordered_map<std::string, AppQuickFix> infos;
        AppQuickFix appQuickFix;
        infos.emplace("appQuickFix_1", appQuickFix);
        ErrCode ret = deployer->ProcessPatchDeployStart(bundleFilePaths, bundleInfo, infos);
        EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0083
 * Function: ProcessPatchDeployStart
 * @tc.name: test ProcessPatchDeployStart
 * @tc.require: issueI5MZ5D
 * @tc.desc: ProcessPatchDeployStart
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0083, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::vector<std::string> bundleFilePaths;
        BundleInfo bundleInfo;
        std::unordered_map<std::string, AppQuickFix> infos;
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
        infos.emplace("appQuickFix_1", appQuickFix);
        appQuickFix.deployingAppqfInfo.nativeLibraryPath = "wrong";
        infos.emplace("appQuickFix_1", appQuickFix);
        ErrCode ret = deployer->ProcessPatchDeployStart(bundleFilePaths, bundleInfo, infos);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_SO_INCOMPATIBLE);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0084
 * Function: ProcessPatchDeployStart
 * @tc.name: test ProcessPatchDeployStart
 * @tc.require: issueI5MZ5D
 * @tc.desc: ProcessPatchDeployStart
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0084, Function | SmallTest | Level0)
{
    AddInnerBundleInfo(BUNDLE_NAME, PROVISION_TYPE_DEBUG, QuickFixType::HOT_RELOAD);
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        appQuickFix.deployingAppqfInfo.versionCode = 2;
        appQuickFix.deployingAppqfInfo.type = QuickFixType::HOT_RELOAD;
        appQuickFix.deployingAppqfInfo.hqfInfos[0].type = QuickFixType::HOT_RELOAD;
        BundleInfo bundleInfo;
        ErrCode ret = deployer->GetBundleInfo(appQuickFix.bundleName, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
        QuickFixChecker checker;
        ret = deployer->ProcessHotReloadDeployStart(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_OK);
    }

    UninstallBundleInfo(BUNDLE_NAME);
}

/**
 * @tc.number: BmsBundleQuickFixTest_0085
 * Function: ProcessPatchDeployStart
 * @tc.name: test ProcessPatchDeployStart
 * @tc.require: issueI5MZ5D
 * @tc.desc: ProcessPatchDeployStart
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0085, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        auto ret = deployer->ProcessHotReloadDeployStart(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_BUNDLE_NAME_NOT_EXIST);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0086
 * Function: ProcessPatchDeployStart
 * @tc.name: test ProcessPatchDeployStart
 * @tc.require: issueI5MZ5D
 * @tc.desc: ProcessPatchDeployStart
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0086, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppQuickFix appQuickFix = CreateAppQuickFix();
        BundleInfo bundleInfo;
        auto ret = deployer->ProcessHotReloadDeployStart(appQuickFix, bundleInfo);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_BUNDLE_NAME_NOT_EXIST);
        deployer->ResetNativeSoAttrs();
        std::vector<HqfInfo> hqfInfo;
        appQuickFix.deployingAppqfInfo.hqfInfos = hqfInfo;
        deployer->ResetNativeSoAttrs(appQuickFix);
        appQuickFix.deployingAppqfInfo.hqfInfos.emplace_back(HqfInfo());
        deployer->ResetNativeSoAttrs(appQuickFix);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0087
 * Function: FetchPatchNativeSoAttrs
 * @tc.name: test FetchPatchNativeSoAttrs
 * @tc.require: issueI5MZ5D
 * @tc.desc: FetchPatchNativeSoAttrs
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0087, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        AppqfInfo appqfInfo;
        HqfInfo hqfInfo;
        bool isLibIsolated = false;
        std::string nativeLibraryPath;
        std::string cpuAbi;
        auto ret = deployer->FetchPatchNativeSoAttrs(appqfInfo, hqfInfo, isLibIsolated,
            nativeLibraryPath, cpuAbi);
        EXPECT_FALSE(ret);

        isLibIsolated = true;
        ret = deployer->FetchPatchNativeSoAttrs(appqfInfo, hqfInfo, isLibIsolated,
            nativeLibraryPath, cpuAbi);
        EXPECT_FALSE(ret);

        AppQuickFix appQuickFix;
        ret = deployer->HasNativeSoInBundle(appQuickFix);
        EXPECT_TRUE(ret);

        appQuickFix.deployingAppqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
        hqfInfo.nativeLibraryPath = QUICK_FIX_SO_PATH;
        appQuickFix.deployingAppqfInfo.hqfInfos.emplace_back(hqfInfo);
        ret = deployer->HasNativeSoInBundle(appQuickFix);
        EXPECT_TRUE(ret);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0088
 * Function: ToInnerAppQuickFix
 * @tc.name: test ToInnerAppQuickFix
 * @tc.require: issueI5MZ5D
 * @tc.desc: ToInnerAppQuickFix
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0088, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        std::unordered_map<std::string, AppQuickFix> infos;
        InnerAppQuickFix oldInnerAppQuickFix;
        InnerAppQuickFix newInnerAppQuickFix;
        auto ret = deployer->ToInnerAppQuickFix(infos, oldInnerAppQuickFix, newInnerAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_INTERNAL_ERROR);

        infos.emplace("aaaa", AppQuickFix());
        ret = deployer->ToInnerAppQuickFix(infos, oldInnerAppQuickFix, newInnerAppQuickFix);
        EXPECT_EQ(ret, ERR_OK);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0089
 * Function: CheckPatchVersionCode
 * @tc.name: test CheckPatchVersionCode
 * @tc.require: issueI5MZ5D
 * @tc.desc: CheckPatchVersionCode
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0089, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        const AppQuickFix newAppQuickFix;
        const AppQuickFix oldAppQuickFix;
        auto ret = deployer->CheckPatchVersionCode(newAppQuickFix, oldAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_CODE_ERROR);

        newAppQuickFix.deployingAppqfInfo.versionCode = 1;
        ret = deployer->CheckPatchVersionCode(newAppQuickFix, oldAppQuickFix);
        EXPECT_EQ(ret, ERR_OK);

        oldAppQuickFix.deployingAppqfInfo.versionCode = 2;
        ret = deployer->CheckPatchVersionCode(newAppQuickFix, oldAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_CODE_ERROR);

        oldAppQuickFix.deployedAppqfInfo.versionCode = 2;
        ret = deployer->CheckPatchVersionCode(newAppQuickFix, oldAppQuickFix);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_VERSION_CODE_ERROR);

        InnerAppQuickFix innerAppQuickFix;
        std::string targetPath;
        ret = deployer->MoveHqfFiles(innerAppQuickFix, targetPath);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleQuickFixTest_0090
 * Function: MoveHqfFiles
 * @tc.name: test MoveHqfFiles
 * @tc.require: issueI5MZ5D
 * @tc.desc: MoveHqfFiles
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleQuickFixTest_0090, Function | SmallTest | Level0)
{
    auto deployer = GetQuickFixDeployer();
    EXPECT_FALSE(deployer == nullptr);
    if (deployer != nullptr) {
        InnerAppQuickFix innerAppQuickFix;
        std::string targetPath;
        auto ret = deployer->MoveHqfFiles(innerAppQuickFix, targetPath);
        EXPECT_EQ(ret, ERR_BUNDLEMANAGER_QUICK_FIX_PARAM_ERROR);
    }
}

/**
 * @tc.number: BmsBundleSwitchQuickFix_0001
 * Function: SwitchQuickFix
 * @tc.name: test SwitchQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: SwitchQuickFix
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleSwitchQuickFix_0001, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto quickFixProxy = GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    sptr<MockQuickFixCallback> callback = new (std::nothrow) MockQuickFixCallback();
    EXPECT_NE(callback, nullptr) << "the callback is nullptr";
    ErrCode ret = quickFixProxy->SwitchQuickFix(BUNDLE_NAME, true, callback);
    EXPECT_EQ(ret, ERR_OK);

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleSwitchQuickFix_0002
 * Function: SwitchQuickFix
 * @tc.name: test SwitchQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: SwitchQuickFix
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleSwitchQuickFix_0002, Function | SmallTest | Level0)
{
    auto quickFixProxy = GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    sptr<MockQuickFixCallback> callback = new (std::nothrow) MockQuickFixCallback();
    EXPECT_NE(callback, nullptr) << "the callback is nullptr";
    ErrCode ret = quickFixProxy->SwitchQuickFix("", true, callback);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleDeleteQuickFix_0001
 * Function: DeleteQuickFix
 * @tc.name: test DeleteQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: DeleteQuickFix
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleDeleteQuickFix_0001, Function | SmallTest | Level0)
{
    ErrCode installResult = InstallBundle(HAP_FILE_PATH1);
    EXPECT_EQ(installResult, ERR_OK);

    auto quickFixProxy = GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    sptr<MockQuickFixCallback> callback = new (std::nothrow) MockQuickFixCallback();
    EXPECT_NE(callback, nullptr) << "the callback is nullptr";
    ErrCode ret = quickFixProxy->DeleteQuickFix(BUNDLE_NAME, callback);
    EXPECT_EQ(ret, ERR_OK);

    ErrCode unInstallResult = UnInstallBundle(BUNDLE_NAME);
    EXPECT_EQ(unInstallResult, ERR_OK);
}

/**
 * @tc.number: BmsBundleDeleteQuickFix_0002
 * Function: DeleteQuickFix
 * @tc.name: test DeleteQuickFix
 * @tc.require: issueI5N7AD
 * @tc.desc: DeleteQuickFix
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleDeleteQuickFix_0002, Function | SmallTest | Level0)
{
    auto quickFixProxy = GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    sptr<MockQuickFixCallback> callback = new (std::nothrow) MockQuickFixCallback();
    EXPECT_NE(callback, nullptr) << "the callback is nullptr";
    ErrCode ret = quickFixProxy->DeleteQuickFix("", callback);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: BmsBundleCopyFiles_0001
 * Function: CopyFiles
 * @tc.name: test CopyFiles
 * @tc.require: issueI5N7AD
 * @tc.desc: CopyFiles
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleCopyFiles_0001, Function | SmallTest | Level0)
{
    auto quickFixProxy = GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    const std::vector<std::string> sourceFiles {FILE1_PATH, FILE2_PATH};
    CreateFiles(sourceFiles);
    std::vector<std::string> destFiles;
    ErrCode ret = quickFixProxy->CopyFiles(sourceFiles, destFiles);
    EXPECT_EQ(ret, ERR_OK);
    DeleteFiles(sourceFiles);
    DeleteFiles(destFiles);
}

/**
 * @tc.number: BmsBundleCopyFiles_0002
 * Function: CopyFiles
 * @tc.name: test CopyFiles
 * @tc.require: issueI5N7AD
 * @tc.desc: CopyFiles
 */
HWTEST_F(BmsBundleQuickFixTest, BmsBundleCopyFiles_0002, Function | SmallTest | Level0)
{
    auto quickFixProxy = GetQuickFixManagerProxy();
    EXPECT_NE(quickFixProxy, nullptr) << "the quickFixProxy is nullptr";
    const std::vector<std::string> sourceFiles {"", ""};
    std::vector<std::string> destFiles;
    ErrCode ret = quickFixProxy->CopyFiles(sourceFiles, destFiles);
    EXPECT_NE(ret, ERR_OK);
}

} // OHOS