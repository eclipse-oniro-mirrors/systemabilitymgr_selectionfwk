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

#define SELECTION_HISYSEVENT_REPORT_TIME (12 * 60 * 60 * 1000)   // 12h

#include "hisysevent_adapter.h"
#include "selection_timer.h"

using namespace OHOS::HiviewDFX;
using namespace OHOS::SelectionFwk;
namespace {
// fault event
constexpr const char* SELECTION_PROCESS_ABNORMAL = "SELECTION_PROCESS_ABNORMAL";
// param
constexpr const char* FAIL_REASON = "FAIL_REASON";
constexpr const char* BUNDLE_NAME = "BUNDLE_NAME";
constexpr const char* ERROR_CODE = "ERROR_CODE";

// statistic event
constexpr const char* SELECTION_STATISTIC = "SELECTION_STATISTIC";
// param
constexpr const char* SELECTION_TRIGGER_COUNT = "SELECTION_TRIGGER_COUNT";
constexpr const char* FAILED_COUNT = "FAILED_COUNT";
}

OHOS::sptr<HisyseventAdapter>& HisyseventAdapter::GetInstance()
{
    static OHOS::sptr<HisyseventAdapter> instance;

    if (!instance) {
        instance = new (std::nothrow) HisyseventAdapter();
    }

    return instance;
}

void HisyseventAdapter::AddFailCount()
{
    ++failCount_;
}

void HisyseventAdapter::AddSelectionCount()
{
    ++selectionCount_;
}

void HisyseventAdapter::InitCount()
{
    failCount_ = 0;
    selectionCount_ = 0;
}

void HisyseventAdapter::ReportStatisticInfo()
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::SELECTIONFWK, SELECTION_STATISTIC,
        HiSysEvent::EventType::STATISTIC,
        SELECTION_TRIGGER_COUNT, selectionCount_,
        FAILED_COUNT, failCount_);
    if (ret != 0) {
        SELECTION_HILOGE("HiSysEventWrite error, ret: %{public}d", ret);
    }
    InitCount();
}

void HisyseventAdapter::StartHisyseventTimer()
{
    SelectionFwkTimer::GetInstance()->Register([this] {this->ReportStatisticInfo();},
        SELECTION_HISYSEVENT_REPORT_TIME);
    SELECTION_HILOGI("StartDfxTimer");
}

void HisyseventAdapter::ReportShowPanelFailed(const std::string& bundleName, int32_t errorCode, int32_t failReason)
{
    AddFailCount();
    int ret = HiSysEventWrite(HiSysEvent::Domain::SELECTIONFWK, SELECTION_PROCESS_ABNORMAL,
        HiSysEvent::EventType::FAULT,
        BUNDLE_NAME, bundleName,
        ERROR_CODE, errorCode,
        FAIL_REASON, failReason);
    if (ret != 0) {
        SELECTION_HILOGE("HiSysEventWrite error, ret: %{public}d", ret);
    }
}
