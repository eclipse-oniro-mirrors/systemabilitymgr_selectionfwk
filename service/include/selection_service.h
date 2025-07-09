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

#include "ability_connect_callback_stub.h"
#include "callback_object.h"
#include "focus_change_info.h"
#include "iselection_listener.h"
#include "selection_service_stub.h"
#include "refbase.h"
#include "system_ability.h"
#include <i_input_event_consumer.h>

namespace OHOS::SelectionFwk {
using namespace MMI;

constexpr const char *SYS_SELECTION_SWITCH = "sys.selection.switch";
constexpr const char *SYS_SELECTION_TRIGGER = "sys.selection.trigger";
constexpr const char *SYS_SELECTION_APP = "sys.selection.app";
constexpr const char *DEFAULT_SWITCH = "on";
constexpr const char *DEFAULT_TRIGGER = "ctrl";
constexpr const char *DEFAULT_SELECTION_APP = "com.hm.youdao/ExtensionAbility";

class SelectionExtensionAbilityConnection : public OHOS::AAFwk::AbilityConnectionStub {
public:
    SelectionExtensionAbilityConnection() = default;
    ~SelectionExtensionAbilityConnection() = default;
    void OnAbilityConnectDone(
        const OHOS::AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override;
    void OnAbilityDisconnectDone(const OHOS::AppExecFwk::ElementName &element, int resultCode) override;
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

protected:
    void OnStart() override;
    void OnStop() override;
    void HandleKeyEvent(int32_t keyCode);
    void HandlePointEvent(int32_t type);

private:
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

    int32_t inputMonitorId_ {-1};
    mutable std::mutex mutex_;
    static sptr<ISelectionListener> listener_;
    sptr<SelectionExtensionAbilityConnection> connectInner_ {nullptr};
    std::mutex connectInnerMutex_;
    std::atomic<int> pid_ = -1;
    std::atomic<int> userId_ = -1;
};
}

#endif // SELECTION_SERVICE_H