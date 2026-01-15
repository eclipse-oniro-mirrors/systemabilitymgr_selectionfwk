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

#include "selectioninputlistener_fuzzer.h"
#include "system_ability_definition.h"
#include "selection_data_inner.h"
#include "iselection_listener.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "selection_log.h"
#include "iservice_registry.h"
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

void TestOnSelectionChange(std::string fuzzedString, int32_t fuzzedInt32, uint32_t fuzzedUInt32)
{
    auto remote = GetSelectionSystemAbility();
    auto listener = iface_cast<ISelectionListener>(remote);

    if (listener == nullptr) {
        SELECTION_HILOGE("get listener is null");
        return;
    }

    SelectionInfoData dataInner;
    dataInner.data.selectionType = MOVE_SELECTION;
    dataInner.data.startDisplayX = fuzzedInt32;
    dataInner.data.startDisplayY = fuzzedInt32;
    dataInner.data.endDisplayX = fuzzedInt32;
    dataInner.data.endDisplayY = fuzzedInt32;
    dataInner.data.startWindowX = fuzzedInt32;
    dataInner.data.startWindowY = fuzzedInt32;
    dataInner.data.endWindowX = fuzzedInt32;
    dataInner.data.endWindowY = fuzzedInt32;
    dataInner.data.displayId = fuzzedUInt32;
    dataInner.data.windowId = fuzzedUInt32;

    listener->OnSelectionChange(dataInner);
}

void TestFocusChange(uint32_t fuzzedUInt32)
{
    auto remote = GetSelectionSystemAbility();
    auto listener = iface_cast<ISelectionListener>(remote);

    if (listener == nullptr) {
        SELECTION_HILOGE("get listener is null");
        return;
    }
    SelectionFocusChangeInfo selectionFocusChangeInfo;
    selectionFocusChangeInfo.windowId_ = fuzzedUInt32;
    selectionFocusChangeInfo.windowType_ = fuzzedUInt32;

    listener->FocusChange(selectionFocusChangeInfo);
}
} // namespace SelectionFwk
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(uint32_t)) {
        return 0;
    }

    std::string fuzzedString(reinterpret_cast<const char *>(data), size);
    auto fuzzedUInt32 = static_cast<uint32_t>(size);
    auto fuzzedInt32 = static_cast<int32_t>(size);

    OHOS::SelectionFwk::TestOnSelectionChange(fuzzedString, fuzzedInt32, fuzzedUInt32);
    OHOS::SelectionFwk::TestFocusChange(fuzzedUInt32);

    return 0;
}