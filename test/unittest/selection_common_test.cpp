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

#include <memory>
#include <optional>

#include "gtest/gtest.h"

#include "selection_common.h"
#include "ffrt_block_queue.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;

class SelectionCommonTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SelectionCommonTest::SetUpTestCase()
{
    std::cout << "SelectionCommonTest SetUpTestCase" << std::endl;
}

void SelectionCommonTest::TearDownTestCase()
{
    std::cout << "SelectionCommonTest TearDownTestCase" << std::endl;
}

void SelectionCommonTest::SetUp()
{
    std::cout << "SelectionCommonTest SetUp" << std::endl;
}

void SelectionCommonTest::TearDown()
{
    std::cout << "SelectionCommonTest TearDown" << std::endl;
}

/**
 * @tc.name: SelectionCommon003
 * @tc.desc: test IsNumber method with characters
 * @tc.type: FUNC
 */
HWTEST_F(SelectionCommonTest, SelectionCommon003, TestSize.Level0)
{
    std::string str;
    ASSERT_FALSE(IsNumber(str));
    ASSERT_FALSE(IsNumber("+"));
    ASSERT_FALSE(IsNumber("-"));
    ASSERT_FALSE(IsNumber("100a"));
    ASSERT_TRUE(IsNumber("100"));
}


/**
 * @tc.name: SelectionCommon004
 * @tc.desc: test ParseAppInfo method with strings
 * @tc.type: FUNC
 */
HWTEST_F(SelectionCommonTest, SelectionCommon004, TestSize.Level0)
{
    ASSERT_EQ(ParseAppInfo("a"), std::nullopt);
    ASSERT_EQ(ParseAppInfo("a/"), std::nullopt);
    ASSERT_EQ(ParseAppInfo("/a"), std::nullopt);
    std::string appInfoStr = "a/b";
    auto appInfo = ParseAppInfo(appInfoStr);
    ASSERT_NE(appInfo, std::nullopt);
    ASSERT_EQ(std::get<0>(appInfo.value()), "a");
    ASSERT_EQ(std::get<1>(appInfo.value()), "b");
}

/**
 * @tc.name: SelectionCommon005
 * @tc.desc: test operator==
 * @tc.type: FUNC
 */
HWTEST_F(SelectionCommonTest, SelectionCommon005, TestSize.Level0)
{
    AbilityRuntimeInfo abilityInfo{101, "a", "b"};
    AbilityRuntimeInfo &abilityInfo2 = abilityInfo;
    ASSERT_TRUE(abilityInfo == abilityInfo2);

    AbilityRuntimeInfo abilityInfo3{101, "c", "d"};
    ASSERT_FALSE(abilityInfo == abilityInfo3);

    AbilityRuntimeInfo abilityInfo4{101, "a", "d"};
    ASSERT_FALSE(abilityInfo == abilityInfo4);

    AbilityRuntimeInfo abilityInfo5{101, "a", "b"};
    ASSERT_TRUE(abilityInfo == abilityInfo5);
}

// ============================================================================
// FFRTBlockQueue Tests
// ============================================================================

constexpr uint32_t TEST_QUEUE_TIMEOUT_MS = 100;

class FFRTBlockQueueTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    std::unique_ptr<FFRTBlockQueue<int>> queue_;
};

void FFRTBlockQueueTest::SetUpTestCase()
{
    std::cout << "FFRTBlockQueueTest SetUpTestCase" << std::endl;
}

void FFRTBlockQueueTest::TearDownTestCase()
{
    std::cout << "FFRTBlockQueueTest TearDownTestCase" << std::endl;
}

void FFRTBlockQueueTest::SetUp()
{
    queue_ = std::make_unique<FFRTBlockQueue<int>>(TEST_QUEUE_TIMEOUT_MS);
}

void FFRTBlockQueueTest::TearDown()
{
    queue_.reset();
}

/**
 * @tc.name: FFRTBlockQueue001
 * @tc.desc: Remove front element from queue, remaining order preserved
 * @tc.type: FUNC
 */
HWTEST_F(FFRTBlockQueueTest, FFRTBlockQueue001, TestSize.Level0)
{
    queue_->Push(1);
    queue_->Push(2);
    queue_->Push(3);
    ASSERT_TRUE(queue_->Remove(1));
    int front;
    ASSERT_TRUE(queue_->GetFront(front));
    ASSERT_EQ(front, 2);
}

/**
 * @tc.name: FFRTBlockQueue002
 * @tc.desc: Remove middle element from queue, remaining order preserved
 * @tc.type: FUNC
 */
HWTEST_F(FFRTBlockQueueTest, FFRTBlockQueue002, TestSize.Level0)
{
    queue_->Push(1);
    queue_->Push(2);
    queue_->Push(3);
    ASSERT_TRUE(queue_->Remove(2));
    int front;
    ASSERT_TRUE(queue_->GetFront(front));
    ASSERT_EQ(front, 1);
    queue_->Pop();
    ASSERT_TRUE(queue_->GetFront(front));
    ASSERT_EQ(front, 3);
}

/**
 * @tc.name: FFRTBlockQueue003
 * @tc.desc: Remove non-existing element returns false, queue unchanged
 * @tc.type: FUNC
 */
HWTEST_F(FFRTBlockQueueTest, FFRTBlockQueue003, TestSize.Level0)
{
    queue_->Push(1);
    queue_->Push(2);
    ASSERT_FALSE(queue_->Remove(99));
    int front;
    ASSERT_TRUE(queue_->GetFront(front));
    ASSERT_EQ(front, 1);
}

/**
 * @tc.name: FFRTBlockQueue004
 * @tc.desc: Remove from empty queue returns false
 * @tc.type: FUNC
 */
HWTEST_F(FFRTBlockQueueTest, FFRTBlockQueue004, TestSize.Level0)
{
    ASSERT_FALSE(queue_->Remove(1));
}

/**
 * @tc.name: FFRTBlockQueue005
 * @tc.desc: Remove only element leaves queue empty
 * @tc.type: FUNC
 */
HWTEST_F(FFRTBlockQueueTest, FFRTBlockQueue005, TestSize.Level0)
{
    queue_->Push(42);
    ASSERT_TRUE(queue_->Remove(42));
    int front;
    ASSERT_FALSE(queue_->GetFront(front));
}

/**
 * @tc.name: FFRTBlockQueue006
 * @tc.desc: Remove with duplicates only removes first match
 * @tc.type: FUNC
 */
HWTEST_F(FFRTBlockQueueTest, FFRTBlockQueue006, TestSize.Level0)
{
    queue_->Push(1);
    queue_->Push(1);
    queue_->Push(2);
    ASSERT_TRUE(queue_->Remove(1));
    int front;
    ASSERT_TRUE(queue_->GetFront(front));
    ASSERT_EQ(front, 1);
    queue_->Pop();
    ASSERT_TRUE(queue_->GetFront(front));
    ASSERT_EQ(front, 2);
}

/**
 * @tc.name: FFRTBlockQueue007
 * @tc.desc: Pop on empty queue does not crash
 * @tc.type: FUNC
 */
HWTEST_F(FFRTBlockQueueTest, FFRTBlockQueue007, TestSize.Level0)
{
    queue_->Pop();
}

/**
 * @tc.name: FFRTBlockQueue008
 * @tc.desc: Pop after Remove emptied queue does not crash
 * @tc.type: FUNC
 */
HWTEST_F(FFRTBlockQueueTest, FFRTBlockQueue008, TestSize.Level0)
{
    queue_->Push(1);
    queue_->Remove(1);
    queue_->Pop();
}
}
}