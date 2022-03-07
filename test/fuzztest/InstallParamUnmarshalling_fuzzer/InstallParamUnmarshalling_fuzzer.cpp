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

#include <stddef.h>
#include <stdint.h>

#include "message_parcel.h"
#include "message_option.h"
#include "install_param.h"

#include "InstallParamUnmarshalling_fuzzer.h"

namespace OHOS {
    void fuzzinstallparamunmarshalling(const uint8_t* data, size_t size) {
        MessageParcel reply;
        MessageOption option;
        MessageParcel dataMessageParcel;
        dataMessageParcel.WriteBuffer(data, size);
        AppExecFwk::InstallParam::Unmarshalling(dataMessageParcel);
    }
}



// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Run your code on data.
    OHOS::fuzzinstallparamunmarshalling(data , size);
    return 0;
}

