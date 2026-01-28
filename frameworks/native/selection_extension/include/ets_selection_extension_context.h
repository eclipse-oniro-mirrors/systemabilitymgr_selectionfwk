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

#ifndef ETS_SELECTION_EXTENSION_CONTEXT_H
#define ETS_SELECTION_EXTENSION_CONTEXT_H

#include <memory>
#include "native_engine/native_engine.h"
#include "selection_extension_context.h"
#include "ani.h"

namespace OHOS::AbilityRuntime {
ani_object CreateEtsSelectionExtensionContext(ani_env *env, std::shared_ptr<SelectionExtensionContext> context);
}

#endif // JS_SELECTION_EXTENSION_CONTEXT_H
