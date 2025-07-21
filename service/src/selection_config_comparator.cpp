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

#include <sstream>
#include "selection_config_comparator.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
static const std::string DEFAULT_SELECTION_APP = "com.selection.selectionapplication/SelectionExtensionAbility";

std::string ComparisionResult::ToString() const
{
    std::ostringstream oss;
    oss << "ComparisionResult: shouldCreate: " << shouldCreate
        << " shouldStop: " << shouldStop
        << " direction: " << static_cast<int>(direction)
        << " shouldRestartApp: " << shouldRestartApp
        << " selectionConfig: [ " << selectionConfig.ToString() << "]";
    return oss.str();
}

ComparisionResult SelectionConfigComparator::Compare(int uid, const SelectionConfig &sysSelectionConfig,
    std::optional<SelectionConfig> &dbSelectionConfig)
{
    if (dbSelectionConfig.has_value()) {
        SELECTION_HILOGI("dbSelectionConfig has value");
    } else {
        SELECTION_HILOGI("dbSelectionConfig is nullopt!");
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
        result.selectionConfig.SetUid(uid);
        result.selectionConfig.SetEnabled(true);
        result.selectionConfig.SetTriggered(false);
        result.selectionConfig.SetApplicationInfo(DEFAULT_SELECTION_APP);
        return result;
    }

    if (dbSelectionConfig.value().GetEnable()) {
        if (sysSelectionConfig.GetUid() != uid) {
            result.direction = FromDbToSys;
            result.selectionConfig = dbSelectionConfig.value();
            auto appInfoFromDB = dbSelectionConfig.value().GetApplicationInfo();
            if (appInfoFromDB == sysSelectionConfig.GetApplicationInfo()) {
                result.shouldRestartApp = true;
            }
            return result;
        }
        result.direction = FromSysToDb;
        result.selectionConfig = sysSelectionConfig;
        if (!sysSelectionConfig.GetEnable()) {
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
    if (!sysSelectionConfig.GetEnable()) {
        result.shouldStop = true;
    }

    return result;
}
} // namespace SelectionFwk
} // namespace OHOS