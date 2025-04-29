/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef BASE_WORD_SELECTION_SERVICE_H
#define BASE_WORD_SELECTION_SERVICE_H

#include <string>

#include "word_selection_stub.h"
#include "refbase.h"
#include "system_ability.h"
#include <i_input_event_consumer.h>

namespace OHOS {
using namespace MMI;
class WordSelectionService : public SystemAbility, public WordSelectionStub {
    DECLARE_SYSTEM_ABILITY(WordSelectionService);

public:
    WordSelectionService(int32_t saId, bool runOnCreate);
    ~WordSelectionService();

    int AddVolume(int volume) override;
protected:
    void OnStart() override;
    void OnStop() override;
    void HandleKeyEvent(int32_t keyCode);
    void HandlePointEvent(int32_t type);
private:
    int32_t inputMonitorId_ {-1};
    void InputMonitorInit();
    void InputMonitorCancel();
};

class WordSelectionInputMonitor : public IInputEventConsumer {
public:
    virtual void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const;
    virtual void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const;
    virtual void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const;
};

}

#endif // BASE_WORD_SELECTION_SERVICE_H