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

#define SELECTION_HISYSEVENT_REPORT_TIME (24 * 60 * 60 * 1000)   // 24h

#include "hisysevent_adapter.h"

using namespace OHOS::HiviewDFX;
using namespace OHOS::SelectionFwk;
namespace {
// fail event
constexpr const char* SPAWN_CHILD_PROCESS_FAIL = "SPAWN_CHILD_PROCESS_FAIL";
// statistic event
constexpr const char* SPAWN_PROCESS_DURATION = "SPAWN_PROCESS_DURATION";

// param
constexpr const char* PROCESS_NAME = "PROCESS_NAME";
constexpr const char* ERROR_CODE = "ERROR_CODE";
constexpr const char* SPAWN_RESULT = "SPAWN_RESULT";
constexpr const char* MAXDURATION = "MAXDURATION";
constexpr const char* MINDURATION = "MINDURATION";
constexpr const char* TOTALDURATION = "TOTALDURATION";
constexpr const char* EVENTCOUNT = "EVENTCOUNT";
constexpr const char* STAGE = "STAGE";
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
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_PROCESS_DURATION,
        HiSysEvent::EventType::STATISTIC,
        MAXDURATION, failCount_,
        MINDURATION, failCount_,
        TOTALDURATION, selectionCount_,
        EVENTCOUNT, selectionCount_,
        STAGE, "selection_statistic");
    if (ret != 0) {
        SELECTION_HILOGE("HiSysEventWrite error, ret: %{public}d", ret);
    }
    InitCount();
}

void HisyseventAdapter::StartHisyseventTimer()
{
    timerId_ = reportEventTimer_->Register([this] {this->ReportStatisticInfo();},
        SELECTION_HISYSEVENT_REPORT_TIME);
    SELECTION_HILOGI("StartDfxTimer timerId : %{public}u!", timerId_);
}

void HisyseventAdapter::ReportShowPanelFailed(const std::string& bundleName, int32_t errorCode, int32_t failReason)
{
    AddFailCount();
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_CHILD_PROCESS_FAIL,
        HiSysEvent::EventType::FAULT,
        PROCESS_NAME, bundleName,
        ERROR_CODE, errorCode,
        SPAWN_RESULT, failReason);
    if (ret != 0) {
        SELECTION_HILOGE("HiSysEventWrite error, ret: %{public}d", ret);
    }
}
