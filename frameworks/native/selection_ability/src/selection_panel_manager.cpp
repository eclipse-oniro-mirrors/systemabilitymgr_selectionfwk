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

#include "selection_panel_manager.h"
#include "selection_ability.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

SelectionPanelManager& SelectionPanelManager::GetInstance()
{
    static SelectionPanelManager instance;
    return instance;
}

void SelectionPanelManager::AddSelectionPanel(uint32_t id, std::shared_ptr<SelectionPanel> &obj)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (obj == nullptr) {
        SELECTION_HILOGE("selectionPanel is nullptr");
        return;
    }
    storage_[id] = obj;
}

std::shared_ptr<SelectionPanel> SelectionPanelManager::GetSelectionPanel(uint32_t id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = storage_.find(id);
    return (it != storage_.end()) ? it->second : nullptr;
}

bool SelectionPanelManager::FindWindowID(uint32_t id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return storage_.find(id) != storage_.end();
}

void SelectionPanelManager::Dispose()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto iter = storage_.begin(); iter != storage_.end();) {
        auto selectionPanel = iter->second;
        if (selectionPanel == nullptr) {
            SELECTION_HILOGE("selectionPanel is nullptr");
            ++iter;
            continue;
        }
        if (selectionPanel->GetPanelType() == PanelType::MENU_PANEL) {
            selectionPanel->HidePanel();
        } else if (selectionPanel->GetPanelType() == PanelType::MAIN_PANEL) {
            SelectionAbility::GetInstance()->DestroyPanel(selectionPanel);
            iter = storage_.erase(iter);
            continue;
        }
        ++iter;
    }
}
} // namespace SelectionFwk
} // namespace OHOS