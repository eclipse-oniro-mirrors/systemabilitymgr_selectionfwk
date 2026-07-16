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

#ifndef SELECTION_PASTEBOARD_MANAGER_H
#define SELECTION_PASTEBOARD_MANAGER_H

#include <string>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "pasteboard_client.h"
#include <linux/input.h>
#include <linux/uinput.h>
#include <future>
#include <atomic>
#include <refbase.h>

namespace OHOS::SelectionFwk {
using namespace OHOS::MiscServices;

// SelectionPasteboardDisposableObserver - moved from original module
class SelectionPasteboardDisposableObserver : public PasteboardDisposableObserver {
public:
    SelectionPasteboardDisposableObserver();
    virtual ~SelectionPasteboardDisposableObserver() = default;

    void SetBundleName(const std::string& bundleName);
    void OnTextReceived(const std::string &text, int32_t errCode) override;
    bool IsAllWhitespace(const std::string &str);

private:
    std::string bundleName_;
};

// Main manager class - encapsulates all pasteboard functionality
class SelectionPasteboardManager : public std::enable_shared_from_this<SelectionPasteboardManager> {
public:
    SelectionPasteboardManager();
    ~SelectionPasteboardManager();

    // Initialize the pasteboard manager with base input monitor
    bool Initialize();

    // Get selection content from pasteboard
    int32_t GetSelectionContent(std::string& selectionContent, uint32_t windowId, const std::string& bundleName);

    // Check if can get selection content
    bool CanGetSelectionContent() const;
    void SetCanGetSelectionContentFlag(bool flag);

    // Cleanup resources
    void Cleanup();

private:
    // Initialize virtual keyboard device
    bool InitUidev();

    // Inject Ctrl+C using virtual keyboard
    int32_t InjectCtrlC() const;

    // Wait for any pending async InjectCtrlC to complete
    void WaitForPendingAsync();

    // Convert pasteboard error codes to service error codes
    int32_t PasteBoardErrorCodeToSelectionService(int32_t pasteBoardErrCode);

    // Helper functions

    // Member variables
    sptr<SelectionPasteboardDisposableObserver> pasteboardObserver_;

    // Virtual keyboard
    bool initialized_;
    int32_t fd_;
    struct uinput_user_dev uidev_;
    std::atomic<bool> canGetSelectionContentFlag_;
    // Async injection state
    std::future<int32_t> injectCtrlCFuture_;
    std::atomic<bool> injectFailed_;
    std::atomic<bool> injectCtrlCRunning_;
    std::weak_ptr<SelectionPasteboardManager> self_weak_;
};

}

#endif // SELECTION_PASTEBOARD_MANAGER_H