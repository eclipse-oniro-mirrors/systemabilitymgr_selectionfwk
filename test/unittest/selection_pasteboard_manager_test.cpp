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

#include "gtest/gtest.h"

#include "selection_pasteboard_manager.h"
#include "selection_errors.h"

namespace OHOS::SelectionFwk {

using namespace testing::ext;

class SelectionPasteboardManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SelectionPasteboardManagerTest::SetUpTestCase()
{
    std::cout << "SelectionPasteboardManagerTest SetUpTestCase" << std::endl;
}

void SelectionPasteboardManagerTest::TearDownTestCase()
{
    std::cout << "SelectionPasteboardManagerTest TearDownTestCase" << std::endl;
}

void SelectionPasteboardManagerTest::SetUp()
{
    std::cout << "SelectionPasteboardManagerTest SetUp" << std::endl;
}

void SelectionPasteboardManagerTest::TearDown()
{
    std::cout << "SelectionPasteboardManagerTest TearDown" << std::endl;
}

/**
 * @tc.name: SelectionPasteboardManager001
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with all whitespace
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager001, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    ASSERT_TRUE(observer.IsAllWhitespace(" "));
    ASSERT_TRUE(observer.IsAllWhitespace("\t"));
    ASSERT_TRUE(observer.IsAllWhitespace("\n"));
    ASSERT_TRUE(observer.IsAllWhitespace("\r"));
    ASSERT_TRUE(observer.IsAllWhitespace(" \t\n\r"));
    ASSERT_TRUE(observer.IsAllWhitespace("    "));
}

/**
 * @tc.name: SelectionPasteboardManager002
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with non-whitespace
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager002, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    ASSERT_FALSE(observer.IsAllWhitespace("a"));
    ASSERT_FALSE(observer.IsAllWhitespace("abc"));
    ASSERT_FALSE(observer.IsAllWhitespace(" a"));
    ASSERT_FALSE(observer.IsAllWhitespace("a "));
    ASSERT_FALSE(observer.IsAllWhitespace(" a "));
    ASSERT_FALSE(observer.IsAllWhitespace("hello world"));
}

/**
 * @tc.name: SelectionPasteboardManager003
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with empty string
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager003, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    ASSERT_TRUE(observer.IsAllWhitespace(""));
}

/**
 * @tc.name: SelectionPasteboardManager004
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with UTF-8 whitespace
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager004, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    // Test with various Unicode whitespace characters
    ASSERT_TRUE(observer.IsAllWhitespace(u8"\u00A0"));  // Non-breaking space
    ASSERT_TRUE(observer.IsAllWhitespace(u8"\u3000"));  // Ideographic space
    ASSERT_TRUE(observer.IsAllWhitespace(u8"\u2000"));  // En quad
}

/**
 * @tc.name: SelectionPasteboardManager005
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with UTF-8 non-whitespace
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager005, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    ASSERT_FALSE(observer.IsAllWhitespace("中文"));
    ASSERT_FALSE(observer.IsAllWhitespace(" hello"));
    ASSERT_FALSE(observer.IsAllWhitespace("hello "));
    ASSERT_FALSE(observer.IsAllWhitespace(" hello "));
    ASSERT_FALSE(observer.IsAllWhitespace("中文 "));
}

/**
 * @tc.name: SelectionPasteboardManager007
 * @tc.desc: test SelectionPasteboardManager constructor
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager007, TestSize.Level0)
{
    SelectionPasteboardManager manager;

    ASSERT_FALSE(manager.CanGetSelectionContent());
}

/**
 * @tc.name: SelectionPasteboardManager008
 * @tc.desc: test SelectionPasteboardManager Initialize
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager008, TestSize.Level0)
{
    std::shared_ptr<SelectionPasteboardManager> manager = std::make_shared<SelectionPasteboardManager>();

    ASSERT_TRUE(manager->Initialize());
}

/**
 * @tc.name: SelectionPasteboardManager009
 * @tc.desc: test SelectionPasteboardManager Initialize when already initialized
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager009, TestSize.Level0)
{
    std::shared_ptr<SelectionPasteboardManager> manager = std::make_shared<SelectionPasteboardManager>();

    ASSERT_TRUE(manager->Initialize());
    ASSERT_TRUE(manager->Initialize());
}

/**
 * @tc.name: SelectionPasteboardManager010
 * @tc.desc: test SelectionPasteboardManager CanGetSelectionContent after Initialize
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager010, TestSize.Level0)
{
    std::shared_ptr<SelectionPasteboardManager> manager = std::make_shared<SelectionPasteboardManager>();

    manager->Initialize();
    ASSERT_FALSE(manager->CanGetSelectionContent());
}

/**
 * @tc.name: SelectionPasteboardManager011
 * @tc.desc: test SelectionPasteboardManager SetCanGetSelectionContentFlag
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager011, TestSize.Level0)
{
    std::shared_ptr<SelectionPasteboardManager> manager = std::make_shared<SelectionPasteboardManager>();

    manager->Initialize();
    manager->SetCanGetSelectionContentFlag(true);
    ASSERT_TRUE(manager->CanGetSelectionContent());

    manager->SetCanGetSelectionContentFlag(false);
    ASSERT_FALSE(manager->CanGetSelectionContent());
}

/**
 * @tc.name: SelectionPasteboardManager012
 * @tc.desc: test SelectionPasteboardManager GetSelectionContent without Initialize
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager012, TestSize.Level0)
{
    SelectionPasteboardManager manager;

    std::string content;
    int32_t ret = manager.GetSelectionContent(content, 100, "test.bundle");
    ASSERT_EQ(ret, SelectionServiceError::INVALID_DATA);
}

/**
 * @tc.name: SelectionPasteboardManager017
 * @tc.desc: test SelectionPasteboardManager SetCanGetSelectionContentFlag without Initialize
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager017, TestSize.Level0)
{
    SelectionPasteboardManager manager;

    // Even without Initialize, SetCanGetSelectionContentFlag sets the flag
    manager.SetCanGetSelectionContentFlag(true);
    ASSERT_TRUE(manager.CanGetSelectionContent());

    // Can also set it back to false
    manager.SetCanGetSelectionContentFlag(false);
    ASSERT_FALSE(manager.CanGetSelectionContent());
}

/**
 * @tc.name: SelectionPasteboardManager019
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with mixed content
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager019, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    ASSERT_FALSE(observer.IsAllWhitespace("  a  "));
    ASSERT_FALSE(observer.IsAllWhitespace("\ta\n"));
    ASSERT_FALSE(observer.IsAllWhitespace(" \n\t\r x"));
}

/**
 * @tc.name: SelectionPasteboardManager020
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with tabs and newlines
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager020, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    ASSERT_TRUE(observer.IsAllWhitespace("\t\n\r\f\v"));
    ASSERT_TRUE(observer.IsAllWhitespace("\n\n\n"));
    ASSERT_TRUE(observer.IsAllWhitespace("\t\t\t"));
}

/**
 * @tc.name: SelectionPasteboardManager022
 * @tc.desc: test SelectionPasteboardManager SetCanGetSelectionContentFlag toggle
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager022, TestSize.Level0)
{
    std::shared_ptr<SelectionPasteboardManager> manager = std::make_shared<SelectionPasteboardManager>();
    manager->Initialize();

    for (int i = 0; i < 10; i++) {
        manager->SetCanGetSelectionContentFlag(true);
        ASSERT_TRUE(manager->CanGetSelectionContent());

        manager->SetCanGetSelectionContentFlag(false);
        ASSERT_FALSE(manager->CanGetSelectionContent());
    }
}

/**
 * @tc.name: SelectionPasteboardManager023
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with invalid UTF-8 bytes
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager023, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    ASSERT_FALSE(observer.IsAllWhitespace(std::string(1, '\x80')));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string(1, '\xBF')));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string(1, '\xF8')));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string(1, '\xFF')));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string(" \x80")));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string("\x80 ")));
}

/**
 * @tc.name: SelectionPasteboardManager024
 * @tc.desc: test SelectionPasteboardDisposableObserver IsAllWhitespace with truncated UTF-8 sequences
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager024, TestSize.Level0)
{
    SelectionPasteboardDisposableObserver observer;

    ASSERT_FALSE(observer.IsAllWhitespace(std::string("\xC2")));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string("\xE0\xA0")));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string("\xF0\x90\x80")));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string(" \xC2")));
    ASSERT_FALSE(observer.IsAllWhitespace(std::string("\xE0\xA0 ")));
}

/**
 * @tc.name: SelectionPasteboardManager025
 * @tc.desc: GetSelectionContent on uninitialized manager returns INVALID_DATA consistently across calls
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager025, TestSize.Level0)
{
    SelectionPasteboardManager manager;
    std::string content;
    int32_t ret1 = manager.GetSelectionContent(content, 100, "test.bundle");
    ASSERT_EQ(ret1, SelectionServiceError::INVALID_DATA);
    int32_t ret2 = manager.GetSelectionContent(content, 100, "test.bundle");
    ASSERT_EQ(ret2, SelectionServiceError::INVALID_DATA);
}

/**
 * @tc.name: SelectionPasteboardManager026
 * @tc.desc: Multiple sequential GetSelectionContent calls do not accumulate stale state
 * @tc.type: FUNC
 */
HWTEST_F(SelectionPasteboardManagerTest, SelectionPasteboardManager026, TestSize.Level0)
{
    SelectionPasteboardManager manager;
    std::string content;
    for (int i = 0; i < 5; i++) {
        int32_t ret = manager.GetSelectionContent(content, 100, "test.bundle");
        ASSERT_EQ(ret, SelectionServiceError::INVALID_DATA)
            << "Iteration " << i << " returned unexpected error code";
    }
}

} // namespace OHOS::SelectionFwk
