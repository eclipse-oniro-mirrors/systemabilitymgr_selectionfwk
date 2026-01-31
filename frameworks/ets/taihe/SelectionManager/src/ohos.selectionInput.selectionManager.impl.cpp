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

#include "ohos.selectionInput.selectionManager.proj.hpp"
#include "ohos.selectionInput.selectionManager.impl.hpp"
#include "stdexcept"
#include "panel_listener_impl.h"
#include "ani_common.h"
#include "selection_ability.h"
#include "ets_selection_engine_setting.h"
#include "selection_client.h"
#include "ani_base_context.h"

using namespace taihe;
using namespace OHOS::SelectionFwk;

namespace {

std::map<std::string, std::vector<std::unique_ptr<callbackType>>> selectionManagerJsCbMap_;

class PanelImpl {
public:
    PanelImpl(std::shared_ptr<SelectionPanel> selectionPanel): selectionPanel_(selectionPanel) {
        // Don't forget to implement the constructor.
        SELECTION_HILOGI("taihe::PanelImpl constructor start");
    }

    void setUiContentSync(::taihe::string_view path) {
        SELECTION_HILOGI("taihe::setUiContent start");
        ani_env *env = get_env();
        int32_t errCode = selectionPanel_->SetUiContent(std::string(path), env);
        if (errCode != EXCEPTION_SUCCESS) {
            set_business_error(JsUtils::Convert(errCode), JsUtils::ToMessage(JsUtils::Convert(errCode)));
        }
    }

    void showSync() {
        SELECTION_HILOGI("taihe::Show start");
        int32_t errCode = SelectionAbility::GetInstance()->ShowPanel(selectionPanel_);
        if (errCode != EXCEPTION_SUCCESS) {
            set_business_error(JsUtils::Convert(errCode), JsUtils::ToMessage(JsUtils::Convert(errCode)));
        }
    }

    void hideSync() {
        SELECTION_HILOGI("taihe::Hide start");
        int32_t errCode = SelectionAbility::GetInstance()->HidePanel(selectionPanel_);
        if (errCode != EXCEPTION_SUCCESS) {
            set_business_error(JsUtils::Convert(errCode), JsUtils::ToMessage(JsUtils::Convert(errCode)));
        }
    }

    void startMovingSync() {
        SELECTION_HILOGI("taihe::startMoving start");
        int32_t errCode = selectionPanel_->StartMoving();
        if (errCode != EXCEPTION_SUCCESS) {
            set_business_error(JsUtils::Convert(errCode), JsUtils::ToMessage(JsUtils::Convert(errCode)));
        }
    }

    void moveToGlobalDisplaySync(int32_t x, int32_t y) {
        SELECTION_HILOGI("taihe::moveToGlobalDisplay start");
        int32_t errCode = selectionPanel_->MoveTo(x, y);
        if (errCode != EXCEPTION_SUCCESS) {
            set_business_error(JsUtils::Convert(errCode), JsUtils::ToMessage(JsUtils::Convert(errCode)));
        }
    }

    void onDestroy(taihe::callback_view<void(UndefinedType_t const&)> callback) {
        std::string type = callbackType_Destroy;
        panelSubscribe(type, callback);
        SELECTION_HILOGI("register onDestroy success");
    }

    void offDestroy(::taihe::optional_view<::taihe::callback<void(UndefinedType_t const&)>> callback) {
        std::string type = callbackType_Destroy;
        if (callback.has_value()) {
            panelUnSubscribe(type, callback.value());
        } else {
            panelUnSubscribe(type);
        }
        
    }

    void onHide(taihe::callback_view<void(UndefinedType_t const&)> callback) {
        std::string type = callbackType_Hide;
        panelSubscribe(type, callback);
        SELECTION_HILOGI("register onHide success");
    }

    void offHide(::taihe::optional_view<::taihe::callback<void(UndefinedType_t const&)>> callback) {
        std::string type = callbackType_Hide;
        if (callback.has_value()) {
            panelUnSubscribe(type, callback.value());
        } else {
            panelUnSubscribe(type);
        }
    }

    void destroyPanel() {
        int32_t errCode = SelectionAbility::GetInstance()->DestroyPanel(selectionPanel_);
        if (errCode != EXCEPTION_SUCCESS) {
            set_business_error(JsUtils::Convert(errCode), JsUtils::ToMessage(JsUtils::Convert(errCode)));
        }
    }
    std::shared_ptr<SelectionPanel> GetSelectionPanel() {
        return selectionPanel_;
    }

    void panelSubscribe(const std::string& type, callbackType &&cbObject) {
        SELECTION_HILOGI("panelSubscribe: register %{public}s callback", type.c_str());
        std::shared_ptr<PanelListenerImpl> observer = PanelListenerImpl::GetInstance();
        auto windowId = selectionPanel_->GetWindowId();
        observer->Subscribe(windowId, type, std::forward<callbackType>(cbObject));
        bool ret = selectionPanel_->SetPanelStatusListener(observer, type);
        if (!ret) {
            SELECTION_HILOGE("failed to subscribe %{public}s!", type.c_str());
            observer->RemoveInfo(type, selectionPanel_->GetWindowId());
        }
    }

    void panelUnSubscribe(const std::string& type, const callbackType &cbObject) {
        SELECTION_HILOGI("panelUnSubscribe: Unregister %{public}s callback", type.c_str());
        std::shared_ptr<PanelListenerImpl> observer = PanelListenerImpl::GetInstance();
        observer->RemoveInfo(type, selectionPanel_->GetWindowId(), cbObject);
    }

    void panelUnSubscribe(const std::string& type) {
        SELECTION_HILOGI("panelUnSubscribe: Unregister %{public}s callback", type.c_str());
        std::shared_ptr<PanelListenerImpl> observer = PanelListenerImpl::GetInstance();
        observer->RemoveInfo(type, selectionPanel_->GetWindowId());
        selectionPanel_->ClearPanelListener(type);
    }

private:
    std::shared_ptr<SelectionPanel> selectionPanel_;
};

void onSelectionComplete(::taihe::callback_view<void(::ohos::selectionInput::selectionManager::SelectionInfo const& a)> f) {
    std::string type = callbackType_SelectionComplete;
    EtsSelectionEngineSetting::Subscribe(type, f);
}

void offSelectionComplete(::taihe::optional_view<::taihe::callback<void(::ohos::selectionInput::selectionManager::SelectionInfo const& a)>> f) {
    std::string type = callbackType_SelectionComplete;
    if (f.has_value()) {
        EtsSelectionEngineSetting::UnSubscribe(type, f.value());
    } else {
        EtsSelectionEngineSetting::UnSubscribe(type);
    }
}

::taihe::string getSelectionContentSync() {
    SELECTION_HILOGI("GetSelectionContent");
    std::string selectionContent;
    SelectionClient::GetInstance().GetSelectionContent(selectionContent);
    return ::taihe::string(selectionContent);
}

const int MENU_PANEL = 1;

::ohos::selectionInput::selectionManager::Panel createPanelSync(uintptr_t ctx, ::ohos::selectionInput::SelectionPanel::PanelInfo const& info) {
    // The parameters in the make_holder function should be of the same type
    // as the parameters in the constructor of the actual implementation class.
    //ctx->Context, info->panelInfo
    ani_env *env = get_env();
    ani_object ani_obj = reinterpret_cast<ani_object>(ctx);
    std::shared_ptr<OHOS::AbilityRuntime::Context> context = OHOS::AbilityRuntime::GetStageModeContext(env, ani_obj);
    OHOS::SelectionFwk::PanelInfo panelInfo;
    panelInfo.panelType = (info.panelType == MENU_PANEL) ?
        OHOS::SelectionFwk::PanelType::MENU_PANEL : OHOS::SelectionFwk::PanelType::MAIN_PANEL;
    panelInfo.x = info.x;
    panelInfo.y = info.y;
    panelInfo.width = info.width;
    panelInfo.height = info.height;
    std::shared_ptr<SelectionPanel> selectionPanel;
    SelectionAbility::GetInstance()->CreatePanel(context, panelInfo, selectionPanel);

    return taihe::make_holder<PanelImpl, ::ohos::selectionInput::selectionManager::Panel>(selectionPanel);
}

void destroyPanelSync(::ohos::selectionInput::selectionManager::weak::Panel panel) {
    panel->destroyPanel();
}
}  // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_onSelectionComplete(onSelectionComplete);
TH_EXPORT_CPP_API_offSelectionComplete(offSelectionComplete);
TH_EXPORT_CPP_API_getSelectionContentSync(getSelectionContentSync);
TH_EXPORT_CPP_API_createPanelSync(createPanelSync);
TH_EXPORT_CPP_API_destroyPanelSync(destroyPanelSync);
// NOLINTEND
