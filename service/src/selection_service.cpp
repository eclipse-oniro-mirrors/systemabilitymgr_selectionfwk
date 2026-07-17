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
#include "selection_common.h"

#include <chrono>
#include <thread>
#include <ipc_skeleton.h>
#include <dlfcn.h>  // 用于 dlopen/dlsym

// #include "ability_manager_client.h"  // 移除：已解耦到插件
#include "iremote_object.h"
#include "system_ability_definition.h"
#include "selection_errors.h"
#include "selection_log.h"
#include <input_manager.h>
#include "parameter.h"
#include "common_event_manager.h"
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
#include "hisysevent_adapter.h"
#include "selection_timer.h"

#define SELECTION_MAX_TRY_TIMES 50
#define SELECTION_SLEEP_TIME 100
#define SELECTION_UINT32_MAX UINT32_MAX

using namespace OHOS;
using namespace OHOS::SelectionFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::MMI;
using namespace OHOS::EventFwk;

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(SelectionService::GetInstance().GetRefPtr());
const unsigned int TIMEOUT_FOR_CONNECT_DISCONNECT = 5;
sptr<ISelectionListener> SelectionService::listener_ { nullptr };

SelectionExtensionAbilityConnection::SelectionExtensionAbilityConnection(int32_t userId)
{
    userId_ = userId;
}

void SelectionExtensionAbilityConnection::OnAbilityConnectDone(
    const ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode)
{
    SELECTION_HILOGI("OnAbilityConnectDone, bundle = %{public}s, ability = %{public}s, resultCode = %{public}d",
        element.GetBundleName().c_str(), element.GetAbilityName().c_str(), resultCode);
    connectedAbilityInfo = {userId_, element.GetBundleName(), element.GetAbilityName()};

    std::lock_guard<std::mutex> lock(connectMutex_);
    if (connectPromise_) {
        connectPromise_->set_value();
        connectPromise_.reset();
    }
    SELECTION_HILOGI("OnAbilityConnectDone end.");
}

void SelectionExtensionAbilityConnection::OnAbilityDisconnectDone(const ElementName &element, int resultCode)
{
    SELECTION_HILOGI("OnAbilityDisconnectDone, bundle = %{public}s,ability = %{public}s, resultCode = %{public}d",
        element.GetBundleName().c_str(), element.GetAbilityName().c_str(), resultCode);
    connectedAbilityInfo = std::nullopt;

    std::lock_guard<std::mutex> lock(disconnectMutex_);
    if (disconnectPromise_) {
        disconnectPromise_->set_value();
    }

    auto disconnectAppInfo = element.GetBundleName() + "/" + element.GetAbilityName();
    auto selectionConfig = MemSelectionConfig::GetInstance().GetSelectionConfig();
    auto curAppInfo = selectionConfig.GetApplicationInfo();
    if (selectionConfig.GetEnable() && needReconnectWithException && curAppInfo == disconnectAppInfo) {
        SELECTION_HILOGE("do not restart app [%{public}s] even it disconnected abnormally.", curAppInfo.c_str());
    }
    SELECTION_HILOGI("OnAbilityDisconnectDone end.");
}

int32_t SelectionExtensionAbilityConnection::WaitForConnect()
{
    SELECTION_HILOGI("WaitForConnect start.");
    std::unique_lock<std::mutex> lock(connectMutex_);
    connectPromise_ = std::make_unique<std::promise<void>>();
    auto future = connectPromise_->get_future();
    lock.unlock();
    auto status = future.wait_for(std::chrono::seconds(TIMEOUT_FOR_CONNECT_DISCONNECT));
    SELECTION_CHECK(status == std::future_status::ready, return -1, "WaitForConnect timeout: %{public}d.", status);
    SELECTION_HILOGI("WaitForConnect success.");
    return 0;
}

void SelectionExtensionAbilityConnection::InitDisconnectPromise()
{
    SELECTION_HILOGI("InitDisconnectPromise");
    std::lock_guard<std::mutex> lock(disconnectMutex_);
    disconnectPromise_ = std::make_unique<std::promise<void>>();
}

void SelectionExtensionAbilityConnection::DestroyDisconnectPromise()
{
    SELECTION_HILOGI("DestroyDisconnectPromise");
    std::lock_guard<std::mutex> lock(disconnectMutex_);
    if (disconnectPromise_ != nullptr) {
        disconnectPromise_.reset();
    }
}

int32_t SelectionExtensionAbilityConnection::WaitForDisconnect()
{
    SELECTION_HILOGI("WaitForDisconnect start.");
    std::unique_lock<std::mutex> lock(disconnectMutex_);
    if (!disconnectPromise_) {
        SELECTION_HILOGE("disconnectPromise_ is null!");
        return -1;
    }
    auto future = disconnectPromise_->get_future();
    lock.unlock();
    auto status = future.wait_for(std::chrono::seconds(TIMEOUT_FOR_CONNECT_DISCONNECT));
    SELECTION_CHECK(status == std::future_status::ready, return -1, "WaitForDisConnect timeout: %{public}d.", status);
    SELECTION_HILOGI("WaitForDisconnect success.");
    return 0;
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

ErrCode SelectionService::RegisterListener(const sptr<ISelectionListener>& listener)
{
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
    if (!SelectionAppValidator::GetInstance().Validate()) {
        return SelectionServiceError::UNAUTHENTICATED_ERROR;
    }
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

ErrCode SelectionService::GetSelectionContent(std::string& selectionContent)
{
    SELECTION_HILOGI("[SelectionService] GetSelectionContent in");
    if (!SelectionAppValidator::GetInstance().Validate()) {
        return SelectionServiceError::UNAUTHENTICATED_ERROR;
    }

    if (!inputMonitor_) {
        return SelectionServiceError::INVALID_DATA;
    }

    if (!inputMonitor_->GetCanGetSelectionContentFlag()) {
        SELECTION_HILOGE("GetSelectionContent at wrong timing.");
        return SelectionServiceError::INVALID_TIMING;
    }

    auto ret = inputMonitor_->GetSelectionContent(selectionContent);
    return ret;
}

ErrCode SelectionService::SetPanelShowingStatus(bool status)
{
    SELECTION_HILOGI("[SelectionService] SetPanelShowingStatus in");

    if (!inputMonitor_) {
        return SelectionServiceError::INVALID_DATA;
    }

    auto ret = inputMonitor_->SetPanelShowingStatus(status);
    return ret;
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
        auto selectionConfig = MemSelectionConfig::GetInstance().GetSelectionConfig();
        dprintf(fd, "selection.switch: %s\n", selectionConfig.GetEnable() ? "on" : "off");
        dprintf(fd, "selection.app: %s\n", selectionConfig.GetApplicationInfo().c_str());
        dprintf(fd, "selection.trigger: %s\n", selectionConfig.GetTriggered() ? "ctrl" : "immediate");
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
    SELECTION_CHECK(key != nullptr && value != nullptr, return, "key or value is nullptr");
    SELECTION_HILOGI("%{public}s: value=[%{public}s], DEFAULT_SWITCH =[%{public}s]", key, value, DEFAULT_SWITCH);
    SelectionService *selectionService = static_cast<SelectionService *>(context);
    SELECTION_CHECK(selectionService != nullptr, return, "selectionService is nullptr");
    bool isEnabledValue = (strcmp(value, DEFAULT_SWITCH) == 0);
    SELECTION_HILOGI("isEnabledValue is %{public}d", isEnabledValue);
    MemSelectionConfig::GetInstance().SetEnabled(isEnabledValue);

    selectionService->PersistSelectionConfig();
}

static void WatchTriggerMode(const char *key, const char *value, void *context)
{
    SELECTION_CHECK(key != nullptr && value != nullptr, return, "key or value is nullptr");
    SELECTION_HILOGI("WatchTriggerMode begin, %{public}s: value=%{public}s", key, value);
    SelectionService *selectionService = static_cast<SelectionService *>(context);
    SELECTION_CHECK(selectionService != nullptr, return, "selectionService is nullptr");
    bool triggerValue = (strcmp(value, DEFAULT_TRIGGER) == 0);
    SELECTION_HILOGI("triggerValue is %{public}d", triggerValue);
    MemSelectionConfig::GetInstance().SetTriggered(triggerValue);

    selectionService->PersistSelectionConfig();
}

static void WatchAppSwitch(const char *key, const char *value, void *context)
{
    SELECTION_CHECK(key != nullptr && value != nullptr, return, "key or value is nullptr");
    SELECTION_HILOGI("WatchAppSwitch begin, %{public}s: value=%{public}s", key, value);
    SelectionService *selectionService = static_cast<SelectionService *>(context);
    SELECTION_CHECK(selectionService != nullptr, return, "selectionService is nullptr");

    const std::string appInfoStr = value;
    auto appInfo = ParseAppInfo(appInfoStr);
    if (!appInfo.has_value()) {
        return;
    }
    MemSelectionConfig::GetInstance().SetApplicationInfo(appInfoStr);
    selectionService->PersistSelectionConfig();
    if (!MemSelectionConfig::GetInstance().GetEnable()) {
        SELECTION_HILOGI("Do not reconnect ability because switch is off.");
        return;
    }
    SELECTION_HILOGI("app switch, disconnect current extAbility");
    selectionService->DisconnectCurrentExtAbility();
}

static uint32_t StrToUint(const std::string &value)
{
    errno = 0;
    char *pEnd = nullptr;
    uint64_t result = std::strtoul(value.c_str(), &pEnd, 0);
    if (pEnd == value.c_str() || result > SELECTION_UINT32_MAX || errno == ERANGE) {
        return 0;
    }
    return static_cast<uint32_t>(result);
}

static void WatchTimeoutChange(const char *key, const char *value, void *context)
{
    SELECTION_CHECK(key != nullptr && value != nullptr, return, "key or value is nullptr");
    SELECTION_HILOGI("WatchTimeoutChange begin, %{public}s: value=%{public}s", key, value);
    const std::string timeoutStr(value);
    uint32_t timeout = StrToUint(timeoutStr);
    SELECTION_CHECK(timeout != 0, return, "Invild timeout");
    MemSelectionConfig::GetInstance().SetTimeout(timeout);
}

void SelectionService::PersistSelectionConfig()
{
    if (!CheckUserLoggedIn()) {
        SELECTION_HILOGW("Do not save selection config to DB because user is not logged in.");
        return;
    }
    auto selectionConfig = MemSelectionConfig::GetInstance().GetSelectionConfig();
    SELECTION_HILOGI("========== PersistSelectionConfig: Start ==========");

    // 🔧 使用简化的插件加载方式
    if (!LoadPluginSo()) {
        SELECTION_HILOGW("Using in-memory config as fallback, service continues to run");
        return;
    }

    if (databaseSave_) {
        int ret = databaseSave_(GetUserId(), &selectionConfig);
        if (ret != SELECTION_CONFIG_OK) {
            SELECTION_HILOGE("Save database failed. ret = %{public}d", ret);
        }
    } else {
        SELECTION_HILOGE("DatabaseSaveConfig function not available");
    }

    SELECTION_HILOGI("========== PersistSelectionConfig: End ==========");
}

void SelectionService::HandleCommonEvent(const CommonEventData &data)
{
    const AAFwk::Want &want = data.GetWant();
    std::string action = want.GetAction();
    auto element = want.GetElement();
    SELECTION_HILOGI("the action: %{public}s, bundleName: %{public}s, abilityName: %{public}s",
        action.c_str(), element.GetBundleName().c_str(), element.GetAbilityName().c_str());
    if (action == CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED) {
        SetScreenLockedFlag(true);
    } else if (action == CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED) {
        SetScreenLockedFlag(false);
    } else if (action == CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        SynchronizeSelectionConfig();
    } else if (action == CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED ||
               action == CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED) {
        WatchExtAbilityInstalled(element.GetBundleName(), element.GetAbilityName());
    }
}

void SelectionService::Init()
{
    SelectionConfigComparator::GetInstance().Init();
    SynchronizeSelectionConfig();
    RegisterSystemAbilityStatusChangeListener();
    WatchParams();
}

void SelectionService::Shutdown()
{
    InputMonitorCancel();
    CancelFocusChangedMonitor();
    UnsubscribeSysEventReceiver();
}

int32_t SelectionService::DoConnectNewExtAbility(const std::string& bundleName, const std::string& abilityName)
{
    SELECTION_HILOGI("Start new SelectionExtension, bundleName:%{public}s, abilityName:%{public}s", bundleName.c_str(),
        abilityName.c_str());
    AAFwk::Want want;
    want.SetElementName(bundleName, abilityName);

    int32_t userId = GetUserId();
    connectInner_ = sptr<SelectionExtensionAbilityConnection>::MakeSptr(userId);
    if (connectInner_ == nullptr) {
        SELECTION_HILOGE("new(std::nothrow) SelectionExtensionAbilityConnection() failed!");
        return SELECTION_CONFIG_FAILURE;
    }

    // 🔧 使用简化的插件加载方式
    if (!LoadPluginSo() || !abilityConnect_) {
        SELECTION_HILOGE("Ability manager plugin not available");
        connectInner_ = nullptr;
        return SELECTION_CONFIG_FAILURE;
    }

    auto ret = abilityConnect_(&want, &connectInner_, userId);
    if (ret != 0) {
        SELECTION_HILOGE("[selectevent] StartExtensionAbility failed. error code is %{public}d.", ret);
        connectInner_ = nullptr;
        return ret;
    }
    ret = connectInner_->WaitForConnect();
    if (ret != 0) {
        HisyseventAdapter::GetInstance()->ReportShowPanelFailed(bundleName, ret,
            static_cast<int32_t>(SelectFailedReason::CONNECT_EXTENSION_TIMEOUT));
        SELECTION_HILOGI("[selectevent] StartExtensionAbility timeout.");
        return ret;
    }
    SELECTION_HILOGI("[selectevent] StartExtensionAbility success.");
    return 0;
}

void SelectionService::DoDisconnectCurrentExtAbility()
{
    pid_.store(-1);
    SELECTION_HILOGI("Disconnect current extensionAbility");
    SELECTION_CHECK(connectInner_ != nullptr, return, "connectInner_ is null");

    connectInner_->needReconnectWithException = false;
    connectInner_->InitDisconnectPromise();

    // 如果插件已卸载，直接清理连接对象，不重新加载插件
    if (!abilityDisconnect_) {
        SELECTION_HILOGW("Ability manager plugin not available, clean up connection only");
        connectInner_->DestroyDisconnectPromise();
        connectInner_ = nullptr;
        return;
    }

    int32_t ret = abilityDisconnect_(&connectInner_);
    if (ret != ERR_OK) {
        connectInner_->DestroyDisconnectPromise();
        SELECTION_HILOGE("DisconnectServiceAbility failed, ret: %{public}d", ret);
        return;
    }
    ret = connectInner_->WaitForDisconnect();
    if (ret != 0) {
        SELECTION_HILOGI("hisysevent report DISCONNECT_EXTENSION_TIMEOUT");
        HisyseventAdapter::GetInstance()->ReportShowPanelFailed("selection_service",
            ret, static_cast<int32_t>(SelectFailedReason::DISCONNECT_EXTENSION_TIMEOUT));
    }

    connectInner_->DestroyDisconnectPromise();
    connectInner_ = nullptr;
    SELECTION_HILOGI("[selectevent] DisconnectAbility success.");
}

bool SelectionService::HasExtAbilityConnection() const
{
    if (connectInner_ != nullptr && connectInner_->connectedAbilityInfo.has_value()) {
        return true;
    }
    SELECTION_HILOGI("No selection extension is connected");
    return false;
}

int SelectionService::GetCurrentSelectionAppInfo(std::string &bundleName, std::string &abilityName)
{
    const std::string appInfoStr = MemSelectionConfig::GetInstance().GetApplicationInfo();
    auto appInfo = ParseAppInfo(appInfoStr);
    if (!appInfo.has_value()) {
        return -1;
    }
    bundleName = std::get<0>(appInfo.value());
    abilityName = std::get<1>(appInfo.value());
    return 0;
}

bool SelectionService::IsAnySelectionPanelShowing()
{
    std::vector<sptr<OHOS::Rosen::WindowVisibilityInfo>> windowVisibilityInfos;
#ifdef SCENE_BOARD_ENABLE
    Rosen::WMError ret = Rosen::WindowManagerLite::GetInstance().GetVisibilityWindowInfo(windowVisibilityInfos);
#else
    Rosen::WMError ret = Rosen::WindowManager::GetInstance().GetVisibilityWindowInfo(windowVisibilityInfos);
#endif
    SELECTION_CHECK(ret == OHOS::Rosen::WMError::WM_OK, return false,
        "GetVisibilityWindowInfo error, ret is: %{public}d", ret);

    std::string currentBundleName;
    std::string currentAbilityName;
    SELECTION_CHECK(GetCurrentSelectionAppInfo(currentBundleName, currentAbilityName) == 0, return false,
        "current appInfo is empty");
    SELECTION_CHECK(currentBundleName != "" && currentAbilityName != "", return false,
        "bundleName or ability is empty string");

    for (auto windowVisibilityInfo : windowVisibilityInfos) {
        if (currentBundleName == windowVisibilityInfo->GetBundleName() ||
            currentAbilityName == windowVisibilityInfo->GetAbilityName()) {
            SELECTION_HILOGI("the panel is showing");
            return true;
        }
    }
    SELECTION_HILOGI("there is no panel showing");
    return false;
}

int SelectionService::ConnectExtAbilityFromConfig()
{
    std::string bundleName;
    std::string abilityName;
    SELECTION_CHECK(GetCurrentSelectionAppInfo(bundleName, abilityName) == 0, return -1, "current appInfo is empty");
    std::lock_guard<std::mutex> lockGuard(connectMutex_);
    int ret = DoConnectNewExtAbility(bundleName, abilityName);
    SELECTION_HILOGI("ConnectExtAbilityFromConfig ret = %{public}d", ret);
    return ret;
}

int32_t SelectionService::ConnectNewExtAbility(const std::string& bundleName, const std::string& abilityName)
{
    std::lock_guard<std::mutex> lockGuard(connectMutex_);
    AbilityRuntimeInfo newAbilityInfo{GetUserId(), bundleName, abilityName};
    if (connectInner_ != nullptr &&
        connectInner_->connectedAbilityInfo.has_value() &&
        newAbilityInfo == connectInner_->connectedAbilityInfo.value()) {
        SELECTION_HILOGI("Ability (userId:%{public}d, bundleName:%{public}s, abilityName:%{public}s) "
            "has been connected.",
            newAbilityInfo.userId, newAbilityInfo.bundleName.c_str(), newAbilityInfo.abilityName.c_str());
        return 0;
    }
    return DoConnectNewExtAbility(bundleName, abilityName);
}

int32_t SelectionService::ReconnectExtAbility(const std::string& bundleName, const std::string& abilityName)
{
    SELECTION_HILOGI("ReconnectExtAbility start.");
    std::lock_guard<std::mutex> lockGuard(connectMutex_);
    AbilityRuntimeInfo newAbilityInfo{GetUserId(), bundleName, abilityName};
    if (connectInner_ != nullptr &&
        connectInner_->connectedAbilityInfo.has_value() &&
        newAbilityInfo == connectInner_->connectedAbilityInfo.value()) {
        SELECTION_HILOGI("Ability (userId:%{public}d, bundleName:%{public}s, abilityName:%{public}s) "
            "has been connected.",
            newAbilityInfo.userId, newAbilityInfo.bundleName.c_str(), newAbilityInfo.abilityName.c_str());
        return 0;
    }
    DoDisconnectCurrentExtAbility();
    /* no need to connect extension ability, the function name should change */
    SELECTION_HILOGI("ReconnectExtAbility end.");
    return 0;
}

void SelectionService::DisconnectCurrentExtAbility()
{
    SELECTION_HILOGI("DisconnectCurrentExtAbility start.");
    std::lock_guard<std::mutex> lockGuard(connectMutex_);
    DoDisconnectCurrentExtAbility();
    SELECTION_HILOGI("DisconnectCurrentExtAbility end.");
}

void SelectionService::WatchParams()
{
    SELECTION_HILOGI("WatchParams begin");
    if (WatchParameter(SYS_SELECTION_SWITCH, WatchEnableSwitch, this) != 0) {
        SELECTION_HILOGE("Failed to watch SYS_SELECTION_SWITCH");
    }
    if (WatchParameter(SYS_SELECTION_TRIGGER, WatchTriggerMode, this) != 0) {
        SELECTION_HILOGE("Failed to watch SYS_SELECTION_TRIGGER");
    }
    if (WatchParameter(SYS_SELECTION_APP, WatchAppSwitch, this) != 0) {
        SELECTION_HILOGE("Failed to watch SYS_SELECTION_APP");
    }
    if (WatchParameter(SYS_SELECTION_TIMEOUT, WatchTimeoutChange, this) != 0) {
        SELECTION_HILOGE("Failed to watch SYS_SELECTION_TIMEOUT");
    }
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

bool SelectionService::CheckUserLoggedIn()
{
    return LoadAccountLocalId() != -1;
}

void SelectionService::SynchronizeSelectionConfig()
{
    if (!CheckUserLoggedIn()) {
        SELECTION_HILOGW("No selection config sync because user is not logged in.");
        return;
    }

    SelectionConfig sysSelectionConfig = SysSelectionConfigRepository::GetInstance()->GetSysParameters();
    SELECTION_HILOGI("sysSelectionConfig: %{public}s", sysSelectionConfig.ToString().c_str());

    // 从数据库加载配置
    auto dbSelectionConfig = LoadDatabaseSelectionConfig();

    // 比较配置
    ComparisionResult result;
    {
        std::lock_guard<std::mutex> lockGuard(connectMutex_);
        result = SelectionConfigComparator::GetInstance().Compare(userId_.load(), sysSelectionConfig,
            dbSelectionConfig, (connectInner_ ? connectInner_->connectedAbilityInfo : std::nullopt));
    }
    MemSelectionConfig::GetInstance().SetSelectionConfig(result.selectionConfig);

    // 根据方向同步配置
    if (result.direction == SyncDirection::FromDbToSys) {
        SyncConfigToSystem(result.selectionConfig);
    } else if (result.direction == SyncDirection::FromSysToDb) {
        SyncConfigToDatabase(userId_.load(), result.selectionConfig);
    }

    // 处理需要创建的情况
    if (result.shouldCreate) {
        SELECTION_HILOGI("result.shouldCreate");
        SyncConfigToDatabase(userId_.load(), result.selectionConfig);
        SELECTION_HILOGI("result.selectionConfig.isEnable = %{public}d", result.selectionConfig.GetEnable());
        SyncConfigToSystem(result.selectionConfig);
    }

    // 处理同步结果
    ProcessSyncResult(result);
}

std::optional<SelectionConfig> SelectionService::LoadDatabaseSelectionConfig()
{
    // 🔧 使用简化的插件加载方式
    if (!LoadPluginSo()) {
        SELECTION_HILOGW("Using system default config as fallback");
        return std::nullopt;
    }

    if (!databaseGet_) {
        SELECTION_HILOGE("Database_GetConfig function not available");
        return std::nullopt;
    }

    SelectionConfig config;
    int ret = databaseGet_(userId_.load(), &config);
    if (ret == 0) {
        return config;
    } else if (ret == SELECTION_CONFIG_NOT_FOUND) {
        SELECTION_HILOGI("No config found in database for user %{public}d", userId_.load());
        return std::nullopt;
    } else {
        SELECTION_HILOGE("Failed to get config, error=%{public}d", ret);
        return std::nullopt;
    }
}

void SelectionService::SyncConfigToSystem(const SelectionConfig& config)
{
    SELECTION_HILOGI("SyncConfigToSystem: %{public}s", config.ToString().c_str());
    SysSelectionConfigRepository::GetInstance()->SetSysParameters(config);
}

void SelectionService::SyncConfigToDatabase(int32_t userId, const SelectionConfig& config)
{
    SELECTION_HILOGI("SyncConfigToDatabase: %{public}s", config.ToString().c_str());

    // 🔧 使用简化的插件加载方式
    if (!LoadPluginSo() || !databaseSave_) {
        SELECTION_HILOGW("Config saved to system params as fallback");
        return;
    }

    auto ret = databaseSave_(userId, &config);
    if (ret != SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("Save database failed. ret = %{public}d", ret);
    }
}

void SelectionService::ProcessSyncResult(const ComparisionResult& result)
{
    // 处理需要停止服务的情况
    if (result.shouldStop) {
        SELECTION_HILOGI("result.shouldStop");
        SysSelectionConfigRepository::GetInstance()->DisableSAService();
        UnloadService();
        return;
    }

    // 解析应用信息
    auto appInfoStr = result.selectionConfig.GetApplicationInfo();
    auto appInfo = ParseAppInfo(appInfoStr);
    if (!appInfo.has_value()) {
        return;
    }

    // 处理需要启动的情况
    if (result.shouldStart) {
        SELECTION_HILOGI("result.shouldStart");
    }

    // 处理需要重启应用的情况
    if (result.shouldRestartApp) {
        SELECTION_HILOGI("result.shouldRestartApp");
        ReconnectExtAbility(std::get<0>(appInfo.value()), std::get<1>(appInfo.value()));
    }
}

bool SelectionService::GetScreenLockedFlag()
{
    return isScreenLocked_.load();
}

void SelectionService::WatchExtAbilityInstalled(const std::string& bundleName, const std::string& abilityName)
{
    std::lock_guard<std::mutex> lockGuard(connectMutex_);
    if (connectInner_ != nullptr) {
        SELECTION_HILOGI("WatchExtAbilityInstalled: connectInner is not nullptr");
        return;
    }
    auto selectionConfig = MemSelectionConfig::GetInstance().GetSelectionConfig();
    auto appInfo = ParseAppInfo(selectionConfig.GetApplicationInfo());
    if (!appInfo.has_value()) {
        return;
    }

    std::string targetBundleName;
    std::string targetAbilityName;
    std::tie(targetBundleName, targetAbilityName) = appInfo.value();

    SELECTION_HILOGI("WatchExtAbilityInstalled: addedBundleName is %{public}s, addedAbilityName is %{public}s; "
        "targetBundleName is %{public}s, targetAbilityName is %{public}s",
        bundleName.c_str(), abilityName.c_str(), targetBundleName.c_str(), targetAbilityName.c_str());

    if (targetBundleName == bundleName) {
        SELECTION_HILOGI("user is installing the selection extension app: %{public}s", bundleName.c_str());
    }
}

void SelectionService::OnStart()
{
    SELECTION_HILOGI("[selectevent][SelectionService][OnStart]begin");
    int ret = WatchParameter(BOOTEVENT_BOOT_COMPLETED, [](const char* key, const char* value, void* context) {
        SelectionService *selectionService = static_cast<SelectionService *>(context);
        selectionService->PerformParamBootCompleted(key, value, context);
    }, reinterpret_cast<void*>(this));
    if (ret != 0) {
        SELECTION_HILOGE("Faild to watch %{public}s with ret %{public}d, init now.", BOOTEVENT_BOOT_COMPLETED, ret);
        Init();
    }
    Publish(SelectionService::GetInstance());
    HisyseventAdapter::GetInstance()->StartHisyseventTimer();
    SELECTION_HILOGI("timer init, [selectevent][SelectionService][OnStart]end.");
}

void SelectionService::OnStop()
{
    SELECTION_HILOGI("[selectevent][SelectionService][OnStop]begin");
    Shutdown();
    UnloadPluginSo();
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
    SELECTION_HILOGI("RegisterSystemAbilityStatusChangeListener start!");
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
    if (isMonitorInitialized_) {
        SELECTION_HILOGE("The monitor has been initialized.");
        return;
    }
    if (inputMonitorId_ >= 0) {
        SELECTION_HILOGE("There has added monitor already!");
        return;
    }

    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sam == nullptr) {
        SELECTION_HILOGE("get system ability manager failed!");
        return;
    }
    auto remoteObj = sam->CheckSystemAbility(MULTIMODAL_INPUT_SERVICE_ID);
    if (remoteObj == nullptr) {
        SELECTION_HILOGE("CheckSystemAbility MULTIMODAL_INPUT_SERVICE_ID failed.");
        return;
    }

    SELECTION_HILOGI("CheckSystemAbility MULTIMODAL_INPUT_SERVICE_ID succeed.");
    inputMonitor_ = std::make_shared<SelectionInputMonitor>();
    inputMonitorId_ = InputManager::GetInstance()->AddMonitor(inputMonitor_);
    if (inputMonitorId_ < 0) {
        SELECTION_HILOGE("Failed to AddMonitor, ret: %{public}d", inputMonitorId_);
    }
    isMonitorInitialized_ = inputMonitorId_ >= 0;
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
    std::lock_guard<std::mutex> lock(initMutex_);
    if (isWindowInitialized_) {
        SELECTION_HILOGE("The forcus changed listener has been registered.");
        return;
    }
    FocusMonitorManager::GetInstance().RegisterFocusChangedListener(
        [this](const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused) {
            HandleFocusChanged(focusChangeInfo, isFocused);
        });
    isWindowInitialized_ = true;
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

void SelectionService::SubscribeSysEventReceiver()
{
    SELECTION_HILOGI("SubscribeSysEventReceiver start.");
    if (isCommonEventInitialized_) {
        SELECTION_HILOGE("The common event has been subscribed.");
        return;
    }
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED);
    CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(CommonEventSubscribeInfo::COMMON);
    selectionSysEventReceiver_ = std::make_shared<SelectionSysEventReceiver>(subscribeInfo);
    bool subResult = CommonEventManager::SubscribeCommonEvent(selectionSysEventReceiver_);
    SELECTION_CHECK(subResult, return, "subscribe common event failed");
    isCommonEventInitialized_ = true;
    SELECTION_HILOGI("subscribe common event success");
}

void SelectionService::UnsubscribeSysEventReceiver()
{
    SELECTION_HILOGI("UnsubscribeSysEventReceiver start.");
    SELECTION_CHECK(isCommonEventInitialized_, return, "The common event has not been subscribed.");

    bool subResult = CommonEventManager::UnSubscribeCommonEvent(selectionSysEventReceiver_);
    if (!subResult) {
        SELECTION_HILOGE("unsubscribe common event failed");
        return;
    }
    isCommonEventInitialized_ = false;
    SELECTION_HILOGI("unsubscribe common event success");
}

void SelectionService::SetScreenLockedFlag(bool isLocked)
{
    isScreenLocked_.store(isLocked);
}

void SelectionService::UnloadService()
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
    } else {
        SELECTION_HILOGI("UnloadSystemAbility selection_service success");
    }
}

void SelectionService::PerformParamBootCompleted(const char* key, const char* value, void* context)
{
    SELECTION_CHECK(key != nullptr && value != nullptr, return, "key or value is nullptr");
    if (strcmp(value, "true") != 0) {
        return;
    }

    Init();
}

// 🔧 插件加载辅助方法实现（简化版：直接使用 dlopen）
bool SelectionService::LoadPluginSo()
{
    std::lock_guard<std::mutex> lock(pluginMutex_);
    if (pluginSo_) {
        return true;  // 已加载
    }

    pluginSo_ = dlopen(PLUGIN_SO_PATH, RTLD_LAZY);
    if (!pluginSo_) {
        SELECTION_HILOGE("Failed to dlopen %{public}s: %{public}s", PLUGIN_SO_PATH, dlerror());
        return false;
    }

    // 加载数据库函数
    databaseSave_ = (DatabaseSaveConfigFunc)dlsym(pluginSo_, "DatabaseSaveConfig");
    databaseGet_ = (DatabaseGetConfigFunc)dlsym(pluginSo_, "DatabaseGetConfig");
    databaseAvailable_ = (DatabaseIsAvailableFunc)dlsym(pluginSo_, "DatabaseIsAvailable");

    // 加载能力管理函数
    abilityConnect_ = (AbilityManagerConnectFunc)dlsym(pluginSo_, "AbilityManagerConnectAbility");
    abilityDisconnect_ = (AbilityManagerDisconnectFunc)dlsym(pluginSo_, "AbilityManagerDisconnectAbility");
    abilityAvailable_ = (AbilityManagerIsAvailableFunc)dlsym(pluginSo_, "AbilityManagerIsAvailable");

    // 加载剪贴板函数
    pasteboardGetContent_ = (PasteboardGetContentFunc)dlsym(pluginSo_, "PasteboardGetSelectionContent");
    pasteboardCanGetContent_ = (PasteboardCanGetContentFunc)dlsym(pluginSo_, "PasteboardCanGetSelectionContent");
    pasteboardSetFlag_ = (PasteboardSetFlagFunc)dlsym(pluginSo_, "PasteboardSetCanGetSelectionContentFlag");
    if (!databaseSave_ || !databaseGet_ || !databaseAvailable_ ||
        !abilityConnect_ || !abilityDisconnect_ || !abilityAvailable_ ||
        !pasteboardGetContent_ || !pasteboardCanGetContent_ || !pasteboardSetFlag_) {
        SELECTION_HILOGE("Failed to load plugin functions");
        dlclose(pluginSo_);
        pluginSo_ = nullptr;
        return false;
    }

    SELECTION_HILOGI("Plugin so loaded successfully");
    ResetPluginUnloadTimer();  // 启动/重置5分钟卸载定时器
    return true;
}

void SelectionService::UnloadPluginSo()
{
    // 取消卸载定时器
    if (pluginUnloadTimerId_ != 0) {
        SelectionFwkTimer::GetInstance()->UnRegister(pluginUnloadTimerId_);
        pluginUnloadTimerId_ = 0;
    }

    if (pluginSo_) {
        // 调用清理函数
        auto cleanupFunc = (void(*)())dlsym(pluginSo_, "PluginCleanupAll");
        if (cleanupFunc) {
            cleanupFunc();
        }

        // 延时等待依赖的so完成清理，避免dlclose时crash
        std::this_thread::sleep_for(std::chrono::milliseconds(CLEANUP_DELAY_TIME));

        dlclose(pluginSo_);
        pluginSo_ = nullptr;
    }

    databaseSave_ = nullptr;
    databaseGet_ = nullptr;
    databaseAvailable_ = nullptr;
    abilityConnect_ = nullptr;
    abilityDisconnect_ = nullptr;
    abilityAvailable_ = nullptr;
    pasteboardGetContent_ = nullptr;
    pasteboardCanGetContent_ = nullptr;
    pasteboardSetFlag_ = nullptr;
}

void SelectionService::ResetPluginUnloadTimer()
{
    // 取消旧的定时器
    if (pluginUnloadTimerId_ != 0) {
        SelectionFwkTimer::GetInstance()->UnRegister(pluginUnloadTimerId_);
        pluginUnloadTimerId_ = 0;
    }

    // 启动新的定时器：5分钟后自动卸载插件
    pluginUnloadTimerId_ = SelectionFwkTimer::GetInstance()->Register([this]() {
        OnPluginUnloadTimer();
    }, PLUGIN_UNLOAD_TIMEOUT_MS);

    SELECTION_HILOGI("Plugin unload timer reset: %{public}d ms", PLUGIN_UNLOAD_TIMEOUT_MS);
}

void SelectionService::OnPluginUnloadTimer()
{
    SELECTION_HILOGI("Plugin unload timer triggered, unloading plugin");
    // 如果有划词扩展的弹窗在显示，则不断开扩展也不卸载插件
    if (IsAnySelectionPanelShowing()) {
        SELECTION_HILOGI("OnPluginUnloadTimer: Selection panel is showing, keep plugin and extension");
        return;
    }
    // 先断开扩展应用连接（如果存在），避免插件卸载后无法断开连接
    if (HasExtAbilityConnection()) {
        SELECTION_HILOGI("Disconnecting extension ability before unloading plugin");
        DisconnectCurrentExtAbility();
    }
    UnloadPluginSo();
    pluginUnloadTimerId_ = 0;
}

int SelectionService::GetDatabaseConfig(int32_t uid, SelectionConfig& config)
{
    if (!LoadPluginSo() || !databaseGet_) {
        SELECTION_HILOGE("Database plugin not available");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }

    SelectionConfig* configPtr = &config;
    int ret = databaseGet_(uid, configPtr);
    if (ret != 0) {
        SELECTION_HILOGE("DatabaseGetConfig failed, ret=%{public}d", ret);
    }
    ResetPluginUnloadTimer();  // 重置5分钟卸载定时器
    return ret;
}

int SelectionService::SaveDatabaseConfig(int32_t uid, const SelectionConfig& config)
{
    if (!LoadPluginSo() || !databaseSave_) {
        SELECTION_HILOGE("Database plugin not available");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }

    int ret = databaseSave_(uid, &config);
    if (ret != 0) {
        SELECTION_HILOGE("DatabaseSaveConfig failed, ret=%{public}d", ret);
    }
    ResetPluginUnloadTimer();  // 重置5分钟卸载定时器
    return ret;
}

bool SelectionService::IsDatabaseAvailable()
{
    if (!LoadPluginSo() || !databaseAvailable_) {
        return false;
    }
    return databaseAvailable_() != 0;
}

int SelectionService::GetPasteboardContent(std::string& content, uint32_t windowId, const std::string& bundleName)
{
    constexpr uint32_t MAX_PASTERBOARD_TEXT_LENGTH = 2000;
    constexpr uint32_t BYTES_PER_CHINESE_CHAR = 3;
    constexpr uint32_t bufferSize = MAX_PASTERBOARD_TEXT_LENGTH * BYTES_PER_CHINESE_CHAR + 1;

    if (!LoadPluginSo() || !pasteboardGetContent_) {
        SELECTION_HILOGE("Pasteboard plugin not available");
        return SelectionServiceError::INVALID_DATA;
    }

    char buffer[bufferSize] = {0};
    int ret = pasteboardGetContent_(buffer, bufferSize, windowId, bundleName.c_str());
    if (ret == 0) {
        content = buffer;
    }
    ResetPluginUnloadTimer();  // 重置5分钟卸载定时器
    return ret;
}

bool SelectionService::CanGetPasteboardContent()
{
    if (!LoadPluginSo() || !pasteboardCanGetContent_) {
        return false;
    }
    bool result = pasteboardCanGetContent_() != 0;
    ResetPluginUnloadTimer();  // 重置5分钟卸载定时器
    return result;
}

void SelectionService::SetPasteboardFlag(bool flag)
{
    if (LoadPluginSo() && pasteboardSetFlag_) {
        pasteboardSetFlag_(flag ? 1 : 0);
        ResetPluginUnloadTimer();  // 重置5分钟卸载定时器
    }
}
