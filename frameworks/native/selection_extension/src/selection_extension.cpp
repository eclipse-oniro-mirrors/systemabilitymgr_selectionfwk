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

#include "selection_extension.h"
#include "js_selection_extension.h"
#include "ets_selection_extension.h"
#include "runtime.h"
#include "selection_log.h"

namespace OHOS::AbilityRuntime {
using namespace OHOS::AppExecFwk;

SelectionExtension* SelectionExtension::Create(const std::unique_ptr<Runtime>& runtime)
{
    if (runtime == nullptr) {
        return new SelectionExtension();
    }
    switch (runtime->GetLanguage()) {
        case Runtime::Language::JS:
            SELECTION_HILOGI("create JsSelectionExtension");
            return JsSelectionExtension::Create(runtime);
        case Runtime::Language::ETS:
            SELECTION_HILOGI("create EtsSelectionExtension");
            return EtsSelectionExtension::Create(runtime);
        default:
            return new SelectionExtension();
    }
}

void SelectionExtension::Init(const std::shared_ptr<AbilityLocalRecord>& record,
                              const std::shared_ptr<OHOSApplication>& application,
                              std::shared_ptr<AbilityHandler>& handler,
                              const sptr<IRemoteObject>& token)
{
    SELECTION_HILOGI("call %{public}s", __func__);
    ExtensionBase<SelectionExtensionContext>::Init(record, application, handler, token);
}

} // namespace OHOS::AbilityRuntime