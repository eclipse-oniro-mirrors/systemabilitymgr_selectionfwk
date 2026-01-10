/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SELECTION_LOG_H
#define SELECTION_LOG_H
#include "hilog/log.h"
#include <string>

#if defined(__cplusplus)
extern "C" {
#endif

#undef LOG_TAG
#define LOG_TAG "SELECTION_SERVICE"
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002901

#ifndef SELECTION_DEBUG_ENABLE
#define SELECTION_DEBUG_ENABLE 0
#endif

#define SELECTIONFWK_FILENAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#define SELECTION_HILOGF(fmt, ...)                                                                         \
    ((void)HILOG_IMPL(LOG_CORE, LOG_FATAL, LOG_DOMAIN, LOG_TAG, "[%{public}s(%{public}s:%{public}d)]" fmt, \
                      SELECTIONFWK_FILENAME, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define SELECTION_HILOGE(fmt, ...)                                                                         \
    ((void)HILOG_IMPL(LOG_CORE, LOG_ERROR, LOG_DOMAIN, LOG_TAG, "[%{public}s(%{public}s:%{public}d)]" fmt, \
                      SELECTIONFWK_FILENAME, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define SELECTION_HILOGW(fmt, ...)                                                                        \
    ((void)HILOG_IMPL(LOG_CORE, LOG_WARN, LOG_DOMAIN, LOG_TAG, "[%{public}s(%{public}s:%{public}d)]" fmt, \
                      SELECTIONFWK_FILENAME, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define SELECTION_HILOGI(fmt, ...)                                                                        \
    ((void)HILOG_IMPL(LOG_CORE, LOG_INFO, LOG_DOMAIN, LOG_TAG, "[%{public}s(%{public}s:%{public}d)]" fmt, \
                      SELECTIONFWK_FILENAME, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define SELECTION_HILOGD(fmt, ...)                                                                         \
    ((void)HILOG_IMPL(LOG_CORE, LOG_DEBUG, LOG_DOMAIN, LOG_TAG, "[%{public}s(%{public}s:%{public}d)]" fmt, \
                      SELECTIONFWK_FILENAME, __FUNCTION__, __LINE__, ##__VA_ARGS__))

#define SELECTION_CHECK_ONLY_LOG(ret, expected, log_msg, ...) \
    do { \
        if ((ret) != (expected)) { \
            SELECTION_HILOGE(log_msg, ##__VA_ARGS__); \
        } \
    } while (0)

#define SELECTION_CHECK(retCode, exper, fmt, ...) \
    do { \
        if (!(retCode)) { \
            SELECTION_HILOGE(fmt, ##__VA_ARGS__); \
            exper; \
        } \
    } while (0)

namespace ErrorCode {
// Error Code definition in the selection management system
enum {
    // no error
    NO_ERROR = 0, // no error
    ERROR_PARAMETER_CHECK_FAILED,

    ERROR_SELECTION_SERVICE,
    ERROR_PANEL_DESTROYED,
    ERROR_INVALID_OPERATION
};
}; // namespace ErrorCode

#ifdef __cplusplus
}
#endif

#endif // SELECTION_LOG_H