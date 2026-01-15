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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "parameter.h"
#include "selection_app_validator.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;
using ::testing::Return;
using ::testing::Mock;

class MockSelectionAppValidator : public SelectionAppValidator {
public:
    MOCK_METHOD(std::optional<std::string>, GetCurrentBundleName, (), (const, override));
};

class SelectionAppValidatorTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SelectionAppValidatorTest::SetUpTestCase()
{
    std::cout << "SelectionAppValidatorTest SetUpTestCase" << std::endl;
}

void SelectionAppValidatorTest::TearDownTestCase()
{
    std::cout << "SelectionAppValidatorTest TearDownTestCase" << std::endl;
}

void SelectionAppValidatorTest::SetUp()
{
    std::cout << "SelectionAppValidatorTest SetUp" << std::endl;
}

void SelectionAppValidatorTest::TearDown()
{
    std::cout << "SelectionAppValidatorTest TearDown" << std::endl;
}

/**
 * @tc.name: SelectionAppValidator001
 * @tc.desc: selection_app_validator testcase validator001
 * @tc.type: FUNC
 */
HWTEST_F(SelectionAppValidatorTest, SelectionAppValidator001, TestSize.Level0)
{
    auto& appValidator = SelectionAppValidator::GetInstance();
    auto currentBundle = appValidator.GetCurrentBundleName();
    ASSERT_EQ(currentBundle, std::nullopt);
    bool res = appValidator.Validate();
    ASSERT_FALSE(res);
}

/**
 * @tc.name: SelectionAppValidator002
 * @tc.desc: test Validate with same current bundleName and sys bundleName
 * @tc.type: FUNC
 */
HWTEST_F(SelectionAppValidatorTest, SelectionAppValidator002, TestSize.Level0)
{
    MockSelectionAppValidator validator;
    EXPECT_CALL(validator, GetCurrentBundleName()).WillRepeatedly(Return("a"));
    int ret = SetParameter("sys.selection.app", "a/b");
    ASSERT_EQ(ret, 0);
    bool res = validator.Validate();
    ASSERT_TRUE(res);

    ret = SetParameter("sys.selection.app", "b/a");
    ASSERT_EQ(ret, 0);
    res = validator.Validate();
    ASSERT_FALSE(res);
}

/**
 * @tc.name: SelectionAppValidator003
 * @tc.desc: test Validate with invalid sys bundleName
 * @tc.type: FUNC
 */
HWTEST_F(SelectionAppValidatorTest, SelectionAppValidator003, TestSize.Level0)
{
    MockSelectionAppValidator validator;
    EXPECT_CALL(validator, GetCurrentBundleName()).Times(1).WillOnce(Return("a"));
    int ret = SetParameter("sys.selection.app", "a");
    ASSERT_EQ(ret, 0);
    bool res = validator.Validate();
    ASSERT_FALSE(res);
}
}
}