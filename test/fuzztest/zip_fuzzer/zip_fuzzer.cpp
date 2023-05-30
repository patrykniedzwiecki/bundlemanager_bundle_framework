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

#include "zip_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "zip.h"
#include "zlib_callback_info.h"

#include "securec.h"

using namespace OHOS::AppExecFwk;
namespace OHOS {
constexpr size_t FOO_MAX_LEN = 1024;
constexpr size_t U32_AT_SIZE = 4;
constexpr size_t SIZE_fLAG = 2;

bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
{
    std::string srcPath (reinterpret_cast<const char*>(data), size);
    std::string destPath = "";
    std::shared_ptr<LIBZIP::ZlibCallbackInfo> zlibCallbackInfo;
    bool includeHiddenFiles = false;
    if (size % SIZE_fLAG == 0) {
        includeHiddenFiles = true;
    }
    Zip(srcPath, destPath, includeHiddenFiles, zlibCallbackInfo);
    LIBZIP::OPTIONS options;
    std::string srcFile (reinterpret_cast<const char*>(data), size);
    std::string destFile = "";
    Unzip(srcFile, destFile, options, zlibCallbackInfo);
    return true;
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }

    if (size < OHOS::U32_AT_SIZE) {
        return 0;
    }

    /* Validate the length of size */
    if (size > OHOS::FOO_MAX_LEN) {
        return 0;
    }
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}