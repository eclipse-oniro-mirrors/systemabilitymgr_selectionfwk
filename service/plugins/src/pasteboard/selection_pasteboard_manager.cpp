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

#include "selection_pasteboard_manager.h"
#include "selection_common.h"
#include "hisysevent_adapter.h"
#include <sys/time.h>
#include <cstring>
#include <chrono>
#include <atomic>
#include <fcntl.h>
#include <future>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include "selection_log.h"
#include "selection_errors.h"
 
namespace OHOS::SelectionFwk {

// SelectionPasteboardDisposableObserver implementation
SelectionPasteboardDisposableObserver::SelectionPasteboardDisposableObserver() = default;

void SelectionPasteboardDisposableObserver::SetBundleName(const std::string& bundleName)
{
    bundleName_ = bundleName;
}

// Constants moved from original module
constexpr const uint32_t MAX_PASTERBOARD_TEXT_LENGTH = 2000;
constexpr const uint32_t BYTES_PER_CHINESE_CHAR = 3;
constexpr const uint32_t MAX_DELAY_WAIT_FOR_PB = 110;
constexpr int32_t PB_ERR_OUT_OF_RANGE = 5;
constexpr int32_t PB_ERR_CANNOT_GET_CONTENT = 7;
 
static const unsigned int UTF8_2BYTE_LEN = 2;
static const unsigned int UTF8_3BYTE_LEN = 3;
static const unsigned int UTF8_4BYTE_LEN = 4;
 
const unsigned int SLEEP_USEC_AFTER_CTRL_DOWN = 1000;
const unsigned int SLEEP_USEC_AFTER_C_DOWN = 1000;
const unsigned int SLEEP_USEC_AFTER_C_UP = 60000;
 
// Threading
std::mutex mtx_;
std::condition_variable cv_;
std::string g_selectionContent;
int32_t g_pasteBoardErrorCode;
 
// SelectionPasteboardDisposableObserver implementation
 
bool SelectionPasteboardDisposableObserver::IsAllWhitespace(const std::string &str)
{
    static const std::string invisibleChars =
        " \t\n\r\f\v"
        "\u00A0\u1680\u180E"
        "\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A"
        "\u200B\u200C\u200D\u200E\u200F"
        "\u2028\u2029\u202A\u202B\u202C\u202D\u202E"
        "\u205F\u2060\u2061\u2062\u2063\u2064"
        "\u3000\uFEFF";
 
    for (size_t i = 0; i < str.size();) {
        if (static_cast<unsigned char>(str[i]) < 0x80) {
            if (invisibleChars.find(str[i]) == std::string::npos) {
                return false;
            }
            ++i;
        } else {
            uint32_t len = 0;
            if ((str[i] & 0xE0) == 0xC0) {
                len = UTF8_2BYTE_LEN;
            } else if ((str[i] & 0xF0) == 0xE0) {
                len = UTF8_3BYTE_LEN;
            } else if ((str[i] & 0xF8) == 0xF0) {
                len = UTF8_4BYTE_LEN;
            } else {
                return false;
            }

            if (i + len > str.size()) {
                return false;
            }

            std::string utf8Char = str.substr(i, len);
            if (invisibleChars.find(utf8Char) == std::string::npos) {
                return false;
            }

            i += len;
        }
    }
 
    return true;
}
 
void SelectionPasteboardDisposableObserver::OnTextReceived(const std::string &text, int32_t errCode)
{
    SELECTION_HILOGW("[selectevent] Pasteboard call sa. Text received "
        "length: %{public}zu, errCode: %{public}d.", text.length(), errCode);
    if (errCode != 0) {
        HisyseventAdapter::GetInstance()->ReportShowPanelFailed(bundleName_, errCode,
            static_cast<int32_t>(SelectFailedReason::TEXT_RECEIVE_FAILED));
        SELECTION_HILOGE("Error receiving text, errCode: %{public}d", errCode);
    }

    if (IsAllWhitespace(text)) {
        SELECTION_HILOGI("Received empty text or all whitespaces.");
    }
    SELECTION_HILOGI("Notify SelectionPasteboardManager return text");
    std::lock_guard<std::mutex> lock(mtx_);
    g_selectionContent = text;
    g_pasteBoardErrorCode = errCode;
    cv_.notify_one();
}
 
// SelectionPasteboardManager implementation
SelectionPasteboardManager::SelectionPasteboardManager()
    : initialized_(false), fd_(-1), canGetSelectionContentFlag_(false), injectFailed_(false), injectCtrlCRunning_(false)
{
}
 
SelectionPasteboardManager::~SelectionPasteboardManager()
{
    WaitForPendingAsync();
    Cleanup();
}
 
bool SelectionPasteboardManager::Initialize()
{
    if (initialized_) {
        SELECTION_HILOGI("SelectionPasteboardManager already initialized");
        return true;
    }
    self_weak_ = shared_from_this(); // 确保对象已构造完成
    pasteboardObserver_ = sptr<SelectionPasteboardDisposableObserver>::MakeSptr();
 
    // Initialize virtual keyboard device (moved from original InitUidev)
    if (!InitUidev()) {
        SELECTION_HILOGE("Failed to initialize uidev");
        return false;
    }
 
    initialized_ = true;
    return true;
}
 
bool SelectionPasteboardManager::CanGetSelectionContent() const
{
    return canGetSelectionContentFlag_.load();
}
 
void SelectionPasteboardManager::SetCanGetSelectionContentFlag(bool flag)
{
    canGetSelectionContentFlag_.store(flag);
}
 
void SelectionPasteboardManager::Cleanup()
{
    if (fd_ == -1) {
        return;
    }
    if (ioctl(fd_, UI_DEV_DESTROY) < 0) {
        SELECTION_HILOGW("Failed to destroy virtual device.");
    }
    close(fd_);
    fd_ = -1;
    initialized_ = false;
}
 
bool SelectionPasteboardManager::InitUidev()
{
    SELECTION_HILOGI("Begin to init uidev.");
    fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        SELECTION_HILOGE("Failed to open /dev/uinput.");
        return false;
    }
    SELECTION_HILOGI("Opened /dev/uinput with fd %{public}d.", fd_);
 
    if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) < 0) {
        SELECTION_HILOGE("Unable to set EV_KEY event bit.");
        goto CLEAN;
    }
 
    if (ioctl(fd_, UI_SET_KEYBIT, KEY_LEFTCTRL) < 0) {
        SELECTION_HILOGE("Unable to set KEY_LEFTCTRL event bit.");
        goto CLEAN;
    }
    if (ioctl(fd_, UI_SET_KEYBIT, KEY_C) < 0) {
        SELECTION_HILOGE("Unable to set KEY_C event bit.");
        goto CLEAN;
    }
 
    memset_s(&uidev_, sizeof(uidev_), 0, sizeof(uidev_));
    if (snprintf_s(uidev_.name, UINPUT_MAX_NAME_SIZE, UINPUT_MAX_NAME_SIZE, "Selection VKeyboard") < 0) {
        SELECTION_HILOGE("Invalid arguments passed to snprintf_s.");
        goto CLEAN;
    }
    uidev_.id.bustype = BUS_USB;
    uidev_.id.vendor  = 0x1;
    uidev_.id.product = 0x1;
    uidev_.id.version = 1;
 
    if (write(fd_, &uidev_, sizeof(uidev_)) < 0) {
        SELECTION_HILOGE("Failed to write device config.");
        goto CLEAN;
    }
 
    if (ioctl(fd_, UI_DEV_CREATE) < 0) {
        SELECTION_HILOGE("Failed to create virtual device.");
        goto CLEAN;
    }
    SELECTION_HILOGI("End up init uidev.");
    return true;
 
CLEAN:
    SELECTION_HILOGE("Failed to init uidev, clean up fd now.");
    close(fd_);
    fd_ = -1;
    return false;
}
 
int32_t SelectionPasteboardManager::InjectCtrlC() const
{
    SELECTION_HILOGI("InjectCtrlC to /dev/uinput using fd %{public}d.", fd_);
    if (fd_ == -1) {
        SELECTION_HILOGE("fd of /dev/uinput is invalid, skip injecting ctrl c.");
        return SelectionServiceError::INVALID_DATA;
    }
 
    struct input_event ev;
    auto sendEvent = [&](int type, int code, int value) {
        memset_s(&ev, sizeof(ev), 0, sizeof(ev));
        struct timeval time{};
        gettimeofday(&time, NULL);
        ev.input_event_sec = time.tv_sec;
        ev.input_event_usec = time.tv_usec;
        ev.type = type;
        ev.code = code;
        ev.value = value;
        if (write(fd_, &ev, sizeof(ev)) < 0) {
            SELECTION_HILOGE("Failed to send event {type=%{public}d, code=%{public}d, value=%{public}d}",
                type, code, value);
        }
    };
 
    sendEvent(EV_KEY, KEY_LEFTCTRL, 1);
    sendEvent(EV_SYN, SYN_REPORT, 0);
    usleep(SLEEP_USEC_AFTER_CTRL_DOWN);
 
    sendEvent(EV_KEY, KEY_C, 1);
    sendEvent(EV_SYN, SYN_REPORT, 0);
    usleep(SLEEP_USEC_AFTER_C_DOWN);
 
    sendEvent(EV_KEY, KEY_C, 0);
    sendEvent(EV_SYN, SYN_REPORT, 0);
    usleep(SLEEP_USEC_AFTER_C_UP);
 
    sendEvent(EV_KEY, KEY_LEFTCTRL, 0);
    sendEvent(EV_SYN, SYN_REPORT, 0);
 
    SELECTION_HILOGI("End up InjectCtrlC to /dev/uinput.");
    return ERR_OK;
}
 
int32_t SelectionPasteboardManager::PasteBoardErrorCodeToSelectionService(int32_t pasteBoardErrCode)
{
    int32_t ret = ERR_OK;
    switch (pasteBoardErrCode) {
        case ERR_OK:
            SELECTION_HILOGI("Pasteboard called OnTextReceived timeout.");
            ret = SelectionServiceError::GET_CONTENT_TIMEOUT;
            break;
        case PB_ERR_OUT_OF_RANGE:
            SELECTION_HILOGE("Selection content out of range");
            ret = SelectionServiceError::CONTENT_OUT_OF_RANGE;
            break;
        case PB_ERR_CANNOT_GET_CONTENT:
            SELECTION_HILOGE("Current application forbid to copy.");
            ret = SelectionServiceError::CANNOT_GET_CONTENT;
            break;
        default:
            SELECTION_HILOGE("Some other error when receive content from pasteboard.");
            ret = SelectionServiceError::INVALID_DATA;
    }
    return ret;
}

void SelectionPasteboardManager::WaitForPendingAsync()
{
    if (injectCtrlCFuture_.valid()) {
        SELECTION_HILOGI("Waiting for pending async InjectCtrlC to complete.");
        injectCtrlCFuture_.wait();
        injectCtrlCRunning_.store(false);
    }
}

int32_t SelectionPasteboardManager::GetSelectionContent(std::string& selectionContent, uint32_t windowId,
    const std::string& bundleName)
{
    SELECTION_HILOGI("SelectionPasteboardManager::GetSelectionContent start");
    if (!initialized_) {
        SELECTION_HILOGE("SelectionPasteboardManager not initialized is nullptr");
        return SelectionServiceError::INVALID_DATA;
    }
    // 在开始新的异步注入之前，确保之前的异步注入已完成
    WaitForPendingAsync();
    pasteboardObserver_->SetBundleName(bundleName);
    int32_t ret = PasteboardClient::GetInstance()->SubscribeDisposableObserver(pasteboardObserver_,
        windowId, DisposableType::PLAIN_TEXT, MAX_PASTERBOARD_TEXT_LENGTH * BYTES_PER_CHINESE_CHAR);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("Failed to SubscribeDisposableObserver, ret: %{public}d.", ret);
        return SelectionServiceError::INVALID_DATA;
    }
    // ---- 异步注入CtrlC ----
    injectFailed_.store(false);
    injectCtrlCRunning_.store(true);
    injectCtrlCFuture_ = std::async(std::launch::async, [self_weak = self_weak_]() -> int32_t {
        auto self = self_weak.lock(); // 尝试获取 shared_ptr
        if (!self) {
            return SelectionServiceError::INVALID_DATA; // 对象已销毁
        }
        int32_t result = self->InjectCtrlC();
        if (result != ERR_OK) {
            self->injectFailed_.store(true);
            cv_.notify_one();
        }
        self->injectCtrlCRunning_.store(false);
        return result;
    });
 
    std::unique_lock<std::mutex> lock(mtx_);
    SELECTION_HILOGI("Start wait for pasteboard OnTextReceived (async InjectCtrlC)");
    bool received = cv_.wait_for(lock, std::chrono::milliseconds(MAX_DELAY_WAIT_FOR_PB),
        [this] {
            return !g_selectionContent.empty() || injectFailed_.load();
        });
    if (received && !g_selectionContent.empty()) {
        selectionContent = g_selectionContent;
        g_selectionContent = "";
        ret = ERR_OK;
        SELECTION_HILOGI("receive text success");
    } else {
        // 超时或注入失败
        if (injectFailed_.load()) {
            // 注入CtrlC失败—从未来获取错误代码
            int32_t injectResult = SelectionServiceError::INVALID_DATA;
            if (injectCtrlCFuture_.valid()) {
                injectResult = injectCtrlCFuture_.get();
            }
            HisyseventAdapter::GetInstance()->ReportShowPanelFailed(
                bundleName, injectResult,
                static_cast<int32_t>(SelectFailedReason::INJECT_CTRLC_FAILED));
            SELECTION_HILOGE("Async InjectCtrlC failed: %{public}d", injectResult);
            ret = SelectionServiceError::INVALID_DATA;
        } else {
            // 等待剪贴板回调超时
            SELECTION_HILOGE("GetSelectionContent: receive content from pasteboard failed (timeout)");
            ret = PasteBoardErrorCodeToSelectionService(g_pasteBoardErrorCode);
        }
        g_pasteBoardErrorCode = ERR_OK;
    }
    return ret;
}

}