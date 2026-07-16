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

#include "database_plugin_impl.h"
#include "selection_config_database.h"
#include "selection_errors.h"
#include "selection_log.h"

using namespace OHOS::NativeRdb;

namespace OHOS {
namespace SelectionFwk {

bool DatabasePluginImpl::Initialize()
{
    SELECTION_HILOGI("DatabasePluginImpl::Initialize called");
    selectionDatabase_ = SelectionConfigDataBase::GetInstance();
    bool success = selectionDatabase_ != nullptr;
    SELECTION_HILOGI("DatabasePluginImpl::Initialize %{public}s", success ? "succeeded" : "failed");
    return success;
}

void DatabasePluginImpl::Cleanup()
{
    SELECTION_HILOGI("DatabasePluginImpl::Cleanup called");
    std::lock_guard<std::mutex> guard(databaseMutex_);
    selectionDatabase_.reset();
    SELECTION_HILOGI("DatabasePluginImpl::Cleanup completed");
}

const char* DatabasePluginImpl::GetModuleName()
{
    return "Database";
}

int DatabasePluginImpl::GetModuleVersion()
{
    return 1;
}

int DatabasePluginImpl::Save(int uid, const SelectionConfig &info)
{
    SELECTION_HILOGI("DatabasePluginImpl::Save called, uid=%{public}d", uid);

    std::lock_guard<std::mutex> guard(databaseMutex_);
    if (selectionDatabase_ == nullptr) {
        SELECTION_HILOGE("Database not initialized");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }

    ValuesBucket values;
    values.Clear();
    values.PutInt("uid", uid);
    values.PutInt("enable", info.GetEnable());
    values.PutInt("trigger", info.GetTriggered());
    values.PutString("bundleName", info.GetApplicationInfo());

    int ret = selectionDatabase_->BeginTransaction();
    if (ret < SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("BeginTransaction error: %{public}d", ret);
        return ret;
    }

    int changedRows = 0;
    RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo("uid", std::to_string(uid));

    ret = selectionDatabase_->Update(changedRows, values, predicates);
    if (ret != SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("Update error: %{public}d", ret);
        (void)selectionDatabase_->RollBack();
        return ret;
    }

    if (changedRows == 0) {
        ret = selectionDatabase_->Insert(values);
        if (ret < SELECTION_CONFIG_OK) {
            SELECTION_HILOGE("Insert error: %{public}d", ret);
            (void)selectionDatabase_->RollBack();
            return ret;
        }
    }

    ret = selectionDatabase_->Commit();
    if (ret < SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("Commit error: %{public}d", ret);
        (void)selectionDatabase_->RollBack();
        return ret;
    }

    SELECTION_HILOGI("Save success: enable=%{public}d trigger=%{public}d app=%{public}s",
        info.GetEnable(), info.GetTriggered(), info.GetApplicationInfo().c_str());
    return ret;
}

std::optional<SelectionConfig> DatabasePluginImpl::GetOneByUserId(int uid)
{
    SELECTION_HILOGI("DatabasePluginImpl::GetOneByUserId called, uid=%{public}d", uid);

    SelectionConfig info;
    std::lock_guard<std::mutex> guard(databaseMutex_);
    std::vector<std::string> columns;
    RdbPredicates rdbPredicates(SELECTION_CONFIG_TABLE_NAME);
    rdbPredicates.EqualTo("uid", std::to_string(uid));

    if (selectionDatabase_ == nullptr) {
        SELECTION_HILOGE("Database not initialized");
        return std::nullopt;
    }

    int ret = selectionDatabase_->BeginTransaction();
    if (ret < SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("BeginTransaction error: %{public}d", ret);
        return std::nullopt;
    }

    auto resultSet = selectionDatabase_->Query(rdbPredicates, columns);
    if (resultSet == nullptr) {
        SELECTION_HILOGE("Query error");
        (void)selectionDatabase_->RollBack();
        return std::nullopt;
    }

    ret = selectionDatabase_->Commit();
    if (ret < SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("Commit error: %{public}d", ret);
        (void)selectionDatabase_->RollBack();
        return std::nullopt;
    }

    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    if (rowCount == 0) {
        SELECTION_HILOGI("Cannot find uid in selection_config table");
        return std::nullopt;
    }

    ret = ProcessQueryResult(resultSet, info);
    if (ret != 0) {
        SELECTION_HILOGE("ProcessQueryResult error: %{public}d", ret);
        return std::nullopt;
    }

    SELECTION_HILOGI("GetOneByUserId success: enable=%{public}d trigger=%{public}d app=%{public}s",
        info.GetEnable(), info.GetTriggered(), info.GetApplicationInfo().c_str());
    return info;
}

bool DatabasePluginImpl::IsAvailable() const
{
    std::lock_guard<std::mutex> guard(databaseMutex_);
    return selectionDatabase_ != nullptr;
}

bool DatabasePluginImpl::HealthCheck() const
{
    // 简单健康检查：数据库指针非空
    return selectionDatabase_ != nullptr;
}

const char* DatabasePluginImpl::GetStatus() const
{
    if (selectionDatabase_ == nullptr) {
        return "uninitialized";
    }
    return "available";
}

int DatabasePluginImpl::ProcessQueryResult(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    SelectionConfig &info)
{
    struct SelectionConfigTableInfo table;
    int ret = RetrieveResultSetMetadata(resultSet, table);
    if (ret < SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("GetResultSetTableInfo failed");
        return ret;
    }

    bool endFlag = false;
    int enable = 0;
    std::string applicationInfo = "";
    int trigger = 0;
    int uid = -1;

    for (int32_t i = 0; i < table.rowCount && !endFlag; i++, resultSet->IsEnded(endFlag)) {
        if (resultSet->GoToRow(i) != E_OK) {
            SELECTION_HILOGE("GoToRow %{public}d", i);
            return -1;
        }
        if (resultSet->GetInt(table.enableIndex, enable) == E_OK &&
            resultSet->GetString(table.applicationInfoIndex, applicationInfo) == E_OK &&
            resultSet->GetInt(table.triggerIndex, trigger) == E_OK &&
            resultSet->GetInt(table.uidIndex, uid) == E_OK) {
            info.SetEnabled(enable == 1 ? true : false);
            info.SetTriggered(trigger == 1 ? true : false);
            info.SetApplicationInfo(applicationInfo);
            info.SetUid(uid);
        }
        SELECTION_HILOGI("enable=%{public}d trigger=%{public}d applicationInfo=%{public}s",
            enable, trigger, applicationInfo.c_str());
    }

    return 0;
}

int DatabasePluginImpl::RetrieveResultSetMetadata(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    struct SelectionConfigTableInfo &table)
{
    if (resultSet == nullptr) {
        SELECTION_HILOGE("resultSet is null");
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }

    int32_t rowCount = 0;
    int32_t columnCount = 0;
    std::vector<std::string> columnNames;
    if (resultSet->GetRowCount(rowCount) != E_OK ||
        resultSet->GetColumnCount(columnCount) != E_OK ||
        resultSet->GetAllColumnNames(columnNames) != E_OK) {
        SELECTION_HILOGE("get table info failed");
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }

    int32_t columnNamesCount = static_cast<int32_t>(columnNames.size());
    for (int32_t i = 0; i < columnNamesCount; i++) {
        std::string &columnName = columnNames.at(i);
        if (columnName == "id") {
            table.primaryKeyIndex = i;
        }
        if (columnName == "uid") {
            table.uidIndex = i;
        }
        if (columnName == "enable") {
            table.enableIndex = i;
        }
        if (columnName == "bundleName") {
            table.applicationInfoIndex = i;
        }
        if (columnName == "trigger") {
            table.triggerIndex = i;
        }
        if (columnName == "shortcutKeys") {
            table.shortcutKeysIndex = i;
        }
    }

    table.rowCount = rowCount;
    table.columnCount = columnCount;
    SELECTION_HILOGI("info[%{public}d/%{public}d]: %{public}d/%{public}d/%{public}d/%{public}d/%{public}d/%{public}d",
        rowCount, columnCount, table.primaryKeyIndex, table.uidIndex, table.enableIndex,
        table.applicationInfoIndex, table.triggerIndex, table.shortcutKeysIndex);
    return SELECTION_CONFIG_OK;
}

} // namespace SelectionFwk
} // namespace OHOS

