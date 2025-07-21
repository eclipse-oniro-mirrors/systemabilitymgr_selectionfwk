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

#include "system_ability_status_change_listener.h"
#include "selection_log.h"

namespace OHOS::SelectionFwk {

SystemAbilityStatusChangeListener::SystemAbilityStatusChangeListener(
    std::function<void(int32_t, const std::string &)> func)
    : func_(std::move(func))
{
}

void SystemAbilityStatusChangeListener::OnAddSystemAbility(int32_t systemAbilityId,
    const std::string &deviceId)
{
    SELECTION_HILOGI("SystemAbility [%{public}d] is added on [%{public}s]!", systemAbilityId, deviceId.c_str());
    if (func_ == nullptr) {
        return;
    }
    func_(systemAbilityId, deviceId);
}

void SystemAbilityStatusChangeListener::OnRemoveSystemAbility(int32_t systemAbilityId,
    const std::string &deviceId)
{
    SELECTION_HILOGW("SystemAbility [%{public}d] is removed on [%{public}s]!", systemAbilityId, deviceId.c_str());
}

}
