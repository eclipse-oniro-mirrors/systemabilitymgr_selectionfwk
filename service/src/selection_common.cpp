/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string>
#include "selection_common.h"
#include "selection_log.h"
namespace OHOS {
namespace SelectionFwk {

bool AbilityRuntimeInfo::operator==(const AbilityRuntimeInfo& other) const
{
    if (this == &other) {
        return true;
    }
    return (userId == other.userId && bundleName == other.bundleName && abilityName == other.abilityName);
}

bool IsNumber(const std::string& str)
{
    if (str.empty()) {
        return false;
    }

    size_t i = 0;
    if (str[i] == '+' || str[i] == '-') {
        i++;
    }

    if (i != 0 && str.length() == 1) {
        return false;
    }

    if (std::all_of(str.begin() + i, str.end(), [](unsigned char c) { return std::isdigit(c); })) {
        return true;
    }

    return false;
}

std::optional<std::tuple<std::string, std::string>> ParseAppInfo(const std::string& appInfo)
{
    auto pos = appInfo.find('/');
    if (pos == std::string::npos) {
        SELECTION_HILOGE("app info: %{public}s is invalid!", appInfo.c_str());
        return std::nullopt;
    }
    const std::string bundleName = appInfo.substr(0, pos);
    const std::string extName = appInfo.substr(pos + 1);
    if (bundleName.empty() || extName.empty()) {
        SELECTION_HILOGE("bundleName or extName is empty");
        return std::nullopt;
    }
    return std::make_tuple(bundleName, extName);
}
} // namespace SelectionFwk
} // namespace OHOS
