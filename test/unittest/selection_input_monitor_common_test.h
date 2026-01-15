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

#ifndef SELECTION_INPUT_MONITOR_COMMON_TEST_H
#define SELECTION_INPUT_MONITOR_COMMON_TEST_H

#include "gtest/gtest.h"
#include <chrono>
#include <thread>

#include "selection_config.h"
#include "selection_input_monitor.h"

namespace OHOS {
namespace SelectionFwk {
constexpr int GLOBAL_X_OFFSET = 100;
constexpr int GLOBAL_Y_OFFSET = 200;
constexpr int WINDOW_X_OFFSET = 50;
constexpr int WINDOW_Y_OFFSET = 60;
constexpr int DOUBLE_CLICK_INTERVAL = 100;
constexpr int TEST_CLICK_WAIT_INTERVAL = 600;
constexpr int TEST_CLICK_POSITION_X = 50;
constexpr int TEST_CLICK_POSITION_Y = 50;
constexpr int SMALL_MOVE = 10;
constexpr int TIMEOUT_TIME = 10;

using namespace testing::ext;

struct EventStruct {
    int buttonId;
    int action;
};

inline std::shared_ptr<PointerEvent> GetPointerEvent()
{
    std::shared_ptr<PointerEvent> pointEvent = PointerEvent::Create();
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetGlobalX(GLOBAL_X_OFFSET);
    pointerItem.SetGlobalY(GLOBAL_Y_OFFSET);
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
void NONE_BUTTON_MOVE(std::shared_ptr<T> handler)
{
    NONE_BUTTON_MOVE(handler, GLOBAL_X_OFFSET + MAX_POSITION_CHANGE_OFFSET + SMALL_MOVE,
        GLOBAL_Y_OFFSET + MAX_POSITION_CHANGE_OFFSET + SMALL_MOVE);
}

template <typename T>
void NONE_BUTTON_MOVE(std::shared_ptr<T> handler, int x, int y)
{
    std::shared_ptr<PointerEvent> pointEvent = GetPointerEvent();

    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(1);
    pointerItem.SetGlobalX(x);
    pointerItem.SetGlobalY(y);
    pointerItem.SetWindowX(WINDOW_X_OFFSET);
    pointerItem.SetWindowY(WINDOW_Y_OFFSET);

    pointEvent->SetPointerId(pointerItem.GetPointerId());
    pointEvent->AddPointerItem(pointerItem);

    pointEvent->SetButtonId(PointerEvent::BUTTON_NONE);
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
void CTRL_DOWN_REPEAT(std::shared_ptr<T> handler)
{
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    keyEvent->SetRepeatKey(true);
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

template <typename T>
void CTRL_UP_REPEAT(std::shared_ptr<T> handler)
{
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    keyEvent->SetRepeatKey(true);
    handler->OnInputEvent(keyEvent);
}

template <typename T>
void SHIFT_DOWN(std::shared_ptr<T> handler)
{
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_SHIFT_LEFT);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    handler->OnInputEvent(keyEvent);
}

template <typename T>
void SHIFT_UP(std::shared_ptr<T> handler)
{
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_SHIFT_LEFT);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    handler->OnInputEvent(keyEvent);
}

template <typename T>
void CTRL_C(std::shared_ptr<T> handler)
{
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    handler->OnInputEvent(keyEvent);

    keyEvent->SetKeyCode(KeyEvent::KEYCODE_C);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    handler->OnInputEvent(keyEvent);

    keyEvent->SetKeyCode(KeyEvent::KEYCODE_C);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    handler->OnInputEvent(keyEvent);

    keyEvent->SetKeyCode(KeyEvent::KEYCODE_CTRL_LEFT);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    handler->OnInputEvent(keyEvent);
}

template <typename T>
void LEAVE_WINDOW(std::shared_ptr<T> handler)
{
    std::shared_ptr<PointerEvent> pointEvent = GetPointerEvent();
    pointEvent->SetButtonId(PointerEvent::BUTTON_NONE);
    pointEvent->SetPointerAction(PointerEvent::PointerEvent::POINTER_ACTION_LEAVE_WINDOW);
    handler->OnInputEvent(pointEvent);
}

template <typename T>
void ENTER_WINDOW(std::shared_ptr<T> handler)
{
    std::shared_ptr<PointerEvent> pointEvent = GetPointerEvent();
    pointEvent->SetButtonId(PointerEvent::BUTTON_NONE);
    pointEvent->SetPointerAction(PointerEvent::PointerEvent::POINTER_ACTION_ENTER_WINDOW);
    handler->OnInputEvent(pointEvent);
}

inline void WAIT_TIMEOUT()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(DOUBLE_CLICK_TIME + TIMEOUT_TIME));
}

inline void WAIT_TIMEOUT(int mseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(mseconds));
}

inline void CHECK_INFO(const SelectionInfo& info)
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

template <typename T>
void LEFT_BUTTON_DOWN(std::shared_ptr<T> handler, int x, int y)
{
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();

    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(1);
    pointerItem.SetGlobalX(x);
    pointerItem.SetGlobalY(y);
    pointerItem.SetWindowX(WINDOW_X_OFFSET);
    pointerItem.SetWindowY(WINDOW_Y_OFFSET);

    pointerEvent->SetPointerId(pointerItem.GetPointerId());
    pointerEvent->AddPointerItem(pointerItem);
    pointerEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    handler->OnInputEvent(pointerEvent);
}

template <typename T>
void LEFT_BUTTON_UP(std::shared_ptr<T> handler, int x, int y)
{
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();

    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(1);
    pointerItem.SetGlobalX(x);
    pointerItem.SetGlobalY(y);
    pointerItem.SetWindowX(WINDOW_X_OFFSET);
    pointerItem.SetWindowY(WINDOW_Y_OFFSET);

    pointerEvent->SetPointerId(pointerItem.GetPointerId());
    pointerEvent->AddPointerItem(pointerItem);
    pointerEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    handler->OnInputEvent(pointerEvent);
}

template <typename T>
void LEFT_BUTTON_CLICK(std::shared_ptr<T> handler, int x, int y)
{
    LEFT_BUTTON_DOWN(handler, x, y);
    LEFT_BUTTON_UP(handler, x, y);
}

template <typename T>
void LEFT_BUTTON_DOUBLECLICK(std::shared_ptr<T> handler, int x, int y)
{
    LEFT_BUTTON_CLICK(handler, x, y);
    WAIT_TIMEOUT(DOUBLE_CLICK_INTERVAL);
    LEFT_BUTTON_CLICK(handler, x, y);
}

template <typename T>
void LEFT_BUTTON_TRIPLECLICK(std::shared_ptr<T> handler, int x, int y)
{
    LEFT_BUTTON_CLICK(handler, x, y);
    WAIT_TIMEOUT(DOUBLE_CLICK_INTERVAL);
    LEFT_BUTTON_CLICK(handler, x, y);
    WAIT_TIMEOUT(DOUBLE_CLICK_INTERVAL);
    LEFT_BUTTON_CLICK(handler, x, y);
}

} // namespace SelectionFwk
} // namespace OHOS

#endif // SELECTION_INPUT_MONITOR_COMMON_TEST_H