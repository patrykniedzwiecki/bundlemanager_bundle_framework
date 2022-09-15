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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_ERRORS_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_ERRORS_H

#include "errors.h"

namespace OHOS {
namespace AppExecFwk {
constexpr ErrCode SUCCESS = 0;
constexpr ErrCode ERROR_PERMISSION_DENIED_ERROR = 201;
constexpr ErrCode ERROR_PARAM_CHECK_ERROR = 401;
constexpr ErrCode ERROR_SYSTEM_ABILITY_NOT_FOUND = 801;
constexpr ErrCode ERROR_BUNDLE_NOT_EXIST = 17700001;
constexpr ErrCode ERROR_ABILITY_NOT_EXIST = 17700003;
constexpr ErrCode ERROR_INVALID_USER_ID = 17700004;
constexpr ErrCode ERROR_INSTALL_PARSE_FAILED = 17700010;
constexpr ErrCode ERROR_INSTALL_VERIFY_SIGNATURE_FAILED = 17700011;
constexpr ErrCode ERROR_INSTALL_HAP_FILEPATH_INVALID = 17700012;
constexpr ErrCode ERROR_INSTALL_HAP_SIZE_TOO_LARGE = 17700013;
constexpr ErrCode ERROR_INSTALL_INCORRECT_SUFFIX = 17700014;
constexpr ErrCode ERROR_INSTALL_MULTIPLE_HAP_INFO_INCONSISTENT = 17700015;
constexpr ErrCode ERROR_INSTALL_NO_DISK_SPACE_LEFT = 17700016;
constexpr ErrCode ERROR_INSTALL_VERSION_DOWNGRADE = 17700017;
constexpr ErrCode ERROR_UNINSTALL_PREINSTALL_APP_FAILED = 17700020;
constexpr ErrCode ERROR_BUNDLE_SERVICE_EXCEPTION = 17700101;
constexpr ErrCode ERROR_SYSTEM_IO_OPERATION = 17700103;
constexpr ErrCode ERROR_OUT_OF_MEMORY_ERROR = 17700102;
}
}
#endif