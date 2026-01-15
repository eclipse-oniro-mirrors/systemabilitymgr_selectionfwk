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

#include "selection_input_monitor_common_test.h"

namespace OHOS {
namespace SelectionFwk {

class BaseSelectionInputMonitorCtrlTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    std::shared_ptr<BaseSelectionInputMonitor> inputMonitor = nullptr;
};

void BaseSelectionInputMonitorCtrlTest::SetUpTestCase()
{
    std::cout << "BaseSelectionInputMonitorCtrlTest SetUpTestCase" << std::endl;
}

void BaseSelectionInputMonitorCtrlTest::TearDownTestCase()
{
    std::cout << "BaseSelectionInputMonitorCtrlTest TearDownTestCase" << std::endl;
}

void BaseSelectionInputMonitorCtrlTest::SetUp()
{
    std::cout << "BaseSelectionInputMonitorCtrlTest SetUp" << std::endl;
    MemSelectionConfig::GetInstance().SetTriggered(true);
    inputMonitor = std::make_shared<BaseSelectionInputMonitor>();
}

void BaseSelectionInputMonitorCtrlTest::TearDown()
{
    std::cout << "BaseSelectionInputMonitorCtrlTest TearDown" << std::endl;
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl001, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl001 start" << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
    ASSERT_EQ(info.selectionType, TRIPLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl002, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl002 start" << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
    ASSERT_EQ(info.selectionType, DOUBLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl003, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl003 start" << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
    ASSERT_EQ(info.selectionType, MOVE_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl004, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl004 start" << std::endl;
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);
    WAIT_TIMEOUT(TEST_CLICK_WAIT_INTERVAL);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.startDisplayX, GLOBAL_X_OFFSET);
    ASSERT_EQ(info.startDisplayY, GLOBAL_Y_OFFSET);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl005, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl005 start" << std::endl;
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);
    MemSelectionConfig::GetInstance().SetTriggered(false);
    std::cout << "param set sys.selection.trigger ctrl" << std::endl;
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl006, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl006 start" << std::endl;
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);
    NONE_BUTTON_MOVE(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl007, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl007 start" << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.startDisplayX, TEST_CLICK_POSITION_X);
    ASSERT_EQ(info.startDisplayY, TEST_CLICK_POSITION_Y);
    ASSERT_EQ(info.selectionType, DOUBLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl008, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl008 start" << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.startDisplayX, TEST_CLICK_POSITION_X);
    ASSERT_EQ(info.startDisplayY, TEST_CLICK_POSITION_Y);
    ASSERT_EQ(info.selectionType, TRIPLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl009, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl009 start" << std::endl;
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, false);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl010, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl010 start" << std::endl;
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.startDisplayX, TEST_CLICK_POSITION_X);
    ASSERT_EQ(info.startDisplayY, TEST_CLICK_POSITION_Y);
    ASSERT_EQ(info.selectionType, DOUBLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl011, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl011 start" << std::endl;
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.startDisplayX, TEST_CLICK_POSITION_X);
    ASSERT_EQ(info.startDisplayY, TEST_CLICK_POSITION_Y);
    ASSERT_EQ(info.selectionType, TRIPLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl012, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl012 start" << std::endl;
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, false);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl013, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl013 start" << std::endl;
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.startDisplayX, TEST_CLICK_POSITION_X);
    ASSERT_EQ(info.startDisplayY, TEST_CLICK_POSITION_Y);
    ASSERT_EQ(info.selectionType, DOUBLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl014, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl014 start" << std::endl;
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, GLOBAL_X_OFFSET, GLOBAL_Y_OFFSET);
    NONE_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.startDisplayX, TEST_CLICK_POSITION_X);
    ASSERT_EQ(info.startDisplayY, TEST_CLICK_POSITION_Y);
    ASSERT_EQ(info.selectionType, TRIPLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl015, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl015 start" << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    NONE_BUTTON_MOVE(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.selectionType, MOVE_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl016, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl016 start" << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    CTRL_DOWN(inputMonitor);
    NONE_BUTTON_MOVE(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.selectionType, MOVE_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl017, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl017 start" << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEAVE_WINDOW(inputMonitor);
    ENTER_WINDOW(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.selectionType, MOVE_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl018, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl018 start" << std::endl;
    LEFT_BUTTON_DOUBLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    LEAVE_WINDOW(inputMonitor);
    ENTER_WINDOW(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.selectionType, DOUBLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl019, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl019 start" << std::endl;
    LEFT_BUTTON_TRIPLECLICK(inputMonitor, TEST_CLICK_POSITION_X, TEST_CLICK_POSITION_Y);

    LEAVE_WINDOW(inputMonitor);
    ENTER_WINDOW(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.selectionType, TRIPLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl020, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl020 start" << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEAVE_WINDOW(inputMonitor);
    ENTER_WINDOW(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    ASSERT_EQ(info.selectionType, MOVE_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl021, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl021 start" << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor);
    NONE_BUTTON_MOVE(inputMonitor, GLOBAL_X_OFFSET + 1, GLOBAL_Y_OFFSET + 1);
    LEFT_BUTTON_CLICK(inputMonitor, GLOBAL_X_OFFSET + 1, GLOBAL_Y_OFFSET + 1);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
    ASSERT_EQ(info.selectionType, DOUBLE_CLICKED_SELECTION);
}

HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl022, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl022 start" << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor);
    NONE_BUTTON_MOVE(inputMonitor, GLOBAL_X_OFFSET + MAX_POSITION_CHANGE_OFFSET + 5,
        GLOBAL_Y_OFFSET + MAX_POSITION_CHANGE_OFFSET + 5);
    LEFT_BUTTON_CLICK(inputMonitor, GLOBAL_X_OFFSET + MAX_POSITION_CHANGE_OFFSET + 5,
        GLOBAL_Y_OFFSET + MAX_POSITION_CHANGE_OFFSET + 5);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: SelectInputMonitorCtrl023
 * @tc.desc: test anther key to interrupt word selection
 * @tc.type: FUNC
 */
HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl023, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl023 start" << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    SHIFT_DOWN(inputMonitor);
    SHIFT_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: SelectInputMonitorCtrl024
 * @tc.desc: test duplicative ctrl down.
 * @tc.type: FUNC
 */
HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl024, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl024 start" << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_DOWN_REPEAT(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
    ASSERT_EQ(info.selectionType, MOVE_SELECTION);
}

/**
 * @tc.name: SelectInputMonitorCtrl025
 * @tc.desc: test word selection after ctrl down.
 * @tc.type: FUNC
 */
HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl025, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl025 start" << std::endl;
    CTRL_DOWN(inputMonitor);
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    CTRL_DOWN_REPEAT(inputMonitor);
    CTRL_UP_REPEAT(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: SelectInputMonitorCtrl026
 * @tc.desc: test pressing and holding the Ctrl key and clicking the Ctrl key.
 * @tc.type: FUNC
 */
HWTEST_F(BaseSelectionInputMonitorCtrlTest, SelectInputMonitorCtrl026, TestSize.Level0)
{
    std::cout << "SelectInputMonitorCtrl026 start" << std::endl;
    CTRL_DOWN(inputMonitor);
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    CTRL_DOWN_REPEAT(inputMonitor);
    CTRL_UP_REPEAT(inputMonitor);
    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsSelectionTriggered();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
    ASSERT_EQ(info.selectionType, MOVE_SELECTION);
}
} // namespace SelectionFwk
} // namespace OHOS