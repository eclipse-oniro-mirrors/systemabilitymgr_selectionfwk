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

#include "ets_selection_engine_setting.h"
#include "selection_system_ability_utils.h"
#include "selection_listener_impl.h"

namespace OHOS {
namespace SelectionFwk {

std::shared_ptr<EtsSelectionEngineSetting> EtsSelectionEngineSetting::selectionDelegate_ = nullptr;
std::mutex EtsSelectionEngineSetting::selectionMutex_;
sptr<ISelectionListener> EtsSelectionEngineSetting::listenerStub_ = nullptr;

static void convertSelectionInfo (const SelectionInfo &selectionInfo, ohos::selectionInput::selectionManager::SelectionInfo& info)
{
    info.startDisplayX = selectionInfo.startDisplayX;
    info.startDisplayY = selectionInfo.startDisplayY;
    info.endDisplayX = selectionInfo.endDisplayX;
    info.endDisplayY = selectionInfo.endDisplayY;
    info.startWindowX = selectionInfo.startWindowX;
    info.startWindowY = selectionInfo.startWindowY;
    info.endWindowX = selectionInfo.endWindowX;
    info.endWindowY = selectionInfo.endWindowY;
    info.displayID = selectionInfo.displayId;
    info.windowID = selectionInfo.windowId;
    info.bundleName = selectionInfo.bundleName;
}
int32_t EtsSelectionEngineSetting::OnSelectionEvent(const SelectionInfo &selectionInfo)
{
    //selectionComplete
    const std::string type = "selectionComplete";
    auto callbackVec = EtsCbMap_[type];
    if (callbackVec.empty()) {
        SELECTION_HILOGE("callback of selecationComplete is empty");
        return -1;
    }
    ohos::selectionInput::selectionManager::SelectionInfo info{
        ohos::selectionInput::selectionManager::SelectionType::from_value(static_cast<int32_t>(selectionInfo.selectionType)),
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, std::string()};
    convertSelectionInfo(selectionInfo, info);
    for (auto &func : callbackVec) {
        func(info);
    }
    return 0;
}

void EtsSelectionEngineSetting::Subscribe(const std::string &type, callbackTypePara &&cb)
{
    SELECTION_HILOGI("EtsSelectionEngineSetting::Subscribe type: %{public}s.", type.c_str());
    auto engine = EtsSelectionEngineSetting::GetEtsSelectionEngineSetting();
    if (engine == nullptr) {
        SELECTION_HILOGE("engine is nullptr.");
        return;
    }

    if (Register() != EXCEPTION_SUCCESS) {
        SELECTION_HILOGE("Failed to register lister to service!");
        return;
    }

    engine->RegisterListener(type, std::forward<callbackTypePara>(cb));
}
void EtsSelectionEngineSetting::UnSubscribe(const std::string &type, const callbackTypePara &cb)
{
    SELECTION_HILOGI("EtsSelectionEngineSetting::Subscribe type: %{public}s.", type.c_str());
    auto engine = EtsSelectionEngineSetting::GetEtsSelectionEngineSetting();
    if (engine == nullptr) {
        SELECTION_HILOGE("engine is nullptr.");
        return;
    }

    engine->UnRegisterListener(type, cb);
}

void EtsSelectionEngineSetting::UnSubscribe(const std::string &type)
{
    SELECTION_HILOGI("EtsSelectionEngineSetting::Subscribe type: %{public}s.", type.c_str());
    auto engine = EtsSelectionEngineSetting::GetEtsSelectionEngineSetting();
    if (engine == nullptr) {
        SELECTION_HILOGE("engine is nullptr.");
        return;
    }

    engine->UnRegisterListener(type);
}

SFErrorCode EtsSelectionEngineSetting::Register()
{
    auto delegate = GetEtsSelectionEngineSetting();
    if (delegate == nullptr) {
        SELECTION_HILOGE("failed to get delegate!");
        return EXCEPTION_SELECTION_SERVICE;
    }
    
    return RegisterListenerToService(delegate);
}

std::shared_ptr<EtsSelectionEngineSetting> EtsSelectionEngineSetting::GetEtsSelectionEngineSetting()
{
    if (selectionDelegate_ == nullptr) {
        std::lock_guard<std::mutex> lock(selectionMutex_);
        if (selectionDelegate_ == nullptr) {
            std::shared_ptr<EtsSelectionEngineSetting> delegate(new EtsSelectionEngineSetting());
            if (delegate == nullptr) {
                SELECTION_HILOGE("EtsSelectionEngineSetting is nullptr!");
                return nullptr;
            }
            selectionDelegate_ = delegate;
        }
    }
    return selectionDelegate_;
}

SFErrorCode EtsSelectionEngineSetting::RegisterListenerToService(std::shared_ptr<EtsSelectionEngineSetting> &selectionEnging)
{
    auto proxy = SelectionSystemAbilityUtils::GetSelectionSystemAbility();
    if (proxy == nullptr) {
        SELECTION_HILOGE("selection system ability is nullptr!");
        return EXCEPTION_SELECTION_SERVICE;
    }
    listenerStub_ = new (std::nothrow) SelectionListenerImpl(selectionEnging);
    if (listenerStub_ == nullptr) {
        SELECTION_HILOGE("Failed to create SelectionListenerImpl instance.");
        return EXCEPTION_SELECTION_SERVICE;
    }
    SELECTION_HILOGI("Begin calling SA RegisterListener!");
    if (proxy->RegisterListener(listenerStub_) != ERR_OK) {
        return EXCEPTION_SELECTION_SERVICE;
    }

    return EXCEPTION_SUCCESS;
}

void EtsSelectionEngineSetting::RegisterListener(const std::string &type, callbackTypePara &&cb)
{
    SELECTION_HILOGI("RegisterListener %{public}s", type.c_str());
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (EtsCbMap_.empty() || EtsCbMap_.find(type) == EtsCbMap_.end()) {
        SELECTION_HILOGI("methodName %{public}s is not registered!", type.c_str());
    }
    auto callbacks = EtsCbMap_[type];
    bool ret = std::any_of(callbacks.begin(), callbacks.end(), [&cb](const auto &callback) {
        return callback == cb;
    });
    if (ret) {
        SELECTION_HILOGI("EtsSelectionEngineSetting callback already registered!");
        return;
    }

    SELECTION_HILOGI("add %{public}s callbackObj into EtsCbMap_.", type.c_str());
    EtsCbMap_[type].push_back(std::move(cb));
}
void EtsSelectionEngineSetting::UnRegisterListener(const std::string &type, const callbackTypePara &cb)
{
    SELECTION_HILOGI("unregister listener: %{public}s.", type.c_str());
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (EtsCbMap_.empty() || EtsCbMap_.find(type) == EtsCbMap_.end()) {
        SELECTION_HILOGE("methodName %{public}s is not unregistered!", type.c_str());
        return;
    }

    for (auto item = EtsCbMap_[type].begin(); item != EtsCbMap_[type].end(); item++) {
        if ((cb == *item)) {
            EtsCbMap_[type].erase(item);
            break;
        }
    }
    if (EtsCbMap_[type].empty()) {
        EtsCbMap_.erase(type);
    }

    auto proxy = SelectionSystemAbilityUtils::GetSelectionSystemAbility();
    if (proxy == nullptr || listenerStub_ == nullptr) {
        SELECTION_HILOGE("selection system ability or listenerStub_ is nullptr!");
        return;
    }
    proxy->UnregisterListener(listenerStub_);
    listenerStub_ = nullptr;
}

void EtsSelectionEngineSetting::UnRegisterListener(const std::string &type)
{
    SELECTION_HILOGI("unregister listener: %{public}s.", type.c_str());
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (EtsCbMap_.empty() || EtsCbMap_.find(type) == EtsCbMap_.end()) {
        SELECTION_HILOGE("methodName %{public}s is not unregistered!", type.c_str());
        return;
    }

    EtsCbMap_.erase(type);
    SELECTION_HILOGE("callback is nullptr!");
    return;
}

} // namespace SelectionFwk
} // namespace OHOS
