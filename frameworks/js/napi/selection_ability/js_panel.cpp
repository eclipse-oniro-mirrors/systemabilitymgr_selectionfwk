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

#include "js_panel.h"

#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "selection_log.h"
#include "panel_listener_impl.h"
#include "selection_js_utils.h"
#include "selectionmethod_trace.h"
#include "selection_ability.h"
#include "event_checker.h"

namespace OHOS {
namespace SelectionFwk {
using namespace std::chrono;
constexpr int32_t MAX_WAIT_TIME = 10;
const std::string JsPanel::CLASS_NAME = "Panel";
thread_local napi_ref JsPanel::panelConstructorRef_ = nullptr;
std::mutex JsPanel::panelConstructorMutex_;
FFRTBlockQueue<JsEventInfo> JsPanel::jsQueue_{ MAX_WAIT_TIME };
constexpr size_t ARGC_MAX = 6;

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
        DECLARE_NAPI_FUNCTION("startMoving", StartMoving),
        DECLARE_NAPI_FUNCTION("moveTo", MoveTo),
        DECLARE_NAPI_FUNCTION("on", Subscribe),
        DECLARE_NAPI_FUNCTION("off", UnSubscribe)
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
        auto code = ctxt->selectionPanel->SetUiContent(ctxt->path, env);
        jsQueue_.Pop();
        if (code != ErrorCode::NO_ERROR) {
            ctxt->SetErrorCode(code);
            JsUtils::ThrowException(env, JsUtils::Convert(code), "failed to SetUiContent", TYPE_NONE);
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
    auto exec = [env, ctxt](AsyncCall::Context *ctx) {
        jsQueue_.Wait(ctxt->info);
        if (ctxt->selectionPanel == nullptr) {
            SELECTION_HILOGE("selectionPanel is nullptr!");
            jsQueue_.Pop();
            return;
        }
        auto code = SelectionAbility::GetInstance()->ShowPanel(ctxt->selectionPanel);
        jsQueue_.Pop();
        if (code == ErrorCode::NO_ERROR) {
            ctxt->SetState(napi_ok);
            return;
        }
        ctxt->SetErrorCode(code);
        JsUtils::ThrowException(env, JsUtils::Convert(code), "failed to show", TYPE_NONE);
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
    auto exec = [env, ctxt](AsyncCall::Context *ctx) {
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
        JsUtils::ThrowException(env, JsUtils::Convert(code), "failed to hide", TYPE_NONE);
    };
    ctxt->SetAction(std::move(input));
    // 1 means JsAPI:hide has 1 param at most.
    AsyncCall asyncCall(env, info, ctxt, 1);
    return asyncCall.Call(env, exec, "panel.hide");
}

napi_value JsPanel::StartMoving(napi_env env, napi_callback_info info)
{
    napi_value self = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, 0, nullptr, &self, nullptr));
    RESULT_CHECK_RETURN(env, (self != nullptr), JsUtils::Convert(ErrorCode::ERROR_SELECTION_SERVICE),
                        "", TYPE_NONE, JsUtil::Const::Null(env));
    void *native = nullptr;
    NAPI_CALL(env, napi_unwrap(env, self, &native));
    RESULT_CHECK_RETURN(env, (native != nullptr), JsUtils::Convert(ErrorCode::ERROR_SELECTION_SERVICE),
                        "", TYPE_NONE, JsUtil::Const::Null(env));
    auto selectionPanel = reinterpret_cast<JsPanel *>(native)->GetNative();
    if (selectionPanel == nullptr) {
        SELECTION_HILOGE("selectionPanel is nullptr!");
        JsUtils::ThrowException(env, JsUtils::Convert(ErrorCode::ERROR_SELECTION_SERVICE),
            "failed to start moving, selectionPanel is nullptr", TYPE_NONE);
        return JsUtil::Const::Null(env);
    }

    auto ret = selectionPanel->StartMoving();
    if (ret != ErrorCode::NO_ERROR) {
        JsUtils::ThrowException(env, JsUtils::Convert(ret), "failed to start moving", TYPE_NONE);
    }
    return JsUtil::Const::Null(env);
}

napi_value JsPanel::MoveTo(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGD("moveto start!");
    auto ctxt = std::make_shared<PanelContentContext>(env, info);
    auto input = [ctxt](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        napi_status status = napi_generic_failure;
        PARAM_CHECK_RETURN(env, argc > 1, "at least two parameters is required ", TYPE_NONE, status);
        // 0 means the first param x<int32_t>
        PARAM_CHECK_RETURN(env, JsUtils::GetValue(env, argv[0], ctxt->x) == napi_ok, "x type must be number",
            TYPE_NONE, status);
        // 1 means the second param y<int32_t>
        PARAM_CHECK_RETURN(env, JsUtils::GetValue(env, argv[1], ctxt->y) == napi_ok, "y type must be number",
            TYPE_NONE, status);
        ctxt->info = { std::chrono::system_clock::now(), JsEvent::MOVE_TO };
        jsQueue_.Push(ctxt->info);
        return napi_ok;
    };

    auto exec = [env, ctxt](AsyncCall::Context *ctx) {
        int64_t start = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        jsQueue_.Wait(ctxt->info);
        PrintEditorQueueInfoIfTimeout(start, ctxt->info);
        if (ctxt->selectionPanel == nullptr) {
            SELECTION_HILOGE("selectionPanel is nullptr!");
            jsQueue_.Pop();
            return;
        }
        auto code = ctxt->selectionPanel->MoveTo(ctxt->x, ctxt->y);
        jsQueue_.Pop();
        if(code != ErrorCode::NO_ERROR) {
            JsUtils::ThrowException(env, JsUtils::Convert(code), "failed to moveTo", TYPE_NONE);
        }
        ctxt->SetState(napi_ok);
    };
    ctxt->SetAction(std::move(input));
    // 3 means JsAPI:moveTo has 3 params at most.
    AsyncCall asyncCall(env, info, ctxt, 3);
    return asyncCall.Call(env, exec, "moveTo");
}

void JsPanel::PrintEditorQueueInfoIfTimeout(int64_t start, const JsEventInfo &currentInfo)
{
    int64_t end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (end - start >= MAX_WAIT_TIME) {
        JsEventInfo frontInfo;
        auto ret = jsQueue_.GetFront(frontInfo);
        int64_t frontTime = duration_cast<microseconds>(frontInfo.timestamp.time_since_epoch()).count();
        int64_t currentTime = duration_cast<microseconds>(currentInfo.timestamp.time_since_epoch()).count();
        SELECTION_HILOGI("ret:%{public}d,front[%{public}" PRId64 ",%{public}d],current[%{public}" PRId64
            ",%{public}d]", ret, frontTime, static_cast<int32_t>(frontInfo.event), currentTime,
            static_cast<int32_t>(currentInfo.event));
    }
}

napi_value JsPanel::Subscribe(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGD("JsPanel start.");
    size_t argc = ARGC_MAX;
    napi_value argv[ARGC_MAX] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    std::string type;
    // 2 means least param num.
    if (argc < 2 || !JsUtil::GetValue(env, argv[0], type) ||
        !EventChecker::IsValidEventType(EventSubscribeModule::PANEL, type) ||
        JsUtil::GetType(env, argv[1]) != napi_function) {
        SELECTION_HILOGE("subscribe failed, type: %{public}s!", type.c_str());
        return nullptr;
    }
    SELECTION_HILOGD("subscribe type: %{public}s.", type.c_str());
    std::shared_ptr<PanelListenerImpl> observer = PanelListenerImpl::GetInstance();
    auto selectionPanel = UnwrapPanel(env, thisVar);
    if (selectionPanel == nullptr) {
        SELECTION_HILOGE("selectionPanel is nullptr!");
        return nullptr;
    }
    // 1 means the second param callback.
    std::shared_ptr<JSCallbackObject> cbObject = std::make_shared<JSCallbackObject>(
        env, argv[1], std::this_thread::get_id(), AppExecFwk::EventHandler::Current());
    observer->Subscribe(selectionPanel->windowId_, type, cbObject);
    bool ret = selectionPanel->SetPanelStatusListener(observer, type);
    if (!ret) {
        SELECTION_HILOGE("failed to subscribe %{public}s!", type.c_str());
        observer->RemoveInfo(type, selectionPanel->windowId_);
    }
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    return result;
}

napi_value JsPanel::UnSubscribe(napi_env env, napi_callback_info info)
{
    size_t argc = ARGC_MAX;
    napi_value argv[ARGC_MAX] = { nullptr };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    std::string type;
    // 1 means least param num.
    PARAM_CHECK_RETURN(env, argc >= 1, "at least one parameter is required!", TYPE_NONE, nullptr);
    PARAM_CHECK_RETURN(env, JsUtil::GetValue(env, argv[0], type), "type must be string!", TYPE_NONE, nullptr);
    PARAM_CHECK_RETURN(env, EventChecker::IsValidEventType(EventSubscribeModule::PANEL, type),
        "type should be show/hide/sizeChange!", TYPE_NONE, nullptr);
    // if the second param is not napi_function/napi_null/napi_undefined, return
    auto paramType = JsUtil::GetType(env, argv[1]);
    PARAM_CHECK_RETURN(env, (paramType == napi_function || paramType == napi_null || paramType == napi_undefined),
        "callback should be function or null or undefined!", TYPE_NONE, nullptr);
    // if the second param is napi_function, delete it, else delete all
    argv[1] = paramType == napi_function ? argv[1] : nullptr;

    SELECTION_HILOGD("unsubscribe type: %{public}s.", type.c_str());
    std::shared_ptr<PanelListenerImpl> observer = PanelListenerImpl::GetInstance();
    auto selectionPanel = UnwrapPanel(env, thisVar);
    if (selectionPanel == nullptr) {
        SELECTION_HILOGE("selectionPanel is nullptr!");
        return nullptr;
    }

    if (paramType == napi_function) {
        std::shared_ptr<JSCallbackObject> cbObject = std::make_shared<JSCallbackObject>(
            env, argv[1], std::this_thread::get_id(), AppExecFwk::EventHandler::Current());
        observer->RemoveInfo(type, selectionPanel->windowId_, cbObject);
    } else {
        observer->RemoveInfo(type, selectionPanel->windowId_);
        selectionPanel->ClearPanelListener(type);
    }

    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

std::shared_ptr<SelectionPanel> JsPanel::UnwrapPanel(napi_env env, napi_value thisVar)
{
    void *native = nullptr;
    napi_status status = napi_unwrap(env, thisVar, &native);
    CHECK_RETURN((status == napi_ok && native != nullptr), "failed to unwrap!", nullptr);
    auto jsPanel = reinterpret_cast<JsPanel *>(native);
    if (jsPanel == nullptr) {
        return nullptr;
    }
    auto selectionPanel = jsPanel->GetNative();
    CHECK_RETURN(selectionPanel != nullptr, "SelectionPanel is nullptr", nullptr);
    return selectionPanel;
}

} // namespace SelectionFwk
} // namespace OHOS
