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
#include "parcel.h"

namespace OHOS {
namespace SelectionFwk {

struct SelectionData {
    std::string text { "" };
    int32_t cursorStartPos = 0;
    int32_t cursorEndPos = 0;
    uint32_t windowId = 0;
    uint32_t bundleID = 0;
};

typedef enum {
    MOVE_SELECTION = 0,
    DOUBLE_CLICKED_SELECTION = 1,
    TRIPLE_CLICKED_SELECTION = 2,
} SelectionType;

struct SelectionDataInner : public Parcelable {
    SelectionType selectionType;
    std::string text { "" };
    int32_t startPosX = 0;
    int32_t startPosY = 0;
    int32_t endPosX = 0;
    int32_t endPosY = 0;
    uint32_t windowId = 0;
    uint32_t bundleID = 0;

    bool ReadFromParcel(Parcel &in)
    {
        selectionType = static_cast<SelectionType>(in.ReadInt8());
        text = in.ReadString();
        startPosX = in.ReadInt32();
        startPosY = in.ReadInt32();
        endPosX = in.ReadInt32();
        endPosY = in.ReadInt32();
        windowId = in.ReadUint32();
        bundleID = in.ReadUint32();
        return true;
    }

    bool Marshalling(Parcel &out) const
    {
        if (!out.WriteInt8(static_cast<int8_t>(selectionType))) {
            return false;
        }
        if (!out.WriteString(text)) {
            return false;
        }
        if (!out.WriteInt32(startPosX)) {
            return false;
        }
        if (!out.WriteInt32(startPosY)) {
            return false;
        }
        if (!out.WriteInt32(endPosX)) {
            return false;
        }
        if (!out.WriteInt32(endPosY)) {
            return false;
        }
        if (!out.WriteUint32(windowId)) {
            return false;
        }
        if (!out.WriteUint32(bundleID)) {
            return false;
        }
        return true;
    }

    static SelectionDataInner *Unmarshalling(Parcel &in)
    {
        SelectionDataInner *data = new (std::nothrow) SelectionDataInner();
        if (data && !data->ReadFromParcel(in)) {
            delete data;
            data = nullptr;
        }
        return data;
    }
};

class SelectionInterface {
public:
    virtual ~SelectionInterface() = default;
    virtual int32_t OnSelectionEvent(const SelectionData &selectionData) = 0;
};
} // namespace SelectionFwk
}  // namespace OHOS
#endif // SELECTION_INTERFACE_H