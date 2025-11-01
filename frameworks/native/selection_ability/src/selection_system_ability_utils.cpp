/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "selection_system_ability_utils.h"
#include "iservice_registry.h"
#include "selection_log.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace SelectionFwk {

bool SelectionSystemAbilityUtils::IsSelectionSystemAbilityExistent()
{
    sptr<ISelectionService> abilityManager = GetSelectionSystemAbility();
    return abilityManager != nullptr;
}

sptr<ISelectionService> SelectionSystemAbilityUtils::GetSelectionSystemAbility()
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        SELECTION_HILOGE("system ability manager is nullptr!");
        return nullptr;
    }

    sptr<IRemoteObject> systemAbility = nullptr;
    systemAbility = systemAbilityManager->CheckSystemAbility(SELECTION_FWK_SA_ID);
    if (systemAbility == nullptr) {
        SELECTION_HILOGE("get system ability is nullptr!");
        return nullptr;
    }

    sptr<ISelectionService> abilityManager = iface_cast<ISelectionService>(systemAbility);
    return abilityManager;
}

}
}
