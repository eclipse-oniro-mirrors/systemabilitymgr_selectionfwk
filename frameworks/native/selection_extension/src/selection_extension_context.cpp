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

#include "selection_extension_context.h"
#include "ability_manager_client.h"
#include "selection_extension_hilog.h"

namespace OHOS::AbilityRuntime {
const size_t SelectionExtensionContext::CONTEXT_TYPE_ID(std::hash<const char*> {}("SelectionExtensionContext"));
int32_t SelectionExtensionContext::ILLEGAL_REQUEST_CODE(-1);

ErrCode SelectionExtensionContext::StartAbility(const AAFwk::Want& want) const
{
    HILOG_DEBUG("%{public}s begin.", __func__);
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, token_, ILLEGAL_REQUEST_CODE);
    HILOG_DEBUG("%{public}s ret=%{public}d", __func__, err);
    if (err != ERR_OK) {
        HILOG_ERROR("SelectionExtensionContext::StartAbility failed: %{public}d", err);
    }
    return err;
}

ErrCode SelectionExtensionContext::StartAbility(const AAFwk::Want& want, const AAFwk::StartOptions& startOptions) const
{
    HILOG_DEBUG("%{public}s start.", __func__);
    ErrCode err =
        AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, startOptions, token_, ILLEGAL_REQUEST_CODE);
    HILOG_DEBUG("%{public}s ret=%{public}d", __func__, err);
    if (err != ERR_OK) {
        HILOG_ERROR("InputMethodExtensionContext::StartAbility failed: %{public}d", err);
    }
    return err;
}

} // namespace OHOS::AbilityRuntime