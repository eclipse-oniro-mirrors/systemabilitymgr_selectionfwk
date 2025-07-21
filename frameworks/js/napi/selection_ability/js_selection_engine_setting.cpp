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

#include "js_selection_engine_setting.h"

#include "event_checker.h"
#include "selection_ability.h"
#include "selection_js_utils.h"
#include "callback_handler.h"
#include "napi/native_node_api.h"
#include "selection_listener_impl.h"
#include "selection_log.h"
#include "util.h"
#include "napi_base_context.h"
#include "js_panel.h"
#include "js_selection_utils.h"
#include "selection_app_validator.h"
#include "selection_api_event_reporter.h"
#include "selection_system_ability_utils.h"

using namespace OHOS;
namespace OHOS {
namespace SelectionFwk {

constexpr size_t ARGC_ONE = 1;
constexpr size_t ARGC_TWO = 2;

const std::string JsSelectionEngineSetting::KDS_CLASS_NAME = "SelectionAbility";
thread_local napi_ref JsSelectionEngineSetting::KDSRef_ = nullptr;
std::mutex JsSelectionEngineSetting::selectionMutex_;
std::shared_ptr<JsSelectionEngineSetting> JsSelectionEngineSetting::selectionDelegate_{ nullptr };
std::mutex JsSelectionEngineSetting::eventHandlerMutex_;
std::shared_ptr<AppExecFwk::EventHandler> JsSelectionEngineSetting::handler_{ nullptr };
sptr<ISelectionListener> JsSelectionEngineSetting::listenerStub_ { nullptr };

napi_value JsSelectionEngineSetting::Subscribe(napi_env env, napi_callback_info info)
{
    if (!SelectionAppValidator::GetInstance().Validate()) {
        JsUtils::ThrowException(env, JsUtils::Convert(ErrorCode::ERROR_INVALID_OPERATION),
            "BundleName is invalid", TYPE_NONE);
        return nullptr;
    }
    size_t argc = ARGC_TWO;
    napi_value argv[ARGC_TWO] = { nullptr };
    napi_value thisVar = nullptr;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));
    std::string type;
    // 2 means least param num.
    if (argc < ARGC_TWO || !JsUtil::GetValue(env, argv[0], type) ||
        !EventChecker::IsValidEventType(EventSubscribeModule::SELECTION_METHOD_ABILITY, type) ||
        JsUtil::GetType(env, argv[1]) != napi_function) {
        SELECTION_HILOGE("subscribe failed, type: %{public}s!", type.c_str());
        JsUtils::ThrowException(env, SFErrorCode::EXCEPTION_PARAMCHECK, "", TYPE_NONE);
        return nullptr;
    }
    SELECTION_HILOGI("subscribe type: %{public}s.", type.c_str());

    std::shared_ptr<JSCallbackObject> callback =
        std::make_shared<JSCallbackObject>(env, argv[1], std::this_thread::get_id(),
            AppExecFwk::EventHandler::Current());
    auto engine = JsSelectionEngineSetting::GetJsSelectionEngineSetting();
    engine->RegisterListener(argv[ARGC_ONE], type, callback);
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}


napi_value JsSelectionEngineSetting::UnSubscribe(napi_env env, napi_callback_info info)
{
    size_t argc = ARGC_TWO;
    napi_value argv[ARGC_TWO] = { nullptr };
    napi_value thisVar = nullptr;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));
    std::string type;
    // 1 means least param num.
    if (argc < 1 || !JsUtil::GetValue(env, argv[0], type) ||
        !EventChecker::IsValidEventType(EventSubscribeModule::SELECTION_METHOD_ABILITY, type)) {
        SELECTION_HILOGE("unsubscribe failed, type: %{public}s!", type.c_str());
        JsUtils::ThrowException(env, SFErrorCode::EXCEPTION_PARAMCHECK, "", TYPE_NONE);
        return nullptr;
    }

    // if the second param is not napi_function/napi_null/napi_undefined, return
    auto paramType = JsUtil::GetType(env, argv[1]);
    if (paramType != napi_function && paramType != napi_null && paramType != napi_undefined) {
        return nullptr;
    }
    // if the second param is napi_function, delete it, else delete all
    argv[1] = paramType == napi_function ? argv[1] : nullptr;

    SELECTION_HILOGD("unsubscribe type: %{public}s.", type.c_str());
    auto delegate = reinterpret_cast<JsSelectionEngineSetting *>(JsUtils::GetNativeSelf(env, info));
    if (delegate == nullptr) {
        return nullptr;
    }
    delegate->UnRegisterListener(argv[ARGC_ONE], type);
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

napi_status JsSelectionEngineSetting::GetContext(napi_env env, napi_value in,
    std::shared_ptr<OHOS::AbilityRuntime::Context> &context)
{
    bool stageMode = false;
    napi_status status = OHOS::AbilityRuntime::IsStageContext(env, in, stageMode);
    if (status != napi_ok || (!stageMode)) {
        SELECTION_HILOGE("it's not in stage mode.");
        return status;
    }
    context = OHOS::AbilityRuntime::GetStageModeContext(env, in);
    if (context == nullptr) {
        SELECTION_HILOGE("context is nullptr.");
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status JsSelectionEngineSetting::CheckPanelInfoAndConext(napi_env env, size_t argc, napi_value *argv,
    std::shared_ptr<PanelContext> ctxt)
{
    PARAM_CHECK_RETURN(env, argc >= ARGC_TWO, "at least two parameters is required.", TYPE_NONE, napi_invalid_arg);
    napi_valuetype valueType = napi_undefined;
    // 0 means parameter of ctx<BaseContext>
    napi_typeof(env, argv[0], &valueType);
    PARAM_CHECK_RETURN(env, valueType == napi_object, "ctx type must be BaseContext.", TYPE_NONE, napi_invalid_arg);
    napi_status status = GetContext(env, argv[0], ctxt->context);
    PARAM_CHECK_RETURN(env, status == napi_ok, "js param context covert failed.", TYPE_NONE, napi_invalid_arg);
    // 1 means parameter of info<PanelInfo>
    napi_typeof(env, argv[1], &valueType);
    PARAM_CHECK_RETURN(env, valueType == napi_object, "param info type must be PanelInfo.", TYPE_NONE,
        napi_invalid_arg);
    status = OHOS::SelectionFwk::JsSelectionUtils::GetValue(env, argv[1], ctxt->panelInfo);
    SELECTION_HILOGD("output js param panelInfo covert , panelType/x/y/width/height: \
        %{public}d/%{public}d/%{public}d/%{public}d/%{public}d.", static_cast<int32_t>(ctxt->panelInfo.panelType),
        ctxt->panelInfo.x, ctxt->panelInfo.y, ctxt->panelInfo.width, ctxt->panelInfo.height);
    PARAM_CHECK_RETURN(env, status == napi_ok, "js param info covert failed!", TYPE_NONE, napi_invalid_arg);
    PARAM_CHECK_RETURN(env, ctxt->panelInfo.x >= 0 && ctxt->panelInfo.y >= 0 && ctxt->panelInfo.width > 0 &&
        ctxt->panelInfo.height > 0, "js param is invalid: x/y cannot be negative, width/height must be positive!",
        TYPE_NONE, napi_invalid_arg);
    return status;
}

napi_value JsSelectionEngineSetting::CreatePanel(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGI("SelectionEngineSetting CreatePanel start.");
    auto eventReporter = std::make_shared<SelectionApiEventReporter>("createPanel");
    auto ctxt = std::make_shared<PanelContext>();
    auto input = [=](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        napi_status status = CheckPanelInfoAndConext(env, argc, argv, ctxt);
        if (status != napi_ok) {
            SELECTION_HILOGI("Check parameter invalid, Report error message to hiappevent");
            eventReporter->WriteEndEvent(SelectionApiEventReporter::API_FAIL, SFErrorCode::EXCEPTION_PARAMCHECK);
        }
        return status;
    };

    auto exec = [eventReporter, ctxt](AsyncCall::Context *ctx) {
        auto ret = SelectionAbility::GetInstance()->CreatePanel(ctxt->context, ctxt->panelInfo, ctxt->panel);
        if (ret != ErrorCode::NO_ERROR) {
            ctxt->SetErrorCode(ret);
            eventReporter->WriteEndEvent(SelectionApiEventReporter::API_FAIL, JsUtils::Convert(ret));
            return;
        }
        ctxt->SetState(napi_ok);
        eventReporter->WriteEndEvent(SelectionApiEventReporter::API_SUCCESS, ret);
    };

    auto output = [ctxt](napi_env env, napi_value *result) -> napi_status {
        JsPanel *jsPanel = nullptr;
        napi_value constructor = JsPanel::Init(env);
        CHECK_RETURN(constructor != nullptr, "failed to get panel constructor!", napi_generic_failure);

        napi_status status = napi_new_instance(env, constructor, 0, nullptr, result);
        CHECK_RETURN(status == napi_ok, "jsPanel new instance failed!", napi_generic_failure);

        status = napi_unwrap(env, *result, (void **)(&jsPanel));
        CHECK_RETURN((status == napi_ok) && (jsPanel != nullptr), "get jsPanel unwrap failed!", napi_generic_failure);
        jsPanel->SetNative(ctxt->panel);
        return napi_ok;
    };

    ctxt->SetAction(std::move(input), std::move(output));
    // 3 means JsAPI:createPanel has 3 params at most.
    AsyncCall asyncCall(env, info, ctxt, 3);
    return asyncCall.Call(env, exec, "createPanel");
}

napi_value JsSelectionEngineSetting::DestroyPanel(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGI("SelectionEngineSetting DestroyPanel start.");
    auto ctxt = std::make_shared<PanelContext>();
    auto input = [ctxt](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        PARAM_CHECK_RETURN(env, argc >= 1, "at least one parameter is required!", TYPE_NONE, napi_invalid_arg);
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[0], &valueType);
        PARAM_CHECK_RETURN(env, valueType == napi_object, "param panel type must be SelectionPanel.", TYPE_NONE,
            napi_invalid_arg);
        bool isPanel = false;
        napi_value constructor = JsPanel::Init(env);
        CHECK_RETURN(constructor != nullptr, "failed to get panel constructor.", napi_invalid_arg);
        napi_status status = napi_instanceof(env, argv[0], constructor, &isPanel);
        CHECK_RETURN((status == napi_ok) && isPanel, "param verification failed, it's not expected panel instance!",
            napi_invalid_arg);
        JsPanel *jsPanel = nullptr;
        status = napi_unwrap(env, argv[0], (void **)(&jsPanel));
        CHECK_RETURN((status == napi_ok) && (jsPanel != nullptr), "failed to unwrap JsPanel!", napi_invalid_arg);
        ctxt->panel = jsPanel->GetNative();
        CHECK_RETURN((ctxt->panel != nullptr), "panel is nullptr!", napi_invalid_arg);
        return status;
    };

    auto exec = [ctxt](AsyncCall::Context *ctx) { ctxt->SetState(napi_ok); };

    auto output = [ctxt](napi_env env, napi_value *result) -> napi_status {
        CHECK_RETURN((ctxt->panel != nullptr), "panel is nullptr!", napi_generic_failure);
        auto errCode = SelectionAbility::GetInstance()->DestroyPanel(ctxt->panel);
        if (errCode != ErrorCode::NO_ERROR) {
            SELECTION_HILOGE("DestroyPanel failed, errCode: %{public}d!", JsUtils::Convert(errCode));
            ctxt->SetErrorCode(errCode);
            return napi_generic_failure;
        }
        ctxt->panel = nullptr;
        return napi_ok;
    };

    ctxt->SetAction(std::move(input), std::move(output));
    // 2 means JsAPI:destroyPanel has 2 params at most.
    AsyncCall asyncCall(env, info, ctxt, ARGC_TWO);
    return asyncCall.Call(env, exec, "destroyPanel");
}

void JsSelectionEngineSetting::RegisterListener(napi_value callback, std::string type,
    std::shared_ptr<JSCallbackObject> callbackObj)
{
    SELECTION_HILOGI("RegisterListener %{public}s", type.c_str());
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (jsCbMap_.empty() || jsCbMap_.find(type) == jsCbMap_.end()) {
        SELECTION_HILOGI("methodName %{public}s is not registered!", type.c_str());
    }
    auto callbacks = jsCbMap_[type];
    bool ret = std::any_of(callbacks.begin(), callbacks.end(), [&callback](std::shared_ptr<JSCallbackObject> cb) {
        return JsUtils::Equals(cb->env_, callback, cb->callback_, cb->threadId_);
    });
    if (ret) {
        SELECTION_HILOGI("JsSelectionEngineSetting callback already registered!");
        return;
    }

    SELECTION_HILOGI("add %{public}s callbackObj into jsCbMap_.", type.c_str());
    jsCbMap_[type].push_back(std::move(callbackObj));
}

void JsSelectionEngineSetting::UnRegisterListener(napi_value callback, std::string type)
{
    SELECTION_HILOGI("unregister listener: %{public}s.", type.c_str());
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (jsCbMap_.empty() || jsCbMap_.find(type) == jsCbMap_.end()) {
        SELECTION_HILOGE("methodName %{public}s is not unregistered!", type.c_str());
        return;
    }

    if (callback == nullptr) {
        jsCbMap_.erase(type);
        SELECTION_HILOGE("callback is nullptr!");
        return;
    }

    for (auto item = jsCbMap_[type].begin(); item != jsCbMap_[type].end(); item++) {
        if ((callback != nullptr) &&
            (JsUtils::Equals((*item)->env_, callback, (*item)->callback_, (*item)->threadId_))) {
            jsCbMap_[type].erase(item);
            break;
        }
    }
    if (jsCbMap_[type].empty()) {
        jsCbMap_.erase(type);
    }

    auto proxy = SelectionSystemAbilityUtils::GetSelectionSystemAbility();
    if (proxy == nullptr || listenerStub_ == nullptr) {
        SELECTION_HILOGE("selection system ability or listenerStub_ is nullptr!");
        return;
    }
    proxy->UnregisterListener(listenerStub_);
    listenerStub_ = nullptr;
}

std::shared_ptr<JsSelectionEngineSetting> JsSelectionEngineSetting::GetJsSelectionEngineSetting()
{
    if (selectionDelegate_ == nullptr) {
        std::lock_guard<std::mutex> lock(selectionMutex_);
        if (selectionDelegate_ == nullptr) {
            std::shared_ptr<JsSelectionEngineSetting> delegate(new JsSelectionEngineSetting);
            if (delegate == nullptr) {
                SELECTION_HILOGE("JsSelectionEngineSetting is nullptr!");
                return nullptr;
            }
            selectionDelegate_ = delegate;
        }
    }
    {
        std::lock_guard<std::mutex> lock(eventHandlerMutex_);
        handler_ = AppExecFwk::EventHandler::Current();
    }
    return selectionDelegate_;
}

SFErrorCode JsSelectionEngineSetting::RegisterListenerToService(
    std::shared_ptr<JsSelectionEngineSetting> &selectionEnging)
{
    auto proxy = SelectionSystemAbilityUtils::GetSelectionSystemAbility();
    if (proxy == nullptr) {
        SELECTION_HILOGE("selection system ability is nullptr!");
        return EXCEPTION_SELECTION_SERVICE;
    }
    listenerStub_ = new (std::nothrow) SelectionListenerImpl(selectionEnging);
    if (listenerStub_ == nullptr) {
        SELECTION_HILOGE("Failed to create SelectionListenerImpl instance.");
        return EXCEPTION_SELECTION_SERVICE;
    }
    SELECTION_HILOGI("Begin calling SA RegisterListener!");
    if (proxy->RegisterListener(listenerStub_) != ERR_OK) {
        return EXCEPTION_SELECTION_SERVICE;
    }

    return EXCEPTION_SUCCESS;
}

SFErrorCode JsSelectionEngineSetting::Register(napi_env env)
{
    auto delegate = GetJsSelectionEngineSetting();
    if (delegate == nullptr) {
        SELECTION_HILOGE("failed to get delegate!");
        return EXCEPTION_SELECTION_SERVICE;
    }
    
    return RegisterListenerToService(delegate);
};

napi_value JsSelectionEngineSetting::Init(napi_env env, napi_value exports)
{
    SELECTION_HILOGI("napi init");
    napi_property_descriptor descriptor[] = {
        DECLARE_NAPI_FUNCTION("on", Subscribe),
        DECLARE_NAPI_FUNCTION("off", UnSubscribe),
        DECLARE_NAPI_FUNCTION("createPanel", CreatePanel),
        DECLARE_NAPI_FUNCTION("destroyPanel", DestroyPanel),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(descriptor) / sizeof(napi_property_descriptor),
        descriptor));
    NAPI_CALL(env, napi_set_named_property(env, exports, "SelectionType", GetJsSelectionTypeProperty(env)));
    if (Register(env) != EXCEPTION_SUCCESS) {
        SELECTION_HILOGE("Failed to register lister to service!");
        return exports;
    }
    SelectionAppValidator::GetInstance().SetValid();
    return exports;
}

napi_value JsSelectionEngineSetting::GetJsSelectionTypeProperty(napi_env env)
{
    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));

    auto ret = JsUtil::Object::WriteProperty(env, obj, "MOUSE_MOVE", static_cast<int32_t>(MOVE_SELECTION));
    if (!ret) {
        SELECTION_HILOGE("Failed to init module selectionInput.selectionManager.SelectionType as MOUSE_MOVE");
        return nullptr;
    }
    ret = JsUtil::Object::WriteProperty(env, obj, "DOUBLE_CLICK", static_cast<int32_t>(DOUBLE_CLICKED_SELECTION));
    if (!ret) {
        SELECTION_HILOGE("Failed to init module selectionInput.selectionManager.SelectionType as DOUBLE_CLICK");
        return nullptr;
    }
    ret = JsUtil::Object::WriteProperty(env, obj, "TRIPLE_CLICK", static_cast<int32_t>(TRIPLE_CLICKED_SELECTION));
    if (!ret) {
        SELECTION_HILOGE("Failed to init module selectionInput.selectionManager.SelectionType as TRIPLE_CLICK");
        return nullptr;
    }
    return obj;
}

std::shared_ptr<AppExecFwk::EventHandler> JsSelectionEngineSetting::GetEventHandler()
{
    std::lock_guard<std::mutex> lock(eventHandlerMutex_);
    return handler_;
}

std::shared_ptr<JsSelectionEngineSetting::SelectionEntry> JsSelectionEngineSetting::GetEntry(const std::string &type,
    EntrySetter entrySetter)
{
    SELECTION_HILOGI("start, type: %{public}s", type.c_str());
    std::shared_ptr<SelectionEntry> entry = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (jsCbMap_[type].empty()) {
            SELECTION_HILOGI("%{public}s cb-vector is empty.", type.c_str());
            return nullptr;
        }
        entry = std::make_shared<SelectionEntry>(jsCbMap_[type], type);
    }
    if (entrySetter != nullptr) {
        entrySetter(*entry);
    }
    return entry;
}

napi_value JsSelectionEngineSetting::Write(napi_env env, const SelectionInfo &selectionInfo)
{
    napi_value jsObject = nullptr;
    napi_create_object(env, &jsObject);
    auto ret = JsUtil::Object::WriteProperty(env, jsObject, "bundleName", selectionInfo.bundleName);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "windowID", selectionInfo.windowId);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "displayID", selectionInfo.displayId);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "endWindowY", selectionInfo.endWindowY);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "endWindowX", selectionInfo.endWindowX);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "startWindowY", selectionInfo.startWindowY);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "startWindowX", selectionInfo.startWindowX);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "endDisplayY", selectionInfo.endDisplayY);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "endDisplayX", selectionInfo.endDisplayX);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "startDisplayY", selectionInfo.startDisplayY);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "startDisplayX", selectionInfo.startDisplayX);
    ret = ret && JsUtil::Object::WriteProperty(env, jsObject, "text", selectionInfo.text);
    SELECTION_HILOGD("write selectionInfo into object, ret=%{public}s", ret ? "true" : "false");
    return ret ? jsObject : JsUtil::Const::Null(env);
}

int32_t JsSelectionEngineSetting::OnSelectionEvent(const SelectionInfo &selectionInfo)
{
    SELECTION_HILOGD("OnSelectionEvent begin");
    std::string type = "selectionCompleted";
    auto entry = GetEntry(type, [&selectionInfo](SelectionEntry &entry) {entry.selectionInfo = selectionInfo; });
    if (entry == nullptr) {
        SELECTION_HILOGE("failed to get SelectionEntry entry!");
        return 1;
    }

    auto eventHandler = GetEventHandler();
    if (eventHandler == nullptr) {
        SELECTION_HILOGE("eventHandler is nullptr!");
        return 1;
    }

    SELECTION_HILOGI("selection text length is [%{public}u]", entry->selectionInfo.text.length());
    auto task = [entry]() {
        auto paramGetter = [entry](napi_env env, napi_value *args, uint8_t argc) -> bool {
            if (argc == 0) {
                return false;
            }
            napi_value jsObject = Write(env, entry->selectionInfo);
            if (jsObject == JsUtil::Const::Null(env)) {
                SELECTION_HILOGE("jsObject is nullptr!");
                return false;
            }
            // 0 means the first param of callback.
            args[0] = jsObject;
            return true;
        };
        // 1 means callback has one param.
        JsCallbackHandler::Traverse(entry->vecCopy, { 1, paramGetter });
    };
    eventHandler->PostTask(task, type, 0, AppExecFwk::EventQueue::Priority::VIP);
    return 0;
}

}
}
