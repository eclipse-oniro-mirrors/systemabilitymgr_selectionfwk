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

#include "sys_selection_config_repository.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

class SysSelectionConfigRepositoryMockTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SysSelectionConfigRepositoryMockTest::SetUpTestCase()
{
    std::cout << "SysSelectionConfigRepositoryMockTest SetUpTestCase" << std::endl;
}

void SysSelectionConfigRepositoryMockTest::TearDownTestCase()
{
    std::cout << "SysSelectionConfigRepositoryMockTest TearDownTestCase" << std::endl;
}

void SysSelectionConfigRepositoryMockTest::SetUp()
{
    std::cout << "SysSelectionConfigRepositoryMockTest SetUp" << std::endl;
}

void SysSelectionConfigRepositoryMockTest::TearDown()
{
    std::cout << "SysSelectionConfigRepositoryMockTest TearDown" << std::endl;
}

/**
 * @tc.name: SysSelectionConfigRepository001
 * @tc.desc: test SetSysParameters
 * @tc.type: FUNC
 */
HWTEST_F(SysSelectionConfigRepositoryMockTest, SysSelectionConfigRepositoryMock001, TestSize.Level0)
{
    SysSelectionConfigRepository::GetInstance()->SetEnabled(true);
    SysSelectionConfigRepository::GetInstance()->SetTriggered(true);
    SysSelectionConfigRepository::GetInstance()->SetUid(1001);
    SysSelectionConfigRepository::GetInstance()->SetApplicationInfo("");

    SysSelectionConfigRepository::GetInstance()->GetEnable();
    SysSelectionConfigRepository::GetInstance()->GetTriggered();
    int ret = SysSelectionConfigRepository::GetInstance()->GetUid();
    ASSERT_EQ(ret, -1);
    std::string appInfo = SysSelectionConfigRepository::GetInstance()->GetApplicationInfo();
    ASSERT_EQ(appInfo, "");
    SysSelectionConfigRepository::GetInstance()->DisableSAService();
}
}
}