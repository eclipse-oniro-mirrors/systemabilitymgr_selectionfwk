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
#include "scene_board_judgement.h"
#include "selection_log.h"
#include "selectionmethod_trace.h"
#include "selection_panel_manger.h"

namespace OHOS {
namespace SelectionFwk {
using WMError = OHOS::Rosen::WMError;
using WindowState = OHOS::Rosen::WindowState;
using namespace Rosen;
using WindowGravity = OHOS::Rosen::WindowGravity;
using WindowState = OHOS::Rosen::WindowState;
std::atomic<uint32_t> SelectionPanel::sequenceId_ { 0 };
constexpr int32_t MAXWAITTIME = 30;
constexpr int32_t WAITTIME = 10;
std::mutex SelectionPanel::windowMutex_;

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
    SELECTION_HILOGI(
        "start , panelType/x/y/width/height: %{public}d/%{public}d/%{public}d/%{public}d/%{public}d.",
        static_cast<int32_t>(panelType_), x_, y_, width_, height_);

    // winOption_ = new (std::nothrow) OHOS::Rosen::WindowOption();
    // if (winOption_ == nullptr) {
    //     return ErrorCode::ERROR_SELECTION_SERVICE;
    // }
    // winOption_->SetWindowType(OHOS::Rosen::WindowType::WINDOW_TYPE_DYNAMIC);
    // winOption_->SetWindowRect(OHOS::Rosen::Rect{10, 10, 80, 50});
    // WMError wmError = WMError::WM_OK;
    // window_ = OHOS::Rosen::Window::Create(GeneratePanelName(), winOption_, context, wmError);
    // if (wmError == WMError::WM_ERROR_INVALID_PERMISSION || wmError == WMError::WM_ERROR_NOT_SYSTEM_APP) {
    //     SELECTION_HILOGE("create window failed, permission denied, %{public}d!", wmError);
    //     return ErrorCode::ERROR_NOT_IME;
    // }
    // if (window_ == nullptr || wmError != WMError::WM_OK) {
    //     SELECTION_HILOGE("create window failed: %{public}d!", wmError);
    //     return ErrorCode::ERROR_SELECTION_SERVICE;
    // }
    // isScbEnable_ = Rosen::SceneBoardJudgement::IsSceneBoardEnabled();
    // if (SetPanelProperties() != ErrorCode::NO_ERROR) {
    //     wmError = window_->Destroy();
    //     SELECTION_HILOGI("destroy window end, wmError is %{public}d.", wmError);
    //     return ErrorCode::ERROR_SELECTION_SERVICE;
    // }
    // windowId_ = window_->GetWindowId();
    // SELECTION_HILOGI("success, type/flag/windowId/isScbEnable_: %{public}d/%{public}d/%{public}u/%{public}d.",
    //     static_cast<int32_t>(panelType_), static_cast<int32_t>(panelFlag_), windowId_, isScbEnable_);
    // // if (panelInfo.panelType == SOFT_KEYBOARD && isScbEnable_) {
    // //     RegisterKeyboardPanelInfoChangeListener();
    // // }
    // window_->RaiseToAppTop();
    // SELECTION_HILOGI("selectionPanel RaiseToAppTop  2.");
    // window_->Resize(80, 50);
    // window_->SetBackgroundColor("green");
    // window_->Show();
    // window_->Resize(80, 50);
    // window_->SetBackgroundColor("green");
    // SELECTION_HILOGI("selectionPanel show.");
    OHOS::Rosen::Rect baseWindowRect = { 150, 150, 400, 600 };
    sptr<Rosen::WindowOption> baseOp = new Rosen::WindowOption();
    baseOp->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_MAIN_WINDOW);
    baseOp->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FULLSCREEN);
    baseOp->SetWindowRect(baseWindowRect);
    baseOp->SetZIndex(1980);
    auto displayId = Rosen::DisplayManager::GetInstance().GetDefaultDisplayId();
    baseOp->SetDisplayId(displayId);

    sptr<Rosen::Window> window = Rosen::Window::Create("Demo_SSW_BaseWindow", baseOp, nullptr);
    if (!window) {
        SELECTION_HILOGE("Window creation failed");
        return ErrorCode::NO_ERROR;
    }
    window_ = window;
    SELECTION_HILOGI("Window::Create Success");
    return 0;
}

std::string SelectionPanel::GeneratePanelName()
{
    uint32_t sequenceId = GenerateSequenceId();
    std::string windowName = panelType_ == MENU_PANEL ? "menuPanel" + std::to_string(sequenceId) :
                                                           "mainPanel" + std::to_string(sequenceId);
    SELECTION_HILOGD("SelectionPanel, windowName: %{public}s.", windowName.c_str());
    return windowName;
}

int32_t SelectionPanel::SetPanelProperties()
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    WindowGravity gravity = WindowGravity::WINDOW_GRAVITY_FLOAT;
    if (!isScbEnable_) {
        WMError wmError = window_->SetWindowGravity(gravity, invalidGravityPercent);
        if (wmError != WMError::WM_OK) {
            SELECTION_HILOGE("failed to set window gravity, wmError is %{public}d, start destroy window!", wmError);
            return ErrorCode::ERROR_SELECTION_SERVICE;
        }
        return ErrorCode::NO_ERROR;
    }
    keyboardLayoutParams_.gravity_ = gravity;
    auto ret = window_->AdjustKeyboardLayout(keyboardLayoutParams_);
    if (ret != WMError::WM_OK) {
        SELECTION_HILOGE("SetWindowGravity failed, wmError is %{public}d, start destroy window!", ret);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    return ErrorCode::NO_ERROR;
}

int32_t SelectionPanel::DestroyPanel()
{
    auto ret = HidePanel();
    if (ret != ErrorCode::NO_ERROR) {
        SELECTION_HILOGE("SelectionPanel, hide panel failed, ret: %{public}d!", ret);
    }
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto result = window_->Destroy();
    SELECTION_HILOGI("destroy ret: %{public}d", result);
    PanelStatusChange(SelectionWindowStatus::DESTROYED);
    return ErrorCode::NO_ERROR;
}

uint32_t SelectionPanel::GenerateSequenceId()
{
    uint32_t seqId = ++sequenceId_;
    if (seqId == std::numeric_limits<uint32_t>::max()) {
        return ++sequenceId_;
    }
    return seqId;
}

int32_t SelectionPanel::SetUiContent(const std::string &contentInfo, napi_env env)
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr, can not SetUiContent!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if(IsDestroyed()) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTORYED;
    }
    WMError ret = WMError::WM_OK;

    window_->NapiSetUIContent(contentInfo, env, nullptr);
    WMError wmError = window_->SetTransparent(true);
    if (isWaitSetUiContent_) {
        isWaitSetUiContent_ = false;
    }
    SELECTION_HILOGI("SetTransparent ret: %{public}u.", wmError);
    SELECTION_HILOGI("NapiSetUIContent ret: %{public}d.", ret);
    return ret == WMError::WM_ERROR_INVALID_PARAM ? ErrorCode::ERROR_PARAMETER_CHECK_FAILED : ErrorCode::NO_ERROR;
}

PanelType SelectionPanel::GetPanelType()
{
    return panelType_;
}

int32_t SelectionPanel::ShowPanel()
{
    SELECTION_HILOGD("SelectionPanel start.");
    int32_t waitTime = 0;
    while (isWaitSetUiContent_ && waitTime < MAXWAITTIME) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAITTIME));
        waitTime += WAITTIME;
        SELECTION_HILOGI("SelectionPanel show pannel waitTime %{public}d.", waitTime);
    }
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if(IsDestroyed()) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTORYED;
    }
    if (IsShowing()) {
        SELECTION_HILOGI("panel already shown.");
        return ErrorCode::NO_ERROR;
    }
    auto ret = WMError::WM_OK;
    {
        SelectionMethodSyncTrace tracer("SelectionPanel_ShowPanel");
        ret = window_->Show();
    }
    if (ret != WMError::WM_OK) {
        SELECTION_HILOGE("ShowPanel error, err = %{public}d", ret);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    SELECTION_HILOGI("Selection panel shown successfully.");
    return ErrorCode::NO_ERROR;
}

bool SelectionPanel::IsShowing()
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto windowState = window_->GetWindowState();
    if (windowState == WindowState::STATE_SHOWN) {
        return true;
    }
    SELECTION_HILOGD("windowState: %{public}d.", static_cast<int>(windowState));
    return false;
}

void SelectionPanel::PanelStatusChange(const SelectionWindowStatus &status)
{
    if (panelStatusListener_ != nullptr) {
        SELECTION_HILOGD("panelStatusListener_ is not nullptr.");
        if (status == SelectionWindowStatus::HIDDEN && hiddedRegistered_ ) {
            panelStatusListener_->OnPanelStatus(windowId_, panelStatusMap_.at(status));
        }
        if (status == SelectionWindowStatus::DESTROYED && destroyedRegistered_ ) {
            panelStatusListener_->OnPanelStatus(windowId_, panelStatusMap_.at(status));
        }
    }
}

int32_t SelectionPanel::HidePanel()
{
    SELECTION_HILOGD("SelectionPanel start");
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if(IsDestroyed()) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTORYED;
    }
    if (IsHidden()) {
        SELECTION_HILOGI("panel already hidden.");
        return ErrorCode::NO_ERROR;
    }
    auto ret = WMError::WM_OK;
    {
        SelectionMethodSyncTrace tracer("SelectionPanel_HidePanel");
        ret = window_->Hide();
    }
    if (ret != WMError::WM_OK) {
        SELECTION_HILOGE("HidePanel error, err: %{public}d!", ret);
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    SELECTION_HILOGI("success, panelType/x/y/width/height: %{public}d/%{public}d/%{public}d/%{public}d/%{public}d.",
        static_cast<int32_t>(panelType_), x_, y_, width_, height_);
    PanelStatusChange(SelectionWindowStatus::HIDDEN);
    return ErrorCode::NO_ERROR;
}

bool SelectionPanel::IsHidden()
{
    auto windowState = window_->GetWindowState();
    if (windowState == WindowState::STATE_HIDDEN) {
        return true;
    }
    SELECTION_HILOGD("windowState: %{public}d.", static_cast<int>(windowState));
    return false;
}

bool SelectionPanel::IsDestroyed() const
{
    return window_ && window_->GetWindowState() == WindowState::STATE_DESTROYED;
}

int32_t SelectionPanel::StartMoving()
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if(IsDestroyed()) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTORYED;
    }
    auto ret = window_->StartMoveWindow();
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
    SELECTION_HILOGD("moveto start!");
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if(IsDestroyed()) {
        SELECTION_HILOGE("window is destroyed!");
        return ErrorCode::ERROR_PANEL_DESTORYED;
    }
    auto ret = window_->MoveTo(x, y);
    SELECTION_HILOGI("x/y: %{public}d/%{public}d, ret = %{public}d", x, y, ret);
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
    if (type == panelStatusMap_.at(SelectionWindowStatus::DESTROYED)) {
        destroyedRegistered_ = isRegister;
    } else if (type == panelStatusMap_.at(SelectionWindowStatus::HIDDEN)) {
        hiddedRegistered_ = isRegister;
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
    if (destroyedRegistered_ || hiddedRegistered_) {
        return;
    }
    panelStatusListener_ = nullptr;
}

int32_t SelectionPanel::GetWindowId()
{
    std::lock_guard<std::mutex> lock(windowMutex_);
    return window_->GetWindowId();

}

} // namespace SelectionFwk
} // namespace OHOS