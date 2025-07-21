/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define private public
#include "selection_config_comparator.h"
#include "selection_config.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

class SelectionConfigComparatorTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SelectionConfigComparatorTest::SetUpTestCase()
{
    std::cout << "SelectionConfigComparatorTest SetUpTestCase" << std::endl;
}

void SelectionConfigComparatorTest::TearDownTestCase()
{
    std::cout << "SelectionConfigComparatorTest TearDownTestCase" << std::endl;
}

void SelectionConfigComparatorTest::SetUp()
{
    std::cout << "SelectionConfigComparatorTest SetUp" << std::endl;
}

void SelectionConfigComparatorTest::TearDown()
{
    std::cout << "SelectionConfigComparatorTest TearDown" << std::endl;
}

void SetSelectionConfig(SelectionConfig& dbSelectionConfig, int uid, bool bEnable)
{
    dbSelectionConfig.SetUid(uid);
    dbSelectionConfig.SetEnabled(bEnable);
}

/**
 * @tc.name: SelectionConfigComparator001
 * @tc.desc: database testcase 001
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator001, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig sysSelectionConfig;
    std::optional<SelectionConfig> dbSelectionConfig = std::nullopt;
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfig);
    ASSERT_TRUE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
}

/**
 * @tc.name: SelectionConfigComparator002
 * @tc.desc: database testcase 002
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator002, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromDbToSys);
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_FALSE(result.shouldStop);
}

/**
 * @tc.name: SelectionConfigComparator003
 * @tc.desc: database testcase 003
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator003, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, false);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.shouldStop);
    ASSERT_FALSE(result.selectionConfig.GetEnable());
    ASSERT_FALSE(result.shouldRestartApp);
}

/**
 * @tc.name: SelectionConfigComparator004
 * @tc.desc: database testcase 004
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator004, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, true);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
}

/**
 * @tc.name: SelectionConfigComparator005
 * @tc.desc: database testcase 005
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator005, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, false);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_TRUE(result.shouldStop);
    ASSERT_EQ(result.direction, SyncDirection::FromDbToSys);
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
}

/**
 * @tc.name: SelectionConfigComparator006
 * @tc.desc: database testcase 006
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator006, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, false);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, false);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
}

/**
 * @tc.name: SelectionConfigComparator007
 * @tc.desc: database testcase 007
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator007, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, false);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, true);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
}

/**
 * @tc.name: SelectionConfigComparator008
 * @tc.desc: database testcase 008
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator008, TestSize.Level1)
{
    int uid = 100;
    std::string appInfo = "com.example.test/SelectionExtensionAbility";
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true);
    dbSelectionConfig.SetApplicationInfo(appInfo);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    sysSelectionConfig.SetApplicationInfo(appInfo);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromDbToSys);
    ASSERT_TRUE(result.shouldRestartApp);
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_FALSE(result.shouldStop);
}
}
}