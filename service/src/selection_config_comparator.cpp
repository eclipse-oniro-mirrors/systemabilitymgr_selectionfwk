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

#include "selection_config_comparator.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

std::string ComparisionResult::ToString() const {
    std::string result = "";
    result = "ComparisionResult: shouldCreate: " + std::to_string(shouldCreate) + " shouldStop: " +
        std::to_string(shouldStop) + " direction: " + std::to_string(static_cast<int>(direction)) +
        " selectionConfig: [ " + selectionConfig.ToString() + "]";
    return result;
}

ComparisionResult SelectionConfigComparator::Compare(int uid, const SelectionConfig &sysSelectionConfig,
    std::optional<SelectionConfig> &dbSelectionConfig)
{
    SELECTION_HILOGI("sysSelectionConfig: %{public}s", sysSelectionConfig.ToString().c_str());
    if (dbSelectionConfig.has_value()) {
        SELECTION_HILOGI("dbSelectionConfig: %{public}s", dbSelectionConfig->ToString().c_str());
    } else {
        SELECTION_HILOGE("dbSelectionConfig is nullopt!");
    }
    auto result = DoCompare(uid, sysSelectionConfig, dbSelectionConfig);
    SELECTION_HILOGI("result: %{public}s", result.ToString().c_str());
    return result;
}
ComparisionResult SelectionConfigComparator::DoCompare(int uid, const SelectionConfig &sysSelectionConfig,
    std::optional<SelectionConfig> &dbSelectionConfig)
{
    ComparisionResult result;
    if (!dbSelectionConfig.has_value()) {
        result.shouldCreate  = true;
        result.selectionConfig = sysSelectionConfig;
        result.selectionConfig.SetUid(uid);
        return result;
    }

    if (dbSelectionConfig.value().IsEnabled()) {
        if (sysSelectionConfig.GetUid() != uid) {
            result.direction = FromDbToSys;
            result.selectionConfig = dbSelectionConfig.value();
            return result;
        }
        result.direction = FromSysToDb;
        result.selectionConfig = sysSelectionConfig;
        if (!sysSelectionConfig.IsEnabled()) {
            result.shouldStop = true;
        }
        return result;
    }

    if (sysSelectionConfig.GetUid() != uid) {
        result.selectionConfig = dbSelectionConfig.value();
        result.shouldStop = true;
        return result;
    }

    result.direction = FromSysToDb;
    result.selectionConfig = sysSelectionConfig;
    if (!sysSelectionConfig.IsEnabled()) {
        result.shouldStop = true;
    }

    return result;
}
} // namespace SelectionFwk
} // namespace OHOS