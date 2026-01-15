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

#include "system_ability_status_change_listener.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

class SystemAbilityStatusChangeListenerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SystemAbilityStatusChangeListenerTest::SetUpTestCase()
{
    std::cout << "SystemAbilityStatusChangeListenerTest SetUpTestCase" << std::endl;
}

void SystemAbilityStatusChangeListenerTest::TearDownTestCase()
{
    std::cout << "SystemAbilityStatusChangeListenerTest TearDownTestCase" << std::endl;
}

void SystemAbilityStatusChangeListenerTest::SetUp()
{
    std::cout << "SystemAbilityStatusChangeListenerTest SetUp" << std::endl;
}

void SystemAbilityStatusChangeListenerTest::TearDown()
{
    std::cout << "SystemAbilityStatusChangeListenerTest TearDown" << std::endl;
}

/**
 * @tc.name: SystemAbilityStatusChangeListener001
 * @tc.desc: test OnAddSystemAbility with func
 * @tc.type: FUNC
 */
HWTEST_F(SystemAbilityStatusChangeListenerTest, SystemAbilityStatusChangeListener001, TestSize.Level0)
{
    SystemAbilityStatusChangeListener listener(nullptr);
    listener.OnAddSystemAbility(1, "device1");

    bool funcCalled = false;
    SystemAbilityStatusChangeListener listener2([&funcCalled](int32_t, const std::string &) {
        funcCalled = true;
    });

    listener2.OnAddSystemAbility(1, "device1");
    listener.OnRemoveSystemAbility(1, "device1");

    EXPECT_TRUE(funcCalled);
}
}
}