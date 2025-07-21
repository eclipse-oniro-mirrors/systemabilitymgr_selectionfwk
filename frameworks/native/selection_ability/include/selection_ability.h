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

#ifndef FRAMEWORKS_SELECTION_ABILITY_INCLUDE_SELECTION_ABILITY_H
#define FRAMEWORKS_SELECTION_ABILITY_INCLUDE_SELECTION_ABILITY_H

#include <thread>
#include <mutex>
#include <memory>
#include "refbase.h"
#include "context.h"
#include "concurrent_map.h"
#include "selection_panel.h"
#include "panel_info.h"

namespace OHOS {
namespace SelectionFwk {
class SelectionAbility : public RefBase {
public:
    SelectionAbility();
    ~SelectionAbility();
    static sptr<SelectionAbility> GetInstance();
    int32_t CreatePanel(const std::shared_ptr<AbilityRuntime::Context> &context, const PanelInfo &panelInfo,
        std::shared_ptr<SelectionPanel> &selectionPanel);
    int32_t DestroyPanel(const std::shared_ptr<SelectionPanel> &selectionPanel);
    int32_t ShowPanel(const std::shared_ptr<SelectionPanel> &selectionPanel);
    int32_t HidePanel(const std::shared_ptr<SelectionPanel> &selectionPanel);

private:
    static std::mutex instanceLock_;
    static sptr<SelectionAbility> instance_;

    ConcurrentMap<PanelType, std::shared_ptr<SelectionPanel>> panels_ {};
    std::atomic_bool isShowAfterCreate_ { false };
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // FRAMEWORKS_SELECTION_ABILITY_INCLUDE_SELECTION_ABILITY_H