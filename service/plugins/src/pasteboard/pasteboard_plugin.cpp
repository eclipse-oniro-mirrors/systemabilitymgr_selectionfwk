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

#include "pasteboard_plugin_impl.h"
#include "selection_pasteboard_manager.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

bool PasteboardPluginImpl::Initialize()
{
    SELECTION_HILOGI("PasteboardPluginImpl::Initialize called");
    pasteboardManager_ = std::make_shared<SelectionPasteboardManager>();
    bool success = pasteboardManager_ != nullptr && pasteboardManager_->Initialize();
    SELECTION_HILOGI("PasteboardPluginImpl::Initialize %{public}s", success ? "succeeded" : "failed");
    return success;
}

void PasteboardPluginImpl::Cleanup()
{
    SELECTION_HILOGI("PasteboardPluginImpl::Cleanup called");
    if (pasteboardManager_) {
        pasteboardManager_->Cleanup();
        pasteboardManager_.reset();
    }
    SELECTION_HILOGI("PasteboardPluginImpl::Cleanup completed");
}

const char* PasteboardPluginImpl::GetModuleName()
{
    return "Pasteboard";
}

int PasteboardPluginImpl::GetModuleVersion()
{
    return 1;
}

bool PasteboardPluginImpl::InitPasteboard()
{
    SELECTION_HILOGI("PasteboardPluginImpl::InitPasteboard called");
    if (pasteboardManager_ == nullptr) {
        SELECTION_HILOGE("PasteboardManager not initialized");
        return false;
    }
    // The manager is already initialized in Initialize()
    return true;
}

int32_t PasteboardPluginImpl::GetSelectionContent(std::string& content, uint32_t windowId,
    const std::string& bundleName)
{
    SELECTION_HILOGI("PasteboardPluginImpl::GetSelectionContent called, windowId=%{public}u", windowId);
    if (pasteboardManager_ == nullptr) {
        SELECTION_HILOGE("PasteboardManager not initialized");
        return -1;
    }
    return pasteboardManager_->GetSelectionContent(content, windowId, bundleName);
}

bool PasteboardPluginImpl::CanGetSelectionContent() const
{
    if (pasteboardManager_ == nullptr) {
        SELECTION_HILOGE("PasteboardManager not initialized");
        return false;
    }
    return pasteboardManager_->CanGetSelectionContent();
}

void PasteboardPluginImpl::SetCanGetSelectionContentFlag(bool flag)
{
    SELECTION_HILOGI("PasteboardPluginImpl::SetCanGetSelectionContentFlag called, flag=%{public}d", flag);
    if (pasteboardManager_ != nullptr) {
        pasteboardManager_->SetCanGetSelectionContentFlag(flag);
    }
}

bool PasteboardPluginImpl::IsAvailable() const
{
    return pasteboardManager_ != nullptr;
}

bool PasteboardPluginImpl::HealthCheck() const
{
    if (pasteboardManager_ == nullptr) {
        return false;
    }
    // 健康检查：管理器已初始化
    return true;
}

const char* PasteboardPluginImpl::GetStatus() const
{
    if (pasteboardManager_ == nullptr) {
        return "uninitialized";
    }
    return "available";
}

} // namespace SelectionFwk
} // namespace OHOS
