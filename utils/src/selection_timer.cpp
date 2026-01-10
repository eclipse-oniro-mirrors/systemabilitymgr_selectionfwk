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

#include "selection_timer.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

std::shared_ptr<SelectionFwkTimer> SelectionFwkTimer::GetInstance()
{
    static std::shared_ptr<SelectionFwkTimer> instance = [] {
        return std::make_shared<SelectionFwkTimer>();
    }();

    return instance;
}

SelectionFwkTimer::SelectionFwkTimer()
{
    selectionFwkTimer_ = std::make_unique<Utils::Timer>("selectionFwkTimer", -1);
    selectionFwkTimer_->Setup();
    SELECTION_HILOGI("timer setup");
}

SelectionFwkTimer::~SelectionFwkTimer()
{
    // unregister all timerId before shutdown the timer
    for (auto timerId : timerRegSet_) {
        selectionFwkTimer_->Unregister(timerId);
    }
    selectionFwkTimer_->Shutdown();
    SELECTION_HILOGI("timer shutdown");
}

uint32_t SelectionFwkTimer::Register(const TimerCallback& callback, uint32_t interval, bool once)
{
    uint32_t timerId = selectionFwkTimer_->Register(callback, interval, once);
    {
        std::lock_guard<std::mutex> lock(timerSetMtx);
        timerRegSet_.insert(timerId);
    }
    return timerId;
}

void SelectionFwkTimer::UnRegister(uint32_t timerId)
{
    selectionFwkTimer_->Unregister(timerId);
    {
        std::lock_guard<std::mutex> lock(timerSetMtx);
        if (timerRegSet_.find(timerId) != timerRegSet_.end()) {
            timerRegSet_.erase(timerId);
        }
    }
}

} // namespace SelectionFwk
} // namespace OHOS