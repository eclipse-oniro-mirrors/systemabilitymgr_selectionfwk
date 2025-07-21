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

#include "gtest/gtest.h"
#include <chrono>
#include <thread>

#include "selection_input_monitor.h"

namespace OHOS {
namespace SelectionFwk {
const int DISPLAY_X_OFFSET = 100;
const int DISPLAY_Y_OFFSET = 200;
const int WINDOW_X_OFFSET = 50;
const int WINDOW_Y_OFFSET = 60;

using namespace testing::ext;

struct EventStruct {
    int buttonId;
    int action;
};

std::shared_ptr<PointerEvent> GetPointerEvent()
{
    std::shared_ptr<PointerEvent> pointEvent = PointerEvent::Create();
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetDisplayX(DISPLAY_X_OFFSET);
    pointerItem.SetDisplayY(DISPLAY_Y_OFFSET);
    pointerItem.SetWindowX(WINDOW_X_OFFSET);
    pointerItem.SetWindowY(WINDOW_Y_OFFSET);

    pointEvent->AddPointerItem(pointerItem);
    return pointEvent;
}

template <typename T>
void LEFT_BUTTON_DOWN(std::shared_ptr<T> handler)
{
    std::shared_ptr<PointerEvent> pointEvent = GetPointerEvent();
    pointEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    handler->OnInputEvent(pointEvent);
}

template <typename T>
void LEFT_BUTTON_UP(std::shared_ptr<T> handler)
{
    std::shared_ptr<PointerEvent> pointEvent = GetPointerEvent();
    pointEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    handler->OnInputEvent(pointEvent);
}

template <typename T>
void LEFT_BUTTON_CLICK(std::shared_ptr<T> handler)
{
    LEFT_BUTTON_DOWN(handler);
    LEFT_BUTTON_UP(handler);
}

template <typename T>
void RIGHT_BUTTON_DOWN(std::shared_ptr<T> handler)
{
    std::shared_ptr<PointerEvent> pointEvent = GetPointerEvent();
    pointEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_RIGHT);
    pointEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    handler->OnInputEvent(pointEvent);
}

template <typename T>
void RIGHT_BUTTON_UP(std::shared_ptr<T> handler)
{
    std::shared_ptr<PointerEvent> pointEvent = GetPointerEvent();
    pointEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_RIGHT);
    pointEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    handler->OnInputEvent(pointEvent);
}

template <typename T>
void LEFT_BUTTON_MOVE(std::shared_ptr<T> handler)
{
    std::shared_ptr<PointerEvent> pointEvent = GetPointerEvent();
    pointEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerAction(PointerEvent::PointerEvent::POINTER_ACTION_MOVE);
    handler->OnInputEvent(pointEvent);
}

template <typename T>
void CTRL_DOWN(std::shared_ptr<T> handler)
{
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    handler->OnInputEvent(keyEvent);
}

template <typename T>
void CTRL_UP(std::shared_ptr<T> handler)
{
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    handler->OnInputEvent(keyEvent);
}

void WAIT_TIMEOUT()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(DOUBLE_CLICK_TIME + 1)); // >500ms
}

void CHECK_INFO(const SelectionInfo& info)
{
    EXPECT_NE(info.selectionType, 0);
    EXPECT_GT(info.startDisplayX, 0);
    EXPECT_GT(info.startDisplayY, 0);
    EXPECT_GT(info.endDisplayX, 0);
    EXPECT_GT(info.endDisplayY, 0);
    EXPECT_GT(info.startWindowX, 0);
    EXPECT_GT(info.startWindowY, 0);
    EXPECT_GT(info.endWindowX, 0);
    EXPECT_GT(info.endWindowY, 0);
}

class BaseSelectionInputMonitorTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    std::shared_ptr<BaseSelectionInputMonitor> inputMonitor = nullptr;
};

void BaseSelectionInputMonitorTest::SetUpTestCase()
{
    std::cout << "BaseSelectionInputMonitorTest SetUpTestCase" << std::endl;
}

void BaseSelectionInputMonitorTest::TearDownTestCase()
{
    std::cout << "BaseSelectionInputMonitorTest TearDownTestCase" << std::endl;
}

void BaseSelectionInputMonitorTest::SetUp()
{
    std::cout << "BaseSelectionInputMonitorTest SetUp" << std::endl;
    inputMonitor = std::make_shared<BaseSelectionInputMonitor>();
}

void BaseSelectionInputMonitorTest::TearDown()
{
    std::cout << "BaseSelectionInputMonitorTest TearDown" << std::endl;
}

/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor001, TestSize.Level1)
{
    std::cout << " SelectInputMonitor001 start " << std::endl;
    BaseSelectionInputMonitor::ctrlSelectFlag = false;

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);
    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor002, TestSize.Level1)
{
    std::cout << " SelectInputMonitor002 start " << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor003, TestSize.Level1)
{
    std::cout << " SelectInputMonitor003 start " << std::endl;
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);
    LEFT_BUTTON_CLICK(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor004, TestSize.Level1)
{
    std::cout << " SelectInputMonitor004 start " << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    RIGHT_BUTTON_DOWN(inputMonitor);
    RIGHT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, false);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor005, TestSize.Level1)
{
    std::cout << " SelectInputMonitor005 start " << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    WAIT_TIMEOUT();

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, false);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor006, TestSize.Level1)
{
    std::cout << " SelectInputMonitor006 start " << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    WAIT_TIMEOUT();

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor007, TestSize.Level1)
{
    std::cout << " SelectInputMonitor007 start " << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    WAIT_TIMEOUT();

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor008, TestSize.Level1)
{
    std::cout << " SelectInputMonitor008 start " << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    WAIT_TIMEOUT();

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    WAIT_TIMEOUT();

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, false);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor009, TestSize.Level1)
{
    std::cout << " SelectInputMonitor009 start " << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor010, TestSize.Level1)
{
    std::cout << " SelectInputMonitor010 start " << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor011, TestSize.Level1)
{
    std::cout << " SelectInputMonitor011 start " << std::endl;
    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor012, TestSize.Level1)
{
    std::cout << " SelectInputMonitor012 start " << std::endl;
    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_MOVE(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor013, TestSize.Level1)
{
    std::cout << " SelectInputMonitor013 start " << std::endl;
    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor014, TestSize.Level1)
{
    std::cout << " SelectInputMonitor014 start " << std::endl;
    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
    auto info = inputMonitor->GetSelectionInfo();
    CHECK_INFO(info);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor015, TestSize.Level1)
{
    std::cout << " SelectInputMonitor015 start " << std::endl;
    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, false);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor016, TestSize.Level1)
{
    std::cout << " SelectInputMonitor016 start " << std::endl;
    CTRL_DOWN(inputMonitor);
    CTRL_UP(inputMonitor);

    LEFT_BUTTON_DOWN(inputMonitor);
    LEFT_BUTTON_UP(inputMonitor);

    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, false);
}

} // namespace SelectionFwk
} // namespace OHOS
