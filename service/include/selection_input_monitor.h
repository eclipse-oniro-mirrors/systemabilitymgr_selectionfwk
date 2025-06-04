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

#ifndef SELECTION_INPUT_MONITOR_H
#define SELECTION_INPUT_MONITOR_H

#include <string>
#include <i_input_event_consumer.h>

namespace OHOS::SelectionFwk {
using namespace MMI;

constexpr const uint32_t DOUBLE_CLICK_TIME = 500;

typedef enum {
    SELECT_INPUT_INITIAL = 0,
    SELECT_INPUT_WORD_BEGIN = 1,
    SELECT_INPUT_WAIT_LEFT_MOVE = 2,
    SELECT_INPUT_LEFT_MOVE = 3,
    SELECT_INPUT_WAIT_DOUBLE_CLICK = 4,
    SELECT_INPUT_WAIT_TRIPLE_CLICK = 5,
    SELECT_INPUT_DOUBLE_CLICKED = 6,
    SELECT_INPUT_TRIPLE_CLICKED = 7,
} SelectInputState;

typedef enum {
    SUB_INITIAL = 0,
    SUB_WAIT_POINTER_ACTION_BUTTON_DOWN = 1,
    SUB_WAIT_POINTER_ACTION_BUTTON_UP   = 2,
    SUB_WAIT_KEY_CTRL_DOWN = 3,
    SUB_WAIT_KEY_CTRL_UP = 4,
} SelectInputSubState;

struct SelectionDataInner;

class SelectionEventListener {
public:
    virtual void OnTextSelected(std::shared_ptr<SelectionDataInner> selectionData) {
    };
    virtual ~SelectionEventListener() = default;
};

class DefaultSelectionEventListener : public SelectionEventListener {
public:
    virtual void OnTextSelected(std::shared_ptr<SelectionDataInner> selectionData);

private:
    void InjectCtrlC();
    void SimulateKeyWithCtrl(int32_t keyCode, int32_t keyAction);
};

class SelectionInputMonitor : public IInputEventConsumer {
public:
    SelectionInputMonitor()
        : selectionEventListener_(std::make_shared<SelectionEventListener>()) {
    }

    SelectionInputMonitor(std::shared_ptr<SelectionEventListener> selectionEventListener)
        : selectionEventListener_(selectionEventListener) {
    }

    virtual void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const;
    virtual void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const;
    virtual void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const;

public:
    static bool ctrlSelectFlag;
    static bool lastTextSelectedFlag;

private:
    void InputInitialProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordBeginProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordWaitLeftMoveProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordWaitDoubleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordJudgeTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordWaitTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void FinishedWordSelection() const;
    void ResetProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void JudgeTripleClick() const;
    bool IsTextSelected() const;

    static uint32_t curSelectState;
    static uint32_t subSelectState;
    static int64_t lastClickTime;

    std::shared_ptr<SelectionEventListener> selectionEventListener_;
};
}

#endif // SELECTION_INPUT_MONITOR_H