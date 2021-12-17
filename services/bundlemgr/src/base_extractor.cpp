/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "base_extractor.h"

#include <fstream>
#include <dirent.h>

#include "string_ex.h"
#include "app_log_wrapper.h"
#include "bundle_constants.h"

namespace OHOS {
namespace AppExecFwk {
BaseExtractor::BaseExtractor(const std::string &source) : sourceFile_(source), zipFile_(source)
{
    APP_LOGI("BaseExtractor instance is created");
}

BaseExtractor::~BaseExtractor()
{
    APP_LOGI("BaseExtractor instance is destroyed");
}

bool BaseExtractor::Init()
{
    if (!zipFile_.Open()) {
        APP_LOGE("open zip file failed");
        return false;
    }
    initial_ = true;
    APP_LOGI("success");
    return true;
}

bool BaseExtractor::ExtractByName(const std::string &fileName, std::ostream &dest) const
{
    if (!initial_) {
        APP_LOGE("extractor is not initial");
        return false;
    }
    if (!zipFile_.ExtractFile(fileName, dest)) {
        APP_LOGE("extractor is not ExtractFile");
        return false;
    }
    return true;
}

bool BaseExtractor::ExtractFile(const std::string &fileName, const std::string &targetPath) const
{
    APP_LOGD("begin to extract %{public}s file into %{public}s targetPath", fileName.c_str(), targetPath.c_str());
    std::ofstream fileStream;
    fileStream.open(targetPath, std::ios_base::out | std::ios_base::binary);
    if (!fileStream.is_open()) {
        APP_LOGE("fail to open %{public}s file to write", targetPath.c_str());
        return false;
    }
    if ((!ExtractByName(fileName, fileStream)) || (!fileStream.good())) {
        APP_LOGE("fail to extract %{public}s zip file into stream", fileName.c_str());
        fileStream.clear();
        fileStream.close();
        if (remove(targetPath.c_str()) != 0) {
            APP_LOGE("fail to remove %{private}s file which writes stream error", targetPath.c_str());
        }
        return false;
    }
    fileStream.clear();
    fileStream.close();
    return true;
}

bool BaseExtractor::GetZipFileNames(std::vector<std::string> &fileNames)
{
    auto &entryMap = zipFile_.GetAllEntries();
    for (auto &entry : entryMap) {
        fileNames.emplace_back(entry.first);
    }
    return true;
}
}  // namespace AppExecFwk
}  // namespace OHOS
