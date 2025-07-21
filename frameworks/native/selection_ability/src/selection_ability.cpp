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

#include "selection_ability.h"

#include <unistd.h>
#include <utility>
#include "selection_panel.h"
#include "selection_log.h"
#include "selection_panel_manager.h"
#include "selection_app_validator.h"
#include "selection_system_ability_utils.h"

namespace OHOS {
namespace SelectionFwk {
sptr<SelectionAbility> SelectionAbility::instance_;
std::mutex SelectionAbility::instanceLock_;

SelectionAbility::SelectionAbility() { }

SelectionAbility::~SelectionAbility()
{
    SELECTION_HILOGI("SelectionAbility::~SelectionAbility.");
}

sptr<SelectionAbility> SelectionAbility::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(instanceLock_);
        if (instance_ == nullptr) {
            SELECTION_HILOGI("SelectionAbility need new SA.");
            instance_ = new (std::nothrow) SelectionAbility();
            if (instance_ == nullptr) {
                SELECTION_HILOGE("instance is nullptr!");
                return instance_;
            }
        }
    }
    return instance_;
}

int32_t SelectionAbility::CreatePanel(const std::shared_ptr<AbilityRuntime::Context> &context,
    const PanelInfo &panelInfo, std::shared_ptr<SelectionPanel> &selectionPanel)
{
    SELECTION_HILOGI("enter CreatePanel, panelInfo: {panelType: %{public}d}", panelInfo.panelType);
    std::ostringstream buffer;
    panels_.ForEach([&](const PanelType &panelType, auto &) {
        buffer << static_cast<uint32_t>(panelType) << ",";
        return false;
    });
    auto panelTypes = buffer.str();
    SELECTION_HILOGI("panels_: %{public}s", panelTypes.c_str());

    if (!SelectionSystemAbilityUtils::IsSelectionSystemAbilityExistent()) {
        SELECTION_HILOGE("selection system ability is not existent!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if (!SelectionAppValidator::GetInstance().Validate()) {
        SELECTION_HILOGE("bundleName is not valid");
        return ErrorCode::ERROR_INVALID_OPERATION;
    }

    int32_t result = ErrorCode::NO_ERROR;
    auto flag = panels_.ComputeIfAbsent(panelInfo.panelType,
        [&result, &panelInfo, &context, &selectionPanel] (
            const PanelType &panelType, std::shared_ptr<SelectionPanel> &panel) {
            selectionPanel = std::make_shared<SelectionPanel>();
            result = selectionPanel->CreatePanel(context, panelInfo);
            if (result == ErrorCode::NO_ERROR) {
                panel = selectionPanel;
                SelectionPanelManager::GetInstance().AddSelectionPanel(selectionPanel->GetWindowId(), selectionPanel);
                return true;
            }
            selectionPanel = nullptr;
            return false;
        });

    if (!flag && result == ErrorCode::NO_ERROR) {
        return ErrorCode::ERROR_PARAMETER_CHECK_FAILED;
    }
    return result;
}

int32_t SelectionAbility::DestroyPanel(const std::shared_ptr<SelectionPanel> &selectionPanel)
{
    SELECTION_HILOGI("SelectionAbility DestroyPanel start.");
    if (selectionPanel == nullptr) {
        SELECTION_HILOGE("panel is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if (!SelectionSystemAbilityUtils::IsSelectionSystemAbilityExistent()) {
        SELECTION_HILOGE("selection system ability is not existent!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    auto ret = selectionPanel->DestroyPanel();
    if (ret == ErrorCode::NO_ERROR) {
        PanelType panelType = selectionPanel->GetPanelType();
        panels_.Erase(panelType);
    }
    return ret;
}

int32_t SelectionAbility::ShowPanel(const std::shared_ptr<SelectionPanel> &selectionpanel)
{
    if (selectionpanel == nullptr) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto ret = selectionpanel->ShowPanel();
    if (ret != ErrorCode::NO_ERROR) {
        SELECTION_HILOGD("failed, ret: %{public}d", ret);
        return ret;
    }
    return ErrorCode::NO_ERROR;
}

int32_t SelectionAbility::HidePanel(const std::shared_ptr<SelectionPanel> &selectionpanel)
{
    if (selectionpanel == nullptr) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto ret = selectionpanel->HidePanel();
    if (ret != ErrorCode::NO_ERROR) {
        SELECTION_HILOGD("failed, ret: %{public}d", ret);
        return ret;
    }
    return ErrorCode::NO_ERROR;
}

} // namespace SelectionFwk
} // namespace OHOS