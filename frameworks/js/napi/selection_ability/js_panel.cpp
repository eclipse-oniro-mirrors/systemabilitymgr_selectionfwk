/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "js_panel.h"

#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "selection_log.h"
#include "panel_listener_impl.h"
#include "js_utils.h"
#include "selectionmethod_trace.h"


#include "selection_ability.h"

namespace OHOS {
namespace SelectionFwk {
constexpr int32_t MAX_WAIT_TIME = 10;
const std::string JsPanel::CLASS_NAME = "Panel";
thread_local napi_ref JsPanel::panelConstructorRef_ = nullptr;
std::mutex JsPanel::panelConstructorMutex_;
FFRTBlockQueue<JsEventInfo> JsPanel::jsQueue_{ MAX_WAIT_TIME };

napi_value JsPanel::Init(napi_env env)
{
    SELECTION_HILOGI("JsPanel start.");
    napi_value constructor = nullptr;
    std::lock_guard<std::mutex> lock(panelConstructorMutex_);
    if (panelConstructorRef_ != nullptr) {
        napi_status status = napi_get_reference_value(env, panelConstructorRef_, &constructor);
        CHECK_RETURN(status == napi_ok, "failed to get jsPanel constructor.", nullptr);
        return constructor;
    }
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("setUiContent", SetUiContent),
        DECLARE_NAPI_FUNCTION("show", Show),
        DECLARE_NAPI_FUNCTION("hide", Hide),
        // DECLARE_NAPI_FUNCTION("startMoving", StartMoving),
    };
    NAPI_CALL(env, napi_define_class(env, CLASS_NAME.c_str(), CLASS_NAME.size(), JsNew, nullptr,
                       sizeof(properties) / sizeof(napi_property_descriptor), properties, &constructor));
    CHECK_RETURN(constructor != nullptr, "failed to define class!", nullptr);
    NAPI_CALL(env, napi_create_reference(env, constructor, 1, &panelConstructorRef_));
    return constructor;
}

napi_value JsPanel::JsNew(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGD("create panel instance start.");
    std::shared_ptr<PanelListenerImpl> panelImpl = PanelListenerImpl::GetInstance();
    if (panelImpl != nullptr) {
        SELECTION_HILOGD("set eventHandler.");
        panelImpl->SetEventHandler(AppExecFwk::EventHandler::Current());
    }
    JsPanel *panel = new (std::nothrow) JsPanel();
    CHECK_RETURN(panel != nullptr, "no memory for JsPanel!", nullptr);
    auto finalize = [](napi_env env, void *data, void *hint) {
        SELECTION_HILOGD("jsPanel finalize.");
        auto *jsPanel = reinterpret_cast<JsPanel *>(data);
        CHECK_RETURN_VOID(jsPanel != nullptr, "finalize nullptr!");
        jsPanel->GetNative() = nullptr;
        delete jsPanel;
    };
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
    if (status != napi_ok) {
        SELECTION_HILOGE("failed to get cb info: %{public}d!", status);
        delete panel;
        return nullptr;
    }
    status = napi_wrap(env, thisVar, panel, finalize, nullptr, nullptr);
    if (status != napi_ok) {
        SELECTION_HILOGE("failed to wrap: %{public}d!", status);
        delete panel;
        return nullptr;
    }
    return thisVar;
}

JsPanel::~JsPanel()
{
    selectionPanel_ = nullptr;
}

void JsPanel::SetNative(const std::shared_ptr<SelectionPanel> &panel)
{
    selectionPanel_ = panel;
}

std::shared_ptr<SelectionPanel> JsPanel::GetNative()
{
    return selectionPanel_;
}

napi_value JsPanel::SetUiContent(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGI("JsPanel start.");
    auto ctxt = std::make_shared<PanelContentContext>(env, info);
    auto input = [ctxt](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        napi_status status = napi_generic_failure;
        PARAM_CHECK_RETURN(env, argc >= 1, "at least one parameter is required!", TYPE_NONE, status);
        // 0 means the first param path<std::string>
        PARAM_CHECK_RETURN(env, JsUtils::GetValue(env, argv[0], ctxt->path) == napi_ok,
            "js param path covert failed, must be string!", TYPE_NONE, status);
        // if type of argv[1] is object, we will get value of 'storage' from it.
        // if (argc >= 2) {
        //     napi_valuetype valueType = napi_undefined;
        //     status = napi_typeof(env, argv[1], &valueType);
        //     CHECK_RETURN(status == napi_ok, "get valueType failed!", status);
        //     if (valueType == napi_object) {
        //         napi_ref storage = nullptr;
        //         napi_create_reference(env, argv[1], 1, &storage);
        //         auto contentStorage = (storage == nullptr) ? nullptr
        //                                                    : std::shared_ptr<NativeReference>(
        //                                                          reinterpret_cast<NativeReference *>(storage));
        //         ctxt->contentStorage = contentStorage;
        //     }
        // }
        ctxt->info = { std::chrono::system_clock::now(), JsEvent::SET_UI_CONTENT };
        jsQueue_.Push(ctxt->info);
        return napi_ok;
    };

    auto exec = [ctxt](AsyncCall::Context *ctx) { ctxt->SetState(napi_ok); };
    auto output = [ctxt](napi_env env, napi_value *result) -> napi_status {
        jsQueue_.Wait(ctxt->info);
        if (ctxt->selectionPanel == nullptr) {
            SELECTION_HILOGE("selectionPanel is nullptr!");
            jsQueue_.Pop();
            return napi_generic_failure;
        }
        // auto code = ctxt->selectionPanel->SetUiContent(ctxt->path, env, ctxt->contentStorage);
        auto code = ctxt->selectionPanel->SetUiContent(ctxt->path, env);
        jsQueue_.Pop();
        if (code == ErrorCode::ERROR_PARAMETER_CHECK_FAILED) {
            ctxt->SetErrorCode(code);
            ctxt->SetErrorMessage("path should be a path to specific page.");
            return napi_generic_failure;
        }
        return napi_ok;
    };
    ctxt->SetAction(std::move(input), std::move(output));
    // 2 means JsAPI:setUiContent has 2 params at most.
    AsyncCall asyncCall(env, info, ctxt, 2);
    return asyncCall.Call(env, exec, "setUiContent");
}

napi_value JsPanel::Show(napi_env env, napi_callback_info info)
{
    SelectionMethodSyncTrace tracer("JsPanel_Show");
    auto ctxt = std::make_shared<PanelContentContext>(env, info);
    auto input = [ctxt](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        ctxt->info = { std::chrono::system_clock::now(), JsEvent::SHOW };
        jsQueue_.Push(ctxt->info);
        return napi_ok;
    };
    auto exec = [ctxt](AsyncCall::Context *ctx) {
        jsQueue_.Wait(ctxt->info);
        if (ctxt->selectionPanel == nullptr) {
            SELECTION_HILOGE("selectionPanel is nullptr!");
            jsQueue_.Pop();
            return;
        }
        auto code = SelectionAbility::GetInstance()->ShowPanel(ctxt->selectionPanel);
        if (code == ErrorCode::NO_ERROR) {
            ctxt->SetState(napi_ok);
            jsQueue_.Pop();
            return;
        }
        jsQueue_.Pop();
        ctxt->SetErrorCode(code);
    };
    ctxt->SetAction(std::move(input));
    // 1 means JsAPI:show has 1 param at most.
    AsyncCall asyncCall(env, info, ctxt, 1);
    return asyncCall.Call(env, exec, "show");
}

napi_value JsPanel::Hide(napi_env env, napi_callback_info info)
{
    SelectionMethodSyncTrace tracer("JsPanel_Hide");
    auto ctxt = std::make_shared<PanelContentContext>(env, info);
    
    auto input = [ctxt](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        ctxt->info = { std::chrono::system_clock::now(), JsEvent::HIDE };
        jsQueue_.Push(ctxt->info);
        return napi_ok;
    };
    auto exec = [ctxt](AsyncCall::Context *ctx) {
        jsQueue_.Wait(ctxt->info);
        if (ctxt->selectionPanel == nullptr) {
            SELECTION_HILOGE("selectionPanel is nullptr!");
            jsQueue_.Pop();
            return;
        }
        auto code = SelectionAbility::GetInstance()->HidePanel(ctxt->selectionPanel);
        jsQueue_.Pop();
        if (code == ErrorCode::NO_ERROR) {
            ctxt->SetState(napi_ok);
            return;
        }
        ctxt->SetErrorCode(code);
    };
    ctxt->SetAction(std::move(input));
    // 1 means JsAPI:hide has 1 param at most.
    AsyncCall asyncCall(env, info, ctxt, 1);
    return asyncCall.Call(env, exec, "panel.hide");
}

// napi_value JsPanel::StartMoving(napi_env env, napi_callback_info info)
// {
//     napi_value self = nullptr;
//     NAPI_CALL(env, napi_get_cb_info(env, info, 0, nullptr, &self, nullptr));
//     RESULT_CHECK_RETURN(env, (self != nullptr), JsUtils::Convert(ErrorCode::ERROR_IME),
//                         "", TYPE_NONE, JsUtil::Const::Null(env));
//     void *native = nullptr;
//     NAPI_CALL(env, napi_unwrap(env, self, &native));
//     RESULT_CHECK_RETURN(env, (native != nullptr), JsUtils::Convert(ErrorCode::ERROR_IME),
//                         "", TYPE_NONE, JsUtil::Const::Null(env));
//     auto selectionPanel = reinterpret_cast<JsPanel *>(native)->GetNative();
//     if (selectionPanel == nullptr) {
//         SELECTION_HILOGE("selectionPanel is nullptr!");
//         JsUtils::ThrowException(env, JsUtils::Convert(ErrorCode::ERROR_IME),
//             "failed to start moving, selectionPanel is nullptr", TYPE_NONE);
//         return JsUtil::Const::Null(env);
//     }

//     auto ret = selectionPanel->StartMoving();
//     if (ret != ErrorCode::NO_ERROR) {
//         JsUtils::ThrowException(env, JsUtils::Convert(ret), "failed to start moving", TYPE_NONE);
//     }
//     return JsUtil::Const::Null(env);
// }

} // namespace SelectionFwk
} // namespace OHOS
