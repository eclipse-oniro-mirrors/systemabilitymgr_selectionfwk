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

#include "selection_client.h"

#include "iselection_service.h"
#include "iservice_registry.h"
#include "selection_log.h"
#include "system_ability_definition.h"

using namespace OHOS;
using namespace OHOS::SelectionFwk;

SelectionClient& SelectionClient::GetInstance()
{
    static SelectionClient instance;
    return instance;
}

bool SelectionClient::IsCurrentSelectionApp(int pid)
{
    SELECTION_HILOGI("SelectionClient::IsCurrentSelectionApp");
    bool result = false;
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        SELECTION_HILOGE("system ability manager is nullptr!");
        return result;
    }
    sptr<IRemoteObject> systemAbility = nullptr;
    systemAbility = systemAbilityManager->GetSystemAbility(SELECTION_FWK_SA_ID);
    if (systemAbility == nullptr) {
        SELECTION_HILOGE("get system ability is nullptr!");
        return result;
    }
    auto abilityManager = iface_cast<ISelectionService>(systemAbility);
    if (abilityManager == nullptr) {
        SELECTION_HILOGE("abilityManager is nullptr!");
        return result;
    }
    ErrCode errCode = abilityManager->IsCurrentSelectionApp(pid, result);
    if (errCode != 0) {
        SELECTION_HILOGE("Failed to call IsCurrentSelectionApp, errCode: %{public}d.", errCode);
    }
    return result;
}

int32_t SelectionClient::GetSelectionContent(std::string& selectionContent)
{
    SELECTION_HILOGI("SelectionClient::GetSelectionContent");
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        SELECTION_HILOGE("system ability manager is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    sptr<IRemoteObject> systemAbility = nullptr;
    systemAbility = systemAbilityManager->GetSystemAbility(SELECTION_FWK_SA_ID);
    if (systemAbility == nullptr) {
        SELECTION_HILOGE("get system ability is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto abilityManager = iface_cast<ISelectionService>(systemAbility);
    if (abilityManager == nullptr) {
        SELECTION_HILOGE("abilityManager is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto ret = abilityManager->GetSelectionContent(selectionContent);
    if (ret != ErrorCode::NO_ERROR) {
        SELECTION_HILOGE("SelectionClient::GetSelectionContent GetSelectionContent failed, ret = %{public}d", ret);
    }
    return ret;
}
