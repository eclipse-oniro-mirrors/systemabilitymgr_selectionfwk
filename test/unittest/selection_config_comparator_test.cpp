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

void SetSelectionConfig(SelectionConfig& selectionConfig, int uid, bool isEnabled, const std::string &appInfo)
{
    selectionConfig.SetUid(uid);
    selectionConfig.SetEnabled(isEnabled);
    selectionConfig.SetApplicationInfo(appInfo);
}

/**
 * @tc.name: SelectionConfigComparator001
 * @tc.desc: database testcase 001
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator001, TestSize.Level0)
{
    int uid = 100;
    SelectionConfig sysSelectionConfig;
    std::optional<SelectionConfig> dbSelectionConfig = std::nullopt;

    std::string defaultAppInfo("a/b");
    SelectionConfig defaultSelectionConfig;
    SetSelectionConfig(defaultSelectionConfig, 0, true, defaultAppInfo);
    auto& comparator = SelectionConfigComparator::GetInstance();
    comparator.Init();
    comparator.Init(defaultSelectionConfig);
    auto result = comparator.Compare(uid, sysSelectionConfig, dbSelectionConfig);

    ASSERT_TRUE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "a/b");
}

/**
 * @tc.name: SelectionConfigComparator002
 * @tc.desc: database testcase 002
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator002, TestSize.Level0)
{
    int uid = 100;
    std::string appInfo("a/b");
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true, appInfo);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    auto result = SelectionConfigComparator::GetInstance().Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromDbToSys);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "a/b");
}

/**
 * @tc.name: SelectionConfigComparator003
 * @tc.desc: database testcase 003
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator003, TestSize.Level0)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true, "a/b");
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, false, "c/d");
    auto result = SelectionConfigComparator::GetInstance().Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_TRUE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_FALSE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "c/d");
}

/**
 * @tc.name: SelectionConfigComparator004
 * @tc.desc: database testcase 004
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator004, TestSize.Level0)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true, "a/b");
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, true, "c/d");
    auto result = SelectionConfigComparator::GetInstance().Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "c/d");
}

/**
 * @tc.name: SelectionConfigComparator005
 * @tc.desc: database testcase 005
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator005, TestSize.Level0)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, false, "a/b");
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    auto result = SelectionConfigComparator::GetInstance().Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_TRUE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromDbToSys);
    ASSERT_FALSE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "a/b");
}

/**
 * @tc.name: SelectionConfigComparator006
 * @tc.desc: database testcase 006
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator006, TestSize.Level0)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, false, "a/b");
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, false, "a/b");
    auto result = SelectionConfigComparator::GetInstance().Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_TRUE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_FALSE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "a/b");
}

/**
 * @tc.name: SelectionConfigComparator007
 * @tc.desc: database testcase 007
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator007, TestSize.Level0)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, false, "a/b");
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, true, "c/d");
    auto result = SelectionConfigComparator::GetInstance().Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "c/d");
}

/**
 * @tc.name: SelectionConfigComparator008
 * @tc.desc: database testcase 008
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator008, TestSize.Level0)
{
    int uid = 100;
    std::string appInfo = "com.example.test/SelectionExtensionAbility";
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true, appInfo);
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, -1, false, appInfo);
    auto result = SelectionConfigComparator::GetInstance().Compare(uid, sysSelectionConfig, dbSelectionConfigOpt);

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_TRUE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromDbToSys);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), appInfo);
}

/**
 * @tc.name: SelectionConfigComparator009
 * @tc.desc: database testcase 009
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator009, TestSize.Level0)
{
    int uid = 100;
    int loginUid = 101;
    std::string appInfo = "com.example.test/SelectionExtensionAbility";
    SelectionConfig defaultSelectionConfig;
    SetSelectionConfig(defaultSelectionConfig, 0, true, appInfo);
    auto& comparator = SelectionConfigComparator::GetInstance();
    comparator.Init(defaultSelectionConfig);

    std::optional<SelectionConfig> dbSelectionConfigOpt = std::nullopt;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, false, appInfo);
    auto result = SelectionConfigComparator::GetInstance().Compare(loginUid, sysSelectionConfig, dbSelectionConfigOpt);

    ASSERT_TRUE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_TRUE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::NONE);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 101);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), appInfo);
}

/**
 * @tc.name: SelectionConfigComparator010
 * @tc.desc: database testcase 010
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator010, TestSize.Level0)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true, "a/b");
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, true, "a/b");

    AbilityRuntimeInfo abilityRuntimeInfo;
    abilityRuntimeInfo.userId = uid;
    abilityRuntimeInfo.bundleName = "com.example.test/SelectionExtensionAbility";
    abilityRuntimeInfo.abilityName = "appAbility";
    std::optional<AbilityRuntimeInfo> abilityRuntimeInfoOpt = abilityRuntimeInfo;

    auto result = SelectionConfigComparator::GetInstance().Compare(
        uid,
        sysSelectionConfig,
        dbSelectionConfigOpt,
        abilityRuntimeInfoOpt
    );

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStart);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "a/b");
}

/**
 * @tc.name: SelectionConfigComparator011
 * @tc.desc: database testcase 011
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigComparatorTest, SelectionConfigComparator011, TestSize.Level0)
{
    int uid = 100;
    SelectionConfig dbSelectionConfig;
    SetSelectionConfig(dbSelectionConfig, uid, true, "a/b");
    std::optional<SelectionConfig> dbSelectionConfigOpt = dbSelectionConfig;
    SelectionConfig sysSelectionConfig;
    SetSelectionConfig(sysSelectionConfig, uid, true, "a/b");

    std::optional<AbilityRuntimeInfo> abilityRuntimeInfoOpt = std::nullopt;

    auto result = SelectionConfigComparator::GetInstance().Compare(
        uid,
        sysSelectionConfig,
        dbSelectionConfigOpt,
        abilityRuntimeInfoOpt
    );

    ASSERT_FALSE(result.shouldCreate);
    ASSERT_FALSE(result.shouldStart);
    ASSERT_FALSE(result.shouldStop);
    ASSERT_FALSE(result.shouldRestartApp);
    ASSERT_EQ(result.direction, SyncDirection::FromSysToDb);
    ASSERT_TRUE(result.selectionConfig.GetEnable());
    ASSERT_EQ(result.selectionConfig.GetUid(), 100);
    ASSERT_EQ(result.selectionConfig.GetApplicationInfo(), "a/b");
}
}
}