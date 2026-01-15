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

#include "parameter.h"
#include "param_wrapper.h"
#include "sys_selection_config_repository.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

class SysSelectionConfigRepositoryTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SysSelectionConfigRepositoryTest::SetUpTestCase()
{
    std::cout << "SysSelectionConfigRepositoryTest SetUpTestCase" << std::endl;
}

void SysSelectionConfigRepositoryTest::TearDownTestCase()
{
    std::cout << "SysSelectionConfigRepositoryTest TearDownTestCase" << std::endl;
}

void SysSelectionConfigRepositoryTest::SetUp()
{
    std::cout << "SysSelectionConfigRepositoryTest SetUp" << std::endl;
}

void SysSelectionConfigRepositoryTest::TearDown()
{
    std::cout << "SysSelectionConfigRepositoryTest TearDown" << std::endl;
}

/**
 * @tc.name: SysSelectionConfigRepository001
 * @tc.desc: test SysSelectionConfigRepository::GetInstance
 * @tc.type: FUNC
 */
HWTEST_F(SysSelectionConfigRepositoryTest, SysSelectionConfigRepository001, TestSize.Level0)
{
    auto obj = SysSelectionConfigRepository::GetInstance();
    ASSERT_NE(obj, nullptr);
}

/**
 * @tc.name: SysSelectionConfigRepository002
 * @tc.desc: test SetSysParameters
 * @tc.type: FUNC
 */
HWTEST_F(SysSelectionConfigRepositoryTest, SysSelectionConfigRepository002, TestSize.Level0)
{
    SelectionConfig config;
    config.SetEnabled(true);
    config.SetTriggered(true);
    config.SetApplicationInfo("a/b");
    config.SetUid(100);
    int ret = SysSelectionConfigRepository::GetInstance()->SetSysParameters(config);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SysSelectionConfigRepository003
 * @tc.desc: test GetSysParameters
 * @tc.type: FUNC
 */
HWTEST_F(SysSelectionConfigRepositoryTest, SysSelectionConfigRepository003, TestSize.Level0)
{
    SelectionConfig config = SysSelectionConfigRepository::GetInstance()->GetSysParameters();
    ASSERT_EQ(config.GetEnable(), true);
    ASSERT_EQ(config.GetTriggered(), true);
    ASSERT_EQ(config.GetApplicationInfo(), "a/b");
    ASSERT_EQ(config.GetUid(), 100);
    SysSelectionConfigRepository::GetInstance()->SetTriggered(false);
    ASSERT_EQ(SysSelectionConfigRepository::GetInstance()->GetTriggered(), false);
    SysSelectionConfigRepository::GetInstance()->SetTriggered(true);
}

/**
 * @tc.name: SysSelectionConfigRepository004
 * @tc.desc: test DisableSAService
 * @tc.type: FUNC
 */
HWTEST_F(SysSelectionConfigRepositoryTest, SysSelectionConfigRepository004, TestSize.Level0)
{
    SysSelectionConfigRepository::GetInstance()->DisableSAService();
    SelectionConfig config = SysSelectionConfigRepository::GetInstance()->GetSysParameters();
    ASSERT_EQ(config.GetEnable(), false);
    int ret = SetParameter("sys.selection.switch", "on");
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SysSelectionConfigRepository005
 * @tc.desc: test SetParameter with invalid uid
 * @tc.type: FUNC
 */
HWTEST_F(SysSelectionConfigRepositoryTest, SysSelectionConfigRepository005, TestSize.Level0)
{
    auto ret = SetParameter("sys.selection.uid", "a");
    ASSERT_EQ(ret, 0);
    SelectionConfig config = SysSelectionConfigRepository::GetInstance()->GetSysParameters();
    ASSERT_NE(config.GetUid(), 0);

    ret = SetParameter("sys.selection.uid", "2147483648");
    config = SysSelectionConfigRepository::GetInstance()->GetSysParameters();
    ASSERT_NE(config.GetUid(), 0);

    ret = SetParameter("sys.selection.uid", "-2147483649");
    config = SysSelectionConfigRepository::GetInstance()->GetSysParameters();
    ASSERT_NE(config.GetUid(), 0);

    ret = SetParameter("sys.selection.uid", "21474836481");
    int res = SysSelectionConfigRepository::GetInstance()->GetUid();
    ASSERT_EQ(res, -1);

    ret = SetParameter("sys.selection.uid", "100");
    ASSERT_EQ(ret, 0);
}
}
}