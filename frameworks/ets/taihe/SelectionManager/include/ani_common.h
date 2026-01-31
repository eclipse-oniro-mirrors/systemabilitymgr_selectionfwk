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

#ifndef SELECTIONFWK_ETS_TAIHE_ANI_COMMON_H
#define SELECTIONFWK_ETS_TAIHE_ANI_COMMON_H

#include <string>
#include "ohos.selectionInput.selectionManager.proj.hpp"
#include "ohos.selectionInput.selectionManager.impl.hpp"
#include "taihe/runtime.hpp"
#include "selection_log.h"
#include "selection_js_utils.h"

namespace OHOS {
namespace SelectionFwk {

using UndefinedType_t = ::ohos::selectionInput::selectionManager::UndefinedType;
using callbackType = taihe::callback<void(UndefinedType_t const&)>;
using callbackTypePara = taihe::callback<void(const ::ohos::selectionInput::selectionManager::SelectionInfo &)>;

const std::string callbackType_SelectionComplete = "selectionComplete";
const std::string callbackType_Hide = "hidden";
const std::string callbackType_Destroy = "destroyed";

} // namespace SelectionFwk
} // namespace OHOS
#endif