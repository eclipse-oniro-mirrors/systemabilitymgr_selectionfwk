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
#include "selection_api_event_reporter.h"
#include <thread>
#include "app_event.h"
#include "app_event_processor_mgr.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
static int64_t g_processId = -1;
static std::mutex g_mutex;
static const std::string SDK_NAME("BasicServicesKit");

RandomGenerator SelectionApiEventReporter::randomGenerator_;

SelectionApiEventReporter::SelectionApiEventReporter(const std::string& apiName) : apiName_(apiName)
{
    SELECTION_HILOGI("Create SelectionApiEventReporter object with apiName: %{public}s", apiName.c_str());
    transId_ = GenerateTransId();
    beginTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::unique_lock<std::mutex> lock(g_mutex);
    if (g_processId == -1) {
        HiviewDFX::HiAppEvent::ReportConfig config;
        config.name = "ha_app_event";
        config.configName = "SDK_OCG";
        g_processId = HiviewDFX::HiAppEvent::AppEventProcessorMgr::AddProcessor(config);
    }
}

void SelectionApiEventReporter::WriteEndEvent(const int result, const int32_t errCode)
{
    SELECTION_HILOGI("Begin to WriteEndEvent with result: %{public}d, errCode: %{public}d", result, errCode);
    int64_t endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    HiviewDFX::HiAppEvent::Event event("api_diagnostic", "api_exec_end", HiviewDFX::HiAppEvent::BEHAVIOR);
    event.AddParam("trans_id", transId_);
    event.AddParam("api_name", apiName_);
    event.AddParam("sdk_name", SDK_NAME);
    event.AddParam("begin_time", beginTime_);
    event.AddParam("end_time", endTime);
    event.AddParam("result", result);
    event.AddParam("error_code", errCode);
    int32_t ret = Write(event);
    SELECTION_HILOGI("WriteEndEvent transId:%{public}s, apiName:%{public}s, sdkName:%{public}s, result:%{public}d, "
        "errCode:%{public}d, ret:%{public}d",
        transId_.c_str(), apiName_.c_str(), SDK_NAME.c_str(), result, errCode, ret);
    if (ret != 0) {
        SELECTION_HILOGE("WriteEndEvent HiAppEvent failed, ret: %{public}d", ret);
    }
}

std::string SelectionApiEventReporter::GenerateTransId() const
{
    return std::string("transId_") + std::to_string(randomGenerator_.Generate());
}
} // SelectionFwk
} // OHOS