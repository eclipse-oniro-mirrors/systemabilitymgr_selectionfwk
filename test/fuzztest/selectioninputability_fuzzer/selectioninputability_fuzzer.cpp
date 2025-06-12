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

#include "selectioninputability_fuzzer.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "selection_log.h"
#include <vector>
#include <cstdint>
#include <iostream>

namespace OHOS {
namespace SelectionFwk {

sptr<IRemoteObject> GetSelectionSystemAbility()
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        SELECTION_HILOGE("system ability manager is nullptr!");
        return nullptr;
    }

    sptr<IRemoteObject> systemAbility = nullptr;
    systemAbility = systemAbilityManager->GetSystemAbility(SELECTION_FWK_SA_ID);
    if (systemAbility == nullptr) {
        SELECTION_HILOGE("get system ability is nullptr!");
        return nullptr;
    }

    return systemAbility;
}

void TestSendRequest(uint32_t code)
{
    sptr<IRemoteObject> systemAbility = GetSelectionSystemAbility();
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    systemAbility->SendRequest(code, data, reply, option);
}

} // namespace SelectionFwk
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(uint32_t)) {
        return 0;
    }

    auto fuzzedCode = static_cast<uint32_t>(size);

    OHOS::SelectionFwk::TestSendRequest(fuzzedCode);
    return 0;
}