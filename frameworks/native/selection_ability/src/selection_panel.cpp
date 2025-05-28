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
    SELECTION_HILOGD(
        "start, type/flag: %{public}d/%{public}d.", static_cast<int32_t>(panelType_), static_cast<int32_t>(panelFlag_));
    // panelType_ = panelInfo.panelType;
    // panelFlag_ = panelInfo.panelFlag;
    // winOption_ = new (std::nothrow) OHOS::Rosen::WindowOption();
    // if (winOption_ == nullptr) {
    //     return ErrorCode::ERROR_NULL_POINTER;
    // }
    // if (panelInfo.panelType == PanelType::STATUS_BAR) {//状态栏面板
    //     winOption_->SetWindowType(OHOS::Rosen::WindowType::WINDOW_TYPE_INPUT_METHOD_STATUS_BAR);//窗口类型需要添加WINDOW_TYPE_SELECTION_FLOAT
    // } else {
    //     winOption_->SetWindowType(OHOS::Rosen::WindowType::WINDOW_TYPE_INPUT_METHOD_FLOAT);//窗口类型需要添加WINDOW_TYPE_SELECTION_FLOAT
    // }
    // winOption_->SetWindowType(OHOS::Rosen::WindowType::WINDOW_TYPE_DIALOG);//窗口类型需要确认
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
    // // if (panelInfo.panelType == SOFT_KEYBOARD && isScbEnable_) {//软键盘+可用
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
    baseOp->SetWindowType(Rosen::WindowType::WINDOW_TYPE_DYNAMIC);
    baseOp->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    baseOp->SetWindowRect(baseWindowRect);
    baseOp->SetZIndex(1980);
    auto displayId = Rosen::DisplayManager::GetInstance().GetDefaultDisplayId();
    baseOp->SetDisplayId(displayId);
    // baseOp->SetWindowZOrder(Rosen::WindowZOrder::TOP_MOST);
    // baseOp->SetVisible(true);
    SELECTION_HILOGE("After SetWindowRect");

    sptr<Rosen::Window> window = Rosen::Window::Create("Demo_SSW_BaseWindow", baseOp, nullptr);
    if (!window) {
        SELECTION_HILOGE("Window creation failed");
        return ErrorCode::NO_ERROR;
    }
    SELECTION_HILOGE("After Window::Create");
    auto error = window->Show();
    if (error != WMError::WM_OK) {
        SELECTION_HILOGE("After Window::Show, error=%{public}d, WM_ERROR_INVALID_PARAM=%d", error, WM_ERROR_INVALID_PARAM);
    }
    // window->SetWindowFocus();
    // window->RaiseToTop();
    SELECTION_HILOGE("After Window::Show");
    return ErrorCode::NO_ERROR;
}

std::string SelectionPanel::GeneratePanelName()
{
    uint32_t sequenceId = GenerateSequenceId();
    std::string windowName = panelType_ == SOFT_KEYBOARD ? "softKeyboard" + std::to_string(sequenceId) ://panelType类型：softKeyboard、statusBar
                                                           "statusBar" + std::to_string(sequenceId);
    SELECTION_HILOGD("SelectionPanel, windowName: %{public}s.", windowName.c_str());
    return windowName;
}

int32_t SelectionPanel::SetPanelProperties()//设置输入面板属性，根据面板类型和标志来配置窗口的位置属性
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window is nullptr!");
        return ErrorCode::ERROR_OPERATE_PANEL;
    }
    WindowGravity gravity = WindowGravity::WINDOW_GRAVITY_FLOAT;//浮动位置
    if (panelType_ == SOFT_KEYBOARD && panelFlag_ == FLG_FIXED) {//软键盘+固定
        gravity = WindowGravity::WINDOW_GRAVITY_BOTTOM;
    } else if (panelType_ == SOFT_KEYBOARD && panelFlag_ == FLG_FLOATING) {//软键盘+浮动
        auto surfaceNode = window_->GetSurfaceNode();
        if (surfaceNode == nullptr) {
            SELECTION_HILOGE("surfaceNode is nullptr!");
            return ErrorCode::ERROR_OPERATE_PANEL;
        }
        surfaceNode->SetFrameGravity(Rosen::Gravity::TOP_LEFT);
        Rosen::RSTransactionProxy::GetInstance()->FlushImplicitTransaction();
    } else if (panelType_ == STATUS_BAR) {//状态栏面板
        auto surfaceNo = window_->GetSurfaceNode();
        if (surfaceNo == nullptr) {
            SELECTION_HILOGE("surfaceNo is nullptr!");
            return ErrorCode::ERROR_OPERATE_PANEL;
        }
        surfaceNo->SetFrameGravity(Rosen::Gravity::TOP_LEFT);
        Rosen::RSTransactionProxy::GetInstance()->FlushImplicitTransaction();
        return ErrorCode::NO_ERROR;
    }
    if (!isScbEnable_) {//非场景模式的重力设置------是什么东西，划词需要吗
        WMError wmError = window_->SetWindowGravity(gravity, invalidGravityPercent);
        if (wmError != WMError::WM_OK) {
            SELECTION_HILOGE("failed to set window gravity, wmError is %{public}d, start destroy window!", wmError);
            return ErrorCode::ERROR_OPERATE_PANEL;
        }
        return ErrorCode::NO_ERROR;
    }
    keyboardLayoutParams_.gravity_ = gravity;//场景模式
    auto ret = window_->AdjustKeyboardLayout(keyboardLayoutParams_);
    if (ret != WMError::WM_OK) {
        SELECTION_HILOGE("SetWindowGravity failed, wmError is %{public}d, start destroy window!", ret);
        return ErrorCode::ERROR_OPERATE_PANEL;
    }
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

// void SelectionPanel::SetPanelHeightCallback(CallbackFunc heightCallback)
// {
//     panelHeightCallback_ = std::move(heightCallback);
// }

int32_t SelectionPanel::SetUiContent(const std::string &contentInfo, napi_env env)
{
    if (window_ == nullptr) {
        SELECTION_HILOGE("window_ is nullptr, can not SetUiContent!");
        return ErrorCode::ERROR_NULL_POINTER;
    }
    WMError ret = WMError::WM_OK;

    window_->NapiSetUIContent(contentInfo, env, nullptr);//调用napi接口设置UI内容
    // if (storage == nullptr) {
    //     ret = window_->NapiSetUIContent(contentInfo, env, nullptr);
    // } else {
    //     ret = window_->NapiSetUIContent(contentInfo, env, storage->GetNapiValue());
    // }
    WMError wmError = window_->SetTransparent(true);
    if (isWaitSetUiContent_) {
        isWaitSetUiContent_ = false;
    }
    SELECTION_HILOGI("SetTransparent ret: %{public}u.", wmError);
    SELECTION_HILOGI("NapiSetUIContent ret: %{public}d.", ret);
    return ret == WMError::WM_ERROR_INVALID_PARAM ? ErrorCode::ERROR_PARAMETER_CHECK_FAILED : ErrorCode::NO_ERROR;
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
    // if (!isScbEnable_) {
    //     PanelStatusChangeToImc(SelectionWindowStatus::SHOW, window_->GetRect());//通知输入法管理器或其他监听者
    // }
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
// 用于处理输入法面板状态变化，并将相关信息通知给输入法服务代理。（当输入法面板的状态或位置发生变化时，当函数会被调用）
// void SelectionPanel::PanelStatusChangeToImc(const SelectionWindowStatus &status, const Rosen::Rect &rect)
// {
//     ImeWindowInfo info;
//     info.panelInfo.panelType = panelType_;
//     info.panelInfo.panelFlag = panelFlag_;
//     if (info.panelInfo.panelType != SOFT_KEYBOARD || info.panelInfo.panelFlag == FLG_CANDIDATE_COLUMN) {
//         SELECTION_HILOGD("no need to deal.");
//         return;
//     }
//     auto proxy = ImaUtils::GetImsaProxy();//通过IMSA（Input Method Service Agent）代理对象获取输入法系统能力==》OnDemandStartStopSa(涉及到与系统能力管理器的交互)
//     if (proxy == nullptr) {
//         SELECTION_HILOGE("proxy is nullptr!");
//         return;
//     }
//     std::string name = window_->GetWindowName() + "/" + std::to_string(window_->GetWindowId());
//     info.windowInfo.name = std::move(name);
//     info.windowInfo.left = rect.posX_;
//     info.windowInfo.top = rect.posY_;
//     info.windowInfo.width = rect.width_;
//     info.windowInfo.height = rect.height_;
//     SELECTION_HILOGD("rect[%{public}d, %{public}d, %{public}u, %{public}u], status: %{public}d, "
//                 "panelFlag: %{public}d.",
//         rect.posX_, rect.posY_, rect.width_, rect.height_, status, info.panelInfo.panelFlag);
//     proxy->PanelStatusChange(static_cast<uint32_t>(status), info);
// }


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
    // if (!isScbEnable_) {
    //     PanelStatusChangeToImc(InputWindowStatus::HIDE, { 0, 0, 0, 0 });
    // }
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

// int32_t SelectionPanel::StartMoving()
// {
//     if (window_ == nullptr) {
//         SELECTION_HILOGE("window_ is nullptr!");
//         return ErrorCode::ERROR_IME;
//     }
//     // if (panelType_ != STATUS_BAR) {
//     //     IMSA_HILOGE("SOFT_KEYBOARD panel can not move!");
//     //     return ErrorCode::ERROR_INVALID_PANEL_TYPE;
//     // }
//     // if (panelFlag_ != FLG_FLOATING) {
//     //     IMSA_HILOGE("invalid panel flag: %{public}d", panelFlag_);
//     //     return ErrorCode::ERROR_INVALID_PANEL_FLAG;
//     // }
//     auto ret = window_->StartMoveWindow();
//     if (ret == WmErrorCode::WM_ERROR_DEVICE_NOT_SUPPORT) {
//         SELECTION_HILOGE("window manager service not support error ret = %{public}d.", ret);
//         return ErrorCode::ERROR_DEVICE_UNSUPPORTED;
//     }
//     if (ret != WmErrorCode::WM_OK) {
//         SELECTION_HILOGE("window manager service error ret = %{public}d.", ret);
//         return ErrorCode::ERROR_WINDOW_MANAGER;
//     }
//     SELECTION_HILOGI("StartMoving  success!");
//     return ErrorCode::NO_ERROR;
// }

} // namespace SelectionFwk
} // namespace OHOS