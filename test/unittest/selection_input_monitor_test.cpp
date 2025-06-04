/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
    inputMonitor = std::make_shared<SelectionInputMonitor>(std::make_shared<DefaultSelectionEventListener>());
}

void SelectionInputMonitorTest::TearDown()
{
    std::cout << "SelectionInputMonitorTest TearDown" << std::endl;
}

/**
 * @tc.name: param check samgr ready event
 * @tc.desc: param check samgr ready event
 * @tc.type: FUNC
 */
HWTEST_F(SelectionInputMonitorTest, SelectInputMonitor001, TestSize.Level1)
{
    std::cout << " SelectInputMonitor001 start " << std::endl;
    SelectionInputMonitor::ctrlSelectFlag = false;
    SelectionInputMonitor::lastTextSelectedFlag = false;

    // bool screenLockedFlag = OHOS::ScreenLock::ScreenLockManager::GetInstance()->IsScreenLocked();
    // if (screenLockedFlag) {
    //     ASSERT_EQ(true, true);
    //     return;
    // }

    std::shared_ptr<PointerEvent> pointEvent = PointerEvent::Create();
    pointEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    inputMonitor->OnInputEvent(pointEvent);

    pointEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    inputMonitor->OnInputEvent(pointEvent);

    pointEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    inputMonitor->OnInputEvent(pointEvent);

    auto ret = inputMonitor->lastTextSelectedFlag;
    ASSERT_EQ(ret, true);
}

}
}