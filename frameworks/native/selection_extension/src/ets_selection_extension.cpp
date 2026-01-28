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

#include "ets_selection_extension.h"
#include "ets_runtime.h"
#include "selection_log.h"
#include "ability_business_error.h"
#include "ability_info.h"
#include "ani_common_want.h"
#include "remote_object_taihe_ani.h"
#include "ets_selection_extension_context.h"
#include "ets_extension_context.h"
#include "ets_runtime.h"

namespace OHOS::AbilityRuntime {
using namespace OHOS::AppExecFwk;

EtsSelectionExtension::EtsSelectionExtension(ETSRuntime& etsRuntime) : etsRuntime_(etsRuntime) {}
EtsSelectionExtension::~EtsSelectionExtension()
{
    SELECTION_HILOGD("EtsSelectionExtension destory");
    auto context = GetContext();
    if (context) {
        context->Unbind();
    }
}

EtsSelectionExtension* EtsSelectionExtension::Create(const std::unique_ptr<Runtime>& runtime)
{
    return new EtsSelectionExtension(static_cast<ETSRuntime&>(*runtime));
}

void EtsSelectionExtension::Init(const std::shared_ptr<AbilityLocalRecord> &record,
    const std::shared_ptr<OHOSApplication> &application, std::shared_ptr<AbilityHandler> &handler,
    const sptr<IRemoteObject> &token)
{
    SELECTION_HILOGI("EtsSelectionExtension::Init");
    SelectionExtension::Init(record, application, handler, token);
    std::string srcPath = "";
    GetSrcPath(srcPath);
    if (srcPath.empty()) {
        SELECTION_HILOGE("get srcPath failed");
        return;
    }

    std::string moduleName(Extension::abilityInfo_->moduleName);
    moduleName.append("::").append(abilityInfo_->name);
    etsObj_ = etsRuntime_.LoadModule(
        moduleName, srcPath, abilityInfo_->hapPath, abilityInfo_->compileMode == CompileMode::ES_MODULE,
        false, abilityInfo_->srcEntrance);
    if (etsObj_ == nullptr) {
        SELECTION_HILOGE("null etsObj_");
        return;
    }

    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        SELECTION_HILOGE("null env");
        return;
    }
    BindContext(env);
}

sptr<IRemoteObject> EtsSelectionExtension::OnConnect(const AAFwk::Want &want)
{
    SELECTION_HILOGI("EtsSelectionExtension OnConnect start.");
    Extension::OnConnect(want);
    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        SELECTION_HILOGE("null env");
        return nullptr;
    }
    ani_ref wantRef = OHOS::AppExecFwk::WrapWant(env, want);
    if (wantRef == nullptr) {
        SELECTION_HILOGE("null wantRef");
        return nullptr;
    }
    ani_ref result = CallObjectMethod(true, "onConnect", nullptr, wantRef);
    auto obj = reinterpret_cast<ani_object>(result);
    auto remoteObj = AniGetNativeRemoteObject(env, obj);
    if (remoteObj == nullptr) {
        SELECTION_HILOGE("remoteObj null");
        return nullptr;
    }
    SELECTION_HILOGD("end");
    return remoteObj;
}

void EtsSelectionExtension::OnDisconnect(const AAFwk::Want &want)
{
    SELECTION_HILOGD("OnDisconnect");
    Extension::OnDisconnect(want);
    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        SELECTION_HILOGE("null env");
        return;
    }
    ani_ref wantRef = OHOS::AppExecFwk::WrapWant(env, want);
    if (wantRef == nullptr) {
        SELECTION_HILOGE("null wantRef");
        return;
    }
    CallObjectMethod(false, "onDisconnect", nullptr, wantRef);
    SELECTION_HILOGD("null wantRef");
}

ani_ref EtsSelectionExtension::CallObjectMethod(bool withResult, const char *name, const char *signature, ...)
{
    SELECTION_HILOGI("EtsSelectionExtension::CallObjectMethod(%{public}s), start.", name);
    ani_status status = ANI_ERROR;
    ani_method method = nullptr;
    auto env = etsRuntime_.GetAniEnv();
    if (env == nullptr) {
        SELECTION_HILOGI("null env");
        return nullptr;
    }
    if ((status = env->Class_FindMethod(etsObj_->aniCls, name, signature, &method)) != ANI_OK) {
        SELECTION_HILOGE("Class_FindMethod status : %{public}d", status);
        return nullptr;
    }
    if (method == nullptr) {
        return nullptr;
    }
    ani_ref res = nullptr;
    va_list args;
    ani_boolean errorExists;
    env->ExistUnhandledError(&errorExists);
    SELECTION_HILOGI("errorExists");
    if (errorExists) {
        SELECTION_HILOGI("error already happen");
    }
    if (withResult) {
        va_start(args, signature);
        if ((status = env->Object_CallMethod_Ref_V(etsObj_->aniObj, method, &res, args)) != ANI_OK) {
            SELECTION_HILOGE("Object_CallMethod_Ref_V status : %{public}d", status);
            return nullptr;
        }
        va_end(args);
        return res;
    }
    va_start(args, signature);
    if ((status = env->Object_CallMethod_Void_V(etsObj_->aniObj, method, args)) != ANI_OK) {
        SELECTION_HILOGE("Object_CallMethod_Void_V status : %{public}d", status);
    }
    va_end(args);
    return nullptr;
}

void EtsSelectionExtension::GetSrcPath(std::string &srcPath)
{
    TAG_LOGD(AAFwkTag::APP_SERVICE_EXT, "called");
    SELECTION_HILOGD("EtsSelectionExtension::GetSrcPath called");
    if (!Extension::abilityInfo_->srcEntrance.empty()) {
        srcPath.append(Extension::abilityInfo_->moduleName + "/");
        srcPath.append(Extension::abilityInfo_->srcEntrance);
        auto pos = srcPath.rfind(".");
        if (pos != std::string::npos) {
            srcPath.erase(pos);
            srcPath.append(".abc");
        }
    }
}

void EtsSelectionExtension::BindContext(ani_env *env)
{
    SELECTION_HILOGI("EtsSelectionExtension::BindContext called");
    if (env == nullptr) {
        SELECTION_HILOGE("Want info is null or env is null");
        return;
    }
    auto context = GetContext();
    if (context == nullptr) {
        SELECTION_HILOGE("Failed to get context");
        return;
    }
    ani_object contextObj = CreateETSContext(env, context);
    if (contextObj == nullptr) {
        SELECTION_HILOGE("null contextObj");
        return;
    }
    ani_field contextField;
    auto status = env->Class_FindField(etsObj_->aniCls, "context", &contextField);
    if (status != ANI_OK) {
        SELECTION_HILOGE("Class_GetField context failed");
        return;
    }
    ani_ref contextRef = nullptr;
    if (env->GlobalReference_Create(contextObj, &contextRef) != ANI_OK) {
        SELECTION_HILOGE("GlobalReference_Create contextObj failed");
        return;
    }
    if (env->Object_SetField_Ref(etsObj_->aniObj, contextField, contextRef) != ANI_OK) {
        SELECTION_HILOGE("Object_SetField_Ref contextObj failed");
    }
    SELECTION_HILOGI("BindContext end");
}

ani_object EtsSelectionExtension::CreateETSContext(ani_env *env, std::shared_ptr<SelectionExtensionContext> context)
{
    SELECTION_HILOGI("EtsSelectionExtension::CreateETSContext called");
    return CreateEtsSelectionExtensionContext(env, context);
}

} // namespace OHOS::AbilityRuntime