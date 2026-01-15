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

#include "selection_config.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

class SelectionConfigTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SelectionConfigTest::SetUpTestCase()
{
    std::cout << "SelectionConfigTest SetUpTestCase" << std::endl;
}

void SelectionConfigTest::TearDownTestCase()
{
    std::cout << "SelectionConfigTest TearDownTestCase" << std::endl;
}

void SelectionConfigTest::SetUp()
{
    std::cout << "SelectionConfigTest SetUp" << std::endl;
}

void SelectionConfigTest::TearDown()
{
    std::cout << "SelectionConfigTest TearDown" << std::endl;
}

/**
 * @tc.name: SelectionConfig001
 * @tc.desc: test the methods of MemSelectionConfig
 * @tc.type: FUNC
 */
HWTEST_F(SelectionConfigTest, SelectionConfig001, TestSize.Level0)
{
    SelectionConfig config;
    MemSelectionConfig::GetInstance().SetSelectionConfig(config);
    auto memConfig = MemSelectionConfig::GetInstance().GetSelectionConfig();
    ASSERT_EQ(memConfig.GetEnable(), false);

    MemSelectionConfig::GetInstance().SetApplicationInfo("a/b");
    ASSERT_EQ(MemSelectionConfig::GetInstance().GetApplicationInfo(), "a/b");

    MemSelectionConfig::GetInstance().SetApplicationInfo(
        "com.selection.selectionapplication/SelectionExtensionAbility");
    auto appInfo = MemSelectionConfig::GetInstance().GetApplicationInfo();
    ASSERT_EQ(appInfo, "com.selection.selectionapplication/SelectionExtensionAbility");

    MemSelectionConfig::GetInstance().SetEnabled(true);
    bool enabled = MemSelectionConfig::GetInstance().GetEnable();
    ASSERT_EQ(enabled, true);

    uint32_t timeout = 500;
    MemSelectionConfig::GetInstance().SetTimeout(timeout);
    ASSERT_EQ(MemSelectionConfig::GetInstance().GetTimeout(), 500);
}
}
}