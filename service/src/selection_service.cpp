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
#include "common_event_manager.h"
#include "selection_interface.h"

using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::MMI;
using namespace OHOS::EventFwk;

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(SelectionService::GetInstance().GetRefPtr());
std::shared_mutex SelectionService::adminLock_;
sptr<SelectionService> SelectionService::instance_;

uint32_t SelectionInputMonitor::curSelectState = SELECT_INPUT_INITIAL;
uint32_t SelectionInputMonitor::subSelectState = SUB_INITIAL;
int64_t SelectionInputMonitor::lastClickTime = 0;
bool SelectionInputMonitor::ctrlSelectFlag = false;

void SelectionExtensionAbilityConnection::OnAbilityConnectDone(
    const ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode)
{
    SELECTION_HILOGI("OnAbilityConnectDone, bundle = %{public}s, ability = %{public}s, resultCode = %{public}d",
        element.GetBundleName().c_str(), element.GetAbilityName().c_str(), resultCode);
}
void SelectionExtensionAbilityConnection::OnAbilityDisconnectDone(const ElementName &element, int resultCode)
{
    SELECTION_HILOGI("OnAbilityDisconnectDone, bundle = %{public}s,ability = %{public}s, resultCode = %{public}d",
        element.GetBundleName().c_str(), element.GetAbilityName().c_str(), resultCode);
    remoteObject_ = nullptr;
}

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

ErrCode SelectionService::UnregisterListener(const sptr<IRemoteObject> &listener)
{
    listenerStub_ = nullptr;
    return 0;
}

sptr<ISelectionListener> SelectionService::GetListener() {
    std::lock_guard<std::mutex> lock(mutex_);
    return listenerStub_;
}

ErrCode SelectionService::RegisterListener(const sptr<IRemoteObject> &listener)
{
    SELECTION_HILOGI("Enter RegisterListener");
    if (listener == nullptr) {
        SELECTION_HILOGE("RegisterListener: Input listener is nullptr.");
        return 1;
    }

    auto listenerStub = iface_cast<ISelectionListener>(listener);
    if (listenerStub == nullptr) {
        SELECTION_HILOGE("RegisterListener: Failed to cast listener to ISelectionListener.");
        return 1;
    }

    if (listenerStub_ && listenerStub_ == listenerStub) {
        SELECTION_HILOGW("RegisterListener: Listener already registered.");
        return 0;
    }

    listenerStub_ = listenerStub;
    return 0;
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

void SelectionService::DisconnectCurrentExtAbility()
{
    SELECTION_HILOGD("Disconnect current extensionAbility");
    if (connectInner_ == nullptr) {
        SELECTION_HILOGE("connectInner_ is null");
        return;
    }

    int32_t ret = AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(connectInner_);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("DisconnectServiceAbility failed, ret: %{public}d", ret);
        return;
    }
}

int32_t SelectionService::ConnectNewExtAbility( const std::string& bundleName, const std::string& abilityName)
{
    SELECTION_HILOGD("Start new SelectionExtension, bundleName:%{public}s, abilityName:%{public}s", bundleName.c_str(),
        abilityName.c_str());
    AAFwk::Want want;
    want.SetElementName(bundleName, abilityName);
    connectInner_ = new(std::nothrow) SelectionExtensionAbilityConnection();

    auto ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, connectInner_, -1);
    if (ret != 0) {
        SELECTION_HILOGE("StartExtensionAbility failed %{public}d", ret);
        return ret;
    }
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
    selectionService->DisconnectCurrentExtAbility();
    auto ret = selectionService->ConnectNewExtAbility(bundleName, extName);
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
    Publish(SelectionService::GetInstance());
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
    std::shared_ptr<SelectionInputMonitor> inputMonitor = std::make_shared<SelectionInputMonitor>(
        std::make_shared<DefaultSelectionEventListener>());
    if (inputMonitorId_ < 0) {
        sleep(30);
        inputMonitorId_ =
            InputManager::GetInstance()->AddMonitor(std::static_pointer_cast<IInputEventConsumer>(inputMonitor));
            SELECTION_HILOGI("[SelectionService] input monitor init end");
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

void DefaultSelectionEventListener::OnTextSelected()
{
    SELECTION_HILOGI("End word selection action.");
    InjectCtrlC();
    SELECTION_HILOGI("End Inject Ctrl + C.");
    // send selection data
    sptr<ISelectionListener> listener = SelectionService::GetInstance()->GetListener();
    if (listener == nullptr) {
        SELECTION_HILOGE("get listener is null");
        return;
    }
    SelectionDataInner data;
    data.text = "Hello, world!";
    data.cursorStartPos = 0;
    data.cursorEndPos = 13;
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
    } else if (subSelectState == SUB_WAIT_KEY_CTRL_UP && action == KeyEvent::KEY_ACTION_UP) {
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

 void SelectionInputMonitor::ResetProcess(std::shared_ptr<PointerEvent> pointerEvent) const
 {
    curSelectState = SELECT_INPUT_INITIAL;
    subSelectState = SUB_INITIAL;
    OnInputEvent(pointerEvent);
 }

void SelectionInputMonitor::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    int32_t pointerId = pointerEvent->GetPointerId();
    if (curSelectState == SELECT_INPUT_INITIAL && pointerId != PointerEvent::MOUSE_BUTTON_LEFT) {
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
        return;
    }
    // world selection action
    if (curSelectState != SELECT_INPUT_DOUBLE_CLICKED) {
        curSelectState = SELECT_INPUT_INITIAL;
        SELECTION_HILOGI("set curSelectState to SELECT_INPUT_INITIAL");
    }

    selectionEventListener_->OnTextSelected();
}

void DefaultSelectionEventListener::InjectCtrlC() const
{
    // 创建KeyEvent对象
    auto keyEvent1 = KeyEvent::Create();

    // 设置Ctrl键按下
    keyEvent1->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent1->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    keyEvent1->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    KeyEvent::KeyItem item1;
    item1.SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    item1.SetPressed(true);
    item1.SetDownTime(500);
    keyEvent1->AddKeyItem(item1);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent1);

    // 设置C键按下
    auto keyEvent2 = KeyEvent::Create();
    keyEvent2->SetKeyCode(KeyEvent::KEYCODE_C);
    keyEvent2->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    keyEvent2->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    KeyEvent::KeyItem item2;
    item2.SetKeyCode(KeyEvent::KEYCODE_C);
    item2.SetPressed(true);
    item2.SetDownTime(500);
    keyEvent2->AddKeyItem(item2);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent2);

    // 设置C键释放
    auto keyEvent3 = KeyEvent::Create();
    keyEvent3->SetKeyCode(KeyEvent::KEYCODE_C);
    keyEvent3->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    keyEvent3->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    KeyEvent::KeyItem item3;
    item3.SetKeyCode(KeyEvent::KEYCODE_C);
    item3.SetPressed(true);
    item3.SetDownTime(500);
    keyEvent3->AddKeyItem(item3);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent3);

    // 设置Ctrl键释放
    auto keyEvent4 = KeyEvent::Create();
    keyEvent4->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent4->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    keyEvent4->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    KeyEvent::KeyItem item4;
    item4.SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    item4.SetPressed(true);
    item4.SetDownTime(500);
    keyEvent4->AddKeyItem(item4);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent4);
}
