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

#include <condition_variable>
#include <mutex>
#include <input_manager.h>
#include "selection_service.h"
#include "selection_common.h"
#include "selection_log.h"
#include "common_event_manager.h"
#include "selection_config.h"
#include "selection_input_monitor.h"
#include "selection_errors.h"
#include "hisysevent_adapter.h"
#include "selection_timer.h"
#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif

using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::MMI;
using namespace OHOS::EventFwk;

std::atomic<uint32_t> selSeqId = 0;
const std::unordered_set<std::string> appBlocklist = {};

static int64_t GetCurrentTimeMillis()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint32_t GenerateSequenceId()
{
    return selSeqId.fetch_add(1, std::memory_order_seq_cst);
}

bool BaseSelectionInputMonitor::IsSelectionTriggered() const
{
    return IsSelectionDone();
}

const SelectionInfo& BaseSelectionInputMonitor::GetSelectionInfo() const
{
    return selectionInfo_;
}

void BaseSelectionInputMonitor::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    SELECTION_HILOGD("Before keyEvent, curSelectState: %{public}d, subSelectState: %{public}d; "
        "keyAction: %{public}d",
        curSelectState, subSelectState, keyEvent->GetKeyAction());
    ProcessInputEvent(keyEvent);
    SELECTION_HILOGD("After keyEvent, curSelectState: %{public}d, subSelectState: %{public}d",
        curSelectState, subSelectState);
}

void BaseSelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    SELECTION_HILOGD("Before pointerEvent, curSelectState: %{public}d, subSelectState: %{public}d; "
        "buttonId: %{public}d, pointerAction: %{public}d",
        curSelectState, subSelectState, pointerEvent->GetButtonId(), pointerEvent->GetPointerAction());
    ProcessInputEvent(pointerEvent);
    SELECTION_HILOGD("After pointerEvent, curSelectState: %{public}d, subSelectState: %{public}d",
        curSelectState, subSelectState);
}

void BaseSelectionInputMonitor::ProcessInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    if (curSelectState == SelectInputState::SELECT_INPUT_DONE) {
        if (subSelectState != SelectInputSubState::SUB_INITIAL) {
            curSelectState = SelectInputState::SELECT_INPUT_WORD_END;
            return;
        }
        ResetState();
        return;
    }

    if (subSelectState != SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN &&
        subSelectState != SelectInputSubState::SUB_WAIT_KEY_CTRL_UP &&
        subSelectState != SelectInputSubState::SUB_WAIT_BUTTON_OR_CTRL_DOWN) {
        return;
    }

    int32_t keyCode = keyEvent->GetKeyCode();
    int32_t action = keyEvent->GetKeyAction();
    if (keyCode != KeyEvent::KEYCODE_CTRL_LEFT && keyCode != KeyEvent::KEYCODE_CTRL_RIGHT) {
        ResetState();
        return;
    }
    if (action == KeyEvent::KEY_ACTION_DOWN && keyEvent->IsRepeatKey()) {
        SELECTION_HILOGD("current ctrl down is repeat, ignore");
        return;
    }
    SELECTION_HILOGI("[SelectionService] Processed ctrl key with action: %{public}d.", action);
    if ((subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN ||
         subSelectState == SelectInputSubState::SUB_WAIT_BUTTON_OR_CTRL_DOWN) &&
        action == KeyEvent::KEY_ACTION_DOWN) {
        subSelectState = SelectInputSubState::SUB_WAIT_KEY_CTRL_UP;
        return;
    }

    if (subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_UP && action == KeyEvent::KEY_ACTION_UP) {
        if (curSelectState == SelectInputState::SELECT_INPUT_WORD_END) {
            curSelectState = SelectInputState::SELECT_INPUT_DONE;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_DONE.");
        }
        subSelectState = SelectInputSubState::SUB_INITIAL;
    } else {
        if (action == KeyEvent::KEY_ACTION_UP && keyEvent->IsRepeatKey()) {
            SELECTION_HILOGD("current ctrl up is repeat, ignore");
            return;
        }
        curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
        subSelectState = SelectInputSubState::SUB_INITIAL;
        SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_INITIAL.");
    }
    FinishedWordSelection();
    return;
}

void BaseSelectionInputMonitor::ProcessInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t buttonId = pointerEvent->GetButtonId();
    int32_t action = pointerEvent->GetPointerAction();
    if (action == PointerEvent::POINTER_ACTION_ENTER_WINDOW ||
        action == PointerEvent::POINTER_ACTION_LEAVE_WINDOW) {
        return;
    }
    if (buttonId == PointerEvent::BUTTON_NONE && action == PointerEvent::POINTER_ACTION_MOVE) {
        if (ProcessMovement(pointerEvent)) {
            return;
        }
    }
    if (buttonId != PointerEvent::MOUSE_BUTTON_LEFT) {
        ResetState();
        return;
    }

    SELECTION_HILOGD("[SelectionService] into PointerEvent, curSelectState = %{public}d.", curSelectState);
    switch (curSelectState) {
        case SelectInputState::SELECT_INPUT_INITIAL:
            InputInitialProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WORD_BEGIN:
            InputWordBeginProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE:
            InputWordLeftMoveProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WAIT_DOUBLE_CLICK:
            InputWordDoubleClickProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WORD_END:
            InputWordEndProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_DONE:
            InputDoneProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WAIT_TRIPLE_CLICK:
            InputWordTripleClickProcess(pointerEvent);
            break;

        default:
            break;
    }

    FinishedWordSelection();
    return;
}

bool BaseSelectionInputMonitor::ProcessMovement(std::shared_ptr<PointerEvent> pointerEvent) const
{
    if (subSelectState == SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_DOWN) {
        if (IsTinyMovement(pointerEvent)) {
            if (curSelectState == SelectInputState::SELECT_INPUT_DONE) {
                curSelectState = SelectInputState::SELECT_INPUT_WAIT_TRIPLE_CLICK;
            }
            return true;
        }
    }
    if (subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN ||
        subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_UP) {
        return true;
    }
    if (subSelectState == SelectInputSubState::SUB_WAIT_BUTTON_OR_CTRL_DOWN) {
        subSelectState = SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN;
        return true;
    }
    return false;
}

bool BaseSelectionInputMonitor::IsTinyMovement(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerId, pointerItem);
    if (abs(selectionInfo_.startDisplayX - pointerItem.GetGlobalX()) > MAX_POSITION_CHANGE_OFFSET ||
        abs(selectionInfo_.startDisplayY - pointerItem.GetGlobalY()) > MAX_POSITION_CHANGE_OFFSET) {
        return false;
    }
    return true;
}

bool BaseSelectionInputMonitor::IsClickTimeout(uint32_t time) const
{
    auto curTime = GetCurrentTimeMillis();
    auto duration = curTime - lastClickTime;
    lastClickTime = curTime;
    return (duration > time);
}

void BaseSelectionInputMonitor::OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const
{
    SELECTION_HILOGI("[SelectionService] into axisEvent");
};

void BaseSelectionInputMonitor::ResetProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    SELECTION_HILOGI("ResetProcess with action: %{public}d.", pointerEvent->GetPointerAction());
    curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
    subSelectState = SelectInputSubState::SUB_INITIAL;
    OnInputEvent(pointerEvent);
}

void BaseSelectionInputMonitor::SaveSelectionStartInfo(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerId, pointerItem);
    selectionInfo_.startDisplayX = pointerItem.GetGlobalX();
    selectionInfo_.startDisplayY = pointerItem.GetGlobalY();
    selectionInfo_.endDisplayX = pointerItem.GetGlobalX();
    selectionInfo_.endDisplayY = pointerItem.GetGlobalY();
    selectionInfo_.startWindowX = pointerItem.GetWindowX();
    selectionInfo_.startWindowY = pointerItem.GetWindowY();
    selectionInfo_.endWindowX = pointerItem.GetWindowX();
    selectionInfo_.endWindowY = pointerItem.GetWindowY();
    int32_t displayId = pointerEvent->GetTargetDisplayId();
    int32_t windowId = pointerEvent->GetTargetWindowId();
    if (displayId < 0 || windowId < 0) {
        SELECTION_HILOGE("Get invalid displayId or windowId from pointerEvent! displayId:%{public}d,"
            "windowId: %{public}d", displayId, windowId);
        return;
    }
    selectionInfo_.displayId = static_cast<std::uint32_t>(displayId);
    selectionInfo_.windowId = static_cast<std::uint32_t>(windowId);

    OHOS::Rosen::WindowInfoOption windowInfoOption;
    windowInfoOption.windowId = static_cast<std::int32_t>(selectionInfo_.windowId);
    SELECTION_HILOGI("Begin to call ListWindowInfo");
    std::vector<sptr<Rosen::WindowInfo>> infos;
#ifdef SCENE_BOARD_ENABLE
    Rosen::WMError ret = Rosen::WindowManagerLite::GetInstance().ListWindowInfo(windowInfoOption, infos);
#else
    Rosen::WMError ret = Rosen::WindowManager::GetInstance().ListWindowInfo(windowInfoOption, infos);
#endif
    SELECTION_HILOGI("ListWindowInfo(windowId: %{public}d), ret: %{public}d, infos size: %{public}zu",
        windowInfoOption.windowId, ret, infos.size());
    for (auto& info : infos) {
        SELECTION_HILOGI("WindowInfo bundleName:%{public}s, windowtype:%{public}d, width:%{public}d, height:%{public}d",
            info->windowMetaInfo.bundleName.c_str(), info->windowMetaInfo.windowType,
            info->windowLayoutInfo.rect.width_, info->windowLayoutInfo.rect.height_);
    }
    if (ret == Rosen::WMError::WM_OK && !infos.empty()) {
        selectionInfo_.bundleName = infos[0]->windowMetaInfo.bundleName;
    }
}

void BaseSelectionInputMonitor::SaveSelectionEndInfo(std::shared_ptr<PointerEvent> pointerEvent) const
{
    SaveSelectionType();
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerId, pointerItem);
    selectionInfo_.endDisplayX = pointerItem.GetGlobalX();
    selectionInfo_.endDisplayY = pointerItem.GetGlobalY();
    selectionInfo_.endWindowX = pointerItem.GetWindowX();
    selectionInfo_.endWindowY = pointerItem.GetWindowY();
}

void BaseSelectionInputMonitor::SaveSelectionType() const
{
    switch (curSelectState) {
        case SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE:
            selectionInfo_.selectionType = MOVE_SELECTION;
            break;
        case SelectInputState::SELECT_INPUT_WAIT_DOUBLE_CLICK:
            selectionInfo_.selectionType = DOUBLE_CLICKED_SELECTION;
            break;
        case SelectInputState::SELECT_INPUT_WAIT_TRIPLE_CLICK:
            selectionInfo_.selectionType = TRIPLE_CLICKED_SELECTION;
            break;
        default:
            break;
    }
}

bool BaseSelectionInputMonitor::IsSelectionDone() const
{
    return curSelectState == SelectInputState::SELECT_INPUT_DONE;
}

bool BaseSelectionInputMonitor::IsInputWordEnd() const
{
    return curSelectState == SelectInputState::SELECT_INPUT_WORD_END;
}

bool BaseSelectionInputMonitor::GetCtrlSelectFlag() const
{
    return MemSelectionConfig::GetInstance().GetTriggered();
}

void BaseSelectionInputMonitor::InputInitialProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    int32_t buttonId = pointerEvent->GetButtonId();
    if (action == PointerEvent::POINTER_ACTION_BUTTON_DOWN && buttonId == PointerEvent::MOUSE_BUTTON_LEFT) {
        curSelectState = SelectInputState::SELECT_INPUT_WORD_BEGIN;
        subSelectState = SelectInputSubState::SUB_INITIAL;
        lastClickTime = GetCurrentTimeMillis();
        SaveSelectionStartInfo(pointerEvent);
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WORD_BEGIN.");
    }
    return;
}

void BaseSelectionInputMonitor::InputWordBeginProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action == PointerEvent::POINTER_ACTION_MOVE) {
        curSelectState = SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE;
        subSelectState = SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_LEFT_MOVE.");
    } else if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        curSelectState = SelectInputState::SELECT_INPUT_WAIT_DOUBLE_CLICK;
        subSelectState = SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_DOWN;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_DOUBLE_CLICK.");
    }
    return;
}

void BaseSelectionInputMonitor::ProcessWordSelection(std::shared_ptr<PointerEvent> pointerEvent) const
{
    SaveSelectionEndInfo(pointerEvent);
    if (GetCtrlSelectFlag()) {
        if (curSelectState == SelectInputState::SELECT_INPUT_WAIT_DOUBLE_CLICK) {
            subSelectState = SelectInputSubState::SUB_WAIT_BUTTON_OR_CTRL_DOWN;
        } else {
            subSelectState = SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN;
        }
        curSelectState = SelectInputState::SELECT_INPUT_WORD_END;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WORD_END");
    } else {
        if (curSelectState == SelectInputState::SELECT_INPUT_WAIT_DOUBLE_CLICK) {
            subSelectState = SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_DOWN;
        } else {
            subSelectState = SelectInputSubState::SUB_INITIAL;
        }
        curSelectState = SelectInputState::SELECT_INPUT_DONE;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_DONE.");
    }
}

void BaseSelectionInputMonitor::InputWordLeftMoveProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        ProcessWordSelection(pointerEvent);
    } else if (action != PointerEvent::POINTER_ACTION_MOVE) {
        SELECTION_HILOGI("Action reset. subSelectState is %{public}d, action is %{public}d.", subSelectState, action);
        ResetProcess(pointerEvent);
    }
    return;
}

void BaseSelectionInputMonitor::JudgeTripleClick(std::shared_ptr<PointerEvent> pointerEvent) const
{
    if (IsClickTimeout(TRIPLE_CLICK_TIME)) {
        ResetProcess(pointerEvent);
        return;
    }
    curSelectState = SelectInputState::SELECT_INPUT_WAIT_TRIPLE_CLICK;
    subSelectState = SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP;
    SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_TRIPLE_CLICK.");
}

void BaseSelectionInputMonitor::InputWordDoubleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (subSelectState == SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_DOWN &&
        action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        if (IsClickTimeout(DOUBLE_CLICK_TIME)) {
            ResetProcess(pointerEvent);
            return;
        }
        subSelectState = SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP;
        SELECTION_HILOGI("set subSelectState to SUB_WAIT_POINTER_ACTION_BUTTON_UP.");
        return;
    }
    if (subSelectState == SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP) {
        if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
            ProcessWordSelection(pointerEvent);
        } else if (action == PointerEvent::POINTER_ACTION_MOVE) {
            curSelectState = SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_LEFT_MOVE.");
        }
        return;
    }
    if (subSelectState == SelectInputSubState::SUB_WAIT_BUTTON_OR_CTRL_DOWN &&
        action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        JudgeTripleClick(pointerEvent);
    }
    return;
}

void BaseSelectionInputMonitor::InputWordEndProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action != PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        ResetState();
        return;
    }
    if (subSelectState != SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_DOWN &&
        subSelectState != SelectInputSubState::SUB_WAIT_BUTTON_OR_CTRL_DOWN) {
        ResetProcess(pointerEvent);
        return;
    }

    SELECTION_HILOGI("Begin JudgeTripleClick.");
    JudgeTripleClick(pointerEvent);
}

void BaseSelectionInputMonitor::InputDoneProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    InputWordEndProcess(pointerEvent);
}

void BaseSelectionInputMonitor::InputWordTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        SELECTION_HILOGI("[Deprecated] this branch may not be entered.");
        if (IsClickTimeout(TRIPLE_CLICK_TIME)) {
            ResetProcess(pointerEvent);
            return;
        }
        subSelectState = SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP;
    } else if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        ProcessWordSelection(pointerEvent);
    } else if (action == PointerEvent::POINTER_ACTION_MOVE) {
        curSelectState = SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_LEFT_MOVE.");
    } else {
        SELECTION_HILOGI("Action reset. subSelectState is %{public}d, action is %{public}d.", subSelectState, action);
        ResetProcess(pointerEvent);
    }
    return;
}

void BaseSelectionInputMonitor::FinishedWordSelection() const
{
    if (!IsSelectionDone()) {
        return;
    }
    GenerateSequenceId();
    SELECTION_HILOGW("[selectevent] curSelectState:%{public}d. Selection event id is %{public}u.", curSelectState,
        selSeqId.load());
}

void BaseSelectionInputMonitor::ResetState() const
{
    curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
    subSelectState = SelectInputSubState::SUB_INITIAL;
    SELECTION_HILOGD("ResetState.");
}

SelectionInputMonitor::SelectionInputMonitor()
{
    baseInputMonitor_ = std::make_shared<BaseSelectionInputMonitor>();
}

SelectionInputMonitor::~SelectionInputMonitor()
{
    SELECTION_HILOGI("~SelectionInputMonitor called");
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    baseInputMonitor_->OnInputEvent(keyEvent);
    FinishedWordSelection();
}

void SelectionInputMonitor::HandleWordSelected() const
{
    if (!SelectionService::GetInstance()->HasExtAbilityConnection()) {
        int32_t ret = SelectionService::GetInstance()->ConnectExtAbilityFromConfig();
        if (ret != 0) {
            SELECTION_HILOGE("start selection extension ability failed");
        }
    }
    SelectionService::GetInstance()->ResetPluginUnloadTimer();
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    HandleWindowFocused(pointerEvent);
    if (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        SELECTION_HILOGD("Detect multimode event: POINTER_ACTION_BUTTON_DOWN");
    }
    baseInputMonitor_->OnInputEvent(pointerEvent);
    FinishedWordSelection();
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const
{
    baseInputMonitor_->OnInputEvent(axisEvent);
}

bool SelectionInputMonitor::GetCanGetSelectionContentFlag() const
{
    return SelectionService::GetInstance()->CanGetPasteboardContent();
}

void SelectionInputMonitor::SetCanGetSelectionContentFlag(bool flag) const
{
    SelectionService::GetInstance()->SetPasteboardFlag(flag);
}

void SelectionInputMonitor::HandleWindowFocused(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action != PointerEvent::POINTER_ACTION_BUTTON_DOWN && action != PointerEvent::POINTER_ACTION_DOWN) {
        return;
    }
    auto windowId = pointerEvent->GetTargetWindowId();
    Rosen::FocusChangeInfo focusInfo;
#ifdef SCENE_BOARD_ENABLE
    Rosen::WindowManagerLite::GetInstance().GetFocusWindowInfo(focusInfo);
#else
    Rosen::WindowManager::GetInstance().GetFocusWindowInfo(focusInfo);
#endif
    auto windowType = static_cast<uint32_t>(focusInfo.windowType_);
    if (windowId != focusInfo.windowId_) {
        SELECTION_HILOGI("Clicked window is not focused, focus-changed event will dispose selection panel later");
        return;
    }
    SelectionFocusChangeInfo focusChangeInfo(focusInfo.windowId_, focusInfo.displayId_, focusInfo.pid_,
        focusInfo.uid_, windowType, true, FocusChangeSource::InputManager);
    sptr<ISelectionListener> listener = SelectionService::GetInstance()->GetListener();
    if (listener == nullptr) {
        SELECTION_HILOGD("Selection listener is nullptr");
        return;
    }
    ErrCode errCode = listener->FocusChange(focusChangeInfo);
    if (errCode != NO_ERROR) {
        SELECTION_HILOGE("Failed to call ISelectionListener::FocusChange, error code: %{public}d.", errCode);
    }
}

bool SelectionInputMonitor::IsAppInBlocklist(const std::string& bundleName) const
{
    return std::find(appBlocklist.begin(), appBlocklist.end(), bundleName) != appBlocklist.end();
}

void SelectionInputMonitor::FinishedWordSelection() const
{
    if (!baseInputMonitor_->IsSelectionTriggered()) {
        return;
    }

    HandleWordSelected();

    if (SelectionService::GetInstance()->GetScreenLockedFlag()) {
        SELECTION_HILOGW("The screen is locked, skip notifying selection info.");
        return;
    }
    auto selectionInfo = baseInputMonitor_->GetSelectionInfo();
    if (selectionInfo.bundleName.empty()) {
        SELECTION_HILOGE("Failed to get Selected bundleName, skip notifying selection info.");
    }

    if (IsAppInBlocklist(selectionInfo.bundleName)) {
        SELECTION_HILOGW("The app [%{public}s] is in the blocklist, skip notifying selection info.",
            selectionInfo.bundleName.c_str());
        return;
    }
    if (!MemSelectionConfig::GetInstance().GetEnable()) {
        SELECTION_HILOGI("Selection switch is off, skip notifying selection info.");
        return;
    }

    SelectionInfoData infoData;
    infoData.data = selectionInfo;

    sptr<ISelectionListener> listener = SelectionService::GetInstance()->GetListener();
    if (listener == nullptr) {
        SELECTION_HILOGE("Selection listener is nullptr");
        return;
    }
    SetCanGetSelectionContentFlag(true);
    listener->OnSelectionChange(infoData);
}

int32_t SelectionInputMonitor::GetSelectionContent(std::string& selectionContent)
{
    SELECTION_HILOGI("SelectionInputMonitor::GetSelectionContent start");
    if (!baseInputMonitor_) {
        SELECTION_HILOGE("baseInputMonitor_ is nullptr");
        return SelectionServiceError::INVALID_DATA;
    }

    HisyseventAdapter::GetInstance()->AddSelectionCount();
    SetCanGetSelectionContentFlag(false);
    auto selectionInfo = baseInputMonitor_->GetSelectionInfo();

    return SelectionService::GetInstance()->GetPasteboardContent(selectionContent, selectionInfo.windowId,
        selectionInfo.bundleName);
}

int32_t SelectionInputMonitor::SetPanelShowingStatus(bool status) const
{
    SELECTION_HILOGI("set panel showing status: %{public}d", status);
    isPanelShowing_.store(status);
    return 0;
}

bool SelectionInputMonitor::GetPanelShowingStatus() const
{
    return isPanelShowing_.load();
}