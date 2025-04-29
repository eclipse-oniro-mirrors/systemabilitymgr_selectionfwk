/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef BASE_WORD_SELECTION_INTERFACE_H
#define BASE_WORD_SELECTION_INTERFACE_H

#include <string>

#include "iremote_broker.h"
#include "iremote_object.h"
#include "iremote_stub.h"
#include "iremote_proxy.h"

namespace OHOS {
class WordSelectionInterface : public IRemoteBroker {
public:
    virtual int32_t AddVolume(int volume) = 0;
    enum {
        ADD_VOLUME = 1,
    };
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.test.WordSelectionSAInterface");
};
}
#endif // BASE_WORD_SELECTION_INTERFACE_H