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

#ifndef FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_BUNDLE_TOOL_INCLUDE_BUNDLE_TOOL_CALLBACK_STUB_H
#define FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_BUNDLE_TOOL_INCLUDE_BUNDLE_TOOL_CALLBACK_STUB_H

#include "i_bundle_tool_callback.h"
#include "iremote_object.h"
#include "iremote_stub.h"
#include "nocopyable.h"

namespace OHOS {
namespace AppExecFwk {
/**
 * @class BundleToolCallbackStub
 * BundleToolCallbackStub.
 */
class BundleToolCallbackStub : public IRemoteStub<IBundleToolCallback> {
public:
    BundleToolCallbackStub(std::condition_variable &cv): cv_(cv)
    {
    }
    virtual ~BundleToolCallbackStub() = default;

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
private:
    DISALLOW_COPY_AND_MOVE(BundleToolCallbackStub);

    std::condition_variable &cv_;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_BUNDLE_TOOL_INCLUDE_BUNDLE_TOOL_CALLBACK_STUB_H