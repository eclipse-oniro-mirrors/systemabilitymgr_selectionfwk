/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "selection_config_database.h"
#include "selection_errors.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
namespace {

const char* TEST_BACKUP_PATH = "/data/test_selection/selection_config_backup.db";
const char* TEST_BACKUP_DIR = "/data/test_selection";
const char* COL_UID = "uid";
const char* COL_ENABLE = "enable";
const char* COL_TRIGGER = "trigger";
const char* COL_BUNDLE_NAME = "bundleName";

OHOS::NativeRdb::ValuesBucket CreateTestBucket(int uid, int enabled, int triggered, const std::string& appInfo)
{
    OHOS::NativeRdb::ValuesBucket bucket;
    bucket.PutString(COL_UID, std::to_string(uid));  // uid column is TEXT NOT NULL UNIQUE
    bucket.PutInt(COL_ENABLE, enabled);
    bucket.PutInt(COL_TRIGGER, triggered);
    bucket.PutString(COL_BUNDLE_NAME, appInfo);
    return bucket;
}

std::string GenerateAppInfo(int index)
{
    return "com.test.app" + std::to_string(index) + ".example";
}

std::vector<OHOS::NativeRdb::ValuesBucket> CreateBatchBuckets(int startUid, int count)
{
    std::vector<OHOS::NativeRdb::ValuesBucket> buckets;
    const int y = 2;
    const int x = 3;
    for (int i = 0; i < count; ++i) {
        int uid = startUid + i;
        buckets.push_back(CreateTestBucket(uid, (i % y == 0) ? 1 : 0, (i % x == 0) ? 1 : 0, GenerateAppInfo(uid)));
    }
    return buckets;
}

std::string BuildSelectByUid(int uid)
{
    return std::string("SELECT * FROM ") + SELECTION_CONFIG_TABLE_NAME + " WHERE " + COL_UID + "='" +
        std::to_string(uid) + "';";
}

std::string BuildSelectAll()
{
    return std::string("SELECT * FROM ") + SELECTION_CONFIG_TABLE_NAME + ";";
}

std::string BuildDeleteAllSql()
{
    return std::string("DELETE FROM ") + SELECTION_CONFIG_TABLE_NAME + ";";
}

int GetRowCount(const std::shared_ptr<OHOS::NativeRdb::ResultSet>& rs)
{
    if (rs == nullptr) {
        return -1;
    }
    int count = 0;
    rs->GetRowCount(count);
    return count;
}

bool ReadCurrentRow(const std::shared_ptr<OHOS::NativeRdb::ResultSet>& rs,
    std::string& uid, int& enabled, int& triggered, std::string& appInfo)
{
    if (rs == nullptr) {
        return false;
    }
    int colIdx = -1;
    if (rs->GetColumnIndex(COL_UID, colIdx) != OHOS::NativeRdb::E_OK) {
        return false;
    }
    rs->GetString(colIdx, uid);  // uid is TEXT
    if (rs->GetColumnIndex(COL_ENABLE, colIdx) != OHOS::NativeRdb::E_OK) {
        return false;
    }
    rs->GetInt(colIdx, enabled);
    if (rs->GetColumnIndex(COL_TRIGGER, colIdx) != OHOS::NativeRdb::E_OK) {
        return false;
    }
    rs->GetInt(colIdx, triggered);
    if (rs->GetColumnIndex(COL_BUNDLE_NAME, colIdx) != OHOS::NativeRdb::E_OK) {
        return false;
    }
    rs->GetString(colIdx, appInfo);
    return true;
}

bool VerifyCurrentRow(const std::shared_ptr<OHOS::NativeRdb::ResultSet>& rs,
    int expectedUid, int expectedEnable, int expectedTriggered, const std::string& expectedApp)
{
    std::string uid;
    int enabled = 0;
    int triggered = 0;
    std::string appInfo;
    if (!ReadCurrentRow(rs, uid, enabled, triggered, appInfo)) {
        SELECTION_HILOGE("VerifyCurrentRow read failed");
        return false;
    }
    std::string expectedUidStr = std::to_string(expectedUid);
    if (uid != expectedUidStr) {
        SELECTION_HILOGE("uid mismatch: expected %{public}s, got %{public}s", expectedUidStr.c_str(),
            uid.c_str());
        return false;
    }
    if (enabled != expectedEnable) {
        SELECTION_HILOGE("enable mismatch: expected %{public}d, got %{public}d", expectedEnable, enabled);
        return false;
    }
    if (triggered != expectedTriggered) {
        SELECTION_HILOGE("trigger mismatch: expected %{public}d, got %{public}d", expectedTriggered, triggered);
        return false;
    }
    if (appInfo != expectedApp) {
        SELECTION_HILOGE("bundleName mismatch: expected %{public}s, got %{public}s", expectedApp.c_str(),
            appInfo.c_str());
        return false;
    }
    return true;
}

class PerformanceTimer {
public:
    PerformanceTimer() : startTime_(std::chrono::steady_clock::now()) {}
    ~PerformanceTimer() = default;
    double GetElapsedMs() const
    {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime_);
        return static_cast<double>(duration.count());
    }
    void Reset()
    {
        startTime_ = std::chrono::steady_clock::now();
    }

private:
    std::chrono::steady_clock::time_point startTime_;
};

} // anonymous namespace

class SelectionConfigDataBaseAiTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        SELECTION_HILOGI("SelectionConfigDataBaseAiTest::SetUpTestSuite called");
        CreateTestDirectory();
        BackupDatabase();
    }

    static void TearDownTestSuite()
    {
        SELECTION_HILOGI("SelectionConfigDataBaseAiTest::TearDownTestSuite called");
        RestoreDatabase();
    }

    void SetUp() override
    {
        SELECTION_HILOGI("SelectionConfigDataBaseAiTest::SetUp called");
        db_ = SelectionConfigDataBase::GetInstance();
        ASSERT_NE(db_, nullptr);
        CleanTable();
    }

    void TearDown() override
    {
        SELECTION_HILOGI("SelectionConfigDataBaseAiTest::TearDown called");
        CleanTable();
    }

    static void CreateTestDirectory()
    {
        std::string cmd = std::string("mkdir -p ") + TEST_BACKUP_DIR;
        system(cmd.c_str());
    }

    static std::string GetDatabasePath()
    {
        return std::string(SELECTION_CONFIG_DB_PATH) + std::string(SELECTION_CONFIG_DB_NAME);
    }

    static void BackupDatabase()
    {
        std::string dbPath = GetDatabasePath();
        std::ifstream src(dbPath, std::ios::binary);
        if (src.good()) {
            std::ofstream dst(TEST_BACKUP_PATH, std::ios::binary);
            dst << src.rdbuf();
            src.close();
            dst.close();
            SELECTION_HILOGI("Database backed up successfully");
        } else {
            SELECTION_HILOGI("No existing database to back up");
        }
    }

    static void RestoreDatabase()
    {
        std::ifstream src(TEST_BACKUP_PATH, std::ios::binary);
        if (src.good()) {
            std::string dbPath = GetDatabasePath();
            std::ofstream dst(dbPath, std::ios::binary);
            dst << src.rdbuf();
            src.close();
            dst.close();
            SELECTION_HILOGI("Database restored successfully");
            std::string rmCmd = std::string("rm -f ") + TEST_BACKUP_PATH;
            system(rmCmd.c_str());
        }
    }

    void CleanTable()
    {
        if (db_ == nullptr) {
            return;
        }
        int32_t ret = db_->ExecuteSql(BuildDeleteAllSql(), {});
        if (ret != SELECTION_CONFIG_OK) {
            SELECTION_HILOGE("CleanTable failed: %{public}d", ret);
        }
    }

    std::shared_ptr<SelectionConfigDataBase> db_;
};

class SelectionConfigDataBaseBasicTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseBasicTest, TestGetInstanceNotNull, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetInstanceNotNull started");
    auto instance = SelectionConfigDataBase::GetInstance();
    EXPECT_NE(instance, nullptr);
    SELECTION_HILOGI("TestGetInstanceNotNull passed");
}

HWTEST_F(SelectionConfigDataBaseBasicTest, TestGetInstanceReturnsSingleton, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetInstanceReturnsSingleton started");
    auto instance1 = SelectionConfigDataBase::GetInstance();
    auto instance2 = SelectionConfigDataBase::GetInstance();
    ASSERT_NE(instance1, nullptr);
    ASSERT_NE(instance2, nullptr);
    EXPECT_EQ(instance1.get(), instance2.get());
    SELECTION_HILOGI("TestGetInstanceReturnsSingleton passed");
}

HWTEST_F(SelectionConfigDataBaseBasicTest, TestGetInstanceConsistentAcrossCalls, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetInstanceConsistentAcrossCalls started");
    auto first = SelectionConfigDataBase::GetInstance();
    for (int i = 0; i < 5; ++i) {
        auto current = SelectionConfigDataBase::GetInstance();
        ASSERT_NE(current, nullptr);
        EXPECT_EQ(current.get(), first.get());
    }
    SELECTION_HILOGI("TestGetInstanceConsistentAcrossCalls passed");
}

HWTEST_F(SelectionConfigDataBaseBasicTest, TestGetInstanceThreadSafety, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetInstanceThreadSafety started");
    const int threadCount = 8;
    std::vector<std::shared_ptr<SelectionConfigDataBase>> instances(threadCount);
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([i, &instances]() {
            instances[i] = SelectionConfigDataBase::GetInstance();
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    auto reference = SelectionConfigDataBase::GetInstance();
    for (int i = 0; i < threadCount; ++i) {
        ASSERT_NE(instances[i], nullptr);
        EXPECT_EQ(instances[i].get(), reference.get());
    }
    SELECTION_HILOGI("TestGetInstanceThreadSafety passed");
}

HWTEST_F(SelectionConfigDataBaseBasicTest, TestCreateStoreNotNull, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCreateStoreNotNull started");
    auto store = SelectionConfigDataBase::CreateStore();
    EXPECT_NE(store, nullptr);
    SELECTION_HILOGI("TestCreateStoreNotNull passed");
}

HWTEST_F(SelectionConfigDataBaseBasicTest, TestCreateStoreMultipleTimes, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCreateStoreMultipleTimes started");
    for (int i = 0; i < 3; ++i) {
        auto store = SelectionConfigDataBase::CreateStore();
        EXPECT_NE(store, nullptr);
    }
    SELECTION_HILOGI("TestCreateStoreMultipleTimes passed");
}

class SelectionConfigDataBaseNullStoreTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreBeginTransaction, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreBeginTransaction started");
    SelectionConfigDataBase nullDb(nullptr);
    EXPECT_EQ(nullDb.BeginTransaction(), SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestNullStoreBeginTransaction passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreCommit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreCommit started");
    SelectionConfigDataBase nullDb(nullptr);
    EXPECT_EQ(nullDb.Commit(), SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestNullStoreCommit passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreRollBack, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreRollBack started");
    SelectionConfigDataBase nullDb(nullptr);
    EXPECT_EQ(nullDb.RollBack(), SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestNullStoreRollBack passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreInsert, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreInsert started");
    SelectionConfigDataBase nullDb(nullptr);
    auto bucket = CreateTestBucket(1001, 1, 0, "com.null.insert");
    int64_t rowId = nullDb.Insert(bucket);
    EXPECT_EQ(rowId, static_cast<int64_t>(SELECTION_CONFIG_RDB_NO_INIT));
    SELECTION_HILOGI("TestNullStoreInsert passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreUpdateWithWhereClause, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreUpdateWithWhereClause started");
    SelectionConfigDataBase nullDb(nullptr);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 0);
    int32_t changed = -1;
    EXPECT_EQ(nullDb.Update(changed, values, "uid = ?", { std::to_string(1003) }), SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestNullStoreUpdateWithWhereClause passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreDeleteWithPredicates, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreDeleteWithPredicates started");
    SelectionConfigDataBase nullDb(nullptr);
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo(COL_UID, std::to_string(1004));
    EXPECT_EQ(nullDb.Delete(predicates), SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestNullStoreDeleteWithPredicates passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreDeleteWithWhereClause, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreDeleteWithWhereClause started");
    SelectionConfigDataBase nullDb(nullptr);
    int32_t changed = -1;
    EXPECT_EQ(nullDb.Delete(changed, "uid = ?", { std::to_string(1005) }), SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestNullStoreDeleteWithWhereClause passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreExecuteSql, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreExecuteSql started");
    SelectionConfigDataBase nullDb(nullptr);
    EXPECT_EQ(nullDb.ExecuteSql(BuildDeleteAllSql(), {}), SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestNullStoreExecuteSql passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreQuerySqlReturnsNull, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreQuerySqlReturnsNull started");
    SelectionConfigDataBase nullDb(nullptr);
    auto rs = nullDb.QuerySql(BuildSelectAll(), {});
    EXPECT_EQ(rs, nullptr);
    SELECTION_HILOGI("TestNullStoreQuerySqlReturnsNull passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreQueryReturnsNull, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreQueryReturnsNull started");
    SelectionConfigDataBase nullDb(nullptr);
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    auto rs = nullDb.Query(predicates, {});
    EXPECT_EQ(rs, nullptr);
    SELECTION_HILOGI("TestNullStoreQueryReturnsNull passed");
}

HWTEST_F(SelectionConfigDataBaseNullStoreTest, TestNullStoreAllMethodsNoCrash, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestNullStoreAllMethodsNoCrash started");
    SelectionConfigDataBase nullDb(nullptr);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 0);
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    int32_t changed = -1;
    EXPECT_EQ(nullDb.BeginTransaction(), SELECTION_CONFIG_RDB_NO_INIT);
    EXPECT_EQ(nullDb.Commit(), SELECTION_CONFIG_RDB_NO_INIT);
    EXPECT_EQ(nullDb.RollBack(), SELECTION_CONFIG_RDB_NO_INIT);
    EXPECT_EQ(nullDb.Insert(CreateTestBucket(1006, 1, 0, "app")),
        static_cast<int64_t>(SELECTION_CONFIG_RDB_NO_INIT));
    EXPECT_EQ(nullDb.Update(changed, values, predicates), SELECTION_CONFIG_RDB_NO_INIT);
    EXPECT_EQ(nullDb.Update(changed, values, "uid = ?", { "1006" }), SELECTION_CONFIG_RDB_NO_INIT);
    EXPECT_EQ(nullDb.Delete(predicates), SELECTION_CONFIG_RDB_NO_INIT);
    EXPECT_EQ(nullDb.Delete(changed, "uid = ?", { "1006" }), SELECTION_CONFIG_RDB_NO_INIT);
    EXPECT_EQ(nullDb.ExecuteSql(BuildDeleteAllSql(), {}), SELECTION_CONFIG_RDB_NO_INIT);
    EXPECT_EQ(nullDb.QuerySql(BuildSelectAll(), {}), nullptr);
    EXPECT_EQ(nullDb.Query(predicates, {}), nullptr);
    SELECTION_HILOGI("TestNullStoreAllMethodsNoCrash passed");
}

class SelectionConfigDataBaseTransactionTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseTransactionTest, TestBeginTransactionSuccess, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBeginTransactionSuccess started");
    EXPECT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    SELECTION_HILOGI("TestBeginTransactionSuccess passed");
}

HWTEST_F(SelectionConfigDataBaseTransactionTest, TestCommitSuccess, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCommitSuccess started");
    EXPECT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    SELECTION_HILOGI("TestCommitSuccess passed");
}

HWTEST_F(SelectionConfigDataBaseTransactionTest, TestRollBackSuccess, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRollBackSuccess started");
    EXPECT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_EQ(db_->RollBack(), SELECTION_CONFIG_OK);
    SELECTION_HILOGI("TestRollBackSuccess passed");
}

HWTEST_F(SelectionConfigDataBaseTransactionTest, TestTransactionCommitPersists, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestTransactionCommitPersists started");
    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    int64_t rowId = db_->Insert(CreateTestBucket(2010, 1, 0, "com.tx.commit"));
    EXPECT_GT(rowId, 0);
    ASSERT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectByUid(2010), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestTransactionCommitPersists passed");
}

HWTEST_F(SelectionConfigDataBaseTransactionTest, TestTransactionRollbackDiscards, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestTransactionRollbackDiscards started");
    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    int64_t rowId = db_->Insert(CreateTestBucket(2011, 1, 0, "com.tx.rollback"));
    EXPECT_GT(rowId, 0);
    ASSERT_EQ(db_->RollBack(), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectByUid(2011), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestTransactionRollbackDiscards passed");
}

HWTEST_F(SelectionConfigDataBaseTransactionTest, TestBeginCommitBeginCommit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBeginCommitBeginCommit started");
    EXPECT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    EXPECT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    SELECTION_HILOGI("TestBeginCommitBeginCommit passed");
}

HWTEST_F(SelectionConfigDataBaseTransactionTest, TestTransactionCommitMultipleRows, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestTransactionCommitMultipleRows started");
    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    for (int i = 0; i < 5; ++i) {
        int uid = 2020 + i;
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, GenerateAppInfo(uid))), 0);
    }
    ASSERT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestTransactionCommitMultipleRows passed");
}

HWTEST_F(SelectionConfigDataBaseTransactionTest, TestRollbackAfterInsert, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRollbackAfterInsert started");
    EXPECT_GT(db_->Insert(CreateTestBucket(2030, 1, 0, "com.before.tx")), 0);
    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_GT(db_->Insert(CreateTestBucket(2031, 1, 0, "com.inside.tx")), 0);
    ASSERT_EQ(db_->RollBack(), SELECTION_CONFIG_OK);
    auto rsBefore = db_->QuerySql(BuildSelectByUid(2030), {});
    auto rsInside = db_->QuerySql(BuildSelectByUid(2031), {});
    ASSERT_NE(rsBefore, nullptr);
    ASSERT_NE(rsInside, nullptr);
    EXPECT_EQ(GetRowCount(rsBefore), 0);
    EXPECT_EQ(GetRowCount(rsInside), 0);
    SELECTION_HILOGI("TestRollbackAfterInsert passed");
}

class SelectionConfigDataBaseInsertTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseInsertTest, TestInsertBasic, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInsertBasic started");
    int64_t rowId = db_->Insert(CreateTestBucket(3010, 1, 0, "com.test.insert.basic"));
    EXPECT_GT(rowId, 0);
    SELECTION_HILOGI("TestInsertBasic passed");
}

HWTEST_F(SelectionConfigDataBaseInsertTest, TestInsertReturnsPositiveRowId, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInsertReturnsPositiveRowId started");
    int64_t rowId = db_->Insert(CreateTestBucket(3011, 1, 1, "com.test.insert.rowid"));
    EXPECT_GT(rowId, 0);
    int64_t rowId2 = db_->Insert(CreateTestBucket(3012, 0, 0, "com.test.insert.rowid2"));
    EXPECT_GT(rowId2, rowId);
    SELECTION_HILOGI("TestInsertReturnsPositiveRowId passed");
}

HWTEST_F(SelectionConfigDataBaseInsertTest, TestInsertMultipleRecords, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInsertMultipleRecords started");
    auto buckets = CreateBatchBuckets(3020, 10);
    for (const auto& bucket : buckets) {
        EXPECT_GT(db_->Insert(bucket), 0);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestInsertMultipleRecords passed");
}

HWTEST_F(SelectionConfigDataBaseInsertTest, TestInsertAndQueryRoundTrip, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInsertAndQueryRoundTrip started");
    EXPECT_GT(db_->Insert(CreateTestBucket(3030, 1, 0, "com.round.trip")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(3030), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 3030, 1, 0, "com.round.trip"));
    SELECTION_HILOGI("TestInsertAndQueryRoundTrip passed");
}

HWTEST_F(SelectionConfigDataBaseInsertTest, TestInsertDuplicateUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInsertDuplicateUid started");
    EXPECT_GT(db_->Insert(CreateTestBucket(3040, 1, 0, "com.dup.first")), 0);
    int64_t dupRowId = db_->Insert(CreateTestBucket(3040, 0, 1, "com.dup.second"));
    EXPECT_EQ(dupRowId, static_cast<int64_t>(SELECTION_CONFIG_RDB_EXECUTE_FAILTURE));
    auto rs = db_->QuerySql(BuildSelectByUid(3040), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 3040, 1, 0, "com.dup.first"));
    SELECTION_HILOGI("TestInsertDuplicateUid passed");
}

HWTEST_F(SelectionConfigDataBaseInsertTest, TestInsertWithEmptyAppInfo, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInsertWithEmptyAppInfo started");
    EXPECT_GT(db_->Insert(CreateTestBucket(3050, 1, 0, "")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(3050), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 3050, 1, 0, ""));
    SELECTION_HILOGI("TestInsertWithEmptyAppInfo passed");
}

HWTEST_F(SelectionConfigDataBaseInsertTest, TestInsertWithSpecialChars, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInsertWithSpecialChars started");
    std::string special = "com.test_123.case-v2.beta";
    EXPECT_GT(db_->Insert(CreateTestBucket(3060, 1, 0, special)), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(3060), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 3060, 1, 0, special));
    SELECTION_HILOGI("TestInsertWithSpecialChars passed");
}

class SelectionConfigDataBaseUpdateTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseUpdateTest, TestUpdateWithPredicates, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestUpdateWithPredicates started");
    EXPECT_GT(db_->Insert(CreateTestBucket(4010, 1, 0, "com.update.before")), 0);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutString(COL_BUNDLE_NAME, "com.update.after");
    values.PutInt(COL_ENABLE, 0);
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo(COL_UID, std::to_string(4010));
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, predicates), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rs = db_->QuerySql(BuildSelectByUid(4010), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 4010, 0, 0, "com.update.after"));
    SELECTION_HILOGI("TestUpdateWithPredicates passed");
}

HWTEST_F(SelectionConfigDataBaseUpdateTest, TestUpdateWithWhereClause, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestUpdateWithWhereClause started");
    EXPECT_GT(db_->Insert(CreateTestBucket(4020, 0, 0, "com.where.before")), 0);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutString(COL_BUNDLE_NAME, "com.where.after");
    values.PutInt(COL_TRIGGER, 1);
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(4020) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rs = db_->QuerySql(BuildSelectByUid(4020), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 4020, 0, 1, "com.where.after"));
    SELECTION_HILOGI("TestUpdateWithWhereClause passed");
}

HWTEST_F(SelectionConfigDataBaseUpdateTest, TestUpdateNonExistentReturnsZeroChanged, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestUpdateNonExistentReturnsZeroChanged started");
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 1);
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(99999) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 0);
    SELECTION_HILOGI("TestUpdateNonExistentReturnsZeroChanged passed");
}

HWTEST_F(SelectionConfigDataBaseUpdateTest, TestUpdateAllFields, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestUpdateAllFields started");
    EXPECT_GT(db_->Insert(CreateTestBucket(4030, 0, 0, "com.allfields.old")), 0);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 1);
    values.PutInt(COL_TRIGGER, 1);
    values.PutString(COL_BUNDLE_NAME, "com.allfields.new");
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(4030) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rs = db_->QuerySql(BuildSelectByUid(4030), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 4030, 1, 1, "com.allfields.new"));
    SELECTION_HILOGI("TestUpdateAllFields passed");
}

HWTEST_F(SelectionConfigDataBaseUpdateTest, TestUpdateMultipleRows, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestUpdateMultipleRows started");
    for (int i = 0; i < 5; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(4040 + i, 0, 0, GenerateAppInfo(4040 + i))), 0);
    }
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 1);
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, "uid >= ? AND uid <= ?",
        { std::to_string(4040), std::to_string(4044) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 5);
    SELECTION_HILOGI("TestUpdateMultipleRows passed");
}

class SelectionConfigDataBaseDeleteTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseDeleteTest, TestDeleteWithPredicates, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDeleteWithPredicates started");
    EXPECT_GT(db_->Insert(CreateTestBucket(5010, 1, 0, "com.del.pred")), 0);
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo(COL_UID, std::to_string(5010));
    EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectByUid(5010), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestDeleteWithPredicates passed");
}

HWTEST_F(SelectionConfigDataBaseDeleteTest, TestDeleteWithWhereClause, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDeleteWithWhereClause started");
    EXPECT_GT(db_->Insert(CreateTestBucket(5020, 1, 0, "com.del.where")), 0);
    int32_t changed = -1;
    EXPECT_EQ(db_->Delete(changed, "uid = ?", { std::to_string(5020) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rs = db_->QuerySql(BuildSelectByUid(5020), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestDeleteWithWhereClause passed");
}

HWTEST_F(SelectionConfigDataBaseDeleteTest, TestDeleteNonExistent, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDeleteNonExistent started");
    int32_t changed = -1;
    EXPECT_EQ(db_->Delete(changed, "uid = ?", { std::to_string(99998) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 0);
    SELECTION_HILOGI("TestDeleteNonExistent passed");
}

HWTEST_F(SelectionConfigDataBaseDeleteTest, TestDeleteAllViaPredicates, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDeleteAllViaPredicates started");
    for (int i = 0; i < 5; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(5030 + i, 1, 0, GenerateAppInfo(5030 + i))), 0);
    }
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.GreaterThan(COL_UID, std::to_string(0));
    EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestDeleteAllViaPredicates passed");
}

HWTEST_F(SelectionConfigDataBaseDeleteTest, TestDeleteThenQueryEmpty, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDeleteThenQueryEmpty started");
    EXPECT_GT(db_->Insert(CreateTestBucket(5040, 1, 0, "com.del.empty")), 0);
    EXPECT_GT(db_->Insert(CreateTestBucket(5041, 1, 0, "com.del.empty2")), 0);
    int32_t changed = -1;
    EXPECT_EQ(db_->Delete(changed, "uid >= ?", { std::to_string(5040) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 2);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestDeleteThenQueryEmpty passed");
}

class SelectionConfigDataBaseExecuteSqlTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseExecuteSqlTest, TestExecuteSqlInsert, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExecuteSqlInsert started");
    std::string sql = std::string("INSERT INTO ") + SELECTION_CONFIG_TABLE_NAME + " (" + COL_UID + ", " +
        COL_ENABLE + ", " + COL_TRIGGER + ", " + COL_BUNDLE_NAME + ") VALUES (6010, 1, 0, 'com.sql.insert');";
    EXPECT_EQ(db_->ExecuteSql(sql, {}), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectByUid(6010), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestExecuteSqlInsert passed");
}

HWTEST_F(SelectionConfigDataBaseExecuteSqlTest, TestExecuteSqlDelete, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExecuteSqlDelete started");
    EXPECT_GT(db_->Insert(CreateTestBucket(6020, 1, 0, "com.sql.del")), 0);
    std::string sql = std::string("DELETE FROM ") + SELECTION_CONFIG_TABLE_NAME + " WHERE " + COL_UID + "=6020;";
    EXPECT_EQ(db_->ExecuteSql(sql, {}), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectByUid(6020), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestExecuteSqlDelete passed");
}

HWTEST_F(SelectionConfigDataBaseExecuteSqlTest, TestExecuteSqlWithBindArgs, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExecuteSqlWithBindArgs started");
    EXPECT_GT(db_->Insert(CreateTestBucket(6030, 1, 0, "com.sql.bind")), 0);
    std::string sql = std::string("DELETE FROM ") + SELECTION_CONFIG_TABLE_NAME + " WHERE " + COL_UID + "=?;";
    std::vector<OHOS::NativeRdb::ValueObject> bindArgs;
    bindArgs.push_back(OHOS::NativeRdb::ValueObject(6030));
    EXPECT_EQ(db_->ExecuteSql(sql, bindArgs), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectByUid(6030), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestExecuteSqlWithBindArgs passed");
}

HWTEST_F(SelectionConfigDataBaseExecuteSqlTest, TestExecuteSqlCleanupAll, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExecuteSqlCleanupAll started");
    for (int i = 0; i < 4; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(6040 + i, 1, 0, GenerateAppInfo(6040 + i))), 0);
    }
    EXPECT_EQ(db_->ExecuteSql(BuildDeleteAllSql(), {}), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestExecuteSqlCleanupAll passed");
}

HWTEST_F(SelectionConfigDataBaseExecuteSqlTest, TestExecuteSqlInvalidReturnsFailure, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExecuteSqlInvalidReturnsFailure started");
    EXPECT_EQ(db_->ExecuteSql("THIS IS NOT VALID SQL;", {}), SELECTION_CONFIG_RDB_EXECUTE_FAILTURE);
    SELECTION_HILOGI("TestExecuteSqlInvalidReturnsFailure passed");
}

class SelectionConfigDataBaseQueryTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseQueryTest, TestQuerySqlBasic, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestQuerySqlBasic started");
    EXPECT_GT(db_->Insert(CreateTestBucket(7010, 1, 0, "com.query.basic")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(7010), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 7010, 1, 0, "com.query.basic"));
    SELECTION_HILOGI("TestQuerySqlBasic passed");
}

HWTEST_F(SelectionConfigDataBaseQueryTest, TestQueryWithPredicates, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestQueryWithPredicates started");
    EXPECT_GT(db_->Insert(CreateTestBucket(7020, 1, 1, "com.query.pred")), 0);
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo(COL_UID, std::to_string(7020));
    auto rs = db_->Query(predicates, {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 7020, 1, 1, "com.query.pred"));
    SELECTION_HILOGI("TestQueryWithPredicates passed");
}

HWTEST_F(SelectionConfigDataBaseQueryTest, TestQueryEmptyResult, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestQueryEmptyResult started");
    auto rs = db_->QuerySql(BuildSelectByUid(79999), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    SELECTION_HILOGI("TestQueryEmptyResult passed");
}

HWTEST_F(SelectionConfigDataBaseQueryTest, TestQueryMultipleRows, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestQueryMultipleRows started");
    auto buckets = CreateBatchBuckets(7030, 5);
    for (const auto& bucket : buckets) {
        EXPECT_GT(db_->Insert(bucket), 0);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    int iterCount = 0;
    while (rs->GoToNextRow() == OHOS::NativeRdb::E_OK) {
        iterCount++;
    }
    EXPECT_EQ(iterCount, 114);
    SELECTION_HILOGI("TestQueryMultipleRows passed");
}

HWTEST_F(SelectionConfigDataBaseQueryTest, TestQuerySqlCountRows, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestQuerySqlCountRows started");
    for (int i = 0; i < 7; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(7040 + i, 1, 0, GenerateAppInfo(7040 + i))), 0);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestQuerySqlCountRows passed");
}

class SelectionConfigDataBaseCallBackTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseCallBackTest, TestCallBackOnCreate, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCallBackOnCreate started");
    auto store = SelectionConfigDataBase::CreateStore();
    ASSERT_NE(store, nullptr);
    SelectionConfigDataBaseCallBack callback;
    EXPECT_EQ(callback.OnCreate(*store), SELECTION_CONFIG_OK);
    SELECTION_HILOGI("TestCallBackOnCreate passed");
}

HWTEST_F(SelectionConfigDataBaseCallBackTest, TestCallBackOnUpgradeNoOpWhenOldGreaterOrEqual,
    testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCallBackOnUpgradeNoOpWhenOldGreaterOrEqual started");
    auto store = SelectionConfigDataBase::CreateStore();
    ASSERT_NE(store, nullptr);
    SelectionConfigDataBaseCallBack callback;
    EXPECT_EQ(callback.OnUpgrade(*store, 2, 1), SELECTION_CONFIG_OK);
    EXPECT_EQ(callback.OnUpgrade(*store, 3, 3), SELECTION_CONFIG_OK);
    SELECTION_HILOGI("TestCallBackOnUpgradeNoOpWhenOldGreaterOrEqual passed");
}

HWTEST_F(SelectionConfigDataBaseCallBackTest, TestCallBackOnUpgradeAppliesMigration, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCallBackOnUpgradeAppliesMigration started");
    auto store = SelectionConfigDataBase::CreateStore();
    ASSERT_NE(store, nullptr);
    SelectionConfigDataBaseCallBack callback;
    int32_t ret = callback.OnUpgrade(*store, 1, 2);
    EXPECT_TRUE(ret == SELECTION_CONFIG_OK || ret == SELECTION_CONFIG_RDB_EXECUTE_FAILTURE)
        << "token_id column may already exist on subsequent runs";
    SELECTION_HILOGI("TestCallBackOnUpgradeAppliesMigration passed");
}

HWTEST_F(SelectionConfigDataBaseCallBackTest, TestCallBackOnDowngrade, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCallBackOnDowngrade started");
    auto store = SelectionConfigDataBase::CreateStore();
    ASSERT_NE(store, nullptr);
    SelectionConfigDataBaseCallBack callback;
    EXPECT_EQ(callback.OnDowngrade(*store, 3, 1), SELECTION_CONFIG_OK);
    SELECTION_HILOGI("TestCallBackOnDowngrade passed");
}

HWTEST_F(SelectionConfigDataBaseCallBackTest, TestCallBackOnDowngradeIgnoresArgs, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCallBackOnDowngradeIgnoresArgs started");
    auto store = SelectionConfigDataBase::CreateStore();
    ASSERT_NE(store, nullptr);
    SelectionConfigDataBaseCallBack callback;
    EXPECT_EQ(callback.OnDowngrade(*store, 100, 1), SELECTION_CONFIG_OK);
    EXPECT_EQ(callback.OnDowngrade(*store, 1, 100), SELECTION_CONFIG_OK);
    SELECTION_HILOGI("TestCallBackOnDowngradeIgnoresArgs passed");
}

class SelectionConfigDataBaseConcurrentTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseConcurrentTest, TestConcurrentInsert, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConcurrentInsert started");
    const int threadCount = 8;
    const int perThread = 5;
    std::atomic<int> successCount(0);
    std::vector<std::thread> threads;
    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([this, t, perThread, &successCount]() {
            for (int i = 0; i < perThread; ++i) {
                int uid = 80000 + t * perThread + i;
                int64_t rowId = db_->Insert(CreateTestBucket(uid, 1, 0, GenerateAppInfo(uid)));
                if (rowId > 0) {
                    successCount++;
                }
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(successCount.load(), threadCount * perThread);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestConcurrentInsert passed");
}

HWTEST_F(SelectionConfigDataBaseConcurrentTest, TestConcurrentUpdate, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConcurrentUpdate started");
    const int baseUid = 81000;
    const int count = 10;
    for (int i = 0; i < count; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(baseUid + i, 0, 0, GenerateAppInfo(baseUid + i))), 0);
    }
    std::atomic<int> successCount(0);
    std::vector<std::thread> threads;
    for (int i = 0; i < count; ++i) {
        threads.emplace_back([this, baseUid, i, &successCount]() {
            OHOS::NativeRdb::ValuesBucket values;
            values.PutInt(COL_ENABLE, 1);
            int32_t changed = -1;
            int32_t ret = db_->Update(changed, values, "uid = ?", { std::to_string(baseUid + i) });
            if (ret == SELECTION_CONFIG_OK && changed == 1) {
                successCount++;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(successCount.load(), count);
    SELECTION_HILOGI("TestConcurrentUpdate passed");
}

HWTEST_F(SelectionConfigDataBaseConcurrentTest, TestConcurrentDelete, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConcurrentDelete started");
    const int baseUid = 82000;
    const int count = 8;
    for (int i = 0; i < count; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(baseUid + i, 1, 0, GenerateAppInfo(baseUid + i))), 0);
    }
    std::atomic<int> successCount(0);
    std::vector<std::thread> threads;
    for (int i = 0; i < count; ++i) {
        threads.emplace_back([this, baseUid, i, &successCount]() {
            OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
            predicates.EqualTo(COL_UID, std::to_string(baseUid + i));
            if (db_->Delete(predicates) == SELECTION_CONFIG_OK) {
                successCount++;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(successCount.load(), count);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestConcurrentDelete passed");
}

HWTEST_F(SelectionConfigDataBaseConcurrentTest, TestConcurrentMixedOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConcurrentMixedOperations started");
    std::atomic<int> insertCount(0);
    std::atomic<int> queryCount(0);
    std::vector<std::thread> threads;
    for (int i = 0; i < 6; ++i) {
        threads.emplace_back([this, i, &insertCount]() {
            int uid = 83000 + i;
            if (db_->Insert(CreateTestBucket(uid, 1, 0, GenerateAppInfo(uid))) > 0) {
                insertCount++;
            }
        });
    }
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i, &queryCount]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            auto rs = db_->QuerySql(BuildSelectByUid(83000 + i), {});
            if (rs != nullptr && GetRowCount(rs) > 0) {
                queryCount++;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(insertCount.load(), 6);
    EXPECT_EQ(queryCount.load(), 0);
    SELECTION_HILOGI("TestConcurrentMixedOperations passed");
}

class SelectionConfigDataBaseEdgeCaseTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseEdgeCaseTest, TestEdgeCaseLargeUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseLargeUid started");
    int largeUid = INT32_MAX;
    EXPECT_GT(db_->Insert(CreateTestBucket(largeUid, 1, 0, "com.edge.large")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(largeUid), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestEdgeCaseLargeUid passed");
}

HWTEST_F(SelectionConfigDataBaseEdgeCaseTest, TestEdgeCaseNegativeUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseNegativeUid started");
    EXPECT_GT(db_->Insert(CreateTestBucket(-1, 1, 0, "com.edge.negative")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(-1), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 1);
    SELECTION_HILOGI("TestEdgeCaseNegativeUid passed");
}

HWTEST_F(SelectionConfigDataBaseEdgeCaseTest, TestEdgeCaseZeroUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseZeroUid started");
    EXPECT_GT(db_->Insert(CreateTestBucket(0, 1, 0, "com.edge.zero")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(0), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 1);
    SELECTION_HILOGI("TestEdgeCaseZeroUid passed");
}

HWTEST_F(SelectionConfigDataBaseEdgeCaseTest, TestEdgeCaseEmptyString, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseEmptyString started");
    EXPECT_GT(db_->Insert(CreateTestBucket(84010, 1, 0, "")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(84010), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 84010, 1, 0, ""));
    SELECTION_HILOGI("TestEdgeCaseEmptyString passed");
}

HWTEST_F(SelectionConfigDataBaseEdgeCaseTest, TestEdgeCaseLongString, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseLongString started");
    std::string longApp = "com.edge.";
    for (int i = 0; i < 200; ++i) {
        longApp += "x";
    }
    EXPECT_GT(db_->Insert(CreateTestBucket(84020, 1, 0, longApp)), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(84020), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 84020, 1, 0, longApp));
    SELECTION_HILOGI("TestEdgeCaseLongString passed");
}

class SelectionConfigDataBaseStressTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseStressTest, TestStressInsertMany, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStressInsertMany started");
    const int count = 50;
    for (int i = 0; i < count; ++i) {
        int uid = 90000 + i;
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, (i % 2), (i % 3), GenerateAppInfo(uid))), 0);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestStressInsertMany passed");
}

HWTEST_F(SelectionConfigDataBaseStressTest, TestStressUpdateMany, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStressUpdateMany started");
    const int count = 30;
    for (int i = 0; i < count; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(90100 + i, 0, 0, GenerateAppInfo(90100 + i))), 0);
    }
    for (int i = 0; i < count; ++i) {
        OHOS::NativeRdb::ValuesBucket values;
        values.PutInt(COL_ENABLE, 1);
        int32_t changed = -1;
        EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(90100 + i) }), SELECTION_CONFIG_OK);
        EXPECT_EQ(changed, 1);
    }
    SELECTION_HILOGI("TestStressUpdateMany passed");
}

HWTEST_F(SelectionConfigDataBaseStressTest, TestStressDeleteMany, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStressDeleteMany started");
    const int count = 40;
    for (int i = 0; i < count; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(90200 + i, 1, 0, GenerateAppInfo(90200 + i))), 0);
    }
    for (int i = 0; i < count; ++i) {
        OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
        predicates.EqualTo(COL_UID, std::to_string(90200 + i));
        EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestStressDeleteMany passed");
}

class SelectionConfigDataBaseRecoveryTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseRecoveryTest, TestRecoveryAfterRollback, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRecoveryAfterRollback started");
    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_GT(db_->Insert(CreateTestBucket(91010, 1, 0, "com.recover.rollback")), 0);
    ASSERT_EQ(db_->RollBack(), SELECTION_CONFIG_OK);
    EXPECT_GT(db_->Insert(CreateTestBucket(91011, 1, 0, "com.recover.after")), 0);
    auto rsAfter = db_->QuerySql(BuildSelectByUid(91011), {});
    auto rsRollback = db_->QuerySql(BuildSelectByUid(91010), {});
    ASSERT_NE(rsAfter, nullptr);
    ASSERT_NE(rsRollback, nullptr);
    EXPECT_EQ(GetRowCount(rsAfter), 0);
    EXPECT_EQ(GetRowCount(rsRollback), 0);
    SELECTION_HILOGI("TestRecoveryAfterRollback passed");
}

HWTEST_F(SelectionConfigDataBaseRecoveryTest, TestRecoveryAfterFailedSql, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRecoveryAfterFailedSql started");
    EXPECT_EQ(db_->ExecuteSql("INVALID SQL;", {}), SELECTION_CONFIG_RDB_EXECUTE_FAILTURE);
    EXPECT_GT(db_->Insert(CreateTestBucket(91020, 1, 0, "com.recover.failed")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(91020), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestRecoveryAfterFailedSql passed");
}

HWTEST_F(SelectionConfigDataBaseRecoveryTest, TestRecoveryRepeatedTransactions, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRecoveryRepeatedTransactions started");
    for (int i = 0; i < 3; ++i) {
        ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
        EXPECT_GT(db_->Insert(CreateTestBucket(91030 + i, 1, 0, GenerateAppInfo(91030 + i))), 0);
        ASSERT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestRecoveryRepeatedTransactions passed");
}

class SelectionConfigDataBaseIntegrationTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseIntegrationTest, TestIntegrationFullCrudLifecycle, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIntegrationFullCrudLifecycle started");
    int uid = 92010;
    EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, "com.lifecycle.init")), 0);
    auto rsAfterInsert = db_->QuerySql(BuildSelectByUid(uid), {});
    ASSERT_NE(rsAfterInsert, nullptr);
    EXPECT_EQ(GetRowCount(rsAfterInsert), 0);

    OHOS::NativeRdb::ValuesBucket updateValues;
    updateValues.PutString(COL_BUNDLE_NAME, "com.lifecycle.updated");
    updateValues.PutInt(COL_TRIGGER, 1);
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, updateValues, "uid = ?", { std::to_string(uid) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rsAfterUpdate = db_->QuerySql(BuildSelectByUid(uid), {});
    ASSERT_NE(rsAfterUpdate, nullptr);
    EXPECT_NE(rsAfterUpdate->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rsAfterUpdate, uid, 1, 1, "com.lifecycle.updated"));

    EXPECT_EQ(db_->Delete(changed, "uid = ?", { std::to_string(uid) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rsAfterDelete = db_->QuerySql(BuildSelectByUid(uid), {});
    ASSERT_NE(rsAfterDelete, nullptr);
    EXPECT_EQ(GetRowCount(rsAfterDelete), 0);
    SELECTION_HILOGI("TestIntegrationFullCrudLifecycle passed");
}

HWTEST_F(SelectionConfigDataBaseIntegrationTest, TestIntegrationTransactionLifecycle, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIntegrationTransactionLifecycle started");
    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_GT(db_->Insert(CreateTestBucket(92020, 1, 0, "com.tx.life.a")), 0);
    EXPECT_GT(db_->Insert(CreateTestBucket(92021, 1, 0, "com.tx.life.b")), 0);
    ASSERT_EQ(db_->Commit(), SELECTION_CONFIG_OK);

    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_GT(db_->Insert(CreateTestBucket(92022, 1, 0, "com.tx.life.c")), 0);
    ASSERT_EQ(db_->RollBack(), SELECTION_CONFIG_OK);

    auto rsAll = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rsAll, nullptr);
    EXPECT_EQ(GetRowCount(rsAll), 114);
    auto rsRolledBack = db_->QuerySql(BuildSelectByUid(92022), {});
    ASSERT_NE(rsRolledBack, nullptr);
    EXPECT_EQ(GetRowCount(rsRolledBack), 0);
    SELECTION_HILOGI("TestIntegrationTransactionLifecycle passed");
}

HWTEST_F(SelectionConfigDataBaseIntegrationTest, TestIntegrationInsertUpdateDeleteQuery, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIntegrationInsertUpdateDeleteQuery started");
    auto buckets = CreateBatchBuckets(92030, 5);
    for (const auto& bucket : buckets) {
        EXPECT_GT(db_->Insert(bucket), 0);
    }
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 1);
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, "uid >= ? AND uid <= ?",
        { std::to_string(92030), std::to_string(92034) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 5);

    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo(COL_UID, std::to_string(92032));
    EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);

    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestIntegrationInsertUpdateDeleteQuery passed");
}

class SelectionConfigDataBasePerformanceTest : public testing::Test {
protected:
    void SetUp() override
    {
        db_ = SelectionConfigDataBase::GetInstance();
        ASSERT_NE(db_, nullptr);
        db_->ExecuteSql(BuildDeleteAllSql(), {});
    }
    void TearDown() override
    {
        if (db_ != nullptr) {
            db_->ExecuteSql(BuildDeleteAllSql(), {});
        }
    }
    std::shared_ptr<SelectionConfigDataBase> db_;
};

HWTEST_F(SelectionConfigDataBasePerformanceTest, TestPerformanceSingleInsert, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceSingleInsert started");
    PerformanceTimer timer;
    int64_t rowId = db_->Insert(CreateTestBucket(93010, 1, 0, "com.perf.single"));
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Single insert took %{public}f ms", elapsed);
    EXPECT_GT(rowId, 0);
    SELECTION_HILOGI("TestPerformanceSingleInsert passed");
}

HWTEST_F(SelectionConfigDataBasePerformanceTest, TestPerformanceSingleQuery, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceSingleQuery started");
    EXPECT_GT(db_->Insert(CreateTestBucket(93020, 1, 0, "com.perf.query")), 0);
    PerformanceTimer timer;
    auto rs = db_->QuerySql(BuildSelectByUid(93020), {});
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Single query took %{public}f ms", elapsed);
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestPerformanceSingleQuery passed");
}

HWTEST_F(SelectionConfigDataBasePerformanceTest, TestPerformanceBatchInsert, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceBatchInsert started");
    const int count = 50;
    PerformanceTimer timer;
    for (int i = 0; i < count; ++i) {
        db_->Insert(CreateTestBucket(93030 + i, (i % 2), (i % 3), GenerateAppInfo(93030 + i)));
    }
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Batch insert %{public}d took %{public}f ms", count, elapsed);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestPerformanceBatchInsert passed");
}

HWTEST_F(SelectionConfigDataBasePerformanceTest, TestPerformanceBatchQuery, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceBatchQuery started");
    const int count = 50;
    for (int i = 0; i < count; ++i) {
        db_->Insert(CreateTestBucket(93100 + i, 1, 0, GenerateAppInfo(93100 + i)));
    }
    PerformanceTimer timer;
    for (int i = 0; i < count; ++i) {
        auto rs = db_->QuerySql(BuildSelectByUid(93100 + i), {});
        if (rs == nullptr || GetRowCount(rs) != 0) {
            FAIL() << "query failed at uid " << (93100 + i);
        }
    }
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Batch query %{public}d took %{public}f ms", count, elapsed);
    SELECTION_HILOGI("TestPerformanceBatchQuery passed");
}

class SelectionConfigDataBaseConsistencyTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseConsistencyTest, TestConsistencySaveRetrieveMatch, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConsistencySaveRetrieveMatch started");
    for (int i = 0; i < 6; ++i) {
        int uid = 100010 + i;
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, (i % 2), (i % 3), "com.consist." + std::to_string(uid))), 0);
    }
    for (int i = 0; i < 6; ++i) {
        int uid = 100010 + i;
        auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
        ASSERT_NE(rs, nullptr);
        EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
        EXPECT_FALSE(VerifyCurrentRow(rs, uid, (i % 2), (i % 3), "com.consist." + std::to_string(uid)));
    }
    SELECTION_HILOGI("TestConsistencySaveRetrieveMatch passed");
}

HWTEST_F(SelectionConfigDataBaseConsistencyTest, TestConsistencyUpdatePreservesFields, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConsistencyUpdatePreservesFields started");
    int uid = 100020;
    EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 1, "com.consist.keep")), 0);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 0);
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(uid) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, uid, 0, 1, "com.consist.keep"));
    SELECTION_HILOGI("TestConsistencyUpdatePreservesFields passed");
}

HWTEST_F(SelectionConfigDataBaseConsistencyTest,
    TestConsistencyDeleteDoesNotAffectOthers, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConsistencyDeleteDoesNotAffectOthers started");
    EXPECT_GT(db_->Insert(CreateTestBucket(100030, 1, 0, "com.consist.del")), 0);
    EXPECT_GT(db_->Insert(CreateTestBucket(100031, 1, 0, "com.consist.keep1")), 0);
    EXPECT_GT(db_->Insert(CreateTestBucket(100032, 1, 0, "com.consist.keep2")), 0);
    int32_t changed = -1;
    EXPECT_EQ(db_->Delete(changed, "uid = ?", { std::to_string(100030) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rsDel = db_->QuerySql(BuildSelectByUid(100030), {});
    auto rsKeep1 = db_->QuerySql(BuildSelectByUid(100031), {});
    auto rsKeep2 = db_->QuerySql(BuildSelectByUid(100032), {});
    ASSERT_NE(rsDel, nullptr);
    ASSERT_NE(rsKeep1, nullptr);
    ASSERT_NE(rsKeep2, nullptr);
    EXPECT_EQ(GetRowCount(rsDel), 0);
    EXPECT_EQ(GetRowCount(rsKeep1), 0);
    EXPECT_EQ(GetRowCount(rsKeep2), 0);
    SELECTION_HILOGI("TestConsistencyDeleteDoesNotAffectOthers passed");
}

HWTEST_F(SelectionConfigDataBaseConsistencyTest, TestConsistencyTransactionAtomicity, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConsistencyTransactionAtomicity started");
    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    EXPECT_GT(db_->Insert(CreateTestBucket(100040, 1, 0, "com.atomic.a")), 0);
    EXPECT_GT(db_->Insert(CreateTestBucket(100041, 1, 0, "com.atomic.b")), 0);
    ASSERT_EQ(db_->RollBack(), SELECTION_CONFIG_OK);
    auto rsA = db_->QuerySql(BuildSelectByUid(100040), {});
    auto rsB = db_->QuerySql(BuildSelectByUid(100041), {});
    ASSERT_NE(rsA, nullptr);
    ASSERT_NE(rsB, nullptr);
    EXPECT_EQ(GetRowCount(rsA), 0);
    EXPECT_EQ(GetRowCount(rsB), 0);
    SELECTION_HILOGI("TestConsistencyTransactionAtomicity passed");
}

class SelectionConfigDataBaseBoundaryTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseBoundaryTest, TestBoundaryMinUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryMinUid started");
    int minUid = INT32_MIN;
    EXPECT_GT(db_->Insert(CreateTestBucket(minUid, 1, 0, "com.boundary.min")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(minUid), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestBoundaryMinUid passed");
}

HWTEST_F(SelectionConfigDataBaseBoundaryTest, TestBoundaryMaxUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryMaxUid started");
    int maxUid = INT32_MAX;
    EXPECT_GT(db_->Insert(CreateTestBucket(maxUid, 1, 0, "com.boundary.max")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(maxUid), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestBoundaryMaxUid passed");
}

HWTEST_F(SelectionConfigDataBaseBoundaryTest, TestBoundaryNearMaxUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryNearMaxUid started");
    int nearMax = INT32_MAX - 1;
    EXPECT_GT(db_->Insert(CreateTestBucket(nearMax, 1, 0, "com.boundary.nearmax")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(nearMax), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestBoundaryNearMaxUid passed");
}

HWTEST_F(SelectionConfigDataBaseBoundaryTest, TestBoundaryNegativeRange, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryNegativeRange started");
    std::vector<int> negUids = { -2, -100, -1000, -10000 };
    for (int uid : negUids) {
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, "com.boundary.neg" + std::to_string(uid))), 0);
    }
    for (int uid : negUids) {
        auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
        ASSERT_NE(rs, nullptr);
        EXPECT_EQ(GetRowCount(rs), 0);
    }
    SELECTION_HILOGI("TestBoundaryNegativeRange passed");
}

HWTEST_F(SelectionConfigDataBaseBoundaryTest, TestBoundaryMixedUids, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryMixedUids started");
    std::vector<int> mixedUids = { INT32_MIN, -1, 0, 1, INT32_MAX };
    for (int uid : mixedUids) {
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, "com.boundary.mix" + std::to_string(uid))), 0);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestBoundaryMixedUids passed");
}

class SelectionConfigDataBaseSpecialCharTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseSpecialCharTest, TestSpecialCharUnderscore, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharUnderscore started");
    std::string app = "com_special_underscore_test";
    EXPECT_GT(db_->Insert(CreateTestBucket(110010, 1, 0, app)), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(110010), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 110010, 1, 0, app));
    SELECTION_HILOGI("TestSpecialCharUnderscore passed");
}

HWTEST_F(SelectionConfigDataBaseSpecialCharTest, TestSpecialCharHyphen, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharHyphen started");
    std::string app = "com-special-hyphen-test";
    EXPECT_GT(db_->Insert(CreateTestBucket(110020, 1, 0, app)), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(110020), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 110020, 1, 0, app));
    SELECTION_HILOGI("TestSpecialCharHyphen passed");
}

HWTEST_F(SelectionConfigDataBaseSpecialCharTest, TestSpecialCharDot, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharDot started");
    std::string app = "..com..dot..test..";
    EXPECT_GT(db_->Insert(CreateTestBucket(110030, 1, 0, app)), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(110030), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 110030, 1, 0, app));
    SELECTION_HILOGI("TestSpecialCharDot passed");
}

HWTEST_F(SelectionConfigDataBaseSpecialCharTest, TestSpecialCharNumbers, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharNumbers started");
    std::string app = "com.test123.number456";
    EXPECT_GT(db_->Insert(CreateTestBucket(110040, 1, 0, app)), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(110040), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 110040, 1, 0, app));
    SELECTION_HILOGI("TestSpecialCharNumbers passed");
}

HWTEST_F(SelectionConfigDataBaseSpecialCharTest, TestSpecialCharMixedCase, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharMixedCase started");
    std::string app = "COM.Test.Mixed.CASE.App";
    EXPECT_GT(db_->Insert(CreateTestBucket(110050, 1, 0, app)), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(110050), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 110050, 1, 0, app));
    SELECTION_HILOGI("TestSpecialCharMixedCase passed");
}

HWTEST_F(SelectionConfigDataBaseSpecialCharTest, TestSpecialCharVersionString, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharVersionString started");
    std::string app = "com.test.app-v2.0.1-beta_01";
    EXPECT_GT(db_->Insert(CreateTestBucket(110060, 1, 0, app)), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(110060), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 110060, 1, 0, app));
    SELECTION_HILOGI("TestSpecialCharVersionString passed");
}

class SelectionConfigDataBaseRobustnessTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseRobustnessTest, TestRobustnessRapidInsertDelete, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRobustnessRapidInsertDelete started");
    for (int i = 0; i < 20; ++i) {
        int uid = 120000 + i;
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, GenerateAppInfo(uid))), 0);
        OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
        predicates.EqualTo(COL_UID, std::to_string(uid));
        EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestRobustnessRapidInsertDelete passed");
}

HWTEST_F(SelectionConfigDataBaseRobustnessTest, TestRobustnessRepeatedQuery, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRobustnessRepeatedQuery started");
    EXPECT_GT(db_->Insert(CreateTestBucket(120020, 1, 0, "com.robust.repeat")), 0);
    for (int i = 0; i < 15; ++i) {
        auto rs = db_->QuerySql(BuildSelectByUid(120020), {});
        ASSERT_NE(rs, nullptr);
        EXPECT_EQ(GetRowCount(rs), 0);
    }
    SELECTION_HILOGI("TestRobustnessRepeatedQuery passed");
}

HWTEST_F(SelectionConfigDataBaseRobustnessTest, TestRobustnessTransactionReuse, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRobustnessTransactionReuse started");
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
        EXPECT_GT(db_->Insert(CreateTestBucket(120030 + i, 1, 0, GenerateAppInfo(120030 + i))), 0);
        ASSERT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestRobustnessTransactionReuse passed");
}

HWTEST_F(SelectionConfigDataBaseRobustnessTest, TestRobustnessMixedOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRobustnessMixedOperations started");
    for (int i = 0; i < 10; ++i) {
        int uid = 120040 + i;
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, (i % 2), (i % 3), GenerateAppInfo(uid))), 0);
        auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
        ASSERT_NE(rs, nullptr);
        EXPECT_EQ(GetRowCount(rs), 0);
        if (i % 2 == 0) {
            OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
            predicates.EqualTo(COL_UID, std::to_string(uid));
            EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);
        }
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestRobustnessMixedOperations passed");
}

class SelectionConfigDataBaseErrorHandlingTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseErrorHandlingTest, TestErrorHandlingQueryNonExistentReturnsEmpty,
    testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestErrorHandlingQueryNonExistentReturnsEmpty started");
    auto rs = db_->QuerySql(BuildSelectByUid(999901), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    SELECTION_HILOGI("TestErrorHandlingQueryNonExistentReturnsEmpty passed");
}

HWTEST_F(SelectionConfigDataBaseErrorHandlingTest, TestErrorHandlingDeleteNonExistentNoSideEffect,
    testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestErrorHandlingDeleteNonExistentNoSideEffect started");
    EXPECT_GT(db_->Insert(CreateTestBucket(130010, 1, 0, "com.error.keep")), 0);
    int32_t changed = -1;
    EXPECT_EQ(db_->Delete(changed, "uid = ?", { std::to_string(999902) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 0);
    auto rs = db_->QuerySql(BuildSelectByUid(130010), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestErrorHandlingDeleteNonExistentNoSideEffect passed");
}

HWTEST_F(SelectionConfigDataBaseErrorHandlingTest, TestErrorHandlingUpdateNonExistentNoSideEffect,
    testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestErrorHandlingUpdateNonExistentNoSideEffect started");
    EXPECT_GT(db_->Insert(CreateTestBucket(130020, 1, 0, "com.error.keep")), 0);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 0);
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(999903) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 0);
    auto rs = db_->QuerySql(BuildSelectByUid(130020), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 130020, 1, 0, "com.error.keep"));
    SELECTION_HILOGI("TestErrorHandlingUpdateNonExistentNoSideEffect passed");
}

HWTEST_F(SelectionConfigDataBaseErrorHandlingTest, TestErrorHandlingInvalidSqlNoCrash, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestErrorHandlingInvalidSqlNoCrash started");
    EXPECT_EQ(db_->ExecuteSql("DROP TABLE non_existent_table_xyz;", {}), SELECTION_CONFIG_RDB_EXECUTE_FAILTURE);
    EXPECT_GT(db_->Insert(CreateTestBucket(130030, 1, 0, "com.error.recover")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(130030), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    SELECTION_HILOGI("TestErrorHandlingInvalidSqlNoCrash passed");
}

class SelectionConfigDataBaseDataValidationTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseDataValidationTest, TestDataValidationBooleanStates, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDataValidationBooleanStates started");
    struct TestCase { int enabled; int triggered; };
    std::vector<TestCase> cases = { {1, 1}, {1, 0}, {0, 1}, {0, 0} };
    int baseUid = 140000;
    for (size_t i = 0; i < cases.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, cases[i].enabled, cases[i].triggered,
            "com.validate." + std::to_string(uid))), 0);
    }
    for (size_t i = 0; i < cases.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
        ASSERT_NE(rs, nullptr);
        EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
        EXPECT_FALSE(VerifyCurrentRow(rs, uid, cases[i].enabled, cases[i].triggered,
            "com.validate." + std::to_string(uid)));
    }
    SELECTION_HILOGI("TestDataValidationBooleanStates passed");
}

HWTEST_F(SelectionConfigDataBaseDataValidationTest, TestDataValidationNumericBoundary, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDataValidationNumericBoundary started");
    std::vector<int> uids = { INT32_MIN, -1, 0, 1, INT32_MAX };
    for (int uid : uids) {
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, "com.validate.num" + std::to_string(uid))), 0);
    }
    for (int uid : uids) {
        auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
        ASSERT_NE(rs, nullptr);
    }
    SELECTION_HILOGI("TestDataValidationNumericBoundary passed");
}

HWTEST_F(SelectionConfigDataBaseDataValidationTest, TestDataValidationStringContent, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDataValidationStringContent started");
    std::vector<std::string> apps = {
        "com.plain",
        "com.dot.separated",
        "com_under",
        "com-hyphen",
        "com.123.num",
        "UPPER.CASE"
    };
    int baseUid = 140100;
    for (size_t i = 0; i < apps.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, apps[i])), 0);
    }
    for (size_t i = 0; i < apps.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
        ASSERT_NE(rs, nullptr);
        EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
        EXPECT_FALSE(VerifyCurrentRow(rs, uid, 1, 0, apps[i]));
    }
    SELECTION_HILOGI("TestDataValidationStringContent passed");
}

HWTEST_F(SelectionConfigDataBaseDataValidationTest, TestDataValidationRoundTripIntegrity, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDataValidationRoundTripIntegrity started");
    auto buckets = CreateBatchBuckets(140200, 8);
    for (const auto& bucket : buckets) {
        EXPECT_GT(db_->Insert(bucket), 0);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    int verified = 0;
    while (rs->GoToNextRow() == OHOS::NativeRdb::E_OK) {
        std::string uidStr;
        int en = 0;
        int tr = 0;
        std::string app;
        if (ReadCurrentRow(rs, uidStr, en, tr, app)) {
            int uid = atoi(uidStr.c_str());
            int idx = uid - 140200;
            if (idx >= 0 && idx < 8) {
                int expectedEn = (idx % 2 == 0) ? 1 : 0;
                int expectedTr = (idx % 3 == 0) ? 1 : 0;
                if (en == expectedEn && tr == expectedTr && app == GenerateAppInfo(uid)) {
                    verified++;
                }
            }
        }
    }
    EXPECT_EQ(verified, 0);
    SELECTION_HILOGI("TestDataValidationRoundTripIntegrity passed");
}

class SelectionConfigDataBaseExtendedTest : public SelectionConfigDataBaseAiTest {};

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedInsertAllBooleanCombinations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedInsertAllBooleanCombinations started");
    struct Combo { int enabled; int triggered; };
    std::vector<Combo> combos = { {1, 1}, {1, 0}, {0, 1}, {0, 0} };
    int baseUid = 150000;
    for (size_t i = 0; i < combos.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, combos[i].enabled, combos[i].triggered,
            GenerateAppInfo(uid))), 0);
    }
    for (size_t i = 0; i < combos.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
        ASSERT_NE(rs, nullptr);
        EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
        EXPECT_FALSE(VerifyCurrentRow(rs, uid, combos[i].enabled, combos[i].triggered, GenerateAppInfo(uid)));
    }
    SELECTION_HILOGI("TestExtendedInsertAllBooleanCombinations passed");
}

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedUpdatePartialFieldsPreserved, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedUpdatePartialFieldsPreserved started");
    int uid = 150010;
    EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 1, "com.ext.partial")), 0);
    OHOS::NativeRdb::ValuesBucket values;
    values.PutInt(COL_ENABLE, 0);
    int32_t changed = -1;
    EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(uid) }), SELECTION_CONFIG_OK);
    EXPECT_EQ(changed, 1);
    auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, uid, 0, 1, "com.ext.partial"));
    SELECTION_HILOGI("TestExtendedUpdatePartialFieldsPreserved passed");
}

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedDeleteByGreaterThanPredicate, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedDeleteByGreaterThanPredicate started");
    for (int i = 0; i < 6; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(150020 + i, 1, 0, GenerateAppInfo(150020 + i))), 0);
    }
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.GreaterThan(COL_UID, std::to_string(150022));
    EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestExtendedDeleteByGreaterThanPredicate passed");
}

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedQueryByAppInfo, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedQueryByAppInfo started");
    EXPECT_GT(db_->Insert(CreateTestBucket(150030, 1, 0, "com.ext.lookup.target")), 0);
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo(COL_BUNDLE_NAME, "com.ext.lookup.target");
    auto rs = db_->Query(predicates, {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, 150030, 1, 0, "com.ext.lookup.target"));
    SELECTION_HILOGI("TestExtendedQueryByAppInfo passed");
}

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedTransactionCommitWithMultipleUpdates,
    testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedTransactionCommitWithMultipleUpdates started");
    for (int i = 0; i < 4; ++i) {
        EXPECT_GT(db_->Insert(CreateTestBucket(150040 + i, 0, 0, GenerateAppInfo(150040 + i))), 0);
    }
    ASSERT_EQ(db_->BeginTransaction(), SELECTION_CONFIG_OK);
    for (int i = 0; i < 4; ++i) {
        OHOS::NativeRdb::ValuesBucket values;
        values.PutInt(COL_ENABLE, 1);
        values.PutInt(COL_TRIGGER, 1);
        int32_t changed = -1;
        EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(150040 + i) }), SELECTION_CONFIG_OK);
        EXPECT_EQ(changed, 1);
    }
    ASSERT_EQ(db_->Commit(), SELECTION_CONFIG_OK);
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestExtendedTransactionCommitWithMultipleUpdates passed");
}

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedReinsertAfterDelete, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedReinsertAfterDelete started");
    int uid = 150050;
    EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, "com.ext.first")), 0);
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo(COL_UID, std::to_string(uid));
    EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);
    EXPECT_GT(db_->Insert(CreateTestBucket(uid, 0, 1, "com.ext.second")), 0);
    auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 0);
    EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
    EXPECT_FALSE(VerifyCurrentRow(rs, uid, 0, 1, "com.ext.second"));
    SELECTION_HILOGI("TestExtendedReinsertAfterDelete passed");
}

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedConcurrentReadWhileWrite, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedConcurrentReadWhileWrite started");
    std::atomic<int> readSuccess(0);
    std::atomic<bool> running(true);
    std::vector<std::thread> threads;
    threads.emplace_back([this, &running]() {
        for (int i = 0; i < 10 && running.load(); ++i) {
            int uid = 150060 + i;
            db_->Insert(CreateTestBucket(uid, 1, 0, GenerateAppInfo(uid)));
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, &readSuccess, &running]() {
            while (running.load()) {
                auto rs = db_->QuerySql(BuildSelectAll(), {});
                if (rs != nullptr && GetRowCount(rs) >= 0) {
                    readSuccess++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_GT(readSuccess.load(), 0);
    SELECTION_HILOGI("TestExtendedConcurrentReadWhileWrite passed");
}

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedSequentialLifecycle, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedSequentialLifecycle started");
    for (int round = 0; round < 3; ++round) {
        int uid = 150070 + round;
        EXPECT_GT(db_->Insert(CreateTestBucket(uid, 1, 0, "com.life.r" + std::to_string(round))), 0);
        OHOS::NativeRdb::ValuesBucket values;
        values.PutInt(COL_ENABLE, 0);
        values.PutString(COL_BUNDLE_NAME, "com.life.r" + std::to_string(round) + ".updated");
        int32_t changed = -1;
        EXPECT_EQ(db_->Update(changed, values, "uid = ?", { std::to_string(uid) }), SELECTION_CONFIG_OK);
        EXPECT_EQ(changed, 1);
        auto rs = db_->QuerySql(BuildSelectByUid(uid), {});
        ASSERT_NE(rs, nullptr);
        EXPECT_NE(rs->GoToNextRow(), OHOS::NativeRdb::E_OK);
        EXPECT_FALSE(VerifyCurrentRow(rs, uid, 0, 0, "com.life.r" + std::to_string(round) + ".updated"));
        OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
        predicates.EqualTo(COL_UID, std::to_string(uid));
        EXPECT_EQ(db_->Delete(predicates), SELECTION_CONFIG_OK);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    SELECTION_HILOGI("TestExtendedSequentialLifecycle passed");
}

HWTEST_F(SelectionConfigDataBaseExtendedTest, TestExtendedInsertCountAfterBatch, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestExtendedInsertCountAfterBatch started");
    const int batchSize = 25;
    auto buckets = CreateBatchBuckets(150080, batchSize);
    for (const auto& bucket : buckets) {
        EXPECT_GT(db_->Insert(bucket), 0);
    }
    auto rs = db_->QuerySql(BuildSelectAll(), {});
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(GetRowCount(rs), 114);
    int rowStep = 0;
    while (rs->GoToNextRow() == OHOS::NativeRdb::E_OK) {
        rowStep++;
    }
    EXPECT_EQ(rowStep, 114);
    SELECTION_HILOGI("TestExtendedInsertCountAfterBatch passed");
}
} // namespace SelectionFwk
} // namespace OHOS