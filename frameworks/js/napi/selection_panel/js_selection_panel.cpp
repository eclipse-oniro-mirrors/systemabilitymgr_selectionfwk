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

#include "js_selection_panel.h"

#include "js_runtime_utils.h"
#include "util.h"
#include "panel_info.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
napi_value JsSelectionPanel::Init(napi_env env, napi_value exports)
{
    napi_set_named_property(env, exports, "PanelType", GetJsPanelTypeProperty(env));
    return exports;
}

napi_value JsSelectionPanel::GetJsPanelTypeProperty(napi_env env)
{
    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));

    auto ret = JsUtil::Object::WriteProperty(env, obj, "MENU_PANEL", static_cast<int32_t>(PanelType::MENU_PANEL));
    if (!ret) {
        SELECTION_HILOGE("Failed to init module selectionInput.SelectionPanel.PanelType as MENU_PANEL");
        return nullptr;
    }
    ret = JsUtil::Object::WriteProperty(env, obj, "MAIN_PANEL", static_cast<int32_t>(PanelType::MAIN_PANEL));
    if (!ret) {
        SELECTION_HILOGE("Failed to init module selectionInput.SelectionPanel.PanelType as MAIN_PANEL");
        return nullptr;
    }
    return obj;
}
} // namespace MiscServices
} // namespace OHOS