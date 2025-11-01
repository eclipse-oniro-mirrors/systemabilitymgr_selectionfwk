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
#include <sstream>

namespace OHOS {
namespace SelectionFwk {
bool SelectionConfig::GetEnable() const
{
    return isEnabled_;
}

bool SelectionConfig::GetTriggered() const
{
    return isTriggered_;
}

int SelectionConfig::GetUid() const
{
    return uid_;
}

std::string SelectionConfig::GetApplicationInfo() const
{
    return applicationInfo_;
}

void SelectionConfig::SetEnabled(bool enabled)
{
    isEnabled_ = enabled;
}

void SelectionConfig::SetTriggered(bool isTriggered)
{
    isTriggered_ = isTriggered;
}

void SelectionConfig::SetUid(int uid)
{
    uid_ = uid;
}

void SelectionConfig::SetApplicationInfo(const std::string &applicationInfo)
{
    applicationInfo_ = applicationInfo;
}

std::string SelectionConfig::ToString() const
{
    std::ostringstream oss;
    oss << "enable: " << isEnabled_ << ", trigger: "
        << isTriggered_ << ", applicationInfo: " << applicationInfo_;
    return oss.str();
}

MemSelectionConfig& MemSelectionConfig::GetInstance()
{
    static MemSelectionConfig instance;
    return instance;
}

void MemSelectionConfig::SetSelectionConfig(const SelectionConfig &config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    delegate_ = config;
}

SelectionConfig MemSelectionConfig::GetSelectionConfig()
{
    return delegate_;
}

bool MemSelectionConfig::GetEnable() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return delegate_.GetEnable();
}

bool MemSelectionConfig::GetTriggered() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return delegate_.GetTriggered();
}

std::string MemSelectionConfig::GetApplicationInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return delegate_.GetApplicationInfo();
}

void MemSelectionConfig::SetEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    delegate_.SetEnabled(enabled);
}

void MemSelectionConfig::SetTriggered(bool isTriggered)
{
    std::lock_guard<std::mutex> lock(mutex_);
    delegate_.SetTriggered(isTriggered);
}

void MemSelectionConfig::SetApplicationInfo(const std::string &applicationInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    delegate_.SetApplicationInfo(applicationInfo);
}
} // namespace SelectionFwk
} // namespace OHOS