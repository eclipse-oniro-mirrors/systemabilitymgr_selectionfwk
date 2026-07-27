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

#ifndef PASTEBOARD_PLUGIN_IMPL_H
#define PASTEBOARD_PLUGIN_IMPL_H

#include <memory>
#include <string>

namespace OHOS {
namespace SelectionFwk {

class SelectionPasteboardManager;

// 剪贴板插件实现类（内部使用，通过C函数导出）
class PasteboardPluginImpl {
public:
    PasteboardPluginImpl() = default;
    ~PasteboardPluginImpl() = default;

    bool Initialize();
    void Cleanup();
    const char* GetModuleName();
    int GetModuleVersion();

    bool InitPasteboard();
    int32_t GetSelectionContent(std::string& content, uint32_t windowId, const std::string& bundleName);
    bool CanGetSelectionContent() const;
    void SetCanGetSelectionContentFlag(bool flag);
    bool IsAvailable() const;
    bool HealthCheck() const;
    const char* GetStatus() const;

private:
    std::shared_ptr<SelectionPasteboardManager> pasteboardManager_;
};

} // namespace SelectionFwk
} // namespace OHOS

#endif // PASTEBOARD_PLUGIN_IMPL_H
