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

#define private public
#include "selection_input_monitor.h"
#include "screenlock_manager.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

struct EventStruct {
    int buttonId;
    int action;
};
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
    vector<EventStruct> events = {
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_MOVE},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_UP}
    };

    for (uint16_t i = 0; i < events.size(); i++) {
        auto event = events[i];
        std::shared_ptr<PointerEvent> pointEvent = PointerEvent::Create();
        pointEvent->SetButtonId(event.buttonId);
        pointEvent->SetPointerAction(event.action);
        inputMonitor->OnInputEvent(pointEvent);
    }
    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor002, TestSize.Level1)
{
    std::cout << " SelectInputMonitor002 start " << std::endl;
    vector<EventStruct> events = {
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_UP},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_UP}
    };

    for (uint16_t i = 0; i < events.size(); i++) {
        auto event = events[i];
        std::shared_ptr<PointerEvent> pointEvent = PointerEvent::Create();
        pointEvent->SetButtonId(event.buttonId);
        pointEvent->SetPointerAction(event.action);
        inputMonitor->OnInputEvent(pointEvent);
    }
    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor003, TestSize.Level1)
{
    std::cout << " SelectInputMonitor003 start " << std::endl;
    vector<EventStruct> events = {
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_UP},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_UP},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_UP}
    };

    for (uint16_t i = 0; i < events.size(); i++) {
        auto event = events[i];
        std::shared_ptr<PointerEvent> pointEvent = PointerEvent::Create();
        pointEvent->SetButtonId(event.buttonId);
        pointEvent->SetPointerAction(event.action);
        inputMonitor->OnInputEvent(pointEvent);
    }
    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, true);
}

HWTEST_F(BaseSelectionInputMonitorTest, SelectInputMonitor004, TestSize.Level1)
{
    std::cout << " SelectInputMonitor004 start " << std::endl;
    vector<EventStruct> events = {
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
        {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_ACTION_BUTTON_UP},
        {PointerEvent::MOUSE_BUTTON_RIGHT, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
        {PointerEvent::MOUSE_BUTTON_RIGHT, PointerEvent::POINTER_ACTION_BUTTON_UP}
    };

    for (uint16_t i = 0; i < events.size(); i++) {
        auto event = events[i];
        std::shared_ptr<PointerEvent> pointEvent = PointerEvent::Create();
        pointEvent->SetButtonId(event.buttonId);
        pointEvent->SetPointerAction(event.action);
        inputMonitor->OnInputEvent(pointEvent);
    }
    auto ret = inputMonitor->IsTextSelected();
    ASSERT_EQ(ret, false);
}

}
}
