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

#ifndef SYS_SELECTION_CONFIG_REPOSITORY_H
#define SYS_SELECTION_CONFIG_REPOSITORY_H
#include <memory>
#include <mutex>
#include "selection_config.h"

namespace OHOS {
namespace SelectionFwk {
class SysSelectionConfigRepository {
public:
    static std::shared_ptr<SysSelectionConfigRepository> GetInstance();
    int SetSysParameters(const SelectionConfig &config);
    SelectionConfig GetSysParameters();
    void DisableSAService();

private:
    void SetEnabled(bool enabled);
    void SetTriggered(bool isTriggered);
    void SetUid(int uid);
    void SetApplicationInfo(const std::string &applicationInfo);
    int GetEnable();
    int GetTriggered();
    int GetUid();
    std::string GetApplicationInfo();
    static std::shared_ptr<SysSelectionConfigRepository> instance_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SYS_SELECTION_CONFIG_REPOSITORY_H