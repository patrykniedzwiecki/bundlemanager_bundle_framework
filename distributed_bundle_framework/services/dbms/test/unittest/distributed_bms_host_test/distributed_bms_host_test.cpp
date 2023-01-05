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
#include <gtest/gtest.h>
#include "distributed_bms_host.h"

#include "appexecfwk_errors.h"
#include "distributed_bms_proxy.h"
#include "mock_distributed_bms_interface.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;

namespace OHOS {
class DistributedBmsHostTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void DistributedBmsHostTest::SetUpTestCase()
{}

void DistributedBmsHostTest::TearDownTestCase()
{}

void DistributedBmsHostTest::SetUp()
{}

void DistributedBmsHostTest::TearDown()
{}

/**
 * @tc.number: OnRemoteRequest_0100
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0100, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_REMOTE_ABILITY_INFO), data, reply, option);
    EXPECT_EQ(res, ERR_INVALID_STATE);
}

/**
 * @tc.number: OnRemoteRequest_0200
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0200, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    AppExecFwk::ElementName elementName;
    data.WriteParcelable(&elementName);
    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>(IDistributedBms::Message::GET_REMOTE_ABILITY_INFO),
        data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_0300
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0300, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    AppExecFwk::ElementName elementName;
    data.WriteParcelable(&elementName);
    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>(IDistributedBms::Message::GET_REMOTE_ABILITY_INFO_WITH_LOCALE),
        data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_0400
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0400, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    std::vector<ElementName> elementNames;
    std::string localeInfo;
    DistributedBmsProxy proxy(nullptr);
    proxy.WriteParcelableVector(elementNames, data);
    data.WriteString(localeInfo);
    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_REMOTE_ABILITY_INFOS), data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_0500
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0500, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    std::vector<ElementName> elementNames;
    std::string localeInfo;
    DistributedBmsProxy proxy(nullptr);
    proxy.WriteParcelableVector(elementNames, data);
    data.WriteString(localeInfo);
    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_REMOTE_ABILITY_INFOS_WITH_LOCALE), data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_0600
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0600, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    std::vector<ElementName> elementNames;
    std::string localeInfo;
    DistributedBmsProxy proxy(nullptr);
    proxy.WriteParcelableVector(elementNames, data);
    data.WriteString(localeInfo);
    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_REMOTE_ABILITY_INFOS_WITH_LOCALE), data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_0700
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0700, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    ElementName elementName;
    std::string localeInfo;
    data.WriteParcelable(&elementName);
    data.WriteString(localeInfo);
    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_ABILITY_INFO), data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_0800
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0800, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    ElementName elementName;
    std::string localeInfo;
    data.WriteParcelable(&elementName);
    data.WriteString(localeInfo);

    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_ABILITY_INFO_WITH_LOCALE), data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_0900
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_0900, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    std::vector<ElementName> elementNames;
    std::string localeInfo;
    DistributedBmsProxy proxy(nullptr);
    proxy.WriteParcelableVector(elementNames, data);
    data.WriteString(localeInfo);

    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_ABILITY_INFOS), data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_1000
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_1000, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    std::vector<ElementName> elementNames;
    std::string localeInfo;
    DistributedBmsProxy proxy(nullptr);
    proxy.WriteParcelableVector(elementNames, data);
    data.WriteString(localeInfo);

    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_ABILITY_INFOS_WITH_LOCALE), data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_1100
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_1100, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    
    std::string networkId;
    std::string bundleName;
    data.WriteString(networkId);
    data.WriteString(bundleName);

    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>
        (IDistributedBms::Message::GET_DISTRIBUTED_BUNDLE_INFO), data, reply, option);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: OnRemoteRequest_1200
 * @tc.name: Test OnRemoteRequest
 * @tc.desc: Verify the OnRemoteRequest return IPC_STUB_UNKNOW_TRANS_ERR.
 */
HWTEST_F(DistributedBmsHostTest, OnRemoteRequest_1200, Function | MediumTest | Level1)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(DistributedBmsHost::GetDescriptor());
    
    std::string networkId;
    std::string bundleName;
    data.WriteString(networkId);
    data.WriteString(bundleName);

    MockDistributedBmsHost host;
    int res = host.OnRemoteRequest(static_cast<uint32_t>(-1), data, reply, option);
    EXPECT_EQ(res, IPC_STUB_UNKNOW_TRANS_ERR);
}

/**
 * @tc.number: HandleGetRemoteAbilityInfo_0100
 * @tc.name: Test HandleGetRemoteAbilityInfo
 * @tc.desc: Verify the HandleGetRemoteAbilityInfo return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetRemoteAbilityInfo_0100, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    ElementName elementName;
    elementName.SetBundleName("bundleName");
    data.WriteParcelable(&elementName);
    std::string localeInfo = "localeInfo";
    data.WriteString(localeInfo);
    MockDistributedBmsHost host;
    int res = host.HandleGetRemoteAbilityInfo(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: HandleGetRemoteAbilityInfo_0200
 * @tc.name: Test HandleGetRemoteAbilityInfo
 * @tc.desc: Verify the HandleGetRemoteAbilityInfo return ERR_APPEXECFWK_PARCEL_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetRemoteAbilityInfo_0200, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    int res = host.HandleGetRemoteAbilityInfo(data, reply);
    EXPECT_EQ(res, ERR_APPEXECFWK_PARCEL_ERROR);
}

/**
 * @tc.number: HandleGetRemoteAbilityInfo_0300
 * @tc.name: Test HandleGetRemoteAbilityInfo
 * @tc.desc: Verify the HandleGetRemoteAbilityInfo return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetRemoteAbilityInfo_0300, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    ElementName elementName;
    elementName.SetBundleName("");
    data.WriteParcelable(&elementName);
    int res = host.HandleGetRemoteAbilityInfo(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: HandleGetRemoteAbilityInfo_0400
 * @tc.name: Test HandleGetRemoteAbilityInfo
 * @tc.desc: Verify the HandleGetRemoteAbilityInfo return ERR_APPEXECFWK_PARCEL_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetRemoteAbilityInfo_0400, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    ElementName elementName;
    elementName.SetBundleName("bundleName");
    data.WriteParcelable(&elementName);
    int res = host.HandleGetRemoteAbilityInfo(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: HandleGetRemoteAbilityInfos_0100
 * @tc.name: Test HandleGetRemoteAbilityInfos
 * @tc.desc: Verify the HandleGetRemoteAbilityInfos return ERR_APPEXECFWK_PARCEL_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetRemoteAbilityInfos_0100, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    std::vector<ElementName> elementNames;
    std::string localeInfo;
    DistributedBmsProxy proxy(nullptr);
    data.WriteString(localeInfo);
    proxy.WriteParcelableVector(elementNames, data);
    int res = host.HandleGetRemoteAbilityInfos(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: HandleGetRemoteAbilityInfos_0200
 * @tc.name: Test HandleGetRemoteAbilityInfos
 * @tc.desc: Verify the HandleGetRemoteAbilityInfos return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetRemoteAbilityInfos_0200, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    int res = host.HandleGetRemoteAbilityInfos(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: HandleGetRemoteAbilityInfos_0300
 * @tc.name: Test HandleGetRemoteAbilityInfos
 * @tc.desc: Verify the HandleGetRemoteAbilityInfos return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetRemoteAbilityInfos_0300, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    std::vector<ElementName> elementNames;
    DistributedBmsProxy proxy(nullptr);
    proxy.WriteParcelableVector(elementNames, data);
    int res = host.HandleGetRemoteAbilityInfos(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: HandleGetAbilityInfo_0100
 * @tc.name: Test HandleGetAbilityInfo
 * @tc.desc: Verify the HandleGetAbilityInfo return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetAbilityInfo_0100, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    ElementName elementName;
    elementName.SetBundleName("bundleName");
    data.WriteParcelable(&elementName);
    int res = host.HandleGetAbilityInfo(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: HandleGetAbilityInfo_0200
 * @tc.name: Test HandleGetAbilityInfo
 * @tc.desc: Verify the HandleGetAbilityInfo return NO_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetAbilityInfo_0200, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    ElementName elementName;
    elementName.SetBundleName("bundleName");
    data.WriteParcelable(&elementName);
    std::string localeInfo = "localeInfo";
    data.WriteString(localeInfo);
    int res = host.HandleGetAbilityInfo(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}

/**
 * @tc.number: HandleGetAbilityInfo_0300
 * @tc.name: Test HandleGetAbilityInfo
 * @tc.desc: Verify the HandleGetAbilityInfo return ERR_APPEXECFWK_PARCEL_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetAbilityInfo_0300, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    int res = host.HandleGetAbilityInfo(data, reply);
    EXPECT_EQ(res, ERR_APPEXECFWK_PARCEL_ERROR);
}

/**
 * @tc.number: HandleGetAbilityInfo_0400
 * @tc.name: Test HandleGetAbilityInfo
 * @tc.desc: Verify the HandleGetAbilityInfo return ERR_APPEXECFWK_PARCEL_ERROR.
 */
HWTEST_F(DistributedBmsHostTest, HandleGetAbilityInfo_0400, Function | MediumTest | Level1)
{
    Parcel data;
    Parcel reply;
    MockDistributedBmsHost host;
    std::string localeInfo = "localeInfo";
    data.WriteString(localeInfo);
    int res = host.HandleGetAbilityInfo(data, reply);
    EXPECT_EQ(res, NO_ERROR);
}
}