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

#include <map>
#include <mutex>
#include <future>
#include <string>

#include "ability_connect_callback_stub.h"
#include "focus_change_info.h"
#include "iselection_listener.h"
#include "selection_service_stub.h"
#include "refbase.h"
#include "system_ability.h"
#include <i_input_event_consumer.h>
#include "common_event_data.h"
#include "common_event_subscriber.h"
#include "common_event_subscribe_info.h"
#include "selection_common.h"
#include "selection_config_comparator.h"
#include "selection_input_monitor.h"

namespace OHOS::SelectionFwk {
using namespace MMI;

constexpr const char *BOOTEVENT_BOOT_COMPLETED = "bootevent.boot.completed";
constexpr const char *SYS_SELECTION_SWITCH = "sys.selection.switch";
constexpr const char *SYS_SELECTION_TRIGGER = "sys.selection.trigger";
constexpr const char *SYS_SELECTION_APP = "sys.selection.app";
constexpr const char *SYS_SELECTION_TIMEOUT = "sys.selection.timeout";
constexpr const char *DEFAULT_SWITCH = "on";
constexpr const char *DEFAULT_TRIGGER = "ctrl";

constexpr const uint32_t CLEANUP_DELAY_TIME = 50;          // 清理资源与卸载so之间的延迟间隔（毫秒）

class SelectionExtensionAbilityConnection : public OHOS::AAFwk::AbilityConnectionStub {
public:
    SelectionExtensionAbilityConnection(int32_t userId);
    ~SelectionExtensionAbilityConnection() = default;
    void OnAbilityConnectDone(
        const OHOS::AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override;
    void OnAbilityDisconnectDone(const OHOS::AppExecFwk::ElementName &element, int resultCode) override;

    int32_t WaitForConnect();
    int32_t WaitForDisconnect();

    void InitDisconnectPromise();
    void DestroyDisconnectPromise();

public:
    bool needReconnectWithException = true;
    std::optional<AbilityRuntimeInfo> connectedAbilityInfo;

private:
    int32_t userId_;
    std::mutex connectMutex_;
    std::mutex disconnectMutex_;
    std::unique_ptr<std::promise<void>> connectPromise_;
    std::unique_ptr<std::promise<void>> disconnectPromise_;
};

class SelectionSysEventReceiver : public EventFwk::CommonEventSubscriber,
    public std::enable_shared_from_this<SelectionSysEventReceiver> {
public:
    SelectionSysEventReceiver(const EventFwk::CommonEventSubscribeInfo &subscribeInfo);
    ~SelectionSysEventReceiver() = default;

    void OnReceiveEvent(const EventFwk::CommonEventData &data) override;
};

class SelectionService : public SystemAbility, public SelectionServiceStub {
    DECLARE_SYSTEM_ABILITY(SelectionService);

public:
    static sptr<SelectionService> GetInstance();
    SelectionService();
    SelectionService(int32_t saId, bool runOnCreate);
    virtual ~SelectionService();

    ErrCode RegisterListener(const sptr<ISelectionListener>& listener) override;
    ErrCode UnregisterListener(const sptr<ISelectionListener>& listener) override;
    ErrCode IsCurrentSelectionApp(int pid, bool &resultValue) override;
    ErrCode GetSelectionContent(std::string& selectionContent) override;
    ErrCode SetPanelShowingStatus(bool status) override;
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
    int32_t ConnectNewExtAbility(const std::string& bundleName, const std::string& abilityName);
    int32_t ReconnectExtAbility(const std::string& bundleName, const std::string& abilityName);
    void DisconnectCurrentExtAbility();
    void UnloadService();
    bool HasExtAbilityConnection() const;
    int ConnectExtAbilityFromConfig();
    int GetCurrentSelectionAppInfo(std::string &bundleName, std::string &abilityName);
    bool IsAnySelectionPanelShowing();

    sptr<ISelectionListener> GetListener();
    void PersistSelectionConfig();
    void HandleCommonEvent(const EventFwk::CommonEventData &data);
    bool GetScreenLockedFlag();
    void WatchExtAbilityInstalled(const std::string& bundleName, const std::string& abilityName);

    // 剪贴板操作方法（供 SelectionInputMonitor 调用）
    int GetPasteboardContent(std::string& content, uint32_t windowId, const std::string& bundleName);
    bool CanGetPasteboardContent();
    void SetPasteboardFlag(bool flag);

    // 插件卸载定时器管理（供SelectionInputMonitor 调用）
    void ResetPluginUnloadTimer();

    // 数据库配置操作方法（供 SelectionConfigComparator 调用）
    int GetDatabaseConfig(int32_t uid, SelectionConfig& config);
    int SaveDatabaseConfig(int32_t uid, const SelectionConfig& config);
    bool IsDatabaseAvailable();

protected:
    void OnStart() override;
    void OnStop() override;

private:
    void Init();
    void Shutdown();
    int32_t DoConnectNewExtAbility(const std::string& bundleName, const std::string& abilityName);
    void DoDisconnectCurrentExtAbility();
    void InitSystemAbilityChangeHandlers();
    void RegisterSystemAbilityStatusChangeListener();
    void InputMonitorInit();
    void InputMonitorCancel();
    void WatchParams();
    void InitFocusChangedMonitor();
    void CancelFocusChangedMonitor();
    void HandleFocusChanged(const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused);
    void SynchronizeSelectionConfig();

    // 配置同步辅助函数
    std::optional<SelectionConfig> LoadDatabaseSelectionConfig();
    void SyncConfigToSystem(const SelectionConfig& config);
    void SyncConfigToDatabase(int32_t userId, const SelectionConfig& config);
    void ProcessSyncResult(const ComparisionResult& result);

    // 🔧 插件加载辅助方法（简化版：直接使用 dlopen）
    static constexpr const char* PLUGIN_SO_PATH = "libselection_plugins_impl.z.so";
    static constexpr uint32_t PLUGIN_UNLOAD_TIMEOUT_MS = 300000;  // 5分钟卸载超时
    bool LoadPluginSo();
    void UnloadPluginSo();
    void OnPluginUnloadTimer();

    // 数据库操作函数指针类型
    using DatabaseSaveConfigFunc = int(*)(int, const SelectionConfig*);
    using DatabaseGetConfigFunc = int(*)(int, SelectionConfig*);
    using DatabaseIsAvailableFunc = int(*)();

    // 剪贴板函数指针类型
    using PasteboardGetContentFunc = int(*)(char*, int, uint32_t, const char*);
    using PasteboardCanGetContentFunc = int(*)();
    using PasteboardSetFlagFunc = void(*)(int);

    // 能力管理操作函数指针类型
    using AbilityManagerConnectFunc = int(*)(const void*, const void*, int32_t);
    using AbilityManagerDisconnectFunc = int(*)(const void*);
    using AbilityManagerIsAvailableFunc = int(*)();

    int GetUserId();
    int LoadAccountLocalId();
    virtual bool CheckUserLoggedIn();
    void SubscribeSysEventReceiver();
    void UnsubscribeSysEventReceiver();
    void SetScreenLockedFlag(bool isLocked);
    void PerformParamBootCompleted(const char* key, const char* value, void* context);

    std::map<int32_t, std::function<void(int32_t, const std::string&)>> systemAbilityChangeHandlers_;
    std::shared_ptr<SelectionInputMonitor> inputMonitor_;

    // 🔧 插件 .so 句柄和函数指针（替代 plugin_manager）
    void* pluginSo_ = nullptr;
    int32_t pluginUnloadTimerId_ {0};  // 插件自动卸载定时器ID
    DatabaseSaveConfigFunc databaseSave_ = nullptr;
    DatabaseGetConfigFunc databaseGet_ = nullptr;
    DatabaseIsAvailableFunc databaseAvailable_ = nullptr;
    AbilityManagerConnectFunc abilityConnect_ = nullptr;
    AbilityManagerDisconnectFunc abilityDisconnect_ = nullptr;
    AbilityManagerIsAvailableFunc abilityAvailable_ = nullptr;
    PasteboardGetContentFunc pasteboardGetContent_ = nullptr;
    PasteboardCanGetContentFunc pasteboardCanGetContent_ = nullptr;
    PasteboardSetFlagFunc pasteboardSetFlag_ = nullptr;

    int32_t inputMonitorId_ {-1};
    mutable std::mutex mutex_;
    mutable std::mutex pluginMutex_;
    static sptr<ISelectionListener> listener_;
    sptr<SelectionExtensionAbilityConnection> connectInner_ {nullptr};
    std::mutex connectMutex_;
    std::atomic<int> pid_ = -1;
    std::atomic<int> userId_ = -1;
    std::shared_ptr<SelectionSysEventReceiver> selectionSysEventReceiver_ {nullptr};
    std::atomic<bool> isScreenLocked_ = false;
    std::mutex initMutex_;
    bool isMonitorInitialized_ = false;
    bool isWindowInitialized_ = false;
    bool isCommonEventInitialized_ = false;
};
}

#endif // SELECTION_SERVICE_H