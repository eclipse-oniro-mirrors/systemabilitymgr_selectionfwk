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

#include "selection_common.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

class SelectionCommonTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SelectionCommonTest::SetUpTestCase()
{
    std::cout << "SelectionCommonTest SetUpTestCase" << std::endl;
}

void SelectionCommonTest::TearDownTestCase()
{
    std::cout << "SelectionCommonTest TearDownTestCase" << std::endl;
}

void SelectionCommonTest::SetUp()
{
    std::cout << "SelectionCommonTest SetUp" << std::endl;
}

void SelectionCommonTest::TearDown()
{
    std::cout << "SelectionCommonTest TearDown" << std::endl;
}

/**
 * @tc.name: SelectionCommon001
 * @tc.desc: test the IsAllWhitespace method with null characters
 * @tc.type: FUNC
 */
HWTEST_F(SelectionCommonTest, SelectionCommon001, TestSize.Level0)
{
    ASSERT_TRUE(IsAllWhitespace(""));
    ASSERT_TRUE(IsAllWhitespace("    "));
    ASSERT_TRUE(IsAllWhitespace("\t\n\r\f\v"));
    ASSERT_TRUE(IsAllWhitespace("\u00A0"));
    ASSERT_TRUE(IsAllWhitespace("\u3000"));
    ASSERT_TRUE(IsAllWhitespace("\u200B\u200C\u200D"));
    ASSERT_TRUE(IsAllWhitespace("   \u200B\u2060   "));
    ASSERT_TRUE(IsAllWhitespace("\u00A0\u3000\u200B"));
    ASSERT_TRUE(IsAllWhitespace("\u00A0\u3000\u200B\u200D"));
}

/**
 * @tc.name: SelectionCommon002
 * @tc.desc: test the IsAllWhitespace method with non-null characters
 * @tc.type: FUNC
 */
HWTEST_F(SelectionCommonTest, SelectionCommon002, TestSize.Level0)
{
    ASSERT_FALSE(IsAllWhitespace("hello"));
    ASSERT_FALSE(IsAllWhitespace("a\u200B"));
    ASSERT_FALSE(IsAllWhitespace("\u200B hello \u200D"));
    ASSERT_FALSE(IsAllWhitespace("\xF0\x9F\x98\x87"));
}

/**
 * @tc.name: SelectionCommon003
 * @tc.desc: test the IsNumber method with characters
 * @tc.type: FUNC
 */
HWTEST_F(SelectionCommonTest, SelectionCommon003, TestSize.Level0)
{
    std::string str;
    ASSERT_FALSE(IsNumber(str));
    ASSERT_FALSE(IsNumber("+"));
    ASSERT_FALSE(IsNumber("-"));
    ASSERT_FALSE(IsNumber("100a"));
    ASSERT_TRUE(IsNumber("100"));
}


/**
 * @tc.name: SelectionCommon004
 * @tc.desc: test the ParseAppInfo method with strings
 * @tc.type: FUNC
 */
HWTEST_F(SelectionCommonTest, SelectionCommon004, TestSize.Level0)
{
    ASSERT_EQ(ParseAppInfo("a"), std::nullopt);
    ASSERT_EQ(ParseAppInfo("a/"), std::nullopt);
    ASSERT_EQ(ParseAppInfo("/a"), std::nullopt);
    std::string appInfoStr = "a/b";
    auto appInfo = ParseAppInfo(appInfoStr);
    ASSERT_NE(appInfo, std::nullopt);
    ASSERT_EQ(std::get<0>(appInfo.value()), "a");
    ASSERT_EQ(std::get<1>(appInfo.value()), "b");
}

/**
 * @tc.name: SelectionCommon005
 * @tc.desc: test operator==
 * @tc.type: FUNC
 */
HWTEST_F(SelectionCommonTest, SelectionCommon005, TestSize.Level0)
{
    AbilityRuntimeInfo abilityInfo{101, "a", "b"};
    AbilityRuntimeInfo &abilityInfo2 = abilityInfo;
    ASSERT_TRUE(abilityInfo == abilityInfo2);

    AbilityRuntimeInfo abilityInfo3{101, "c", "d"};
    ASSERT_FALSE(abilityInfo == abilityInfo3);

    AbilityRuntimeInfo abilityInfo4{101, "a", "d"};
    ASSERT_FALSE(abilityInfo == abilityInfo4);

    AbilityRuntimeInfo abilityInfo5{101, "a", "b"};
    ASSERT_TRUE(abilityInfo == abilityInfo5);
}
}
}