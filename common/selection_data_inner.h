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
#ifndef SELECTION_DATA_INNER
#define SELECTION_DATA_INNER

#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include "parcel.h"
#include "selection_interface.h"

namespace OHOS {
namespace SelectionFwk {

struct SelectionInfoData : public Parcelable {
    SelectionInfo data;

    bool ReadFromParcel(Parcel &in) {
        data.selectionType = static_cast<SelectionType>(in.ReadInt8());
        data.text = in.ReadString();
        data.startDisplayX = in.ReadInt32();
        data.startDisplayY = in.ReadInt32();
        data.endDisplayX = in.ReadInt32();
        data.endDisplayY = in.ReadInt32();
        data.startWindowX = in.ReadInt32();
        data.startWindowY = in.ReadInt32();
        data.endWindowX = in.ReadInt32();
        data.endWindowY = in.ReadInt32();
        data.displayId = in.ReadUint32();
        data.windowId = in.ReadUint32();
        data.bundleName = in.ReadString();
        return true;
    }

    bool Marshalling(Parcel &out) const {
        if (!out.WriteInt8(static_cast<int8_t>(data.selectionType))) {
            return false;
        }
        if (!out.WriteString(data.text)) {
            return false;
        }
        if (!out.WriteInt32(data.startDisplayX)) {
            return false;
        }
        if (!out.WriteInt32(data.startDisplayY)) {
            return false;
        }
        if (!out.WriteInt32(data.endDisplayX)) {
            return false;
        }
        if (!out.WriteInt32(data.endDisplayY)) {
            return false;
        }
        if (!out.WriteInt32(data.startWindowX)) {
            return false;
        }
        if (!out.WriteInt32(data.startWindowY)) {
            return false;
        }
        if (!out.WriteInt32(data.endWindowX)) {
            return false;
        }
        if (!out.WriteInt32(data.endWindowY)) {
            return false;
        }
        if (!out.WriteUint32(data.displayId)) {
            return false;
        }
        if (!out.WriteUint32(data.windowId)) {
            return false;
        }
        if (!out.WriteString(data.bundleName)) {
            return false;
        }
        return true;
    }

    static SelectionInfoData *Unmarshalling(Parcel &in) {
        SelectionInfoData *data = new (std::nothrow) SelectionInfoData();
        if (data && !data->ReadFromParcel(in)) {
            delete data;
            data = nullptr;
        }
        return data;
    }

    std::string ToString() const {
        std::ostringstream oss;
        oss << "SelectionInfo { selectionType: " << data.selectionType <<
            ", text.length: " << data.text.length() <<
            ", startDisplayX: " << data.startDisplayX <<
            ", startDisplayY: " << data.startDisplayY <<
            ", endDisplayX: " << data.endDisplayX <<
            ", endDisplayY: " << data.endDisplayY <<
            ", startWindowX: " << data.startWindowX <<
            ", startWindowY: " << data.startWindowY <<
            ", endWindowX: " << data.endWindowX <<
            ", endWindowY: " << data.endWindowY <<
            ", displayId: " << data.displayId <<
            ", windowId: " << data.windowId <<
            ", bundleName: " << data.bundleName << "}";
        return oss.str();
    }
};

enum class FocusChangeSource : uint32_t {
    WindowManager,
    InputManager,
};

class SelectionFocusChangeInfo : public Parcelable {
public:
    SelectionFocusChangeInfo() = default;
    SelectionFocusChangeInfo(uint32_t winId, uint64_t displayId, int32_t pid, int32_t uid, uint32_t type,
        bool isFocused, FocusChangeSource source): windowId_(winId), displayId_(displayId), pid_(pid), uid_(uid),
        windowType_(type), isFocused_(isFocused), source_(source) {};

    ~SelectionFocusChangeInfo() = default;

    virtual bool Marshalling(Parcel& parcel) const
    {
        bool ret = parcel.WriteInt32(windowId_) && parcel.WriteUint64(displayId_) &&
            parcel.WriteInt32(pid_) && parcel.WriteInt32(uid_) &&
            parcel.WriteUint32(static_cast<uint32_t>(windowType_)) &&
            parcel.WriteBool(isFocused_) &&
            parcel.WriteUint32(static_cast<uint32_t>(source_));
        return ret;
    }

    static SelectionFocusChangeInfo* Unmarshalling(Parcel& parcel)
    {
        auto focusChangeInfo = new SelectionFocusChangeInfo();
        bool res = parcel.ReadInt32(focusChangeInfo->windowId_) && parcel.ReadUint64(focusChangeInfo->displayId_) &&
            parcel.ReadInt32(focusChangeInfo->pid_) && parcel.ReadInt32(focusChangeInfo->uid_) &&
            parcel.ReadUint32(focusChangeInfo->windowType_) &&
            parcel.ReadBool(focusChangeInfo->isFocused_);
        if (!res) {
            delete focusChangeInfo;
            return nullptr;
        }
        focusChangeInfo->source_ = static_cast<FocusChangeSource>(parcel.ReadUint32());
        return focusChangeInfo;
    }

    std::string ToString() const
    {
        std::ostringstream oss;
        oss << "SelectionFocusChangeInfo { windowId: " << windowId_ << ", displayId: " << displayId_ <<
            ", windowType: " << windowType_ << ", isFocused: " << isFocused_ <<
            ", source_: " << static_cast<uint32_t>(source_) << "}";
        return oss.str();
    }

    int32_t windowId_ = -1;
    uint64_t displayId_ = 0;
    int32_t pid_ = -1;
    int32_t uid_ = -1;
    uint32_t windowType_ = 1;
    bool isFocused_ = false;
    FocusChangeSource source_ = FocusChangeSource::WindowManager;
};
}
}

#endif