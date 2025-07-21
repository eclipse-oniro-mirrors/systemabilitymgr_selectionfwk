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

#include <accesstoken_kit.h>
#include <chrono>
#include <thread>
#include <ipc_skeleton.h>
#include <tokenid_kit.h>

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
#include "system_ability_status_change_listener.h"
#include "common_event_support.h"

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
    if (needReconnectWithException && curAppInfo == disconnectAppInfo) {
        SELECTION_HILOGE("Restart app [%{public}s] because it may have disconnected abnormally.", curAppInfo.c_str());
        auto ret = SelectionService::GetInstance()->ConnectNewExtAbility(element.GetBundleName(),
            element.GetAbilityName());
        SELECTION_HILOGI("Reconnect extension ability ret = %{public}d", ret);
    }
}

SelectionSysEventReceiver::SelectionSysEventReceiver(const EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : EventFwk::CommonEventSubscriber(subscribeInfo)
{
}

void SelectionSysEventReceiver::OnReceiveEvent(const CommonEventData &data)
{
    SelectionService::GetInstance()->HandleCommonEvent(data);
}

sptr<SelectionService> SelectionService::GetInstance()
{
    static sptr<SelectionService> instance = new (std::nothrow) SelectionService();
    return instance;
}

SelectionService::SelectionService() : SystemAbility(SELECTION_FWK_SA_ID, true)
{
    InitSystemAbilityChangeHandlers();
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

sptr<ISelectionListener> SelectionService::GetListener()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return listener_;
}

bool SelectionService::IsSystemCalling()
{
    const auto tokenId = IPCSkeleton::GetCallingTokenID();
    const auto flag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    SELECTION_HILOGD("calling tokenId:%{private}u, flag:%{public}u", tokenId, flag);
    if (flag == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE ||
        flag == Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL) {
        return true;
    }
    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(accessTokenIDEx);
    if (!isSystemApp) {
        SELECTION_HILOGE("Calling tockenId is not system app or system service");
    }
    return isSystemApp;
}

ErrCode SelectionService::RegisterListener(const sptr<ISelectionListener>& listener)
{
    if (!IsSystemCalling()) {
        return SelectionServiceError::NOT_SYSTEM_APP_ERROR;
    }

    if (!SelectionAppValidator::GetInstance().Validate()) {
        return SelectionServiceError::UNAUTHENTICATED_ERROR;
    }

    pid_.store(IPCSkeleton::GetCallingPid());
    SELECTION_HILOGI("Enter RegisterListener");
    if (listener == nullptr) {
        SELECTION_HILOGE("RegisterListener: selection listener is nullptr.");
        return SelectionServiceError::INVALID_DATA;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        listener_ = listener;
    }

    return 0;
}

ErrCode SelectionService::UnregisterListener(const sptr<ISelectionListener>& listener)
{
    if (!IsSystemCalling()) {
        return SelectionServiceError::NOT_SYSTEM_APP_ERROR;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    listener_ = nullptr;
    return 0;
}

ErrCode SelectionService::IsCurrentSelectionApp(int pid, bool &resultValue)
{
    if (!IsSystemCalling()) {
        return SelectionServiceError::NOT_SYSTEM_APP_ERROR;
    }
    resultValue = (pid_.load() != -1 && pid == pid_.load());
    SELECTION_HILOGI("Checking IsCurrentSelectionApp: %{public}d", resultValue);
    return 0;
}

int32_t SelectionService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    SELECTION_HILOGI("Dump start.");
    std::string command = "";
    if (args.size() == 1) {
        command = Str16ToStr8(args.at(0));
    }
    if (command == "-h") {
        SELECTION_HILOGI("Dump start -h.");
        std::string result;
        result.append("Usage:dump  <command> [options]\n")
            .append("Description:\n")
            .append("-h show help\n")
            .append("-a dump all selection variables\n");
        dprintf(fd, "%s\n", result.c_str());
    } else if (command == "-a") {
        SELECTION_HILOGI("Dump start -a.");
        auto &selectionConfig = MemSelectionConfig::GetInstance().GetSelectionConfig();
        dprintf(fd, "selection.switch: %s\n", selectionConfig.GetEnable() ? "on" : "off");
        dprintf(fd, "selection.app: %s\n", selectionConfig.GetApplicationInfo().c_str());
        dprintf(fd, "selection.trigger: %s\n", selectionConfig.GetTriggered() ? "ctrl" : "");
        dprintf(fd, "selection.uid: %d\n", selectionConfig.GetUid());
        dprintf(fd, "extension.pid: %d\n", pid_.load());
        dprintf(fd, "inputmanager.monitorId: %d\n", inputMonitorId_);
        dprintf(fd, "isScreenLocked: %d\n", isScreenLocked_.load());
    } else {
        SELECTION_HILOGI("Dump start -other.");
        dprintf(fd, "selection dump parameter error,enter '-h' for usage.\n");
    }
    SELECTION_HILOGI("Dump command=%{public}s end.", command.c_str());
    return ERR_OK;
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
    SELECTION_HILOGI("WatchTriggerMode begin, %{public}s: value=%{public}s", key, value);
    int triggerCmpResult = strcmp(value, DEFAULT_TRIGGER);
    BaseSelectionInputMonitor::ctrlSelectFlag = (triggerCmpResult == 0);
    SELECTION_HILOGI("ctrlSelectFlag is %{public}d", BaseSelectionInputMonitor::ctrlSelectFlag);

    bool triggerValue = (triggerCmpResult == 0);
    MemSelectionConfig::GetInstance().SetTriggered(triggerValue);

    selectionService->PersistSelectionConfig();
}

static void WatchAppSwitch(const char *key, const char *value, void *context)
{
    SELECTION_HILOGI("WatchAppSwitch begin, %{public}s: value=%{public}s", key, value);
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
    SELECTION_HILOGI("StartExtensionAbility ret = %{public}d", ret);

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

void SelectionService::HandleCommonEvent(const CommonEventData &data)
{
    const AAFwk::Want &want = data.GetWant();
    std::string action = want.GetAction();
    SELECTION_HILOGI("the action: %{public}s", action.c_str());
    if (action == CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED) {
        SetScreenLockedFlag(true);
    } else if (action == CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED) {
        SetScreenLockedFlag(false);
    } else if (action == CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        SynchronizeSelectionConfig();
    }
}

void SelectionService::DisconnectCurrentExtAbility()
{
    pid_.store(-1);
    SELECTION_HILOGI("Disconnect current extensionAbility");
    std::lock_guard<std::mutex> lockGuard(connectInnerMutex_);
    if (connectInner_ == nullptr) {
        SELECTION_HILOGE("connectInner_ is null");
        return;
    }

    connectInner_->needReconnectWithException = false;
    int32_t ret = AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(connectInner_);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("DisconnectServiceAbility failed, ret: %{public}d", ret);
        return;
    }
    connectInner_ = nullptr;
    SELECTION_HILOGI("[selectevent] DisconnectAbility success.");
}

int32_t SelectionService::ConnectNewExtAbility(const std::string& bundleName, const std::string& abilityName)
{
    SELECTION_HILOGI("Start new SelectionExtension, bundleName:%{public}s, abilityName:%{public}s", bundleName.c_str(),
        abilityName.c_str());
    AAFwk::Want want;
    want.SetElementName(bundleName, abilityName);

    std::lock_guard<std::mutex> lockGuard(connectInnerMutex_);
    connectInner_ = sptr<SelectionExtensionAbilityConnection>::MakeSptr();
    if (connectInner_ == nullptr) {
        SELECTION_HILOGE("new(std::nothrow) SelectionExtensionAbilityConnection() failed!");
        return SELECTION_CONFIG_FAILURE;
    }
    auto ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, connectInner_, GetUserId());
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
        UnloadSelectionSa();
    }

    if (result.shouldRestartApp) {
        auto appInfo = result.selectionConfig.GetApplicationInfo();
        SELECTION_HILOGI("result.shouldRestartApp, appInfo: %{public}s", appInfo.c_str());
        WatchAppSwitch(SYS_SELECTION_APP, appInfo.c_str(), this);
    }
}

bool SelectionService::GetScreenLockedFlag()
{
    return isScreenLocked_.load();
}

void SelectionService::OnStart()
{
    SELECTION_HILOGI("[selectevent][SelectionService][OnStart]begin");
    Publish(SelectionService::GetInstance());
    RegisterSystemAbilityStatusChangeListener();
    SynchronizeSelectionConfig();
    WatchParams();
    SELECTION_HILOGI("[selectevent][SelectionService][OnStart]end.");
}

void SelectionService::OnStop()
{
    SELECTION_HILOGI("[selectevent][SelectionService][OnStop]begin");
    InputMonitorCancel();
    CancelFocusChangedMonitor();
    SELECTION_HILOGI("[selectevent][SelectionService][OnStop]end.");
}

void SelectionService::InitSystemAbilityChangeHandlers()
{
    systemAbilityChangeHandlers_[MULTIMODAL_INPUT_SERVICE_ID] = [this](int32_t saId, const std::string &devId) {
        InputMonitorInit();
    };
    systemAbilityChangeHandlers_[WINDOW_MANAGER_SERVICE_ID] = [this](int32_t saId, const std::string &devId) {
        InitFocusChangedMonitor();
    };
    systemAbilityChangeHandlers_[COMMON_EVENT_SERVICE_ID] = [this](int32_t saId, const std::string &devId) {
        SubscribeSysEventReceiver();
    };
}

void SelectionService::RegisterSystemAbilityStatusChangeListener()
{
    auto abilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (abilityManager == nullptr) {
        SELECTION_HILOGE("SystemAbilityManager is nullptr!");
        return;
    }
    for (auto& pair : systemAbilityChangeHandlers_) {
        auto listener = sptr<SystemAbilityStatusChangeListener>::MakeSptr([&](int32_t saId, const std::string &devId) {
            auto iter = systemAbilityChangeHandlers_.find(saId);
            if (iter == systemAbilityChangeHandlers_.end()) {
                SELECTION_HILOGE("SystemAbilityStatusChange handler is not found for [%{public}d]!", saId);
                return;
            }
            iter->second(saId, devId);
        });
        if (listener == nullptr) {
            SELECTION_HILOGE("SystemAbilityStatusChangeListener is nullptr!");
            continue;
        }
        int32_t ret = abilityManager->SubscribeSystemAbility(pair.first, listener);
        if (ret != ERR_OK) {
            SELECTION_HILOGE("Failed to SubscribeSystemAbility. ret: %{public}d", ret);
            continue;
        }
    }
}

void SelectionService::InputMonitorInit()
{
    SELECTION_HILOGI("[SelectionService] input monitor init");
    if (inputMonitorId_ >= 0) {
        SELECTION_HILOGE("There has added monitor already!");
        return;
    }

    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    auto remoteObj = sam->CheckSystemAbility(MULTIMODAL_INPUT_SERVICE_ID);
    if (remoteObj == nullptr) {
        SELECTION_HILOGE("CheckSystemAbility MULTIMODAL_INPUT_SERVICE_ID failed.");
        return;
    }

    SELECTION_HILOGI("CheckSystemAbility MULTIMODAL_INPUT_SERVICE_ID succeed.");
    std::shared_ptr<IInputEventConsumer> inputMonitor = std::make_shared<SelectionInputMonitor>();
    inputMonitorId_ = InputManager::GetInstance()->AddMonitor(inputMonitor);
    if (inputMonitorId_ < 0) {
        SELECTION_HILOGE("Failed to AddMonitor, ret: %{public}d", inputMonitorId_);
    }
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

void SelectionService::SubscribeSysEventReceiver()
{
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(CommonEventSubscribeInfo::COMMON);
    selectionSysEventReceiver_ = std::make_shared<SelectionSysEventReceiver>(subscribeInfo);
    bool subResult = CommonEventManager::SubscribeCommonEvent(selectionSysEventReceiver_);
    if (!subResult) {
        SELECTION_HILOGE("subscribe common event failed");
        return;
    }

    SELECTION_HILOGI("subscribe common event success");
}

void SelectionService::SetScreenLockedFlag(bool isLocked)
{
    isScreenLocked_.store(isLocked);
}

void SelectionService::UnloadSelectionSa()
{
    SELECTION_HILOGI("selection_service [%{public}d] unloading actively", SELECTION_FWK_SA_ID);
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        SELECTION_HILOGE("get system ability manager failed!");
        return;
    }
    int32_t ret = samgr->UnloadSystemAbility(SELECTION_FWK_SA_ID);
    if (ret != ERR_NONE) {
        SELECTION_HILOGE("Failed to unload system selection_service,  ret = [%{public}d].", ret);
    }
    SELECTION_HILOGI("UnloadSystemAbility selection_service success");
}
