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

#include "sys_selection_config_repository.h"
#include <cstring>
#include "parameter.h"
#include "param_wrapper.h"
#include "selection_log.h"
#include "selection_common.h"

namespace OHOS {
namespace SelectionFwk {
static const char *SELECTION_SWITCH = "sys.selection.switch";
static const char *SELECTION_TRIGGER = "sys.selection.trigger";
static const char *SELECTION_APPLICATION = "sys.selection.app";
static const char *SELECTION_UID = "sys.selection.uid";
static const int BUFFER_LEN = 200;

#define SELECTION_MAX_UID_LENGTH 11

std::shared_ptr<SysSelectionConfigRepository> SysSelectionConfigRepository::instance_ = nullptr;

std::shared_ptr<SysSelectionConfigRepository> SysSelectionConfigRepository::GetInstance()
{
    static std::mutex instanceMutex;
    std::lock_guard<std::mutex> guard(instanceMutex);
    if (instance_ == nullptr) {
        SELECTION_HILOGI("reset to new SysSelectionConfigRepository instance");
        instance_ = std::make_shared<SysSelectionConfigRepository>();
    }
    return instance_;
}

int SysSelectionConfigRepository::SetSysParameters(const SelectionConfig &info)
{
    SetEnabled(info.GetEnable());
    SetTriggered(info.GetTriggered());
    SetApplicationInfo(info.GetApplicationInfo());
    SetUid(info.GetUid());
    return 0;
}

SelectionConfig SysSelectionConfigRepository::GetSysParameters()
{
    SelectionConfig info;
    info.SetEnabled(GetEnable());
    info.SetTriggered(GetTriggered());
    info.SetApplicationInfo(GetApplicationInfo());
    info.SetUid(GetUid());
    return info;
}

void SysSelectionConfigRepository::DisableSAService()
{
    auto ret = SetParameter(SELECTION_SWITCH, "off");
    if (ret < 0) {
        SELECTION_HILOGE("Failed to SetParameter(%{public}s, off), ret: %{public}d", SELECTION_SWITCH, ret);
    }
}

int SysSelectionConfigRepository::GetEnable()
{
    char value[BUFFER_LEN];
    auto ret = GetParameter(SELECTION_SWITCH, "", value, BUFFER_LEN);
    if (ret < 0) {
        SELECTION_HILOGE("Failed to GetParameter(%{public}s), ret: %{public}d", SELECTION_SWITCH, ret);
        return 0;
    }

    SELECTION_HILOGI("GetParameter(%{public}s) returns [%{public}s]", SELECTION_SWITCH, value);
    if (strcmp(value, "on") == 0) {
        return 1;
    }
    return 0;
}

int SysSelectionConfigRepository::GetTriggered()
{
    char value[BUFFER_LEN];
    auto ret = GetParameter(SELECTION_TRIGGER, "", value, BUFFER_LEN);
    if (ret < 0) {
        SELECTION_HILOGE("Failed to GetParameter(%{public}s), ret: %{public}d", SELECTION_TRIGGER, ret);
        return 0;
    }

    SELECTION_HILOGI("GetParameter(%{public}s) returns [%{public}s]", SELECTION_TRIGGER, value);
    if (strcmp(value, "ctrl") == 0) {
        return 1;
    }
    return 0;
}

int SysSelectionConfigRepository::GetUid()
{
    std::string uidStr;
    auto ret = OHOS::system::GetStringParameter(SELECTION_UID, uidStr);
    if (ret != 0) {
        SELECTION_HILOGE("Failed to GetStringParameter(%{public}s), ret: %{public}d", SELECTION_UID, ret);
        return -1;
    }

    if (!IsNumber(uidStr)) {
        SELECTION_HILOGE("uidStr maybe not all of digit!");
        return -1;
    }

    size_t maxLen = (uidStr[0] == '-') ? SELECTION_MAX_UID_LENGTH : SELECTION_MAX_UID_LENGTH - 1;

    if (uidStr.length() > maxLen) {
        SELECTION_HILOGE("uidStr exceeds the range of int!");
        return -1;
    }

    if ((uidStr.length() == maxLen) &&
        ((uidStr[0] != '-' && uidStr > "2147483647") ||
         (uidStr[0] == '-' && uidStr > "-2147483648"))) {
        SELECTION_HILOGE("uidStr exceeds the range of int!");
        return -1;
    }

    int uid = std::stoi(uidStr);
    return uid;
}

std::string SysSelectionConfigRepository::GetApplicationInfo()
{
    std::string appinfo;
    if (OHOS::system::GetStringParameter(SELECTION_APPLICATION, appinfo) != 0) {
        SELECTION_HILOGE("GetStringParameter failed for SELECTION_APPLICATION");
        return "";
    }

    return appinfo;
}

void SysSelectionConfigRepository::SetEnabled(bool enabled)
{
    SELECTION_HILOGI("enabled: %{public}d", enabled);
    auto value = enabled ? "on" : "off";
    auto ret = SetParameter(SELECTION_SWITCH, value);
    if (ret < 0) {
        SELECTION_HILOGE("Failed to SetParameter(%{public}s, %{public}s), ret: %{public}d",
            SELECTION_SWITCH, value, ret);
    }
}

void SysSelectionConfigRepository::SetTriggered(bool isTriggered)
{
    auto value = isTriggered ? "ctrl" : "immediate";
    auto ret = SetParameter(SELECTION_TRIGGER, value);
    if (ret < 0) {
        SELECTION_HILOGE("Failed to SetParameter(%{public}s, %{public}s), ret: %{public}d",
            SELECTION_TRIGGER, value, ret);
    }
}

void SysSelectionConfigRepository::SetUid(int uid)
{
    std::string uidStr = std::to_string(uid);
    auto ret = SetParameter(SELECTION_UID, uidStr.c_str());
    if (ret < 0) {
        SELECTION_HILOGE("Failed to SetParameter(%{public}s, %{public}s), ret: %{public}d",
            SELECTION_UID, uidStr.c_str(), ret);
    }
}

void SysSelectionConfigRepository::SetApplicationInfo(const std::string &applicationInfo)
{
    auto ret = SetParameter(SELECTION_APPLICATION, applicationInfo.c_str());
    if (ret < 0) {
        SELECTION_HILOGE("Failed to SetParameter(%{public}s, %{public}s), ret: %{public}d",
            SELECTION_APPLICATION, applicationInfo.c_str(), ret);
    }
}
} // namespace SelectionFwk
} // namespace OHOS