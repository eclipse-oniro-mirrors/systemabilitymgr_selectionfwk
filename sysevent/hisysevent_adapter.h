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

#ifndef HISYSEVENT_ADAPTER_H
#define HISYSEVENT_ADAPTER_H

#include "hisysevent.h"
#include "refbase.h"
#include "selection_log.h"
#include "timer.h"

namespace OHOS::SelectionFwk {

enum class SelectFailedReason : int32_t {
    INJECT_CTRLC_FAILED = 0,
    TEXT_RECEIVE_FAILED,
    CREATE_PANEL_FAILED,
};

class HisyseventAdapter : public RefBase {
public:
    static OHOS::sptr<HisyseventAdapter>& GetInstance();
    HisyseventAdapter()
    {
        reportEventTimer_ = std::make_unique<Utils::Timer>("DfxReporter", -1);
        reportEventTimer_->Setup();
    }
    ~HisyseventAdapter()
    {
        if (reportEventTimer_ != nullptr) {
            reportEventTimer_->Unregister(timerId_);
            SELECTION_HILOGI("StopDfxTimer timerId : %{public}u!", timerId_);
            reportEventTimer_->Shutdown();
        }
        SELECTION_HILOGI("reportEventTimer Shutdown successfully!");
    }

    void ReportShowPanelFailed(const std::string& bundleName, int32_t errorCode, int32_t failReason);
    void AddFailCount();
    void AddSelectionCount();
    void InitCount();
    void ReportStatisticInfo();
    void StartHisyseventTimer();

private:
    uint32_t failCount_ = 0;
    uint32_t selectionCount_ = 0;
    std::unique_ptr<Utils::Timer> reportEventTimer_ = nullptr;
    uint32_t timerId_ = 0;
};
}

#endif // HISYSEVENT_ADAPTER_H