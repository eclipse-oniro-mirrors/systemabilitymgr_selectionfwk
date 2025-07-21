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
#include <string>

#include "ability_connect_callback_stub.h"
#include "callback_object.h"
#include "focus_change_info.h"
#include "iselection_listener.h"
#include "selection_service_stub.h"
#include "refbase.h"
#include "system_ability.h"
#include <i_input_event_consumer.h>
#include "common_event_data.h"
#include "common_event_subscriber.h"
#include "common_event_subscribe_info.h"

namespace OHOS::SelectionFwk {
using namespace MMI;

constexpr const char *SYS_SELECTION_SWITCH = "sys.selection.switch";
constexpr const char *SYS_SELECTION_TRIGGER = "sys.selection.trigger";
constexpr const char *SYS_SELECTION_APP = "sys.selection.app";
constexpr const char *DEFAULT_SWITCH = "on";
constexpr const char *DEFAULT_TRIGGER = "ctrl";

class SelectionExtensionAbilityConnection : public OHOS::AAFwk::AbilityConnectionStub {
public:
    SelectionExtensionAbilityConnection() = default;
    ~SelectionExtensionAbilityConnection() = default;
    void OnAbilityConnectDone(
        const OHOS::AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override;
    void OnAbilityDisconnectDone(const OHOS::AppExecFwk::ElementName &element, int resultCode) override;

public:
    bool needReconnectWithException = true;
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
    ~SelectionService();

    ErrCode RegisterListener(const sptr<ISelectionListener>& listener) override;
    ErrCode UnregisterListener(const sptr<ISelectionListener>& listener) override;
    ErrCode IsCurrentSelectionApp(int pid, bool &resultValue) override;
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
    int32_t ConnectNewExtAbility(const std::string& bundleName, const std::string& abilityName);
    void DisconnectCurrentExtAbility();
    sptr<ISelectionListener> GetListener();
    void PersistSelectionConfig();
    void HandleCommonEvent(const EventFwk::CommonEventData &data);
    bool GetScreenLockedFlag();

protected:
    void OnStart() override;
    void OnStop() override;
    void HandleKeyEvent(int32_t keyCode);
    void HandlePointEvent(int32_t type);

private:
    void InitSystemAbilityChangeHandlers();
    void RegisterSystemAbilityStatusChangeListener();
    void InputMonitorInit();
    void InputMonitorCancel();
    void WatchParams();
    void InitFocusChangedMonitor();
    void CancelFocusChangedMonitor();
    void HandleFocusChanged(const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused);
    void SynchronizeSelectionConfig();
    int GetUserId();
    int LoadAccountLocalId();
    bool IsUserLoggedIn();
    bool IsSystemCalling();
    void SubscribeSysEventReceiver();
    void SetScreenLockedFlag(bool isLocked);
    void UnloadSelectionSa();

    std::map<int32_t, std::function<void(int32_t, const std::string&)>> systemAbilityChangeHandlers_;
    int32_t inputMonitorId_ {-1};
    mutable std::mutex mutex_;
    static sptr<ISelectionListener> listener_;
    sptr<SelectionExtensionAbilityConnection> connectInner_ {nullptr};
    std::mutex connectInnerMutex_;
    std::atomic<int> pid_ = -1;
    std::atomic<int> userId_ = -1;
    std::shared_ptr<SelectionSysEventReceiver> selectionSysEventReceiver_ {nullptr};
    std::atomic<bool> isScreenLocked_ = false;
};
}

#endif // SELECTION_SERVICE_H