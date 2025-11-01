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
#include "db_selection_config_repository.h"
#include "selection_config_comparator.h"
#include "sys_selection_config_repository.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
static const std::string DEFAULT_SELECTION_APP = "com.selection.selectionapplication/SelectionExtensionAbility";
static const int DEFAULT_UID = -1;

std::string ComparisionResult::ToString() const
{
    std::ostringstream oss;
    oss << "ComparisionResult: shouldCreate: " << shouldCreate
        << " shouldStop: " << shouldStop
        << " shouldStart: " << shouldStart
        << " direction: " << static_cast<int>(direction)
        << " shouldRestartApp: " << shouldRestartApp
        << " selectionConfig: [ " << selectionConfig.ToString() << "]";
    return oss.str();
}

SelectionConfigComparator& SelectionConfigComparator::GetInstance()
{
    static SelectionConfigComparator instance;
    return instance;
}

void SelectionConfigComparator::Init()
{
    auto selectionConfig = DbSelectionConfigRepository::GetInstance()->GetOneByUserId(DEFAULT_UID);
    if (!selectionConfig.has_value()) {
        selectionConfig = SysSelectionConfigRepository::GetInstance()->GetSysParameters();
        DbSelectionConfigRepository::GetInstance()->Save(DEFAULT_UID, selectionConfig.value());
        SELECTION_HILOGI("There is no default selectionConfig in DB, initialize it using sys parameters %{public}s.",
            selectionConfig.value().ToString().c_str());
    }
    Init(selectionConfig.value());
    SELECTION_HILOGI("Initialized default selection config as %{public}s.", defaultSelectionConfig_.ToString().c_str());
}

void SelectionConfigComparator::Init(const SelectionConfig& defaultSelectionConfig)
{
    defaultSelectionConfig_ = defaultSelectionConfig;
}

ComparisionResult SelectionConfigComparator::Compare(int uid, const SelectionConfig &sysSelectionConfig,
    std::optional<SelectionConfig> &dbSelectionConfig, const std::optional<AbilityRuntimeInfo> &connectedAbilityInfo)
{
    if (dbSelectionConfig.has_value()) {
        SELECTION_HILOGI("dbSelectionConfig has value");
    } else {
        SELECTION_HILOGI("dbSelectionConfig is nullopt!");
    }
    auto result = DoCompare(uid, sysSelectionConfig, dbSelectionConfig, connectedAbilityInfo);
    SELECTION_HILOGI("result: %{public}s", result.ToString().c_str());
    return result;
}

ComparisionResult SelectionConfigComparator::DoCompare(int uid, const SelectionConfig &sysSelectionConfig,
    std::optional<SelectionConfig> &dbSelectionConfig, const std::optional<AbilityRuntimeInfo> &connectedAbilityInfo)
{
    ComparisionResult result;
    if (!dbSelectionConfig.has_value()) {
        result.shouldCreate  = true;
        bool isEnable = defaultSelectionConfig_.GetEnable();
        result.shouldStop = !isEnable;
        result.selectionConfig = defaultSelectionConfig_;
        result.selectionConfig.SetUid(uid);
        if (isEnable && sysSelectionConfig.GetApplicationInfo() == defaultSelectionConfig_.GetApplicationInfo()) {
            result.shouldRestartApp = true;
        }
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
        } else if (!connectedAbilityInfo.has_value()) {
            result.shouldStart = true;
        }
        return result;
    }

    if (sysSelectionConfig.GetUid() != uid) {
        result.selectionConfig = dbSelectionConfig.value();
        result.shouldStop = true;
        result.direction = FromDbToSys;
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