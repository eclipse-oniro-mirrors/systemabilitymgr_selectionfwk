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

#ifndef SELECTION_CONFIG_H
#define SELECTION_CONFIG_H

#include <string>
#include <mutex>

namespace OHOS {
namespace SelectionFwk {
class SelectionConfig {
public:
    bool GetEnable() const;
    bool GetTriggered() const;
    int GetUid() const;
    std::string GetApplicationInfo() const;
    void SetEnabled(bool enabled);
    void SetTriggered(bool isTriggered);
    void SetApplicationInfo(const std::string &applicationInfo);
    void SetUid(int uid);

    std::string ToString() const;

private:
    bool isEnabled_ = true;
    bool isTriggered_ = false;
    int uid_ = -1;
    std::string applicationInfo_ = "com.huawei.hmos.vassistant/MiniMenuServiceExtAbility";
};


class MemSelectionConfig {
public:
    static MemSelectionConfig &GetInstance();
    SelectionConfig &GetSelectionConfig();
    void SetSelectionConfig(const SelectionConfig &config);
    bool GetEnable() const;
    bool GetTriggered() const;
    std::string GetApplicationInfo() const;
    void SetEnabled(bool enabled);
    void SetTriggered(bool isTriggered);
    void SetApplicationInfo(const std::string &applicationInfo);

private:
    MemSelectionConfig() = default;
    ~MemSelectionConfig() = default;
    SelectionConfig delegate_;
    mutable std::mutex mutex_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTION_CONFIG_H