/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SELECTION_INTERFACE_H
#define SELECTION_INTERFACE_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace OHOS {
namespace SelectionFwk {

typedef enum {
    MOVE_SELECTION = 1,
    DOUBLE_CLICKED_SELECTION = 2,
    TRIPLE_CLICKED_SELECTION = 3,
} SelectionType;

struct SelectionInfo {
    SelectionType selectionType;
    std::string text = "";
    int32_t startDisplayX = 0;
    int32_t startDisplayY = 0;
    int32_t endDisplayX = 0;
    int32_t endDisplayY = 0;
    int32_t startWindowX = 0;
    int32_t startWindowY = 0;
    int32_t endWindowX = 0;
    int32_t endWindowY = 0;
    uint32_t displayId = 0;
    uint32_t windowId = 0;
    std::string bundleName = "";
};

class SelectionInterface {
public:
    virtual ~SelectionInterface() = default;
    virtual int32_t OnSelectionEvent(const SelectionInfo &selectionInfo) = 0;
};
} // namespace SelectionFwk
}  // namespace OHOS
#endif // SELECTION_INTERFACE_H