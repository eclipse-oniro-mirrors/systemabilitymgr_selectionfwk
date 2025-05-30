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

#ifndef SELECTION_PANEL_H
#define SELECTION_PANEL_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory> 
#include <string>
#include <atomic>

#include "panel_common.h"
#include "panel_info.h"
#include "refbase.h"
#include "context.h"
#include "wm_common.h"
#include "window.h"
#include "ui/rs_surface_node.h"
#include "selection_window_info.h"
#include "panel_status_listener.h"

namespace OHOS {
namespace SelectionFwk {
class SelectionPanel {
public:
    static constexpr uint32_t INVALID_WINDOW_ID = 0;
    SelectionPanel() = default;
    ~SelectionPanel();
    int32_t CreatePanel(const std::shared_ptr<AbilityRuntime::Context> &context, const PanelInfo &panelInfo);
    int32_t DestroyPanel();
    int32_t SetUiContent(const std::string &contentInfo, napi_env env);
    int32_t ShowPanel();
    int32_t HidePanel();
    int32_t StartMoving();
    int32_t MoveTo(int32_t x, int32_t y);
    PanelType GetPanelType();
    bool IsShowing();
    bool IsHidden();

    uint32_t windowId_ = INVALID_WINDOW_ID;

private:
    std::string GeneratePanelName();
    int32_t SetPanelProperties();
    static uint32_t GenerateSequenceId();
    void PanelStatusChange(const SelectionWindowStatus &status);

    PanelType panelType_ = PanelType::STATUS_BAR;
    PanelFlag panelFlag_ = PanelFlag::FLG_FIXED;
    sptr<OHOS::Rosen::Window> window_ = nullptr;
    sptr<OHOS::Rosen::WindowOption> winOption_ = nullptr;
    bool isScbEnable_ { false };
    Rosen::KeyboardLayoutParams keyboardLayoutParams_;
    static std::atomic<uint32_t> sequenceId_;
    uint32_t invalidGravityPercent = 0;
    std::atomic<bool> isWaitSetUiContent_ { true };
    std::shared_ptr<PanelStatusListener> panelStatusListener_ = nullptr;
    bool showRegistered_ = false;
    bool hideRegistered_ = false;

};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTION_PANEL_H