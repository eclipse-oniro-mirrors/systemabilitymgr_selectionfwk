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

#ifdef HILOG_FATAL
#undef HILOG_FATAL
#endif

#ifdef HILOG_ERROR
#undef HILOG_ERROR
#endif

#ifdef HILOG_WARN
#undef HILOG_WARN
#endif

#ifdef HILOG_INFO
#undef HILOG_INFO
#endif

#ifdef HILOG_DEBUG
#undef HILOG_DEBUG
#endif

#ifndef SE_LOG_DOMAIN
#define SE_LOG_DOMAIN 0xD002902
#endif

#ifndef SE_LOG_TAG
#define SE_LOG_TAG "SELECTION_EXT"
#endif

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#define HILOG_FATAL(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_FATAL, SE_LOG_DOMAIN, SE_LOG_TAG, \
    "[%{public}s(%{public}s:%{public}d)]" fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define HILOG_ERROR(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_ERROR, SE_LOG_DOMAIN, SE_LOG_TAG, \
    "[%{public}s(%{public}s:%{public}d)]" fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define HILOG_WARN(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_WARN, SE_LOG_DOMAIN, SE_LOG_TAG, \
    "[%{public}s(%{public}s:%{public}d)]" fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define HILOG_INFO(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_INFO, SE_LOG_DOMAIN, SE_LOG_TAG, \
    "[%{public}s(%{public}s:%{public}d)]" fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define HILOG_DEBUG(fmt, ...)            \
    ((void)HILOG_IMPL(LOG_CORE, LOG_DEBUG, SE_LOG_DOMAIN, SE_LOG_TAG, \
    "[%{public}s(%{public}s:%{public}d)]" fmt, __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__))

#endif // SELECTION_LOG_H