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

#include <chrono>
#include <thread>
#include <ipc_skeleton.h>

#include "ability_manager_client.h"
#include "db_selection_config_repository.h"
#include "iremote_object.h"
#include "callback_handler.h"
#include "system_ability_definition.h"
#include "selection_errors.h"
#include "selection_log.h"
#include <input_manager.h>
#include "parameter.h"
#include "common_event_manager.h"
#include "selection_config_comparator.h"
#include "selection_input_monitor.h"
#include "selection_interface.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "focus_monitor_manager.h"
#include "os_account_manager.h"
#include "sys_selection_config_repository.h"
#include "selection_app_validator.h"

#define SELECTION_MAX_TRY_TIMES 50
#define SELECTION_SLEEP_TIME 100

using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::MMI;
using namespace OHOS::EventFwk;

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(SelectionService::GetInstance().GetRefPtr());
sptr<ISelectionListener> SelectionService::listener_ { nullptr };

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
    auto disconnectAppInfo = element.GetBundleName() + "/" + element.GetAbilityName();
    auto curAppInfo = MemSelectionConfig::GetInstance().GetSelectionConfig().GetApplicationInfo();
    if (curAppInfo == disconnectAppInfo) {
        auto ret = SelectionService::GetInstance()->ConnectNewExtAbility(element.GetBundleName(),
            element.GetAbilityName());
        SELECTION_HILOGD("Reconnect extension ability ret = %{public}d", ret);
    }
}

sptr<SelectionService> SelectionService::GetInstance()
{
    static sptr<SelectionService> instance = new (std::nothrow) SelectionService();
    return instance;
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

sptr<ISelectionListener> SelectionService::GetListener() {
    std::lock_guard<std::mutex> lock(mutex_);
    return listener_;
}

ErrCode SelectionService::RegisterListener(const sptr<ISelectionListener>& listener)
{
    if (!SelectionAppValidator::GetInstance().Validate()) {
        return SESSION_UNAUTHENTICATED_ERR;
    }

    pid_.store(IPCSkeleton::GetCallingPid());
    SELECTION_HILOGI("Enter RegisterListener");
    if (listener == nullptr) {
        SELECTION_HILOGE("RegisterListener: selection listener is nullptr.");
        return ERR_INVALID_DATA;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        listener_ = listener;
    }

    return 0;
}

ErrCode SelectionService::UnregisterListener(const sptr<ISelectionListener>& listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    listener_ = nullptr;
    return 0;
}

ErrCode SelectionService::IsCurrentSelectionApp(int pid, bool &resultValue)
{
    resultValue = (pid_.load() != -1 && pid == pid_.load());
    SELECTION_HILOGI("Checking IsCurrentSelectionApp: %{public}d", resultValue);
    return 0;
}

int32_t SelectionService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    dprintf(fd, "---------------------SelectionService::Dump--------------------\n");
    return OHOS::NO_ERROR;
}

static void WatchEnableSwitch(const char *key, const char *value, void *context)
{
    SelectionService *selectionService = static_cast<SelectionService *>(context);
    SELECTION_HILOGI("%{public}s: value=[%{public}s], DEFAULT_SWITCH =[%{public}s]", key, value, DEFAULT_SWITCH);
    bool isEnabledValue = (strcmp(value, DEFAULT_SWITCH) == 0);
    SELECTION_HILOGI("isEnabledValue is %{public}d", isEnabledValue);
    MemSelectionConfig::GetInstance().SetEnabled(isEnabledValue);

    selectionService->PersistSelectionConfig();
}

static void WatchTriggerMode(const char *key, const char *value, void *context)
{
    SelectionService *selectionService = static_cast<SelectionService *>(context);
    SELECTION_HILOGI("WatchTriggerMode begin");
    SELECTION_HILOGI("%{public}s: value=%{public}s", key, value);
    int triggerCmpResult = strcmp(value, DEFAULT_TRIGGER);
    BaseSelectionInputMonitor::ctrlSelectFlag = (triggerCmpResult == 0);
    SELECTION_HILOGI("ctrlSelectFlag is %{public}d", BaseSelectionInputMonitor::ctrlSelectFlag);

    bool triggerValue = (triggerCmpResult == 0);
    MemSelectionConfig::GetInstance().SetTriggered(triggerValue);

    selectionService->PersistSelectionConfig();
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
    char target = '/';
    int count = std::count(appInfo.begin(), appInfo.end(), target);
    if (count != 1) {
        SELECTION_HILOGE("/ is not only one.");
        return;
    }
    auto pos = appInfo.find('/');
    if (appInfo.empty() || pos == std::string::npos || pos + 1 >= appInfo.size()) {
        SELECTION_HILOGE("app info: %{public}s is invalid!", appInfo.c_str());
        return;
    }
    const std::string bundleName = appInfo.substr(0, pos);
    const std::string extName = appInfo.substr(pos + 1);
    if (bundleName.length() == 0 || extName.length() == 0) {
        SELECTION_HILOGE("bundleName or extName is empty");
        return;
    }
    MemSelectionConfig::GetInstance().SetApplicationInfo(appInfo);
    selectionService->DisconnectCurrentExtAbility();
    auto ret = selectionService->ConnectNewExtAbility(bundleName, extName);
    SELECTION_HILOGD("StartExtensionAbility ret = %{public}d", ret);

    selectionService->PersistSelectionConfig();
}

void SelectionService::PersistSelectionConfig()
{
    if (!IsUserLoggedIn()) {
        SELECTION_HILOGW("Do not save selection config to DB because user is not logged in.");
        return;
    }
    auto &selectionConfig = MemSelectionConfig::GetInstance().GetSelectionConfig();
    int ret = DbSelectionConfigRepository::GetInstance()->Save(GetUserId(), selectionConfig);
    if (ret != SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("Add database failed. ret = %{public}d", ret);
    }
}

void SelectionService::DisconnectCurrentExtAbility()
{
    pid_.store(-1);
    SELECTION_HILOGD("Disconnect current extensionAbility");
    std::lock_guard<std::mutex> lockGuard(connectInnerMutex_);
    if (connectInner_ == nullptr) {
        SELECTION_HILOGE("connectInner_ is null");
        return;
    }

    int32_t ret = AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(connectInner_);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("DisconnectServiceAbility failed, ret: %{public}d", ret);
        return;
    }
    connectInner_ = nullptr;
}

int32_t SelectionService::ConnectNewExtAbility( const std::string& bundleName, const std::string& abilityName)
{
    SELECTION_HILOGD("Start new SelectionExtension, bundleName:%{public}s, abilityName:%{public}s", bundleName.c_str(),
        abilityName.c_str());
    AAFwk::Want want;
    want.SetElementName(bundleName, abilityName);

    std::lock_guard<std::mutex> lockGuard(connectInnerMutex_);
    connectInner_ = sptr<SelectionExtensionAbilityConnection>::MakeSptr();
    if (connectInner_ == nullptr) {
        SELECTION_HILOGE("new(std::nothrow) SelectionExtensionAbilityConnection() failed!");
        return SELECTION_CONFIG_FAILURE;
    }
    auto ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, connectInner_, -1);
    if (ret != 0) {
        SELECTION_HILOGE("[selectevent] StartExtensionAbility failed. error code is %{public}d.", ret);
        return ret;
    }
    SELECTION_HILOGI("[selectevent] StartExtensionAbility success.");
    return 0;
}

void SelectionService::WatchParams()
{
    SELECTION_HILOGI("WatchParams begin");
    WatchParameter(SYS_SELECTION_SWITCH, WatchEnableSwitch, this);
    WatchParameter(SYS_SELECTION_TRIGGER, WatchTriggerMode, this);
    WatchParameter(SYS_SELECTION_APP, WatchAppSwitch, this);
    SELECTION_HILOGI("WatchParams end");
}

int SelectionService::GetUserId()
{
    return userId_.load();
}

int SelectionService::LoadAccountLocalId()
{
    int32_t userId = -1;
    int32_t ret = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
    auto iCounter = 0;
    while (ret != 0 && iCounter < SELECTION_MAX_TRY_TIMES) {
        iCounter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(SELECTION_SLEEP_TIME));
        ret = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
    }
    SELECTION_HILOGI("GetForegroundOsAccountLocalId userId.");
    userId_.store(userId);
    return userId;
}

bool SelectionService::IsUserLoggedIn()
{
    return LoadAccountLocalId() != -1;
}

void SelectionService::SynchronizeSelectionConfig()
{
    if (!IsUserLoggedIn()) {
        SELECTION_HILOGW("No selection config sync because user is not logged in.");
        return;
    }
    SelectionConfig sysSelectionConfig = SysSelectionConfigRepository::GetInstance()->GetSysParameters();
    SELECTION_HILOGI("sysSelectionConfig: enable=%{public}d trigger=%{public}d applicationInfo=%{public}s",
        sysSelectionConfig.GetEnable(), sysSelectionConfig.GetTriggered(),
        sysSelectionConfig.GetApplicationInfo().c_str());
    auto dbSelectionConfig = DbSelectionConfigRepository::GetInstance()->GetOneByUserId(userId_.load());
    auto result = SelectionConfigComparator::Compare(userId_.load(), sysSelectionConfig, dbSelectionConfig);
    MemSelectionConfig::GetInstance().SetSelectionConfig(result.selectionConfig);

    if (result.shouldCreate) {
        SELECTION_HILOGI("result.shouldCreate");
        auto ret = DbSelectionConfigRepository::GetInstance()->Save(userId_.load(), result.selectionConfig);
        if (ret != SELECTION_CONFIG_OK) {
            SELECTION_HILOGE("Add database failed. ret = %{public}d", ret);
        }
        SELECTION_HILOGI("result.selectionConfig.isEnable = %{public}d", result.selectionConfig.GetEnable());
        SysSelectionConfigRepository::GetInstance()->SetSysParameters(result.selectionConfig);
        return;
    }

    if (result.direction == SyncDirection::FromDbToSys) {
        SELECTION_HILOGI("result.direction == SyncDirection::FromDbToSys");
        SELECTION_HILOGI("FromDbToSys result: %{public}s", result.ToString().c_str());
        SysSelectionConfigRepository::GetInstance()->SetSysParameters(result.selectionConfig);
    } else if (result.direction == SyncDirection::FromSysToDb) {
        SELECTION_HILOGI("result.direction == SyncDirection::FromSysToDb");
        auto ret = DbSelectionConfigRepository::GetInstance()->Save(userId_.load(), result.selectionConfig);
        if (ret != SELECTION_CONFIG_OK) {
            SELECTION_HILOGE("Add database failed. ret = %{public}d", ret);
        }
    }

    if (result.shouldStop) {
        SELECTION_HILOGI("result.shouldStop");
        SysSelectionConfigRepository::GetInstance()->DisableSAService();
    }
}

void SelectionService::OnStart()
{
    SELECTION_HILOGI("[selectevent][SelectionService][OnStart]begin");
    Publish(SelectionService::GetInstance());
    InputMonitorInit();
    SynchronizeSelectionConfig();
    WatchParams();
    InitFocusChangedMonitor();
    SELECTION_HILOGI("[selectevent][SelectionService][OnStart]end.");
}

void SelectionService::OnStop()
{
    SELECTION_HILOGI("[selectevent][SelectionService][OnStop]begin");
    InputMonitorCancel();
    CancelFocusChangedMonitor();
    SELECTION_HILOGI("[selectevent][SelectionService][OnStop]end.");
}

void SelectionService::InputMonitorInit()
{
    SELECTION_HILOGI("[SelectionService] input monitor init");
    std::shared_ptr<IInputEventConsumer> inputMonitor = std::make_shared<SelectionInputMonitor>();
    if (inputMonitorId_ > 0) {
        return;
    }

    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    auto remoteObj = sam->CheckSystemAbility(MULTIMODAL_INPUT_SERVICE_ID);
    auto iCounter = 0;
    while (remoteObj == nullptr && iCounter < SELECTION_MAX_TRY_TIMES) {
        SELECTION_HILOGI("CheckSystemAbility MULTIMODAL_INPUT_SERVICE_ID failed. wait...");
        iCounter++;
        sleep(1);
        remoteObj = sam->CheckSystemAbility(MULTIMODAL_INPUT_SERVICE_ID);
    }

    if (remoteObj == nullptr) {
        SELECTION_HILOGE("CheckSystemAbility MULTIMODAL_INPUT_SERVICE_ID failed.");
        return;
    }

    SELECTION_HILOGI("CheckSystemAbility MULTIMODAL_INPUT_SERVICE_ID succeed.");
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
        [this](const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused) {
            HandleFocusChanged(focusChangeInfo, isFocused);
        });
}

void SelectionService::CancelFocusChangedMonitor()
{
    SELECTION_HILOGI("[SelectionService] cancel focus changed monitor");
    FocusMonitorManager::GetInstance().UnregisterFocusChangedListener();
}

void SelectionService::HandleFocusChanged(const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused)
{
    SELECTION_HILOGI("[SelectionService] handle focus changed");
    std::lock_guard<std::mutex> lock(mutex_);
    if (listener_ == nullptr || focusChangeInfo == nullptr) {
        SELECTION_HILOGE("listener_ or focusChangeInfo is nullptr.");
        return;
    }
    auto windowType = static_cast<uint32_t>(focusChangeInfo->windowType_);
    SelectionFocusChangeInfo selectionFocusChangeInfo(focusChangeInfo->windowId_, focusChangeInfo->displayId_,
        focusChangeInfo->pid_, focusChangeInfo->uid_, windowType, isFocused, FocusChangeSource::WindowManager);
    listener_->FocusChange(selectionFocusChangeInfo);
}

void SelectionService::HandleKeyEvent(int32_t keyCode)
{
}

void SelectionService::HandlePointEvent(int32_t type)
{
}