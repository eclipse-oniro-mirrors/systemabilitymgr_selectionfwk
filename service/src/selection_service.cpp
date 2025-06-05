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
#include "selection_input_monitor.h"
#include "selection_interface.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "screenlock_manager.h"
#include "focus_monitor_manager.h"


using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::MMI;
using namespace OHOS::EventFwk;

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(SelectionService::GetInstance().GetRefPtr());
std::shared_mutex SelectionService::adminLock_;
sptr<SelectionService> SelectionService::instance_;

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
    SELECTION_HILOGD("Enter RegisterListener");
    if (listener == nullptr) {
        SELECTION_HILOGE("RegisterListener: selection listener is nullptr.");
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
    InitFocusChangedMonitor();
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
    if (inputMonitorId_ > 0) {
        return;
    }

    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    auto remoteObj = sam->GetSystemAbility(MULTIMODAL_INPUT_SERVICE_ID);
    while (remoteObj == nullptr) {
        SELECTION_HILOGI("GetSystemAbility MULTIMODAL_INPUT_SERVICE_ID failed. wait...");
        sleep(1);
        remoteObj = sam->GetSystemAbility(MULTIMODAL_INPUT_SERVICE_ID);
    }
    SELECTION_HILOGI("GetSystemAbility MULTIMODAL_INPUT_SERVICE_ID succeed.");
    inputMonitorId_ = InputManager::GetInstance()->AddMonitor(inputMonitor);
    SELECTION_HILOGI("[SelectionService] input monitor init end");
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

void SelectionService::InitFocusChangedMonitor()
{
    SELECTION_HILOGI("[SelectionService] init focus changed monitor");
    FocusMonitorManager::GetInstance().RegisterFocusChangedListener(
        [this](bool isOnFocused, uint32_t windowId) {
            HandleFocusChanged(isOnFocused, windowId);
        });
}

void SelectionService::HandleFocusChanged(bool isOnFocused, uint32_t windowId)
{
    SELECTION_HILOGI("[SelectionService] handle focus changed");
    if (!isOnFocused) {
        listenerStub_->FocusChange(windowId);
    }
}

void SelectionService::HandleKeyEvent(int32_t keyCode)
{
}

void SelectionService::HandlePointEvent(int32_t type)
{
}