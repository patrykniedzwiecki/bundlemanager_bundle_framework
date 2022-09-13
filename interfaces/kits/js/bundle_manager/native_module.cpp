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

#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

#include "app_log_wrapper.h"
#include "bundle_manager.h"

namespace OHOS {
namespace AppExecFwk {
static napi_value BundleManagerExport(napi_env env, napi_value exports)
{
    napi_value abilityFlag = nullptr;
    NAPI_CALL(env, napi_create_object(env, &abilityFlag));
    CreateAbilityFlagObject(env, abilityFlag);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("queryAbilityInfos", QueryAbilityInfos),
        DECLARE_NAPI_FUNCTION("getPermissionDef", GetPermissionDef),
        DECLARE_NAPI_PROPERTY("AbilityFlag", abilityFlag),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    APP_LOGI("init js bundle manager success.");
    return exports;
}

static napi_module bundle_manager_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = BundleManagerExport,
    .nm_modname = "bundle.bundleManager",
    .nm_priv = ((void *)0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void BUndleManagerRegister(void)
{
    napi_module_register(&bundle_manager_module);
}
}
}