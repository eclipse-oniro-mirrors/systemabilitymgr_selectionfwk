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

#ifndef SELECTION_ABILITY_MANAGER_PLUGIN_IMPL_H
#define SELECTION_ABILITY_MANAGER_PLUGIN_IMPL_H

#include <string>
#include "iremote_object.h"
#include "ability_connect_callback_stub.h"

namespace OHOS {
namespace AAFwk {
class Want;
} // namespace AAFwk

namespace SelectionFwk {

// 能力管理插件实现（内部使用，通过C函数导出）
class AbilityManagerPluginImpl {
public:
    AbilityManagerPluginImpl();
    ~AbilityManagerPluginImpl();

    bool Initialize();
    void Cleanup();
    const char* GetModuleName();
    int GetModuleVersion();

    int32_t ConnectAbility(
        const AAFwk::Want& want,
        const sptr<AAFwk::AbilityConnectionStub>& callback,
        int32_t userId);

    int32_t DisconnectAbility(
        const sptr<AAFwk::AbilityConnectionStub>& callback);

    bool IsAvailable() const;
    bool HealthCheck() const;
    const char* GetStatus() const;

private:
    bool initialized_;
    mutable std::string status_;  // 插件状态描述
};

} // namespace SelectionFwk
} // namespace OHOS

#endif // SELECTION_ABILITY_MANAGER_PLUGIN_IMPL_H
