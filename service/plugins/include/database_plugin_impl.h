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

#ifndef DATABASE_PLUGIN_IMPL_H
#define DATABASE_PLUGIN_IMPL_H

#include <mutex>
#include <memory>
#include "selection_config.h"

namespace OHOS {
namespace NativeRdb {
class ResultSet;
} // namespace NativeRdb

namespace SelectionFwk {

// 数据库插件实现类（内部使用，通过C函数导出）
class DatabasePluginImpl {
public:
    DatabasePluginImpl() = default;
    ~DatabasePluginImpl() = default;

    bool Initialize();
    void Cleanup();
    const char* GetModuleName();
    int GetModuleVersion();

    int Save(int uid, const SelectionConfig &info);
    std::optional<SelectionConfig> GetOneByUserId(int uid);
    bool IsAvailable() const;
    bool HealthCheck() const;
    const char* GetStatus() const;

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

    mutable std::mutex databaseMutex_;
    std::shared_ptr<class SelectionConfigDataBase> selectionDatabase_;

    int ProcessQueryResult(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        SelectionConfig &info);
    int RetrieveResultSetMetadata(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        struct SelectionConfigTableInfo &table);
};

} // namespace SelectionFwk
} // namespace OHOS

#endif // DATABASE_PLUGIN_IMPL_H
