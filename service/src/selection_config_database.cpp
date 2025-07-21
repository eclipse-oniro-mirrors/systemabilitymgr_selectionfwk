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

#include "selection_config_database.h"

#include "selection_errors.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

std::shared_ptr<SelectionConfigDataBase> SelectionConfigDataBase::instance_ = nullptr;

SelectionConfigDataBase::SelectionConfigDataBase(const std::shared_ptr<OHOS::NativeRdb::RdbStore>& store)
    : store_(store)
{
}

std::shared_ptr<SelectionConfigDataBase> SelectionConfigDataBase::GetInstance()
{
    static std::mutex instanceMutex;
    std::lock_guard<std::mutex> guard(instanceMutex);
    if (instance_ == nullptr) {
        SELECTION_HILOGI("reset to new SelectionConfigDataBase instance");
        auto store = SelectionConfigDataBase::CreateStore();
        if (store != nullptr) {
            instance_.reset(new SelectionConfigDataBase(store));
        }
        return instance_;
    }
    return instance_;
}

int32_t SelectionConfigDataBase::BeginTransaction()
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("BeginTransaction store_ is nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int32_t ret = store_->BeginTransaction();
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("BeginTransaction fail :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

int32_t SelectionConfigDataBase::Commit()
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("Commit store_ is nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int32_t ret = store_->Commit();
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("Commit fail :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

int32_t SelectionConfigDataBase::RollBack()
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("RollBack store_ is nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int32_t ret = store_->RollBack();
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("RollBack fail :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

int64_t SelectionConfigDataBase::Insert(const OHOS::NativeRdb::ValuesBucket &insertValues)
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("Insert store_ is  nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int64_t outRowId = 0;
    int32_t ret = store_->Insert(outRowId, SELECTION_CONFIG_TABLE_NAME, insertValues);
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("Insert ret :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    SELECTION_HILOGI("Insert id=%{public}" PRIu64 "", outRowId);
    return outRowId;
}

int32_t SelectionConfigDataBase::Update(
    int32_t &changedRows, const OHOS::NativeRdb::ValuesBucket &values, const OHOS::NativeRdb::RdbPredicates &predicates)
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("Update(RdbPredicates) store_ is nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int32_t ret = store_->Update(changedRows, values, predicates);
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("Update(RdbPredicates) ret :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

int32_t SelectionConfigDataBase::Update(int32_t &changedRows, const OHOS::NativeRdb::ValuesBucket &values,
    const std::string &whereClause, const std::vector<std::string> &whereArgs)
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("Update(whereClause) store_ is nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int32_t ret = store_->Update(changedRows, SELECTION_CONFIG_TABLE_NAME, values, whereClause, whereArgs);
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("Update(whereClause) ret :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

int32_t SelectionConfigDataBase::Delete(const OHOS::NativeRdb::RdbPredicates &predicates)
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("Delete(RdbPredicates) store_ is  nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int32_t deleteRow = 0;
    int32_t ret = store_->Delete(deleteRow, predicates);
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("Delete(RdbPredicates) ret :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

int32_t SelectionConfigDataBase::Delete(
    int32_t &changedRows, const std::string &whereClause, const std::vector<std::string> &whereArgs)
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("Delete store_ is nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int32_t ret = store_->Delete(changedRows, SELECTION_CONFIG_TABLE_NAME, whereClause, whereArgs);
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("Delete(whereClause) ret :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

int32_t SelectionConfigDataBase::ExecuteSql(const std::string &sql,
    const std::vector<OHOS::NativeRdb::ValueObject> &bindArgs)
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("ExecuteSql store_ is nullptr");
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    int32_t ret = store_->ExecuteSql(sql, bindArgs);
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("ExecuteSql ret :%{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

std::shared_ptr<OHOS::NativeRdb::ResultSet> SelectionConfigDataBase::QuerySql(
    const std::string &sql, const std::vector<std::string> &selectionArgs)
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("QuerySql(sql) store_ is nullptr");
        return nullptr;
    }
    return store_->QuerySql(sql);
}

std::shared_ptr<OHOS::NativeRdb::ResultSet> SelectionConfigDataBase::Query(
    const OHOS::NativeRdb::AbsRdbPredicates &predicates, const std::vector<std::string> &columns)
{
    if (store_ == nullptr) {
        SELECTION_HILOGE("Query(AbsRdbPredicates) store_ is nullptr");
        return nullptr;
    }
    return store_->Query(predicates, columns);
}

std::shared_ptr<OHOS::NativeRdb::RdbStore> SelectionConfigDataBase::CreateStore()
{
    std::string selectionDatabaseName = SELECTION_CONFIG_DB_PATH + SELECTION_CONFIG_DB_NAME;
    int32_t errCode = OHOS::NativeRdb::E_OK;
    OHOS::NativeRdb::RdbStoreConfig config(selectionDatabaseName);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    SelectionConfigDataBaseCallBack sqliteOpenHelperCallback;
    auto store = OHOS::NativeRdb::RdbHelper::GetRdbStore(config, DATABASE_OPEN_VERSION,
        sqliteOpenHelperCallback, errCode);
    if (errCode != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("GetRdbStore errCode :%{public}d", errCode);
    } else {
        SELECTION_HILOGI("GetRdbStore success :%{public}d", errCode);
    }
    return store;
}

int32_t SelectionConfigDataBaseCallBack::OnCreate(OHOS::NativeRdb::RdbStore &store)
{
    std::string sql = CREATE_SELECTION_CONFIG_TABLE;
    int32_t ret = store.ExecuteSql(sql);
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("OnCreate failed: %{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    SELECTION_HILOGI("DB OnCreate Done: %{public}d", ret);
    return SELECTION_CONFIG_OK;
}

int32_t SelectionConfigDataBaseCallBack::OnUpgrade(OHOS::NativeRdb::RdbStore &store, int32_t oldVersion,
    int32_t newVersion)
{
    SELECTION_HILOGI("DB OnUpgrade Enter %{public}d => %{public}d", oldVersion, newVersion);
    if (oldVersion >= newVersion) {
        return SELECTION_CONFIG_OK;
    }

    std::string sql = SQL_ADD_TOKEN_ID;
    int32_t ret = store.ExecuteSql(sql);
    if (ret != OHOS::NativeRdb::E_OK) {
        SELECTION_HILOGE("DB OnUpgrade failed: %{public}d", ret);
        return SELECTION_CONFIG_RDB_EXECUTE_FAILTURE;
    }
    return SELECTION_CONFIG_OK;
}

int32_t SelectionConfigDataBaseCallBack::OnDowngrade(OHOS::NativeRdb::RdbStore &store, int32_t oldVersion,
    int32_t newVersion)
{
    SELECTION_HILOGI("DB OnDowngrade Enter");
    (void)store;
    (void)oldVersion;
    (void)newVersion;
    return SELECTION_CONFIG_OK;
}

} // namespace SelectionFwk
} // namespace OHOS
