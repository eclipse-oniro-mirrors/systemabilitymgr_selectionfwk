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

#include "selection_log.h"
#include <input_manager.h>
#include "common_event_manager.h"
#include "pasteboard_client.h"
#include "selection_config.h"
#include "selection_input_monitor.h"
#include "window_manager.h"

using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::MMI;
using namespace OHOS::EventFwk;

bool BaseSelectionInputMonitor::ctrlSelectFlag = false;
std::atomic<uint32_t> selSeqId = 0;

static int64_t GetCurrentTimeMillis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint32_t GenerateSequenceId()
{
    return selSeqId.fetch_add(1, std::memory_order_seq_cst);
}

bool BaseSelectionInputMonitor::IsTextSelected() const {
    return isTextSelected_;
}

const SelectionInfo& BaseSelectionInputMonitor::GetSelectionInfo() const {
    return selectionInfo_;
}

void BaseSelectionInputMonitor::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    if (!ctrlSelectFlag) {
        return;
    }

    if (subSelectState != SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN &&
        subSelectState != SelectInputSubState::SUB_WAIT_KEY_CTRL_UP) {
        return;
    }

    int32_t keyCode = keyEvent->GetKeyCode();
    int32_t action = keyEvent->GetKeyAction();
    if (keyCode != KeyEvent::KEYCODE_CTRL_LEFT && keyCode != KeyEvent::KEYCODE_CTRL_RIGHT) {
        curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
        subSelectState = SelectInputSubState::SUB_INITIAL;
        SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_INITIAL.");
        return;
    }
    SELECTION_HILOGI("[SelectionService] Processed ctrl key.");
    if (subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN && action == KeyEvent::KEY_ACTION_DOWN) {
        subSelectState = SelectInputSubState::SUB_WAIT_KEY_CTRL_UP;
        return;
    }

    if (subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_UP && action == KeyEvent::KEY_ACTION_UP) {
        if (curSelectState == SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE) {
            curSelectState = SelectInputState::SELECT_INPUT_LEFT_MOVE;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_LEFT_MOVE.");
        } else if (curSelectState == SelectInputState::SELECT_INPUT_WAIT_DOUBLE_CLICK) {
            curSelectState = SelectInputState::SELECT_INPUT_DOUBLE_CLICKED;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_DOUBLE_CLICKED.");
        } else if (curSelectState == SelectInputState::SELECT_INPUT_WAIT_TRIPLE_CLICK) {
            curSelectState = SelectInputState::SELECT_INPUT_TRIPLE_CLICKED;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_DOUBLE_CLICKED.");
        }
        subSelectState = SelectInputSubState::SUB_INITIAL;
    } else {
        curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
        subSelectState = SelectInputSubState::SUB_INITIAL;
        SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_INITIAL.");
    }
    FinishedWordSelection();
    return;
}

void BaseSelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t buttonId = pointerEvent->GetButtonId();
    if (subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN ||
        subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_UP) {
        int32_t action = pointerEvent->GetPointerAction();
        if (buttonId == PointerEvent::BUTTON_NONE && action == PointerEvent::POINTER_ACTION_MOVE) {
            return;
        }
    }
    if (buttonId != PointerEvent::MOUSE_BUTTON_LEFT) {
        ResetState();
    }
    SELECTION_HILOGD("[SelectionService] into PointerEvent, curSelectState = %{public}d.", curSelectState);

    switch (curSelectState)
    {
        case SelectInputState::SELECT_INPUT_INITIAL:
            InputInitialProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WORD_BEGIN:
            InputWordBeginProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE:
            InputWordWaitLeftMoveProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WAIT_DOUBLE_CLICK:
            InputWordWaitDoubleClickProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_DOUBLE_CLICKED:
            InputWordJudgeTripleClickProcess(pointerEvent);
            break;

        case SelectInputState::SELECT_INPUT_WAIT_TRIPLE_CLICK:
            InputWordWaitTripleClickProcess(pointerEvent);
            break;

        default:
            break;
    }

    FinishedWordSelection();
    return;
}

void BaseSelectionInputMonitor::OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const
{
    SELECTION_HILOGI("[SelectionService] into axisEvent");
};

void BaseSelectionInputMonitor::ResetProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
    subSelectState = SelectInputSubState::SUB_INITIAL;
    OnInputEvent(pointerEvent);
}

void BaseSelectionInputMonitor::SaveSelectionStartInfo(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerId, pointerItem);
    selectionInfo_.startDisplayX = pointerItem.GetDisplayX();
    selectionInfo_.startDisplayY = pointerItem.GetDisplayY();
    selectionInfo_.endDisplayX = pointerItem.GetDisplayX();
    selectionInfo_.endDisplayY = pointerItem.GetDisplayY();
    selectionInfo_.startWindowX = pointerItem.GetWindowX();
    selectionInfo_.startWindowY = pointerItem.GetWindowY();
    selectionInfo_.endWindowX = pointerItem.GetWindowX();
    selectionInfo_.endWindowY = pointerItem.GetWindowY();
    selectionInfo_.displayId = pointerEvent->GetTargetDisplayId();
    selectionInfo_.windowId = pointerEvent->GetTargetWindowId();

    OHOS::Rosen::WindowInfoOption windowInfoOption;
    windowInfoOption.displayId = selectionInfo_.displayId;
    windowInfoOption.windowId = selectionInfo_.windowId;
    std::vector<sptr<Rosen::WindowInfo>> infos;
    Rosen::WMError ret = Rosen::WindowManager::GetInstance().ListWindowInfo(windowInfoOption, infos);
    SELECTION_HILOGI("ListWindowInfo ret: %{public}d, infos size: %{public}zu", ret, infos.size());
    for (unsigned int i = 0; i < infos.size(); i++) {
        auto info = infos[i];
        SELECTION_HILOGI("ListWindowInfo bundleName: %{public}s, windowtype:%{public}d, width:%{public}d, \
        height:%{public}d", info->windowMetaInfo.bundleName.c_str(), info->windowMetaInfo.windowType,
        info->windowLayoutInfo.rect.width_, info->windowLayoutInfo.rect.height_);
    }
    if (ret == Rosen::WMError::WM_OK && infos.size() > 0) {
        selectionInfo_.bundleName = infos[0]->windowMetaInfo.bundleName;
    }
}

void BaseSelectionInputMonitor::SaveSelectionEndInfo(std::shared_ptr<PointerEvent> pointerEvent) const
{
    if (curSelectState != SelectInputState::SELECT_INPUT_LEFT_MOVE &&
        curSelectState != SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE) {
        return;
    }
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerId, pointerItem);
    selectionInfo_.endDisplayX = pointerItem.GetDisplayX();
    selectionInfo_.endDisplayY = pointerItem.GetDisplayY();
    selectionInfo_.endWindowX = pointerItem.GetWindowX();
    selectionInfo_.endWindowY = pointerItem.GetWindowY();
}

void BaseSelectionInputMonitor::SaveSelectionType() const
{
    switch (curSelectState) {
        case SelectInputState::SELECT_INPUT_LEFT_MOVE:
            selectionInfo_.selectionType = MOVE_SELECTION;
            break;

        case SelectInputState::SELECT_INPUT_DOUBLE_CLICKED:
            selectionInfo_.selectionType = DOUBLE_CLICKED_SELECTION;
            break;

        case SelectInputState::SELECT_INPUT_TRIPLE_CLICKED:
            selectionInfo_.selectionType = TRIPLE_CLICKED_SELECTION;
            break;

        default:
            break;
    }
}

bool BaseSelectionInputMonitor::IsSelectionDone() const {
    if (curSelectState != SelectInputState::SELECT_INPUT_LEFT_MOVE &&
        curSelectState != SelectInputState::SELECT_INPUT_DOUBLE_CLICKED &&
        curSelectState != SelectInputState::SELECT_INPUT_TRIPLE_CLICKED) {
        return false;
    }
    return true;
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

void BaseSelectionInputMonitor::InputWordWaitLeftMoveProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        if (ctrlSelectFlag) {
            subSelectState = SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN;
            SELECTION_HILOGI("set subSelectState to SUB_WAIT_KEY_CTRL_DOWN.");
        } else {
            curSelectState = SelectInputState::SELECT_INPUT_LEFT_MOVE;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_LEFT_MOVE.");
        }
        SaveSelectionEndInfo(pointerEvent);
    } else if (action != PointerEvent::POINTER_ACTION_MOVE) {
        SELECTION_HILOGI("Action reset. subSelectState is %{public}d, action is %{public}d.", subSelectState, action);
        ResetProcess(pointerEvent);
    }
    return;
}

void BaseSelectionInputMonitor::JudgeTripleClick() const
{
    auto curTime = GetCurrentTimeMillis();
    if (curTime - lastClickTime < DOUBLE_CLICK_TIME) {
        curSelectState = SelectInputState::SELECT_INPUT_WAIT_TRIPLE_CLICK;
        subSelectState = SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_TRIPLE_CLICK.");
    } else {
        curSelectState = SelectInputState::SELECT_INPUT_WORD_BEGIN;
        subSelectState = SelectInputSubState::SUB_INITIAL;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WORD_BEGIN.");
    }
    lastClickTime = curTime;
}

void BaseSelectionInputMonitor::InputWordWaitDoubleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (subSelectState == SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_DOWN &&
        action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        auto curTime = GetCurrentTimeMillis();
        if (curTime - lastClickTime < DOUBLE_CLICK_TIME) {
            subSelectState = SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP;
            SELECTION_HILOGI("set subSelectState to SUB_WAIT_POINTER_ACTION_BUTTON_UP.");
        } else {
            curSelectState = SelectInputState::SELECT_INPUT_WORD_BEGIN;
            subSelectState = SelectInputSubState::SUB_INITIAL;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WORD_BEGIN.");
        }
        lastClickTime = curTime;
        return;
    }
    if (subSelectState == SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP) {
        if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
            if (ctrlSelectFlag) {
                subSelectState = SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN;
                SELECTION_HILOGI("set subSelectState to SUB_WAIT_KEY_CTRL_DOWN.");
            } else {
                curSelectState = SelectInputState::SELECT_INPUT_DOUBLE_CLICKED;
                subSelectState = SelectInputSubState::SUB_INITIAL;
                SELECTION_HILOGI("set curSelectState to SELECT_INPUT_DOUBLE_CLICKED.");
            }
            SaveSelectionEndInfo(pointerEvent);
        } else if (action == PointerEvent::POINTER_ACTION_MOVE) {
            curSelectState = SelectInputState::SELECT_INPUT_WAIT_LEFT_MOVE;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_LEFT_MOVE.");
        }
        return;
    }
    if (subSelectState == SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN &&
        action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        JudgeTripleClick();
    }
    return;
}

void BaseSelectionInputMonitor::InputWordJudgeTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (subSelectState == SelectInputSubState::SUB_INITIAL && action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
         SELECTION_HILOGI("Begin JudgeTripleClick.");
        JudgeTripleClick();
    }  else {
        SELECTION_HILOGI("Action reset. subSelectState is %{public}d, action is %{public}d.", subSelectState, action);
        ResetProcess(pointerEvent);
    }
}

void BaseSelectionInputMonitor::InputWordWaitTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (subSelectState != SelectInputSubState::SUB_WAIT_POINTER_ACTION_BUTTON_UP) {
        return;
    }
    if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        if (ctrlSelectFlag) {
            subSelectState = SelectInputSubState::SUB_WAIT_KEY_CTRL_DOWN;
            SELECTION_HILOGI("set subSelectState to SUB_WAIT_KEY_CTRL_DOWN.");
        } else {
            curSelectState = SelectInputState::SELECT_INPUT_TRIPLE_CLICKED;
            subSelectState = SelectInputSubState::SUB_INITIAL;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_TRIPLE_CLICKED.");
        }
        SaveSelectionEndInfo(pointerEvent);
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
    SELECTION_HILOGI("[selectevent] curSelectState:%{public}d. Selection event id is %{public}u.", curSelectState,
        selSeqId.load());
    isTextSelected_ = true;
    SaveSelectionType();
}

void BaseSelectionInputMonitor::ResetFinishedState() const
{
    if (curSelectState != SelectInputState::SELECT_INPUT_DOUBLE_CLICKED) {
        curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_INITIAL");
    }
    isTextSelected_ = false;
}

void BaseSelectionInputMonitor::ResetState() const
{
    isTextSelected_ = false;
    curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
    subSelectState = SelectInputSubState::SUB_INITIAL;
    SELECTION_HILOGD("ResetFinishedState.");
}

SelectionInputMonitor::SelectionInputMonitor() {
    baseInputMonitor_ = std::make_shared<BaseSelectionInputMonitor>();
    appBlacklist_ = {
        "com.huawei.hmos.hishell",
        "com.huawei.hmos.filemanager"
    };
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    baseInputMonitor_->OnInputEvent(keyEvent);
    FinishedWordSelection();
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    HandleWindowFocused(pointerEvent);
    baseInputMonitor_->OnInputEvent(pointerEvent);
    FinishedWordSelection();
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const
{
    baseInputMonitor_->OnInputEvent(axisEvent);
}

void SelectionInputMonitor::HandleWindowFocused(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action != PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        return;
    }
    auto windowId = pointerEvent->GetTargetWindowId();
    Rosen::FocusChangeInfo focusInfo;
    Rosen::WindowManager::GetInstance().GetFocusWindowInfo(focusInfo);
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

bool SelectionInputMonitor::IsAppInBlacklist(const std::string& bundleName) const {
    return std::find(appBlacklist_.begin(), appBlacklist_.end(), bundleName) != appBlacklist_.end();
}

void SelectionInputMonitor::FinishedWordSelection() const
{
    if (!baseInputMonitor_->IsTextSelected()) {
        return;
    }
    baseInputMonitor_->ResetFinishedState();
    if (pasteboardObserver_ == nullptr) {
         pasteboardObserver_ = sptr<SelectionPasteboardDisposableObserver>::MakeSptr(baseInputMonitor_);
    }

    auto selectionInfo = baseInputMonitor_->GetSelectionInfo();
    if (IsAppInBlacklist(selectionInfo.bundleName)) {
        SELECTION_HILOGW("The app [%{public}s] is in the blacklist, skip notifying selection info.",
            selectionInfo.bundleName.c_str());
        return;
    }
    if (!MemSelectionConfig::GetInstance().GetEnable()) {
        SELECTION_HILOGI("Selection switch is off, skip notifying selection info.");
        return;
    }

    int32_t ret = PasteboardClient::GetInstance()->SubscribeDisposableObserver(pasteboardObserver_,
        selectionInfo.bundleName, DisposableType::PLAIN_TEXT, MAX_PASTERBOARD_TEXT_LENGTH * BYTES_PER_CHINESE_CHAR);
    SELECTION_HILOGI("[selectevent] Call pasteboard interface. Selection event id is %{public}u. \
        Error code is %{public}d.", selSeqId.load(), ret);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("Failed to SubscribeDisposableObserver, ret: %{public}d.", ret);
        return;
    }

    InjectCtrlC();
    SELECTION_HILOGI("[selectevent] Inject Ctrl+C. Selection event id is %{public}u.", selSeqId.load());
}

void SelectionInputMonitor::InjectCtrlC() const
{
    // 创建KeyEvent对象
    auto keyEvent1 = KeyEvent::Create();

    // 设置Ctrl键按下
    keyEvent1->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    keyEvent1->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent1->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    KeyEvent::KeyItem item1;
    item1.SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    item1.SetPressed(true);
    keyEvent1->AddKeyItem(item1);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent1);

    // 设置C键按下
    auto keyEvent2 = KeyEvent::Create();
    keyEvent2->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    keyEvent2->SetKeyCode(KeyEvent::KEYCODE_C);
    keyEvent2->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    KeyEvent::KeyItem item2;
    item2.SetKeyCode(KeyEvent::KEYCODE_C);
    item2.SetPressed(true);
    keyEvent2->AddKeyItem(item1);
    keyEvent2->AddKeyItem(item2);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent2);

    // 设置C键释放
    auto keyEvent3 = KeyEvent::Create();
    keyEvent3->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    keyEvent3->SetKeyCode(KeyEvent::KEYCODE_C);
    keyEvent3->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    KeyEvent::KeyItem item3;
    item3.SetKeyCode(KeyEvent::KEYCODE_C);
    item3.SetPressed(false);
    keyEvent3->AddKeyItem(item1);
    keyEvent3->AddKeyItem(item3);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent3);

    // 设置Ctrl键释放
    auto keyEvent4 = KeyEvent::Create();
    keyEvent4->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    keyEvent4->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent4->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    KeyEvent::KeyItem item4;
    item4.SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    item4.SetPressed(false);
    keyEvent4->AddKeyItem(item4);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent4);
}

void SelectionPasteboardDisposableObserver::OnTextReceived(const std::string &text, int32_t errCode)
{
    SELECTION_HILOGI("[selectevent] Pasteboard call sa. Selection event id is %{public}u.", selSeqId.load());
    SELECTION_HILOGI("Text received length: %{public}u, errCode: %{public}d.", text.length(), errCode);
    if (errCode != 0) {
        SELECTION_HILOGI("Error receiving text, errCode: %{public}d", errCode);
        return;
    }
    if (!baseInputMonitor_) {
        return;
    }

    auto isAllWhitespace = std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    if (isAllWhitespace) {
        SELECTION_HILOGI("Received empty text or all whitespaces.");
        return;
    }

    auto selectionInfo = baseInputMonitor_->GetSelectionInfo();
    selectionInfo.text = text;
    SelectionInfoData infoData;
    infoData.data = selectionInfo;
    SELECTION_HILOGI("SelectionInfoData: %{public}s.", infoData.ToString().c_str());
    sptr<ISelectionListener> listener = SelectionService::GetInstance()->GetListener();
    if (listener == nullptr) {
        SELECTION_HILOGE("Selection listener is nullptr");
        return;
    }
    listener->OnSelectionChange(infoData);
}
