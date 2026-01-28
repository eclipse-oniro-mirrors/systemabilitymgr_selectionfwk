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

#include "ets_selection_extension_instance.h"

#include <cstddef>
#include <dlfcn.h>

#include "string_wrapper.h"

namespace OHOS {
namespace AbilityRuntime {
namespace {
//最好是将ets的代码和js的代码拆分为两个动态库，防止耦合，但是现在先实现功能，先暂时耦合在"libselection_extension_ability_native.z.so"中
const char *ETS_ANI_LIBNAME = "libselection_extension_ani.z.so";
const char *ETS_ANI_CREATE_FUNC = "OHOS_ETS_Selection_Extension_Create";
using CreateEtsSelectionExtensionFunc = SelectionExtension*(*)(const std::unique_ptr<Runtime>&);
CreateEtsSelectionExtensionFunc g_etsCreateFunc = nullptr;
}

AppServiceExtension *CreateEtsSelectionExtension(const std::unique_ptr<Runtime> &runtime)
{
    if (g_etsCreateFunc != nullptr) {
        return g_etsCreateFunc(runtime);
    }
    auto handle = dlopen(ETS_ANI_LIBNAME, RTLD_LAZY);
    if (handle == nullptr) {
        TAG_LOGE(AAFwkTag::APP_SERVICE_EXT, "dlopen failed %{public}s, %{public}s", ETS_ANI_LIBNAME, dlerror());
        return nullptr;
    }
    auto symbol = dlsym(handle, ETS_ANI_CREATE_FUNC);
    if (symbol == nullptr) {
        TAG_LOGE(AAFwkTag::APP_SERVICE_EXT, "dlsym failed %{public}s, %{public}s", ETS_ANI_CREATE_FUNC, dlerror());
        dlclose(handle);
        return nullptr;
    }
    g_etsCreateFunc = reinterpret_cast<CreateEtsSelectionExtensionFunc>(symbol);
    return g_etsCreateFunc(runtime);
}
} // namespace AbilityRuntime
} // namespace OHOS