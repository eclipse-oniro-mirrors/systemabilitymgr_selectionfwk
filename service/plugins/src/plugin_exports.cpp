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

#include "database_plugin_impl.h"
#include "pasteboard_plugin_impl.h"
#include "ability_manager_plugin_impl.h"
#include "selection_log.h"
#include "selection_errors.h"
#include "iremote_object.h"
#include "ability_connect_callback_stub.h"
#include "want.h"
#include "securec.h"
#include <memory>
#include <mutex>

using namespace OHOS;
using namespace OHOS::SelectionFwk;

// 静态插件实例，通过函数指针直接访问
namespace {
    // 数据库插件
    std::unique_ptr<DatabasePluginImpl> g_databasePlugin;
    std::mutex g_databaseMutex;
    bool g_databaseInitialized = false;

    // 剪贴板插件
    std::unique_ptr<PasteboardPluginImpl> g_pasteboardPlugin;
    std::mutex g_pasteboardMutex;
    bool g_pasteboardInitialized = false;

    // 能力管理插件
    std::unique_ptr<AbilityManagerPluginImpl> g_abilityManagerPlugin;
    std::mutex g_abilityManagerMutex;
    bool g_abilityManagerInitialized = false;

    // 确保插件初始化
    bool EnsureDatabasePlugin()
    {
        std::lock_guard<std::mutex> lock(g_databaseMutex);
        if (!g_databaseInitialized) {
            g_databasePlugin = std::make_unique<DatabasePluginImpl>();
            g_databaseInitialized = g_databasePlugin->Initialize();
            SELECTION_HILOGI("Database plugin %{public}s",
                             g_databaseInitialized ? "initialized" : "failed");
        }
        return g_databaseInitialized;
    }

    bool EnsurePasteboardPlugin()
    {
        std::lock_guard<std::mutex> lock(g_pasteboardMutex);
        if (!g_pasteboardInitialized) {
            g_pasteboardPlugin = std::make_unique<PasteboardPluginImpl>();
            g_pasteboardInitialized = g_pasteboardPlugin->Initialize();
            SELECTION_HILOGI("Pasteboard plugin %{public}s",
                             g_pasteboardInitialized ? "initialized" : "failed");
        }
        return g_pasteboardInitialized;
    }

    bool EnsureAbilityManagerPlugin()
    {
        std::lock_guard<std::mutex> lock(g_abilityManagerMutex);
        if (!g_abilityManagerInitialized) {
            g_abilityManagerPlugin = std::make_unique<AbilityManagerPluginImpl>();
            g_abilityManagerInitialized = g_abilityManagerPlugin->Initialize();
            SELECTION_HILOGI("AbilityManager plugin %{public}s",
                             g_abilityManagerInitialized ? "initialized" : "failed");
        }
        return g_abilityManagerInitialized;
    }
}

// 直接导出 C 函数，供 selection_service 和 selection_input_monitor 调用
extern "C" {
#pragma GCC visibility push(default)
// ==================== 数据库模块函数 ====================

int DatabaseSaveConfig(int uid, const SelectionConfig* config)
{
    if (!config) {
        SELECTION_HILOGE("DatabaseSaveConfig: config is null");
        return SELECTION_CONFIG_FAILURE;
    }
    if (!EnsureDatabasePlugin()) {
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    return g_databasePlugin->Save(uid, *config);
}

// 返回值: 0=成功, -1=未找到, -2=未初始化
int DatabaseGetConfig(int uid, SelectionConfig* config)
{
    if (!config) {
        SELECTION_HILOGE("DatabaseGetConfig: config is null");
        return SELECTION_CONFIG_FAILURE;
    }
    if (!EnsureDatabasePlugin()) {
        return SELECTION_CONFIG_RDB_NO_INIT;
    }
    auto result = g_databasePlugin->GetOneByUserId(uid);
    if (result.has_value()) {
        *config = result.value();
        return 0;
    }
    return SELECTION_CONFIG_NOT_FOUND;
}

int DatabaseIsAvailable()
{
    return g_databaseInitialized && g_databasePlugin &&
           g_databasePlugin->IsAvailable() ? 1 : 0;
}

// ==================== 剪贴板模块函数 ====================

int PasteboardGetSelectionContent(char* buffer, int bufferSize, uint32_t windowId, const char* bundleName)
{
    if (!buffer || bufferSize <= 0) {
        SELECTION_HILOGE("PasteboardGetSelectionContent: invalid buffer");
        return -1;
    }
    if (!EnsurePasteboardPlugin()) {
        return -1;
    }
    std::string content;
    std::string bundleNameStr = bundleName ? bundleName : "";
    int ret = g_pasteboardPlugin->GetSelectionContent(content, windowId, bundleNameStr);
    if (ret == 0) {
        int copyLen = std::min(bufferSize - 1, static_cast<int>(content.size()));
        errno_t err = memcpy_s(buffer, bufferSize, content.c_str(), copyLen);
        if (err != 0) {
            SELECTION_HILOGE("memcpy_s failed, err=%{public}d", err);
            return -1;
        }
        buffer[copyLen] = '\0';
    }
    return ret;
}

int PasteboardCanGetSelectionContent()
{
    if (!EnsurePasteboardPlugin()) {
        return 0;
    }
    return g_pasteboardPlugin->CanGetSelectionContent() ? 1 : 0;
}

void PasteboardSetCanGetSelectionContentFlag(int flag)
{
    if (!EnsurePasteboardPlugin()) {
        return;
    }
    g_pasteboardPlugin->SetCanGetSelectionContentFlag(flag != 0);
}

int PasteboardIsAvailable()
{
    return g_pasteboardInitialized && g_pasteboardPlugin ? 1 : 0;
}

// ==================== 能力管理模块函数 ====================

// Want 和 callback 作为指针传递，具体结构由调用方和实现方约定
int AbilityManagerConnectAbility(const void* want, const void* callback, int32_t userId)
{
    if (!want || !callback) {
        SELECTION_HILOGE("AbilityManagerConnectAbility: invalid parameters");
        return -1;
    }
    if (!EnsureAbilityManagerPlugin()) {
        return -1;
    }
    // 需要 reinterpret_cast 因为外部是 C 调用
    auto wantPtr = reinterpret_cast<const AAFwk::Want*>(want);
    auto callbackPtr = reinterpret_cast<const sptr<AAFwk::AbilityConnectionStub>*>(callback);
    return g_abilityManagerPlugin->ConnectAbility(*wantPtr, *callbackPtr, userId);
}

int AbilityManagerDisconnectAbility(const void* callback)
{
    if (!callback) {
        SELECTION_HILOGE("AbilityManagerDisconnectAbility: callback is null");
        return -1;
    }
    if (!EnsureAbilityManagerPlugin()) {
        return -1;
    }
    auto callbackPtr = reinterpret_cast<const sptr<AAFwk::AbilityConnectionStub>*>(callback);
    return g_abilityManagerPlugin->DisconnectAbility(*callbackPtr);
}

int AbilityManagerIsAvailable()
{
    return g_abilityManagerInitialized && g_abilityManagerPlugin->IsAvailable() ? 1 : 0;
}

// ==================== 插件生命周期管理函数 ====================

// 清理所有插件资源（在 dlclose 前调用）
void PluginCleanupAll()
{
    SELECTION_HILOGI("PluginCleanupAll called");

    if (g_databasePlugin) {
        g_databasePlugin->Cleanup();
        g_databasePlugin.reset();
        g_databaseInitialized = false;
    }

    if (g_pasteboardPlugin) {
        g_pasteboardPlugin->Cleanup();
        g_pasteboardPlugin.reset();
        g_pasteboardInitialized = false;
    }

    if (g_abilityManagerPlugin) {
        g_abilityManagerPlugin->Cleanup();
        g_abilityManagerPlugin.reset();
        g_abilityManagerInitialized = false;
    }

    SELECTION_HILOGI("PluginCleanupAll completed");
}

#pragma GCC visibility pop
} // extern "C"
