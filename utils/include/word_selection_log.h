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

#ifndef WORD_SELECTION_LOG_H
#define WORD_SELECTION_LOG_H
#include "hilog/log.h"
#include <string>

#if defined(__cplusplus)
extern "C" {
#endif

#undef LOG_TAG
#define LOG_TAG "WORD_SELETION"
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002901

#ifndef WORD_SELETION_DEBUG_ENABLE
#define WORD_SELETION_DEBUG_ENABLE 0
#endif

#define FILENAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define DEMO_LOG(func, fmt, args...)                                                                             \
    do {                                                                                                       \
        func(LOG_CORE, "{%{public}s:%{public}d %{public}s()} " fmt, FILENAME, __LINE__, __FUNCTION__, ##args); \
    } while (false)

#define LOG_DEBUG(fmt, ...) DEMO_LOG(HILOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) DEMO_LOG(HILOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) DEMO_LOG(HILOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) DEMO_LOG(HILOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) DEMO_LOG(HILOG_FATAL, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // WORD_SELECTION_LOG_H