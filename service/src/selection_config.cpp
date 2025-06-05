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

#include "selection_config.h"

#include <algorithm>

namespace OHOS {
namespace SelectionFwk {
int SelectionConfig::IsEnabled() const
{
    return isEnabled_;
}

int SelectionConfig::IsTriggered() const
{
    return isTriggered_;
}

int SelectionConfig::GetUid() const
{
    return uid_;
}

std::string SelectionConfig::GetBundleName() const
{
    return bundleName_;
}

void SelectionConfig::SetEnabled(int enabled)
{
    isEnabled_ = enabled;
}

void SelectionConfig::SetTriggered(int isTriggered)
{
    isTriggered_ = isTriggered;
}

void SelectionConfig::SetUid(int uid)
{
    uid_ = uid;
}

void SelectionConfig::SetBundleName(const std::string &bundleName)
{
    bundleName_ = bundleName;
}

std::string SelectionConfig::ToString() const {
    std::string str = "uid: " + std::to_string(uid_) + ", enable: " + std::to_string(isEnabled_) + ", trigger: " +
    std::to_string(isTriggered_) + ", bundleName: " + bundleName_;
    return str;
}

MemSelectionConfig& MemSelectionConfig::GetInstance()
{
    static MemSelectionConfig instance;
    return instance;
}

void MemSelectionConfig::SetSelectionConfig(const SelectionConfig &config)
{
    delegate_ = config;
}

SelectionConfig& MemSelectionConfig::GetSelectionConfig()
{
    return delegate_;
}

int MemSelectionConfig::IsEnabled() const
{
    return delegate_.IsEnabled();
}

int MemSelectionConfig::IsTriggered() const
{
    return delegate_.IsTriggered();
}

std::string MemSelectionConfig::GetBundleName() const
{
    return delegate_.GetBundleName();
}

void MemSelectionConfig::SetEnabled(int enabled)
{
    delegate_.SetEnabled(enabled);
}

void MemSelectionConfig::SetTriggered(int isTriggered)
{
    delegate_.SetTriggered(isTriggered);
}

void MemSelectionConfig::SetBundleName(const std::string &bundleName)
{
    delegate_.SetBundleName(bundleName);
}
} // namespace SelectionFwk
} // namespace OHOS