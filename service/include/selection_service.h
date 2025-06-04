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
#include "iselection_listener.h"
#include "selection_service_stub.h"
#include "refbase.h"
#include "system_ability.h"
#include <i_input_event_consumer.h>

namespace OHOS::SelectionFwk {
using namespace MMI;

constexpr const char *SYS_SELECTION_SWITCH_USERNAM = "persist.sys.selection.switch.username";
constexpr const char *SYS_SELECTION_TRIGGER_USERNAM = "persist.sys.selection.trigger.username";
constexpr const char *SYS_SELECTION_APP_USERNAM = "persist.sys.selection.app.username";
constexpr const char *SYS_SELECTION_TRIGGER_VAL = "ctrl";

class SelectionExtensionAbilityConnection : public OHOS::AAFwk::AbilityConnectionStub {
public:
    SelectionExtensionAbilityConnection() = default;
    ~SelectionExtensionAbilityConnection() = default;
    void OnAbilityConnectDone(
        const OHOS::AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override;
    void OnAbilityDisconnectDone(const OHOS::AppExecFwk::ElementName &element, int resultCode) override;
private:
    sptr<IRemoteObject> remoteObject_;
};

class SelectionService : public SystemAbility, public SelectionServiceStub {
    DECLARE_SYSTEM_ABILITY(SelectionService);

public:
    static sptr<SelectionService> GetInstance();
    SelectionService();
    SelectionService(int32_t saId, bool runOnCreate);
    ~SelectionService();

    ErrCode AddVolume(int32_t volume, int32_t& funcResult) override;
    ErrCode RegisterListener(const sptr<IRemoteObject> &listener) override;
    ErrCode UnregisterListener(const sptr<IRemoteObject> &listener) override;
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
    int32_t ConnectNewExtAbility(const std::string& bundleName, const std::string& abilityName);
    void DisconnectCurrentExtAbility();
    sptr<ISelectionListener> GetListener();
protected:
    void OnStart() override;
    void OnStop() override;
    void HandleKeyEvent(int32_t keyCode);
    void HandlePointEvent(int32_t type);
private:
    void InputMonitorInit();
    void InputMonitorCancel();
    void WatchParams();

    int32_t inputMonitorId_ {-1};
    static sptr<SelectionService> instance_;
    static std::shared_mutex adminLock_;
    mutable std::mutex mutex_;
    sptr<ISelectionListener> listenerStub_ { nullptr };
    sptr<SelectionExtensionAbilityConnection> connectInner_ {nullptr};
};
}

#endif // SELECTION_SERVICE_H