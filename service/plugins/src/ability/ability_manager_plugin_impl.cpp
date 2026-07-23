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

#include "ability_manager_plugin_impl.h"
#include "ability_manager_client.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

AbilityManagerPluginImpl::AbilityManagerPluginImpl()
    : initialized_(false)
{
    SELECTION_HILOGI("AbilityManagerPluginImpl constructor called");
}

AbilityManagerPluginImpl::~AbilityManagerPluginImpl()
{
    SELECTION_HILOGI("AbilityManagerPluginImpl destructor called");
    Cleanup();
}

bool AbilityManagerPluginImpl::Initialize()
{
    if (initialized_) {
        SELECTION_HILOGI("AbilityManagerPluginImpl already initialized");
        return true;
    }

    SELECTION_HILOGI("AbilityManagerPluginImpl::Initialize succeeded");
    initialized_ = true;
    return true;
}

void AbilityManagerPluginImpl::Cleanup()
{
    SELECTION_HILOGI("AbilityManagerPluginImpl::Cleanup");
    initialized_ = false;
}

const char* AbilityManagerPluginImpl::GetModuleName()
{
    return "AbilityManager";
}

int AbilityManagerPluginImpl::GetModuleVersion()
{
    return 1;
}

int32_t AbilityManagerPluginImpl::ConnectAbility(
    const AAFwk::Want& want,
    const sptr<AAFwk::AbilityConnectionStub>& callback,
    int32_t userId)
{
    SELECTION_HILOGI("AbilityManagerPluginImpl::ConnectAbility called, userId=%{public}d", userId);

    if (!initialized_) {
        SELECTION_HILOGE("AbilityManagerPlugin not initialized");
        return -1;
    }

    // 封装 AbilityManagerClient 调用
    auto ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, callback, userId);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("Failed to connect ability, ret=%{public}d", ret);
    } else {
        SELECTION_HILOGI("ConnectAbility succeeded, ret=%{public}d", ret);
    }

    return ret;
}

int32_t AbilityManagerPluginImpl::DisconnectAbility(
    const sptr<AAFwk::AbilityConnectionStub>& callback)
{
    SELECTION_HILOGI("AbilityManagerPluginImpl::DisconnectAbility called");

    if (!initialized_) {
        SELECTION_HILOGE("AbilityManagerPlugin not initialized");
        return -1;
    }

    // 封装 AbilityManagerClient 调用
    int32_t ret = AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(callback);
    if (ret != ERR_OK) {
        SELECTION_HILOGE("Failed to disconnect ability, ret=%{public}d", ret);
    } else {
        SELECTION_HILOGI("DisconnectAbility succeeded, ret=%{public}d", ret);
    }

    return ret;
}

bool AbilityManagerPluginImpl::IsAvailable() const
{
    return initialized_;
}

bool AbilityManagerPluginImpl::HealthCheck() const
{
    // 健康检查：AbilityManagerClient 应该总是可用的
    return initialized_;
}

const char* AbilityManagerPluginImpl::GetStatus() const
{
    if (!initialized_) {
        return "not_initialized";
    }
    return "available";
}

} // namespace SelectionFwk
} // namespace OHOS
