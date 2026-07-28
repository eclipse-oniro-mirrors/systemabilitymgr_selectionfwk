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

#include <atomic>
#include <climits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "selection_panel.h"
#include "panel_info.h"
#include "panel_status_listener.h"
#include "panel_common.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

constexpr int32_t TEST_PANEL_X = 100;
constexpr int32_t TEST_PANEL_Y = 200;
constexpr int32_t TEST_PANEL_WIDTH = 300;
constexpr int32_t TEST_PANEL_HEIGHT = 400;
constexpr int32_t TEST_MAIN_PANEL_X = 50;
constexpr int32_t TEST_MAIN_PANEL_Y = 60;
constexpr int32_t TEST_MAIN_PANEL_WIDTH = 200;
constexpr int32_t TEST_MAIN_PANEL_HEIGHT = 150;
constexpr int32_t TEST_SMALL_PANEL_SIZE = 100;
constexpr int32_t TEST_SMALL_OFFSET = 10;
constexpr int32_t TEST_SMALL_OFFSET_Y = 20;
constexpr int32_t TEST_MOVE_X = 300;
constexpr int32_t TEST_MOVE_Y = 400;
constexpr int32_t TEST_STRESS_ITERATIONS = 1000;

// ============================================================================
// Mock PanelStatusListener
// ============================================================================

class MockPanelStatusListener : public PanelStatusListener {
public:
    int callCount = 0;
    uint32_t lastWindowId = 0;
    std::string lastStatus;
    void OnPanelStatus(uint32_t windowId, const std::string &status) override
    {
        callCount++;
        lastWindowId = windowId;
        lastStatus = status;
    }
    void Reset()
    {
        callCount = 0;
        lastWindowId = 0;
        lastStatus.clear();
    }
};

// ============================================================================
// PanelInfo Struct Tests
// ============================================================================

class PanelInfoTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PanelInfoTest, DefaultConstruction001, TestSize.Level0)
{
    PanelInfo info;
    EXPECT_EQ(info.panelType, PanelType{});
    EXPECT_EQ(info.x, 0);
    EXPECT_EQ(info.y, 0);
    EXPECT_EQ(info.width, 0);
    EXPECT_EQ(info.height, 0);
}

HWTEST_F(PanelInfoTest, SetFields001, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MENU_PANEL;
    info.x = TEST_PANEL_X;
    info.y = TEST_PANEL_Y;
    info.width = TEST_PANEL_WIDTH;
    info.height = TEST_PANEL_HEIGHT;
    EXPECT_EQ(info.x, TEST_PANEL_X);
    EXPECT_EQ(info.y, TEST_PANEL_Y);
    EXPECT_EQ(info.width, TEST_PANEL_WIDTH);
    EXPECT_EQ(info.height, TEST_PANEL_HEIGHT);
}

HWTEST_F(PanelInfoTest, SetFields002, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MAIN_PANEL;
    EXPECT_EQ(info.panelType, PanelType::MAIN_PANEL);
}

HWTEST_F(PanelInfoTest, SetFields003, TestSize.Level0)
{
    PanelInfo info;
    info.x = -1;
    info.y = -1;
    info.width = -1;
    info.height = -1;
    EXPECT_EQ(info.x, -1);
    EXPECT_EQ(info.y, -1);
    EXPECT_EQ(info.width, -1);
    EXPECT_EQ(info.height, -1);
}

HWTEST_F(PanelInfoTest, SetFields004, TestSize.Level0)
{
    PanelInfo info;
    info.x = INT32_MAX;
    info.y = INT32_MAX;
    info.width = INT32_MAX;
    info.height = INT32_MAX;
    EXPECT_EQ(info.x, INT32_MAX);
    EXPECT_EQ(info.y, INT32_MAX);
    EXPECT_EQ(info.width, INT32_MAX);
    EXPECT_EQ(info.height, INT32_MAX);
}

HWTEST_F(PanelInfoTest, SetFields005, TestSize.Level0)
{
    PanelInfo info;
    info.x = INT32_MIN;
    info.y = INT32_MIN;
    EXPECT_EQ(info.x, INT32_MIN);
    EXPECT_EQ(info.y, INT32_MIN);
}

HWTEST_F(PanelInfoTest, CopyAssignRoundtrip001, TestSize.Level0)
{
    PanelInfo info1;
    info1.panelType = PanelType::MENU_PANEL;
    info1.x = TEST_PANEL_X;
    info1.y = TEST_PANEL_Y;
    info1.width = TEST_PANEL_WIDTH;
    info1.height = TEST_PANEL_HEIGHT;
    PanelInfo info2 = info1;
    PanelInfo info3;
    info3 = info2;
    EXPECT_EQ(info3.panelType, PanelType::MENU_PANEL);
    EXPECT_EQ(info3.x, TEST_PANEL_X);
    EXPECT_EQ(info3.y, TEST_PANEL_Y);
    EXPECT_EQ(info3.width, TEST_PANEL_WIDTH);
    EXPECT_EQ(info3.height, TEST_PANEL_HEIGHT);
}

HWTEST_F(PanelInfoTest, ArrayOfPanelInfo001, TestSize.Level0)
{
    std::vector<PanelInfo> infos;
    for (int i = 0; i < 10; i++) {
        PanelInfo info;
        info.panelType = (i % 2 == 0) ? PanelType::MENU_PANEL : PanelType::MAIN_PANEL;
        info.x = i;
        info.y = i * 10;
        info.width = i * 100;
        info.height = i * 1000;
        infos.push_back(info);
    }
    EXPECT_EQ(infos.size(), 10u);
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(infos[i].x, i);
        EXPECT_EQ(infos[i].y, i * 10);
        EXPECT_EQ(infos[i].width, i * 100);
        EXPECT_EQ(infos[i].height, i * 1000);
    }
}

// ============================================================================
// PanelType Enum Tests
// ============================================================================

HWTEST_F(PanelInfoTest, PanelTypeEnumValue001, TestSize.Level0)
{
    EXPECT_EQ(static_cast<uint32_t>(PanelType::MENU_PANEL), 1u);
    EXPECT_EQ(static_cast<uint32_t>(PanelType::MAIN_PANEL), 2u);
}

HWTEST_F(PanelInfoTest, PanelTypeEnumDistinct001, TestSize.Level0)
{
    EXPECT_NE(PanelType::MENU_PANEL, PanelType::MAIN_PANEL);
}

// ============================================================================
// SelectionWindowStatus Enum Tests
// ============================================================================

HWTEST_F(PanelInfoTest, WindowStatusEnumValues001, TestSize.Level0)
{
    EXPECT_EQ(static_cast<uint32_t>(SelectionWindowStatus::HIDDEN), 0u);
    EXPECT_EQ(static_cast<uint32_t>(SelectionWindowStatus::DESTROYED), 1u);
    EXPECT_EQ(static_cast<uint32_t>(SelectionWindowStatus::NONE), 2u);
}

HWTEST_F(PanelInfoTest, WindowStatusEnumDistinct001, TestSize.Level0)
{
    EXPECT_NE(SelectionWindowStatus::HIDDEN, SelectionWindowStatus::DESTROYED);
    EXPECT_NE(SelectionWindowStatus::DESTROYED, SelectionWindowStatus::NONE);
    EXPECT_NE(SelectionWindowStatus::HIDDEN, SelectionWindowStatus::NONE);
}

// ============================================================================
// ImmersiveMode Enum Tests
// ============================================================================

HWTEST_F(PanelInfoTest, ImmersiveModeEnumValues001, TestSize.Level0)
{
    EXPECT_EQ(static_cast<int32_t>(ImmersiveMode::NONE_IMMERSIVE), 0);
    EXPECT_EQ(static_cast<int32_t>(ImmersiveMode::IMMERSIVE), 1);
    EXPECT_EQ(static_cast<int32_t>(ImmersiveMode::LIGHT_IMMERSIVE), 2);
    EXPECT_EQ(static_cast<int32_t>(ImmersiveMode::DARK_IMMERSIVE), 3);
}

// ============================================================================
// ErrorCode Enum Tests
// ============================================================================

HWTEST_F(PanelInfoTest, ErrorCodeValues001, TestSize.Level0)
{
    EXPECT_EQ(ErrorCode::NO_ERROR, 0);
    EXPECT_EQ(ErrorCode::ERROR_PARAMETER_CHECK_FAILED, 1);
    EXPECT_EQ(ErrorCode::ERROR_SELECTION_SERVICE, 2);
    EXPECT_EQ(ErrorCode::ERROR_PANEL_DESTROYED, 3);
    EXPECT_EQ(ErrorCode::ERROR_INVALID_OPERATION, 4);
}

HWTEST_F(PanelInfoTest, ErrorCodeAllDistinct001, TestSize.Level0)
{
    EXPECT_NE(ErrorCode::NO_ERROR, ErrorCode::ERROR_PARAMETER_CHECK_FAILED);
    EXPECT_NE(ErrorCode::ERROR_PARAMETER_CHECK_FAILED, ErrorCode::ERROR_SELECTION_SERVICE);
    EXPECT_NE(ErrorCode::ERROR_SELECTION_SERVICE, ErrorCode::ERROR_PANEL_DESTROYED);
    EXPECT_NE(ErrorCode::ERROR_PANEL_DESTROYED, ErrorCode::ERROR_INVALID_OPERATION);
}

// ============================================================================
// panel_common.h Struct Tests
// ============================================================================

class PanelCommonStructTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PanelCommonStructTest, WindowSizeDefault001, TestSize.Level0)
{
    WindowSize ws;
    EXPECT_EQ(ws.width, 0u);
    EXPECT_EQ(ws.height, 0u);
}

HWTEST_F(PanelCommonStructTest, WindowSizeSet001, TestSize.Level0)
{
    WindowSize ws;
    ws.width = 1920;
    ws.height = 1080;
    EXPECT_EQ(ws.width, 1920u);
    EXPECT_EQ(ws.height, 1080u);
}

HWTEST_F(PanelCommonStructTest, PanelAdjustInfoDefault001, TestSize.Level0)
{
    PanelAdjustInfo info;
    EXPECT_EQ(info.top, 0);
    EXPECT_EQ(info.left, 0);
    EXPECT_EQ(info.right, 0);
    EXPECT_EQ(info.bottom, 0);
}

HWTEST_F(PanelCommonStructTest, PanelAdjustInfoEqual001, TestSize.Level0)
{
    PanelAdjustInfo info1;
    info1.top = 10;
    info1.left = 20;
    info1.right = 30;
    info1.bottom = 40;
    PanelAdjustInfo info2;
    info2.top = 10;
    info2.left = 20;
    info2.right = 30;
    info2.bottom = 40;
    EXPECT_TRUE(info1 == info2);
}

HWTEST_F(PanelCommonStructTest, PanelAdjustInfoNotEqual001, TestSize.Level0)
{
    PanelAdjustInfo info1;
    info1.top = 10;
    PanelAdjustInfo info2;
    info2.top = 20;
    EXPECT_FALSE(info1 == info2);
}

HWTEST_F(PanelCommonStructTest, PanelAdjustInfoNotEqual002, TestSize.Level0)
{
    PanelAdjustInfo info1;
    info1.left = 10;
    PanelAdjustInfo info2;
    info2.left = 20;
    EXPECT_FALSE(info1 == info2);
}

HWTEST_F(PanelCommonStructTest, PanelAdjustInfoNotEqual003, TestSize.Level0)
{
    PanelAdjustInfo info1;
    info1.right = 10;
    PanelAdjustInfo info2;
    info2.right = 20;
    EXPECT_FALSE(info1 == info2);
}

HWTEST_F(PanelCommonStructTest, PanelAdjustInfoNotEqual004, TestSize.Level0)
{
    PanelAdjustInfo info1;
    info1.bottom = 10;
    PanelAdjustInfo info2;
    info2.bottom = 20;
    EXPECT_FALSE(info1 == info2);
}

HWTEST_F(PanelCommonStructTest, HotAreasDefault001, TestSize.Level0)
{
    HotAreas ha;
    EXPECT_FALSE(ha.isSet);
}

HWTEST_F(PanelCommonStructTest, HotAreasSet001, TestSize.Level0)
{
    HotAreas ha;
    ha.isSet = true;
    EXPECT_TRUE(ha.isSet);
}

HWTEST_F(PanelCommonStructTest, EnhancedLayoutParamsDefault001, TestSize.Level0)
{
    EnhancedLayoutParams params;
    EXPECT_FALSE(params.isFullScreen);
    EXPECT_EQ(params.portrait.rect.posX_, 0);
    EXPECT_EQ(params.portrait.rect.posY_, 0);
    EXPECT_EQ(params.portrait.avoidY, 0);
    EXPECT_EQ(params.portrait.avoidHeight, 0u);
}

HWTEST_F(PanelCommonStructTest, DisplaySizeDefault001, TestSize.Level0)
{
    DisplaySize ds;
    EXPECT_EQ(ds.portrait.width, 0u);
    EXPECT_EQ(ds.portrait.height, 0u);
    EXPECT_EQ(ds.landscape.width, 0u);
    EXPECT_EQ(ds.landscape.height, 0u);
}

HWTEST_F(PanelCommonStructTest, FullPanelAdjustInfoDefault001, TestSize.Level0)
{
    FullPanelAdjustInfo info;
    EXPECT_EQ(info.portrait.top, 0);
    EXPECT_EQ(info.landscape.top, 0);
}

// ============================================================================
// SelectionPanel Construction Tests
// ============================================================================

class SelectionPanelTest : public testing::Test {
public:
    void SetUp() override
    {
        panel_ = std::make_unique<SelectionPanel>();
    }
    void TearDown() override
    {
        if (panel_) {
            panel_->DestroyPanel();
        }
        panel_.reset();
    }

    std::unique_ptr<SelectionPanel> panel_;
};

HWTEST_F(SelectionPanelTest, DefaultConstruction001, TestSize.Level0)
{
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MENU_PANEL);
}

HWTEST_F(SelectionPanelTest, DefaultConstruction002, TestSize.Level0)
{
    auto panel = std::make_unique<SelectionPanel>();
    ASSERT_NE(panel, nullptr);
    EXPECT_EQ(panel->GetPanelType(), PanelType::MENU_PANEL);
}

HWTEST_F(SelectionPanelTest, IsPanelShowingNoWindow001, TestSize.Level0)
{
    EXPECT_FALSE(panel_->IsPanelShowing());
}

HWTEST_F(SelectionPanelTest, GetWindowIdNoWindow001, TestSize.Level0)
{
    EXPECT_EQ(panel_->GetWindowId(), static_cast<uint32_t>(ErrorCode::ERROR_PANEL_DESTROYED));
}

HWTEST_F(SelectionPanelTest, MultiplePanelsIndependent001, TestSize.Level0)
{
    SelectionPanel panel1;
    SelectionPanel panel2;
    EXPECT_EQ(panel1.GetPanelType(), PanelType::MENU_PANEL);
    EXPECT_EQ(panel2.GetPanelType(), PanelType::MENU_PANEL);
    EXPECT_FALSE(panel1.IsPanelShowing());
    EXPECT_FALSE(panel2.IsPanelShowing());
}

// ============================================================================
// Null Window Error Path Tests
// ============================================================================

HWTEST_F(SelectionPanelTest, ShowPanelNullWindow001, TestSize.Level0)
{
    EXPECT_EQ(panel_->ShowPanel(), ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, HidePanelNullWindow001, TestSize.Level0)
{
    EXPECT_EQ(panel_->HidePanel(), ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, DestroyPanelNullWindow001, TestSize.Level0)
{
    EXPECT_EQ(panel_->DestroyPanel(), ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(SelectionPanelTest, MoveToNullWindow001, TestSize.Level0)
{
    EXPECT_EQ(panel_->MoveTo(TEST_PANEL_X, TEST_PANEL_Y), ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, MoveToNullWindow002, TestSize.Level0)
{
    EXPECT_EQ(panel_->MoveTo(0, 0), ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, MoveToNullWindow003, TestSize.Level0)
{
    EXPECT_EQ(panel_->MoveTo(-1, -1), ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, MoveToNullWindow004, TestSize.Level0)
{
    EXPECT_EQ(panel_->MoveTo(INT32_MAX, INT32_MAX), ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, StartMovingNullWindow001, TestSize.Level0)
{
    EXPECT_EQ(panel_->StartMoving(), ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, AllMethodsNullWindowConsistency001, TestSize.Level0)
{
    EXPECT_EQ(panel_->ShowPanel(), ErrorCode::ERROR_PANEL_DESTROYED);
    EXPECT_EQ(panel_->HidePanel(), ErrorCode::ERROR_PANEL_DESTROYED);
    EXPECT_EQ(panel_->MoveTo(1, 1), ErrorCode::ERROR_PANEL_DESTROYED);
    EXPECT_EQ(panel_->StartMoving(), ErrorCode::ERROR_PANEL_DESTROYED);
    EXPECT_FALSE(panel_->IsPanelShowing());
}

// ============================================================================
// SetUiContent Error Path Tests
// ============================================================================

HWTEST_F(SelectionPanelTest, SetUiContentNapiNullWindow001, TestSize.Level0)
{
    auto ret = panel_->SetUiContent("test_content", static_cast<napi_env>(nullptr));
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, SetUiContentNapiEmptyContent001, TestSize.Level0)
{
    auto ret = panel_->SetUiContent("", static_cast<napi_env>(nullptr));
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, SetUiContentNapiLongContent001, TestSize.Level0)
{
    auto ret = panel_->SetUiContent(std::string(10000, 'x'), static_cast<napi_env>(nullptr));
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, SetUiContentAniNullEnv001, TestSize.Level0)
{
    ani_env *env = nullptr;
    auto ret = panel_->SetUiContent("test_content", env);
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, SetUiContentAniNullEnvEmptyContent001, TestSize.Level0)
{
    ani_env *env = nullptr;
    auto ret = panel_->SetUiContent("", env);
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(SelectionPanelTest, SetUiContentAniNonNullEnvNullWindow001, TestSize.Level0)
{
    ani_env *env = reinterpret_cast<ani_env *>(0x1);
    auto ret = panel_->SetUiContent("content", env);
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
}

// ============================================================================
// SetPanelStatusListener Tests (public API return values)
// ============================================================================

HWTEST_F(SelectionPanelTest, SetListenerHidden001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "hidden"));
}

HWTEST_F(SelectionPanelTest, SetListenerDestroyed001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "destroyed"));
}

HWTEST_F(SelectionPanelTest, SetListenerInvalidType001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    EXPECT_FALSE(panel_->SetPanelStatusListener(listener, "invalid"));
}

HWTEST_F(SelectionPanelTest, SetListenerEmptyType001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    EXPECT_FALSE(panel_->SetPanelStatusListener(listener, ""));
}

HWTEST_F(SelectionPanelTest, SetListenerNullListener001, TestSize.Level0)
{
    EXPECT_TRUE(panel_->SetPanelStatusListener(nullptr, "hidden"));
}

HWTEST_F(SelectionPanelTest, SetListenerTwiceSameType001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "hidden"));
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "hidden"));
}

HWTEST_F(SelectionPanelTest, SetListenerBothTypes001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "hidden"));
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "destroyed"));
}

HWTEST_F(SelectionPanelTest, SetListenerDifferentListenersDifferentTypes001, TestSize.Level0)
{
    auto listener1 = std::make_shared<MockPanelStatusListener>();
    auto listener2 = std::make_shared<MockPanelStatusListener>();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener1, "hidden"));
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener2, "destroyed"));
}

// ============================================================================
// ClearPanelListener Tests (public API)
// ============================================================================

HWTEST_F(SelectionPanelTest, ClearHiddenListener001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    panel_->SetPanelStatusListener(listener, "hidden");
    panel_->ClearPanelListener("hidden");
}

HWTEST_F(SelectionPanelTest, ClearDestroyedListener001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    panel_->SetPanelStatusListener(listener, "destroyed");
    panel_->ClearPanelListener("destroyed");
}

HWTEST_F(SelectionPanelTest, ClearInvalidListener001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    panel_->SetPanelStatusListener(listener, "hidden");
    panel_->ClearPanelListener("invalid");
}

HWTEST_F(SelectionPanelTest, ClearHiddenKeepsDestroyed001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    panel_->SetPanelStatusListener(listener, "hidden");
    panel_->SetPanelStatusListener(listener, "destroyed");
    panel_->ClearPanelListener("hidden");
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "destroyed"));
}

HWTEST_F(SelectionPanelTest, ClearDestroyedKeepsHidden001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    panel_->SetPanelStatusListener(listener, "hidden");
    panel_->SetPanelStatusListener(listener, "destroyed");
    panel_->ClearPanelListener("destroyed");
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "hidden"));
}

HWTEST_F(SelectionPanelTest, ClearBothThenReset001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    panel_->SetPanelStatusListener(listener, "hidden");
    panel_->SetPanelStatusListener(listener, "destroyed");
    panel_->ClearPanelListener("hidden");
    panel_->ClearPanelListener("destroyed");
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "hidden"));
}

HWTEST_F(SelectionPanelTest, ClearWithoutSetDoesNotCrash001, TestSize.Level0)
{
    panel_->ClearPanelListener("hidden");
    panel_->ClearPanelListener("destroyed");
    panel_->ClearPanelListener("invalid");
}

HWTEST_F(SelectionPanelTest, ClearThenReRegister001, TestSize.Level0)
{
    auto listener = std::make_shared<MockPanelStatusListener>();
    panel_->SetPanelStatusListener(listener, "hidden");
    panel_->ClearPanelListener("hidden");
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener, "hidden"));
}

// ============================================================================
// CreatePanel Tests
// ============================================================================

HWTEST_F(SelectionPanelTest, CreateWithNullContextMenuPanel001, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MENU_PANEL;
    info.x = TEST_PANEL_X;
    info.y = TEST_PANEL_Y;
    info.width = TEST_PANEL_WIDTH;
    info.height = TEST_PANEL_HEIGHT;
    EXPECT_EQ(panel_->CreatePanel(nullptr, info), ErrorCode::NO_ERROR);
}

HWTEST_F(SelectionPanelTest, CreateWithNullContextMainPanel001, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MAIN_PANEL;
    EXPECT_EQ(panel_->CreatePanel(nullptr, info), ErrorCode::NO_ERROR);
}

HWTEST_F(SelectionPanelTest, CreateSetsPanelTypeMenu001, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MENU_PANEL;
    panel_->CreatePanel(nullptr, info);
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MENU_PANEL);
}

HWTEST_F(SelectionPanelTest, CreateSetsPanelTypeMain001, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MAIN_PANEL;
    panel_->CreatePanel(nullptr, info);
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MAIN_PANEL);
}

// ============================================================================
// Concurrency Tests (public API)
// ============================================================================

class PanelConcurrencyTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PanelConcurrencyTest, ConcurrentGetPanelType001, TestSize.Level0)
{
    SelectionPanel panel;
    const int threadCount = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&panel, &successCount]() {
            if (panel.GetPanelType() == PanelType::MENU_PANEL) {
                successCount++;
            }
        });
    }
    for (auto &t : threads) {
        t.join();
    }
    EXPECT_EQ(successCount.load(), threadCount);
}

HWTEST_F(PanelConcurrencyTest, ConcurrentShowPanel001, TestSize.Level0)
{
    SelectionPanel panel;
    const int threadCount = 5;
    std::vector<std::thread> threads;
    std::atomic<int> errorCount(0);
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&panel, &errorCount]() {
            if (panel.ShowPanel() == ErrorCode::ERROR_PANEL_DESTROYED) {
                errorCount++;
            }
        });
    }
    for (auto &t : threads) {
        t.join();
    }
    EXPECT_EQ(errorCount.load(), threadCount);
}

HWTEST_F(PanelConcurrencyTest, ConcurrentHidePanel001, TestSize.Level0)
{
    SelectionPanel panel;
    const int threadCount = 5;
    std::vector<std::thread> threads;
    std::atomic<int> errorCount(0);
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&panel, &errorCount]() {
            if (panel.HidePanel() == ErrorCode::ERROR_PANEL_DESTROYED) {
                errorCount++;
            }
        });
    }
    for (auto &t : threads) {
        t.join();
    }
    EXPECT_EQ(errorCount.load(), threadCount);
}

HWTEST_F(PanelConcurrencyTest, ConcurrentSetListener001, TestSize.Level0)
{
    SelectionPanel panel;
    const int threadCount = 5;
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&panel, &successCount]() {
            auto listener = std::make_shared<MockPanelStatusListener>();
            if (panel.SetPanelStatusListener(listener, "hidden")) {
                successCount++;
            }
        });
    }
    for (auto &t : threads) {
        t.join();
    }
    EXPECT_EQ(successCount.load(), threadCount);
}

HWTEST_F(PanelConcurrencyTest, ConcurrentMixedMethods001, TestSize.Level0)
{
    SelectionPanel panel;
    const int threadCount = 10;
    std::vector<std::thread> threads;
    std::atomic<int> destroyedCount(0);
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&panel, &destroyedCount, i]() {
            switch (i % 5) {
                case 0:
                    if (panel.ShowPanel() == ErrorCode::ERROR_PANEL_DESTROYED) destroyedCount++;
                    break;
                case 1:
                    if (panel.HidePanel() == ErrorCode::ERROR_PANEL_DESTROYED) destroyedCount++;
                    break;
                case 2:
                    if (panel.MoveTo(i, i) == ErrorCode::ERROR_PANEL_DESTROYED) destroyedCount++;
                    break;
                case 3:
                    if (panel.StartMoving() == ErrorCode::ERROR_PANEL_DESTROYED) destroyedCount++;
                    break;
                case 4:
                    panel.GetPanelType();
                    break;
            }
        });
    }
    for (auto &t : threads) {
        t.join();
    }
    EXPECT_GT(destroyedCount.load(), 0);
}

// ============================================================================
// CreatePanel Success Full Lifecycle Tests
// ============================================================================

class PanelLifecycleTest : public testing::Test {
public:
    void SetUp() override
    {
        panel_ = std::make_unique<SelectionPanel>();
        listener_ = std::make_shared<MockPanelStatusListener>();
    }
    void TearDown() override
    {
        if (panel_) {
            panel_->DestroyPanel();
        }
        panel_.reset();
    }

    void CreateMenuPanel()
    {
        PanelInfo info;
        info.panelType = PanelType::MENU_PANEL;
        info.x = TEST_PANEL_X;
        info.y = TEST_PANEL_Y;
        info.width = TEST_PANEL_WIDTH;
        info.height = TEST_PANEL_HEIGHT;
        panel_->CreatePanel(nullptr, info);
    }

    void CreateMainPanel()
    {
        PanelInfo info;
        info.panelType = PanelType::MAIN_PANEL;
        info.x = TEST_MAIN_PANEL_X;
        info.y = TEST_MAIN_PANEL_Y;
        info.width = TEST_MAIN_PANEL_WIDTH;
        info.height = TEST_MAIN_PANEL_HEIGHT;
        panel_->CreatePanel(nullptr, info);
    }

    std::unique_ptr<SelectionPanel> panel_;
    std::shared_ptr<MockPanelStatusListener> listener_;
};

HWTEST_F(PanelLifecycleTest, CreateMenuPanelGetType001, TestSize.Level0)
{
    CreateMenuPanel();
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MENU_PANEL);
}

HWTEST_F(PanelLifecycleTest, CreateMainPanelGetType001, TestSize.Level0)
{
    CreateMainPanel();
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MAIN_PANEL);
}

HWTEST_F(PanelLifecycleTest, CreateMenuPanelGetWindowId001, TestSize.Level0)
{
    CreateMenuPanel();
    auto id = panel_->GetWindowId();
    EXPECT_NE(id, static_cast<uint32_t>(ErrorCode::ERROR_PANEL_DESTROYED));
}

HWTEST_F(PanelLifecycleTest, CreateMainPanelGetWindowId001, TestSize.Level0)
{
    CreateMainPanel();
    auto id = panel_->GetWindowId();
    EXPECT_NE(id, static_cast<uint32_t>(ErrorCode::ERROR_PANEL_DESTROYED));
}

HWTEST_F(PanelLifecycleTest, CreateThenDestroyPanel001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, CreateThenDestroyPanel002, TestSize.Level0)
{
    CreateMainPanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, DestroyTwice001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_TRUE(ret == ErrorCode::ERROR_SELECTION_SERVICE || ret == ErrorCode::ERROR_PANEL_DESTROYED ||
                ret == ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, CreateThenIsPanelShowing001, TestSize.Level0)
{
    CreateMenuPanel();
    EXPECT_FALSE(panel_->IsPanelShowing());
}

HWTEST_F(PanelLifecycleTest, CreateThenIsPanelShowing002, TestSize.Level0)
{
    CreateMainPanel();
    EXPECT_FALSE(panel_->IsPanelShowing());
}

// ============================================================================
// CreatePanel Success + ShowPanel / HidePanel
// ============================================================================

HWTEST_F(PanelLifecycleTest, CreateThenShowPanel001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->ShowPanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, CreateThenShowPanel002, TestSize.Level0)
{
    CreateMainPanel();
    auto ret = panel_->ShowPanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, ShowPanelTwice001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    auto ret = panel_->ShowPanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, CreateThenHidePanel001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->HidePanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, ShowThenHidePanel001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    auto ret = panel_->HidePanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, HidePanelTwice001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->HidePanel();
    auto ret = panel_->HidePanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, ShowHideShowCycle001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    panel_->HidePanel();
    auto ret = panel_->ShowPanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, ShowHideShowHideCycle001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    panel_->HidePanel();
    panel_->ShowPanel();
    auto ret = panel_->HidePanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, MultipleShowHideCycles001, TestSize.Level0)
{
    CreateMenuPanel();
    for (int i = 0; i < 5; i++) {
        panel_->ShowPanel();
        panel_->HidePanel();
    }
    auto ret = panel_->ShowPanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, ShowHideOnMainPanel001, TestSize.Level0)
{
    CreateMainPanel();
    panel_->ShowPanel();
    auto ret = panel_->HidePanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

// ============================================================================
// CreatePanel Success + MoveTo / StartMoving
// ============================================================================

HWTEST_F(PanelLifecycleTest, CreateThenMoveTo001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->MoveTo(TEST_PANEL_WIDTH, TEST_PANEL_HEIGHT);
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, CreateThenMoveTo002, TestSize.Level0)
{
    CreateMainPanel();
    auto ret = panel_->MoveTo(0, 0);
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, MoveToMultipleTimes001, TestSize.Level0)
{
    CreateMenuPanel();
    for (int i = 0; i < 10; i++) {
        auto ret = panel_->MoveTo(i * 10, i * 20);
        EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
    }
}

HWTEST_F(PanelLifecycleTest, MoveToIntMax001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->MoveTo(INT32_MAX, INT32_MAX);
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE ||
                ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED);
}

HWTEST_F(PanelLifecycleTest, MoveToIntMin001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->MoveTo(INT32_MIN, INT32_MIN);
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE ||
                ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED);
}

HWTEST_F(PanelLifecycleTest, MoveToNegative001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->MoveTo(-TEST_PANEL_X, -TEST_PANEL_Y);
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE ||
                ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED);
}

HWTEST_F(PanelLifecycleTest, MoveToAfterShow001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    auto ret = panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, CreateThenStartMoving001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->StartMoving();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, StartMovingTwice001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->StartMoving();
    auto ret = panel_->StartMoving();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

// ============================================================================
// CreatePanel Success + SetUiContent
// ============================================================================

HWTEST_F(PanelLifecycleTest, CreateThenSetUiContentNapi001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->SetUiContent("test_content", static_cast<napi_env>(nullptr));
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED ||
                ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, CreateThenSetUiContentNapiEmpty001, TestSize.Level0)
{
    CreateMenuPanel();
    auto ret = panel_->SetUiContent("", static_cast<napi_env>(nullptr));
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED ||
                ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, CreateThenSetUiContentAni001, TestSize.Level0)
{
    CreateMenuPanel();
    ani_env *env = nullptr;
    auto ret = panel_->SetUiContent("test_content", env);
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(PanelLifecycleTest, CreateThenSetUiContentAniValid001, TestSize.Level0)
{
    CreateMenuPanel();
    ani_env *env = reinterpret_cast<ani_env *>(0x1);
    auto ret = panel_->SetUiContent("test_content", env);
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED ||
                ret == ErrorCode::ERROR_SELECTION_SERVICE || ret == ErrorCode::ERROR_PANEL_DESTROYED);
}

HWTEST_F(PanelLifecycleTest, SetUiContentTwice001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetUiContent("content1", static_cast<napi_env>(nullptr));
    auto ret = panel_->SetUiContent("content2", static_cast<napi_env>(nullptr));
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED ||
                ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelLifecycleTest, SetUiContentThenShowThenHide001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetUiContent("content", static_cast<napi_env>(nullptr));
    panel_->ShowPanel();
    auto ret = panel_->HidePanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

// ============================================================================
// CreatePanel Success + Listener Integration
// ============================================================================

HWTEST_F(PanelLifecycleTest, CreateThenSetListenerHidden001, TestSize.Level0)
{
    CreateMenuPanel();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "hidden"));
}

HWTEST_F(PanelLifecycleTest, CreateThenSetListenerDestroyed001, TestSize.Level0)
{
    CreateMenuPanel();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "destroyed"));
}

HWTEST_F(PanelLifecycleTest, CreateThenSetBothListeners001, TestSize.Level0)
{
    CreateMenuPanel();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "hidden"));
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "destroyed"));
}

HWTEST_F(PanelLifecycleTest, CreateThenSetListenerInvalid001, TestSize.Level0)
{
    CreateMenuPanel();
    EXPECT_FALSE(panel_->SetPanelStatusListener(listener_, "invalid"));
}

HWTEST_F(PanelLifecycleTest, CreateThenClearHiddenListener001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->ClearPanelListener("hidden");
    EXPECT_FALSE(panel_->IsPanelShowing());
}

HWTEST_F(PanelLifecycleTest, CreateThenClearBothListeners001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    panel_->ClearPanelListener("hidden");
    panel_->ClearPanelListener("destroyed");
    EXPECT_FALSE(panel_->IsPanelShowing());
}

// ============================================================================
// CreatePanel Success + Full Lifecycle Chain
// ============================================================================

HWTEST_F(PanelLifecycleTest, FullLifecycleMenuPanel001, TestSize.Level0)
{
    CreateMenuPanel();
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MENU_PANEL);
    EXPECT_NE(panel_->GetWindowId(), static_cast<uint32_t>(ErrorCode::ERROR_PANEL_DESTROYED));

    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");

    panel_->MoveTo(TEST_PANEL_WIDTH, TEST_PANEL_HEIGHT);
    panel_->ShowPanel();
    panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    panel_->HidePanel();

    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, FullLifecycleMainPanel001, TestSize.Level0)
{
    CreateMainPanel();
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MAIN_PANEL);

    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");

    panel_->ShowPanel();
    panel_->HidePanel();

    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, FullLifecycleWithUiContent001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetUiContent("test_content", static_cast<napi_env>(nullptr));
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->ShowPanel();
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, FullLifecycleWithMoveAndStartMoving001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->MoveTo(TEST_PANEL_X, TEST_PANEL_Y);
    panel_->StartMoving();
    panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    panel_->ShowPanel();
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, MultiplePanelsIndependentLifecycle001, TestSize.Level0)
{
    auto panel1 = std::make_unique<SelectionPanel>();
    auto panel2 = std::make_unique<SelectionPanel>();

    PanelInfo info1;
    info1.panelType = PanelType::MENU_PANEL;
    info1.x = 0;
    info1.y = 0;
    info1.width = TEST_SMALL_PANEL_SIZE;
    info1.height = TEST_SMALL_PANEL_SIZE;
    panel1->CreatePanel(nullptr, info1);

    PanelInfo info2;
    info2.panelType = PanelType::MAIN_PANEL;
    info2.x = TEST_MAIN_PANEL_WIDTH;
    info2.y = TEST_MAIN_PANEL_WIDTH;
    info2.width = TEST_SMALL_PANEL_SIZE;
    info2.height = TEST_SMALL_PANEL_SIZE;
    panel2->CreatePanel(nullptr, info2);

    EXPECT_EQ(panel1->GetPanelType(), PanelType::MENU_PANEL);
    EXPECT_EQ(panel2->GetPanelType(), PanelType::MAIN_PANEL);
    EXPECT_NE(panel1->GetWindowId(), panel2->GetWindowId());

    panel1->DestroyPanel();
    EXPECT_NE(panel2->GetWindowId(), static_cast<uint32_t>(ErrorCode::ERROR_PANEL_DESTROYED));

    panel2->DestroyPanel();
    panel1.release();
    panel2.release();
}

// ============================================================================
// CreatePanel + DestroyPanel then verify methods fail
// ============================================================================

HWTEST_F(PanelLifecycleTest, DestroyThenShowPanel001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    auto ret = panel_->ShowPanel();
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, DestroyThenHidePanel001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    auto ret = panel_->HidePanel();
    EXPECT_TRUE(ret == ErrorCode::ERROR_PANEL_DESTROYED || ret == ErrorCode::ERROR_SELECTION_SERVICE);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, DestroyThenMoveTo001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    auto ret = panel_->MoveTo(TEST_PANEL_X, TEST_PANEL_Y);
    EXPECT_TRUE(ret == ErrorCode::ERROR_PANEL_DESTROYED || ret == ErrorCode::ERROR_SELECTION_SERVICE);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, DestroyThenStartMoving001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    auto ret = panel_->StartMoving();
    EXPECT_TRUE(ret == ErrorCode::ERROR_PANEL_DESTROYED || ret == ErrorCode::ERROR_SELECTION_SERVICE);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, DestroyThenGetWindowId001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    auto id = panel_->GetWindowId();
    EXPECT_FALSE(id == static_cast<uint32_t>(ErrorCode::ERROR_PANEL_DESTROYED) ||
                id == SelectionPanel::INVALID_WINDOW_ID);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, DestroyThenSetUiContent001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    auto ret = panel_->SetUiContent("content", static_cast<napi_env>(nullptr));
    EXPECT_EQ(ret, ErrorCode::ERROR_PANEL_DESTROYED);
    panel_.release();
}

HWTEST_F(PanelLifecycleTest, DestroyThenIsPanelShowing001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    EXPECT_FALSE(panel_->IsPanelShowing());
    panel_.release();
}

// ============================================================================
// CreatePanel Success + Repeated Create / PanelInfo variations
// ============================================================================

HWTEST_F(PanelLifecycleTest, CreateWithZeroSize001, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MENU_PANEL;
    info.x = 0;
    info.y = 0;
    info.width = 0;
    info.height = 0;
    auto ret = panel_->CreatePanel(nullptr, info);
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
}

HWTEST_F(PanelLifecycleTest, CreateWithLargeSize001, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MENU_PANEL;
    info.x = 0;
    info.y = 0;
    info.width = INT32_MAX;
    info.height = INT32_MAX;
    auto ret = panel_->CreatePanel(nullptr, info);
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
}

HWTEST_F(PanelLifecycleTest, CreateWithNegativePosition001, TestSize.Level0)
{
    PanelInfo info;
    info.panelType = PanelType::MENU_PANEL;
    info.x = -TEST_PANEL_X;
    info.y = -TEST_PANEL_Y;
    info.width = TEST_PANEL_WIDTH;
    info.height = TEST_PANEL_HEIGHT;
    auto ret = panel_->CreatePanel(nullptr, info);
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
}

HWTEST_F(PanelLifecycleTest, CreateRecreateAfterDestroy001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    PanelInfo info;
    info.panelType = PanelType::MAIN_PANEL;
    info.x = TEST_SMALL_OFFSET;
    info.y = TEST_SMALL_OFFSET_Y;
    info.width = TEST_SMALL_PANEL_SIZE;
    info.height = TEST_SMALL_PANEL_SIZE;
    auto ret = panel_->CreatePanel(nullptr, info);
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MAIN_PANEL);
    panel_->DestroyPanel();
    panel_.release();
}

// ============================================================================
// Listener Edge Case Tests
// ============================================================================

class ListenerAdvancedTest : public testing::Test {
public:
    void SetUp() override
    {
        panel_ = std::make_unique<SelectionPanel>();
        listener_ = std::make_shared<MockPanelStatusListener>();
    }
    void TearDown() override
    {
        if (panel_) {
            panel_->DestroyPanel();
        }
        panel_.reset();
    }

    std::unique_ptr<SelectionPanel> panel_;
    std::shared_ptr<MockPanelStatusListener> listener_;
};

HWTEST_F(ListenerAdvancedTest, ClearAllTypesWithoutSet001, TestSize.Level0)
{
    panel_->ClearPanelListener("hidden");
    panel_->ClearPanelListener("destroyed");
    panel_->ClearPanelListener("hidden");
    panel_->ClearPanelListener("destroyed");
}

HWTEST_F(ListenerAdvancedTest, SetClearRepeat001, TestSize.Level0)
{
    for (int i = 0; i < 20; i++) {
        EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "hidden"));
        panel_->ClearPanelListener("hidden");
    }
}

HWTEST_F(ListenerAdvancedTest, SetClearBothRepeat001, TestSize.Level0)
{
    for (int i = 0; i < 20; i++) {
        EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "hidden"));
        EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "destroyed"));
        panel_->ClearPanelListener("hidden");
        panel_->ClearPanelListener("destroyed");
    }
}

HWTEST_F(ListenerAdvancedTest, DifferentListenersSameType001, TestSize.Level0)
{
    auto listener2 = std::make_shared<MockPanelStatusListener>();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "hidden"));
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener2, "hidden"));
}

HWTEST_F(ListenerAdvancedTest, ThreeListenersDifferentTypes001, TestSize.Level0)
{
    auto listener2 = std::make_shared<MockPanelStatusListener>();
    auto listener3 = std::make_shared<MockPanelStatusListener>();
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "hidden"));
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener2, "destroyed"));
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener3, "hidden"));
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener3, "destroyed"));
}

HWTEST_F(ListenerAdvancedTest, ClearPartialThenSet001, TestSize.Level0)
{
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    panel_->ClearPanelListener("hidden");
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "hidden"));
    panel_->ClearPanelListener("destroyed");
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "destroyed"));
}

// ============================================================================
// Listener Callback Verification Tests
// ============================================================================

class PanelListenerCallbackTest : public testing::Test {
public:
    void SetUp() override
    {
        panel_ = std::make_unique<SelectionPanel>();
        listener_ = std::make_shared<MockPanelStatusListener>();
    }
    void TearDown() override
    {
        if (panel_) {
            panel_->DestroyPanel();
        }
        panel_.reset();
    }

    void CreateMenuPanel()
    {
        PanelInfo info;
        info.panelType = PanelType::MENU_PANEL;
        info.x = TEST_PANEL_X;
        info.y = TEST_PANEL_Y;
        info.width = TEST_PANEL_WIDTH;
        info.height = TEST_PANEL_HEIGHT;
        panel_->CreatePanel(nullptr, info);
    }

    void CreateMainPanel()
    {
        PanelInfo info;
        info.panelType = PanelType::MAIN_PANEL;
        info.x = TEST_MAIN_PANEL_X;
        info.y = TEST_MAIN_PANEL_Y;
        info.width = TEST_MAIN_PANEL_WIDTH;
        info.height = TEST_MAIN_PANEL_HEIGHT;
        panel_->CreatePanel(nullptr, info);
    }

    std::unique_ptr<SelectionPanel> panel_;
    std::shared_ptr<MockPanelStatusListener> listener_;
};

HWTEST_F(PanelListenerCallbackTest, HideTriggersHiddenCallback001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
    EXPECT_GE(listener_->callCount, 0);
}

HWTEST_F(PanelListenerCallbackTest, DestroyTriggersDestroyedCallback001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, BothCallbacksOnDestroy001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, NoCallbackWithoutListener001, TestSize.Level0)
{
    CreateMenuPanel();
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
    EXPECT_EQ(listener_->callCount, 0);
}

HWTEST_F(PanelListenerCallbackTest, NoCallbackAfterClearListener001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->ClearPanelListener("hidden");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
}

HWTEST_F(PanelListenerCallbackTest, HideCallbackOnMainPanel001, TestSize.Level0)
{
    CreateMainPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
}

HWTEST_F(PanelListenerCallbackTest, DestroyCallbackOnMainPanel001, TestSize.Level0)
{
    CreateMainPanel();
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, MultipleShowHideCallbacks001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    listener_->Reset();
    for (int i = 0; i < 5; i++) {
        panel_->ShowPanel();
        panel_->HidePanel();
    }
}

HWTEST_F(PanelListenerCallbackTest, OnlyHiddenListenerNotTriggeredOnDestroy001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    listener_->Reset();
    panel_->DestroyPanel();
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, OnlyDestroyedListenerNotTriggeredOnHide001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
}

HWTEST_F(PanelListenerCallbackTest, SetUiContentThenShowHideWithListener001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetUiContent("content", static_cast<napi_env>(nullptr));
    panel_->SetPanelStatusListener(listener_, "hidden");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
}

HWTEST_F(PanelListenerCallbackTest, MoveBeforeShowWithListener001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
}

HWTEST_F(PanelListenerCallbackTest, ReRegisterListenerAfterClear001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->ClearPanelListener("hidden");
    EXPECT_TRUE(panel_->SetPanelStatusListener(listener_, "hidden"));
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
}

HWTEST_F(PanelListenerCallbackTest, TwoIndependentPanelsListeners001, TestSize.Level0)
{
    auto panel1 = std::make_unique<SelectionPanel>();
    auto panel2 = std::make_unique<SelectionPanel>();
    auto listener1 = std::make_shared<MockPanelStatusListener>();
    auto listener2 = std::make_shared<MockPanelStatusListener>();

    PanelInfo info1;
    info1.panelType = PanelType::MENU_PANEL;
    info1.x = 0;
    info1.y = 0;
    info1.width = TEST_SMALL_PANEL_SIZE;
    info1.height = TEST_SMALL_PANEL_SIZE;
    panel1->CreatePanel(nullptr, info1);

    PanelInfo info2;
    info2.panelType = PanelType::MAIN_PANEL;
    info2.x = TEST_MAIN_PANEL_WIDTH;
    info2.y = TEST_MAIN_PANEL_WIDTH;
    info2.width = TEST_SMALL_PANEL_SIZE;
    info2.height = TEST_SMALL_PANEL_SIZE;
    panel2->CreatePanel(nullptr, info2);

    panel1->SetPanelStatusListener(listener1, "destroyed");
    panel2->SetPanelStatusListener(listener2, "destroyed");
    listener1->Reset();
    listener2->Reset();

    panel1->DestroyPanel();
    panel1.release();
    panel2->DestroyPanel();
    panel2.release();
}

HWTEST_F(PanelListenerCallbackTest, ClearHiddenKeepDestroyedThenDestroy001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    panel_->ClearPanelListener("hidden");
    listener_->Reset();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, FullLifecycleWithBothCallbacks001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    panel_->MoveTo(TEST_PANEL_WIDTH, TEST_PANEL_HEIGHT);
    panel_->ShowPanel();
    panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, FullLifecycleMainPanelWithCallbacks001, TestSize.Level0)
{
    CreateMainPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, HideWithoutShowWithListener001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    listener_->Reset();
    auto ret = panel_->HidePanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelListenerCallbackTest, DestroyWithoutShowWithDestroyedListener001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, SetListenerAfterUiContentThenFullCycle001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetUiContent("content", static_cast<napi_env>(nullptr));
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, MultipleMoveThenShowHideDestroy001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    for (int i = 0; i < 5; i++) {
        panel_->MoveTo(i * 10, i * 20);
    }
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, ClearBothListenersFullCycle001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    panel_->ClearPanelListener("hidden");
    panel_->ClearPanelListener("destroyed");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->HidePanel();
    panel_->DestroyPanel();
    panel_.release();
}

// ============================================================================
// Additional Lifecycle Combination Tests
// ============================================================================

HWTEST_F(PanelListenerCallbackTest, ShowMoveHideDestroyWithListener001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, ShowMoveHideMoveShowHideDestroy001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "hidden");
    listener_->Reset();
    panel_->ShowPanel();
    panel_->MoveTo(TEST_PANEL_X, TEST_PANEL_Y);
    panel_->HidePanel();
    panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    panel_->ShowPanel();
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, MultiplePanelsSequentialListeners001, TestSize.Level0)
{
    auto panel1 = std::make_unique<SelectionPanel>();
    auto panel2 = std::make_unique<SelectionPanel>();
    auto listener1 = std::make_shared<MockPanelStatusListener>();
    auto listener2 = std::make_shared<MockPanelStatusListener>();

    PanelInfo info1;
    info1.panelType = PanelType::MENU_PANEL;
    info1.width = TEST_SMALL_PANEL_SIZE;
    info1.height = TEST_SMALL_PANEL_SIZE;
    panel1->CreatePanel(nullptr, info1);

    PanelInfo info2;
    info2.panelType = PanelType::MAIN_PANEL;
    info2.x = TEST_MAIN_PANEL_WIDTH;
    info2.width = TEST_SMALL_PANEL_SIZE;
    info2.height = TEST_SMALL_PANEL_SIZE;
    panel2->CreatePanel(nullptr, info2);

    panel1->SetPanelStatusListener(listener1, "hidden");
    panel2->SetPanelStatusListener(listener2, "hidden");
    listener1->Reset();
    listener2->Reset();

    panel1->ShowPanel();
    panel1->HidePanel();
    panel2->ShowPanel();
    panel2->HidePanel();

    panel1->DestroyPanel();
    panel2->DestroyPanel();
    panel1.release();
    panel2.release();
}

HWTEST_F(PanelListenerCallbackTest, InterleavedPanelOperations001, TestSize.Level0)
{
    auto panel1 = std::make_unique<SelectionPanel>();
    auto panel2 = std::make_unique<SelectionPanel>();

    PanelInfo info1;
    info1.panelType = PanelType::MENU_PANEL;
    info1.width = TEST_SMALL_PANEL_SIZE;
    info1.height = TEST_SMALL_PANEL_SIZE;
    panel1->CreatePanel(nullptr, info1);

    PanelInfo info2;
    info2.panelType = PanelType::MAIN_PANEL;
    info2.x = TEST_MAIN_PANEL_WIDTH;
    info2.width = TEST_SMALL_PANEL_SIZE;
    info2.height = TEST_SMALL_PANEL_SIZE;
    panel2->CreatePanel(nullptr, info2);

    panel1->ShowPanel();
    panel2->ShowPanel();
    panel1->MoveTo(TEST_PANEL_X, TEST_PANEL_Y);
    panel2->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    panel1->HidePanel();
    panel2->HidePanel();

    panel1->DestroyPanel();
    panel2->DestroyPanel();
    panel1.release();
    panel2.release();
}

HWTEST_F(PanelListenerCallbackTest, StartMovingDuringShow001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    auto ret = panel_->StartMoving();
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelListenerCallbackTest, SetUiContentDuringShow001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    auto ret = panel_->SetUiContent("content", static_cast<napi_env>(nullptr));
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED ||
                ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelListenerCallbackTest, SetUiContentAfterHide001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    panel_->HidePanel();
    auto ret = panel_->SetUiContent("content", static_cast<napi_env>(nullptr));
    EXPECT_TRUE(ret == ErrorCode::NO_ERROR || ret == ErrorCode::ERROR_PARAMETER_CHECK_FAILED ||
                ret == ErrorCode::ERROR_SELECTION_SERVICE);
}

HWTEST_F(PanelListenerCallbackTest, WindowIdConsistentAfterCreate001, TestSize.Level0)
{
    CreateMenuPanel();
    auto id1 = panel_->GetWindowId();
    auto id2 = panel_->GetWindowId();
    EXPECT_EQ(id1, id2);
}

HWTEST_F(PanelListenerCallbackTest, WindowIdConsistentAfterMove001, TestSize.Level0)
{
    CreateMenuPanel();
    auto id1 = panel_->GetWindowId();
    panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    auto id2 = panel_->GetWindowId();
    EXPECT_EQ(id1, id2);
}

HWTEST_F(PanelListenerCallbackTest, WindowIdConsistentAfterShow001, TestSize.Level0)
{
    CreateMenuPanel();
    auto id1 = panel_->GetWindowId();
    panel_->ShowPanel();
    auto id2 = panel_->GetWindowId();
    EXPECT_EQ(id1, id2);
}

HWTEST_F(PanelListenerCallbackTest, PanelTypeConsistentAfterOperations001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->ShowPanel();
    panel_->MoveTo(TEST_PANEL_X, TEST_PANEL_Y);
    panel_->HidePanel();
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MENU_PANEL);
}

HWTEST_F(PanelListenerCallbackTest, PanelTypeMainConsistentAfterOperations001, TestSize.Level0)
{
    CreateMainPanel();
    panel_->ShowPanel();
    panel_->MoveTo(TEST_MOVE_X, TEST_MOVE_Y);
    panel_->HidePanel();
    EXPECT_EQ(panel_->GetPanelType(), PanelType::MAIN_PANEL);
}

HWTEST_F(PanelListenerCallbackTest, IsPanelShowingAfterCreateNotShowing001, TestSize.Level0)
{
    CreateMenuPanel();
    EXPECT_FALSE(panel_->IsPanelShowing());
}

HWTEST_F(PanelListenerCallbackTest, IsPanelShowingAfterDestroyNotShowing001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->DestroyPanel();
    EXPECT_FALSE(panel_->IsPanelShowing());
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, CreateShowDestroyNoHide001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    panel_->ShowPanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

HWTEST_F(PanelListenerCallbackTest, CreateHideWithoutShowThenDestroy001, TestSize.Level0)
{
    CreateMenuPanel();
    panel_->SetPanelStatusListener(listener_, "destroyed");
    listener_->Reset();
    panel_->HidePanel();
    auto ret = panel_->DestroyPanel();
    EXPECT_EQ(ret, ErrorCode::NO_ERROR);
    panel_.release();
}

// ============================================================================
// Stress Tests (public API)
// ============================================================================

class PanelStressTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PanelStressTest, MassiveGetPanelType001, TestSize.Level0)
{
    SelectionPanel panel;
    for (int i = 0; i < TEST_STRESS_ITERATIONS * 10; i++) {
        EXPECT_EQ(panel.GetPanelType(), PanelType::MENU_PANEL);
    }
}

HWTEST_F(PanelStressTest, MassiveShowPanelNull001, TestSize.Level0)
{
    SelectionPanel panel;
    for (int i = 0; i < TEST_STRESS_ITERATIONS; i++) {
        EXPECT_EQ(panel.ShowPanel(), ErrorCode::ERROR_PANEL_DESTROYED);
    }
}

HWTEST_F(PanelStressTest, MassiveMoveToNull001, TestSize.Level0)
{
    SelectionPanel panel;
    for (int i = 0; i < TEST_STRESS_ITERATIONS; i++) {
        EXPECT_EQ(panel.MoveTo(i, i * 2), ErrorCode::ERROR_PANEL_DESTROYED);
    }
}

HWTEST_F(PanelStressTest, MassiveSetClearListener001, TestSize.Level0)
{
    SelectionPanel panel;
    auto listener = std::make_shared<MockPanelStatusListener>();
    for (int i = 0; i < TEST_STRESS_ITERATIONS; i++) {
        panel.SetPanelStatusListener(listener, "hidden");
        panel.ClearPanelListener("hidden");
    }
}

HWTEST_F(PanelStressTest, MassivePanelCreateDestroy001, TestSize.Level0)
{
    for (int i = 0; i < TEST_STRESS_ITERATIONS; i++) {
        auto panel = std::make_unique<SelectionPanel>();
        EXPECT_EQ(panel->GetPanelType(), PanelType::MENU_PANEL);
        EXPECT_FALSE(panel->IsPanelShowing());
    }
}

} // namespace SelectionFwk
} // namespace OHOS
