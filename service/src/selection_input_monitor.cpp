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

#include "selection_service.h"

#include "selection_common.h"
#include "selection_log.h"
#include <input_manager.h>
#include "common_event_manager.h"
#include "pasteboard_client.h"
#include "selection_config.h"
#include "selection_input_monitor.h"
#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif
#include "selection_errors.h"
#include <mutex>
#include <condition_variable>
#include "hisysevent_adapter.h"

using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::MMI;
using namespace OHOS::EventFwk;

std::atomic<uint32_t> selSeqId = 0;
const unsigned int SLEEP_USEC_AFTER_CTRL_DOWN = 1000;
const unsigned int SLEEP_USEC_AFTER_C_DOWN = 1000;
const unsigned int SLEEP_USEC_AFTER_C_UP = 60000;
const std::unordered_set<std::string> appBlocklist = {};
std::mutex mtx;
std::condition_variable cv;
std::string g_selectionContent = "";
int32_t g_pasteBoardErrorCode = 0;
constexpr int32_t PB_ERR_OUT_OF_RANGE = 5;
constexpr int32_t PB_ERR_CANNOT_GET_CONTENT = 7;
constexpr int32_t MAX_DELAY_WAIT_FOR_PB = 110;

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
        "keyCode: %{public}d, keyAction: %{public}d",
        curSelectState, subSelectState, keyEvent->GetKeyCode(), keyEvent->GetKeyAction());
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
    pasteboardObserver_ = sptr<SelectionPasteboardDisposableObserver>::MakeSptr(baseInputMonitor_);
    InitUidev();
}

SelectionInputMonitor::~SelectionInputMonitor()
{
    if (fd_ == -1) {
        return;
    }
    if (ioctl(fd_, UI_DEV_DESTROY) < 0) {
        SELECTION_HILOGW("Failed to destroy virtual device.");
    }
    close(fd_);
    fd_ = -1;
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    baseInputMonitor_->OnInputEvent(keyEvent);
    FinishedWordSelection();
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    HandleWindowFocused(pointerEvent);
    if (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        SELECTION_HILOGD("Detect multimode event: POINTER_ACTION_BUTTON_DOWN");
        SetCanGetSelectionContentFlag(false);
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
    return canGetSelectionContentFlag_;
}

void SelectionInputMonitor::SetCanGetSelectionContentFlag(bool flag) const
{
    canGetSelectionContentFlag_ = flag;
}

void SelectionInputMonitor::HandleWindowFocused(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action != PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
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
        SELECTION_HILOGE("Selection listener is nullptr");
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

void SelectionInputMonitor::InitUidev()
{
    SELECTION_HILOGI("Begin to init uidev.");
    fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        SELECTION_HILOGE("Failed to open /dev/uinput.");
        return;
    }
    SELECTION_HILOGI("Opened /dev/uinput with fd %{public}d.", fd_);

    if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) < 0) {
        SELECTION_HILOGE("Unable to set EV_KEY event bit.");
        goto CLEAN;
    }

    if (ioctl(fd_, UI_SET_KEYBIT, KEY_LEFTCTRL) < 0) {
        SELECTION_HILOGE("Unable to set KEY_LEFTCTRL event bit.");
        goto CLEAN;
    }
    if (ioctl(fd_, UI_SET_KEYBIT, KEY_C) < 0) {
        SELECTION_HILOGE("Unable to set KEY_C event bit.");
        goto CLEAN;
    }

    memset_s(&uidev_, sizeof(uidev_), 0, sizeof(uidev_));
    if (snprintf_s(uidev_.name, UINPUT_MAX_NAME_SIZE, UINPUT_MAX_NAME_SIZE, "Selection VKeyboard") < 0) {
        SELECTION_HILOGE("Invalid arguments passed to snprintf_s.");
        goto CLEAN;
    }
    uidev_.id.bustype = BUS_USB;
    uidev_.id.vendor  = 0x1;
    uidev_.id.product = 0x1;
    uidev_.id.version = 1;

    if (write(fd_, &uidev_, sizeof(uidev_)) < 0) {
        SELECTION_HILOGE("Failed to write device config.");
        goto CLEAN;
    }

    if (ioctl(fd_, UI_DEV_CREATE) < 0) {
        SELECTION_HILOGE("Failed to create virtual device.");
        goto CLEAN;
    }
    SELECTION_HILOGI("End up init uidev.");
    return;

CLEAN:
    SELECTION_HILOGE("Failed to init uidev, clean up fd now.");
    close(fd_);
    fd_ = -1;
}

void SelectionInputMonitor::FinishedWordSelection() const
{
    if (!baseInputMonitor_->IsSelectionTriggered()) {
        return;
    }

    if (SelectionService::GetInstance()->GetScreenLockedFlag()) {
        SELECTION_HILOGW("The screen is locked, skip notifying selection info.");
        return;
    }
    auto selectionInfo = baseInputMonitor_->GetSelectionInfo();
    if (selectionInfo.bundleName.empty()) {
        SELECTION_HILOGE("Failed to get Selected bundleName, skip notifying selection info.");
        return;
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

    SELECTION_HILOGI("SelectionInfoData: %{public}s.", infoData.ToString().c_str());
    sptr<ISelectionListener> listener = SelectionService::GetInstance()->GetListener();
    if (listener == nullptr) {
        SELECTION_HILOGE("Selection listener is nullptr");
        return;
    }
    SetCanGetSelectionContentFlag(true);
    listener->OnSelectionChange(infoData);
}

int32_t PasteBoardErrorCodeToSelectionService(int32_t pasteBoardErrCode)
{
    int32_t ret = ERR_OK;
    switch (pasteBoardErrCode) {
        case ERR_OK:
            SELECTION_HILOGI("Pasteboard called OnTextReceived timeout.");
            ret = SelectionServiceError::GET_CONTENT_TIMEOUT;
            break;
        case PB_ERR_OUT_OF_RANGE:
            SELECTION_HILOGE("Selection content out of range");
            ret = SelectionServiceError::CONTENT_OUT_OF_RANGE;
            break;
        case PB_ERR_CANNOT_GET_CONTENT:
            SELECTION_HILOGE("Current application forbiden to copy.");
            ret = SelectionServiceError::CANNOT_GET_CONTENT;
            break;
        default:
            SELECTION_HILOGE("Some other error when receive content from pasteboard.");
            ret = SelectionServiceError::INVALID_DATA;
    }
    return ret;
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
    int32_t ret = PasteboardClient::GetInstance()->SubscribeDisposableObserver(pasteboardObserver_,
        selectionInfo.windowId, DisposableType::PLAIN_TEXT, MAX_PASTERBOARD_TEXT_LENGTH * BYTES_PER_CHINESE_CHAR);
    SELECTION_HILOGW("[selectevent] Call pasteboard interface. Selection event id is %{public}u. "
        "Error code is %{public}d.", selSeqId.load(), ret);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("Failed to SubscribeDisposableObserver, ret: %{public}d.", ret);
        return SelectionServiceError::INVALID_DATA;
    }

    SELECTION_HILOGI("Start to inject Ctrl+C. Selection event id is %{public}u.", selSeqId.load());
    ret = InjectCtrlC();
    if (ret != ERR_OK) {
        HisyseventAdapter::GetInstance()->ReportShowPanelFailed(selectionInfo.bundleName, ret,
            static_cast<int32_t>(SelectFailedReason::INJECT_CTRLC_FAILED));
        SELECTION_HILOGE("Failed to inject Ctrl+C");
        return SelectionServiceError::INVALID_DATA;
    }
    SELECTION_HILOGW("[selectevent] Inject Ctrl+C. Selection event id is %{public}u.", selSeqId.load());

    std::unique_lock<std::mutex> lock(mtx);
    SELECTION_HILOGI("Start wait for pasteboard OnTextReceived");
    if (cv.wait_for(lock, std::chrono::milliseconds(MAX_DELAY_WAIT_FOR_PB), [] {
        return !g_selectionContent.empty();
    })) {
        selectionContent = g_selectionContent;
        g_selectionContent = "";
        ret = ERR_OK;
    } else {
        SELECTION_HILOGE("SelectionInputMonitor::GetSelectionContent: receive content from pasteboard failed");
        ret = PasteBoardErrorCodeToSelectionService(g_pasteBoardErrorCode);
        g_pasteBoardErrorCode = ERR_OK;
    }
    return ret;
}

int32_t SelectionInputMonitor::InjectCtrlC() const
{
    SELECTION_HILOGI("InjectCtrlC to /dev/uinput using fd %{public}d.", fd_);
    if (fd_ == -1) {
        SELECTION_HILOGE("fd of /dev/uinput is invalid, skip injecting ctrl c.");
        return SelectionServiceError::INVALID_DATA;
    }

    struct input_event ev;
    auto sendEvent = [&](int type, int code, int value) {
        memset_s(&ev, sizeof(ev), 0, sizeof(ev));
        struct timeval time{};
        gettimeofday(&time, NULL);
        ev.input_event_sec = time.tv_sec;
        ev.input_event_usec = time.tv_usec;
        ev.type = type;
        ev.code = code;
        ev.value = value;
        if (write(fd_, &ev, sizeof(ev)) < 0) {
            SELECTION_HILOGE("Failed to send event {type=%{public}d, code=%{public}d, value=%{public}d}",
                type, code, value);
        }
    };

    sendEvent(EV_KEY, KEY_LEFTCTRL, 1);
    sendEvent(EV_SYN, SYN_REPORT, 0);
    usleep(SLEEP_USEC_AFTER_CTRL_DOWN);

    sendEvent(EV_KEY, KEY_C, 1);
    sendEvent(EV_SYN, SYN_REPORT, 0);
    usleep(SLEEP_USEC_AFTER_C_DOWN);

    sendEvent(EV_KEY, KEY_C, 0);
    sendEvent(EV_SYN, SYN_REPORT, 0);
    usleep(SLEEP_USEC_AFTER_C_UP);

    sendEvent(EV_KEY, KEY_LEFTCTRL, 0);
    sendEvent(EV_SYN, SYN_REPORT, 0);

    SELECTION_HILOGI("End up InjectCtrlC to /dev/uinput.");
    return ERR_OK;
}

void SelectionPasteboardDisposableObserver::OnTextReceived(const std::string &text, int32_t errCode)
{
    SELECTION_HILOGW("[selectevent] Pasteboard call sa. Selection event id is %{public}u. Text received "
        "length: %{public}u, errCode: %{public}d.", selSeqId.load(), text.length(), errCode);

    if (!baseInputMonitor_) {
        return;
    }

    if (errCode != 0) {
        auto selectionInfo = baseInputMonitor_->GetSelectionInfo();
        HisyseventAdapter::GetInstance()->ReportShowPanelFailed(selectionInfo.bundleName, errCode,
            static_cast<int32_t>(SelectFailedReason::TEXT_RECEIVE_FAILED));
        SELECTION_HILOGE("Error receiving text, errCode: %{public}d", errCode);
    }

    if (IsAllWhitespace(text)) {
        SELECTION_HILOGI("Received empty text or all whitespaces.");
    }
    SELECTION_HILOGI("Notify SelectionInputMonitor return text");
    std::lock_guard<std::mutex> lock(mtx);
    g_selectionContent = text;
    g_pasteBoardErrorCode = errCode;
    cv.notify_one();
}
