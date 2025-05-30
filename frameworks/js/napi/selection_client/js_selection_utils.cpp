/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
    napi_status status = napi_get_named_property(env, in, "type", &propType);
    CHECK_RETURN((status == napi_ok), "no property type ", status);
    int32_t panelType = 0;
    status = JsUtils::GetValue(env, propType, panelType);
    CHECK_RETURN((status == napi_ok), "no value of type ", status);

    // PanelFlag is optional, defaults to FLG_FIXED when empty.
    int32_t panelFlag = static_cast<int32_t>(PanelFlag::FLG_FIXED);
    napi_value panelFlagObj = nullptr;
    status = napi_get_named_property(env, in, "flag", &panelFlagObj);
    if (status == napi_ok) {
        JsUtils::GetValue(env, panelFlagObj, panelFlag);
    }

    out.panelType = PanelType(panelType);
    out.panelFlag = PanelFlag(panelFlag);
    return napi_ok;
}
} // namespace SelectionFwk
} // namespace OHOS