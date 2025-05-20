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
#include "napi/native_api.h"
#include "napi_common_want.h"

namespace OHOS::AbilityRuntime {
namespace {
constexpr size_t ARGC_ONE = 1;
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
    HILOG_INFO("JsSelectionExtension Init begin.");
    SelectionExtension::Init(record, application, handler, token);
    std::string srcPath;
    GetSrcPath(srcPath);
    if (srcPath.empty()) {
        HILOG_ERROR("failed to get srcPath!");
        return;
    }

    std::string moduleName(Extension::abilityInfo_->moduleName);
    moduleName.append("::").append(abilityInfo_->name);
    HILOG_INFO("JsSelectionExtension, module: %{public}s, srcPath:%{public}s.", moduleName.c_str(), srcPath.c_str());
    HandleScope handleScope(jsRuntime_);
    jsObj_ = jsRuntime_.LoadModule(moduleName, srcPath, abilityInfo_->hapPath,
                                   abilityInfo_->compileMode == CompileMode::ES_MODULE);
    if (jsObj_ == nullptr) {
        HILOG_ERROR("failed to init jsObj_!");
        return;
    }
    BindContext();
    HILOG_INFO("JsSelectionExtension Init end.");
}

void JsSelectionExtension::OnStart(const AAFwk::Want& want)
{
    HILOG_INFO("JsSelectionExtension OnStart begin.");
    Extension::OnStart(want);
    napi_env env = jsRuntime_.GetNapiEnv();
    napi_value napiWant = OHOS::AppExecFwk::WrapWant(env, want);
    napi_value argv[] = {napiWant};
    CallObjectMethod("onCreate", argv, ARGC_ONE);
    HILOG_INFO("%{public}s end.", __func__);
}

sptr<IRemoteObject> JsSelectionExtension::OnConnect(const AAFwk::Want& want)
{
    HILOG_INFO("JsSelectionExtension OnConnect begin.");
    Extension::OnConnect(want);
    return nullptr;
}

void JsSelectionExtension::OnDisconnect(const AAFwk::Want& want) {}

void JsSelectionExtension::OnStop() {}

napi_value JsSelectionExtension::CallObjectMethod(const char* methodName, const napi_value* argv, size_t argc)
{
    HILOG_INFO("JsSelectionExtension::CallObjectMethod(%{public}s), start.", methodName);

    if (jsObj_ == nullptr) {
        HILOG_ERROR("not found JsSelectionExtension.js.");
        return nullptr;
    }

    HandleScope handleScope(jsRuntime_);
    napi_env env = jsRuntime_.GetNapiEnv();
    napi_value obj = jsObj_->GetNapiValue();
    if (obj == nullptr) {
        HILOG_ERROR("failed to get JsSelectionExtension object!");
        return nullptr;
    }

    napi_value method = nullptr;
    napi_get_named_property(env, obj, methodName, &method);
    if (method == nullptr) {
        HILOG_ERROR("failed to get '%{public}s' from JsSelectionExtension object!", methodName);
        return nullptr;
    }
    HILOG_INFO("JsSelectionExtension::CallFunction(%{public}s), success.", methodName);
    napi_value result = nullptr;
    if (napi_call_function(env, obj, method, argc, argv, &result) != napi_ok) {
        return nullptr;
    }
    return result;
}

void JsSelectionExtension::GetSrcPath(std::string& srcPath)
{
    if (!Extension::abilityInfo_->isModuleJson) {
        /* temporary compatibility api8 + config.json */
        srcPath.append(Extension::abilityInfo_->package);
        srcPath.append("/assets/js/");
        if (!Extension::abilityInfo_->srcPath.empty()) {
            srcPath.append(Extension::abilityInfo_->srcPath);
        }
        srcPath.append("/").append(Extension::abilityInfo_->name).append(".abc");
        return;
    }

    if (!Extension::abilityInfo_->srcEntrance.empty()) {
        srcPath.append(Extension::abilityInfo_->moduleName + "/");
        srcPath.append(Extension::abilityInfo_->srcEntrance);
        srcPath.erase(srcPath.rfind('.'));
        srcPath.append(".abc");
    }
}

void JsSelectionExtension::BindContext()
{
    // TODO:
}

} // namespace OHOS::AbilityRuntime