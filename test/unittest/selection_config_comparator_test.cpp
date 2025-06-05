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
#include "screenlock_manager.h"
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

/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator001, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig sysSelectionConfig;
    std::optional<SelectionConfig> dbSelectionConfig = std::nullopt;
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfig);
    ASSERT_TRUE(result.shouldCreate);
}


/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator002, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    dbSelectionConfig.SetUid(100);
    dbSelectionConfig.SetEnabled(true);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromDbToSys);
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_TRUE(result.selectionConfig.IsEnabled());
}

/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator003, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    dbSelectionConfig.SetUid(100);
    dbSelectionConfig.SetEnabled(true);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    sysSelectionConfig.SetUid(100);
    sysSelectionConfig.SetEnabled(false);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.shouldStop);
    ASSERT_FALSE(result.selectionConfig.IsEnabled());
}

/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator004, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    dbSelectionConfig.SetUid(100);
    dbSelectionConfig.SetEnabled(true);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    sysSelectionConfig.SetUid(100);
    sysSelectionConfig.SetEnabled(true);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
}

/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator005, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    dbSelectionConfig.SetUid(100);
    dbSelectionConfig.SetEnabled(false);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_TRUE(result.shouldStop);
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
}

/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator006, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    dbSelectionConfig.SetUid(100);
    dbSelectionConfig.SetEnabled(false);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    sysSelectionConfig.SetUid(100);
    sysSelectionConfig.SetEnabled(false);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.shouldStop);
}

/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator007, TestSize.Level1)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    dbSelectionConfig.SetUid(100);
    dbSelectionConfig.SetEnabled(false);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    sysSelectionConfig.SetUid(100);
    sysSelectionConfig.SetEnabled(true);
    auto result = SelectionConfigComparator::Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.selectionConfig.IsEnabled());
}
}
}