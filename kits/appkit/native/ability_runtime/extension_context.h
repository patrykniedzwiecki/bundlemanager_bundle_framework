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

#ifndef EXTENSION_CONTEXT_H
#define EXTENSION_CONTEXT_H

#include "context_impl.h"
#include "iremote_object.h"

namespace OHOS {
namespace AbilityRuntime {
/**
 * @brief context supply for extension
 *
 */
class ExtensionContext : public ContextImpl {
public:
    ExtensionContext() = default;
    virtual ~ExtensionContext() = default;

    /**
     * @brief Set the token witch the app launched.
     *
     * @param token The token which the is launched by app.
     */
    void SetToken(const sptr<IRemoteObject> &token);

    /**
     * @brief Get the token witch the app launched.
     *
     * @return token The token which the is launched by app.
     */
    sptr<IRemoteObject> GetToken() const;

protected:
    sptr<IRemoteObject> token_;
private:
};
}  // namespace AbilityRuntime
}  // namespace OHOS
#endif  // EXTENSION_CONTEXT_H