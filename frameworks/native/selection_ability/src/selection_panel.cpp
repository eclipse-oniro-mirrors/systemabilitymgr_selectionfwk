/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

SelectionPanel::~SelectionPanel() = default;
int32_t SelectionPanel::CreatePanel(
    const std::shared_ptr<AbilityRuntime::Context> &context, const PanelInfo &panelInfo)
{
    SELECTION_HILOGI("SelectionPanel CreatePanel start.");
    panelType_ = panelInfo.panelType;
    panelFlag_ = panelInfo.panelFlag;
    SELECTION_HILOGD(
        "start, type/flag: %{public}d/%{public}d.", static_cast<int32_t>(panelType_), static_cast<int32_t>(panelFlag_));
    // winOption_ = new (std::nothrow) OHOS::Rosen::WindowOption();
    // if (winOption_ == nullptr) {
    //     return ErrorCode::ERROR_NULL_POINTER;
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
    //     return ErrorCode::ERROR_OPERATE_PANEL;
    // }
    // isScbEnable_ = Rosen::SceneBoardJudgement::IsSceneBoardEnabled();
    // if (SetPanelProperties() != ErrorCode::NO_ERROR) {
    //     wmError = window_->Destroy();
    //     SELECTION_HILOGI("destroy window end, wmError is %{public}d.", wmError);
    //     return ErrorCode::ERROR_OPERATE_PANEL;
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
    std::string windowName = panelType_ == SOFT_KEYBOARD ? "softKeyboard" + std::to_string(sequenceId) :
                                                           "statusBar" + std::to_string(sequenceId);
    SELECTION_HILOGD("SelectionPanel, windowName: %{public}s.", windowName.c_str());
    return windowName;
}

int32_t SelectionPanel::SetPanelProperties()
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window is nullptr!");
        return ErrorCode::ERROR_OPERATE_PANEL;
    }
    WindowGravity gravity = WindowGravity::WINDOW_GRAVITY_FLOAT;
    if (panelType_ == SOFT_KEYBOARD && panelFlag_ == FLG_FIXED) {
        gravity = WindowGravity::WINDOW_GRAVITY_BOTTOM;
    } else if (panelType_ == SOFT_KEYBOARD && panelFlag_ == FLG_FLOATING) {
        auto surfaceNode = window_->GetSurfaceNode();
        if (surfaceNode == nullptr) {
            SELECTION_HILOGE("surfaceNode is nullptr!");
            return ErrorCode::ERROR_OPERATE_PANEL;
        }
        surfaceNode->SetFrameGravity(Rosen::Gravity::TOP_LEFT);
        Rosen::RSTransactionProxy::GetInstance()->FlushImplicitTransaction();
    } else if (panelType_ == STATUS_BAR) {
        auto surfaceNo = window_->GetSurfaceNode();
        if (surfaceNo == nullptr) {
            SELECTION_HILOGE("surfaceNo is nullptr!");
            return ErrorCode::ERROR_OPERATE_PANEL;
        }
        surfaceNo->SetFrameGravity(Rosen::Gravity::TOP_LEFT);
        Rosen::RSTransactionProxy::GetInstance()->FlushImplicitTransaction();
        return ErrorCode::NO_ERROR;
    }
    if (!isScbEnable_) {
        WMError wmError = window_->SetWindowGravity(gravity, invalidGravityPercent);
        if (wmError != WMError::WM_OK) {
            SELECTION_HILOGE("failed to set window gravity, wmError is %{public}d, start destroy window!", wmError);
            return ErrorCode::ERROR_OPERATE_PANEL;
        }
        return ErrorCode::NO_ERROR;
    }
    keyboardLayoutParams_.gravity_ = gravity;
    auto ret = window_->AdjustKeyboardLayout(keyboardLayoutParams_);
    if (ret != WMError::WM_OK) {
        SELECTION_HILOGE("SetWindowGravity failed, wmError is %{public}d, start destroy window!", ret);
        return ErrorCode::ERROR_OPERATE_PANEL;
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
        return ErrorCode::ERROR_NULL_POINTER;
    }
    auto result = window_->Destroy();
    SELECTION_HILOGI("destroy ret: %{public}d", result);
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
        return ErrorCode::ERROR_NULL_POINTER;
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
        return ErrorCode::ERROR_IMA_NULLPTR;
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
        return ErrorCode::ERROR_OPERATE_PANEL;
    }
    SELECTION_HILOGI("Selection panel shown successfully.");
    PanelStatusChange(SelectionWindowStatus::SHOW);
    return ErrorCode::NO_ERROR;
}

bool SelectionPanel::IsShowing()
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_NULL_POINTER;
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
    if (status == SelectionWindowStatus::SHOW && showRegistered_ && panelStatusListener_ != nullptr) {
        SELECTION_HILOGD("ShowPanel panelStatusListener_ is not nullptr.");
        panelStatusListener_->OnPanelStatus(windowId_, true);
    }
    if (status == SelectionWindowStatus::HIDE && hideRegistered_ && panelStatusListener_ != nullptr) {
        SELECTION_HILOGD("HidePanel panelStatusListener_ is not nullptr.");
        panelStatusListener_->OnPanelStatus(windowId_, false);
    }
}

int32_t SelectionPanel::HidePanel()
{
    SELECTION_HILOGD("SelectionPanel start");
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_NULL_POINTER;
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
        return ErrorCode::ERROR_OPERATE_PANEL;
    }
    SELECTION_HILOGI("success, type/flag: %{public}d/%{public}d.", static_cast<int32_t>(panelType_),
        static_cast<int32_t>(panelFlag_));
    PanelStatusChange(SelectionWindowStatus::HIDE);
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

int32_t SelectionPanel::StartMoving()
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_IME;
    }
    auto ret = window_->StartMoveWindow();
    if (ret == WmErrorCode::WM_ERROR_DEVICE_NOT_SUPPORT) {
        SELECTION_HILOGE("window manager service not support error ret = %{public}d.", ret);
        return ErrorCode::ERROR_DEVICE_UNSUPPORTED;
    }
    if (ret != WmErrorCode::WM_OK) {
        SELECTION_HILOGE("window manager service error ret = %{public}d.", ret);
        return ErrorCode::ERROR_WINDOW_MANAGER;
    }
    SELECTION_HILOGI("StartMoving  success!");
    return ErrorCode::NO_ERROR;
}

int32_t SelectionPanel::MoveTo(int32_t x, int32_t y)
{
    SELECTION_HILOGD("moveto start!");
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr!");
        return ErrorCode::ERROR_NULL_POINTER;
    }
    if (panelFlag_ == FLG_FIXED) {
        SELECTION_HILOGE("FLG_FIXED panel can not moveTo!");
        return ErrorCode::NO_ERROR;
    }
    auto ret = window_->MoveTo(x, y);
    SELECTION_HILOGI("x/y: %{public}d/%{public}d, ret = %{public}d", x, y, ret);
    return ret == WMError::WM_ERROR_INVALID_PARAM ? ErrorCode::ERROR_PARAMETER_CHECK_FAILED : ErrorCode::NO_ERROR;
}

} // namespace SelectionFwk
} // namespace OHOS