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

#include "js_selection_extension.h"
#include "js_runtime.h"
#include "js_runtime_utils.h"
#include "js_selection_extension_context.h"
#include "napi/native_api.h"
#include "napi_common_want.h"
#include "napi_remote_object.h"
#include "selection_log.h"

namespace OHOS::AbilityRuntime {
namespace {
constexpr size_t ARGC_ONE = 1;
}

napi_value AttachSelectionExtensionContext(napi_env env, void* value, void*)
{
    SELECTION_HILOGI("AttachSelectionExtensionContext start.");
    if (value == nullptr) {
        SELECTION_HILOGW("parameter is invalid.");
        return nullptr;
    }
    auto ptr = reinterpret_cast<std::weak_ptr<SelectionExtensionContext>*>(value)->lock();
    if (ptr == nullptr) {
        SELECTION_HILOGW("context is invalid.");
        return nullptr;
    }
    napi_value object = CreateJsSelectionExtensionContext(env, ptr);
    auto systemModule = JsRuntime::LoadSystemModuleByEngine(env, "selectionInput.SelectionExtensionContext",
        &object, 1);
    if (systemModule == nullptr) {
        SELECTION_HILOGE("failed to load system module by engine!");
        return nullptr;
    }
    auto contextObj = systemModule->GetNapiValue();
    napi_coerce_to_native_binding_object(env, contextObj, DetachCallbackFunc, AttachSelectionExtensionContext, value,
                                         nullptr);
    auto workContext = new (std::nothrow) std::weak_ptr<SelectionExtensionContext>(ptr);
    if (workContext == nullptr) {
        SELECTION_HILOGE("workContext is nullptr!");
        return nullptr;
    }
    napi_status status = napi_wrap(
        env, contextObj, workContext,
        [](napi_env, void* data, void*) {
            SELECTION_HILOGI("finalizer for weak_ptr input method extension context is called.");
            delete static_cast<std::weak_ptr<SelectionExtensionContext>*>(data);
        },
        nullptr, nullptr);
    if (status != napi_ok) {
        SELECTION_HILOGE("SelectionExtensionContext wrap failed: %{public}d!", status);
        delete workContext;
        return nullptr;
    }
    return object;
}

JsSelectionExtension::JsSelectionExtension(JsRuntime& jsRuntime) : jsRuntime_(jsRuntime) {}
JsSelectionExtension::~JsSelectionExtension()
{
    jsRuntime_.FreeNativeReference(std::move(jsObj_));
}

JsSelectionExtension* JsSelectionExtension::Create(const std::unique_ptr<Runtime>& runtime)
{
    return new (std::nothrow) JsSelectionExtension(static_cast<JsRuntime&>(*runtime));
}

void JsSelectionExtension::Init(const std::shared_ptr<AbilityLocalRecord>& record,
                                const std::shared_ptr<OHOSApplication>& application,
                                std::shared_ptr<AbilityHandler>& handler,
                                const sptr<IRemoteObject>& token)
{
    SELECTION_HILOGI("%{public}s start.", __func__);
    SelectionExtension::Init(record, application, handler, token);
    std::string srcPath;
    GetSrcPath(srcPath);
    if (srcPath.empty()) {
        SELECTION_HILOGE("failed to get srcPath!");
        return;
    }

    std::string moduleName(Extension::abilityInfo_->moduleName);
    moduleName.append("::").append(abilityInfo_->name);
    SELECTION_HILOGI("JsSelectionExtension, module: %{public}s, srcPath:%{public}s.", moduleName.c_str(),
                     srcPath.c_str());
    HandleScope handleScope(jsRuntime_);
    jsObj_ = jsRuntime_.LoadModule(moduleName, srcPath, abilityInfo_->hapPath,
                                   abilityInfo_->compileMode == CompileMode::ES_MODULE);
    if (jsObj_ == nullptr) {
        SELECTION_HILOGE("failed to init jsObj_!");
        return;
    }

    napi_env env = jsRuntime_.GetNapiEnv();
    napi_value obj = jsObj_->GetNapiValue();
    if (obj == nullptr) {
        SELECTION_HILOGE("failed to get JsSelectionExtension object!");
        return;
    }
    BindContext(env, obj);
    SELECTION_HILOGI("%{public}s end.", __func__);
}

sptr<IRemoteObject> JsSelectionExtension::OnConnect(const AAFwk::Want& want)
{
    SELECTION_HILOGI("%{public}s start.", __func__);
    HandleScope handleScope(jsRuntime_);
    Extension::OnConnect(want);
    napi_env env = jsRuntime_.GetNapiEnv();
    napi_value napiWant = OHOS::AppExecFwk::WrapWant(env, want);
    napi_value argv[] = {napiWant};
    napi_value result = CallObjectMethod("onConnect", argv, ARGC_ONE);
    auto remoteObj = NAPI_ohos_rpc_getNativeRemoteObject(env, result);
    if (remoteObj == nullptr) {
        SELECTION_HILOGE("remoteObj is nullptr.");
    }
    SELECTION_HILOGI("%{public}s end.", __func__);
    return remoteObj;
}

void JsSelectionExtension::OnDisconnect(const AAFwk::Want& want)
{
    SELECTION_HILOGI("%{public}s start.", __func__);
    HandleScope handleScope(jsRuntime_);
    Extension::OnDisconnect(want);
    napi_env env = jsRuntime_.GetNapiEnv();
    napi_value napiWant = OHOS::AppExecFwk::WrapWant(env, want);
    napi_value argv[] = {napiWant};
    CallObjectMethod("onDisconnect", argv, ARGC_ONE);
    SELECTION_HILOGI("%{public}s end.", __func__);
}

napi_value JsSelectionExtension::CallObjectMethod(const char* methodName, const napi_value* argv, size_t argc)
{
    SELECTION_HILOGI("JsSelectionExtension::CallObjectMethod(%{public}s), start.", methodName);

    if (argc > 0 && argv == nullptr) {
        SELECTION_HILOGE("argc > 0 && argv == nullptr.");
        return nullptr;
    }

    if (jsObj_ == nullptr) {
        SELECTION_HILOGE("not found JsSelectionExtension.js.");
        return nullptr;
    }

    napi_env env = jsRuntime_.GetNapiEnv();
    napi_value obj = jsObj_->GetNapiValue();
    if (obj == nullptr) {
        SELECTION_HILOGE("failed to get JsSelectionExtension object!");
        return nullptr;
    }

    napi_value method = nullptr;
    napi_get_named_property(env, obj, methodName, &method);
    if (method == nullptr) {
        SELECTION_HILOGE("failed to get '%{public}s' from JsSelectionExtension object!", methodName);
        return nullptr;
    }
    napi_value result = nullptr;
    if (napi_call_function(env, obj, method, argc, argv, &result) != napi_ok) {
        SELECTION_HILOGE("JsSelectionExtension::CallFunction(%{public}s), failed.", methodName);
        return nullptr;
    }
    SELECTION_HILOGI("JsSelectionExtension::CallFunction(%{public}s), success.", methodName);
    return result;
}

void JsSelectionExtension::GetSrcPath(std::string& srcPath)
{
    if (!Extension::abilityInfo_->isModuleJson) {
        std::filesystem::path modulePath(Extension::abilityInfo_->package);
        modulePath /= "assets/js";
        
        if (!Extension::abilityInfo_->srcPath.empty()) {
            modulePath /= Extension::abilityInfo_->srcPath;
        }
        
        modulePath /= (Extension::abilityInfo_->name + ".abc");
        srcPath = modulePath.string();
        return;
    }

    if (!Extension::abilityInfo_->srcEntrance.empty()) {
        std::filesystem::path modulePath(Extension::abilityInfo_->moduleName);
        modulePath /= Extension::abilityInfo_->srcEntrance;
        modulePath.replace_extension(".abc");
        srcPath = modulePath.string();
    }
}

void JsSelectionExtension::BindContext(napi_env env, napi_value obj)
{
    SELECTION_HILOGI("%{public}s start.", __func__);
    auto context = GetContext();
    if (context == nullptr) {
        SELECTION_HILOGE("failed to get context!");
        return;
    }
    SELECTION_HILOGD("JsSelectionExtension::Init CreateJsSelectionExtensionContext.");
    napi_value contextObj = CreateJsSelectionExtensionContext(env, context);
    auto shellContextRef = jsRuntime_.LoadSystemModule("selectionInput.SelectionExtensionContext",
        &contextObj, ARGC_ONE);
    if (shellContextRef == nullptr) {
        SELECTION_HILOGE("shellContextRef is nullptr!");
        return;
    }
    contextObj = shellContextRef->GetNapiValue();
    if (contextObj == nullptr) {
        SELECTION_HILOGE("failed to get input method extension native object!");
        return;
    }
    auto workContext = new (std::nothrow) std::weak_ptr<SelectionExtensionContext>(context);
    if (workContext == nullptr) {
        SELECTION_HILOGE("workContext is nullptr!");
        return;
    }
    napi_status status = napi_coerce_to_native_binding_object(env, contextObj, DetachCallbackFunc,
        AttachSelectionExtensionContext, workContext, nullptr);
    if (status != napi_ok) {
        SELECTION_HILOGE("napi_coerce_to_native_binding_object failed!");
        return;
    }
    SELECTION_HILOGD("JsSelectionExtension::Init Bind.");
    context->Bind(jsRuntime_, shellContextRef.release());
    SELECTION_HILOGD("JsSelectionExtension::SetProperty.");
    napi_set_named_property(env, obj, "context", contextObj);
    status = napi_wrap(
        env, contextObj, workContext,
        [](napi_env, void* data, void*) {
            SELECTION_HILOGI("Finalizer for weak_ptr input method extension context is called.");
            delete static_cast<std::weak_ptr<SelectionExtensionContext>*>(data);
        },
        nullptr, nullptr);
    if (status != napi_ok) {
        SELECTION_HILOGE("SelectionExtensionContext wrap failed: %{public}d", status);
        delete workContext;
    }
    SELECTION_HILOGI("%{public}s end.", __func__);
}

} // namespace OHOS::AbilityRuntime