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

#include "selection_app_validator.h"

#include <ipc_skeleton.h>
#include "accesstoken_kit.h"
#include "parameter.h"
#include "param_wrapper.h"
#include "selection_log.h"

namespace OHOS::SelectionFwk {

static const char* SELECTION_APPLICATION = "sys.selection.app";

SelectionAppValidator& SelectionAppValidator::GetInstance()
{
    static SelectionAppValidator instance;
    return instance;
}

bool SelectionAppValidator::Validate() const
{
    auto currentBundleName = GetCurrentBundleName();
    if (!currentBundleName.has_value()) {
        SELECTION_HILOGE("Failed to GetCurrentBundleName");
        return false;
    }
    auto bundleNameFromSys = GetBundleNameFromSys();
    if (!bundleNameFromSys.has_value()) {
        SELECTION_HILOGE("Failed to GetBundleNameFromSys");
        return false;
    }
    SELECTION_HILOGI("Validate bundleName: [%{public}s] with bundleName: [%{public}s] from sys",
        currentBundleName.value().c_str(), bundleNameFromSys.value().c_str());
    return currentBundleName.value() == bundleNameFromSys.value();
}

std::optional<std::string> SelectionAppValidator::GetCurrentBundleName() const
{
    auto callingTokenId = IPCSkeleton::GetCallingTokenID();
    OHOS::Security::AccessToken::HapTokenInfo hapTokenInfoRes;
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenInfo(callingTokenId, hapTokenInfoRes);
    if (ret != 0) {
        SELECTION_HILOGE("Failed to GetHapTokenInfo, ret = %{public}d.", ret);
        return std::nullopt;
    }
    return hapTokenInfoRes.bundleName;
}

std::optional<std::string> SelectionAppValidator::GetBundleNameFromSys() const
{
    std::string selectionApp;
    if (OHOS::system::GetStringParameter(SELECTION_APPLICATION, selectionApp) != 0) {
        SELECTION_HILOGE("GetStringParameter failed for SELECTION_APPLICATION");
        return std::nullopt;
    }

    const auto pos = selectionApp.find('/');
    if (pos == std::string::npos) {
        SELECTION_HILOGE("%{public}s: [%{public}s] is invalid", SELECTION_APPLICATION, selectionApp.c_str());
        return std::nullopt;
    }
    return selectionApp.substr(0, pos);
}

} // namespace OHOS::SelectionFwk