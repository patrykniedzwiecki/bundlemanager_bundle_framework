/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "inner_bundle_info.h"

#include <algorithm>
#include <deque>
#include <regex>

#ifdef BUNDLE_FRAMEWORK_APP_CONTROL
#include "app_control_constants.h"
#include "app_control_manager.h"
#endif
#include "bundle_mgr_client.h"
#include "bundle_permission_mgr.h"
#include "common_profile.h"
#include "distributed_module_info.h"
#include "distributed_ability_info.h"
#include "free_install_params.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
const std::string APP_TYPE = "appType";
const std::string UID = "uid";
const std::string GID = "gid";
const std::string BUNDLE_STATUS = "bundleStatus";
const std::string BASE_APPLICATION_INFO = "baseApplicationInfo";
const std::string BASE_BUNDLE_INFO = "baseBundleInfo";
const std::string BASE_ABILITY_INFO = "baseAbilityInfos";
const std::string INNER_MODULE_INFO = "innerModuleInfos";
const std::string SKILL_INFOS = "skillInfos";
const std::string USER_ID = "userId_";
const std::string APP_FEATURE = "appFeature";
const std::string CAN_UNINSTALL = "canUninstall";
const std::string NAME = "name";
const std::string MODULE_PACKAGE = "modulePackage";
const std::string MODULE_PATH = "modulePath";
const std::string MODULE_NAME = "moduleName";
const std::string MODULE_DESCRIPTION = "description";
const std::string MODULE_DESCRIPTION_ID = "descriptionId";
const std::string MODULE_ICON = "icon";
const std::string MODULE_ICON_ID = "iconId";
const std::string MODULE_LABEL = "label";
const std::string MODULE_LABEL_ID = "labelId";
const std::string MODULE_DESCRIPTION_INSTALLATION_FREE = "installationFree";
const std::string MODULE_IS_REMOVABLE = "isRemovable";
const std::string MODULE_UPGRADE_FLAG = "upgradeFlag";
const std::string MODULE_IS_ENTRY = "isEntry";
const std::string MODULE_METADATA = "metaData";
const std::string MODULE_COLOR_MODE = "colorMode";
const std::string MODULE_DISTRO = "distro";
const std::string MODULE_REQ_CAPABILITIES = "reqCapabilities";
const std::string MODULE_DATA_DIR = "moduleDataDir";
const std::string MODULE_RES_PATH = "moduleResPath";
const std::string MODULE_HAP_PATH = "hapPath";
const std::string MODULE_ABILITY_KEYS = "abilityKeys";
const std::string MODULE_SKILL_KEYS = "skillKeys";
const std::string MODULE_FORMS = "formInfos";
const std::string MODULE_SHORTCUT = "shortcutInfos";
const std::string MODULE_COMMON_EVENT = "commonEvents";
const std::string MODULE_MAIN_ABILITY = "mainAbility";
const std::string MODULE_ENTRY_ABILITY_KEY = "entryAbilityKey";
const std::string MODULE_DEPENDENCIES = "dependencies";
const std::string MODULE_IS_LIB_ISOLATED = "isLibIsolated";
const std::string MODULE_NATIVE_LIBRARY_PATH = "nativeLibraryPath";
const std::string MODULE_CPU_ABI = "cpuAbi";
const std::string NEW_BUNDLE_NAME = "newBundleName";
const std::string MODULE_SRC_PATH = "srcPath";
const std::string MODULE_HASH_VALUE = "hashValue";
const std::string SCHEME_SEPARATOR = "://";
const std::string PORT_SEPARATOR = ":";
const std::string PATH_SEPARATOR = "/";
const std::string PARAM_SEPARATOR = "?";
const std::string IS_PREINSTALL_APP = "isPreInstallApp";
const std::string INSTALL_MARK = "installMark";
const char WILDCARD = '*';
const std::string TYPE_WILDCARD = "*/*";
const std::string INNER_BUNDLE_USER_INFOS = "innerBundleUserInfos";
const std::string MODULE_PROCESS = "process";
const std::string MODULE_SRC_ENTRANCE = "srcEntrance";
const std::string MODULE_DEVICE_TYPES = "deviceTypes";
const std::string MODULE_VIRTUAL_MACHINE = "virtualMachine";
const std::string MODULE_UI_SYNTAX = "uiSyntax";
const std::string MODULE_PAGES = "pages";
const std::string MODULE_META_DATA = "metadata";
const std::string MODULE_REQUEST_PERMISSIONS = "requestPermissions";
const std::string MODULE_DEFINE_PERMISSIONS = "definePermissions";
const std::string MODULE_EXTENSION_KEYS = "extensionKeys";
const std::string MODULE_EXTENSION_SKILL_KEYS = "extensionSkillKeys";
const std::string MODULE_IS_MODULE_JSON = "isModuleJson";
const std::string MODULE_IS_STAGE_BASED_MODEL = "isStageBasedModel";
const std::string BUNDLE_IS_NEW_VERSION = "isNewVersion";
const std::string BUNDLE_IS_NEED_UPDATE = "upgradeFlag";
const std::string BUNDLE_BASE_EXTENSION_INFOS = "baseExtensionInfos";
const std::string BUNDLE_EXTENSION_SKILL_INFOS = "extensionSkillInfos";
const std::string BUNDLE_PACK_INFO = "bundlePackInfo";
const std::string ALLOWED_ACLS = "allowedAcls";
const std::string META_DATA_SHORTCUTS_NAME = "ohos.ability.shortcuts";
const std::string APP_INDEX = "appIndex";
const std::string BUNDLE_IS_SANDBOX_APP = "isSandboxApp";
const std::string BUNDLE_SANDBOX_PERSISTENT_INFO = "sandboxPersistentInfo";
const std::string MODULE_COMPILE_MODE = "compileMode";
const std::string BUNDLE_HQF_INFOS = "hqfInfos";
const std::string MODULE_TARGET_MODULE_NAME = "targetModuleName";
const std::string MODULE_TARGET_PRIORITY = "targetPriority";
const std::string MODULE_OVERLAY_MODULE_INFO = "overlayModuleInfo";
const std::string OVERLAY_BUNDLE_INFO = "overlayBundleInfo";
const std::string OVERLAY_TYPE = "overlayType";
const std::string APPLY_QUICK_FIX_FREQUENCY = "applyQuickFixFrequency";
const std::string MODULE_ATOMIC_SERVICE_MODULE_TYPE = "atomicServiceModuleType";
const std::string MODULE_PRELOADS = "preloads";
const std::string HAS_ATOMIC_SERVICE_CONFIG = "hasAtomicServiceConfig";
const std::string MAIN_ATOMIC_MODULE_NAME = "mainAtomicModuleName";
const std::string INNER_SHARED_MODULE_INFO = "innerSharedModuleInfos";
const std::string MODULE_COMPATIBLE_POLICY = "compatiblePolicy";
const std::string MODULE_VERSION_CODE = "versionCode";
const std::string MODULE_VERSION_NAME = "versionName";
const std::string MODULE_PROXY_DATAS = "proxyDatas";
const int32_t SINGLE_HSP_VERSION = 1;

inline CompileMode ConvertCompileMode(const std::string& compileMode)
{
    if (compileMode == Profile::COMPILE_MODE_ES_MODULE) {
        return CompileMode::ES_MODULE;
    } else {
        return CompileMode::JS_BUNDLE;
    }
}

const std::string NameAndUserIdToKey(const std::string &bundleName, int32_t userId)
{
    return bundleName + Constants::FILE_UNDERLINE + std::to_string(userId);
}
}  // namespace

bool Skill::Match(const OHOS::AAFwk::Want &want) const
{
    bool matchAction = MatchAction(want.GetAction());
    if (!matchAction) {
        APP_LOGD("Action does not match");
        return false;
    }
    bool matchEntities = MatchEntities(want.GetEntities());
    if (!matchEntities) {
        APP_LOGD("Entities does not match");
        return false;
    }
    bool matchUriAndType = MatchUriAndType(want.GetUriString(), want.GetType());
    if (!matchUriAndType) {
        APP_LOGD("Uri or Type does not match");
        return false;
    }
    return true;
}

bool Skill::MatchLauncher(const OHOS::AAFwk::Want &want) const
{
    bool matchAction = MatchAction(want.GetAction());
    if (!matchAction) {
        APP_LOGD("Action does not match");
        return false;
    }
    bool matchEntities = MatchEntities(want.GetEntities());
    if (!matchEntities) {
        APP_LOGD("Entities does not match");
        return false;
    }
    return true;
}

bool Skill::MatchAction(const std::string &action) const
{
    // config actions empty, no match
    if (actions.empty()) {
        return false;
    }
    // config actions not empty, param empty, match
    if (action.empty()) {
        return true;
    }
    auto actionMatcher = [action] (const std::string &configAction) {
        if (action == configAction) {
            return true;
        }
        if (action == Constants::ACTION_HOME && configAction == Constants::WANT_ACTION_HOME) {
            return true;
        }
        if (action == Constants::WANT_ACTION_HOME && configAction == Constants::ACTION_HOME) {
            return true;
        }
        return false;
    };
    // config actions not empty, param not empty, if config actions contains param action, match
    return std::find_if(actions.cbegin(), actions.cend(), actionMatcher) != actions.cend();
}

bool Skill::MatchEntities(const std::vector<std::string> &paramEntities) const
{
    // param entities empty, match
    if (paramEntities.empty()) {
        return true;
    }
    // config entities empty, param entities not empty, not match
    if (entities.empty()) {
        return false;
    }
    // config entities not empty, param entities not empty, if every param entity in config entities, match
    std::vector<std::string>::size_type size = paramEntities.size();
    for (std::vector<std::string>::size_type i = 0; i < size; i++) {
        bool ret = std::find(entities.cbegin(), entities.cend(), paramEntities[i]) == entities.cend();
        if (ret) {
            return false;
        }
    }
    return true;
}

bool Skill::MatchUriAndType(const std::string &uriString, const std::string &type) const
{
    if (uriString.empty() && type.empty()) {
        // case1 : param uri empty, param type empty
        if (uris.empty()) {
            return true;
        }
        for (const SkillUri &skillUri : uris) {
            if (skillUri.scheme.empty() && skillUri.type.empty()) {
                return true;
            }
        }
        return false;
    }
    if (uris.empty()) {
        return false;
    }
    if (!uriString.empty() && type.empty()) {
        // case2 : param uri not empty, param type empty
        for (const SkillUri &skillUri : uris) {
            if (MatchUri(uriString, skillUri) && skillUri.type.empty()) {
                return true;
            }
        }
        return false;
    } else if (uriString.empty() && !type.empty()) {
        // case3 : param uri empty, param type not empty
        for (const SkillUri &skillUri : uris) {
            if (skillUri.scheme.empty() && MatchType(type, skillUri.type)) {
                return true;
            }
        }
        return false;
    } else {
        // case4 : param uri not empty, param type not empty
        for (const SkillUri &skillUri : uris) {
            if (MatchUri(uriString, skillUri) && MatchType(type, skillUri.type)) {
                return true;
            }
        }
        return false;
    }
}

bool Skill::StartsWith(const std::string &sourceString, const std::string &targetPrefix) const
{
    return sourceString.rfind(targetPrefix, 0) == 0;
}

std::string Skill::GetOptParamUri(const std::string &uriString) const
{
    std::size_t pos = uriString.rfind(PARAM_SEPARATOR);
    if (pos == std::string::npos) {
        return uriString;
    }
    return uriString.substr(0, pos);
}

bool Skill::MatchUri(const std::string &uriString, const SkillUri &skillUri) const
{
    if (skillUri.scheme.empty()) {
        return uriString.empty();
    }
    if (skillUri.host.empty()) {
        // config uri is : scheme
        // belows are param uri matched conditions:
        // 1.scheme
        // 2.scheme:
        // 3.scheme:/
        // 4.scheme://
        return uriString == skillUri.scheme || StartsWith(uriString, skillUri.scheme + PORT_SEPARATOR);
    }
    std::string optParamUri = GetOptParamUri(uriString);
    std::string skillUriString;
    skillUriString.append(skillUri.scheme).append(SCHEME_SEPARATOR).append(skillUri.host);
    if (!skillUri.port.empty()) {
        skillUriString.append(PORT_SEPARATOR).append(skillUri.port);
    }
    if (skillUri.path.empty() && skillUri.pathStartWith.empty() && skillUri.pathRegex.empty()) {
        // with port, config uri is : scheme://host:port
        // belows are param uri matched conditions:
        // 1.scheme://host:port
        // 2.scheme://host:port/path

        // without port, config uri is : scheme://host
        // belows are param uri matched conditions:
        // 1.scheme://host
        // 2.scheme://host/path
        // 3.scheme://host:port     scheme://host:port/path
        bool ret = (optParamUri == skillUriString || StartsWith(optParamUri, skillUriString + PATH_SEPARATOR));
        if (skillUri.port.empty()) {
            ret = ret || StartsWith(optParamUri, skillUriString + PORT_SEPARATOR);
        }
        return ret;
    }
    skillUriString.append(PATH_SEPARATOR);
    // if one of path, pathStartWith, pathRegex match, then match
    if (!skillUri.path.empty()) {
        // path match
        std::string pathUri(skillUriString);
        pathUri.append(skillUri.path);
        if (optParamUri == pathUri) {
            return true;
        }
    }
    if (!skillUri.pathStartWith.empty()) {
        // pathStartWith match
        std::string pathStartWithUri(skillUriString);
        pathStartWithUri.append(skillUri.pathStartWith);
        if (StartsWith(optParamUri, pathStartWithUri)) {
            return true;
        }
    }
    if (!skillUri.pathRegex.empty()) {
        // pathRegex match
        std::string pathRegexUri(skillUriString);
        pathRegexUri.append(skillUri.pathRegex);
        try {
            std::regex regex(pathRegexUri);
            if (regex_match(optParamUri, regex)) {
                return true;
            }
        } catch(...) {
            APP_LOGE("regex error");
        }
    }
    return false;
}

bool Skill::MatchType(const std::string &type, const std::string &skillUriType) const
{
    // type is not empty
    if (skillUriType.empty()) {
        return false;
    }
    // only match */*
    if (type == Constants::TYPE_ONLY_MATCH_WILDCARD) {
        return skillUriType == TYPE_WILDCARD;
    }
    if (type == TYPE_WILDCARD || skillUriType == TYPE_WILDCARD) {
        // param is */* or config is */*
        return true;
    }
    bool paramTypeRegex = type.back() == WILDCARD;
    if (paramTypeRegex) {
        // param is string/*
        std::string prefix = type.substr(0, type.length() - 1);
        return skillUriType.find(prefix) == 0;
    }
    bool typeRegex = skillUriType.back() == WILDCARD;
    if (typeRegex) {
        // config is string/*
        std::string prefix = skillUriType.substr(0, skillUriType.length() - 1);
        return type.find(prefix) == 0;
    } else {
        return type == skillUriType;
    }
}

InnerBundleInfo::InnerBundleInfo()
{
    baseApplicationInfo_ = std::make_shared<ApplicationInfo>();
    if (baseApplicationInfo_ == nullptr) {
        APP_LOGE("baseApplicationInfo_ is nullptr, create failed");
    }
    baseBundleInfo_ = std::make_shared<BundleInfo>();
    if (baseBundleInfo_ == nullptr) {
        APP_LOGE("baseBundleInfo_ is nullptr, create failed");
    }
    bundlePackInfo_ = std::make_shared<BundlePackInfo>();
    if (bundlePackInfo_ == nullptr) {
        APP_LOGE("bundlePackInfo_ is nullptr, create failed");
    }
    APP_LOGD("inner bundle info instance is created");
}

InnerBundleInfo &InnerBundleInfo::operator=(const InnerBundleInfo &info)
{
    if (this == &info) {
        return *this;
    }
    this->appType_ = info.appType_;
    this->uid_ = info.uid_;
    this->gid_ = info.gid_;
    this->userId_ = info.userId_;
    this->bundleStatus_ = info.bundleStatus_;
    this->appFeature_ = info.appFeature_;
    this->allowedAcls_ = info.allowedAcls_;
    this->mark_ = info.mark_;
    this->appIndex_ = info.appIndex_;
    this->isSandboxApp_ = info.isSandboxApp_;
    this->currentPackage_ = info.currentPackage_;
    this->onlyCreateBundleUser_ = info.onlyCreateBundleUser_;
    this->innerModuleInfos_ = info.innerModuleInfos_;
    this->innerSharedModuleInfos_ = info.innerSharedModuleInfos_;
    this->formInfos_ = info.formInfos_;
    this->commonEvents_ = info.commonEvents_;
    this->shortcutInfos_ = info.shortcutInfos_;
    this->baseAbilityInfos_ = info.baseAbilityInfos_;
    this->skillInfos_ = info.skillInfos_;
    this->innerBundleUserInfos_ = info.innerBundleUserInfos_;
    this->bundlePackInfo_ = std::make_shared<BundlePackInfo>();
    if (this->bundlePackInfo_ == nullptr) {
        APP_LOGE("bundlePackInfo_ is nullptr, create failed");
    } else {
        *(this->bundlePackInfo_) = *(info.bundlePackInfo_);
    }
    this->isNewVersion_ = info.isNewVersion_;
    this->baseExtensionInfos_= info.baseExtensionInfos_;
    this->extensionSkillInfos_ = info.extensionSkillInfos_;
    this->sandboxPersistentInfo_ = info.sandboxPersistentInfo_;
    this->baseApplicationInfo_ = std::make_shared<ApplicationInfo>();
    if (this->baseApplicationInfo_ == nullptr) {
        APP_LOGE("baseApplicationInfo_ is nullptr, create failed");
    } else {
        *(this->baseApplicationInfo_) = *(info.baseApplicationInfo_);
    }
    this->baseBundleInfo_ = std::make_shared<BundleInfo>();
    if (this->baseBundleInfo_ == nullptr) {
        APP_LOGE("baseBundleInfo_ is nullptr, create failed");
    } else {
        *(this->baseBundleInfo_) = *(info.baseBundleInfo_);
    }
    this->hqfInfos_ = info.hqfInfos_;
    this->overlayBundleInfo_ = info.overlayBundleInfo_;
    this->overlayType_ = info.overlayType_;
    this->applyQuickFixFrequency_ = info.applyQuickFixFrequency_;
    this->hasAtomicServiceConfig_ = info.hasAtomicServiceConfig_;
    this->mainAtomicModuleName_ = info.mainAtomicModuleName_;
    this->provisionMetadatas_ = info.provisionMetadatas_;
    return *this;
}

InnerBundleInfo::~InnerBundleInfo()
{
    APP_LOGD("inner bundle info instance is destroyed");
}

void to_json(nlohmann::json &jsonObject, const Distro &distro)
{
    jsonObject = nlohmann::json {
            {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_DELIVERY_WITH_INSTALL, distro.deliveryWithInstall},
            {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_MODULE_NAME, distro.moduleName},
            {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_MODULE_TYPE, distro.moduleType},
            {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_MODULE_INSTALLATION_FREE, distro.installationFree}
    };
}

void to_json(nlohmann::json &jsonObject, const DefinePermission &definePermission)
{
    jsonObject = nlohmann::json {
        {Profile::DEFINEPERMISSION_NAME, definePermission.name},
        {Profile::DEFINEPERMISSION_GRANT_MODE, definePermission.grantMode},
        {Profile::DEFINEPERMISSION_AVAILABLE_LEVEL, definePermission.availableLevel},
        {Profile::DEFINEPERMISSION_PROVISION_ENABLE, definePermission.provisionEnable},
        {Profile::DEFINEPERMISSION_DISTRIBUTED_SCENE_ENABLE, definePermission.distributedSceneEnable},
        {Profile::LABEL, definePermission.label},
        {Profile::LABEL_ID, definePermission.labelId},
        {Profile::DESCRIPTION, definePermission.description},
        {Profile::DESCRIPTION_ID, definePermission.descriptionId}
    };
}

void to_json(nlohmann::json &jsonObject, const Dependency &dependency)
{
    jsonObject = nlohmann::json {
        {Profile::DEPENDENCIES_MODULE_NAME, dependency.moduleName},
        {Profile::DEPENDENCIES_BUNDLE_NAME, dependency.bundleName},
        {Profile::APP_VERSION_CODE, dependency.versionCode}
    };
}

void to_json(nlohmann::json &jsonObject, const InnerModuleInfo &info)
{
    jsonObject = nlohmann::json {
        {NAME, info.name},
        {MODULE_PACKAGE, info.modulePackage},
        {MODULE_NAME, info.moduleName},
        {MODULE_PATH, info.modulePath},
        {MODULE_DATA_DIR, info.moduleDataDir},
        {MODULE_RES_PATH, info.moduleResPath},
        {MODULE_IS_ENTRY, info.isEntry},
        {MODULE_METADATA, info.metaData},
        {MODULE_COLOR_MODE, info.colorMode},
        {MODULE_DISTRO, info.distro},
        {MODULE_DESCRIPTION, info.description},
        {MODULE_DESCRIPTION_ID, info.descriptionId},
        {MODULE_ICON, info.icon},
        {MODULE_ICON_ID, info.iconId},
        {MODULE_LABEL, info.label},
        {MODULE_LABEL_ID, info.labelId},
        {MODULE_DESCRIPTION_INSTALLATION_FREE, info.installationFree},
        {MODULE_IS_REMOVABLE, info.isRemovable},
        {MODULE_UPGRADE_FLAG, info.upgradeFlag},
        {MODULE_REQ_CAPABILITIES, info.reqCapabilities},
        {MODULE_ABILITY_KEYS, info.abilityKeys},
        {MODULE_SKILL_KEYS, info.skillKeys},
        {MODULE_MAIN_ABILITY, info.mainAbility},
        {MODULE_ENTRY_ABILITY_KEY, info.entryAbilityKey},
        {MODULE_SRC_PATH, info.srcPath},
        {MODULE_HASH_VALUE, info.hashValue},
        {MODULE_PROCESS, info.process},
        {MODULE_SRC_ENTRANCE, info.srcEntrance},
        {MODULE_DEVICE_TYPES, info.deviceTypes},
        {MODULE_VIRTUAL_MACHINE, info.virtualMachine},
        {MODULE_UI_SYNTAX, info.uiSyntax},
        {MODULE_PAGES, info.pages},
        {MODULE_META_DATA, info.metadata},
        {MODULE_REQUEST_PERMISSIONS, info.requestPermissions},
        {MODULE_DEFINE_PERMISSIONS, info.definePermissions},
        {MODULE_EXTENSION_KEYS, info.extensionKeys},
        {MODULE_EXTENSION_SKILL_KEYS, info.extensionSkillKeys},
        {MODULE_IS_MODULE_JSON, info.isModuleJson},
        {MODULE_IS_STAGE_BASED_MODEL, info.isStageBasedModel},
        {MODULE_DEPENDENCIES, info.dependencies},
        {MODULE_IS_LIB_ISOLATED, info.isLibIsolated},
        {MODULE_NATIVE_LIBRARY_PATH, info.nativeLibraryPath},
        {MODULE_CPU_ABI, info.cpuAbi},
        {MODULE_HAP_PATH, info.hapPath},
        {MODULE_COMPILE_MODE, info.compileMode},
        {MODULE_TARGET_MODULE_NAME, info.targetModuleName},
        {MODULE_TARGET_PRIORITY, info.targetPriority},
        {MODULE_OVERLAY_MODULE_INFO, info.overlayModuleInfo},
        {MODULE_ATOMIC_SERVICE_MODULE_TYPE, info.atomicServiceModuleType},
        {MODULE_PRELOADS, info.preloads},
        {MODULE_COMPATIBLE_POLICY, info.compatiblePolicy},
        {MODULE_VERSION_CODE, info.versionCode},
        {MODULE_VERSION_NAME, info.versionName},
        {MODULE_PROXY_DATAS, info.proxyDatas},
    };
}

void to_json(nlohmann::json &jsonObject, const SkillUri &uri)
{
    jsonObject = nlohmann::json {
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_SCHEME, uri.scheme},
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_HOST, uri.host},
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PORT, uri.port},
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PATH, uri.path},
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PATHSTARTWITH, uri.pathStartWith},
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PATHREGEX, uri.pathRegex},
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_TYPE, uri.type}
    };
}

void to_json(nlohmann::json &jsonObject, const Skill &skill)
{
    jsonObject = nlohmann::json {
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_ACTIONS, skill.actions},
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_ENTITIES, skill.entities},
        {ProfileReader::BUNDLE_MODULE_PROFILE_KEY_URIS, skill.uris}
    };
}

void to_json(nlohmann::json &jsonObject, const InstallMark &installMark)
{
    jsonObject = nlohmann::json {
        {ProfileReader::BUNDLE_INSTALL_MARK_BUNDLE, installMark.bundleName},
        {ProfileReader::BUNDLE_INSTALL_MARK_PACKAGE, installMark.packageName},
        {ProfileReader::BUNDLE_INSTALL_MARK_STATUS, installMark.status}
    };
}

void to_json(nlohmann::json &jsonObject, const SandboxAppPersistentInfo &sandboxPersistentInfo)
{
    jsonObject = nlohmann::json {
        {ProfileReader::BUNDLE_SANDBOX_PERSISTENT_ACCESS_TOKEN_ID, sandboxPersistentInfo.accessTokenId},
        {ProfileReader::BUNDLE_SANDBOX_PERSISTENT_APP_INDEX, sandboxPersistentInfo.appIndex},
        {ProfileReader::BUNDLE_SANDBOX_PERSISTENT_USER_ID, sandboxPersistentInfo.userId}
    };
}

void InnerBundleInfo::ToJson(nlohmann::json &jsonObject) const
{
    jsonObject[APP_TYPE] = appType_;
    jsonObject[BUNDLE_STATUS] = bundleStatus_;
    jsonObject[ALLOWED_ACLS] = allowedAcls_;
    jsonObject[BASE_APPLICATION_INFO] = *baseApplicationInfo_;
    jsonObject[BASE_BUNDLE_INFO] = *baseBundleInfo_;
    jsonObject[BASE_ABILITY_INFO] = baseAbilityInfos_;
    jsonObject[INNER_MODULE_INFO] = innerModuleInfos_;
    jsonObject[INNER_SHARED_MODULE_INFO] = innerSharedModuleInfos_;
    jsonObject[SKILL_INFOS] = skillInfos_;
    jsonObject[USER_ID] = userId_;
    jsonObject[APP_FEATURE] = appFeature_;
    jsonObject[MODULE_FORMS] = formInfos_;
    jsonObject[MODULE_SHORTCUT] = shortcutInfos_;
    jsonObject[MODULE_COMMON_EVENT] = commonEvents_;
    jsonObject[INSTALL_MARK] = mark_;
    jsonObject[INNER_BUNDLE_USER_INFOS] = innerBundleUserInfos_;
    jsonObject[BUNDLE_IS_NEW_VERSION] = isNewVersion_;
    jsonObject[BUNDLE_BASE_EXTENSION_INFOS] = baseExtensionInfos_;
    jsonObject[BUNDLE_EXTENSION_SKILL_INFOS] = extensionSkillInfos_;
    jsonObject[BUNDLE_PACK_INFO] = *bundlePackInfo_;
    jsonObject[APP_INDEX] = appIndex_;
    jsonObject[BUNDLE_IS_SANDBOX_APP] = isSandboxApp_;
    jsonObject[BUNDLE_SANDBOX_PERSISTENT_INFO] = sandboxPersistentInfo_;
    jsonObject[BUNDLE_HQF_INFOS] = hqfInfos_;
    jsonObject[OVERLAY_BUNDLE_INFO] = overlayBundleInfo_;
    jsonObject[OVERLAY_TYPE] = overlayType_;
    jsonObject[APPLY_QUICK_FIX_FREQUENCY] = applyQuickFixFrequency_;
    jsonObject[HAS_ATOMIC_SERVICE_CONFIG] = hasAtomicServiceConfig_;
    jsonObject[MAIN_ATOMIC_MODULE_NAME] = mainAtomicModuleName_;
}

void from_json(const nlohmann::json &jsonObject, InnerModuleInfo &info)
{
    // these are not required fields.
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        NAME,
        info.name,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_PACKAGE,
        info.modulePackage,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_NAME,
        info.moduleName,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_PATH,
        info.modulePath,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_DATA_DIR,
        info.moduleDataDir,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_HAP_PATH,
        info.hapPath,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_RES_PATH,
        info.moduleResPath,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        MODULE_IS_ENTRY,
        info.isEntry,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<MetaData>(jsonObject,
        jsonObjectEnd,
        MODULE_METADATA,
        info.metaData,
        JsonType::OBJECT,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<ModuleColorMode>(jsonObject,
        jsonObjectEnd,
        MODULE_COLOR_MODE,
        info.colorMode,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<Distro>(jsonObject,
        jsonObjectEnd,
        MODULE_DISTRO,
        info.distro,
        JsonType::OBJECT,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_DESCRIPTION,
        info.description,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        MODULE_DESCRIPTION_ID,
        info.descriptionId,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_ICON,
        info.icon,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        MODULE_ICON_ID,
        info.iconId,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_LABEL,
        info.label,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        MODULE_LABEL_ID,
        info.labelId,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_MAIN_ABILITY,
        info.mainAbility,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_ENTRY_ABILITY_KEY,
        info.entryAbilityKey,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_SRC_PATH,
        info.srcPath,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_HASH_VALUE,
        info.hashValue,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        MODULE_DESCRIPTION_INSTALLATION_FREE,
        info.installationFree,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, bool>>(jsonObject,
        jsonObjectEnd,
        MODULE_IS_REMOVABLE,
        info.isRemovable,
        JsonType::OBJECT,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        MODULE_UPGRADE_FLAG,
        info.upgradeFlag,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        MODULE_REQ_CAPABILITIES,
        info.reqCapabilities,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        MODULE_ABILITY_KEYS,
        info.abilityKeys,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        MODULE_SKILL_KEYS,
        info.skillKeys,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_PROCESS,
        info.process,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_SRC_ENTRANCE,
        info.srcEntrance,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        MODULE_DEVICE_TYPES,
        info.deviceTypes,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_VIRTUAL_MACHINE,
        info.virtualMachine,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_UI_SYNTAX,
        info.uiSyntax,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_PAGES,
        info.pages,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<Metadata>>(jsonObject,
        jsonObjectEnd,
        MODULE_META_DATA,
        info.metadata,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<std::vector<RequestPermission>>(jsonObject,
        jsonObjectEnd,
        MODULE_REQUEST_PERMISSIONS,
        info.requestPermissions,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<std::vector<DefinePermission>>(jsonObject,
        jsonObjectEnd,
        MODULE_DEFINE_PERMISSIONS,
        info.definePermissions,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        MODULE_EXTENSION_KEYS,
        info.extensionKeys,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        MODULE_EXTENSION_SKILL_KEYS,
        info.extensionSkillKeys,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        MODULE_IS_MODULE_JSON,
        info.isModuleJson,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        MODULE_IS_STAGE_BASED_MODEL,
        info.isStageBasedModel,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<Dependency>>(jsonObject,
        jsonObjectEnd,
        MODULE_DEPENDENCIES,
        info.dependencies,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_COMPILE_MODE,
        info.compileMode,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        MODULE_IS_LIB_ISOLATED,
        info.isLibIsolated,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_NATIVE_LIBRARY_PATH,
        info.nativeLibraryPath,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_CPU_ABI,
        info.cpuAbi,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_TARGET_MODULE_NAME,
        info.targetModuleName,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        MODULE_TARGET_PRIORITY,
        info.targetPriority,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<OverlayModuleInfo>>(jsonObject,
        jsonObjectEnd,
        MODULE_OVERLAY_MODULE_INFO,
        info.overlayModuleInfo,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<AtomicServiceModuleType>(jsonObject,
        jsonObjectEnd,
        MODULE_ATOMIC_SERVICE_MODULE_TYPE,
        info.atomicServiceModuleType,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        MODULE_PRELOADS,
        info.preloads,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<CompatiblePolicy>(jsonObject,
        jsonObjectEnd,
        MODULE_COMPATIBLE_POLICY,
        info.compatiblePolicy,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        MODULE_VERSION_CODE,
        info.versionCode,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MODULE_VERSION_NAME,
        info.versionName,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<ProxyData>>(jsonObject,
        jsonObjectEnd,
        MODULE_PROXY_DATAS,
        info.proxyDatas,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    if (Profile::parseResult != ERR_OK) {
        APP_LOGE("read InnerModuleInfo from database error, error code : %{public}d", Profile::parseResult);
    }
}

void from_json(const nlohmann::json &jsonObject, SkillUri &uri)
{
    // these are required fields.
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_SCHEME,
        uri.scheme,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    // these are not required fields.
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_HOST,
        uri.host,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PORT,
        uri.port,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PATH,
        uri.path,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PATHSTARTWITH,
        uri.pathStartWith,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PATHREGX,
        uri.pathRegex,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_PATHREGEX,
        uri.pathRegex,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_TYPE,
        uri.type,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
}

void from_json(const nlohmann::json &jsonObject, Skill &skill)
{
    // these are not required fields.
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_ACTIONS,
        skill.actions,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_ENTITIES,
        skill.entities,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<SkillUri>>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_URIS,
        skill.uris,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
}

void from_json(const nlohmann::json &jsonObject, Distro &distro)
{
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_DELIVERY_WITH_INSTALL,
        distro.deliveryWithInstall,
        JsonType::BOOLEAN,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_MODULE_NAME,
        distro.moduleName,
        JsonType::STRING,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_MODULE_TYPE,
        distro.moduleType,
        JsonType::STRING,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    // mustFlag decide by distro.moduleType
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_MODULE_PROFILE_KEY_MODULE_INSTALLATION_FREE,
        distro.installationFree,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
}

void from_json(const nlohmann::json &jsonObject, InstallMark &installMark)
{
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_INSTALL_MARK_BUNDLE,
        installMark.bundleName,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_INSTALL_MARK_PACKAGE,
        installMark.packageName,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_INSTALL_MARK_STATUS,
        installMark.status,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
}

void from_json(const nlohmann::json &jsonObject, SandboxAppPersistentInfo &sandboxPersistentInfo)
{
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_SANDBOX_PERSISTENT_ACCESS_TOKEN_ID,
        sandboxPersistentInfo.accessTokenId,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_SANDBOX_PERSISTENT_APP_INDEX,
        sandboxPersistentInfo.appIndex,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        ProfileReader::BUNDLE_SANDBOX_PERSISTENT_USER_ID,
        sandboxPersistentInfo.userId,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
}

void from_json(const nlohmann::json &jsonObject, DefinePermission &definePermission)
{
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        Profile::DEFINEPERMISSION_NAME,
        definePermission.name,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        Profile::DEFINEPERMISSION_GRANT_MODE,
        definePermission.grantMode,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        Profile::DEFINEPERMISSION_AVAILABLE_LEVEL,
        definePermission.availableLevel,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        Profile::DEFINEPERMISSION_PROVISION_ENABLE,
        definePermission.provisionEnable,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        Profile::DEFINEPERMISSION_DISTRIBUTED_SCENE_ENABLE,
        definePermission.distributedSceneEnable,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        Profile::LABEL,
        definePermission.label,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        Profile::LABEL_ID,
        definePermission.labelId,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        Profile::DESCRIPTION,
        definePermission.description,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        Profile::DESCRIPTION_ID,
        definePermission.descriptionId,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    if (Profile::parseResult != ERR_OK) {
        APP_LOGE("read DefinePermission from database error, error code : %{public}d", Profile::parseResult);
    }
}

void from_json(const nlohmann::json &jsonObject, Dependency &dependency)
{
    const auto &jsonObjectEnd = jsonObject.end();
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        Profile::DEPENDENCIES_MODULE_NAME,
        dependency.moduleName,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        Profile::DEPENDENCIES_BUNDLE_NAME,
        dependency.bundleName,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int>(jsonObject,
        jsonObjectEnd,
        Profile::APP_VERSION_CODE,
        dependency.versionCode,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
}

int32_t InnerBundleInfo::FromJson(const nlohmann::json &jsonObject)
{
    const auto &jsonObjectEnd = jsonObject.end();
    Profile::parseResult = ERR_OK;
    GetValueIfFindKey<Constants::AppType>(jsonObject,
        jsonObjectEnd,
        APP_TYPE,
        appType_,
        JsonType::NUMBER,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int>(jsonObject,
        jsonObjectEnd,
        UID,
        uid_,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int>(jsonObject,
        jsonObjectEnd,
        GID,
        gid_,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        ALLOWED_ACLS,
        allowedAcls_,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<BundleStatus>(jsonObject,
        jsonObjectEnd,
        BUNDLE_STATUS,
        bundleStatus_,
        JsonType::NUMBER,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<BundleInfo>(jsonObject,
        jsonObjectEnd,
        BASE_BUNDLE_INFO,
        *baseBundleInfo_,
        JsonType::OBJECT,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<ApplicationInfo>(jsonObject,
        jsonObjectEnd,
        BASE_APPLICATION_INFO,
        *baseApplicationInfo_,
        JsonType::OBJECT,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, AbilityInfo>>(jsonObject,
        jsonObjectEnd,
        BASE_ABILITY_INFO,
        baseAbilityInfos_,
        JsonType::OBJECT,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, InnerModuleInfo>>(jsonObject,
        jsonObjectEnd,
        INNER_MODULE_INFO,
        innerModuleInfos_,
        JsonType::OBJECT,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, std::vector<InnerModuleInfo>>>(jsonObject,
        jsonObjectEnd,
        INNER_SHARED_MODULE_INFO,
        innerSharedModuleInfos_,
        JsonType::OBJECT,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, std::vector<Skill>>>(jsonObject,
        jsonObjectEnd,
        SKILL_INFOS,
        skillInfos_,
        JsonType::OBJECT,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int>(jsonObject,
        jsonObjectEnd,
        USER_ID,
        userId_,
        JsonType::NUMBER,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        APP_FEATURE,
        appFeature_,
        JsonType::STRING,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, std::vector<FormInfo>>>(jsonObject,
        jsonObjectEnd,
        MODULE_FORMS,
        formInfos_,
        JsonType::OBJECT,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, ShortcutInfo>>(jsonObject,
        jsonObjectEnd,
        MODULE_SHORTCUT,
        shortcutInfos_,
        JsonType::OBJECT,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, CommonEventInfo>>(jsonObject,
        jsonObjectEnd,
        MODULE_COMMON_EVENT,
        commonEvents_,
        JsonType::OBJECT,
        true,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<InstallMark>(jsonObject,
        jsonObjectEnd,
        INSTALL_MARK,
        mark_,
        JsonType::OBJECT,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    int32_t isOldVersion = ERR_OK;
    GetValueIfFindKey<std::map<std::string, InnerBundleUserInfo>>(jsonObject,
        jsonObjectEnd,
        INNER_BUNDLE_USER_INFOS,
        innerBundleUserInfos_,
        JsonType::OBJECT,
        true,
        isOldVersion,
        ArrayType::NOT_ARRAY);
    if (Profile::parseResult == ERR_OK && isOldVersion == ERR_APPEXECFWK_PARSE_PROFILE_MISSING_PROP) {
        // To be compatible with the old database,
        // if the old data does not have bundleUserInfos,
        // the default user information needs to be constructed.
        BuildDefaultUserInfo();
    }
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        BUNDLE_IS_NEW_VERSION,
        isNewVersion_,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, ExtensionAbilityInfo>>(jsonObject,
        jsonObjectEnd,
        BUNDLE_BASE_EXTENSION_INFOS,
        baseExtensionInfos_,
        JsonType::OBJECT,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::map<std::string, std::vector<Skill>>>(jsonObject,
        jsonObjectEnd,
        BUNDLE_EXTENSION_SKILL_INFOS,
        extensionSkillInfos_,
        JsonType::OBJECT,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<BundlePackInfo>(jsonObject,
        jsonObjectEnd,
        BUNDLE_PACK_INFO,
        *bundlePackInfo_,
        JsonType::OBJECT,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int>(jsonObject,
        jsonObjectEnd,
        APP_INDEX,
        appIndex_,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        BUNDLE_IS_SANDBOX_APP,
        isSandboxApp_,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<SandboxAppPersistentInfo>>(jsonObject,
        jsonObjectEnd,
        BUNDLE_SANDBOX_PERSISTENT_INFO,
        sandboxPersistentInfo_,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<std::vector<HqfInfo>>(jsonObject,
        jsonObjectEnd,
        BUNDLE_HQF_INFOS,
        hqfInfos_,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<std::vector<OverlayBundleInfo>>(jsonObject,
        jsonObjectEnd,
        OVERLAY_BUNDLE_INFO,
        overlayBundleInfo_,
        JsonType::ARRAY,
        false,
        Profile::parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        OVERLAY_TYPE,
        overlayType_,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        APPLY_QUICK_FIX_FREQUENCY,
        applyQuickFixFrequency_,
        JsonType::NUMBER,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<bool>(jsonObject,
        jsonObjectEnd,
        HAS_ATOMIC_SERVICE_CONFIG,
        hasAtomicServiceConfig_,
        JsonType::BOOLEAN,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::string>(jsonObject,
        jsonObjectEnd,
        MAIN_ATOMIC_MODULE_NAME,
        mainAtomicModuleName_,
        JsonType::STRING,
        false,
        Profile::parseResult,
        ArrayType::NOT_ARRAY);
    int32_t ret = Profile::parseResult;
    Profile::parseResult = ERR_OK;
    if (ret != ERR_OK) {
        APP_LOGE("read InnerBundleInfo from database error, error code : %{public}d", ret);
    }
    return ret;
}

void InnerBundleInfo::BuildDefaultUserInfo()
{
    APP_LOGD("BuildDefaultUserInfo: bundleName: %{public}s.",
        baseApplicationInfo_->bundleName.c_str());
    InnerBundleUserInfo defaultInnerBundleUserInfo;
    defaultInnerBundleUserInfo.bundleUserInfo.userId = GetUserId();
    defaultInnerBundleUserInfo.uid = uid_;
    defaultInnerBundleUserInfo.gids.emplace_back(gid_);
    defaultInnerBundleUserInfo.installTime = baseBundleInfo_->installTime;
    defaultInnerBundleUserInfo.updateTime = baseBundleInfo_->updateTime;
    defaultInnerBundleUserInfo.bundleName = baseApplicationInfo_->bundleName;
    defaultInnerBundleUserInfo.bundleUserInfo.enabled = baseApplicationInfo_->enabled;
    AddInnerBundleUserInfo(defaultInnerBundleUserInfo);
}

std::optional<HapModuleInfo> InnerBundleInfo::FindHapModuleInfo(const std::string &modulePackage, int32_t userId) const
{
    auto it = innerModuleInfos_.find(modulePackage);
    if (it == innerModuleInfos_.end()) {
        APP_LOGE("can not find module %{public}s", modulePackage.c_str());
        return std::nullopt;
    }
    HapModuleInfo hapInfo;
    hapInfo.name = it->second.name;
    hapInfo.package = it->second.modulePackage;
    hapInfo.moduleName = it->second.moduleName;
    hapInfo.description = it->second.description;
    hapInfo.descriptionId = it->second.descriptionId;
    hapInfo.label = it->second.label;
    hapInfo.labelId = it->second.labelId;
    hapInfo.iconPath = it->second.icon;
    hapInfo.iconId = it->second.iconId;
    hapInfo.mainAbility = it->second.mainAbility;
    hapInfo.srcPath = it->second.srcPath;
    hapInfo.hapPath = it->second.hapPath;
    hapInfo.supportedModes = baseApplicationInfo_->supportedModes;
    hapInfo.reqCapabilities = it->second.reqCapabilities;
    hapInfo.colorMode = it->second.colorMode;
    hapInfo.isRemovable = it->second.isRemovable;
    hapInfo.upgradeFlag = it->second.upgradeFlag;
    hapInfo.isLibIsolated = it->second.isLibIsolated;
    hapInfo.nativeLibraryPath = it->second.nativeLibraryPath;
    hapInfo.cpuAbi = it->second.cpuAbi;

    hapInfo.bundleName = baseApplicationInfo_->bundleName;
    hapInfo.mainElementName = it->second.mainAbility;
    hapInfo.pages = it->second.pages;
    hapInfo.process = it->second.process;
    hapInfo.resourcePath = it->second.moduleResPath;
    hapInfo.srcEntrance = it->second.srcEntrance;
    hapInfo.uiSyntax = it->second.uiSyntax;
    hapInfo.virtualMachine = it->second.virtualMachine;
    hapInfo.deliveryWithInstall = it->second.distro.deliveryWithInstall;
    hapInfo.installationFree = it->second.distro.installationFree;
    hapInfo.isModuleJson = it->second.isModuleJson;
    hapInfo.isStageBasedModel = it->second.isStageBasedModel;
    std::string moduleType = it->second.distro.moduleType;
    if (moduleType == Profile::MODULE_TYPE_ENTRY) {
        hapInfo.moduleType = ModuleType::ENTRY;
    } else if (moduleType == Profile::MODULE_TYPE_FEATURE) {
        hapInfo.moduleType = ModuleType::FEATURE;
    } else if (moduleType == Profile::MODULE_TYPE_SHARED) {
        hapInfo.moduleType = ModuleType::SHARED;
    } else {
        hapInfo.moduleType = ModuleType::UNKNOWN;
    }
    std::string key;
    key.append(".").append(modulePackage).append(".");
    for (const auto &extension : baseExtensionInfos_) {
        if (extension.first.find(key) != std::string::npos) {
            hapInfo.extensionInfos.emplace_back(extension.second);
        }
    }
    hapInfo.metadata = it->second.metadata;
    bool first = false;
    for (auto &ability : baseAbilityInfos_) {
        if (ability.second.name == Constants::APP_DETAIL_ABILITY) {
            continue;
        }
        if (ability.first.find(key) != std::string::npos) {
            if (!first) {
                hapInfo.deviceTypes = ability.second.deviceTypes;
                first = true;
            }
            auto &abilityInfo = hapInfo.abilityInfos.emplace_back(ability.second);
            GetApplicationInfo(ApplicationFlag::GET_APPLICATION_INFO_WITH_PERMISSION |
                ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, userId,
                abilityInfo.applicationInfo);
        }
    }
    hapInfo.dependencies = it->second.dependencies;
    hapInfo.compileMode = ConvertCompileMode(it->second.compileMode);
    for (const auto &hqf : hqfInfos_) {
        if (hqf.moduleName == it->second.moduleName) {
            hapInfo.hqfInfo = hqf;
            break;
        }
    }
    hapInfo.atomicServiceModuleType = it->second.atomicServiceModuleType;
    for (const auto &item : it->second.preloads) {
        PreloadItem preload(item);
        hapInfo.preloads.emplace_back(preload);
    }
    return hapInfo;
}

std::optional<AbilityInfo> InnerBundleInfo::FindAbilityInfo(
    const std::string &moduleName,
    const std::string &abilityName,
    int32_t userId) const
{
    for (const auto &ability : baseAbilityInfos_) {
        auto abilityInfo = ability.second;
        if ((abilityInfo.name == abilityName) &&
            (moduleName.empty() || (abilityInfo.moduleName == moduleName))) {
            GetApplicationInfo(ApplicationFlag::GET_APPLICATION_INFO_WITH_PERMISSION |
                ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, userId,
                abilityInfo.applicationInfo);
            return abilityInfo;
        }
    }

    return std::nullopt;
}

std::optional<AbilityInfo> InnerBundleInfo::FindAbilityInfoV9(
    const std::string &moduleName, const std::string &abilityName) const
{
    for (const auto &ability : baseAbilityInfos_) {
        auto abilityInfo = ability.second;
        if ((abilityInfo.name == abilityName) &&
            (moduleName.empty() || (abilityInfo.moduleName == moduleName))) {
            return abilityInfo;
        }
    }
    return std::nullopt;
}

ErrCode InnerBundleInfo::FindAbilityInfo(
    const std::string &moduleName, const std::string &abilityName, AbilityInfo &info) const
{
    bool isModuleFind = false;
    for (const auto &ability : baseAbilityInfos_) {
        auto abilityInfo = ability.second;
        if ((abilityInfo.moduleName == moduleName)) {
            isModuleFind = true;
            if (abilityInfo.name == abilityName) {
                info = abilityInfo;
                return ERR_OK;
            }
        }
    }
    if (isModuleFind) {
        return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
    } else {
        return ERR_BUNDLE_MANAGER_MODULE_NOT_EXIST;
    }
}

std::optional<std::vector<AbilityInfo>> InnerBundleInfo::FindAbilityInfos(int32_t userId) const
{
    if (!HasInnerBundleUserInfo(userId)) {
        return std::nullopt;
    }

    std::vector<AbilityInfo> abilitys;
    for (const auto &ability : baseAbilityInfos_) {
        if (ability.second.name == Constants::APP_DETAIL_ABILITY) {
            continue;
        }
        auto abilityInfo = ability.second;
        GetApplicationInfo(ApplicationFlag::GET_APPLICATION_INFO_WITH_PERMISSION |
            ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, userId,
            abilityInfo.applicationInfo);
        abilitys.emplace_back(abilityInfo);
    }

    if (abilitys.empty()) {
        return std::nullopt;
    }

    return abilitys;
}

std::optional<ExtensionAbilityInfo> InnerBundleInfo::FindExtensionInfo(
    const std::string &moduleName, const std::string &extensionName) const
{
    for (const auto &extension : baseExtensionInfos_) {
        if ((extension.second.name == extensionName) &&
            (moduleName.empty() || (extension.second.moduleName == moduleName))) {
            return extension.second;
        }
    }

    return std::nullopt;
}

std::optional<std::vector<ExtensionAbilityInfo>> InnerBundleInfo::FindExtensionInfos() const
{
    std::vector<ExtensionAbilityInfo> extensions;
    for (const auto &extension : baseExtensionInfos_) {
        extensions.emplace_back(extension.second);
    }

    if (extensions.empty()) {
        return std::nullopt;
    }

    return extensions;
}

bool InnerBundleInfo::AddModuleInfo(const InnerBundleInfo &newInfo)
{
    if (newInfo.currentPackage_.empty()) {
        APP_LOGE("current package is empty");
        return false;
    }
    if (FindModule(newInfo.currentPackage_)) {
        APP_LOGE("current package %{public}s is exist", currentPackage_.c_str());
        return false;
    }
    AddInnerModuleInfo(newInfo.innerModuleInfos_);
    AddModuleAbilityInfo(newInfo.baseAbilityInfos_);
    AddModuleSkillInfo(newInfo.skillInfos_);
    AddModuleExtensionInfos(newInfo.baseExtensionInfos_);
    AddModuleExtensionSkillInfos(newInfo.extensionSkillInfos_);
    AddModuleFormInfo(newInfo.formInfos_);
    AddModuleShortcutInfo(newInfo.shortcutInfos_);
    AddModuleCommonEvent(newInfo.commonEvents_);
    return true;
}

void InnerBundleInfo::UpdateBaseBundleInfo(const BundleInfo &bundleInfo, bool isEntry)
{
    baseBundleInfo_->name = bundleInfo.name;

    baseBundleInfo_->versionCode = bundleInfo.versionCode;
    baseBundleInfo_->versionName = bundleInfo.versionName;
    baseBundleInfo_->minCompatibleVersionCode = bundleInfo.minCompatibleVersionCode;

    baseBundleInfo_->compatibleVersion = bundleInfo.compatibleVersion;
    baseBundleInfo_->targetVersion = bundleInfo.targetVersion;

    baseBundleInfo_->isKeepAlive = bundleInfo.isKeepAlive;
    baseBundleInfo_->singleton = bundleInfo.singleton;
    if (!baseBundleInfo_->isPreInstallApp) {
        baseBundleInfo_->isPreInstallApp = bundleInfo.isPreInstallApp;
    }

    baseBundleInfo_->vendor = bundleInfo.vendor;
    baseBundleInfo_->releaseType = bundleInfo.releaseType;
    if (!baseBundleInfo_->isNativeApp) {
        baseBundleInfo_->isNativeApp = bundleInfo.isNativeApp;
    }

    if (isEntry) {
        baseBundleInfo_->mainEntry = bundleInfo.mainEntry;
        baseBundleInfo_->entryModuleName = bundleInfo.entryModuleName;
    }
}

void InnerBundleInfo::UpdateBaseApplicationInfo(const ApplicationInfo &applicationInfo)
{
    baseApplicationInfo_->name = applicationInfo.name;
    baseApplicationInfo_->bundleName = applicationInfo.bundleName;

    baseApplicationInfo_->versionCode = applicationInfo.versionCode;
    baseApplicationInfo_->versionName = applicationInfo.versionName;
    baseApplicationInfo_->minCompatibleVersionCode = applicationInfo.minCompatibleVersionCode;

    baseApplicationInfo_->apiCompatibleVersion = applicationInfo.apiCompatibleVersion;
    baseApplicationInfo_->apiTargetVersion = applicationInfo.apiTargetVersion;

    baseApplicationInfo_->iconPath = applicationInfo.iconPath;
    baseApplicationInfo_->iconId = applicationInfo.iconId;
    baseApplicationInfo_->label = applicationInfo.label;
    baseApplicationInfo_->labelId = applicationInfo.labelId;
    baseApplicationInfo_->description = applicationInfo.description;
    baseApplicationInfo_->descriptionId = applicationInfo.descriptionId;
    baseApplicationInfo_->iconResource = applicationInfo.iconResource;
    baseApplicationInfo_->labelResource = applicationInfo.labelResource;
    baseApplicationInfo_->descriptionResource = applicationInfo.descriptionResource;
    baseApplicationInfo_->singleton = applicationInfo.singleton;
    baseApplicationInfo_->userDataClearable = applicationInfo.userDataClearable;
    baseApplicationInfo_->accessible = applicationInfo.accessible;

    if (!baseApplicationInfo_->isSystemApp) {
        baseApplicationInfo_->isSystemApp = applicationInfo.isSystemApp;
    }
    if (!baseApplicationInfo_->isLauncherApp) {
        baseApplicationInfo_->isLauncherApp = applicationInfo.isLauncherApp;
    }

    baseApplicationInfo_->apiReleaseType = applicationInfo.apiReleaseType;
    baseApplicationInfo_->debug = applicationInfo.debug;
    baseApplicationInfo_->deviceId = applicationInfo.deviceId;
    baseApplicationInfo_->distributedNotificationEnabled = applicationInfo.distributedNotificationEnabled;
    baseApplicationInfo_->entityType = applicationInfo.entityType;
    baseApplicationInfo_->process = applicationInfo.process;
    baseApplicationInfo_->supportedModes = applicationInfo.supportedModes;
    baseApplicationInfo_->vendor = applicationInfo.vendor;
    baseApplicationInfo_->appDistributionType = applicationInfo.appDistributionType;
    baseApplicationInfo_->appProvisionType = applicationInfo.appProvisionType;
    baseApplicationInfo_->formVisibleNotify = applicationInfo.formVisibleNotify;
    baseApplicationInfo_->needAppDetail = applicationInfo.needAppDetail;
    baseApplicationInfo_->appDetailAbilityLibraryPath = applicationInfo.appDetailAbilityLibraryPath;
    UpdatePrivilegeCapability(applicationInfo);
    SetHideDesktopIcon(applicationInfo.hideDesktopIcon);
#ifdef BUNDLE_FRAMEWORK_OVERLAY_INSTALLATION
    baseApplicationInfo_->targetBundleName = applicationInfo.targetBundleName;
    baseApplicationInfo_->targetPriority = applicationInfo.targetPriority;
#endif
}

void InnerBundleInfo::UpdateAppDetailAbilityAttrs()
{
    if (IsExistLauncherAbility()) {
        baseApplicationInfo_->needAppDetail = false;
        baseApplicationInfo_->appDetailAbilityLibraryPath = Constants::EMPTY_STRING;
    }
    for (auto iter = baseAbilityInfos_.begin(); iter != baseAbilityInfos_.end(); ++iter) {
        if (iter->second.name == Constants::APP_DETAIL_ABILITY) {
            if (!baseApplicationInfo_->needAppDetail) {
                baseAbilityInfos_.erase(iter);
                return;
            }
            if (isNewVersion_) {
                iter->second.labelId = baseApplicationInfo_->labelId;
                iter->second.iconId =
                    (baseApplicationInfo_->iconId == 0) ? iter->second.iconId : baseApplicationInfo_->iconId;
            }
            return;
        }
    }
}

bool InnerBundleInfo::IsHideDesktopIcon() const
{
    return baseApplicationInfo_->hideDesktopIcon ? true : !IsExistLauncherAbility();
}

bool InnerBundleInfo::IsExistLauncherAbility() const
{
    bool isExistLauncherAbility = false;
    OHOS::AAFwk::Want want;
    want.SetAction(OHOS::AAFwk::Want::ACTION_HOME);
    want.AddEntity(OHOS::AAFwk::Want::ENTITY_HOME);
    for (const auto& abilityInfoPair : baseAbilityInfos_) {
        auto skillsPair = skillInfos_.find(abilityInfoPair.first);
        if (skillsPair == skillInfos_.end()) {
            continue;
        }
        for (const Skill& skill : skillsPair->second) {
            if (skill.MatchLauncher(want) && (abilityInfoPair.second.type == AbilityType::PAGE)) {
                isExistLauncherAbility = true;
                break;
            }
        }
    }
    return isExistLauncherAbility;
}

void InnerBundleInfo::UpdateNativeLibAttrs(const ApplicationInfo &applicationInfo)
{
    baseApplicationInfo_->cpuAbi = applicationInfo.cpuAbi;
    baseApplicationInfo_->nativeLibraryPath = applicationInfo.nativeLibraryPath;
}

void InnerBundleInfo::UpdateArkNativeAttrs(const ApplicationInfo &applicationInfo)
{
    baseApplicationInfo_->arkNativeFileAbi = applicationInfo.arkNativeFileAbi;
    baseApplicationInfo_->arkNativeFilePath = applicationInfo.arkNativeFilePath;
}

void InnerBundleInfo::UpdatePrivilegeCapability(const ApplicationInfo &applicationInfo)
{
    SetKeepAlive(applicationInfo.keepAlive);
    baseApplicationInfo_->runningResourcesApply = applicationInfo.runningResourcesApply;
    baseApplicationInfo_->associatedWakeUp = applicationInfo.associatedWakeUp;
    SetAllowCommonEvent(applicationInfo.allowCommonEvent);
}

void InnerBundleInfo::UpdateRemovable(bool isPreInstall, bool removable)
{
#ifdef USE_PRE_BUNDLE_PROFILE
    if (!isPreInstall) {
        return;
    }
#endif

    baseApplicationInfo_->removable = removable;
}

void InnerBundleInfo::UpdateModuleInfo(const InnerBundleInfo &newInfo)
{
    if (newInfo.currentPackage_.empty()) {
        APP_LOGE("no package in new info");
        return;
    }

    RemoveModuleInfo(newInfo.currentPackage_);
    AddInnerModuleInfo(newInfo.innerModuleInfos_);
    AddModuleAbilityInfo(newInfo.baseAbilityInfos_);
    AddModuleSkillInfo(newInfo.skillInfos_);
    AddModuleExtensionInfos(newInfo.baseExtensionInfos_);
    AddModuleExtensionSkillInfos(newInfo.extensionSkillInfos_);
    AddModuleFormInfo(newInfo.formInfos_);
    AddModuleShortcutInfo(newInfo.shortcutInfos_);
    AddModuleCommonEvent(newInfo.commonEvents_);
}

bool InnerBundleInfo::GetMaxVerBaseSharedBundleInfo(const std::string &moduleName,
    BaseSharedBundleInfo &baseSharedBundleInfo) const
{
    auto it = innerSharedModuleInfos_.find(moduleName);
    if (it == innerSharedModuleInfos_.end()) {
        APP_LOGE("The shared module(%{public}s) infomation does not exist", moduleName.c_str());
        return false;
    }
    auto sharedModuleInfoVector = it->second;
    if (sharedModuleInfoVector.empty()) {
        APP_LOGE("No version exists for the shared module(%{public}s)", moduleName.c_str());
        return false;
    }
    InnerModuleInfo innerModuleInfo = sharedModuleInfoVector.front();
    if (innerModuleInfo.compatiblePolicy != CompatiblePolicy::BACK_COMPATIBLE &&
        innerModuleInfo.compatiblePolicy != CompatiblePolicy::PRECISE_MATCH) {
        APP_LOGE("GetMaxVerBaseSharedBundleInfo failed, compatiblePolicy is invalid!");
        return false;
    }
    baseSharedBundleInfo.bundleName = baseBundleInfo_->name;
    baseSharedBundleInfo.moduleName = innerModuleInfo.moduleName;
    baseSharedBundleInfo.versionCode = innerModuleInfo.versionCode;
    baseSharedBundleInfo.nativeLibraryPath = innerModuleInfo.nativeLibraryPath;
    return true;
}

bool InnerBundleInfo::GetBaseSharedBundleInfo(const std::string &moduleName, uint32_t versionCode,
    BaseSharedBundleInfo &baseSharedBundleInfo) const
{
    auto it = innerSharedModuleInfos_.find(moduleName);
    if (it == innerSharedModuleInfos_.end()) {
        APP_LOGE("The shared module(%{public}s) infomation does not exist", moduleName.c_str());
        return false;
    }
    auto sharedModuleInfoVector = it->second;
    if (sharedModuleInfoVector.empty()) {
        APP_LOGE("No version exists for the shared module(%{public}s)", moduleName.c_str());
        return false;
    }
    for (const auto &item : sharedModuleInfoVector) {
        if (item.compatiblePolicy != CompatiblePolicy::BACK_COMPATIBLE &&
            item.compatiblePolicy != CompatiblePolicy::PRECISE_MATCH) {
            APP_LOGE("GetBaseSharedBundleInfo failed, compatiblePolicy is invalid!");
            return false;
        }
        if (item.versionCode == versionCode) {
            baseSharedBundleInfo.bundleName = baseBundleInfo_->name;
            baseSharedBundleInfo.moduleName = item.moduleName;
            baseSharedBundleInfo.versionCode = item.versionCode;
            baseSharedBundleInfo.nativeLibraryPath = item.nativeLibraryPath;
            return true;
        }
    }
    APP_LOGE("GetBaseSharedBundleInfo failed, the version(%{public}d) is not exists for this module(%{public}s)",
        versionCode, moduleName.c_str());
    return false;
}

void InnerBundleInfo::InsertInnerSharedModuleInfo(const std::string &moduleName,
    const InnerModuleInfo &innerModuleInfo)
{
    auto iterator = innerSharedModuleInfos_.find(moduleName);
    if (iterator != innerSharedModuleInfos_.end()) {
        auto innerModuleInfoVector = iterator->second;
        bool insertFlag = false;
        for (unsigned long i = 0; i < innerModuleInfoVector.size(); i++) {
            if (innerModuleInfo.versionCode == innerModuleInfoVector.at(i).versionCode) {
                // if the inserted versionCode same as the existing one, replace old innerModuleInfo.
                innerModuleInfoVector.at(i) = innerModuleInfo;
                insertFlag = true;
                break;
            } else if (innerModuleInfo.versionCode > innerModuleInfoVector.at(i).versionCode) {
                // if the inserted versionCode bigger then the existing one, insert the specified location.
                innerModuleInfoVector.emplace(innerModuleInfoVector.begin() + i, innerModuleInfo);
                insertFlag = true;
                break;
            } else {
                continue;
            }
        }
        if (!insertFlag) {
            // insert innerModuleInfo in last location.
            innerModuleInfoVector.emplace(innerModuleInfoVector.end(), innerModuleInfo);
        }
        innerSharedModuleInfos_[moduleName] = innerModuleInfoVector;
    } else {
        std::vector<InnerModuleInfo> newInnerModuleInfoVector;
        newInnerModuleInfoVector.emplace_back(innerModuleInfo);
        innerSharedModuleInfos_.try_emplace(moduleName, newInnerModuleInfoVector);
    }
}

void InnerBundleInfo::SetSharedModuleNativeLibraryPath(const std::string &nativeLibraryPath)
{
    auto iterator = innerSharedModuleInfos_.find(currentPackage_);
    if (iterator == innerSharedModuleInfos_.end()) {
        APP_LOGE("The shared module(%{public}s) infomation does not exist", currentPackage_.c_str());
        return;
    }
    auto innerModuleInfoVector = iterator->second;
    for (auto iter = innerModuleInfoVector.begin(); iter != innerModuleInfoVector.end();) {
        if (iter->versionCode == innerModuleInfos_.at(currentPackage_).versionCode) {
            iter->nativeLibraryPath = nativeLibraryPath;
            break;
        }
    }
    innerSharedModuleInfos_[currentPackage_] = innerModuleInfoVector;
}

bool InnerBundleInfo::GetSharedBundleInfo(SharedBundleInfo &sharedBundleInfo) const
{
    sharedBundleInfo.name = GetBundleName();
    sharedBundleInfo.compatiblePolicy = GetBaseApplicationInfo().compatiblePolicy;
    std::vector<SharedModuleInfo> sharedModuleInfos;
    for (const auto &infoVector : innerSharedModuleInfos_) {
        for (const auto &info : infoVector.second) {
            SharedModuleInfo sharedModuleInfo;
            sharedModuleInfo.name = info.name;
            sharedModuleInfo.versionCode = info.versionCode;
            sharedModuleInfo.versionName = info.versionName;
            sharedModuleInfo.description = info.description;
            sharedModuleInfo.descriptionId = info.descriptionId;
            sharedModuleInfos.emplace_back(sharedModuleInfo);
        }
    }
    sharedBundleInfo.sharedModuleInfos = sharedModuleInfos;
    return true;
}

bool InnerBundleInfo::GetSharedDependencies(const std::string &moduleName,
    std::vector<Dependency> &dependencies) const
{
    if (innerModuleInfos_.find(moduleName) != innerModuleInfos_.end()) {
        dependencies = innerModuleInfos_.at(moduleName).dependencies;
        return true;
    }
    APP_LOGE("GetSharedDependencies can not find module %{public}s", moduleName.c_str());
    return false;
}

bool InnerBundleInfo::GetAllSharedDependencies(const std::string &moduleName,
    std::vector<Dependency> &dependencies) const
{
    if (!GetSharedDependencies(moduleName, dependencies)) {
        return false;
    }
    std::deque<Dependency> dependenciesDeque;
    std::copy(dependencies.begin(), dependencies.end(), std::back_inserter(dependenciesDeque));
    dependencies.clear();
    while (!dependenciesDeque.empty()) {
        bool isAdd = true;
        Dependency itemDependency = dependenciesDeque.front();
        dependenciesDeque.pop_front();
        for (const auto &item : dependencies) {
            if (item.bundleName == itemDependency.bundleName && item.moduleName == itemDependency.moduleName &&
                item.versionCode == itemDependency.versionCode) {
                isAdd = false;
                break;
            }
        }
        if (isAdd) {
            dependencies.push_back(itemDependency);
            std::vector<Dependency> tempDependencies;
            if (GetSharedDependencies(itemDependency.moduleName, tempDependencies)) {
                std::copy(tempDependencies.begin(), tempDependencies.end(), std::back_inserter(dependenciesDeque));
            }
        }
    }
    return true;
}

void InnerBundleInfo::RemoveModuleInfo(const std::string &modulePackage)
{
    auto it = innerModuleInfos_.find(modulePackage);
    if (it == innerModuleInfos_.end()) {
        APP_LOGE("The module(%{public}s) infomation does not exist", modulePackage.c_str());
        return;
    }

    auto oldModuleInfo = it->second;
    if (oldModuleInfo.isEntry) {
        baseBundleInfo_->mainEntry.clear();
        baseBundleInfo_->entryModuleName.clear();
    }
    innerModuleInfos_.erase(it);
    std::string key;
    key.append(".").append(modulePackage).append(".");
    for (auto iter = shortcutInfos_.begin(); iter != shortcutInfos_.end();) {
        if (iter->first.find(key) != std::string::npos) {
            shortcutInfos_.erase(iter++);
        } else {
            ++iter;
        }
    }

    for (auto iter = commonEvents_.begin(); iter != commonEvents_.end();) {
        if (iter->first.find(key) != std::string::npos) {
            commonEvents_.erase(iter++);
        } else {
            ++iter;
        }
    }

    // delete old abilityInfos
    for (auto abilityKey : oldModuleInfo.abilityKeys) {
        auto abilityItem = baseAbilityInfos_.find(abilityKey);
        if (abilityItem == baseAbilityInfos_.end()) {
            continue;
        }

        baseAbilityInfos_.erase(abilityItem);
        formInfos_.erase(abilityKey);
    }

    // delete old skillInfos
    for (auto skillKey : oldModuleInfo.skillKeys) {
        auto skillItem = skillInfos_.find(skillKey);
        if (skillItem == skillInfos_.end()) {
            continue;
        }

        skillInfos_.erase(skillItem);
    }

    // delete old extensionInfos
    for (auto extensionKey : oldModuleInfo.extensionKeys) {
        auto extensionItem = baseExtensionInfos_.find(extensionKey);
        if (extensionItem == baseExtensionInfos_.end()) {
            continue;
        }

        baseExtensionInfos_.erase(extensionItem);
    }

    // delete old extensionSkillInfos
    for (auto extensionSkillKey : oldModuleInfo.extensionSkillKeys) {
        auto extensionSkillItem = extensionSkillInfos_.find(extensionSkillKey);
        if (extensionSkillItem == extensionSkillInfos_.end()) {
            continue;
        }

        extensionSkillInfos_.erase(extensionSkillItem);
    }
}

std::string InnerBundleInfo::ToString() const
{
    nlohmann::json j;
    ToJson(j);
    return j.dump();
}

void InnerBundleInfo::GetApplicationInfo(int32_t flags, int32_t userId, ApplicationInfo &appInfo) const
{
    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetInnerBundleUserInfo(userId, innerBundleUserInfo)) {
        APP_LOGE("can not find userId %{public}d when get applicationInfo", userId);
        return;
    }

    appInfo = *baseApplicationInfo_;
    if (!CheckAppInstallControl(GetAppId(), userId)) {
        appInfo.removable = false;
    }
    if (!GetHasAtomicServiceConfig()) {
        std::vector<std::string> moduleNames;
        GetModuleNames(moduleNames);
        appInfo.split = moduleNames.size() != 1;
    }

    appInfo.accessTokenId = innerBundleUserInfo.accessTokenId;
    appInfo.accessTokenIdEx = innerBundleUserInfo.accessTokenIdEx;
    appInfo.enabled = innerBundleUserInfo.bundleUserInfo.enabled;
    appInfo.uid = innerBundleUserInfo.uid;

    for (const auto &info : innerModuleInfos_) {
        bool deCompress = info.second.hapPath.empty();
        ModuleInfo moduleInfo;
        moduleInfo.moduleName = info.second.moduleName;
        if (deCompress) {
            moduleInfo.moduleSourceDir = info.second.modulePath;
            appInfo.moduleSourceDirs.emplace_back(info.second.modulePath);
        }
        moduleInfo.preloads = info.second.preloads;
        appInfo.moduleInfos.emplace_back(moduleInfo);
        if (deCompress && info.second.isEntry) {
            appInfo.entryDir = info.second.modulePath;
        }
        if ((static_cast<uint32_t>(flags) & GET_APPLICATION_INFO_WITH_PERMISSION) ==
            GET_APPLICATION_INFO_WITH_PERMISSION) {
            for (const auto &item : info.second.requestPermissions) {
                appInfo.permissions.push_back(item.name);
            }
        }
        if ((static_cast<uint32_t>(flags) & GET_APPLICATION_INFO_WITH_METADATA) == GET_APPLICATION_INFO_WITH_METADATA) {
            bool isModuleJson = info.second.isModuleJson;
            if (!isModuleJson && info.second.metaData.customizeData.size() > 0) {
                appInfo.metaData[info.second.moduleName] = info.second.metaData.customizeData;
            }
            if (isModuleJson && info.second.metadata.size() > 0) {
                appInfo.metadata[info.second.moduleName] = info.second.metadata;
            }
        }
        if ((static_cast<uint32_t>(flags) & GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT) !=
            GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT) {
            appInfo.fingerprint.clear();
        }
    }
    if (!appInfo.permissions.empty()) {
        RemoveDuplicateName(appInfo.permissions);
    }
}

ErrCode InnerBundleInfo::GetApplicationInfoV9(int32_t flags, int32_t userId, ApplicationInfo &appInfo) const
{
    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetInnerBundleUserInfo(userId, innerBundleUserInfo)) {
        APP_LOGE("can not find userId %{public}d when get applicationInfo", userId);
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }

    appInfo = *baseApplicationInfo_;
    if (!CheckAppInstallControl(GetAppId(), userId)) {
        appInfo.removable = false;
    }

    appInfo.accessTokenId = innerBundleUserInfo.accessTokenId;
    appInfo.accessTokenIdEx = innerBundleUserInfo.accessTokenIdEx;
    appInfo.enabled = innerBundleUserInfo.bundleUserInfo.enabled;
    appInfo.uid = innerBundleUserInfo.uid;
    if (!GetHasAtomicServiceConfig()) {
        std::vector<std::string> moduleNames;
        GetModuleNames(moduleNames);
        appInfo.split = moduleNames.size() != 1;
    }

    for (const auto &info : innerModuleInfos_) {
        bool deCompress = info.second.hapPath.empty();
        ModuleInfo moduleInfo;
        moduleInfo.moduleName = info.second.moduleName;
        if (deCompress) {
            moduleInfo.moduleSourceDir = info.second.modulePath;
            appInfo.moduleSourceDirs.emplace_back(info.second.modulePath);
        }
        moduleInfo.preloads = info.second.preloads;
        appInfo.moduleInfos.emplace_back(moduleInfo);
        if (deCompress && info.second.isEntry) {
            appInfo.entryDir = info.second.modulePath;
        }
        if ((static_cast<uint32_t>(flags) &
            static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_WITH_PERMISSION)) ==
            static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_WITH_PERMISSION)) {
            for (const auto &item : info.second.requestPermissions) {
                appInfo.permissions.push_back(item.name);
            }
        }
        if ((static_cast<uint32_t>(flags) &
            static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_WITH_METADATA)) ==
            static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_WITH_METADATA)) {
            bool isModuleJson = info.second.isModuleJson;
            if (!isModuleJson && info.second.metaData.customizeData.size() > 0) {
                appInfo.metaData[info.second.moduleName] = info.second.metaData.customizeData;
            }
            if (isModuleJson && info.second.metadata.size() > 0) {
                appInfo.metadata[info.second.moduleName] = info.second.metadata;
            }
        }
    }
    if (!appInfo.permissions.empty()) {
        RemoveDuplicateName(appInfo.permissions);
    }
    return ERR_OK;
}

bool InnerBundleInfo::GetBundleInfo(int32_t flags, BundleInfo &bundleInfo, int32_t userId) const
{
    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetInnerBundleUserInfo(userId, innerBundleUserInfo)) {
        APP_LOGE("can not find userId %{public}d when GetBundleInfo", userId);
        return false;
    }

    bundleInfo = *baseBundleInfo_;

    bundleInfo.uid = innerBundleUserInfo.uid;
    if (!innerBundleUserInfo.gids.empty()) {
        bundleInfo.gid = innerBundleUserInfo.gids[0];
    }
    bundleInfo.installTime = innerBundleUserInfo.installTime;
    bundleInfo.updateTime = innerBundleUserInfo.updateTime;
    bundleInfo.appIndex = appIndex_;
    bundleInfo.overlayType = overlayType_;

    GetApplicationInfo(ApplicationFlag::GET_APPLICATION_INFO_WITH_CERTIFICATE_FINGERPRINT, userId,
        bundleInfo.applicationInfo);
    for (const auto &info : innerModuleInfos_) {
        if ((static_cast<uint32_t>(flags) & GET_BUNDLE_WITH_REQUESTED_PERMISSION)
            == GET_BUNDLE_WITH_REQUESTED_PERMISSION) {
            for (const auto &item : info.second.requestPermissions) {
                bundleInfo.reqPermissions.push_back(item.name);
            }
            for (const auto &item : info.second.definePermissions) {
                bundleInfo.defPermissions.push_back(item.name);
            }
        }
        bundleInfo.hapModuleNames.emplace_back(info.second.modulePackage);
        auto hapmoduleinfo = FindHapModuleInfo(info.second.modulePackage, userId);
        if (hapmoduleinfo) {
            GetModuleWithHashValue(flags, info.second.modulePackage, *hapmoduleinfo);
            bundleInfo.hapModuleInfos.emplace_back(*hapmoduleinfo);
            bundleInfo.moduleNames.emplace_back(info.second.moduleName);
            bundleInfo.moduleDirs.emplace_back(info.second.modulePath);
            bundleInfo.modulePublicDirs.emplace_back(info.second.moduleDataDir);
            bundleInfo.moduleResPaths.emplace_back(info.second.moduleResPath);
        } else {
            APP_LOGE("can not find hapmoduleinfo %{public}s", info.second.moduleName.c_str());
        }
    }
    if ((static_cast<uint32_t>(flags) & GET_BUNDLE_WITH_REQUESTED_PERMISSION)
        == GET_BUNDLE_WITH_REQUESTED_PERMISSION) {
        if (!bundleInfo.reqPermissions.empty()) {
            RemoveDuplicateName(bundleInfo.reqPermissions);
        }
        if (!bundleInfo.defPermissions.empty()) {
            RemoveDuplicateName(bundleInfo.defPermissions);
        }
        if (!BundlePermissionMgr::GetRequestPermissionStates(bundleInfo,
            bundleInfo.applicationInfo.accessTokenId, bundleInfo.applicationInfo.deviceId)) {
            APP_LOGE("get request permission state failed");
        }
        bundleInfo.reqPermissionDetails = GetAllRequestPermissions();
    }
    GetBundleWithAbilities(flags, bundleInfo, userId);
    GetBundleWithExtension(flags, bundleInfo, userId);
    return true;
}

ErrCode InnerBundleInfo::GetBundleInfoV9(int32_t flags, BundleInfo &bundleInfo, int32_t userId) const
{
    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetInnerBundleUserInfo(userId, innerBundleUserInfo)) {
        APP_LOGE("can not find userId %{public}d when GetBundleInfo", userId);
        return ERR_BUNDLE_MANAGER_INTERNAL_ERROR;
    }

    bundleInfo = *baseBundleInfo_;

    bundleInfo.uid = innerBundleUserInfo.uid;
    if (!innerBundleUserInfo.gids.empty()) {
        bundleInfo.gid = innerBundleUserInfo.gids[0];
    }
    bundleInfo.installTime = innerBundleUserInfo.installTime;
    bundleInfo.updateTime = innerBundleUserInfo.updateTime;
    bundleInfo.appIndex = appIndex_;

    for (const auto &info : innerModuleInfos_) {
        bundleInfo.hapModuleNames.emplace_back(info.second.modulePackage);
        auto hapmoduleinfo = FindHapModuleInfo(info.second.modulePackage, userId);
        if (hapmoduleinfo) {
            bundleInfo.moduleNames.emplace_back(info.second.moduleName);
            bundleInfo.moduleDirs.emplace_back(info.second.modulePath);
            bundleInfo.modulePublicDirs.emplace_back(info.second.moduleDataDir);
            bundleInfo.moduleResPaths.emplace_back(info.second.moduleResPath);
        } else {
            APP_LOGE("can not find hapmoduleinfo %{public}s", info.second.moduleName.c_str());
        }
    }
    ProcessBundleFlags(flags, userId, bundleInfo);
    return ERR_OK;
}

bool InnerBundleInfo::GetSharedBundleInfo(int32_t flags, BundleInfo &bundleInfo) const
{
    bundleInfo = *baseBundleInfo_;
    ProcessBundleWithHapModuleInfoFlag(flags, bundleInfo, Constants::ALL_USERID);
    bundleInfo.applicationInfo = *baseApplicationInfo_;
    return true;
}

void InnerBundleInfo::ProcessBundleFlags(
    int32_t flags, int32_t userId, BundleInfo &bundleInfo) const
{
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION))
        == static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION)) {
        if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_METADATA))
            == static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_METADATA)) {
            GetApplicationInfoV9(static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_WITH_METADATA), userId,
                bundleInfo.applicationInfo);
        } else {
            GetApplicationInfoV9(static_cast<int32_t>(GetApplicationFlag::GET_APPLICATION_INFO_DEFAULT), userId,
                bundleInfo.applicationInfo);
        }
    }
    GetBundleWithReqPermissionsV9(flags, userId, bundleInfo);
    ProcessBundleWithHapModuleInfoFlag(flags, bundleInfo, userId);
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_SIGNATURE_INFO))
        == static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_SIGNATURE_INFO)) {
        bundleInfo.signatureInfo.appId = baseBundleInfo_->appId;
        bundleInfo.signatureInfo.fingerprint = baseApplicationInfo_->fingerprint;
    }
}

void InnerBundleInfo::GetBundleWithReqPermissionsV9(int32_t flags, int32_t userId, BundleInfo &bundleInfo) const
{
    if ((static_cast<uint32_t>(flags) &
        static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_REQUESTED_PERMISSION))
        != static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_REQUESTED_PERMISSION)) {
        return;
    }
    for (const auto &info : innerModuleInfos_) {
        for (const auto &item : info.second.requestPermissions) {
            bundleInfo.reqPermissions.push_back(item.name);
        }
        for (const auto &item : info.second.definePermissions) {
            bundleInfo.defPermissions.push_back(item.name);
        }
    }
    if (!bundleInfo.reqPermissions.empty()) {
        RemoveDuplicateName(bundleInfo.reqPermissions);
    }
    if (!bundleInfo.defPermissions.empty()) {
        RemoveDuplicateName(bundleInfo.defPermissions);
    }
    InnerBundleUserInfo innerBundleUserInfo;
    if (!GetInnerBundleUserInfo(userId, innerBundleUserInfo)) {
        APP_LOGE("can not find userId %{public}d when get applicationInfo", userId);
        return;
    }
    uint32_t tokenId = innerBundleUserInfo.accessTokenId;
    std::string deviceId = baseApplicationInfo_->deviceId;
    if (!BundlePermissionMgr::GetRequestPermissionStates(bundleInfo, tokenId, deviceId)) {
        APP_LOGE("get request permission state failed");
    }
    bundleInfo.reqPermissionDetails = GetAllRequestPermissions();
}

void InnerBundleInfo::GetModuleWithHashValue(
    int32_t flags, const std::string &modulePackage, HapModuleInfo &hapModuleInfo) const
{
    if (!(static_cast<uint32_t>(flags) & GET_BUNDLE_WITH_HASH_VALUE)) {
        return;
    }

    auto it = innerModuleInfos_.find(modulePackage);
    if (it == innerModuleInfos_.end()) {
        APP_LOGE("can not find module %{public}s", modulePackage.c_str());
        return;
    }

    hapModuleInfo.hashValue = it->second.hashValue;
}

void InnerBundleInfo::ProcessBundleWithHapModuleInfoFlag(int32_t flags, BundleInfo &bundleInfo, int32_t userId) const
{
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE))
        != static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE)) {
        bundleInfo.hapModuleInfos.clear();
        return;
    }
    for (const auto &info : innerModuleInfos_) {
        auto hapmoduleinfo = FindHapModuleInfo(info.second.modulePackage, userId);
        if (hapmoduleinfo) {
            HapModuleInfo hapModuleInfo = *hapmoduleinfo;
            auto it = innerModuleInfos_.find(info.second.modulePackage);
            if (it == innerModuleInfos_.end()) {
                APP_LOGE("can not find module %{public}s", info.second.modulePackage.c_str());
            } else {
                hapModuleInfo.hashValue = it->second.hashValue;
            }
            if (hapModuleInfo.hapPath.empty()) {
                hapModuleInfo.moduleSourceDir = info.second.modulePath;
            }
            if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_METADATA))
                != static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_METADATA)) {
                hapModuleInfo.metadata.clear();
            }

            GetBundleWithAbilitiesV9(flags, hapModuleInfo, userId);
            GetBundleWithExtensionAbilitiesV9(flags, hapModuleInfo);
            bundleInfo.hapModuleInfos.emplace_back(hapModuleInfo);
        }
    }
}

void InnerBundleInfo::GetBundleWithAbilitiesV9(int32_t flags, HapModuleInfo &hapModuleInfo, int32_t userId) const
{
    hapModuleInfo.abilityInfos.clear();
    if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_ABILITY))
        != static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_ABILITY)) {
        return;
    }
    APP_LOGD("Get bundleInfo with abilities.");
    for (auto &ability : baseAbilityInfos_) {
        if ((ability.second.moduleName != hapModuleInfo.moduleName) ||
            (ability.second.name == Constants::APP_DETAIL_ABILITY)) {
            continue;
        }
        bool isEnabled = IsAbilityEnabled(ability.second, userId);
        if (!(static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_DISABLE))
            && !isEnabled) {
            APP_LOGW("%{public}s is disabled,", ability.second.name.c_str());
            continue;
        }
        AbilityInfo abilityInfo = ability.second;
        abilityInfo.enabled = isEnabled;

        if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_METADATA))
            != static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_METADATA)) {
            abilityInfo.metaData.customizeData.clear();
            abilityInfo.metadata.clear();
        }
        hapModuleInfo.abilityInfos.emplace_back(abilityInfo);
    }
}

void InnerBundleInfo::GetBundleWithExtensionAbilitiesV9(int32_t flags, HapModuleInfo &hapModuleInfo) const
{
    hapModuleInfo.extensionInfos.clear();
    if ((static_cast<uint32_t>(flags) &
        static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_EXTENSION_ABILITY))
        != static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_EXTENSION_ABILITY)) {
        return;
    }
    APP_LOGD("Get bundleInfo with extensionAbilities.");
    for (const auto &extensionInfo : baseExtensionInfos_) {
        if (extensionInfo.second.moduleName != hapModuleInfo.moduleName || !extensionInfo.second.enabled) {
            continue;
        }
        ExtensionAbilityInfo info = extensionInfo.second;

        if ((static_cast<uint32_t>(flags) & static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_METADATA))
            != static_cast<int32_t>(GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_METADATA)) {
            info.metadata.clear();
        }
        hapModuleInfo.extensionInfos.emplace_back(info);
    }
}

void InnerBundleInfo::GetBundleWithAbilities(int32_t flags, BundleInfo &bundleInfo, int32_t userId) const
{
    APP_LOGD("bundleName:%{public}s userid:%{public}d", bundleInfo.name.c_str(), userId);
    if (static_cast<uint32_t>(flags) & GET_BUNDLE_WITH_ABILITIES) {
        for (auto &ability : baseAbilityInfos_) {
            if (ability.second.name == Constants::APP_DETAIL_ABILITY) {
                continue;
            }
            bool isEnabled = IsAbilityEnabled(ability.second, userId);
            if (!(static_cast<uint32_t>(flags) & GET_ABILITY_INFO_WITH_DISABLE)
                && !isEnabled) {
                APP_LOGW("%{public}s is disabled,", ability.second.name.c_str());
                continue;
            }
            AbilityInfo abilityInfo = ability.second;
            abilityInfo.enabled = isEnabled;
            bundleInfo.abilityInfos.emplace_back(abilityInfo);
        }
    }
}

void InnerBundleInfo::GetBundleWithExtension(int32_t flags, BundleInfo &bundleInfo, int32_t userId) const
{
    APP_LOGD("get bundleInfo with extensionInfo begin");
    if ((static_cast<uint32_t>(flags) & GET_BUNDLE_WITH_EXTENSION_INFO) == GET_BUNDLE_WITH_EXTENSION_INFO) {
        for (const auto &extensionInfo : baseExtensionInfos_) {
            if (!extensionInfo.second.enabled) {
                continue;
            }
            ExtensionAbilityInfo info = extensionInfo.second;
            bundleInfo.extensionInfos.emplace_back(info);
        }
    }
    APP_LOGD("get bundleInfo with extensionInfo end");
}

bool InnerBundleInfo::CheckSpecialMetaData(const std::string &metaData) const
{
    if (isNewVersion_) {
        for (const auto &moduleInfo : innerModuleInfos_) {
            for (const auto &data : moduleInfo.second.metadata) {
                if (metaData == data.name) {
                    return true;
                }
            }
        }
        return false;
    }
    // old version
    for (const auto &moduleInfo : innerModuleInfos_) {
        for (const auto &data : moduleInfo.second.metaData.customizeData) {
            if (metaData == data.name) {
                return true;
            }
        }
    }
    return false;
}

void InnerBundleInfo::GetFormsInfoByModule(const std::string &moduleName, std::vector<FormInfo> &formInfos) const
{
    for (const auto &data : formInfos_) {
        for (auto &form : data.second) {
            if (form.moduleName == moduleName) {
                formInfos.emplace_back(form);
            }
        }
    }
}

void InnerBundleInfo::GetFormsInfoByApp(std::vector<FormInfo> &formInfos) const
{
    for (const auto &data : formInfos_) {
        std::copy(data.second.begin(), data.second.end(), std::back_inserter(formInfos));
    }
}

void InnerBundleInfo::GetShortcutInfos(std::vector<ShortcutInfo> &shortcutInfos) const
{
    if (isNewVersion_) {
        AbilityInfo abilityInfo;
        GetMainAbilityInfo(abilityInfo);
        if ((!abilityInfo.resourcePath.empty() || !abilityInfo.hapPath.empty())
            && abilityInfo.metadata.size() > 0) {
            std::vector<std::string> rawJson;
            BundleMgrClient bundleMgrClient;
            bool ret = bundleMgrClient.GetResConfigFile(abilityInfo, META_DATA_SHORTCUTS_NAME, rawJson);
            if (!ret) {
                APP_LOGD("GetResConfigFile return false");
                return;
            }
            if (rawJson.size() == 0) {
                APP_LOGD("rawJson size 0. skip.");
                return;
            }
            nlohmann::json jsonObject = nlohmann::json::parse(rawJson[0], nullptr, false);
            if (jsonObject.is_discarded()) {
                APP_LOGE("shortcuts json invalid");
                return;
            }
            ShortcutJson shortcutJson = jsonObject.get<ShortcutJson>();
            for (const Shortcut &item : shortcutJson.shortcuts) {
                ShortcutInfo shortcutInfo;
                shortcutInfo.id = item.shortcutId;
                shortcutInfo.bundleName = abilityInfo.bundleName;
                shortcutInfo.moduleName = abilityInfo.moduleName;
                shortcutInfo.icon = item.icon;
                shortcutInfo.label = item.label;
                shortcutInfo.iconId = item.iconId;
                shortcutInfo.labelId = item.labelId;
                for (const ShortcutWant &shortcutWant : item.wants) {
                    ShortcutIntent shortcutIntent;
                    shortcutIntent.targetBundle = shortcutWant.bundleName;
                    shortcutIntent.targetModule = shortcutWant.moduleName;
                    shortcutIntent.targetClass = shortcutWant.abilityName;
                    shortcutInfo.intents.emplace_back(shortcutIntent);
                }
                shortcutInfos.emplace_back(shortcutInfo);
            }
        }
        return;
    }
    for (const auto &shortcut : shortcutInfos_) {
        shortcutInfos.emplace_back(shortcut.second);
    }
}

void InnerBundleInfo::GetCommonEvents(const std::string &eventKey, std::vector<CommonEventInfo> &commonEvents) const
{
    CommonEventInfo item;
    for (const auto &commonEvent : commonEvents_) {
        for (const auto &event : commonEvent.second.events) {
            if (event == eventKey) {
                item = commonEvent.second;
                item.uid = GetUid(GetUserId());
                commonEvents.emplace_back(item);
                break;
            }
        }
    }
}

std::optional<InnerModuleInfo> InnerBundleInfo::GetInnerModuleInfoByModuleName(const std::string &moduleName) const
{
    for (const auto &innerModuleInfo : innerModuleInfos_) {
        APP_LOGD("info.moduleName = %{public}s, moduleName= %{public}s",
            innerModuleInfo.second.moduleName.c_str(), moduleName.c_str());
        if (innerModuleInfo.second.moduleName == moduleName) {
            return innerModuleInfo.second;
        }
    }
    return std::nullopt;
}

void InnerBundleInfo::GetModuleNames(std::vector<std::string> &moduleNames) const
{
    for (const auto &innerModuleInfo : innerModuleInfos_) {
        moduleNames.emplace_back(innerModuleInfo.second.moduleName);
    }
}

bool InnerBundleInfo::CheckAppInstallControl(const std::string &appId, int32_t userId) const
{
#ifdef BUNDLE_FRAMEWORK_APP_CONTROL
    std::vector<std::string> appIds;
    ErrCode ret = DelayedSingleton<AppControlManager>::GetInstance()->GetAppInstallControlRule(
        AppControlConstants::EDM_CALLING, AppControlConstants::APP_DISALLOWED_UNINSTALL, userId, appIds);
    if (ret != ERR_OK) {
        APP_LOGE("GetAppInstallControlRule failed code:%{public}d", ret);
        return true;
    }
    if (std::find(appIds.begin(), appIds.end(), appId) == appIds.end()) {
        return true;
    }
    APP_LOGW("appId is not removable");
    return false;
#else
    APP_LOGW("app control is disable");
    return true;
#endif
}

void InnerBundleInfo::ResetBundleState(int32_t userId)
{
    if (userId == Constants::ALL_USERID) {
        for (auto& innerBundleUserInfo : innerBundleUserInfos_) {
            innerBundleUserInfo.second.bundleUserInfo.Reset();
        }

        return;
    }

    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    if (innerBundleUserInfos_.find(key) == innerBundleUserInfos_.end()) {
        APP_LOGD("no this user %{public}s", key.c_str());
        return;
    }

    innerBundleUserInfos_.at(key).bundleUserInfo.Reset();
}

void InnerBundleInfo::RemoveInnerBundleUserInfo(int32_t userId)
{
    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        return;
    }

    auto result = innerBundleUserInfos_.erase(key);
    if (result == 0) {
        APP_LOGE("remove userId:%{public}d key:%{public}s info failed", userId, key.c_str());
    }
    for (auto &innerModuleInfo : innerModuleInfos_) {
        DeleteModuleRemovable(innerModuleInfo.second.moduleName, userId);
    }
}

void InnerBundleInfo::AddInnerBundleUserInfo(
    const InnerBundleUserInfo& innerBundleUserInfo)
{
    auto& key = NameAndUserIdToKey(
        GetBundleName(), innerBundleUserInfo.bundleUserInfo.userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        innerBundleUserInfos_.emplace(key, innerBundleUserInfo);
        return;
    }

    innerBundleUserInfos_[key] = innerBundleUserInfo;
}

bool InnerBundleInfo::GetInnerBundleUserInfo(
    int32_t userId, InnerBundleUserInfo& innerBundleUserInfo) const
{
    if (userId == Constants::NOT_EXIST_USERID) {
        return true;
    }

    if (userId == Constants::ALL_USERID) {
        if (innerBundleUserInfos_.empty()) {
            return false;
        }

        innerBundleUserInfo = innerBundleUserInfos_.begin()->second;
        return true;
    }

    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        return false;
    }

    innerBundleUserInfo = infoItem->second;
    return true;
}

bool InnerBundleInfo::HasInnerBundleUserInfo(int32_t userId) const
{
    if (userId == Constants::ALL_USERID || userId == Constants::ANY_USERID) {
        return !innerBundleUserInfos_.empty();
    }

    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    return infoItem != innerBundleUserInfos_.end();
}

void InnerBundleInfo::SetBundleInstallTime(const int64_t time, int32_t userId)
{
    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        return;
    }

    infoItem->second.installTime = time;
    infoItem->second.updateTime = time;
}

void InnerBundleInfo::SetAccessTokenId(uint32_t accessToken, const int32_t userId)
{
    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        return;
    }

    infoItem->second.accessTokenId = accessToken;
}

void InnerBundleInfo::SetAccessTokenIdEx(
    const Security::AccessToken::AccessTokenIDEx accessTokenIdEx,
    const int32_t userId)
{
    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        return;
    }

    infoItem->second.accessTokenId = accessTokenIdEx.tokenIdExStruct.tokenID;
    infoItem->second.accessTokenIdEx = accessTokenIdEx.tokenIDEx;
}

void InnerBundleInfo::SetBundleUpdateTime(const int64_t time, int32_t userId)
{
    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        return;
    }

    infoItem->second.updateTime = time;
}

bool InnerBundleInfo::IsAbilityEnabled(const AbilityInfo &abilityInfo, int32_t userId) const
{
    APP_LOGD("IsAbilityEnabled bundleName:%{public}s, userId:%{public}d", abilityInfo.bundleName.c_str(), userId);
    if (userId == Constants::NOT_EXIST_USERID) {
        return true;
    }
    auto& key = NameAndUserIdToKey(abilityInfo.bundleName, userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        APP_LOGE("innerBundleUserInfos find key:%{public}s, error", key.c_str());
        return false;
    }
    auto disabledAbilities = infoItem->second.bundleUserInfo.disabledAbilities;
    if (std::find(disabledAbilities.begin(), disabledAbilities.end(), abilityInfo.name) != disabledAbilities.end()) {
        return false;
    } else {
        return true;
    }
}

void InnerBundleInfo::SetOverlayModuleState(const std::string &moduleName, int32_t state, int32_t userId)
{
    APP_LOGD("start to set overlay moduleInfo state of module %{public}s", moduleName.c_str());
    if (overlayType_ == NON_OVERLAY_TYPE) {
        APP_LOGW("no overlay module");
        return;
    }
    for (auto &innerUserInfo : innerBundleUserInfos_) {
        if (innerUserInfo.second.bundleUserInfo.userId != userId) {
            continue;
        }

        auto &overlayStates = innerUserInfo.second.bundleUserInfo.overlayModulesState;
        bool isSetSucc = std::any_of(overlayStates.begin(), overlayStates.end(), [&moduleName, &state](auto &item) {
            if (item.find(moduleName + Constants::FILE_UNDERLINE) != std::string::npos) {
                item = moduleName + Constants::FILE_UNDERLINE + std::to_string(state);
                return true;
            }
            return false;
        });
        if (!isSetSucc) {
            APP_LOGD("no overlay module state info under user %{public}d", userId);
            overlayStates.emplace_back(moduleName + Constants::FILE_UNDERLINE + std::to_string(state));
        }
    }
}

void InnerBundleInfo::SetOverlayModuleState(const std::string &moduleName, int32_t state)
{
    APP_LOGD("start to set overlay moduleInfo state of module %{public}s", moduleName.c_str());
    if (overlayType_ == NON_OVERLAY_TYPE) {
        APP_LOGW("no overlay module");
        return;
    }
    for (auto &innerUserInfo : innerBundleUserInfos_) {
        auto &overlayStates = innerUserInfo.second.bundleUserInfo.overlayModulesState;
        bool isSetSucc = std::any_of(overlayStates.begin(), overlayStates.end(), [&moduleName, &state](auto &item) {
            if (item.find(moduleName + Constants::FILE_UNDERLINE) != std::string::npos) {
                item = moduleName + Constants::FILE_UNDERLINE + std::to_string(state);
                return true;
            }
            return false;
        });
        if (!isSetSucc) {
            overlayStates.emplace_back(moduleName + Constants::FILE_UNDERLINE + std::to_string(state));
        }
    }
}

bool InnerBundleInfo::GetOverlayModuleState(const std::string &moduleName, int32_t userId, int32_t &state) const
{
    APP_LOGD("start to get overlay state of moduleName:%{public}s, userId:%{public}d", moduleName.c_str(), userId);
    if (userId == Constants::NOT_EXIST_USERID) {
        APP_LOGE("invalid userId %{public}d", userId);
        return false;
    }
    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        APP_LOGE("no userInfo under userId %{public}d", userId);
        return false;
    }

    auto overlayModulesState = infoItem->second.bundleUserInfo.overlayModulesState;
    if (overlayModulesState.empty()) {
        APP_LOGE("no overlay module installed under userId %{public}d", userId);
        return false;
    }
    for (const auto &item : overlayModulesState) {
        auto pos = item.find(moduleName + Constants::FILE_UNDERLINE);
        if (pos == std::string::npos) {
            continue;
        }
        return OHOS::StrToInt(item.substr(moduleName.length() + 1), state);
    }
    APP_LOGE("no overlay module installed under userId %{public}d", userId);
    return false;
}

void InnerBundleInfo::ClearOverlayModuleStates(const std::string &moduleName)
{
    // delete overlay module state
    for (auto &innerUserInfo : innerBundleUserInfos_) {
        auto &overlayStates = innerUserInfo.second.bundleUserInfo.overlayModulesState;
        auto iter = std::find_if(overlayStates.begin(), overlayStates.end(), [&moduleName](const auto &item) {
            if (item.find(moduleName + Constants::FILE_UNDERLINE) != std::string::npos) {
                return true;
            }
            return false;
        });
        if (iter != overlayStates.end()) {
            overlayStates.erase(iter);
        }
    }
}

ErrCode InnerBundleInfo::IsAbilityEnabledV9(const AbilityInfo &abilityInfo, int32_t userId, bool &isEnable) const
{
    APP_LOGD("IsAbilityEnabled bundleName:%{public}s, userId:%{public}d", abilityInfo.bundleName.c_str(), userId);
    if (userId == Constants::NOT_EXIST_USERID) {
        isEnable = true;
        return ERR_OK;
    }
    auto& key = NameAndUserIdToKey(abilityInfo.bundleName, userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        APP_LOGE("innerBundleUserInfos find key:%{public}s, error", key.c_str());
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }
    auto disabledAbilities = infoItem->second.bundleUserInfo.disabledAbilities;
    if (std::find(disabledAbilities.begin(), disabledAbilities.end(), abilityInfo.name) != disabledAbilities.end()) {
        isEnable = false;
    } else {
        isEnable = true;
    }
    return ERR_OK;
}

ErrCode InnerBundleInfo::SetAbilityEnabled(
    const std::string &moduleName, const std::string &abilityName, bool isEnabled, int32_t userId)
{
    APP_LOGD("SetAbilityEnabled : %{public}s, %{public}s, %{public}d",
        moduleName.c_str(), abilityName.c_str(), userId);
    for (const auto &ability : baseAbilityInfos_) {
        if ((ability.second.name == abilityName) &&
            (moduleName.empty() || (ability.second.moduleName == moduleName))) {
            auto &key = NameAndUserIdToKey(GetBundleName(), userId);
            auto infoItem = innerBundleUserInfos_.find(key);
            if (infoItem == innerBundleUserInfos_.end()) {
                APP_LOGE("SetAbilityEnabled find innerBundleUserInfo failed");
                return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
            }

            auto iter = std::find(infoItem->second.bundleUserInfo.disabledAbilities.begin(),
                                  infoItem->second.bundleUserInfo.disabledAbilities.end(),
                                  abilityName);
            if (iter != infoItem->second.bundleUserInfo.disabledAbilities.end()) {
                if (isEnabled) {
                    infoItem->second.bundleUserInfo.disabledAbilities.erase(iter);
                }
            } else {
                if (!isEnabled) {
                    infoItem->second.bundleUserInfo.disabledAbilities.push_back(abilityName);
                }
            }
            return ERR_OK;
        }
    }
    APP_LOGE("SetAbilityEnabled find abilityInfo failed");
    return ERR_BUNDLE_MANAGER_ABILITY_NOT_EXIST;
}

void InnerBundleInfo::RemoveDuplicateName(std::vector<std::string> &name) const
{
    std::sort(name.begin(), name.end());
    auto iter = std::unique(name.begin(), name.end());
    name.erase(iter, name.end());
}

std::vector<DefinePermission> InnerBundleInfo::GetAllDefinePermissions() const
{
    std::vector<DefinePermission> definePermissions;
    for (const auto &info : innerModuleInfos_) {
        std::transform(info.second.definePermissions.begin(),
            info.second.definePermissions.end(),
            std::back_inserter(definePermissions),
            [](const auto &p) { return p; });
    }
    if (!definePermissions.empty()) {
        std::sort(definePermissions.begin(), definePermissions.end(),
            [](DefinePermission defPermA, DefinePermission defPermB) {
                return defPermA.name < defPermB.name;
            });
        auto iter = std::unique(definePermissions.begin(), definePermissions.end(),
            [](DefinePermission defPermA, DefinePermission defPermB) {
                return defPermA.name == defPermB.name;
            });
        definePermissions.erase(iter, definePermissions.end());
    }
    return definePermissions;
}

std::vector<RequestPermission> InnerBundleInfo::GetAllRequestPermissions() const
{
    std::vector<RequestPermission> requestPermissions;
    for (const auto &info : innerModuleInfos_) {
        for (const auto &item : info.second.requestPermissions) {
            requestPermissions.push_back(item);
        }
    }
    if (!requestPermissions.empty()) {
        std::sort(requestPermissions.begin(), requestPermissions.end(),
            [](RequestPermission reqPermA, RequestPermission reqPermB) {
                return reqPermA.name < reqPermB.name;
            });
        auto iter = std::unique(requestPermissions.begin(), requestPermissions.end(),
            [](RequestPermission reqPermA, RequestPermission reqPermB) {
                return reqPermA.name == reqPermB.name;
            });
        requestPermissions.erase(iter, requestPermissions.end());
    }
    return requestPermissions;
}

ErrCode InnerBundleInfo::SetApplicationEnabled(bool enabled, int32_t userId)
{
    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        APP_LOGE("SetApplicationEnabled can not find:%{public}s bundleUserInfo in userId: %{public}d",
            GetBundleName().c_str(), userId);
        return ERR_BUNDLE_MANAGER_BUNDLE_NOT_EXIST;
    }

    infoItem->second.bundleUserInfo.enabled = enabled;
    return ERR_OK;
}

const std::string &InnerBundleInfo::GetCurModuleName() const
{
    if (innerModuleInfos_.find(currentPackage_) != innerModuleInfos_.end()) {
        return innerModuleInfos_.at(currentPackage_).moduleName;
    }

    return Constants::EMPTY_STRING;
}

bool InnerBundleInfo::IsBundleRemovable() const
{
    if (IsPreInstallApp()) {
        APP_LOGE("PreInstallApp should not be cleaned");
        return false;
    }

    for (const auto &innerModuleInfo : innerModuleInfos_) {
        if (!innerModuleInfo.second.installationFree) {
            return false;
        }

        for (const auto &stateIter : innerModuleInfo.second.isRemovable) {
            if (!stateIter.second) {
                return false;
            }
        }
    }

    return true;
}

int64_t InnerBundleInfo::GetLastInstallationTime() const
{
    int64_t installTime = 0;
    for (const auto &innerBundleUserInfo : innerBundleUserInfos_) {
        installTime = innerBundleUserInfo.second.updateTime > installTime ?
            innerBundleUserInfo.second.updateTime : installTime;
    }

    return installTime;
}

bool InnerBundleInfo::GetRemovableModules(std::vector<std::string> &moduleToDelete) const
{
    for (const auto &innerModuleInfo : innerModuleInfos_) {
        if (!innerModuleInfo.second.installationFree) {
            continue;
        }

        bool canDelete = true;
        for (const auto &stateIter : innerModuleInfo.second.isRemovable) {
            if (!stateIter.second) {
                canDelete = false;
                break;
            }
        }

        if (canDelete) {
            moduleToDelete.emplace_back(innerModuleInfo.second.moduleName);
        }
    }

    return !moduleToDelete.empty();
}

bool InnerBundleInfo::GetFreeInstallModules(std::vector<std::string> &freeInstallModule) const
{
    for (const auto &innerModuleInfo : innerModuleInfos_) {
        if (!innerModuleInfo.second.installationFree) {
            continue;
        }

        freeInstallModule.emplace_back(innerModuleInfo.second.moduleName);
    }

    return !freeInstallModule.empty();
}

bool InnerBundleInfo::IsUserExistModule(const std::string &moduleName, int32_t userId) const
{
    APP_LOGD("userId:%{public}d moduleName:%{public}s", userId, moduleName.c_str());
    auto modInfoItem = GetInnerModuleInfoByModuleName(moduleName);
    if (!modInfoItem) {
        APP_LOGE("get InnerModuleInfo by moduleName(%{public}s) failed", moduleName.c_str());
        return false;
    }

    auto item = modInfoItem->isRemovable.find(std::to_string(userId));
    if (item == modInfoItem->isRemovable.end()) {
        APP_LOGE("userId:%{public}d has not moduleName:%{public}s", userId, moduleName.c_str());
        return false;
    }

    APP_LOGD("userId:%{public}d exist moduleName:%{public}s", userId, moduleName.c_str());
    return true;
}

ErrCode InnerBundleInfo::IsModuleRemovable(
    const std::string &moduleName, int32_t userId, bool &isRemovable) const
{
    APP_LOGD("userId:%{public}d moduleName:%{public}s", userId, moduleName.c_str());
    auto modInfoItem = GetInnerModuleInfoByModuleName(moduleName);
    if (!modInfoItem) {
        APP_LOGE("get InnerModuleInfo by moduleName(%{public}s) failed", moduleName.c_str());
        return ERR_BUNDLE_MANAGER_MODULE_NOT_EXIST;
    }

    auto item = modInfoItem->isRemovable.find(std::to_string(userId));
    if (item == modInfoItem->isRemovable.end()) {
        APP_LOGW("userId:%{public}d has not moduleName:%{public}s", userId, moduleName.c_str());
        isRemovable = false;
        return ERR_OK;
    }

    isRemovable = item->second;
    APP_LOGD("userId:%{public}d, moduleName:%{public}s, isRemovable:%{public}d,",
        userId, moduleName.c_str(), isRemovable);
    return ERR_OK;
}

bool InnerBundleInfo::AddModuleRemovableInfo(
    InnerModuleInfo &info, const std::string &stringUserId, bool isEnable) const
{
    auto item = info.isRemovable.find(stringUserId);
    if (item == info.isRemovable.end()) {
        auto result = info.isRemovable.try_emplace(stringUserId, isEnable);
        if (!result.second) {
            APP_LOGE("add userId:%{public}s isRemovable:%{public}d failed", stringUserId.c_str(), isEnable);
            return false;
        }

        APP_LOGD("add userId:%{public}s isRemovable:%{public}d into map", stringUserId.c_str(), isEnable);
        return true;
    }

    item->second = isEnable;
    APP_LOGD("set userId:%{public}s isEnable:%{public}d ok", stringUserId.c_str(), isEnable);
    return true;
}

bool InnerBundleInfo::SetModuleRemovable(const std::string &moduleName, bool isEnable, int32_t userId)
{
    std::string stringUserId = std::to_string(userId);
    APP_LOGD("userId:%{public}d moduleName:%{public}s isEnable:%{public}d", userId, moduleName.c_str(), isEnable);
    for (auto &innerModuleInfo : innerModuleInfos_) {
        if (innerModuleInfo.second.moduleName == moduleName) {
            return AddModuleRemovableInfo(innerModuleInfo.second, stringUserId, isEnable);
        }
    }

    return false;
}

void InnerBundleInfo::DeleteModuleRemovableInfo(InnerModuleInfo &info, const std::string &stringUserId)
{
    auto item = info.isRemovable.find(stringUserId);
    if (item == info.isRemovable.end()) {
        return;
    }

    info.isRemovable.erase(stringUserId);
}

void InnerBundleInfo::DeleteModuleRemovable(const std::string &moduleName, int32_t userId)
{
    std::string stringUserId = std::to_string(userId);
    APP_LOGD("userId:%{public}d moduleName:%{public}s", userId, moduleName.c_str());
    for (auto &innerModuleInfo : innerModuleInfos_) {
        if (innerModuleInfo.second.moduleName == moduleName) {
            DeleteModuleRemovableInfo(innerModuleInfo.second, stringUserId);
            return;
        }
    }
}

ErrCode InnerBundleInfo::SetModuleUpgradeFlag(std::string moduleName, int32_t upgradeFlag)
{
    APP_LOGD("moduleName= %{public}s, upgradeFlag = %{public}d", moduleName.c_str(), upgradeFlag);
    for (auto &innerModuleInfo : innerModuleInfos_) {
        if (innerModuleInfo.second.moduleName == moduleName) {
            innerModuleInfo.second.upgradeFlag = upgradeFlag;
            return ERR_OK;
        }
    }
    return ERR_BUNDLE_MANAGER_MODULE_NOT_EXIST;
}

int32_t InnerBundleInfo::GetModuleUpgradeFlag(std::string moduleName) const
{
    auto moduleInfo = GetInnerModuleInfoByModuleName(moduleName);
    if (!moduleInfo) {
        APP_LOGE("get InnerModuleInfo by moduleName(%{public}s) failed", moduleName.c_str());
        return UpgradeFlag::NOT_UPGRADE;
    }
    APP_LOGD("innerModuleInfo.upgradeFlag : %{public}d", moduleInfo->upgradeFlag);
    return moduleInfo->upgradeFlag;
}

int32_t InnerBundleInfo::GetResponseUserId(int32_t requestUserId) const
{
    if (innerBundleUserInfos_.empty()) {
        APP_LOGE("Get responseUserId failed due to user map is empty.");
        return Constants::INVALID_USERID;
    }

    if (requestUserId == Constants::ANY_USERID) {
        return innerBundleUserInfos_.begin()->second.bundleUserInfo.userId;
    }

    if (HasInnerBundleUserInfo(requestUserId)) {
        return requestUserId;
    }

    if (requestUserId < Constants::START_USERID) {
        APP_LOGD("requestUserId(%{public}d) less than start userId.", requestUserId);
        return Constants::INVALID_USERID;
    }

    int32_t responseUserId = Constants::INVALID_USERID;
    for (const auto &innerBundleUserInfo : innerBundleUserInfos_) {
        if (innerBundleUserInfo.second.bundleUserInfo.userId < Constants::START_USERID) {
            responseUserId = innerBundleUserInfo.second.bundleUserInfo.userId;
            break;
        }
    }

    APP_LOGD("requestUserId(%{public}d) and responseUserId(%{public}d).", requestUserId, responseUserId);
    return responseUserId;
}

void InnerBundleInfo::GetUriPrefixList(std::vector<std::string> &uriPrefixList,
    const std::string &excludeModule) const
{
    std::vector<std::string> uriList;
    for (const auto &abilityInfoPair : baseAbilityInfos_) {
        if (abilityInfoPair.second.uri.empty()) {
            continue;
        }
        if (!excludeModule.empty() && abilityInfoPair.first.find(excludeModule) == 0) {
            continue;
        }
        uriList.emplace_back(abilityInfoPair.second.uri);
    }
    for (const auto &extensionInfoPair : baseExtensionInfos_) {
        if (extensionInfoPair.second.uri.empty()) {
            continue;
        }
        if (!excludeModule.empty() && extensionInfoPair.first.find(excludeModule) == 0) {
            continue;
        }
        uriList.emplace_back(extensionInfoPair.second.uri);
    }
    for (const std::string &uri : uriList) {
        size_t schemePos = uri.find(Constants::URI_SEPARATOR);
        if (schemePos == uri.npos) {
            continue;
        }
        size_t cutPos = uri.find(Constants::SEPARATOR, schemePos + Constants::URI_SEPARATOR_LEN);
        std::string uriPrefix;
        if (cutPos == uri.npos) {
            uriPrefix = uri;
        } else {
            uriPrefix = uri.substr(0, cutPos);
        }
        uriPrefixList.emplace_back(uriPrefix);
    }
}

void InnerBundleInfo::GetUriPrefixList(std::vector<std::string> &uriPrefixList, int32_t userId,
    const std::string &excludeModule) const
{
    auto& key = NameAndUserIdToKey(GetBundleName(), userId);
    auto infoItem = innerBundleUserInfos_.find(key);
    if (infoItem == innerBundleUserInfos_.end()) {
        return;
    }
    GetUriPrefixList(uriPrefixList, excludeModule);
}

bool InnerBundleInfo::GetDependentModuleNames(const std::string &moduleName,
    std::vector<std::string> &dependentModuleNames) const
{
    for (auto iter = innerModuleInfos_.begin(); iter != innerModuleInfos_.end(); ++iter) {
        if (iter->second.moduleName == moduleName) {
            for (const auto &dependency : iter->second.dependencies) {
                dependentModuleNames.push_back(dependency.moduleName);
            }
            return true;
        }
    }
    APP_LOGE("GetDependentModuleNames can not find module %{public}s", moduleName.c_str());
    return false;
}

bool InnerBundleInfo::GetAllDependentModuleNames(const std::string &moduleName,
    std::vector<std::string> &dependentModuleNames) const
{
    if (!GetDependentModuleNames(moduleName, dependentModuleNames)) {
        return false;
    }
    std::deque<std::string> moduleDeque;
    std::copy(dependentModuleNames.begin(), dependentModuleNames.end(), std::back_inserter(moduleDeque));
    dependentModuleNames.clear();
    while (!moduleDeque.empty()) {
        std::string name = moduleDeque.front();
        moduleDeque.pop_front();
        if (std::find(dependentModuleNames.begin(), dependentModuleNames.end(), name) == dependentModuleNames.end()) {
            dependentModuleNames.push_back(name);
            std::vector<std::string> tempModuleNames;
            if (GetDependentModuleNames(name, tempModuleNames)) {
                std::copy(tempModuleNames.begin(), tempModuleNames.end(), std::back_inserter(moduleDeque));
            }
        }
    }
    return true;
}

std::string InnerBundleInfo::GetMainAbility() const
{
    AbilityInfo abilityInfo;
    GetMainAbilityInfo(abilityInfo);
    return abilityInfo.name;
}

void InnerBundleInfo::GetMainAbilityInfo(AbilityInfo &abilityInfo) const
{
    for (const auto& item : innerModuleInfos_) {
        const std::string& key = item.second.entryAbilityKey;
        if (!key.empty() && (baseAbilityInfos_.count(key) != 0)) {
            abilityInfo = baseAbilityInfos_.at(key);
            if (item.second.isEntry) {
                return;
            }
        }
    }
}

bool InnerBundleInfo::HasEntry() const
{
    return std::any_of(innerModuleInfos_.begin(), innerModuleInfos_.end(), [](const auto &item) {
            return item.second.isEntry;
        });
}

void InnerBundleInfo::SetAppDistributionType(const std::string &appDistributionType)
{
    baseApplicationInfo_->appDistributionType = appDistributionType;
}

std::string InnerBundleInfo::GetAppDistributionType() const
{
    return baseApplicationInfo_->appDistributionType;
}

void InnerBundleInfo::SetAppProvisionType(const std::string &appProvisionType)
{
    baseApplicationInfo_->appProvisionType = appProvisionType;
}

std::string InnerBundleInfo::GetAppProvisionType() const
{
    return baseApplicationInfo_->appProvisionType;
}

void InnerBundleInfo::SetAppCrowdtestDeadline(int64_t crowdtestDeadline)
{
    baseApplicationInfo_->crowdtestDeadline = crowdtestDeadline;
}

int64_t InnerBundleInfo::GetAppCrowdtestDeadline() const
{
    return baseApplicationInfo_->crowdtestDeadline;
}

std::vector<std::string> InnerBundleInfo::GetDistroModuleName() const
{
    std::vector<std::string> moduleVec;
    for (const auto &item : innerModuleInfos_) {
        moduleVec.push_back(item.second.moduleName);
    }
    return moduleVec;
}

std::string InnerBundleInfo::GetModuleNameByPackage(const std::string &packageName) const
{
    auto it = innerModuleInfos_.find(packageName);
    if (it == innerModuleInfos_.end()) {
        return Constants::EMPTY_STRING;
    }
    return it->second.moduleName;
}

std::string InnerBundleInfo::GetModuleTypeByPackage(const std::string &packageName) const
{
    auto it = innerModuleInfos_.find(packageName);
    if (it == innerModuleInfos_.end()) {
        return Constants::EMPTY_STRING;
    }
    return it->second.distro.moduleType;
}

AppQuickFix InnerBundleInfo::GetAppQuickFix() const
{
    return baseApplicationInfo_->appQuickFix;
}

void InnerBundleInfo::SetAppQuickFix(const AppQuickFix &appQuickFix)
{
    baseApplicationInfo_->appQuickFix = appQuickFix;
    if (appQuickFix.deployedAppqfInfo.hqfInfos.empty() && appQuickFix.deployingAppqfInfo.hqfInfos.empty()) {
        baseApplicationInfo_->appQuickFix.bundleName = Constants::EMPTY_STRING;
        baseApplicationInfo_->appQuickFix.versionCode = 0;
        baseApplicationInfo_->appQuickFix.versionName = Constants::EMPTY_STRING;
    }
    SetQuickFixHqfInfos(appQuickFix.deployedAppqfInfo.hqfInfos);
}

std::vector<HqfInfo> InnerBundleInfo::GetQuickFixHqfInfos() const
{
    return hqfInfos_;
}

void InnerBundleInfo::SetQuickFixHqfInfos(const std::vector<HqfInfo> &hqfInfos)
{
    hqfInfos_ = hqfInfos;
}

bool InnerBundleInfo::FetchNativeSoAttrs(
    const std::string &requestPackage, std::string &cpuAbi, std::string &nativeLibraryPath) const
{
    auto moduleIter = innerModuleInfos_.find(requestPackage);
    if (moduleIter == innerModuleInfos_.end()) {
        APP_LOGE("requestPackage(%{public}s) is not exist", requestPackage.c_str());
        return false;
    }

    auto &moduleInfo = moduleIter->second;
    if (moduleInfo.isLibIsolated) {
        cpuAbi = moduleInfo.cpuAbi;
        nativeLibraryPath = moduleInfo.nativeLibraryPath;
    } else {
        cpuAbi = baseApplicationInfo_->cpuAbi;
        nativeLibraryPath = baseApplicationInfo_->nativeLibraryPath;
    }

    return !nativeLibraryPath.empty();
}

bool InnerBundleInfo::IsLibIsolated(const std::string &moduleName) const
{
    auto moduleInfo = GetInnerModuleInfoByModuleName(moduleName);
    if (!moduleInfo) {
        APP_LOGE("Get moduleInfo(%{public}s) failed.", moduleName.c_str());
        return false;
    }

    return moduleInfo->isLibIsolated;
}

std::vector<std::string> InnerBundleInfo::GetDeviceType(const std::string &packageName) const
{
    auto it = innerModuleInfos_.find(packageName);
    if (it == innerModuleInfos_.end()) {
        APP_LOGW("%{public}s is not existed", packageName.c_str());
        return std::vector<std::string>();
    }
    return innerModuleInfos_.at(packageName).deviceTypes;
}

void InnerBundleInfo::AddApplyQuickFixFrequency()
{
    ++applyQuickFixFrequency_;
}

int32_t InnerBundleInfo::GetApplyQuickFixFrequency() const
{
    return applyQuickFixFrequency_;
}

void InnerBundleInfo::ResetApplyQuickFixFrequency()
{
    applyQuickFixFrequency_ = 0;
}

std::vector<uint32_t> InnerBundleInfo::GetAllHspVersion() const
{
    std::vector<uint32_t> versionCodes;
    for (const auto &[moduleName, modules] : innerSharedModuleInfos_) {
        for (const auto &module : modules) {
            if (std::find(versionCodes.begin(), versionCodes.end(), module.versionCode) == versionCodes.end()) {
                versionCodes.emplace_back(module.versionCode);
            }
        }
    }
    return versionCodes;
}

void InnerBundleInfo::DeleteHspModuleByVersion(int32_t versionCode)
{
    for (auto modulesIt = innerSharedModuleInfos_.begin(); modulesIt != innerSharedModuleInfos_.end();) {
        if (modulesIt->second.size() == SINGLE_HSP_VERSION &&
            modulesIt->second.front().versionCode == static_cast<uint32_t>(versionCode)) {
            modulesIt = innerSharedModuleInfos_.erase(modulesIt);
        } else {
            modulesIt->second.erase(
                std::remove_if(modulesIt->second.begin(), modulesIt->second.end(),
                    [versionCode] (InnerModuleInfo &module) {
                        return module.versionCode == static_cast<uint32_t>(versionCode);
                    }));
            ++modulesIt;
        }
    }
}
}  // namespace AppExecFwk
}  // namespace OHOS
