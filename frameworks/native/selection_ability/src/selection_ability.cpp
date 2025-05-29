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
    // SELECTION_HILOGI("SelectionAbility start.");
    SELECTION_HILOGI("SelectionAbility CreatePanel start.");
//设置面板高度回调(可暂设定为固定高度)
    // auto panelHeightCallback = [this](uint32_t panelHeight, PanelFlag panelFlag) {
    //     NotifyKeyboardHeight(panelHeight, panelFlag);//NotifyKeyboardHeight()待实现
    // };

    auto flag = panels_.ComputeIfAbsent(panelInfo.panelType,
        [&panelInfo, &context, &selectionPanel](
        // [panelHeightCallback, &panelInfo, &context, &selectionPanel](
            const PanelType &panelType, std::shared_ptr<SelectionPanel> &panel) {
            selectionPanel = std::make_shared<SelectionPanel>();
            // selectionPanel->SetPanelHeightCallback(panelHeightCallback);
            auto ret = selectionPanel->CreatePanel(context, panelInfo);
            if (ret == ErrorCode::NO_ERROR) {
                panel = selectionPanel;
                return true;
            }
            selectionPanel = nullptr;
            return false;
        });//前期测试可以指定flag为true，并调用CreatePanel，感觉还是得实现相关类和方法
    // if (flag && isShowAfterCreate_.load() && panelInfo.panelType == SOFT_KEYBOARD &&//SOFT_KEYBOARD是输入法的键盘类型，划词键盘应该怎么设置？？
    //     panelInfo.panelFlag != FLG_CANDIDATE_COLUMN) {//FLG_CANDIDATE_COLUMN候选列表框
    //     isShowAfterCreate_.store(false);
    //     auto task = std::make_shared<TaskSsaShowKeyboard>();//TaskImsaShowKeyboard待添加
    //     TaskManager::GetInstance().PostTask(task);//同级目录待实现
    // }
    return flag ? ErrorCode::NO_ERROR : ErrorCode::ERROR_OPERATE_PANEL;
}

// void SelectionAbility::NotifyKeyboardHeight(uint32_t panelHeight, PanelFlag panelFlag)
// {
//     auto channel = GetInputDataChannelProxy();
//     if (channel == nullptr) {
//         SELECTION_HILOGE("channel is nullptr!");
//         return;
//     }
//     SELECTION_HILOGD("notify panel height: %{public}u, flag: %{public}d.", panelHeight, static_cast<int32_t>(panelFlag));
//     if (panelFlag != PanelFlag::FLG_FIXED) {
//         channel->NotifyKeyboardHeight(0);
//         return;
//     }
//     channel->NotifyKeyboardHeight(panelHeight);
// }

// std::shared_ptr<InputDataChannelProxy> SelectionAbility::GetInputDataChannelProxy()
// {
//     std::lock_guard<std::mutex> lock(dataChannelLock_);
//     return dataChannelProxy_;
// }

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