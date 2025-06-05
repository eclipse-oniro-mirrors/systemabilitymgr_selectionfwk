/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "db_selection_config_repository.h"

#include <string>

#include "selection_errors.h"
#include "selection_config_database.h"
#include "selection_log.h"

using namespace OHOS::NativeRdb;

namespace OHOS {
namespace SelectionFwk {
std::shared_ptr<DbSelectionConfigRepository> DbSelectionConfigRepository::instance_ = nullptr;

DbSelectionConfigRepository::DbSelectionConfigRepository()
{
    selectionDatabase_ = SelectionConfigDataBase::GetInstance();
}

std::shared_ptr<DbSelectionConfigRepository> DbSelectionConfigRepository::GetInstance()
{
    static std::mutex instanceMutex;
    std::lock_guard<std::mutex> guard(instanceMutex);
    if (instance_ == nullptr) {
        SELECTION_HILOGI("reset to new DbSelectionConfigRepository instance");
        instance_.reset(new DbSelectionConfigRepository());
    }
    return instance_;
}

int DbSelectionConfigRepository::Save(int uid, const SelectionConfig &info)
{
    if (selectionDatabase_ == nullptr) {
        SELECTION_HILOGE("selectionDatabase_ is null");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    std::lock_guard<std::mutex> guard(databaseMutex_);
    ValuesBucket values;
    values.Clear();
    values.PutInt("uid", uid);
    values.PutInt("enable", info.IsEnabled());
    values.PutInt("trigger", info.IsTriggered());
    values.PutString("bundleName", info.GetBundleName());
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
    SELECTION_HILOGI("add success: uid=%{public}d enable=%{public}d trigger=%{public}d bundleName=%{public}s",
        uid, info.IsEnabled(), info.IsTriggered(), info.GetBundleName().c_str());
    return ret;
}

std::optional<SelectionConfig> DbSelectionConfigRepository::GetOneByUserId(int uid)
{
    SelectionConfig info;
    std::lock_guard<std::mutex> guard(databaseMutex_);
    std::vector<std::string> columns;
    RdbPredicates rdbPredicates(SELECTION_CONFIG_TABLE_NAME);
    rdbPredicates.EqualTo("uid", std::to_string(uid));
    if (GetConfigFromDatabase(rdbPredicates, columns, info) != SELECTION_CONFIG_OK) {
        return std::nullopt;
    }
    SELECTION_HILOGI("uid=%{public}d enable=%{public}d trigger=%{public}d bundleName=%{public}s",
        uid, info.IsEnabled(), info.IsTriggered(), info.GetBundleName().c_str());
    return info;
}

int DbSelectionConfigRepository::GetConfigFromDatabase(const RdbPredicates &rdbPredicates,
    const std::vector<std::string> &columns, SelectionConfig &info)
{
    if (selectionDatabase_ == nullptr) {
        SELECTION_HILOGE("rightDatabase_ is null");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int ret = selectionDatabase_->BeginTransaction();
    if (ret < SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("BeginTransaction error: %{public}d", ret);
        return ret;
    }
    auto resultSet = selectionDatabase_->Query(rdbPredicates, columns);
    if (resultSet == nullptr) {
        SELECTION_HILOGE("Query error");
        (void)selectionDatabase_->RollBack();
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    ret = selectionDatabase_->Commit();
    if (ret < SELECTION_CONFIG_OK) {
        SELECTION_HILOGE("Commit error: %{public}d", ret);
        (void)selectionDatabase_->RollBack();
        return ret;
    }
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    if (rowCount == 0) {
        SELECTION_HILOGI("Can not found uid in selection_config table");
        return SELECTION_CONFIG_NOT_FOUND;
    }
    return ProcessQueryResult(resultSet, info);
}

int DbSelectionConfigRepository::ProcessQueryResult(const std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet,
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
    std::string bundleName = "";
    int trigger = 0;
    int uid = -1;

    for (int32_t i = 0; i < table.rowCount && !endFlag; i++, resultSet->IsEnded(endFlag)) {
        if (resultSet->GoToRow(i) != E_OK) {
            SELECTION_HILOGE("GoToRow %{public}d", i);
            return -1;
        }
        if (resultSet->GetInt(table.enableIndex, enable) == E_OK &&
            resultSet->GetString(table.bundleNameIndex, bundleName) == E_OK &&
            resultSet->GetInt(table.triggerIndex, trigger) == E_OK &&
            resultSet->GetInt(table.uidIndex, uid) == E_OK) {
            info.SetEnabled(enable);
            info.SetTriggered(trigger);
            info.SetBundleName(bundleName);
            info.SetUid(uid);
        }
        SELECTION_HILOGI("enable=%{public}d trigger=%{public}d bundleName=%{public}s uid=%{public}d",
            enable, trigger, bundleName.c_str(), uid);
    }

    int position = 0;
    resultSet->GetRowIndex(position);
    resultSet->IsEnded(endFlag);
    SELECTION_HILOGI("row=%{public}d col=%{public}d pos=%{public}d end=%{public}s",
        table.rowCount, table.columnCount, position, (endFlag ? "yes" : "no"));
    return 0;
}

int DbSelectionConfigRepository::RetrieveResultSetMetadata(
    const std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet, struct SelectionConfigTableInfo &table)
{
    if (resultSet == nullptr) {
        SELECTION_HILOGE("resultSet is null");
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    int32_t rowCount = 0;
    int32_t columnCount = 0;
    std::vector<std::string> columnNames;
    if (resultSet->GetRowCount(rowCount) != E_OK || resultSet->GetColumnCount(columnCount) != E_OK ||
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
            table.bundleNameIndex = i;
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
        rowCount, columnCount, table.primaryKeyIndex, table.uidIndex, table.enableIndex, table.bundleNameIndex,
        table.triggerIndex, table.shortcutKeysIndex);
    return SELECTION_CONFIG_OK;
}
} // namespace SelectionFwk
} // namespace OHOS