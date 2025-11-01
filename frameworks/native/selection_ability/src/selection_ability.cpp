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

    if (!SelectionSystemAbilityUtils::IsSelectionSystemAbilityExistent()) {
        SELECTION_HILOGE("selection system ability is not existent!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    if (!SelectionAppValidator::GetInstance().Validate()) {
        SELECTION_HILOGE("bundleName is not valid");
        return ErrorCode::ERROR_INVALID_OPERATION;
    }

    auto panel = std::make_shared<SelectionPanel>();
    if (panel == nullptr) {
        SELECTION_HILOGE("Construct selectionPanel failed");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    selectionPanel = panel;

    return selectionPanel->CreatePanel(context, panelInfo);
}

int32_t SelectionAbility::DestroyPanel(const std::shared_ptr<SelectionPanel> &selectionPanel)
{
    SELECTION_HILOGI("SelectionAbility DestroyPanel start.");
    if (selectionPanel == nullptr) {
        SELECTION_HILOGE("panel is nullptr!");
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }

    return selectionPanel->DestroyPanel();
}

int32_t SelectionAbility::ShowPanel(const std::shared_ptr<SelectionPanel> &selectionPanel)
{
    if (selectionPanel == nullptr) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto ret = selectionPanel->ShowPanel();
    if (ret != ErrorCode::NO_ERROR) {
        SELECTION_HILOGD("failed, ret: %{public}d", ret);
        return ret;
    }

    PushPanel(selectionPanel);
    return ErrorCode::NO_ERROR;
}

int32_t SelectionAbility::HidePanel(const std::shared_ptr<SelectionPanel> &selectionPanel)
{
    if (selectionPanel == nullptr) {
        return ErrorCode::ERROR_SELECTION_SERVICE;
    }
    auto ret = selectionPanel->HidePanel();
    if (ret != ErrorCode::NO_ERROR) {
        SELECTION_HILOGD("failed, ret: %{public}d", ret);
        return ret;
    }
    return ErrorCode::NO_ERROR;
}

void SelectionAbility::Dispose(uint32_t winId)
{
    SELECTION_HILOGI("Dispose start, winid:%{public}d, panels_ size: %{public}zu", winId, panels_.size());

    auto selectionPanelOpt = PopPanel();
    if (!selectionPanelOpt.has_value()) {
        return;
    }

    auto selectionPanel = selectionPanelOpt.value();
    if (selectionPanel == nullptr) {
        SELECTION_HILOGI("selectionPanel is nullptr.");
        return;
    }

    if (selectionPanel->GetPanelType() == PanelType::MENU_PANEL) {
        selectionPanel->HidePanel();
    } else {
        selectionPanel->DestroyPanel();
    }
}

void SelectionAbility::PushPanel(const std::shared_ptr<SelectionPanel> &selectionPanel)
{
    std::lock_guard<std::mutex> lock(panelsMutex_);
    panels_.push(selectionPanel);
}

std::optional<std::shared_ptr<SelectionPanel>> SelectionAbility::PopPanel()
{
    std::lock_guard<std::mutex> lock(panelsMutex_);
    if (panels_.empty()) {
        SELECTION_HILOGI("No left panel to dispose.");
        return std::nullopt;
    }

    auto selectionPanel = panels_.front();

    panels_.pop();
    return selectionPanel;
}
} // namespace SelectionFwk
} // namespace OHOS