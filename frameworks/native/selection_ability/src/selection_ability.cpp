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
            instance_->Initialize();
        }
    }
    return instance_;
}

void SelectionAbility::Initialize()
{
    SELECTION_HILOGD("SelectionAbility init.");
    // sptr<SelectionCoreStub> coreStub = new (std::nothrow) SelectionCoreServiceImpl();//核心功能，处理文本操作
    // if (coreStub == nullptr) {
    //     SELECTION_HILOGE("failed to create core!");
    //     return;
    // }
    // sptr<SelectionAgentStub> agentStub = new (std::nothrow) SelectionAgentServiceImpl();//服务代理，将核心功能暴露给外部，处理通信和事件分发，
    // if (agentStub == nullptr) {
    //     SELECTION_HILOGE("failed to create agent!");
    //     return;
    // }
    // agentStub_ = agentStub;
    // coreStub_ = coreStub;
}

int32_t SelectionAbility::CreatePanel(const std::shared_ptr<AbilityRuntime::Context> &context,
    const PanelInfo &panelInfo, std::shared_ptr<SelectionPanel> &selectionPanel)
{
    SELECTION_HILOGI("SelectionAbility CreatePanel start.");

    auto flag = panels_.ComputeIfAbsent(panelInfo.panelType,
        [&panelInfo, &context, &selectionPanel](
            const PanelType &panelType, std::shared_ptr<SelectionPanel> &panel) {
            selectionPanel = std::make_shared<SelectionPanel>();
            auto ret = selectionPanel->CreatePanel(context, panelInfo);
            if (ret == ErrorCode::NO_ERROR) {
                panel = selectionPanel;
                return true;
            }
            selectionPanel = nullptr;
            return false;
        });
    return flag ? ErrorCode::NO_ERROR : ErrorCode::ERROR_OPERATE_PANEL;
}

int32_t SelectionAbility::DestroyPanel(const std::shared_ptr<SelectionPanel> &selectionPanel)
{
    SELECTION_HILOGI("SelectionAbility DestroyPanel start.");
    if (selectionPanel == nullptr) {
        SELECTION_HILOGE("panel is nullptr!");
        return ErrorCode::ERROR_BAD_PARAMETERS;
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
        return ErrorCode::ERROR_BAD_PARAMETERS;
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
        return ErrorCode::ERROR_BAD_PARAMETERS;
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