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
#include "selection_config.h"

namespace OHOS {
namespace SelectionFwk {
class SelectionConfig {
public:
    int IsEnabled() const;
    int IsTriggered() const;
    int GetUid() const;
    std::string GetBundleName() const;
    void SetEnabled(int enabled);
    void SetTriggered(int isTriggered);
    void SetBundleName(const std::string &bundleName);
    void SetUid(int uid);

    std::string ToString() const;

private:
    int isEnabled_ = 1;
    int isTriggered_ = 1;
    int uid_ = 0;
    std::string bundleName_ = "com.hm.youdao/ExtensionAbility";
};


class MemSelectionConfig {
public:
    static MemSelectionConfig &GetInstance();
    SelectionConfig &GetSelectionConfig();
    void SetSelectionConfig(const SelectionConfig &config);
    int IsEnabled() const;
    int IsTriggered() const;
    std::string GetBundleName() const;
    void SetEnabled(int enabled);
    void SetTriggered(int isTriggered);
    void SetBundleName(const std::string &bundleName);

private:
    MemSelectionConfig() = default;
    ~MemSelectionConfig() = default;
    SelectionConfig delegate_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTION_CONFIG_H