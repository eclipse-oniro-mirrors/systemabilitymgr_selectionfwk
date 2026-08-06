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
#include "panel_status_listener.h"

namespace OHOS {
namespace SelectionFwk {
enum class SelectionWindowStatus : uint32_t {
    HIDDEN,
    DESTROYED,
    NONE
};
 
class SelectionPanel {
public:
    static constexpr uint32_t INVALID_WINDOW_ID = 0;
    SelectionPanel() = default;
    ~SelectionPanel();
    int32_t CreatePanel(const std::shared_ptr<AbilityRuntime::Context> &context, const PanelInfo &panelInfo);
    int32_t DestroyPanel();
    int32_t SetUiContent(const std::string &contentInfo, napi_env env);
    int32_t SetUiContent(const std::string &contentInfo, ani_env* env);
    int32_t ShowPanel();
    int32_t HidePanel();
    int32_t StartMoving();
    int32_t MoveTo(int32_t x, int32_t y);
    PanelType GetPanelType();
    bool SetPanelStatusListener(std::shared_ptr<PanelStatusListener> statusListener, const std::string &type);
    void ClearPanelListener(const std::string &type);
    uint32_t GetWindowId();
    bool IsPanelShowing();

private:
    std::string GeneratePanelName();
    int32_t SetPanelProperties();
    static uint32_t GenerateSequenceId();
    void PanelStatusChange(const SelectionWindowStatus &status);
    bool MarkListener(const std::string &type, bool isRegister);
    bool IsPanelListenerClearable();
    int32_t DoHidePanel(const sptr<OHOS::Rosen::Window>& window);
    bool IsShowing(const sptr<OHOS::Rosen::Window>& window);
    bool IsHidden(const sptr<OHOS::Rosen::Window>& window);
    bool IsDestroyed(const sptr<OHOS::Rosen::Window>& window);
    bool IsSelectionSystemAbilityExistent();
    sptr<OHOS::Rosen::Window> GetWindow() { return window_; };

private:
    inline static const std::unordered_map<SelectionWindowStatus, std::string> panelStatusMap_ {
        { SelectionWindowStatus::HIDDEN, "hidden" },
        { SelectionWindowStatus::DESTROYED, "destroyed" }
    };

    PanelType panelType_ = PanelType::MENU_PANEL;
    int32_t x_ = 0;
    int32_t y_ = 0;
    int32_t width_ = 0;
    int32_t height_ = 0;

    sptr<OHOS::Rosen::Window> window_;
    sptr<OHOS::Rosen::WindowOption> winOption_ = nullptr;
    bool isScbEnable_ { false };
    Rosen::KeyboardLayoutParams keyboardLayoutParams_;
    static std::atomic<uint32_t> sequenceId_;
    uint32_t invalidGravityPercent = 0;
    std::atomic<bool> isWaitSetUiContent_ { true };
    std::shared_ptr<PanelStatusListener> panelStatusListener_ = nullptr;
    bool destroyedRegistered_ = false;
    bool hiddenRegistered_ = false;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTION_PANEL_H