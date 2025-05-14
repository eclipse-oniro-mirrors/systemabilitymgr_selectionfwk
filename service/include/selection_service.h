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

#ifndef SELECTION_SERVICE_H
#define SELECTION_SERVICE_H

#include <mutex>
#include <string>

#include "selection_service_stub.h"
#include "refbase.h"
#include "system_ability.h"
#include <i_input_event_consumer.h>

namespace OHOS::SelectionFwk {
using namespace MMI;

constexpr const char *SYS_SELECTION_SWITCH_USERNAM = "sys.selection.switch.username";
constexpr const char *SYS_SELECTION_TRIGGER_USERNAM = "sys.selection.trigger.username";
constexpr const char *SYS_SELECTION_APP_USERNAM = "sys.selection.app.username";

typedef enum SelectInputState {
    SELECT_INPUT_INITIAL = 0,
    SELECT_INPUT_WORD_BEGIN = 1,
    SELECT_INPUT_WAIT_LEFT_MOVE = 2,
    SELECT_INPUT_LEFT_MOVE = 3,
    SELECT_INPUT_WAIT_DOUBLE_CLICK = 4,
    SELECT_INPUT_WAIT_TRIBLE_CLICK = 5,
    SELECT_INPUT_DOUBLE_CLICKED = 6,

    SELECT_INPUT_WAIT_CTRL

} SelectInputState;

class SelectionService : public SystemAbility, public SelectionServiceStub {
    DECLARE_SYSTEM_ABILITY(SelectionService);

public:
    static sptr<SelectionService> GetInstance();
    SelectionService();
    SelectionService(int32_t saId, bool runOnCreate);
    ~SelectionService();

    ErrCode AddVolume(int32_t volume, int32_t& funcResult) override;
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
protected:
    void OnStart() override;
    void OnStop() override;
    void HandleKeyEvent(int32_t keyCode);
    void HandlePointEvent(int32_t type);
private:
    void InputMonitorInit();
    void InputMonitorCancel();
    void WatchParams();

    int32_t inputMonitorId_ {-1};
    static sptr<SelectionService> instance_;
    static std::shared_mutex adminLock_;
};

class SelectionInputMonitor : public IInputEventConsumer {
public:
    virtual void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const;
    virtual void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const;
    virtual void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const;

private:
    void InputInitialProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordBeginProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InputWordWaitLeftMoveProcess(std::shared_ptr<PointerEvent> pointerEvent) const;
    void InjectCtrlC() const;
    static uint32_t curSelectState;
};
}

#endif // SELECTION_SERVICE_H