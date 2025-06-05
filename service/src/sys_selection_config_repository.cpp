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
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
static const char *SELECTION_SWITCH = "sys.selection.switch";
static const char *SELECTION_TRIGGER = "sys.selection.trigger";
static const char *SELECTION_APPLICATION = "sys.selection.app";
static const char *SELECTION_UID = "sys.selection.uid";
static const int BUFFER_LEN = 200;

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
    SELECTION_HILOGI("info.uid = %{public}d", info.GetUid());
    SetEnabled(info.IsEnabled());
    SetTriggered(info.IsTriggered());
    SetBundleName(info.GetBundleName());
    SetUid(info.GetUid());
    return 0;
}

SelectionConfig SysSelectionConfigRepository::GetSysParameters()
{
    SelectionConfig info;
    info.SetEnabled(IsEnabled());
    info.SetTriggered(IsTriggered());
    info.SetBundleName(GetBundleName());
    info.SetUid(GetUid());
    return info;
}

void SysSelectionConfigRepository::DisableSAService()
{
    SetParameter(SELECTION_SWITCH, "off");
}

int SysSelectionConfigRepository::IsEnabled()
{
    char value[BUFFER_LEN];
    GetParameter(SELECTION_SWITCH, "", value, BUFFER_LEN);
    if (strcmp(value, "on") == 0) {
        return 1;
    }
    return 0;
}

int SysSelectionConfigRepository::IsTriggered()
{
    char value[BUFFER_LEN];
    GetParameter(SELECTION_TRIGGER, "", value, BUFFER_LEN);
    if (strcmp(value, "ctrl") == 0) {
        return 1;
    }
    return 0;
}

int SysSelectionConfigRepository::GetUid()
{
    char value[BUFFER_LEN];
    GetParameter(SELECTION_UID, "", value, BUFFER_LEN);
    return std::atoi(value);
}

std::string SysSelectionConfigRepository::GetBundleName()
{
    char value[BUFFER_LEN];
    GetParameter(SELECTION_APPLICATION, "", value, BUFFER_LEN);
    return value;
}

void SysSelectionConfigRepository::SetEnabled(int enabled)
{
    SELECTION_HILOGI("[XYING6]enabled: %{public}d", enabled);
    if (enabled == 1) {
        SetParameter(SELECTION_SWITCH, "on");
    } else if (enabled == 0) {
        SetParameter(SELECTION_SWITCH, "off");
    }
}

void SysSelectionConfigRepository::SetTriggered(int isTriggered)
{
    if (isTriggered == 1) {
        SetParameter(SELECTION_TRIGGER, "ctrl");
    } else if (isTriggered == 0) {
        SetParameter(SELECTION_TRIGGER, "");
    }
}

void SysSelectionConfigRepository::SetUid(int uid)
{
    SELECTION_HILOGI("uid = %{public}d", uid);
    SetParameter(SELECTION_UID, std::to_string(uid).c_str());
}

void SysSelectionConfigRepository::SetBundleName(const std::string &bundleName)
{
    SetParameter(SELECTION_APPLICATION, bundleName.c_str());
}
} // namespace SelectionFwk
} // namespace OHOS