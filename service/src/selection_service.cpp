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

#include "ability_manager_client.h"
#include "iremote_object.h"
#include "callback_handler.h"
#include "system_ability_definition.h"
#include "selection_log.h"
#include <input_manager.h>
#include "parameter.h"
#include <chrono>

using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::MMI;

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(SelectionService::GetInstance().GetRefPtr());
std::shared_mutex SelectionService::adminLock_;
sptr<SelectionService> SelectionService::instance_;
std::mutex SelectionService::eventHandlerMutex_;
std::shared_ptr<AppExecFwk::EventHandler> SelectionService::handler_{ nullptr };

uint32_t SelectionInputMonitor::curSelectState = SELECT_INPUT_INITIAL;
uint32_t SelectionInputMonitor::subSelectState = SUB_INITIAL;
int64_t SelectionInputMonitor::lastClickTime = 0;
bool SelectionInputMonitor::ctrlSelectFlag = false;

sptr<SelectionService> SelectionService::GetInstance()
{
    if (instance_ == nullptr) {
        std::unique_lock<std::shared_mutex> autoLock(adminLock_);
        if (instance_ == nullptr) {
            SELECTION_HILOGI("SelectionService:GetInstance instance = new SelectionService()");
            instance_ = new (std::nothrow) SelectionService();
        }
    }
    return instance_;
}

SelectionService::SelectionService() : SystemAbility(SELECTION_FWK_SA_ID, true)
{
    SELECTION_HILOGI("[SelectionService] SelectionService()");
}

SelectionService::SelectionService(int32_t saId, bool runOnCreate) : SystemAbility(saId, runOnCreate)
{
    SELECTION_HILOGI("[SelectionService] saID=%{public}d runOnCreate=%{public}d", saId, runOnCreate);
}

SelectionService::~SelectionService()
{
    SELECTION_HILOGI("[~SelectionService]");
}

ErrCode SelectionService::AddVolume(int32_t volume, int32_t& funcResult)
{
    SELECTION_HILOGI("[SelectionService][AddVolume]begin");
    return (volume + 1);
}

int32_t SelectionService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    dprintf(fd, "---------------------SelectionService::Dump--------------------\n");
    return OHOS::NO_ERROR;
}

static int64_t GetCurrentTimeMillis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

static void WatchParameterFunc(const char *key, const char *value, void *context)
{
    (void)context;
    SELECTION_HILOGI("WatchParameterFunc begin");
    SELECTION_HILOGI("%{public}s: value=%{public}s", key, value);
}

static void WatchTriggerMode(const char *key, const char *value, void *context)
{
    (void)context;
    SELECTION_HILOGI("WatchParameterFunc begin");
    SELECTION_HILOGI("%{public}s: value=%{public}s", key, value);
    if (strcmp(key, SYS_SELECTION_TRIGGER_USERNAM) == 0) {
        if (strcmp(value, SYS_SELECTION_TRIGGER_VAL) == 0) {
            SelectionInputMonitor::ctrlSelectFlag = true;
        } else {
            SelectionInputMonitor::ctrlSelectFlag = false;
        }
        SELECTION_HILOGI("ctrlSelectFlag is %{public}d", SelectionInputMonitor::ctrlSelectFlag);
    }
}

void SelectionService::StopCurrentAbility()
{
    std::lock_guard<std::mutex> lock(abilityMutex_);
    if (currentBundleName_.empty() || currentAbilityName_.empty()) {
        SELECTION_HILOGD("No ability running");
        return;
    }
    SELECTION_HILOGD("Stop current ability: %{public}s/%{public}s", currentBundleName_.c_str(),
        currentAbilityName_.c_str());

    AAFwk::Want want;
    want.SetElementName(currentBundleName_, currentAbilityName_);
    int32_t ret = AAFwk::AbilityManagerClient::GetInstance()->StopExtensionAbility(want, nullptr);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("StopServiceAbility failed, ret: %{public}d", ret);
        return;
    }

    currentBundleName_.clear();
    currentAbilityName_.clear();
}

int32_t SelectionService::StartNewAbility( const std::string& bundleName, const std::string& abilityName)
{
    SELECTION_HILOGD("Start new SelectionExtension, bundleName:%{public}s, abilityName:%{public}s", bundleName.c_str(),
        abilityName.c_str());
    AAFwk::Want want;
    want.SetElementName(bundleName, abilityName);

    auto ret = AAFwk::AbilityManagerClient::GetInstance()->StartExtensionAbility(want, nullptr);
    if (ret != 0) {
        SELECTION_HILOGE("StartExtensionAbility failed %{public}d", ret);
        return ret;
    }

    currentBundleName_ = bundleName;
    currentAbilityName_ = abilityName;
    SELECTION_HILOGD("StartExtensionAbility success");
    return 0;
}

static void WatchAppSwitch(const char *key, const char *value, void *context)
{
    SELECTION_HILOGD("WatchAppSwitch begin");
    SELECTION_HILOGD("%{public}s: value=%{public}s", key, value);
    SelectionService *selectionService = static_cast<SelectionService *>(context);
    if (selectionService == nullptr) {
        SELECTION_HILOGE("selectionService is nullptr");
        return;
    }

    const std::string appInfo = value;
    auto pos = appInfo.find('/');
    if (pos == std::string::npos || pos + 1 >= appInfo.size()) {
        SELECTION_HILOGE("app info: %{public}s is abnormal!", appInfo.c_str());
        return;
    }
    const std::string bundleName = appInfo.substr(0, pos);
    const std::string extName = appInfo.substr(pos + 1);
    SELECTION_HILOGD("bundleName: %{public}s, extName: %{public}s", bundleName.c_str(), extName.c_str());
    selectionService->StopCurrentAbility();
    auto ret = selectionService->StartNewAbility(bundleName, extName);
    SELECTION_HILOGD("StartExtensionAbility ret = %{public}d", ret);
}

void SelectionService::WatchParams()
{
    SELECTION_HILOGI("WatchParams begin");
    WatchParameter(SYS_SELECTION_SWITCH_USERNAM, WatchParameterFunc, nullptr);
    WatchParameter(SYS_SELECTION_TRIGGER_USERNAM, WatchTriggerMode, nullptr);
    WatchParameter(SYS_SELECTION_APP_USERNAM, WatchAppSwitch, this);
    SELECTION_HILOGI("WatchParams end");
}

void SelectionService::OnStart()
{
    SELECTION_HILOGI("[SelectionService][OnStart]begin");
    Publish(this);
    InputMonitorInit();
    WatchParams();
    SELECTION_HILOGI("[SelectionService][OnStart]end");
}

void SelectionService::OnStop()
{
    SELECTION_HILOGI("[SelectionService][OnStop]begin");
    InputMonitorCancel();
    SELECTION_HILOGI("[SelectionService][OnStop]end");
}

void SelectionService::InputMonitorInit()
{
    SELECTION_HILOGI("[SelectionService] input monitor init");
    std::shared_ptr<SelectionInputMonitor> inputMonitor = std::make_shared<SelectionInputMonitor>();
    if (inputMonitorId_ < 0) {
        inputMonitorId_ =
            InputManager::GetInstance()->AddMonitor(std::static_pointer_cast<IInputEventConsumer>(inputMonitor));
            SELECTION_HILOGI("[SelectionService] input monitor init end");
    }

    {
        std::lock_guard<std::mutex> lock(eventHandlerMutex_);
        handler_ = AppExecFwk::EventHandler::Current();
    }
}

void SelectionService::InputMonitorCancel()
{
    SELECTION_HILOGI("[SelectionService] input monitor cancel");
    InputManager* inputManager = InputManager::GetInstance();
    if (inputMonitorId_ >= 0) {
        inputManager->RemoveMonitor(inputMonitorId_);
        inputMonitorId_ = -1;
    }
}

void SelectionService::HandleKeyEvent(int32_t keyCode)
{
#ifdef HAS_MULTIMODALINPUT_INPUT_PART
    SELECTION_HILOGI("[YMZ] keyCode: %{public}d", keyCode);
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

void SelectionService::HandlePointEvent(int32_t type)
{
#ifdef HAS_MULTIMODALINPUT_INPUT_PART
    SELECTION_HILOGI("type: %{public}d", type);
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
void SelectionInputMonitor::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    SELECTION_HILOGD("[SelectionService] into keyEvent");
    int32_t keyCode = keyEvent->GetKeyCode();
    SELECTION_HILOGD("[SelectionService] keyId: %{public}d", keyCode);
    if (keyCode == KeyEvent::KEYCODE_CTRL_LEFT || keyCode == KeyEvent::KEYCODE_CTRL_RIGHT) {
        SELECTION_HILOGI("[SelectionService] Processed ctrl key.");
        if (ctrlSelectFlag && curSelectState == SELECT_INPUT_WAIT_LEFT_MOVE && subSelectState == SUB_WAIT_CTRL) {
            curSelectState = SELECT_INPUT_LEFT_MOVE;
            subSelectState = SUB_INITIAL;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_LEFT_MOVE.");
            return;
        }
        if (ctrlSelectFlag && curSelectState == SELECT_INPUT_WAIT_DOUBLE_CLICK && subSelectState == SUB_WAIT_CTRL) {
            curSelectState = SELECT_INPUT_DOUBLE_CLICKED;
            subSelectState = SUB_INITIAL;
            SELECTION_HILOGI("[SelectionService] Set curSelectState SELECT_INPUT_LEFT_MOVE.");
            return;
        }
    }

}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    SELECTION_HILOGD("[SelectionService] into PointerEvent");
    int32_t action = pointerEvent->GetPointerAction();
    int32_t buttonId = pointerEvent->GetButtonId();
    SELECTION_HILOGD("[SelectionService] pointerEvent: %{public}d", action);
    SELECTION_HILOGD("[SelectionService] buttonId: %{public}d", buttonId);
    if (curSelectState == SELECT_INPUT_INITIAL && buttonId != PointerEvent::MOUSE_BUTTON_LEFT) {
        return;
    }
    if (subSelectState == SUB_WAIT_CTRL) {
        SELECTION_HILOGI("[SelectionService] pointerEvent: curSelectState == SELECT_INPUT_WAIT_CTRL. return.");
        curSelectState = SELECT_INPUT_INITIAL;
        subSelectState = SUB_INITIAL;
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

        default:
            break;
    }
    if (curSelectState == SELECT_INPUT_LEFT_MOVE || curSelectState == SELECT_INPUT_DOUBLE_CLICKED) {
        // world selection action
        SELECTION_HILOGI("[SelectionService] first, end word selection action.");
        InjectCtrlC();
        SELECTION_HILOGI("[SelectionService] first, end inject ctrl + c.");
        curSelectState = SELECT_INPUT_INITIAL;
    }

    return;
}

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const
{
    SELECTION_HILOGI("[SelectionService] into axisEvent");
};


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
        subSelectState = SUB_WAIT_PRESS_UP;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_LEFT_MOVE.");
    } else if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        curSelectState = SELECT_INPUT_WAIT_DOUBLE_CLICK;
        subSelectState = SUB_WAIT_PRESS_DOWN;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_DOUBLE_CLICK.");
    }
    return;
}

void SelectionInputMonitor::InputWordWaitLeftMoveProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    if (action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        if (ctrlSelectFlag) {
            subSelectState = SUB_WAIT_CTRL;
            SELECTION_HILOGI("set subSelectState to SUB_WAIT_CTRL.");
        } else {
            curSelectState = SELECT_INPUT_LEFT_MOVE;
            SELECTION_HILOGI("set curSelectState to SELECT_INPUT_LEFT_MOVE.");
        }
    }
    return;
}

void SelectionInputMonitor::InputWordWaitDoubleClickProcess(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t action = pointerEvent->GetPointerAction();
    int32_t buttonId = pointerEvent->GetButtonId();
    if (buttonId != PointerEvent::MOUSE_BUTTON_LEFT) {
        curSelectState = SELECT_INPUT_INITIAL;
        subSelectState = SUB_INITIAL;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_INITIAL.");
        return;
    }
    if (subSelectState == SUB_WAIT_PRESS_DOWN && action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        subSelectState = SUB_WAIT_PRESS_UP;
        SELECTION_HILOGI("set subSelectState to SUB_WAIT_PRESS_UP.");
        return;
    }
    if (subSelectState == SUB_WAIT_PRESS_UP && action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        auto curTime = GetCurrentTimeMillis();
        if (curTime - lastClickTime < DOUBLE_CLICK_TIME) {
            if (ctrlSelectFlag) {
                subSelectState = SUB_WAIT_CTRL;
                SELECTION_HILOGI("set subSelectState to SUB_WAIT_CTRL.");
            } else {
                curSelectState = SELECT_INPUT_DOUBLE_CLICKED;
                subSelectState = SUB_INITIAL;
                SELECTION_HILOGI("set curSelectState to SELECT_INPUT_DOUBLE_CLICKED.");
            }
            return;
        }
        curSelectState = SELECT_INPUT_WAIT_DOUBLE_CLICK;
        subSelectState = SUB_WAIT_PRESS_DOWN;
        lastClickTime = curTime;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_WAIT_DOUBLE_CLICK.");
        return;
    }

    curSelectState = SELECT_INPUT_INITIAL;
    subSelectState = SUB_INITIAL;
    SELECTION_HILOGI("set curSelectState to SELECT_INPUT_INITIAL.");
    return;
}

void SelectionInputMonitor::InjectCtrlC() const
{
    auto keyDownEvent = KeyEvent::Create();
    keyDownEvent->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    std::vector<int32_t> downKey;
    downKey.push_back(KeyEvent::KEYCODE_CTRL_LEFT);
    downKey.push_back(KeyEvent::KEYCODE_C);
    keyDownEvent->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyDownEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    
    KeyEvent::KeyItem downItem[downKey.size()];
    for (size_t i = 0; i < downKey.size(); i++) {
        downItem[i].SetKeyCode(downKey[i]);
        downItem[i].SetPressed(true);
        downItem[i].SetDownTime(500);
        keyDownEvent->AddPressedKeyItems(downItem[i]);
    }
    InputManager::GetInstance()->SimulateInputEvent(keyDownEvent);
}

std::shared_ptr<AppExecFwk::EventHandler> SelectionService::GetEventHandler()
{
    std::lock_guard<std::mutex> lock(eventHandlerMutex_);
    return handler_;
}


int32_t SelectionService::OnSelectionEvent(std::string &selectionData)
{
    SELECTION_HILOGI("OnSelectionEvent begin");
    std::string type = "selectionEvent";

    if (selectionData.empty()) {
        SELECTION_HILOGE("selectionData is empty");
        return 1;
    }

    auto entry = GetEntry(type, [&selectionData](SelectionEntry &entry) { entry.text = selectionData; });
    if (entry == nullptr) {
        SELECTION_HILOGE("failed to get uv entry!");
        return 1;
    }

    auto eventHandler = GetEventHandler();
    if (eventHandler == nullptr) {
        SELECTION_HILOGE("eventHandler is nullptr!");
        return 1;
    }

    SELECTION_HILOGI("selectionData is [%{public}s]", selectionData.c_str());


    auto task = [entry]() {
        auto getTextChangeProperty = [entry](napi_env env, napi_value *args, uint8_t argc) -> bool {
            if (argc == 0) {
                return false;
            }
            // 0 means the first param of callback.
            napi_create_string_utf8(env, entry->text.c_str(), NAPI_AUTO_LENGTH, &args[0]);
            return true;
        };
        // 1 means callback has one param.
        JsCallbackHandler::Traverse(entry->vecCopy, { 1, getTextChangeProperty });
    };
    eventHandler->PostTask(task, type, 0, AppExecFwk::EventQueue::Priority::VIP);

    SELECTION_HILOGI("OnSelectionEvent end");
    return 0;
}
