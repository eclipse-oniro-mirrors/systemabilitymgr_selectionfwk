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

#ifndef SELECTION_APP_VALIDATOR_H
#define SELECTION_APP_VALIDATOR_H

#include <memory>
#include <optional>
#include <string>

namespace OHOS::SelectionFwk {
class SelectionAppValidator {
public:
    static SelectionAppValidator& GetInstance();
    virtual ~SelectionAppValidator() = default;
    bool Validate() const;

private:
    SelectionAppValidator() = default;
    SelectionAppValidator(const SelectionAppValidator&) = delete;
    SelectionAppValidator(SelectionAppValidator&&) = delete;
    SelectionAppValidator& operator= (const SelectionAppValidator&) = delete;
    SelectionAppValidator& operator= (SelectionAppValidator&&) = delete;

    virtual std::optional<std::string> GetCurrentBundleName() const;
    std::optional<std::string> GetBundleNameFromSys() const;
};

} // namespace OHOS::SelectionFwk
#endif // SELECTION_APP_VALIDATOR_H