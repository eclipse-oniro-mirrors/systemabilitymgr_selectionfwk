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

#include "focus_change_listener.h"
#include "focus_monitor_manager.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

class FocusMonitorTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void FocusMonitorTest::SetUpTestCase()
{
    std::cout << "FocusMonitorTest SetUpTestCase" << std::endl;
}

void FocusMonitorTest::TearDownTestCase()
{
    std::cout << "FocusMonitorTest TearDownTestCase" << std::endl;
}

void FocusMonitorTest::SetUp()
{
    std::cout << "FocusMonitorTest SetUp" << std::endl;
}

void FocusMonitorTest::TearDown()
{
    std::cout << "FocusMonitorTest TearDown" << std::endl;
}

/**
 * @tc.name: FocusMonitor001
 * @tc.desc: test RegisterFocusChangedListener and OnFocused
 * @tc.type: FUNC
 */
HWTEST_F(FocusMonitorTest, FocusMonitor001, TestSize.Level0)
{
    auto focusChangeHandle = [this](const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused) {
        ASSERT_NE(focusChangeInfo, nullptr);
    };
    FocusMonitorManager::GetInstance().RegisterFocusChangedListener(focusChangeHandle);
    FocusChangedListener listener(focusChangeHandle);
    sptr<Rosen::FocusChangeInfo> focusChangeInfo = new (std::nothrow) Rosen::FocusChangeInfo();
    ASSERT_NE(focusChangeInfo, nullptr);
    listener.OnFocused(focusChangeInfo);
    FocusMonitorManager::GetInstance().UnregisterFocusChangedListener();
}

/**
 * @tc.name: FocusMonitor002
 * @tc.desc: test RegisterFocusChangedListener and OnUnfocused
 * @tc.type: FUNC
 */
HWTEST_F(FocusMonitorTest, FocusMonitor002, TestSize.Level0)
{
    auto focusChangeHandle = [this](const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused) {
        ASSERT_NE(focusChangeInfo, nullptr);
    };
    FocusMonitorManager::GetInstance().RegisterFocusChangedListener(focusChangeHandle);
    FocusChangedListener listener(focusChangeHandle);
    sptr<Rosen::FocusChangeInfo> focusChangeInfo = new (std::nothrow) Rosen::FocusChangeInfo();
    ASSERT_NE(focusChangeInfo, nullptr);
    listener.OnUnfocused(focusChangeInfo);
    FocusMonitorManager::GetInstance().UnregisterFocusChangedListener();
}

/**
 * @tc.name: FocusMonitor003
 * @tc.desc: test OnFocused with nullptr
 * @tc.type: FUNC
 */
HWTEST_F(FocusMonitorTest, FocusMonitor003, TestSize.Level0)
{
    auto focusChangeHandle = [this](const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused) {
        ASSERT_NE(focusChangeInfo, nullptr);
    };
    FocusMonitorManager::GetInstance().RegisterFocusChangedListener(focusChangeHandle);
    FocusMonitorManager::GetInstance().RegisterFocusChangedListener(focusChangeHandle);
    FocusChangedListener listener(focusChangeHandle);
    sptr<Rosen::FocusChangeInfo> focusChangeInfo;
    ASSERT_EQ(focusChangeInfo, nullptr);
    listener.OnFocused(focusChangeInfo);
    FocusMonitorManager::GetInstance().UnregisterFocusChangedListener();
    FocusMonitorManager::GetInstance().UnregisterFocusChangedListener();
}

/**
 * @tc.name: FocusMonitor004
 * @tc.desc: test OnUnfocused with nullptr
 * @tc.type: FUNC
 */
HWTEST_F(FocusMonitorTest, FocusMonitor004, TestSize.Level0)
{
    auto focusChangeHandle = [this](const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool isFocused) {
        ASSERT_NE(focusChangeInfo, nullptr);
    };
    FocusMonitorManager::GetInstance().RegisterFocusChangedListener(focusChangeHandle);
    FocusChangedListener listener(focusChangeHandle);
    sptr<Rosen::FocusChangeInfo> focusChangeInfo;
    ASSERT_EQ(focusChangeInfo, nullptr);
    listener.OnUnfocused(focusChangeInfo);
    FocusMonitorManager::GetInstance().UnregisterFocusChangedListener();
}

/**
 * @tc.name: FocusMonitor005
 * @tc.desc: test nullptr focusChangeHandle
 * @tc.type: FUNC
 */
HWTEST_F(FocusMonitorTest, FocusMonitor005, TestSize.Level0)
{
    std::function<void(const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool)> focusChangeHandle = nullptr;
    FocusChangedListener listener(focusChangeHandle);
    sptr<Rosen::FocusChangeInfo> focusChangeInfo;
    ASSERT_EQ(focusChangeInfo, nullptr);
    listener.OnFocused(focusChangeInfo);
    listener.OnUnfocused(focusChangeInfo);

    sptr<Rosen::FocusChangeInfo> focusChangeInfo2 = new (std::nothrow) Rosen::FocusChangeInfo();
    ASSERT_NE(focusChangeInfo2, nullptr);
    listener.OnFocused(focusChangeInfo2);
    listener.OnUnfocused(focusChangeInfo2);
}
}
}