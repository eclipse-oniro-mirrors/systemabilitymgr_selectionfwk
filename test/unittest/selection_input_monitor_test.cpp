/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "selection_input_monitor_common_test.h"
#include "selection_input_monitor.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Mock;

class MockBaseSelectionInputMonitor : public BaseSelectionInputMonitor {
public:
    MOCK_METHOD(const SelectionInfo&, GetSelectionInfo, (), (const, override));
    MOCK_METHOD(bool, IsSelectionTriggered, (), (const, override));
};

class SelectionInputMonitorTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    std::shared_ptr<SelectionInputMonitor> inputMonitor = nullptr;
};

void SelectionInputMonitorTest::SetUpTestCase()
{
    std::cout << "SelectionInputMonitorTest SetUpTestCase" << std::endl;
}

void SelectionInputMonitorTest::TearDownTestCase()
{
    std::cout << "SelectionInputMonitorTest TearDownTestCase" << std::endl;
}

void SelectionInputMonitorTest::SetUp()
{
    std::cout << "SelectionInputMonitorTest SetUp" << std::endl;
    MemSelectionConfig::GetInstance().SetTriggered(true);
    inputMonitor = std::make_shared<SelectionInputMonitor>();
}

void SelectionInputMonitorTest::TearDown()
{
    std::cout << "SelectionInputMonitorTest TearDown" << std::endl;
}

/**
 * @tc.name: SelectInputMonitor001
 * @tc.desc: test inject ctrl c
 * @tc.type: FUNC
 */
HWTEST_F(SelectionInputMonitorTest, SelectInputMonitor001, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl001 start" << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    int fd = inputMonitor->fd_;
    int ret = inputMonitor->InjectCtrlC();
    ASSERT_EQ(ret, 0);

    inputMonitor->fd_ = -1;
    ret = inputMonitor->InjectCtrlC();
    ASSERT_NE(ret, 0);
    inputMonitor->fd_ = fd;
}

/**
 * @tc.name: SelectInputMonitor002
 * @tc.desc: test GetSelectionContent
 * @tc.type: FUNC
 */
HWTEST_F(SelectionInputMonitorTest, SelectInputMonitor002, TestSize.Level0)
{
    std::cout << "SelectInputMonitor002 start" << std::endl;
    auto reginBaseInputMonitor = inputMonitor->baseInputMonitor_;
    std::shared_ptr<MockBaseSelectionInputMonitor> mockObj= std::make_shared<MockBaseSelectionInputMonitor>();
    SelectionInfo selectionInfo;
    selectionInfo.bundleName = "a/b";
    EXPECT_CALL(*mockObj, GetSelectionInfo()).WillRepeatedly(ReturnRef(selectionInfo));
    EXPECT_CALL(*mockObj, IsSelectionTriggered()).WillRepeatedly(Return(true));
    inputMonitor->baseInputMonitor_ = mockObj;

    MemSelectionConfig::GetInstance().SetEnabled(true);
    ASSERT_EQ(MemSelectionConfig::GetInstance().GetEnable(), true);

    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);
    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    std::string selectionContent;
    inputMonitor->baseInputMonitor_ = reginBaseInputMonitor;
    int32_t ret = inputMonitor->GetSelectionContent(selectionContent);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: SelectInputMonitor003
 * @tc.desc: test FinishedWordSelection failed by switch off
 * @tc.type: FUNC
 */
HWTEST_F(SelectionInputMonitorTest, SelectInputMonitor003, TestSize.Level0)
{
    std::cout << "SelectInputMonitor003 start" << std::endl;
    auto reginBaseInputMonitor = inputMonitor->baseInputMonitor_;
    std::shared_ptr<MockBaseSelectionInputMonitor> mockObj= std::make_shared<MockBaseSelectionInputMonitor>();
    SelectionInfo selectionInfo;
    selectionInfo.bundleName = "a/b";
    EXPECT_CALL(*mockObj, GetSelectionInfo()).WillRepeatedly(ReturnRef(selectionInfo));
    EXPECT_CALL(*mockObj, IsSelectionTriggered()).WillRepeatedly(Return(true));
    inputMonitor->baseInputMonitor_ = mockObj;

    MemSelectionConfig::GetInstance().SetEnabled(false);
    ASSERT_EQ(MemSelectionConfig::GetInstance().GetEnable(), false);
    inputMonitor->FinishedWordSelection();

    MemSelectionConfig::GetInstance().SetEnabled(true);
    ASSERT_EQ(MemSelectionConfig::GetInstance().GetEnable(), true);
}
} // namespace SelectionFwk
} // namespace OHOS