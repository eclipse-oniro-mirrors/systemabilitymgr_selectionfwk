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

#ifndef SELECTIONFWK_PANEL_INFO_H
#define SELECTIONFWK_PANEL_INFO_H

#include "parcel.h"

namespace OHOS {
namespace SelectionFwk {
enum class PanelType: uint32_t {
    MENU_PANEL = 1,
    MAIN_PANEL = 2,
};

struct PanelInfo {
    PanelType panelType{};
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

enum class ImmersiveMode : int32_t {
    NONE_IMMERSIVE = 0,
    IMMERSIVE = 1,
    LIGHT_IMMERSIVE = 2,
    DARK_IMMERSIVE = 3,
    END,
};
} // namespace SelectionFwk
} // namespace OHOS

#endif // SELECTIONFWK_PANEL_INFO_H
