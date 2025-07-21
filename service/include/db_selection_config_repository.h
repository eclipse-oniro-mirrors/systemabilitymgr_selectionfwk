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

#ifndef DB_SELECTION_CONFIG_REPOSITORY_H
#define DB_SELECTION_CONFIG_REPOSITORY_H

#include <mutex>
#include <optional>
#include <string>

#include "data_ability_predicates.h"
#include "rdb_errno.h"
#include "rdb_helper.h"
#include "rdb_open_callback.h"
#include "rdb_predicates.h"
#include "rdb_store.h"
#include "result_set.h"
#include "selection_config.h"
#include "selection_config_database.h"
#include "value_object.h"

namespace OHOS {
namespace SelectionFwk {

class DbSelectionConfigRepository {
public:
    static std::shared_ptr<DbSelectionConfigRepository> GetInstance();
    int Save(int uid, const SelectionConfig &info);
    std::optional<SelectionConfig> GetOneByUserId(int uid);

private:
    struct SelectionConfigTableInfo {
        int32_t rowCount;
        int32_t columnCount;
        int32_t primaryKeyIndex;
        int32_t uidIndex;
        int32_t enableIndex;
        int32_t applicationInfoIndex;
        int32_t triggerIndex;
        int32_t shortcutKeysIndex;
    };

private:
    DbSelectionConfigRepository();
    DISALLOW_COPY_AND_MOVE(DbSelectionConfigRepository);
    int GetConfigFromDatabase(const OHOS::NativeRdb::RdbPredicates &rdbPredicates,
        const std::vector<std::string> &columns, SelectionConfig &info);
    int ProcessQueryResult(const std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet,
        SelectionConfig &info);
    int RetrieveResultSetMetadata(const std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet,
        struct SelectionConfigTableInfo &table);

    static std::shared_ptr<DbSelectionConfigRepository> instance_;
    std::mutex databaseMutex_;
    std::shared_ptr<SelectionConfigDataBase> selectionDatabase_;
};
} // namespace SelectionFwk
} // namespace OHOS

#endif // DB_SELECTION_CONFIG_REPOSITORY_H