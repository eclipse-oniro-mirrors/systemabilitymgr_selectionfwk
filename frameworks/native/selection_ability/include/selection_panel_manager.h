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


#ifndef SELECTION_PANEL_MANAGER_H
#define SELECTION_PANEL_MANAGER_H
#include <unordered_map>
#include <memory>
#include <atomic>
#include <stdexcept>
#include <mutex>

#include "selection_panel.h"

namespace OHOS {
namespace SelectionFwk {
class SelectionPanelManager {
public:
    static SelectionPanelManager& GetInstance();

    ~SelectionPanelManager() = default;
    void AddSelectionPanel(uint32_t id, std::shared_ptr<SelectionPanel> &obj);
    std::shared_ptr<SelectionPanel> GetSelectionPanel(uint32_t id) const;
    bool FindWindowID(uint32_t id) const;
    void Dispose();

private:
    SelectionPanelManager() = default;

    SelectionPanelManager(const SelectionPanelManager& other) = delete;
    SelectionPanelManager& operator=(const SelectionPanelManager& other) = delete;

private:
    std::unordered_map<uint32_t, std::shared_ptr<SelectionPanel>> storage_;
    mutable std::mutex mutex_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTION_PANEL_MANAGER_H
