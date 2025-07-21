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

#include "js_selection_utils.h"
#include "panel_info.h"

namespace OHOS {
namespace SelectionFwk {
constexpr int32_t STR_MAX_LENGTH = 4096;
constexpr size_t STR_TAIL_LENGTH = 1;
constexpr size_t ARGC_MAX = 6;
constexpr size_t ARGC_ONE = 1;

/* napi_value <-> PanelInfo */
napi_status JsSelectionUtils::GetValue(napi_env env, napi_value in, PanelInfo &out)
{
    SELECTION_HILOGD("napi_value -> PanelInfo ");
    napi_value propType = nullptr;
    napi_status status = napi_get_named_property(env, in, "panelType", &propType);
    CHECK_RETURN((status == napi_ok), "no property panelType ", status);
    int32_t panelType = 0;
    status = JsUtils::GetValue(env, propType, panelType);
    CHECK_RETURN((status == napi_ok), "no value of panelType ", status);
    out.panelType = PanelType(panelType);

    bool ret = JsUtil::Object::ReadProperty(env, in, "x", out.x);
    ret = ret && JsUtil::Object::ReadProperty(env, in, "y", out.y);
    ret = ret && JsUtil::Object::ReadProperty(env, in, "width", out.width);
    ret = ret && JsUtil::Object::ReadProperty(env, in, "height", out.height);

    return ret ? napi_ok : napi_generic_failure;
}
} // namespace SelectionFwk
} // namespace OHOS