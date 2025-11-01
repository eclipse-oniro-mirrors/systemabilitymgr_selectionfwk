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

#ifndef SELECTION_CONFIG_COMPARATOR_CPP
#define SELECTION_CONFIG_COMPARATOR_CPP

#include <optional>
#include "selection_config.h"
#include "selection_common.h"

namespace OHOS {
namespace SelectionFwk {
enum SyncDirection {
    NONE,
    FromDbToSys,
    FromSysToDb
};

struct ComparisionResult {
    SyncDirection direction = NONE;
    bool shouldCreate = false;
    bool shouldStop = false;
    bool shouldStart = false;
    bool shouldRestartApp = false;
    SelectionConfig selectionConfig;

    std::string ToString() const;
};

class SelectionConfigComparator {
public:
    static SelectionConfigComparator& GetInstance();

    void Init();
    void Init(const SelectionConfig& defaultSelectionConfig);
    ComparisionResult Compare(int uid, const SelectionConfig &sysSelectionConfig,
                              std::optional<SelectionConfig> &dbSelectionConfig,
                              const std::optional<AbilityRuntimeInfo> &connectedAbilityInfo = std::nullopt);

private:
    SelectionConfigComparator() = default;

    ComparisionResult DoCompare(int uid, const SelectionConfig &sysSelectionConfig,
                                std::optional<SelectionConfig> &dbSelectionConfig,
                                const std::optional<AbilityRuntimeInfo> &connectedAbilityInfo = std::nullopt);

private:
    SelectionConfig defaultSelectionConfig_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTION_CONFIG_COMPARATOR_CPP