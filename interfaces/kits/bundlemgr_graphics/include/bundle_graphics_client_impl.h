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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_GRAPHICS_INCLUDE_BUNDLE_GRAPHICS_CLIENT_IMPL_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_GRAPHICS_INCLUDE_BUNDLE_GRAPHICS_CLIENT_IMPL_H

#include <string>

#include "ability_info.h"
#include "bundle_mgr_interface.h"
#include "pixel_map.h"
#include "singleton.h"

namespace OHOS {
namespace AppExecFwk {
class BundleGraphicsClientImpl : public DelayedSingleton<BundleGraphicsClientImpl> {
public:
    BundleGraphicsClientImpl() = default;
    ~BundleGraphicsClientImpl() = default;

    std::shared_ptr<Media::PixelMap> GetAbilityPixelMapIcon(const std::string &bundleName,
        const std::string &moduleName, const std::string &abilityName);

private:
    sptr<IBundleMgr> GetBundleMgr();
    std::shared_ptr<Media::PixelMap> LoadImageFile(int32_t fd);
    int32_t CreateFileDescriptor(const std::string &filePath);

    OHOS::sptr<OHOS::AppExecFwk::IBundleMgr> bundleMgr_ = nullptr;
};
} // AppExecFwk
} // OHOS
#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_GRAPHICS_INCLUDE_BUNDLE_GRAPHICS_CLIENT_IMPL_H