/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include <cstdio>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_provision_info.h"
#include "base_bundle_installer.h"
#include "bundle_install_checker.h"
#include "bundle_verify_mgr.h"
#include "bundle_util.h"
#include "directory_ex.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace {
const std::string HAP = "hap";
const std::string HAP_ONE = "hap1";
const std::string HAP_TWO = "hap2";
const std::string HAP_THREE = "hap3";
const std::string HAP_FOUR = "hap4";
const std::string HAP_FIVE = "hap5";
const std::string ARM = "arm";
const std::string ARM_SO_PATH = "/lib/arm/arm.so";
const std::string ARM_AN_PATH = "/an/arm/arm.so";
const std::string X86 = "x86";
const std::string X86_SO_PATH = "/lib/x86/x86.so";
const std::string X86_AN_PATH = "/an/x86/x86.so";
const std::string BUNDLE_NAME = "com.example.test";
const std::string MODULE_PACKAGE = "com.example.test";
const std::string MODULE_PATH = "test_tmp";
}  // namespace

class BmsBundleInstallCheckerTest : public testing::Test {
public:
    BmsBundleInstallCheckerTest();
    ~BmsBundleInstallCheckerTest();
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

BmsBundleInstallCheckerTest::BmsBundleInstallCheckerTest()
{}

BmsBundleInstallCheckerTest::~BmsBundleInstallCheckerTest()
{}

void BmsBundleInstallCheckerTest::SetUpTestCase()
{
}

void BmsBundleInstallCheckerTest::TearDownTestCase()
{}

void BmsBundleInstallCheckerTest::SetUp()
{}

void BmsBundleInstallCheckerTest::TearDown()
{}

/**
 * @tc.number: CheckMultiNativeFile_0100
 * @tc.name: test the start function of BundleInstallChecker
 * @tc.desc: 1. BundleInstallChecker
 * @tc.require: issueI5VW01
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckMultiNativeFile_0100, Function | SmallTest | Level0)
{
    BundleInstallChecker bundleInstallChecker;
    std::unordered_map<std::string, InnerBundleInfo> infos;
    InnerBundleInfo innerBundleInfo;
    infos.emplace(HAP, innerBundleInfo);

    InnerBundleInfo innerBundleInfo1;
    innerBundleInfo1.SetArkNativeFileAbi(ARM);
    innerBundleInfo1.SetArkNativeFilePath(ARM_AN_PATH);
    innerBundleInfo1.SetCpuAbi(ARM);
    innerBundleInfo1.SetNativeLibraryPath(ARM_SO_PATH);
    infos.emplace(HAP_ONE, innerBundleInfo1);

    InnerBundleInfo innerBundleInfo2;
    innerBundleInfo2.SetArkNativeFileAbi(ARM);
    innerBundleInfo2.SetArkNativeFilePath(ARM_AN_PATH);
    innerBundleInfo2.SetCpuAbi(ARM);
    innerBundleInfo2.SetNativeLibraryPath(ARM_SO_PATH);
    infos.emplace(HAP_TWO, innerBundleInfo2);
    ErrCode ret = bundleInstallChecker.CheckMultiNativeFile(infos);
    EXPECT_EQ(ret, ERR_OK);

    InnerBundleInfo innerBundleInfo3;
    infos.emplace(HAP_THREE, innerBundleInfo3);
    ret = bundleInstallChecker.CheckMultiNativeFile(infos);
    EXPECT_EQ(ret, ERR_OK);

    InnerBundleInfo innerBundleInfo4;
    innerBundleInfo4.SetArkNativeFileAbi(X86);
    innerBundleInfo4.SetArkNativeFilePath(X86_AN_PATH);
    infos.emplace(HAP_FOUR, innerBundleInfo4);
    ret = bundleInstallChecker.CheckMultiNativeFile(infos);
    EXPECT_NE(ret, ERR_OK);

    InnerBundleInfo innerBundleInfo5;
    innerBundleInfo5.SetCpuAbi(X86);
    innerBundleInfo5.SetNativeLibraryPath(X86_SO_PATH);
    infos.emplace(HAP_FIVE, innerBundleInfo5);
    ret = bundleInstallChecker.CheckMultiNativeFile(infos);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: CheckMultiNativeFile_0200
 * @tc.name: Test CheckMultiNativeFile
 * @tc.desc: 1.Test the CheckMultiNativeFile of InnerBundleInfo
 */
HWTEST_F(BmsBundleInstallCheckerTest, CheckMultiNativeFile_0200, Function | SmallTest | Level0)
{
    InnerBundleInfo innerBundleInfo1;
    innerBundleInfo1.SetCpuAbi(ARM);
    innerBundleInfo1.SetNativeLibraryPath(ARM_SO_PATH);
    std::unordered_map<std::string, InnerBundleInfo> infos;
    infos.emplace(HAP_ONE, innerBundleInfo1);
    BundleInstallChecker installChecker;
    ErrCode ret = installChecker.CheckMultiNativeFile(infos);
    EXPECT_EQ(ret, ERR_OK);
    InnerBundleInfo innerBundleInfo2;
    innerBundleInfo2.SetCpuAbi(X86);
    innerBundleInfo2.SetNativeLibraryPath(X86_SO_PATH);
    infos.emplace(HAP_TWO, innerBundleInfo2);
    ret = installChecker.CheckMultiNativeFile(infos);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: CheckMultiNativeFile_0300
 * @tc.name: Test CheckMultiNativeFile
 * @tc.desc: 1.Test the CheckMultiNativeFile of InnerBundleInfo
 */
HWTEST_F(BmsBundleInstallCheckerTest, CheckMultiNativeFile_0300, Function | SmallTest | Level0)
{
    InnerBundleInfo innerBundleInfo1;
    innerBundleInfo1.SetArkNativeFileAbi(ARM);
    innerBundleInfo1.SetArkNativeFilePath(ARM_AN_PATH);
    std::unordered_map<std::string, InnerBundleInfo> infos;
    infos.emplace(HAP_ONE, innerBundleInfo1);

    InnerBundleInfo innerBundleInfo2;
    innerBundleInfo2.SetArkNativeFileAbi(X86);
    innerBundleInfo2.SetArkNativeFilePath(X86_AN_PATH);
    infos.emplace(HAP_TWO, innerBundleInfo2);
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckMultiNativeFile(infos);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: CheckMultiArkNativeFile_0100
 * @tc.name: Test CheckMultiArkNativeFile
 * @tc.desc: 1.Test the CheckMultiArkNativeFile of InnerBundleInfo
 */
HWTEST_F(BmsBundleInstallCheckerTest, CheckMultiArkNativeFile_0100, Function | SmallTest | Level0)
{
    InnerBundleInfo innerBundleInfo1;
    std::unordered_map<std::string, InnerBundleInfo> infos;
    infos.emplace(HAP_ONE, innerBundleInfo1);
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckMultiArkNativeFile(infos);
    EXPECT_EQ(ret, ERR_OK);
    InnerBundleInfo innerBundleInfo2;
    innerBundleInfo2.SetArkNativeFileAbi(ARM);
    infos.emplace(HAP_TWO, innerBundleInfo1);
    ret = installChecker.CheckMultiArkNativeFile(infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: CheckMultiArkNativeFile_0200
 * @tc.name: Test CheckMultiArkNativeFile
 * @tc.desc: 1.Test the CheckMultiArkNativeFile of InnerBundleInfo
 */
HWTEST_F(BmsBundleInstallCheckerTest, CheckMultiArkNativeFile_0200, Function | SmallTest | Level0)
{
    InnerBundleInfo innerBundleInfo1;
    innerBundleInfo1.SetArkNativeFileAbi(ARM);
    innerBundleInfo1.SetArkNativeFilePath(ARM_AN_PATH);
    innerBundleInfo1.SetCpuAbi(ARM);
    innerBundleInfo1.SetNativeLibraryPath(ARM_SO_PATH);
    std::unordered_map<std::string, InnerBundleInfo> infos;
    infos.emplace(HAP_ONE, innerBundleInfo1);

    InnerBundleInfo innerBundleInfo2;
    innerBundleInfo2.SetArkNativeFileAbi(X86);
    innerBundleInfo2.SetArkNativeFilePath(X86_AN_PATH);
    infos.emplace(HAP_TWO, innerBundleInfo2);
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckMultiArkNativeFile(infos);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: ExtractModule_0100
 * @tc.name: test the start function of ExtractModule
 * @tc.desc: 1. BaseBundleInstaller
 * @tc.require: issueI5VW01
*/
HWTEST_F(BmsBundleInstallCheckerTest, ExtractModule_0100, Function | SmallTest | Level0)
{
    InnerBundleInfo innerBundleInfo;
    innerBundleInfo.currentPackage_ = MODULE_PACKAGE;
    InnerModuleInfo innerModuleInfo;
    innerModuleInfo.isLibIsolated = true;
    innerModuleInfo.cpuAbi = X86;
    innerModuleInfo.nativeLibraryPath = X86;
    innerModuleInfo.moduleName = MODULE_PACKAGE;
    innerModuleInfo.modulePackage = MODULE_PACKAGE;
    innerModuleInfo.name = MODULE_PACKAGE;
    innerBundleInfo.InsertInnerModuleInfo(MODULE_PACKAGE, innerModuleInfo);
    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.modulePackage_ = MODULE_PACKAGE;
    ErrCode ret = baseBundleInstaller.ExtractModule(innerBundleInfo, MODULE_PATH);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: CheckSysCape_0100
 * @tc.name: test the start function of CheckSysCap
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckSysCape_0100, Function | SmallTest | Level0)
{
    std::string bundlePath = "/data/test/test.hap";
    std::vector<std::string> bundlePaths;
    bundlePaths.push_back(bundlePath);
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckSysCap(bundlePaths);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.number: CheckSystemSize_0100
 * @tc.name: test the start function of CheckSystemSize
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckSystemSize_0100, Function | SmallTest | Level0)
{
    std::string bundlePath = "/data/app/el1/bundle";
    Constants::AppType appType = Constants::AppType::SYSTEM_APP;
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckSystemSize(bundlePath, appType);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: CheckSystemSize_0200
 * @tc.name: test the start function of CheckSystemSize
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckSystemSize_0200, Function | SmallTest | Level0)
{
    std::string bundlePath = "/data/app/el1/bundle";
    Constants::AppType appType = Constants::AppType::THIRD_SYSTEM_APP;
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckSystemSize(bundlePath, appType);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: CheckSystemSize_0300
 * @tc.name: test the start function of CheckSystemSize
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckSystemSize_0300, Function | SmallTest | Level0)
{
    std::string bundlePath = "/data/app/el1/bundle";
    Constants::AppType appType = Constants::AppType::THIRD_PARTY_APP;
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckSystemSize(bundlePath, appType);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: CheckSystemSize_0400
 * @tc.name: test the start function of CheckSystemSize
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckSystemSize_0400, Function | SmallTest | Level0)
{
    std::string bundlePath = "";
    Constants::AppType appType = Constants::AppType::THIRD_PARTY_APP;
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckSystemSize(bundlePath, appType);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_DISK_MEM_INSUFFICIENT);
}

/**
 * @tc.number: CheckSystemSize_0500
 * @tc.name: test the start function of CheckSystemSize
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckSystemSize_0500, Function | SmallTest | Level0)
{
    std::string bundlePath = "/data/app/el1/bundle/public/patch_1000001";
    bool res = BundleUtil::CreateDir(bundlePath);
    EXPECT_TRUE(res);

    Constants::AppType appType = Constants::AppType::SYSTEM_APP;
    Constants::AppType appType1 = Constants::AppType::THIRD_SYSTEM_APP;
    Constants::AppType appType2 = Constants::AppType::THIRD_PARTY_APP;
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckSystemSize(bundlePath, appType);
    EXPECT_EQ(ret, ERR_OK);
    ret = installChecker.CheckSystemSize(bundlePath, appType1);
    EXPECT_EQ(ret, ERR_OK);
    ret = installChecker.CheckSystemSize(bundlePath, appType2);
    EXPECT_EQ(ret, ERR_OK);

    res = BundleUtil::DeleteDir(bundlePath);
    EXPECT_TRUE(res);
}

/**
 * @tc.number: CheckSystemSize_0600
 * @tc.name: test the start function of CheckSystemSize
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckSystemSize_0600, Function | SmallTest | Level0)
{
    std::string bundlePath = "/data/app/el1/bundle/100";
    Constants::AppType appType = Constants::AppType::SYSTEM_APP;
    Constants::AppType appType1 = Constants::AppType::THIRD_SYSTEM_APP;
    Constants::AppType appType2 = Constants::AppType::THIRD_PARTY_APP;
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckSystemSize(bundlePath, appType);
    EXPECT_EQ(ret, ERR_OK);
    ret = installChecker.CheckSystemSize(bundlePath, appType1);
    EXPECT_EQ(ret, ERR_OK);
    ret = installChecker.CheckSystemSize(bundlePath, appType2);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: CheckSystemSize_0700
 * @tc.name: test the start function of CheckSystemSize
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckSystemSize_0700, Function | SmallTest | Level0)
{
    std::string bundlePath = "/data/app/el1/bundle/0";
    Constants::AppType appType = Constants::AppType::SYSTEM_APP;
    Constants::AppType appType1 = Constants::AppType::THIRD_SYSTEM_APP;
    Constants::AppType appType2 = Constants::AppType::THIRD_PARTY_APP;
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckSystemSize(bundlePath, appType);
    EXPECT_EQ(ret, ERR_OK);
    ret = installChecker.CheckSystemSize(bundlePath, appType1);
    EXPECT_EQ(ret, ERR_OK);
    ret = installChecker.CheckSystemSize(bundlePath, appType2);
    EXPECT_EQ(ret, ERR_OK);
}
/**
 * @tc.number: CheckHapHashParams_0100
 * @tc.name: test the start function of CheckHapHashParams
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckHapHashParams_0100, Function | SmallTest | Level0)
{
    std::unordered_map<std::string, InnerBundleInfo> infos;
    std::map<std::string, std::string> hashParams;
    hashParams.insert(pair<string, string>("1", "2"));
    BundleInstallChecker installChecker;
    auto ret = installChecker.CheckHapHashParams(infos, hashParams);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_FAILED_CHECK_HAP_HASH_PARAM);
}

/**
 * @tc.number: IsExistedDistroModule_0100
 * @tc.name: test the start function of IsExistedDistroModule
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, IsExistedDistroModule_0100, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    InnerBundleInfo newInfo;
    InnerBundleInfo info;
    InnerModuleInfo innerModuleInfo;
    innerModuleInfo.moduleName = ".entry";
    newInfo.currentPackage_ = "";
    newInfo.innerModuleInfos_.insert(pair<string, InnerModuleInfo>("", innerModuleInfo));
    auto ret = installChecker.IsExistedDistroModule(newInfo, info);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number: IsExistedDistroModule_0200
 * @tc.name: test the start function of IsExistedDistroModule
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, IsExistedDistroModule_0200, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    InnerBundleInfo newInfo;
    InnerBundleInfo info;
    InnerModuleInfo innerModuleInfo;
    innerModuleInfo.moduleName = "";
    newInfo.currentPackage_ = "1";
    newInfo.innerModuleInfos_.insert(pair<string, InnerModuleInfo>("1", innerModuleInfo));
    auto ret = installChecker.IsExistedDistroModule(newInfo, info);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number: IsExistedDistroModule_0300
 * @tc.name: test the start function of IsExistedDistroModule
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, IsExistedDistroModule_0300, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    InnerBundleInfo newInfo;
    InnerBundleInfo info;
    InnerModuleInfo innerModuleInfo;
    innerModuleInfo.moduleName = "";
    newInfo.currentPackage_ = "1";
    newInfo.innerModuleInfos_.insert(pair<string, InnerModuleInfo>("", innerModuleInfo));
    auto ret = installChecker.IsExistedDistroModule(newInfo, info);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number: CheckMainElement_0100
 * @tc.name: test the start function of CheckMainElement
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckMainElement_0100, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    InnerBundleInfo info;
    info.innerModuleInfos_.clear();
    auto ret = installChecker.CheckMainElement(info);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: GetPrivilegeCapabilityValue_0100
 * @tc.name: test the start function of GetPrivilegeCapabilityValue
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetPrivilegeCapabilityValue_0100, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    InnerBundleInfo info;
    std::vector<std::string> existInJson;
    existInJson.push_back("1");
    existInJson.push_back("2");
    std::string key = "1";
    bool existInPreJson = true;
    bool existInProvision = false;
    auto ret = installChecker.GetPrivilegeCapabilityValue(existInJson, key, existInPreJson, existInProvision);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number: GetPrivilegeCapabilityValue_0200
 * @tc.name: test the start function of GetPrivilegeCapabilityValue
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetPrivilegeCapabilityValue_0200, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    InnerBundleInfo info;
    std::vector<std::string> existInJson;
    existInJson.push_back("1");
    existInJson.push_back("2");
    std::string key = "0";
    bool existInPreJson = true;
    bool existInProvision = false;
    auto ret = installChecker.GetPrivilegeCapabilityValue(existInJson, key, existInPreJson, existInProvision);
    EXPECT_FALSE(ret);
}

/**
 * @tc.number: CheckDeviceType_0100
 * @tc.name: test the start function of CheckDeviceType
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckDeviceType_0100, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    std::unordered_map<std::string, InnerBundleInfo> infos;
    InnerBundleInfo innerBundleInfo1;
    InnerBundleInfo innerBundleInfo2;
    infos.insert(pair<string, InnerBundleInfo>("1", innerBundleInfo1));
    infos.insert(pair<string, InnerBundleInfo>("2", innerBundleInfo2));
    auto ret = installChecker.CheckDeviceType(infos);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: CheckDeviceType_0200
 * @tc.name: test the start function of CollectProvisionInfo
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckDeviceType_0200, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    Security::Verify::ProvisionInfo provisionInfo;
    AppPrivilegeCapability appPrivilegeCapability;
    InnerBundleInfo newInfo;
    provisionInfo.type = Security::Verify::ProvisionType::DEBUG;
    provisionInfo.distributionType = Security::Verify::AppDistType::NONE_TYPE;
    installChecker.CollectProvisionInfo(provisionInfo, appPrivilegeCapability, newInfo);
    EXPECT_EQ(newInfo.GetAppProvisionType(), Constants::APP_PROVISION_TYPE_DEBUG);
}

/**
 * @tc.number: CheckDeviceType_0300
 * @tc.name: test the start function of CheckSysCap
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckDeviceType_0300, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    BundlePackInfo bundlePackInfo;
    InnerBundleInfo innerBundleInfo;
    installChecker.SetEntryInstallationFree(bundlePackInfo, innerBundleInfo);
    std::vector<std::string> bundlePaths;
    ErrCode res = installChecker.CheckSysCap(bundlePaths);
    EXPECT_EQ(res, ERR_APPEXECFWK_INSTALL_PARAM_ERROR);
}

/**
 * @tc.number: CheckDeviceType_0400
 * @tc.name: test the start function of IsContainModuleName
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckDeviceType_0400, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    InnerBundleInfo newInfo;
    InnerBundleInfo info;
    bool res = installChecker.IsContainModuleName(newInfo, info);
    EXPECT_EQ(res, false);
}

/**
 * @tc.number: CheckDeviceType_0500
 * @tc.name: test the start function of CheckModuleNameForMulitHaps
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckDeviceType_0500, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    std::unordered_map<std::string, InnerBundleInfo> infos;
    InnerBundleInfo newInfo;
    std::map<std::string, InnerModuleInfo> innerModuleInfos;
    InnerModuleInfo innerModuleInfo;
    innerModuleInfo.moduleName = "";
    newInfo.AddInnerModuleInfo(innerModuleInfos);
    infos.emplace("/data/app/el1/bundle", newInfo);
    ErrCode res = installChecker.CheckModuleNameForMulitHaps(infos);
    EXPECT_EQ(res, ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR);
}

/**
 * @tc.number: CheckDeviceType_0600
 * @tc.name: test the start function of MatchSignature
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckDeviceType_0600, Function | SmallTest | Level0)
{
    BundleInstallChecker installChecker;
    std::vector<std::string> appSignatures;
    bool res = installChecker.MatchSignature(appSignatures, "");
    EXPECT_EQ(res, false);
    appSignatures.push_back("/data/app/el1/bundle");
    res = installChecker.MatchSignature(appSignatures, "");
    EXPECT_EQ(res, false);
}

/**
 * @tc.number: CheckAppLabel_0001
 * @tc.name: test the start function of CheckAppLabel
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckAppLabel_0001, Function | SmallTest | Level0)
{
    InnerBundleInfo oldInfo;
    InnerBundleInfo newInfo;
    BaseBundleInstaller baseBundleInstaller;
    auto ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_OK);
    oldInfo.SetAppFeature("hos_normal_app");
    newInfo.SetAppFeature("hos_system_app");
    ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_APPTYPE_NOT_SAME);
}

/**
 * @tc.number: CheckAppLabel_0002
 * @tc.name: test the start function of CheckAppLabel
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckAppLabel_0002, Function | SmallTest | Level0)
{
    InnerBundleInfo oldInfo;
    InnerBundleInfo newInfo;
    oldInfo.baseBundleInfo_->releaseType = "release_type";
    newInfo.baseBundleInfo_->releaseType = "normal_type";
    BaseBundleInstaller baseBundleInstaller;
    auto ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_RELEASETYPE_NOT_SAME);
}

/**
 * @tc.number: CheckAppLabel_0003
 * @tc.name: test the start function of CheckAppLabel
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckAppLabel_0003, Function | SmallTest | Level0)
{
    InnerBundleInfo oldInfo;
    InnerBundleInfo newInfo;
    oldInfo.SetAppDistributionType("hos_normal_type");
    newInfo.SetAppDistributionType("hos_system_type");
    BaseBundleInstaller baseBundleInstaller;
    auto ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_APP_DISTRIBUTION_TYPE_NOT_SAME);
}

/**
 * @tc.number: CheckAppLabel_0004
 * @tc.name: test the start function of CheckAppLabel
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckAppLabel_0004, Function | SmallTest | Level0)
{
    InnerBundleInfo oldInfo;
    InnerBundleInfo newInfo;
    oldInfo.SetAppProvisionType("hos_normal_type");
    newInfo.SetAppProvisionType("hos_system_type");
    BaseBundleInstaller baseBundleInstaller;
    auto ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_APP_PROVISION_TYPE_NOT_SAME);
}

/**
 * @tc.number: CheckAppLabel_0005
 * @tc.name: test the start function of CheckAppLabel
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckAppLabel_0005, Function | SmallTest | Level0)
{
    InnerBundleInfo oldInfo;
    InnerBundleInfo newInfo;
    oldInfo.SetIsNewVersion(false);
    newInfo.SetIsNewVersion(true);
    BaseBundleInstaller baseBundleInstaller;
    auto ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_STATE_ERROR);
}

/**
 * @tc.number: CheckAppLabel_0006
 * @tc.name: test the start function of CheckAppLabel
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckAppLabel_0006, Function | SmallTest | Level0)
{
    InnerBundleInfo oldInfo;
    InnerBundleInfo newInfo;
    oldInfo.SetAsanEnabled(false);
    newInfo.SetAsanEnabled(true);
    BaseBundleInstaller baseBundleInstaller;
    auto ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_ASAN_ENABLED_NOT_SAME);
}

/**
 * @tc.number: CheckAppLabel_0007
 * @tc.name: test the start function of CheckAppLabel
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckAppLabel_0007, Function | SmallTest | Level0)
{
    InnerBundleInfo oldInfo;
    InnerBundleInfo newInfo;
    oldInfo.baseBundleInfo_->releaseType = "Release";
    newInfo.baseBundleInfo_->releaseType = "Release";
    oldInfo.SetAsanEnabled(true);
    newInfo.SetAsanEnabled(true);
    BaseBundleInstaller baseBundleInstaller;
    auto ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_APPEXECFWK_INSTALL_ASAN_NOT_SUPPORT);
}

/**
 * @tc.number: CheckAppLabel_0008
 * @tc.name: test the start function of CheckAppLabel
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, CheckAppLabel_0008, Function | SmallTest | Level0)
{
    InnerBundleInfo oldInfo;
    InnerBundleInfo newInfo;
    oldInfo.SetApplicationBundleType(BundleType::APP);
    newInfo.SetApplicationBundleType(BundleType::ATOMIC_SERVICE);
    BaseBundleInstaller baseBundleInstaller;
    auto ret = baseBundleInstaller.CheckAppLabel(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_APPEXECFWK_BUNDLE_TYPE_NOT_SAME);
}

/**
 * @tc.number: UpdateDefineAndRequestPermissions_0001
 * @tc.name: test the start function of UpdateDefineAndRequestPermissions_0001
 * @tc.desc: 1. UpdateDefineAndRequestPermissions
*/
HWTEST_F(BmsBundleInstallCheckerTest, UpdateDefineAndRequestPermissions_0001, Function | SmallTest | Level0)
{
    ApplicationInfo applicationInfo;
    applicationInfo.bundleName = BUNDLE_NAME;
    InnerBundleInfo oldInfo;
    oldInfo.SetBaseApplicationInfo(applicationInfo);
    InnerBundleUserInfo userInfo;
    userInfo.bundleName = BUNDLE_NAME;
    userInfo.bundleUserInfo.userId = Constants::DEFAULT_USERID;
    userInfo.accessTokenId = 100;
    userInfo.accessTokenIdEx = 100;
    oldInfo.AddInnerBundleUserInfo(userInfo);

    InnerBundleInfo newInfo;
    newInfo.SetBaseApplicationInfo(applicationInfo);
    newInfo.AddInnerBundleUserInfo(userInfo);
    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();
    auto ret = baseBundleInstaller.UpdateDefineAndRequestPermissions(oldInfo, newInfo);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.number: UpdateDefineAndRequestPermissions_0002
 * @tc.name: test the start function of UpdateDefineAndRequestPermissions_0002
 * @tc.desc: 1. UpdateDefineAndRequestPermissions
*/
HWTEST_F(BmsBundleInstallCheckerTest, UpdateDefineAndRequestPermissions_0002, Function | SmallTest | Level0)
{
    ApplicationInfo applicationInfo;
    applicationInfo.bundleName = BUNDLE_NAME;
    InnerBundleInfo oldInfo;
    oldInfo.SetBaseApplicationInfo(applicationInfo);
    InnerBundleUserInfo userInfo;
    userInfo.bundleName = BUNDLE_NAME;
    userInfo.bundleUserInfo.userId = Constants::DEFAULT_USERID;
    userInfo.accessTokenId = 100;
    userInfo.accessTokenIdEx = 100;
    oldInfo.AddInnerBundleUserInfo(userInfo);
    oldInfo.SetAppType(Constants::AppType::SYSTEM_APP);

    InnerBundleInfo newInfo;
    newInfo.SetBaseApplicationInfo(applicationInfo);
    newInfo.AddInnerBundleUserInfo(userInfo);
    newInfo.SetAppType(Constants::AppType::THIRD_PARTY_APP);

    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();
    bool ret = baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::INSTALL_START);
    EXPECT_TRUE(ret);
    ret =  baseBundleInstaller.dataMgr_->AddInnerBundleInfo(BUNDLE_NAME, oldInfo);
    EXPECT_TRUE(ret);

    auto errCode = baseBundleInstaller.UpdateDefineAndRequestPermissions(oldInfo, newInfo);
    EXPECT_EQ(errCode, ERR_OK);
    ret = baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::UNINSTALL_START);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number: UpdateDefineAndRequestPermissions_0003
 * @tc.name: test the start function of UpdateDefineAndRequestPermissions_0003
 * @tc.desc: 1. UpdateDefineAndRequestPermissions
*/
HWTEST_F(BmsBundleInstallCheckerTest, UpdateDefineAndRequestPermissions_0003, Function | SmallTest | Level0)
{
    ApplicationInfo applicationInfo;
    applicationInfo.bundleName = BUNDLE_NAME;
    InnerBundleInfo oldInfo;
    oldInfo.SetBaseApplicationInfo(applicationInfo);
    InnerBundleUserInfo userInfo;
    userInfo.bundleName = BUNDLE_NAME;
    userInfo.bundleUserInfo.userId = Constants::DEFAULT_USERID;
    userInfo.accessTokenId = 100;
    userInfo.accessTokenIdEx = 0;
    oldInfo.AddInnerBundleUserInfo(userInfo);

    InnerBundleInfo newInfo;
    newInfo.SetBaseApplicationInfo(applicationInfo);
    newInfo.AddInnerBundleUserInfo(userInfo);

    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();
    bool ret = baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::INSTALL_START);
    EXPECT_TRUE(ret);
    ret =  baseBundleInstaller.dataMgr_->AddInnerBundleInfo(BUNDLE_NAME, oldInfo);
    EXPECT_TRUE(ret);

    auto errCode = baseBundleInstaller.UpdateDefineAndRequestPermissions(oldInfo, newInfo);
    EXPECT_EQ(errCode, ERR_OK);
    ret = baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::UNINSTALL_START);
    EXPECT_TRUE(ret);
}

/**
 * @tc.number: UpdateDefineAndRequestPermissions_0004
 * @tc.name: test the start function of UpdateDefineAndRequestPermissions_0004
 * @tc.desc: 1. UpdateDefineAndRequestPermissions
*/
HWTEST_F(BmsBundleInstallCheckerTest, UpdateDefineAndRequestPermissions_0004, Function | SmallTest | Level0)
{
    ApplicationInfo applicationInfo;
    applicationInfo.bundleName = BUNDLE_NAME;
    InnerBundleInfo oldInfo;
    oldInfo.SetBaseApplicationInfo(applicationInfo);
    InnerBundleUserInfo userInfo;
    userInfo.bundleName = BUNDLE_NAME;
    userInfo.bundleUserInfo.userId = Constants::DEFAULT_USERID;
    userInfo.accessTokenId = 100;
    userInfo.accessTokenIdEx = 0;
    oldInfo.AddInnerBundleUserInfo(userInfo);

    InnerBundleInfo newInfo;
    newInfo.SetBaseApplicationInfo(applicationInfo);
    newInfo.AddInnerBundleUserInfo(userInfo);

    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();
    auto errCode = baseBundleInstaller.UpdateDefineAndRequestPermissions(oldInfo, newInfo);
    EXPECT_EQ(errCode, ERR_APPEXECFWK_INSTALL_INTERNAL_ERROR);
}

/**
 * @tc.number: GetCallingEventInfo_0001
 * @tc.name: test the start function of GetCallingEventInfo
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetCallingEventInfo_0001, Function | SmallTest | Level0)
{
    EventInfo eventInfo;
    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.GetCallingEventInfo(eventInfo);
    EXPECT_EQ(eventInfo.callingBundleName, Constants::EMPTY_STRING);
}

/**
 * @tc.number: GetCallingEventInfo_0002
 * @tc.name: test the start function of GetCallingEventInfo
 * @tc.desc: 1. BaseBundleInstaller
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetCallingEventInfo_0002, Function | SmallTest | Level0)
{
    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();
    EventInfo eventInfo;
    baseBundleInstaller.GetCallingEventInfo(eventInfo);
    EXPECT_EQ(eventInfo.callingBundleName, Constants::EMPTY_STRING);
}

/**
 * @tc.number: GetCallingEventInfo_0003
 * @tc.name: test the start function of GetCallingEventInfo
 * @tc.desc: 1. BaseBundleInstaller
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetCallingEventInfo_0003, Function | SmallTest | Level0)
{
    InnerBundleInfo info;
    BundleInfo bundleInfo;
    bundleInfo.name = BUNDLE_NAME;
    bundleInfo.applicationInfo.name = BUNDLE_NAME;
    ApplicationInfo applicationInfo;
    applicationInfo.name = BUNDLE_NAME;
    applicationInfo.bundleName = BUNDLE_NAME;
    info.SetBaseBundleInfo(bundleInfo);
    info.SetBaseApplicationInfo(applicationInfo);
    InnerBundleUserInfo innerBundleUserInfo;
    innerBundleUserInfo.uid = 20010999;
    innerBundleUserInfo.bundleName = BUNDLE_NAME;
    innerBundleUserInfo.bundleUserInfo.userId = 100;
    info.AddInnerBundleUserInfo(innerBundleUserInfo);
    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();
    EXPECT_NE(baseBundleInstaller.dataMgr_, nullptr);
    bool ret1 = baseBundleInstaller.dataMgr_ ->UpdateBundleInstallState(BUNDLE_NAME, InstallState::INSTALL_START);
    bool ret2 =  baseBundleInstaller.dataMgr_ ->AddInnerBundleInfo(BUNDLE_NAME, info);
    EXPECT_TRUE(ret1);
    EXPECT_TRUE(ret2);

    EventInfo eventInfo;
    eventInfo.callingUid = 20010999;
    baseBundleInstaller.GetCallingEventInfo(eventInfo);
    EXPECT_EQ(eventInfo.callingBundleName, BUNDLE_NAME);

    baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::UNINSTALL_START);
}

/**
 * @tc.number: GetInstallEventInfo_0001
 * @tc.name: test the start function of GetInstallEventInfo
 * @tc.desc: 1. BaseBundleInstaller
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetInstallEventInfo_0001, Function | SmallTest | Level0)
{
    BaseBundleInstaller baseBundleInstaller;
    EventInfo eventInfo;
    std::unordered_map<std::string, InnerBundleInfo> newInfos;
    baseBundleInstaller.GetInstallEventInfo(newInfos, eventInfo);
    EXPECT_EQ(eventInfo.fingerprint, Constants::EMPTY_STRING);
}

/**
 * @tc.number: GetInstallEventInfo_0002
 * @tc.name: test the start function of GetInstallEventInfo
 * @tc.desc: 1. BaseBundleInstaller
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetInstallEventInfo_0002, Function | SmallTest | Level0)
{
    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();;
    EventInfo eventInfo;
    std::unordered_map<std::string, InnerBundleInfo> newInfos;
    baseBundleInstaller.GetInstallEventInfo(newInfos, eventInfo);
    EXPECT_EQ(eventInfo.fingerprint, Constants::EMPTY_STRING);
}

/**
 * @tc.number: GetInstallEventInfo_0003
 * @tc.name: test the start function of GetInstallEventInfo
 * @tc.desc: 1. BaseBundleInstaller
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetInstallEventInfo_0003, Function | SmallTest | Level0)
{
    InnerBundleInfo info;
    BundleInfo bundleInfo;
    bundleInfo.name = BUNDLE_NAME;
    bundleInfo.applicationInfo.name = BUNDLE_NAME;
    ApplicationInfo applicationInfo;
    applicationInfo.name = BUNDLE_NAME;
    applicationInfo.bundleName = BUNDLE_NAME;
    applicationInfo.appDistributionType = Constants::APP_DISTRIBUTION_TYPE_APP_GALLERY;
    info.SetBaseBundleInfo(bundleInfo);
    info.SetBaseApplicationInfo(applicationInfo);
    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();
    EXPECT_NE(baseBundleInstaller.dataMgr_, nullptr);
    bool ret1 = baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::INSTALL_START);
    bool ret2 =  baseBundleInstaller.dataMgr_->AddInnerBundleInfo(BUNDLE_NAME, info);
    EXPECT_TRUE(ret1);
    EXPECT_TRUE(ret2);

    baseBundleInstaller.bundleName_ = BUNDLE_NAME;
    EventInfo eventInfo;
    std::unordered_map<std::string, InnerBundleInfo> newInfos;
    baseBundleInstaller.GetInstallEventInfo(newInfos, eventInfo);
    EXPECT_EQ(eventInfo.appDistributionType, Constants::APP_DISTRIBUTION_TYPE_APP_GALLERY);

    baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::UNINSTALL_START);
}

/**
 * @tc.number: GetInstallEventInfo_0004
 * @tc.name: test the start function of GetInstallEventInfo
 * @tc.desc: 1. BaseBundleInstaller
*/
HWTEST_F(BmsBundleInstallCheckerTest, GetInstallEventInfo_0004, Function | SmallTest | Level0)
{
    InnerBundleInfo info;
    BundleInfo bundleInfo;
    bundleInfo.name = BUNDLE_NAME;
    bundleInfo.applicationInfo.name = BUNDLE_NAME;
    ApplicationInfo applicationInfo;
    applicationInfo.name = BUNDLE_NAME;
    applicationInfo.bundleName = BUNDLE_NAME;
    applicationInfo.appDistributionType = Constants::APP_DISTRIBUTION_TYPE_APP_GALLERY;
    info.SetBaseBundleInfo(bundleInfo);
    info.SetBaseApplicationInfo(applicationInfo);
    BaseBundleInstaller baseBundleInstaller;
    baseBundleInstaller.dataMgr_ = std::make_shared<BundleDataMgr>();
    EXPECT_NE(baseBundleInstaller.dataMgr_, nullptr);
    bool ret1 = baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::INSTALL_START);
    bool ret2 =  baseBundleInstaller.dataMgr_->AddInnerBundleInfo(BUNDLE_NAME, info);
    EXPECT_TRUE(ret1);
    EXPECT_TRUE(ret2);

    baseBundleInstaller.bundleName_ = BUNDLE_NAME;
    EventInfo eventInfo;
    std::unordered_map<std::string, InnerBundleInfo> newInfos;
    InnerModuleInfo moduleInfo;
    moduleInfo.hapPath = "xxxx.hap";
    moduleInfo.hashValue = "111";
    info.InsertInnerModuleInfo(BUNDLE_NAME, moduleInfo);
    newInfos.emplace(BUNDLE_NAME, info);
    baseBundleInstaller.GetInstallEventInfo(newInfos, eventInfo);
    EXPECT_EQ(eventInfo.appDistributionType, Constants::APP_DISTRIBUTION_TYPE_APP_GALLERY);
    if (!eventInfo.filePath.empty()) {
        EXPECT_EQ(eventInfo.filePath[0], moduleInfo.hapPath);
        EXPECT_EQ(eventInfo.hashValue[0], moduleInfo.hashValue);
    }
    baseBundleInstaller.dataMgr_->UpdateBundleInstallState(BUNDLE_NAME, InstallState::UNINSTALL_START);
}

/**
 * @tc.number: ConvertToAppProvisionInfo_0001
 * @tc.name: test the start function of ConvertToAppProvisionInfo
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, ConvertToAppProvisionInfo_0001, Function | SmallTest | Level0)
{
    BundleInstallChecker bundleInstallChecker;
    Security::Verify::ProvisionInfo provisionInfo;
    provisionInfo.type = Security::Verify::ProvisionType::DEBUG;
    provisionInfo.distributionType = Security::Verify::AppDistType::NONE_TYPE;
    AppProvisionInfo appProvisionInfo = bundleInstallChecker.ConvertToAppProvisionInfo(provisionInfo);
    EXPECT_EQ(appProvisionInfo.type, Constants::APP_PROVISION_TYPE_DEBUG);
    EXPECT_EQ(appProvisionInfo.appDistributionType, Constants::APP_DISTRIBUTION_TYPE_NONE);
    EXPECT_EQ(appProvisionInfo.apl, "normal");
}

/**
 * @tc.number: ConvertToAppProvisionInfo_0002
 * @tc.name: test the start function of ConvertToAppProvisionInfo
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, ConvertToAppProvisionInfo_0002, Function | SmallTest | Level0)
{
    BundleInstallChecker bundleInstallChecker;
    Security::Verify::ProvisionInfo provisionInfo;
    provisionInfo.type = Security::Verify::ProvisionType::RELEASE;
    provisionInfo.distributionType = Security::Verify::AppDistType::APP_GALLERY;
    AppProvisionInfo appProvisionInfo = bundleInstallChecker.ConvertToAppProvisionInfo(provisionInfo);
    EXPECT_EQ(appProvisionInfo.type, Constants::APP_PROVISION_TYPE_RELEASE);
    EXPECT_EQ(appProvisionInfo.appDistributionType, Constants::APP_DISTRIBUTION_TYPE_APP_GALLERY);
    EXPECT_EQ(appProvisionInfo.apl, "normal");
}

/**
 * @tc.number: ConvertToAppProvisionInfo_0003
 * @tc.name: test the start function of ConvertToAppProvisionInfo
 * @tc.desc: 1. BundleInstallChecker
*/
HWTEST_F(BmsBundleInstallCheckerTest, ConvertToAppProvisionInfo_0003, Function | SmallTest | Level0)
{
    BundleInstallChecker bundleInstallChecker;
    Security::Verify::ProvisionInfo provisionInfo;
    provisionInfo.type = Security::Verify::ProvisionType::RELEASE;
    provisionInfo.distributionType = Security::Verify::AppDistType::ENTERPRISE;
    provisionInfo.bundleInfo.apl = "system_basic";
    AppProvisionInfo appProvisionInfo = bundleInstallChecker.ConvertToAppProvisionInfo(provisionInfo);
    EXPECT_EQ(appProvisionInfo.type, Constants::APP_PROVISION_TYPE_RELEASE);
    EXPECT_EQ(appProvisionInfo.appDistributionType, Constants::APP_DISTRIBUTION_TYPE_ENTERPRISE);
    EXPECT_EQ(appProvisionInfo.apl, "system_basic");

    provisionInfo.distributionType = Security::Verify::AppDistType::OS_INTEGRATION;
    appProvisionInfo = bundleInstallChecker.ConvertToAppProvisionInfo(provisionInfo);
    EXPECT_EQ(appProvisionInfo.appDistributionType, Constants::APP_DISTRIBUTION_TYPE_OS_INTEGRATION);

    provisionInfo.distributionType = Security::Verify::AppDistType::CROWDTESTING;
    appProvisionInfo = bundleInstallChecker.ConvertToAppProvisionInfo(provisionInfo);
    EXPECT_EQ(appProvisionInfo.appDistributionType, Constants::APP_DISTRIBUTION_TYPE_CROWDTESTING);
}

/**
 * @tc.number: AddAppProvisionInfo_0001
 * @tc.name: test the start function of AddAppProvisionInfo and DeleteAppProvisionInfo
 * @tc.desc: 1. BaseBundleInstaller
*/
HWTEST_F(BmsBundleInstallCheckerTest, AddAppProvisionInfo_0001, Function | SmallTest | Level0)
{
    BaseBundleInstaller baseBundleInstaller;
    Security::Verify::ProvisionInfo appProvisionInfo;
    auto ret = baseBundleInstaller.AddAppProvisionInfo("", appProvisionInfo);
    EXPECT_FALSE(ret);

    ret = baseBundleInstaller.AddAppProvisionInfo(BUNDLE_NAME, appProvisionInfo);
    EXPECT_TRUE(ret);

    ret = baseBundleInstaller.DeleteAppProvisionInfo(BUNDLE_NAME);
    EXPECT_TRUE(ret);
}
} // OHOS
