/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <optional>

#include "gtest/gtest.h"

#include "selection_config_database.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

constexpr const char *SELECTION_TEST_TABLE_NAME = "selection_test";

class SelectionConfigDatabaseTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SelectionConfigDatabaseTest::SetUpTestCase()
{
    std::cout << "SelectionConfigDatabaseTest SetUpTestCase" << std::endl;
}

void SelectionConfigDatabaseTest::TearDownTestCase()
{
    std::cout << "SelectionConfigDatabaseTest TearDownTestCase" << std::endl;
}

void SelectionConfigDatabaseTest::SetUp()
{
    std::cout << "SelectionConfigDatabaseTest SetUp" << std::endl;
}

void SelectionConfigDatabaseTest::TearDown()
{
    std::cout << "SelectionConfigDatabaseTest TearDown" << std::endl;
}

/**
 * @tc.name: SelectionConfigDatabase001
 * @tc.desc: test SelectionConfigDataBase::GetInstance
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase001, TestSize.Level0)
{
    auto obj = SelectionConfigDataBase::GetInstance();
    ASSERT_NE(obj, nullptr);
}

/**
 * @tc.name: SelectionConfigDatabase002
 * @tc.desc: test insert values to database
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase002, TestSize.Level0)
{
    OHOS::NativeRdb::ValuesBucket values;
    values.Clear();
    values.PutInt("uid", 1000);
    values.PutInt("enable", false);
    values.PutInt("trigger", true);
    values.PutString("bundleName", "a/b");
    int ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_EQ(ret, 0);

    int number = SelectionConfigDataBase::GetInstance()->Insert(values);
    ASSERT_GE(number, 1);

    ret = SelectionConfigDataBase::GetInstance()->Commit();
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase003
 * @tc.desc: test insert values to database repetitively
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase003, TestSize.Level0)
{
    OHOS::NativeRdb::ValuesBucket values;
    values.Clear();
    values.PutInt("uid", 1000);
    values.PutInt("enable", false);
    values.PutInt("trigger", true);
    values.PutString("bundleName", "a/b");
    int ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Insert(values);
    ASSERT_EQ(ret, -2);
    SelectionConfigDataBase::GetInstance()->RollBack();
}

/**
 * @tc.name: SelectionConfigDatabase004
 * @tc.desc: test update values to database
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase004, TestSize.Level0)
{
    int changedRows;
    OHOS::NativeRdb::ValuesBucket values;
    values.Clear();
    values.PutInt("uid", 1000);
    values.PutInt("enable", true);
    values.PutInt("trigger", true);
    values.PutString("bundleName", "a/b");
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo("uid", "1000");
    int ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Update(changedRows, values, predicates);
    ASSERT_EQ(ret, 0);

    std::string whereClause = "uid = ?";
    std::vector<std::string> whereArgs = {"1000"};
    ret = SelectionConfigDataBase::GetInstance()->Update(changedRows, values, whereClause, whereArgs);
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Commit();
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase005
 * @tc.desc: test query values from database
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase005, TestSize.Level0)
{
    std::vector<std::string> columns;
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo("uid", "1000");
    int ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_EQ(ret, 0);

    auto resultSet = SelectionConfigDataBase::GetInstance()->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);

    ret = SelectionConfigDataBase::GetInstance()->Commit();
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase006
 * @tc.desc: test delete values from database
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase006, TestSize.Level0)
{
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo("uid", "1000");
    int ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Delete(predicates);
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Commit();
    ASSERT_EQ(ret, 0);

    int changedRows;
    std::string whereClause = "uid = ?";
    std::vector<std::string> whereArgs = {"1000"};
    ret = SelectionConfigDataBase::GetInstance()->Delete(changedRows, whereClause, whereArgs);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase007
 * @tc.desc: test update nonexistent line from database
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase007, TestSize.Level0)
{
    int changedRows;
    OHOS::NativeRdb::ValuesBucket values;
    values.Clear();
    values.PutInt("uid", 1001);
    values.PutInt("enable", true);
    values.PutInt("trigger", true);
    values.PutString("bundleName", "a/b");
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_TEST_TABLE_NAME);
    predicates.EqualTo("uid", "1001");
    int ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Update(changedRows, values, predicates);
    ASSERT_NE(ret, 0);

    std::string whereClause = "uid = ?";
    std::vector<std::string> whereArgs = {"1001"};
    ret = SelectionConfigDataBase::GetInstance()->Update(changedRows, values, whereClause, whereArgs);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase008
 * @tc.desc: test delete nonexistent line from database
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase008, TestSize.Level0)
{
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_TEST_TABLE_NAME);
    predicates.EqualTo("uid", "1010");
    int ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Delete(predicates);
    ASSERT_NE(ret, 0);

    int changedRows;
    std::string whereClause = "uid = ?";
    std::vector<std::string> whereArgs = {"1010"};
    ret = SelectionConfigDataBase::GetInstance()->Delete(changedRows, whereClause, whereArgs);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase009
 * @tc.desc: test ExecuteSql and QuerySql
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase009, TestSize.Level0)
{
    std::string sql = "SELECT * FROM selection_config";
    int ret = SelectionConfigDataBase::GetInstance()->ExecuteSql(sql, {});
    ASSERT_EQ(ret, 0);
    auto resultSet = SelectionConfigDataBase::GetInstance()->QuerySql(sql, {});
    ASSERT_NE(resultSet, nullptr);
}

/**
 * @tc.name: SelectionConfigDatabase010
 * @tc.desc: test call methods with nullptr store_
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase010, TestSize.Level0)
{
    auto store = SelectionConfigDataBase::GetInstance()->store_;
    SelectionConfigDataBase::GetInstance()->store_ = nullptr;
    int ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_NE(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Commit();
    ASSERT_NE(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->RollBack();
    ASSERT_NE(ret, 0);

    OHOS::NativeRdb::ValuesBucket values;
    ret = SelectionConfigDataBase::GetInstance()->Insert(values);
    ASSERT_NE(ret, 0);

    int changedRows;
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_TEST_TABLE_NAME);
    ret = SelectionConfigDataBase::GetInstance()->Update(changedRows, values, predicates);
    ASSERT_NE(ret, 0);
    std::string whereClause = "id = ?";
    std::vector<std::string> whereArgs = {"10"};
    ret = SelectionConfigDataBase::GetInstance()->Update(changedRows, values, whereClause, whereArgs);
    ASSERT_NE(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Delete(predicates);
    ASSERT_NE(ret, 0);
    ret = SelectionConfigDataBase::GetInstance()->Delete(changedRows, whereClause, whereArgs);
    ASSERT_NE(ret, 0);

    std::vector<std::string> columns;
    auto resultSet = SelectionConfigDataBase::GetInstance()->Query(predicates, columns);
    ASSERT_EQ(resultSet, nullptr);

    std::string sql = "SELECT * FROM selection_config";
    ret = SelectionConfigDataBase::GetInstance()->ExecuteSql(sql, {});
    ASSERT_NE(ret, 0);
    resultSet = SelectionConfigDataBase::GetInstance()->QuerySql(sql, {});
    ASSERT_EQ(resultSet, nullptr);
    SelectionConfigDataBase::GetInstance()->store_ = store;
}

/**
 * @tc.name: SelectionConfigDatabase011
 * @tc.desc: test OnCreate
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase011, TestSize.Level0)
{
    auto dbCallBack = new (std::nothrow) SelectionConfigDataBaseCallBack();
    std::string testDatabaseName = "/data/local/tmp/selection_test.db";
    int32_t errCode = OHOS::NativeRdb::E_OK;
    OHOS::NativeRdb::RdbStoreConfig config(testDatabaseName);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    SelectionConfigDataBaseCallBack sqliteOpenHelperCallback;
    std::shared_ptr<OHOS::NativeRdb::RdbStore> store =
    OHOS::NativeRdb::RdbHelper::GetRdbStore(config, 1, sqliteOpenHelperCallback, errCode);
    int32_t ret = dbCallBack->OnCreate(*(store));
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase012
 * @tc.desc: test OnUpgrade
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase012, TestSize.Level0)
{
    auto dbCallBack = new (std::nothrow) SelectionConfigDataBaseCallBack();
    std::string testDatabaseName = "/data/local/tmp/selection_test.db";
    int32_t errCode = OHOS::NativeRdb::E_OK;
    OHOS::NativeRdb::RdbStoreConfig config(testDatabaseName);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    SelectionConfigDataBaseCallBack sqliteOpenHelperCallback;
    std::shared_ptr<OHOS::NativeRdb::RdbStore> store =
    OHOS::NativeRdb::RdbHelper::GetRdbStore(config, 1, sqliteOpenHelperCallback, errCode);
    int32_t ret = dbCallBack->OnUpgrade(*(store), 2, 1);
    EXPECT_EQ(ret, 0);
    ret = dbCallBack->OnUpgrade(*(store), 1, 2);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase013
 * @tc.desc: test OnDowngrade
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase013, TestSize.Level0)
{
    auto dbCallBack = new (std::nothrow) SelectionConfigDataBaseCallBack();
    std::string testDatabaseName = "/data/local/tmp/selection_test.db";
    int32_t errCode = OHOS::NativeRdb::E_OK;
    OHOS::NativeRdb::RdbStoreConfig config(testDatabaseName);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    SelectionConfigDataBaseCallBack sqliteOpenHelperCallback;
    std::shared_ptr<OHOS::NativeRdb::RdbStore> store =
    OHOS::NativeRdb::RdbHelper::GetRdbStore(config, 1, sqliteOpenHelperCallback, errCode);
    int32_t ret = dbCallBack->OnDowngrade(*(store), 2, 1);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionConfigDatabase014
 * @tc.desc: test failed to call methods
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigDatabaseTest, SelectionConfigDatabase014, TestSize.Level0)
{
    std::string testDatabaseName = "/data/local/tmp/selection_test.db";
    int32_t errCode = OHOS::NativeRdb::E_OK;
    OHOS::NativeRdb::RdbStoreConfig config(testDatabaseName);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S1);
    SelectionConfigDataBaseCallBack sqliteOpenHelperCallback;
    std::shared_ptr<OHOS::NativeRdb::RdbStore> store =
    OHOS::NativeRdb::RdbHelper::GetRdbStore(config, 1, sqliteOpenHelperCallback, errCode);

    auto reginStore = SelectionConfigDataBase::GetInstance()->store_;
    SelectionConfigDataBase::GetInstance()->store_ = store;

    int ret = OHOS::NativeRdb::RdbHelper::DeleteRdbStore(testDatabaseName);
    EXPECT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    EXPECT_NE(ret, 0);
    ret = SelectionConfigDataBase::GetInstance()->Commit();
    EXPECT_NE(ret, 0);
    ret = SelectionConfigDataBase::GetInstance()->RollBack();
    EXPECT_NE(ret, 0);

    int changedRows;
    OHOS::NativeRdb::ValuesBucket values;
    std::string whereClause = "id = ?";
    std::vector<std::string> whereArgs = {"10"};
    ret = SelectionConfigDataBase::GetInstance()->Update(changedRows, values, whereClause, whereArgs);
    EXPECT_NE(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Delete(changedRows, whereClause, whereArgs);
    EXPECT_NE(ret, 0);

    std::string sql = "SELECT * FROM selection_config";
    ret = SelectionConfigDataBase::GetInstance()->ExecuteSql(sql, {});
    EXPECT_NE(ret, 0);

    auto dbCallBack = new (std::nothrow) SelectionConfigDataBaseCallBack();
    ret = dbCallBack->OnCreate(*(store));
    EXPECT_NE(ret, 0);
    ret = dbCallBack->OnUpgrade(*(store), 1, 2);
    EXPECT_NE(ret, 0);
    SelectionConfigDataBase::GetInstance()->store_ = reginStore;
}
}
}