/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "selection_panel.h"

#include <tuple>
#include <thread>
#include <chrono>

#include "display_manager.h"
#include "hisysevent_adapter.h"
#include "scene_board_judgement.h"
#include "selection_log.h"
#include "selectionmethod_trace.h"
#include "selection_system_ability_utils.h"

namespace OHOS {
namespace SelectionFwk {
using WMError = OHOS::Rosen::WMError;
using WindowState = OHOS::Rosen::WindowState;
using namespace Rosen;
using WindowGravity = OHOS::Rosen::WindowGravity;
using WindowState = OHOS::Rosen::WindowState;
std::atomic<uint32_t> SelectionPanel::sequenceId_ { 0 };
constexpr int32_t MAXWAITTIMES = 3;
constexpr int32_t WAITTIME = 100000;

static std::map<PanelType, std::string> windowTypeToName = {
    {PanelType::MAIN_PANEL, "SelectionMainFwkWindow"},
    {PanelType::MENU_PANEL, "SelectionMenuFwkWindow"}
};

SelectionPanel::~SelectionPanel() = default;

int32_t SelectionPanel::CreatePanel(
    const std::shared_ptr<AbilityRuntime::Context> &context, const PanelInfo &panelInfo)
{
    SELECTION_HILOGI("SelectionPanel CreatePanel start.");
    panelType_ = panelInfo.panelType;
    x_ = panelInfo.x;
    y_ = panelInfo.y;
    width_ = panelInfo.width;
    height_ = panelInfo.height;
    SELECTION_HILOGI("start , panelType:%{public}d.", static_cast<int32_t>(panelType_));

    sptr<Rosen::WindowOption> baseOp = new Rosen::WindowOption();
    baseOp->SetWindowType(Rosen::WindowType::WINDOW_TYPE_SELECTION);
    baseOp->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    OHOS::Rosen::Rect windowRect = {x_, y_, width_, height_};
    baseOp->SetWindowRect(windowRect);
    WMError wmError = WMError::WM_OK;
    auto windowName = windowTypeToName[panelInfo.panelType];
    sptr<Rosen::Window> window = Rosen::Window::Create(windowName, baseOp, context, wmError);
    SELECTION_HILOGI("Window creation returns %{public}d", wmError);
    if (!window) {
        HisyseventAdapter::GetInstance()->ReportShowPanelFailed(windowName, static_cast<int32_t>(wmError),
            static_cast<int32_t>(SelectFailedReason::CREATE_PANEL_FAILED));
        SELECTION_HILOGE("Faied to create window, errCode: %{public}d", wmError);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    window_ = window;
    SELECTION_HILOGI("Window::Create Success");
    return 0;
}

std::string SelectionPanel::GeneratePanelName()
{
    uint32_t sequenceId = GenerateSequenceId();
    std::string windowName = panelType_ == PanelType::MENU_PANEL ? "menuPanel" + std::to_string(sequenceId) :
                                                           "mainPanel" + std::to_string(sequenceId);
    SELECTION_HILOGD("SelectionPanel, windowName: %{public}s.", windowName.c_str());
    return windowName;
}

int32_t SelectionPanel::SetPanelProperties()
{
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    WindowGravity gravity = WindowGravity::WINDOW_GRAVITY_FLOAT;
    if (!isScbEnable_) {
        WMError wmError = window->SetWindowGravity(gravity, invalidGravityPercent);
        if (wmError != WMError::WM_OK) {
            SELECTION_HILOGE("failed to set window gravity, wmError is %{public}d, start destroy window!", wmError);
            return ErrorCode::ERROR_SELECTION_SERVICE;
        }
        return ErrorCode::NO_ERROR;
    }
    keyboardLayoutParams_.gravity_ = gravity;
    auto ret = window->AdjustKeyboardLayout(keyboardLayoutParams_);
    if (ret != WMError::WM_OK) {
        SELECTION_HILOGE("SetWindowGravity failed, wmError is %{public}d, start destroy window!", ret);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    return ErrorCode::NO_ERROR;
}

int32_t SelectionPanel::DestroyPanel()
{
    SELECTION_HILOGI("SelectionPanel DestroyPanel start");
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto ret = DoHidePanel(window);
    if (ret != ErrorCode::NO_ERROR) {
        SELECTION_HILOGE("SelectionPanel, hide panel failed, ret: %{public}d!", ret);
    }

    auto result = window->Destroy();
    if (result != WMError::WM_OK) {
        SELECTION_HILOGE("destroy failed. ret: %{public}d", result);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    SELECTION_HILOGI("destroy ret: %{public}d", result);
    PanelStatusChange(SelectionWindowStatus::DESTROYED);
    return ErrorCode::NO_ERROR;
}

uint32_t SelectionPanel::GenerateSequenceId()
{
    uint32_t seqId = sequenceId_.fetch_add(1, std::memory_order_seq_cst);
    return seqId % std::numeric_limits<uint32_t>::max();
}

int32_t SelectionPanel::SetUiContent(const std::string &contentInfo, napi_env env)
{
    SELECTION_HILOGI("SelectionPanel SetUiContent start");
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr, can not SetUiContent!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (IsDestroyed(window)) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (!IsSelectionSystemAbilityExistent()) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    WMError ret = window->NapiSetUIContent(contentInfo, env, nullptr);
    WMError wmError = window->SetTransparent(true);
    if (isWaitSetUiContent_.load()) {
        isWaitSetUiContent_.store(false);
    }
    SELECTION_HILOGI("SetTransparent ret: %{public}u.", wmError);
    SELECTION_HILOGI("NapiSetUIContent ret: %{public}d.", ret);
    return ret == WMError::WM_ERROR_INVALID_PARAM ? ErrorCode::ERROR_PARAMETER_CHECK_FAILED : ErrorCode::NO_ERROR;
}

int32_t SelectionPanel::SetUiContent(const std::string &contentInfo, ani_env* env)
{
    SELECTION_HILOGI("SelectionPanel SetUiContent start");
    if (env == nullptr) {
        SELECTION_HILOGE("env is nullptr, can not SetUiContent!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr, can not SetUiContent!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (IsDestroyed(window)) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (!IsSelectionSystemAbilityExistent()) {
        SELECTION_HILOGE("Selection SystemAbility is not existent");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    WMError ret = window->AniSetUIContent(contentInfo, env, nullptr);
    WMError wmError = window->SetTransparent(true);
    if (isWaitSetUiContent_.load()) {
        isWaitSetUiContent_.store(false);
    }
    SELECTION_HILOGI("SetTransparent ret: %{public}u.", wmError);
    SELECTION_HILOGI("AniSetUIContent ret: %{public}d.", ret);
    return ret == WMError::WM_ERROR_INVALID_PARAM ? ErrorCode::ERROR_PARAMETER_CHECK_FAILED : ErrorCode::NO_ERROR;
}

PanelType SelectionPanel::GetPanelType()
{
    return panelType_;
}

int32_t SelectionPanel::ShowPanel()
{
    SELECTION_HILOGI("SelectionPanel ShowPanel start.");
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (IsDestroyed(window)) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (!IsSelectionSystemAbilityExistent()) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    int32_t iCounter = 0;
    while (isWaitSetUiContent_.load() && iCounter < MAXWAITTIMES) {
        usleep(WAITTIME);
        iCounter++;
        SELECTION_HILOGI("SelectionPanel show pannel waitTime %{public}d.", iCounter);
    }

    if (isWaitSetUiContent_.load()) {
        SELECTION_HILOGE("isWaitSetUiContent_ is true!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    if (IsShowing(window)) {
        SELECTION_HILOGI("panel already shown.");
        return ErrorCode::NO_ERROR;
    }
    auto ret = WMError::WM_OK;
    {
        SelectionMethodSyncTrace tracer("SelectionPanel_ShowPanel");
        ret = window->Show(0, false, false);
    }
    if (ret != WMError::WM_OK) {
        SELECTION_HILOGE("ShowPanel error, err = %{public}d", ret);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    SELECTION_HILOGI("Selection panel shown successfully.");
    return ErrorCode::NO_ERROR;
}

bool SelectionPanel::IsShowing(const sptr<OHOS::Rosen::Window>& window)
{
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return false;
    }
    auto windowState = window->GetWindowState();
    if (windowState == WindowState::STATE_SHOWN) {
        return true;
    }
    SELECTION_HILOGD("windowState: %{public}d.", static_cast<int>(windowState));
    return false;
}

bool SelectionPanel::IsPanelShowing()
{
    return IsShowing(GetWindow());
}

void SelectionPanel::PanelStatusChange(const SelectionWindowStatus &status)
{
    if (panelStatusListener_ == nullptr) {
        SELECTION_HILOGE("panelStatusListener_ is nullptr.");
        return;
    }

    auto itr = panelStatusMap_.find(status);
    if (itr == panelStatusMap_.end()) {
        SELECTION_HILOGE("wrong status.");
        return;
    }

    uint32_t windowId = GetWindowId();
    if (status == SelectionWindowStatus::HIDDEN && hiddenRegistered_) {
        panelStatusListener_->OnPanelStatus(windowId, itr->second);
    }
    if (status == SelectionWindowStatus::DESTROYED && destroyedRegistered_) {
        panelStatusListener_->OnPanelStatus(windowId, itr->second);
    }
}

int32_t SelectionPanel::HidePanel()
{
    SELECTION_HILOGI("SelectionPanel HidePanel start");
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    return DoHidePanel(window);
}

int32_t SelectionPanel::DoHidePanel(const sptr<OHOS::Rosen::Window>& window)
{
    SELECTION_HILOGI("SelectionPanel DoHidePanel start");
    if (IsDestroyed(window)) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (IsHidden(window)) {
        SELECTION_HILOGI("panel already hidden.");
        return ErrorCode::NO_ERROR;
    }
    if (!IsSelectionSystemAbilityExistent()) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    auto ret = WMError::WM_OK;
    {
        SelectionMethodSyncTrace tracer("SelectionPanel_HidePanel");
        ret = window->Hide();
    }
    if (ret != WMError::WM_OK) {
        SELECTION_HILOGE("HidePanel error, err: %{public}d!", ret);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    SELECTION_HILOGI("success, panelType:%{public}d.", static_cast<int32_t>(panelType_));
    PanelStatusChange(SelectionWindowStatus::HIDDEN);
    return ErrorCode::NO_ERROR;
}

bool SelectionPanel::IsHidden(const sptr<OHOS::Rosen::Window>& window)
{
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return true;
    }
    auto windowState = window->GetWindowState();
    if (windowState == WindowState::STATE_HIDDEN) {
        return true;
    }
    SELECTION_HILOGD("windowState: %{public}d.", static_cast<int>(windowState));
    return false;
}

bool SelectionPanel::IsDestroyed(const sptr<OHOS::Rosen::Window>& window)
{
    return window && window->GetWindowState() == WindowState::STATE_DESTROYED;
}

int32_t SelectionPanel::StartMoving()
{
    SELECTION_HILOGI("SelectionPanel StartMoving start");
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (IsDestroyed(window)) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (!IsSelectionSystemAbilityExistent()) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    auto ret = window->StartMoveWindow();
    if (ret == WmErrorCode::WM_ERROR_DEVICE_NOT_SUPPORT) {
        SELECTION_HILOGE("window manager service not support error ret = %{public}d.", ret);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if (ret != WmErrorCode::WM_OK) {
        SELECTION_HILOGE("window manager service error ret = %{public}d.", ret);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    SELECTION_HILOGI("StartMoving  success!");
    return ErrorCode::NO_ERROR;
}

int32_t SelectionPanel::MoveTo(int32_t x, int32_t y)
{
    SELECTION_HILOGI("SelectionPanel MoveTo start");
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (IsDestroyed(window)) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    if (!IsSelectionSystemAbilityExistent()) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    WMError ret = window->MoveWindowToGlobalDisplay(x, y);
    SELECTION_HILOGI("Moveto finish, ret = %{public}d", ret);
    return ret == WMError::WM_ERROR_INVALID_PARAM ? ErrorCode::ERROR_PARAMETER_CHECK_FAILED : ErrorCode::NO_ERROR;
}

bool SelectionPanel::SetPanelStatusListener(std::shared_ptr<PanelStatusListener> statusListener,
    const std::string &type)
{
    if (!MarkListener(type, true)) {
        return false;
    }
    SELECTION_HILOGD("type: %{public}s.", type.c_str());
    if (panelStatusListener_ == nullptr) {
        auto isExist = [&type](const auto& pair) { return pair.second == type; };
        if (std::find_if(panelStatusMap_.begin(), panelStatusMap_.end(), isExist) != panelStatusMap_.end()) {
            SELECTION_HILOGD("panelStatusListener_ is nullptr, need to be set");
            panelStatusListener_ = std::move(statusListener);
        }
    }
    return true;
}

bool SelectionPanel::MarkListener(const std::string &type, bool isRegister)
{
    if (panelStatusMap_.find(SelectionWindowStatus::DESTROYED) != panelStatusMap_.end() &&
        type == panelStatusMap_.at(SelectionWindowStatus::DESTROYED)) {
        destroyedRegistered_ = isRegister;
    } else if (panelStatusMap_.find(SelectionWindowStatus::HIDDEN) != panelStatusMap_.end() &&
        type == panelStatusMap_.at(SelectionWindowStatus::HIDDEN)) {
        hiddenRegistered_ = isRegister;
    } else {
        SELECTION_HILOGE("type error!");
        return false;
    }
    return true;
}

void SelectionPanel::ClearPanelListener(const std::string &type)
{
    if (!MarkListener(type, false)) {
        return;
    }
    SELECTION_HILOGD("type: %{public}s.", type.c_str());
    if (panelStatusListener_ == nullptr) {
        SELECTION_HILOGD("panelStatusListener_ not set, don't need to remove.");
        return;
    }
    if (IsPanelListenerClearable()) {
        return;
    }
    panelStatusListener_ = nullptr;
}

bool SelectionPanel::IsPanelListenerClearable()
{
    return (destroyedRegistered_ || hiddenRegistered_);
}

uint32_t SelectionPanel::GetWindowId()
{
    auto window = GetWindow();
    if (window == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_PANEL_DESTROYED;
    }
    return window->GetWindowId();
}

bool SelectionPanel::IsSelectionSystemAbilityExistent()
{
    return SelectionSystemAbilityUtils::IsSelectionSystemAbilityExistent();
}
} // namespace SelectionFwk
} // namespace OHOS