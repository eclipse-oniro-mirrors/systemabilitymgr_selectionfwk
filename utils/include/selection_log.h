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

#ifndef SELETION_DEBUG_ENABLE
#define SELETION_DEBUG_ENABLE 0
#endif

#define FILENAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define HILOG_PRINT(func, fmt, args...)                                                                             \
    do {                                                                                                       \
        func(LOG_CORE, "{%{public}s:%{public}d %{public}s()} " fmt, FILENAME, __LINE__, __FUNCTION__, ##args); \
    } while (false)

#define SELECTION_HILOGD(fmt, ...) HILOG_PRINT(HILOG_DEBUG, fmt, ##__VA_ARGS__)
#define SELECTION_HILOGI(fmt, ...) HILOG_PRINT(HILOG_INFO, fmt, ##__VA_ARGS__)
#define SELECTION_HILOGW(fmt, ...) HILOG_PRINT(HILOG_WARN, fmt, ##__VA_ARGS__)
#define SELECTION_HILOGE(fmt, ...) HILOG_PRINT(HILOG_ERROR, fmt, ##__VA_ARGS__)
#define SELECTION_HILOGF(fmt, ...) HILOG_PRINT(HILOG_FATAL, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // SELECTION_LOG_H