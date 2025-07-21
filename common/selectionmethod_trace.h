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

#ifndef SELECTIONMETHOD_TRACE_H
#define SELECTIONMETHOD_TRACE_H

#include <string>

namespace OHOS {
namespace SelectionFwk {
void InitHiTrace();
void ValueTrace(const std::string &name, int64_t count);

void StartAsync(const std::string &value, int32_t taskId);
void FinishAsync(const std::string &value, int32_t taskId);

class SelectionMethodSyncTrace {
public:
    explicit SelectionMethodSyncTrace(const std::string &value);
    SelectionMethodSyncTrace(const std::string &value, const std::string &id);
    virtual ~SelectionMethodSyncTrace();
};

enum class TraceTaskId : int32_t {
    ONSTART_EXTENSION,
    ONSTART_MIDDLE_EXTENSION,
    ONCREATE_EXTENSION,
    ONCONNECT_EXTENSION,
    ONCONNECT_MIDDLE_EXTENSION,
    ON_KEY_EVENT,
    ON_FULL_KEY_EVENT,
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTIONMETHOD_TRACE_H