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

#include "quick_fix/patch_profile.h"

#include <sstream>

#include "app_log_wrapper.h"
#include "bundle_constants.h"
#include "bundle_util.h"
#include "json_util.h"
#include "parameter.h"
#include "string_ex.h"

namespace OHOS {
namespace AppExecFwk {
namespace PatchProfileReader {
constexpr const char* BUNDLE_PATCH_PROFILE_APP_KEY_BUNDLE_NAME = "bundleName";
constexpr const char* BUNDLE_PATCH_PROFILE_APP_KEY_VERSION_CODE = "versionCode";
constexpr const char* BUNDLE_PATCH_PROFILE_APP_KEY_VERSION_NAME = "versionName";
constexpr const char* BUNDLE_PATCH_PROFILE_APP_KEY_PATCH_VERSION_CODE = "patchVersionCode";
constexpr const char* BUNDLE_PATCH_PROFILE_APP_KEY_PATCH_VERSION_NAME = "patchVersionName";
constexpr const char* BUNDLE_PATCH_PROFILE_MODULE_KEY_NAME = "name";
constexpr const char* BUNDLE_PATCH_PROFILE_MODULE_KEY_TYPE = "type";
constexpr const char* BUNDLE_PATCH_PROFILE_MODULE_KEY_DEVICE_TYPES = "deviceTypes";
constexpr const char* BUNDLE_PATCH_PROFILE_MODULE_KEY_ORIGINAL_MODULE_HASH = "originalModuleHash";
constexpr const char* BUNDLE_PATCH_PROFILE_KEY_APP = "app";
constexpr const char* BUNDLE_PATCH_PROFILE_KEY_MODULE = "module";

thread_local int32_t parseResult = 0;
struct App {
    std::string bundleName;
    int32_t versionCode = 0;
    std::string versionName;
    int32_t patchVersionCode = 0;
    std::string patchVersionName;
};

struct Module {
    std::string name;
    std::string type;
    std::vector<std::string> deviceTypes;
    std::string originalModuleHash;
};

struct PatchJson {
    App app;
    Module module;
};

void from_json(const nlohmann::json &jsonObject, App &app)
{
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_APP_KEY_BUNDLE_NAME,
        app.bundleName,
        JsonType::STRING,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_APP_KEY_VERSION_CODE,
        app.versionCode,
        JsonType::NUMBER,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_APP_KEY_VERSION_NAME,
        app.versionName,
        JsonType::STRING,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_APP_KEY_PATCH_VERSION_CODE,
        app.patchVersionCode,
        JsonType::NUMBER,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_APP_KEY_PATCH_VERSION_NAME,
        app.patchVersionName,
        JsonType::STRING,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
}

void from_json(const nlohmann::json &jsonObject, Module &module)
{
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_MODULE_KEY_NAME,
        module.name,
        JsonType::STRING,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_MODULE_KEY_TYPE,
        module.type,
        JsonType::STRING,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_MODULE_KEY_DEVICE_TYPES,
        module.deviceTypes,
        JsonType::ARRAY,
        true,
        parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_MODULE_KEY_ORIGINAL_MODULE_HASH,
        module.originalModuleHash,
        JsonType::STRING,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
}

void from_json(const nlohmann::json &jsonObject, PatchJson &patchJson)
{
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<App>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_KEY_APP,
        patchJson.app,
        JsonType::OBJECT,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<Module>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PATCH_PROFILE_KEY_MODULE,
        patchJson.module,
        JsonType::OBJECT,
        true,
        parseResult,
        ArrayType::NOT_ARRAY);
}

void ToPatchInfo(PatchJson &patchJson, AppQuickFix &appQuickFix)
{
    appQuickFix.bundleName = patchJson.app.bundleName;
    appQuickFix.versionCode = patchJson.app.versionCode;
    appQuickFix.versionName = patchJson.app.versionName;
    appQuickFix.deployingAppqfInfo.versionCode = patchJson.app.patchVersionCode;
    appQuickFix.deployingAppqfInfo.versionName = patchJson.app.patchVersionName;
    HqfInfo hqfInfo;
    hqfInfo.moduleName = patchJson.module.name;
    hqfInfo.hapSha256 = patchJson.module.originalModuleHash;
    appQuickFix.deployingAppqfInfo.hqfInfos.emplace_back(hqfInfo);
}
}

bool PatchProfile::DefaultNativeSo(
    const PatchExtractor &patchExtractor, bool isSystemLib64Exist, AppqfInfo &appqfInfo)
{
    if (isSystemLib64Exist) {
        if (patchExtractor.IsDirExist(Constants::LIBS + Constants::ARM64_V8A)) {
            appqfInfo.cpuAbi = Constants::ARM64_V8A;
            auto iter = Constants::ABI_MAP.find(Constants::ARM64_V8A);
            if (iter != Constants::ABI_MAP.end()) {
                appqfInfo.nativeLibraryPath = Constants::LIBS + iter->second;
                return true;
            }
            APP_LOGE("Can't find ARM64_V8A in ABI_MAP");
            return false;
        }
        APP_LOGE(" ARM64_V8A's directory doesn't exist");
        return false;
    }

    if (patchExtractor.IsDirExist(Constants::LIBS + Constants::ARM_EABI_V7A)) {
        appqfInfo.cpuAbi = Constants::ARM_EABI_V7A;
        auto iter = Constants::ABI_MAP.find(Constants::ARM_EABI_V7A);
        if (iter != Constants::ABI_MAP.end()) {
            appqfInfo.nativeLibraryPath = Constants::LIBS + iter->second;
            return true;
        }
        APP_LOGE("Can't find ARM_EABI_V7A in ABI_MAP");
        return false;
    }

    if (patchExtractor.IsDirExist(Constants::LIBS + Constants::ARM_EABI)) {
        appqfInfo.cpuAbi = Constants::ARM_EABI;
        auto iter = Constants::ABI_MAP.find(Constants::ARM_EABI);
        if (iter != Constants::ABI_MAP.end()) {
            appqfInfo.nativeLibraryPath = Constants::LIBS + iter->second;
            return true;
        }
        APP_LOGE("Can't find ARM_EABI in ABI_MAP");
        return false;
    }
    APP_LOGE("ARM_EABI_V7A and ARM_EABI directories do not exist");
    return false;
}

bool PatchProfile::ParseNativeSo(const PatchExtractor &patchExtractor, AppqfInfo &appqfInfo)
{
    std::string abis = GetAbiList();
    std::vector<std::string> abiList;
    SplitStr(abis, Constants::ABI_SEPARATOR, abiList, false, false);
    if (abiList.empty()) {
        APP_LOGE("Abi is empty");
        return false;
    }
    bool isDefault = std::find(abiList.begin(), abiList.end(), Constants::ABI_DEFAULT) != abiList.end();
    bool isSystemLib64Exist = BundleUtil::IsExistDir(Constants::SYSTEM_LIB64);
    APP_LOGD("abi list : %{public}s, isDefault : %{public}d, isSystemLib64Exist : %{public}d",
        abis.c_str(), isDefault, isSystemLib64Exist);
    bool soExist = patchExtractor.IsDirExist(Constants::LIBS);
    if (!soExist) {
        APP_LOGD("so not exist");
        if (isDefault) {
            appqfInfo.cpuAbi = isSystemLib64Exist ? Constants::ARM64_V8A : Constants::ARM_EABI_V7A;
            return true;
        }
        for (const auto &abi : abiList) {
            if (Constants::ABI_MAP.find(abi) != Constants::ABI_MAP.end()) {
                appqfInfo.cpuAbi = abi;
                return true;
            }
        }
        APP_LOGE("None of the abiList are in the ABI_MAP");
        return false;
    }

    APP_LOGD("so exist");
    if (isDefault) {
        return DefaultNativeSo(patchExtractor, isSystemLib64Exist, appqfInfo);
    }
    
    for (const auto &abi : abiList) {
        std::string libsPath;
        libsPath.append(Constants::LIBS).append(abi).append(Constants::PATH_SEPARATOR);
        if (Constants::ABI_MAP.find(abi) != Constants::ABI_MAP.end() && patchExtractor.IsDirExist(libsPath)) {
            appqfInfo.cpuAbi = abi;
            auto iter = Constants::ABI_MAP.find(abi);
            if (iter != Constants::ABI_MAP.end()) {
                appqfInfo.nativeLibraryPath = Constants::LIBS + iter->second;
                return true;
            }
            APP_LOGE("Can't find %{public}s in ABI_MAP", abi.c_str());
            return false;
        }
    }
    return false;
}

ErrCode PatchProfile::TransformTo(
    const std::ostringstream &source, const PatchExtractor &patchExtractor, AppQuickFix &appQuickFix)
{
    nlohmann::json jsonObject = nlohmann::json::parse(source.str(), nullptr, false);
    if (jsonObject.is_discarded()) {
        APP_LOGE("bad profile");
        return ERR_APPEXECFWK_PARSE_BAD_PROFILE;
    }
    PatchProfileReader::PatchJson patchJson = jsonObject.get<PatchProfileReader::PatchJson>();
    if (PatchProfileReader::parseResult != ERR_OK) {
        APP_LOGE("parseResult is %{public}d", PatchProfileReader::parseResult);
        int32_t ret = PatchProfileReader::parseResult;
        PatchProfileReader::parseResult = ERR_OK;
        return ret;
    }
    PatchProfileReader::ToPatchInfo(patchJson, appQuickFix);
    if (!ParseNativeSo(patchExtractor, appQuickFix.deployingAppqfInfo)) {
        APP_LOGE("ParseNativeSo failed");
        return ERR_APPEXECFWK_PARSE_NATIVE_SO_FAILED;
    }
    return ERR_OK;
}
}  // namespace AppExecFwk
}  // namespace OHOS