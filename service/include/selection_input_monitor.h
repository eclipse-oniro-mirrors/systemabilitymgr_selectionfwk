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

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <string>
#include <unordered_set>
#include <i_input_event_consumer.h>
#include "selection_interface.h"
#include "pasteboard_disposable_observer.h"

namespace OHOS::SelectionFwk {
using namespace MMI;
using namespace OHOS::MiscServices;

constexpr const uint32_t DOUBLE_CLICK_TIME = 550;
constexpr const uint32_t TRIPLE_CLICK_TIME = 300;
constexpr const uint32_t MAX_PASTERBOARD_TEXT_LENGTH = 2000;
constexpr const uint32_t BYTES_PER_CHINESE_CHAR = 3;  // In UTF-8, common Chinese characters occupy 3 bytes.
constexpr const uint32_t MAX_POSITION_CHANGE_OFFSET = 10;

enum class SelectInputState : uint32_t {
    SELECT_INPUT_INITIAL = 0,
    SELECT_INPUT_WORD_BEGIN,
    SELECT_INPUT_WAIT_LEFT_MOVE,
    SELECT_INPUT_WAIT_DOUBLE_CLICK,
    SELECT_INPUT_WAIT_TRIPLE_CLICK,
    SELECT_INPUT_WORD_END,
    SELECT_INPUT_DONE,
};

enum class SelectInputSubState : uint32_t {
    SUB_INITIAL = 0,
    SUB_WAIT_POINTER_ACTION_BUTTON_DOWN,
    SUB_WAIT_POINTER_ACTION_BUTTON_UP,
    SUB_WAIT_KEY_CTRL_DOWN,
    SUB_WAIT_KEY_CTRL_UP,
    SUB_WAIT_BUTTON_OR_CTRL_DOWN,
};

class BaseSelectionInputMonitor : public IInputEventConsumer {
public:
    BaseSelectionInputMonitor() {
    }

    virtual void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const;
    virtual void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const;
    virtual void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const;

    virtual bool IsSelectionTriggered() const;
    virtual const SelectionInfo& GetSelectionInfo() const;

private:
    void ProcessInputEvent(std::shared_ptr<KeyEvent> keyEvent) const;
    void ProcessInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const;

    void InputInitialProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordBeginProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void ProcessWordSelection(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordLeftMoveProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordDoubleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordEndProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputDoneProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordTripleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void FinishedWordSelection() const;
    void ResetProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void ResetState() const;
    void JudgeTripleClick(std::shared_ptr<PointerEvent> pointerEvent) const;
    void SaveSelectionStartInfo(std::shared_ptr<PointerEvent> pointerEvent) const;
    void SaveSelectionEndInfo(std::shared_ptr<PointerEvent> pointerEvent) const;
    void SaveSelectionType() const;
    bool IsSelectionDone() const;
    bool GetCtrlSelectFlag() const;
    bool IsClickTimeout(uint32_t time) const;
    bool ProcessMovement(std::shared_ptr<PointerEvent> pointerEvent) const;
    bool IsTinyMovement(std::shared_ptr<PointerEvent> pointerEvent) const;

private:
    mutable SelectInputState curSelectState = SelectInputState::SELECT_INPUT_INITIAL;
    mutable SelectInputSubState subSelectState = SelectInputSubState::SUB_INITIAL;
    mutable int64_t lastClickTime = 0;
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
    ~SelectionInputMonitor();

    virtual void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const;
    virtual void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const;
    virtual void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const;

    int32_t GetSelectionContent(std::string& selectionContent);
    bool CanGetSelectionContent() const;

    bool GetCanGetSelectionContentFlag() const;
    void SetCanGetSelectionContentFlag(bool flag) const;

private:
    void InitUidev();
    void FinishedWordSelection() const;
    int32_t InjectCtrlC() const;
    void HandleWindowFocused(std::shared_ptr<PointerEvent> pointerEvent) const;
    bool IsAppInBlocklist(const std::string& bundleName) const;

private:
    std::shared_ptr<BaseSelectionInputMonitor> baseInputMonitor_;
    mutable sptr<SelectionPasteboardDisposableObserver> pasteboardObserver_;
    int fd_ = -1;
    struct uinput_user_dev uidev_;
    mutable bool canGetSelectionContentFlag_ = false;
};
}

#endif // SELECTION_INPUT_MONITOR_H