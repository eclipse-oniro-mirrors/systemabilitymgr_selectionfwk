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

#include "selection_extension_module_loader.h"
#include "selection_extension.h"

namespace OHOS::AbilityRuntime {

SelectionExtensionModuleLoader::SelectionExtensionModuleLoader() = default;
SelectionExtensionModuleLoader::~SelectionExtensionModuleLoader() = default;

Extension* SelectionExtensionModuleLoader::Create(const std::unique_ptr<Runtime>& runtime) const
{
    return SelectionExtension::Create(runtime);
}

std::map<std::string, std::string> SelectionExtensionModuleLoader::GetParams()
{
    // type means extension type in ExtensionAbilityType of extension_ability_info.h
    return {{"type", "46"}, {"name", "SelectionExtensionAbility"}};
}

extern "C" __attribute__((visibility("default"))) void* OHOS_EXTENSION_GetExtensionModule()
{
    return &SelectionExtensionModuleLoader::GetInstance();
}
} // namespace OHOS::AbilityRuntime