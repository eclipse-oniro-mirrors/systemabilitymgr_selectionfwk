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

#include "callback_handler.h"

#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
constexpr size_t MAX_ARGV_COUNT = 10;
void JsCallbackHandler::Execute(const std::shared_ptr<JSCallbackObject> &object, const ArgContainer &argContainer,
    napi_value &output)
{
    if (object->threadId_ != std::this_thread::get_id()) {
        SELECTION_HILOGW("threadId not same!");
        return;
    }
    napi_value argv[MAX_ARGV_COUNT] = { nullptr };
    if (argContainer.argvProvider != nullptr && !argContainer.argvProvider(object->env_, argv, MAX_ARGV_COUNT)) {
        return;
    }
    napi_value callback = nullptr;
    napi_value global = nullptr;
    napi_get_reference_value(object->env_, object->callback_, &callback);
    if (callback == nullptr) {
        SELECTION_HILOGE("callback is nullptr!");
        return;
    }
    napi_get_global(object->env_, &global);
    SelectionMethodSyncTrace tracer("Execute napi_call_function");
    auto status = napi_call_function(object->env_, global, callback, argContainer.argc, argv, &output);
    if (status != napi_ok) {
        output = nullptr;
        SELECTION_HILOGE("napi_call_function is failed!");
        return;
    }
}
} // namespace SelectionFwk
} // namespace OHOS