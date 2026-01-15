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
#include "gmock/gmock.h"

#include "db_selection_config_repository.h"
#include "selection_config_database.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;
using ::testing::Return;
using ::testing::Mock;
using namespace OHOS::NativeRdb;

class MockSelectionConfigDataBase : public SelectionConfigDataBase {
public:
    MOCK_METHOD(int32_t, BeginTransaction, (), (override));
    MOCK_METHOD(int32_t, Update, (int&, const ValuesBucket&, const RdbPredicates&), (override));
    MOCK_METHOD(int64_t, Insert, (const ValuesBucket&), (override));
    MOCK_METHOD(std::shared_ptr<OHOS::NativeRdb::ResultSet>, Query, (const OHOS::NativeRdb::AbsRdbPredicates &,
        const std::vector<std::string> &), (override));
    MOCK_METHOD(int32_t, Commit, (), (override));
};

class DbSelectionConfigRepositoryTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void DbSelectionConfigRepositoryTest::SetUpTestCase()
{
    std::cout << "DbSelectionConfigRepositoryTest SetUpTestCase" << std::endl;
}

void DbSelectionConfigRepositoryTest::TearDownTestCase()
{
    std::cout << "DbSelectionConfigRepositoryTest TearDownTestCase" << std::endl;
}

void DbSelectionConfigRepositoryTest::SetUp()
{
    std::cout << "DbSelectionConfigRepositoryTest SetUp" << std::endl;
}

void DbSelectionConfigRepositoryTest::TearDown()
{
    std::cout << "DbSelectionConfigRepositoryTest TearDown" << std::endl;
}

/**
 * @tc.name: DbSelectionConfigRepository001
 * @tc.desc: test DbSelectionConfigRepository::GetInstance()
 * @tc.type: FUNC
 */
HWTEST_F(DbSelectionConfigRepositoryTest, DbSelectionConfigRepository001, TestSize.Level0)
{
    auto obj = DbSelectionConfigRepository::GetInstance();
    ASSERT_NE(obj, nullptr);
}

/**
 * @tc.name: DbSelectionConfigRepository002
 * @tc.desc: test saving a new user to the database
 * @tc.type: FUNC
 */
HWTEST_F(DbSelectionConfigRepositoryTest, DbSelectionConfigRepository002, TestSize.Level0)
{
    int uid = 1010;
    SelectionConfig info;
    int ret = DbSelectionConfigRepository::GetInstance()->Save(uid, info);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: DbSelectionConfigRepository003
 * @tc.desc: test querying a record from the database based on the uid.
 * @tc.type: FUNC
 */
HWTEST_F(DbSelectionConfigRepositoryTest, DbSelectionConfigRepository003, TestSize.Level0)
{
    int uid = 1010;
    SelectionConfig info;
    int ret = DbSelectionConfigRepository::GetInstance()->Save(uid, info);
    ASSERT_EQ(ret, 0);
    auto selectionConfig = DbSelectionConfigRepository::GetInstance()->GetOneByUserId(uid);
    ASSERT_NE(selectionConfig, std::nullopt);
}

/**
 * @tc.name: DbSelectionConfigRepository004
 * @tc.desc: test querying a record from the database based on a nonexistent uid.
 * @tc.type: FUNC
 */
HWTEST_F(DbSelectionConfigRepositoryTest, DbSelectionConfigRepository004, TestSize.Level0)
{
    int uid = 101;
    auto selectionConfig = DbSelectionConfigRepository::GetInstance()->GetOneByUserId(uid);
    ASSERT_EQ(selectionConfig, std::nullopt);
}

/**
 * @tc.name: DbSelectionConfigRepository005
 * @tc.desc: test saving a new user to database
 * @tc.type: FUNC
 */
HWTEST_F(DbSelectionConfigRepositoryTest, DbSelectionConfigRepository005, TestSize.Level0)
{
    int uid = 1010;
    SelectionConfig info;
    int ret = DbSelectionConfigRepository::GetInstance()->Save(uid, info);
    ASSERT_EQ(ret, 0);

    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    predicates.EqualTo("uid", "1010");
    ret = SelectionConfigDataBase::GetInstance()->BeginTransaction();
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Delete(predicates);
    ASSERT_EQ(ret, 0);

    ret = SelectionConfigDataBase::GetInstance()->Commit();
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: DbSelectionConfigRepository006
 * @tc.desc: test methods with nullptr selectionDatabase_
 * @tc.type: FUNC
 */
HWTEST_F(DbSelectionConfigRepositoryTest, DbSelectionConfigRepository006, TestSize.Level0)
{
    auto reginSelectionDatabase = DbSelectionConfigRepository::GetInstance()->selectionDatabase_;
    DbSelectionConfigRepository::GetInstance()->selectionDatabase_ = nullptr;
    int uid = 1010;
    SelectionConfig info;
    int ret = DbSelectionConfigRepository::GetInstance()->Save(uid, info);
    ASSERT_NE(ret, 0);

    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    ret = DbSelectionConfigRepository::GetInstance()->GetConfigFromDatabase(predicates, {}, info);
    ASSERT_NE(ret, 0);

    std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = nullptr;
    DbSelectionConfigRepository::SelectionConfigTableInfo table;
    ret = DbSelectionConfigRepository::GetInstance()->RetrieveResultSetMetadata(resultSet, table);
    ASSERT_NE(ret, 0);
    DbSelectionConfigRepository::GetInstance()->selectionDatabase_ = reginSelectionDatabase;
}

/**
 * @tc.name: DbSelectionConfigRepository007
 * @tc.desc: test Save by mocking SelectionDatabase methods
 * @tc.type: FUNC
 */
HWTEST_F(DbSelectionConfigRepositoryTest, DbSelectionConfigRepository007, TestSize.Level0)
{
    auto reginSelectionDatabase = DbSelectionConfigRepository::GetInstance()->selectionDatabase_;
    std::shared_ptr<MockSelectionConfigDataBase> mockObj = std::make_shared<MockSelectionConfigDataBase>();
    EXPECT_CALL(*mockObj, BeginTransaction()).Times(1).WillOnce(Return(-1));
    DbSelectionConfigRepository::GetInstance()->selectionDatabase_ = mockObj;
    int uid = 1011;
    SelectionConfig info;
    int ret = DbSelectionConfigRepository::GetInstance()->Save(uid, info);
    ASSERT_NE(ret, 0);

    EXPECT_CALL(*mockObj, BeginTransaction()).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockObj, Update(testing::_, testing::_, testing::_)).WillRepeatedly(Return(-1));
    ret = DbSelectionConfigRepository::GetInstance()->Save(uid, info);
    ASSERT_NE(ret, 0);

    EXPECT_CALL(*mockObj, BeginTransaction()).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockObj, Update(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockObj, Insert(testing::_)).WillRepeatedly(Return(-1));
    ret = DbSelectionConfigRepository::GetInstance()->Save(uid, info);
    ASSERT_NE(ret, 0);

    EXPECT_CALL(*mockObj, BeginTransaction()).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockObj, Update(testing::_, testing::_, testing::_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockObj, Insert(testing::_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockObj, Commit()).WillRepeatedly(Return(-1));
    ret = DbSelectionConfigRepository::GetInstance()->Save(uid, info);
    ASSERT_NE(ret, 0);
    DbSelectionConfigRepository::GetInstance()->selectionDatabase_ = reginSelectionDatabase;
}

/**
 * @tc.name: DbSelectionConfigRepository008
 * @tc.desc: test GetConfigFromDatabase by mocking SelectionDatabase methods
 * @tc.type: FUNC
 */
HWTEST_F(DbSelectionConfigRepositoryTest, DbSelectionConfigRepository008, TestSize.Level0)
{
    auto reginSelectionDatabase = DbSelectionConfigRepository::GetInstance()->selectionDatabase_;
    std::shared_ptr<MockSelectionConfigDataBase> mockObj = std::make_shared<MockSelectionConfigDataBase>();
    EXPECT_CALL(*mockObj, BeginTransaction()).WillRepeatedly(Return(-1));
    DbSelectionConfigRepository::GetInstance()->selectionDatabase_ = mockObj;
    SelectionConfig info;
    OHOS::NativeRdb::RdbPredicates predicates(SELECTION_CONFIG_TABLE_NAME);
    int ret = DbSelectionConfigRepository::GetInstance()->GetConfigFromDatabase(predicates, {}, info);
    ASSERT_NE(ret, 0);

    EXPECT_CALL(*mockObj, BeginTransaction()).WillRepeatedly(Return(0));
    EXPECT_CALL(*mockObj, Query(testing::_, testing::_)).WillRepeatedly(Return(nullptr));
    ret = DbSelectionConfigRepository::GetInstance()->GetConfigFromDatabase(predicates, {}, info);
    ASSERT_NE(ret, 0);
    DbSelectionConfigRepository::GetInstance()->selectionDatabase_ = reginSelectionDatabase;
}
}
}