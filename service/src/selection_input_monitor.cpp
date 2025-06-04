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
#include "selection_input_monitor.h"
#include "screenlock_manager.h"

using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::MMI;
using namespace OHOS::EventFwk;

uint32_t SelectionInputMonitor::curSelectState = SELECT_INPUT_INITIAL;
uint32_t SelectionInputMonitor::subSelectState = SUB_INITIAL;
int64_t SelectionInputMonitor::lastClickTime = 0;
bool SelectionInputMonitor::ctrlSelectFlag = false;
bool SelectionInputMonitor::lastTextSelectedFlag = false;


static int64_t GetCurrentTimeMillis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void DefaultSelectionEventListener::OnTextSelected(std::shared_ptr<SelectionDataInner> selectionData)
{
    SELECTION_HILOGI("End word selection action.");
    InjectCtrlC();
    SELECTION_HILOGI("End Inject Ctrl + C.");
    sptr<ISelectionListener> listener = SelectionService::GetInstance()->GetListener();
    if (listener == nullptr) {
        SELECTION_HILOGE("get listener is null");
        return;
    }
    SelectionDataInner data;
    data.selectionType = MOVE_SELECTION;
    data.text = "Hello, world!";
    data.startPosX = 0;
    data.startPosY = 13;
    data.endPosX = 0;
    data.endPosY = 13;
    data.windowId = 1001;
    data.bundleID = 2002;
    listener->OnSelectionChange(data);
    return;
}

bool SelectionInputMonitor::IsTextSelected() const {
    if (curSelectState != SELECT_INPUT_LEFT_MOVE && curSelectState != SELECT_INPUT_DOUBLE_CLICKED &&
        curSelectState != SELECT_INPUT_TRIPLE_CLICKED) {
        return false;
    }
    return true;
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    if (!ctrlSelectFlag) {
        return;
    }

    if (subSelectState != SUB_WAIT_KEY_CTRL_DOWN && subSelectState != SUB_WAIT_KEY_CTRL_UP) {
        return;
    }

    int32_t keyCode = keyEvent->GetKeyCode();
    int32_t action = keyEvent->GetKeyAction();
    if (keyCode != KeyEvent::KEYCODE_CTRL_LEFT && keyCode != KeyEvent::KEYCODE_CTRL_RIGHT) {
        curSelectState = SELECT_INPUT_INITIAL;
        subSelectState = SUB_INITIAL;
        SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_INITIAL.");
        return;
    }
    SELECTION_HILOGI("[SelectionService] Processed ctrl key.");
    if (subSelectState == SUB_WAIT_KEY_CTRL_DOWN && action == KeyEvent::KEY_ACTION_DOWN) {
        subSelectState = SUB_WAIT_KEY_CTRL_UP;
        return;
    }

    if (subSelectState == SUB_WAIT_KEY_CTRL_UP && action == KeyEvent::KEY_ACTION_UP) {
        if (curSelectState == SELECT_INPUT_WAIT_LEFT_MOVE) {
            curSelectState = SELECT_INPUT_LEFT_MOVE;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_LEFT_MOVE.");
        } else if (curSelectState == SELECT_INPUT_WAIT_DOUBLE_CLICK) {
            curSelectState = SELECT_INPUT_DOUBLE_CLICKED;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_DOUBLE_CLICKED.");
        } else if (curSelectState == SELECT_INPUT_WAIT_TRIPLE_CLICK) {
            curSelectState = SELECT_INPUT_TRIPLE_CLICKED;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_DOUBLE_CLICKED.");
        }
        subSelectState = SUB_INITIAL;
    } else {
        curSelectState = SELECT_INPUT_INITIAL;
        subSelectState = SUB_INITIAL;
        SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_INITIAL.");
    }
    FinishedWordSelection();
    return;
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    bool screenLockedFlag = OHOS::ScreenLock::ScreenLockManager::GetInstance()->IsScreenLocked();
    if (screenLockedFlag) {
        SELECTION_HILOGD("It is not screen on.");
        return;
    }
    SELECTION_HILOGI("pointerEvent->windowId: %{public}d", pointerEvent->GetTargetWindowId());
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerId, pointerItem);
    // SELECTION_HILOGI("pointerItem, display: %{public}d, %{public}d, \
    //     RawDxy: %{public}d, %{public}d. \
    //     windows: %{public}d, %{public}d, windowId: %{public}d, deviceId: %{public}d",
        // pointerItem.GetDisplayX(),pointerItem.GetDisplayY(),
    //     pointerItem.GetRawDx(),pointerItem.GetRawDy(),
    //     pointerItem.GetWindowX(), pointerItem.GetWindowY(),
    //     pointerItem.GetTargetWindowId(), pointerItem.GetDeviceId());
    if (curSelectState == SELECT_INPUT_INITIAL && pointerId != PointerEvent::MOUSE_BUTTON_LEFT) {
        // SELECTION_HILOGI("[SelectionService] into PointerEvent, pointerId = %{public}d. curSelectState: %{public}d",
            // pointerId, curSelectState);
        return;
    }
    SELECTION_HILOGD("[SelectionService] into PointerEvent, curSelectState = %{public}d.", curSelectState);

    switch (curSelectState)
    {
        case SELECT_INPUT_INITIAL:
            InputInitialProcess(pointerEvent);
            break;

        case SELECT_INPUT_WORD_BEGIN:
            InputWordBeginProcess(pointerEvent);
            break;

        case SELECT_INPUT_WAIT_LEFT_MOVE:
            InputWordWaitLeftMoveProcess(pointerEvent);
            break;

        case SELECT_INPUT_WAIT_DOUBLE_CLICK:
            InputWordWaitDoubleClickProcess(pointerEvent);
            break;

        case SELECT_INPUT_DOUBLE_CLICKED:
            InputWordJudgeTripleClickProcess(pointerEvent);
            break;

        case SELECT_INPUT_WAIT_TRIPLE_CLICK:
            InputWordWaitTripleClickProcess(pointerEvent);
            break;

        default:
            break;
    }

    FinishedWordSelection();
    return;
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const
{
    SELECTION_HILOGI("[SelectionService] into axisEvent");
};

void SelectionInputMonitor::ResetProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    curSelectState = SELECT_INPUT_INITIAL;
    subSelectState = SUB_INITIAL;
    OnInputEvent(pointerEvent);
}

void SelectionInputMonitor::InputInitialProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    int32_t buttonId = pointerEvent->GetButtonId();
    if (action == PointerEvent::POINTER_ACTION_BUTTON_DOWN && buttonId == PointerEvent::MOUSE_BUTTON_LEFT) {
        curSelectState = SELECT_INPUT_WORD_BEGIN;
        subSelectState = SUB_INITIAL;
        lastClickTime = GetCurrentTimeMillis();
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WORD_BEGIN.");
    }
    return;
}

void SelectionInputMonitor::InputWordBeginProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action == PointerEvent::POINTER_ACTION_MOVE) {
        curSelectState = SELECT_INPUT_WAIT_LEFT_MOVE;
        subSelectState = SUB_WAIT_POINTER_ACTION_BUTTON_UP;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_LEFT_MOVE.");
    } else if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        curSelectState = SELECT_INPUT_WAIT_DOUBLE_CLICK;
        subSelectState = SUB_WAIT_POINTER_ACTION_BUTTON_DOWN;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_DOUBLE_CLICK.");
    }
    return;
}

void SelectionInputMonitor::InputWordWaitLeftMoveProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        if (ctrlSelectFlag) {
            subSelectState = SUB_WAIT_KEY_CTRL_DOWN;
            SELECTION_HILOGI("set subSelectState to SUB_WAIT_KEY_CTRL_DOWN.");
        } else {
            curSelectState = SELECT_INPUT_LEFT_MOVE;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_LEFT_MOVE.");
        }
    } else if (action != PointerEvent::POINTER_ACTION_MOVE) {
        SELECTION_HILOGI("Action reset. subSelectState is %{public}d, action is %{public}d.", subSelectState, action);
        ResetProcess(pointerEvent);
    }
    return;
}

void SelectionInputMonitor::JudgeTripleClick() const
{
    auto curTime = GetCurrentTimeMillis();
    if (curTime - lastClickTime < DOUBLE_CLICK_TIME) {
        curSelectState = SELECT_INPUT_WAIT_TRIPLE_CLICK;
        subSelectState = SUB_WAIT_POINTER_ACTION_BUTTON_UP;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_TRIPLE_CLICK.");
    } else {
        curSelectState = SELECT_INPUT_WORD_BEGIN;
        subSelectState = SUB_INITIAL;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WORD_BEGIN.");
    }
    lastClickTime = curTime;
}

void SelectionInputMonitor::InputWordWaitDoubleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (subSelectState == SUB_WAIT_POINTER_ACTION_BUTTON_DOWN && action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        auto curTime = GetCurrentTimeMillis();
        if (curTime - lastClickTime < DOUBLE_CLICK_TIME) {
            subSelectState = SUB_WAIT_POINTER_ACTION_BUTTON_UP;
            SELECTION_HILOGI("set subSelectState to SUB_WAIT_POINTER_ACTION_BUTTON_UP.");
        } else {
            curSelectState = SELECT_INPUT_WORD_BEGIN;
            subSelectState = SUB_INITIAL;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WORD_BEGIN.");
        }
        lastClickTime = curTime;
        return;
    }
    if (subSelectState == SUB_WAIT_POINTER_ACTION_BUTTON_UP) {
        if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
            if (ctrlSelectFlag) {
                subSelectState = SUB_WAIT_KEY_CTRL_DOWN;
                SELECTION_HILOGI("set subSelectState to SUB_WAIT_KEY_CTRL_DOWN.");
            } else {
                curSelectState = SELECT_INPUT_DOUBLE_CLICKED;
                subSelectState = SUB_INITIAL;
                SELECTION_HILOGI("set curSelectState to SELECT_INPUT_DOUBLE_CLICKED.");
            }
        } else if (action == PointerEvent::POINTER_ACTION_MOVE) {
            curSelectState = SELECT_INPUT_WAIT_LEFT_MOVE;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_LEFT_MOVE.");
        }
        return;
    }
    if (subSelectState == SUB_WAIT_KEY_CTRL_DOWN && action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        JudgeTripleClick();
    }
    return;
}

void SelectionInputMonitor::InputWordJudgeTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (subSelectState == SUB_INITIAL && action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
         SELECTION_HILOGI("Begin JudgeTripleClick.");
        JudgeTripleClick();
    }  else {
        SELECTION_HILOGI("Action reset. subSelectState is %{public}d, action is %{public}d.", subSelectState, action);
        ResetProcess(pointerEvent);
    }
}

void SelectionInputMonitor::InputWordWaitTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (subSelectState != SUB_WAIT_POINTER_ACTION_BUTTON_UP) {
        return;
    }
    if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        if (ctrlSelectFlag) {
            subSelectState = SUB_WAIT_KEY_CTRL_DOWN;
            SELECTION_HILOGI("set subSelectState to SUB_WAIT_KEY_CTRL_DOWN.");
        } else {
            curSelectState = SELECT_INPUT_TRIPLE_CLICKED;
            subSelectState = SUB_INITIAL;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_TRIPLE_CLICKED.");
        }
    } else if (action == PointerEvent::POINTER_ACTION_MOVE) {
        curSelectState = SELECT_INPUT_WAIT_LEFT_MOVE;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_LEFT_MOVE.");
    } else {
        SELECTION_HILOGI("Action reset. subSelectState is %{public}d, action is %{public}d.", subSelectState, action);
        ResetProcess(pointerEvent);
    }
    return;
}

void SelectionInputMonitor::FinishedWordSelection() const
{
    if (!IsTextSelected()) {
        lastTextSelectedFlag = false;
        return;
    }
    lastTextSelectedFlag = true;
    // world selection action
    if (curSelectState != SELECT_INPUT_DOUBLE_CLICKED) {
        curSelectState = SELECT_INPUT_INITIAL;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_INITIAL");
    }

    std::shared_ptr<SelectionDataInner> selectionData = std::make_shared<SelectionDataInner>();

    selectionEventListener_->OnTextSelected(selectionData);
}

void DefaultSelectionEventListener::SimulateKeyWithCtrl(int32_t keyCode, int32_t keyAction)
{
    auto KeyEvent = KeyEvent::Create();
    KeyEvent->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    KeyEvent->SetKeyAction(keyAction);
    KeyEvent::KeyItem item1, item2;
    item1.SetPressed(keyAction == KeyEvent::KEY_ACTION_DOWN);
    item1.SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    item2.SetPressed(keyAction == KeyEvent::KEY_ACTION_DOWN);
    item2.SetKeyCode(keyCode);
    KeyEvent->AddKeyItem(item1);
    KeyEvent->AddKeyItem(item2);
    InputManager::GetInstance()->SimulateInputEvent(KeyEvent);
}

void DefaultSelectionEventListener::InjectCtrlC()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    SimulateKeyWithCtrl(KeyEvent::KEYCODE_C, KeyEvent::KEY_ACTION_DOWN);
    SimulateKeyWithCtrl(KeyEvent::KEYCODE_C, KeyEvent::KEY_ACTION_UP);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // // 创建KeyEvent对象
    // auto keyEvent1 = KeyEvent::Create();

    // // 设置Ctrl键按下
    // int deviceId = 100;
    // int downTime = 104329296;
    // keyEvent1->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    // keyEvent1->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    // keyEvent1->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    // keyEvent1->SetDeviceId(deviceId);
    // KeyEvent::KeyItem item1;
    // item1.SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    // item1.SetPressed(true);
    // item1.SetDeviceId(deviceId);
    // item1.SetDownTime(downTime);
    // keyEvent1->AddKeyItem(item1);
    // InputManager::GetInstance()->SimulateInputEvent(keyEvent1);

    // // 设置C键按下
    // auto keyEvent2 = KeyEvent::Create();
    // keyEvent2->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    // keyEvent2->SetKeyCode(KeyEvent::KEYCODE_C);
    // keyEvent2->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    // keyEvent2->SetDeviceId(deviceId);
    // KeyEvent::KeyItem item2;
    // item2.SetKeyCode(KeyEvent::KEYCODE_C);
    // item2.SetPressed(true);
    // item2.SetDeviceId(deviceId);
    // item2.SetUnicode(99);
    // item2.SetDownTime(downTime);
    // keyEvent2->AddKeyItem(item1);
    // keyEvent2->AddKeyItem(item2);
    // InputManager::GetInstance()->SimulateInputEvent(keyEvent2);

    // // 设置C键释放
    // auto keyEvent3 = KeyEvent::Create();
    // keyEvent3->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    // keyEvent3->SetKeyCode(KeyEvent::KEYCODE_C);
    // keyEvent3->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    // keyEvent3->SetDeviceId(deviceId);
    // KeyEvent::KeyItem item3;
    // item3.SetKeyCode(KeyEvent::KEYCODE_C);
    // item3.SetPressed(false);
    // item3.SetDeviceId(deviceId);
    // item3.SetUnicode(99);
    // item3.SetDownTime(downTime);
    // keyEvent3->AddKeyItem(item1);
    // keyEvent3->AddKeyItem(item3);
    // InputManager::GetInstance()->SimulateInputEvent(keyEvent3);

    // // 设置Ctrl键释放
    // auto keyEvent4 = KeyEvent::Create();
    // keyEvent4->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    // keyEvent4->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    // keyEvent4->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    // keyEvent4->SetDeviceId(deviceId);
    // KeyEvent::KeyItem item4;
    // item4.SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    // item4.SetPressed(false);
    // item4.SetDeviceId(deviceId);
    // item4.SetDownTime(downTime);
    // keyEvent4->AddKeyItem(item4);
    // InputManager::GetInstance()->SimulateInputEvent(keyEvent4);
}
