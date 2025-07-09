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
#include <vector>
#include <i_input_event_consumer.h>
#include "selection_interface.h"
#include "pasteboard_disposable_observer.h"
namespace OHOS::SelectionFwk {
using namespace MMI;
using namespace OHOS::MiscServices;

constexpr const uint32_t DOUBLE_CLICK_TIME = 500;
constexpr const uint32_t MAX_PASTERBOARD_TEXT_LENGTH = 2000;
constexpr const uint32_t BYTES_PER_CHINESE_CHAR = 3;  // In UTF-8, common Chinese characters occupy 3 bytes.

enum class SelectInputState: uint32_t {
    SELECT_INPUT_INITIAL = 0,
    SELECT_INPUT_WORD_BEGIN = 1,
    SELECT_INPUT_WAIT_LEFT_MOVE = 2,
    SELECT_INPUT_LEFT_MOVE = 3,
    SELECT_INPUT_WAIT_DOUBLE_CLICK = 4,
    SELECT_INPUT_WAIT_TRIPLE_CLICK = 5,
    SELECT_INPUT_DOUBLE_CLICKED = 6,
    SELECT_INPUT_TRIPLE_CLICKED = 7,
};

enum class SelectInputSubState: uint32_t {
    SUB_INITIAL = 0,
    SUB_WAIT_POINTER_ACTION_BUTTON_DOWN = 1,
    SUB_WAIT_POINTER_ACTION_BUTTON_UP   = 2,
    SUB_WAIT_KEY_CTRL_DOWN = 3,
    SUB_WAIT_KEY_CTRL_UP = 4,
};

class BaseSelectionInputMonitor : public IInputEventConsumer {
public:
    BaseSelectionInputMonitor() {
    }

    virtual void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const;
    virtual void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const;
    virtual void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const;

    void ResetFinishedState() const;
    bool IsTextSelected() const;
    const SelectionInfo& GetSelectionInfo() const;

public:
    static bool ctrlSelectFlag;

private:
    void InputInitialProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordBeginProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordWaitLeftMoveProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordWaitDoubleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordJudgeTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordWaitTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void FinishedWordSelection() const;
    void ResetProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void ResetState() const;
    void JudgeTripleClick() const;
    void SaveSelectionStartInfo(std::shared_ptr<PointerEvent> pointerEvent) const;
    void SaveSelectionEndInfo(std::shared_ptr<PointerEvent> pointerEvent) const;
    void SaveSelectionType() const;
    bool IsSelectionDone() const;

private:
    mutable SelectInputState curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
    mutable SelectInputSubState subSelectState = SelectInputSubState::SUB_INITIAL;
    mutable int64_t lastClickTime = 0;
    mutable bool isTextSelected_ = false;
    mutable SelectionInfo selectionInfo_;
};

class SelectionPasteboardDisposableObserver : public PasteboardDisposableObserver {
public:
    SelectionPasteboardDisposableObserver(const std::shared_ptr<BaseSelectionInputMonitor> &baseInputMonitor)
        : baseInputMonitor_(baseInputMonitor) {
    }
    virtual ~SelectionPasteboardDisposableObserver() = default;

    void OnTextReceived(const std::string &text, int32_t errCode) override;

private:
    std::shared_ptr<BaseSelectionInputMonitor> baseInputMonitor_;
};

class SelectionInputMonitor : public IInputEventConsumer {
public:
    SelectionInputMonitor();

    virtual void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const;
    virtual void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const;
    virtual void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const;

private:
    void FinishedWordSelection() const;
    void InjectCtrlC() const;
    void HandleWindowFocused(std::shared_ptr<PointerEvent> pointerEvent) const;
    bool IsAppInBlacklist(const std::string& bundleName) const;

private:
    std::shared_ptr<BaseSelectionInputMonitor> baseInputMonitor_;
    mutable sptr<SelectionPasteboardDisposableObserver> pasteboardObserver_;
    std::vector<std::string> appBlacklist_;
};
}

#endif // SELECTION_INPUT_MONITOR_H