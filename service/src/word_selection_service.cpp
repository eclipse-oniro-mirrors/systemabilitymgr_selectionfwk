/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "word_selection_service.h"

#include "iremote_object.h"
#include "system_ability_definition.h"
#include "word_selection_log.h"
#include <input_manager.h>

using namespace OHOS;
REGISTER_SYSTEM_ABILITY_BY_ID(WordSelectionService, WORD_SELECTION_SA_ID, true);

WordSelectionService::WordSelectionService(int32_t saId, bool runOnCreate) : SystemAbility(saId, runOnCreate)
{
    LOG_INFO("[YMZ][WordSelectionService] saID=%{public}d runOnCreate=%{public}d", saId, runOnCreate);
}

WordSelectionService::~WordSelectionService()
{
    LOG_INFO("[YMZ][~WordSelectionService]");
}

int WordSelectionService::AddVolume(int volume)
{
    LOG_INFO("[YMZ][WordSelectionService][AddVolume]begin");
    return (volume + 1);
}

void WordSelectionService::OnStart()
{
    LOG_INFO("[YMZ][WordSelectionService][OnStart]begin");
    Publish(this);
    InputMonitorInit();
    LOG_INFO("[YMZ][WordSelectionService][OnStart]end");
}

void WordSelectionService::OnStop()
{
    LOG_INFO("[YMZ][WordSelectionService][OnStop]begin");
    InputMonitorCancel();
    LOG_INFO("[YMZ][WordSelectionService][OnStop]end");
}


void WordSelectionService::InputMonitorInit()
{
    LOG_INFO("[YMZ]WordSelection service input monitor init");
    std::shared_ptr<WordSelectionInputMonitor> inputMonitor = std::make_shared<WordSelectionInputMonitor>();
    if (inputMonitorId_ < 0) {
        inputMonitorId_ =
            InputManager::GetInstance()->AddMonitor(std::static_pointer_cast<IInputEventConsumer>(inputMonitor));
    }
}

void WordSelectionService::InputMonitorCancel()
{
    LOG_INFO("[YMZ]WordSelection service input monitor cancel");
    InputManager* inputManager = InputManager::GetInstance();
    if (inputMonitorId_ >= 0) {
        inputManager->RemoveMonitor(inputMonitorId_);
        inputMonitorId_ = -1;
    }
}

void WordSelectionService::HandleKeyEvent(int32_t keyCode)
{
#ifdef HAS_MULTIMODALINPUT_INPUT_PART
    LOG_INFO("[YMZ] keyCode: %{public}d", keyCode);
    int64_t now = static_cast<int64_t>(time(nullptr));
    if (IsScreenOn()) {
        this->RefreshActivityInner(now, UserActivityType::USER_ACTIVITY_TYPE_BUTTON, false);
    } else {
        if (keyCode == KeyEvent::KEYCODE_F1) {
            POWER_HILOGI(FEATURE_WAKEUP, "[UL_POWER] Wakeup by double click");
            std::string reason = "double click";
            reason.append(std::to_string(keyCode));
            this->WakeupDevice(now, WakeupDeviceType::WAKEUP_DEVICE_DOUBLE_CLICK, reason);
        } else if (keyCode >= KeyEvent::KEYCODE_0 && keyCode <= KeyEvent::KEYCODE_NUMPAD_RIGHT_PAREN) {
            POWER_HILOGI(FEATURE_WAKEUP, "[UL_POWER] Wakeup by keyboard");
            std::string reason = "keyboard:";
            reason.append(std::to_string(keyCode));
            this->WakeupDevice(now, WakeupDeviceType::WAKEUP_DEVICE_KEYBOARD, reason);
        }
    }
#endif
}

void WordSelectionService::HandlePointEvent(int32_t type)
{
#ifdef HAS_MULTIMODALINPUT_INPUT_PART
    LOG_INFO("type: %{public}d", type);
    int64_t now = static_cast<int64_t>(time(nullptr));
    if (this->IsScreenOn()) {
        this->RefreshActivityInner(now, UserActivityType::USER_ACTIVITY_TYPE_ATTENTION, false);
    } else {
        if (type == PointerEvent::SOURCE_TYPE_MOUSE) {
            std::string reason = "mouse click";
            POWER_HILOGI(FEATURE_WAKEUP, "[UL_POWER] Wakeup by mouse");
            this->WakeupDevice(now, WakeupDeviceType::WAKEUP_DEVICE_MOUSE, reason);
        }
    }
#endif
}

// foundation/multimodalinput/input/interfaces/native/innerkits/event/include/key_event.h
void WordSelectionInputMonitor::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    LOG_INFO("[YMZ]keyId: %{public}d", keyEvent->GetKeyCode());
}

void WordSelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    LOG_INFO("[YMZ]pointerEvent: %{public}d", (int32_t)pointerEvent->GetPointerAction());
}

void WordSelectionInputMonitor::OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const {};