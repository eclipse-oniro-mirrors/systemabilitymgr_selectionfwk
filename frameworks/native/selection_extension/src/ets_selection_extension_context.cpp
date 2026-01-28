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

#include "ets_selection_extension_context.h"
#include "ets_error_utils.h"
#include "ets_context_utils.h"
#include "ets_extension_context.h"
#include "ets_runtime.h"
#include "ani_common_util.h"
#include "ani_common_want.h"
#include "selection_log.h"

namespace OHOS::AbilityRuntime {

namespace {

constexpr const char *CONTEXT_CLASS_NAME = "@ohos.selectionInput.SelectionExtensionContext.SelectionExtensionContext";
constexpr const char *SELECTION_EXTENSION_CONTEXT_CLASS_NAME =
"@ohos.selectionInput.SelectionExtensionContext.SelectionExtensionContext";
constexpr const char *CLEANER_CLASS_NAME = "@ohos.selectionInput.SelectionExtensionContext.Cleaner";
constexpr const char *CLASSNAME_ASYNC_CALLBACK_WRAPPER = "@ohos.selectionInput.SelectionExtensionContext.AsyncCallbackWrapper";

bool AsyncCallback(ani_env *env, ani_object call, ani_object error, ani_object result)
{
    if (env == nullptr) {
        SELECTION_HILOGE("null env");
        return false;
    }
    ani_class clsCall = nullptr;
    ani_status status = env->FindClass(CLASSNAME_ASYNC_CALLBACK_WRAPPER, &clsCall);
    if (status!= ANI_OK || clsCall == nullptr) {
        SELECTION_HILOGE("FindClass status: %{public}d, or null clsCall", status);
        return false;
    }
    ani_method method = nullptr;
    if ((status = env->Class_FindMethod(clsCall, "invoke", nullptr, &method)) != ANI_OK || method == nullptr) {
        SELECTION_HILOGE("Class_FindMethod status: %{public}d, or null method", status);
        return false;
    }
    if (error == nullptr) {
        ani_ref nullRef = nullptr;
        env->GetNull(&nullRef);
        error = reinterpret_cast<ani_object>(nullRef);
    }
    if (result == nullptr) {
        ani_ref undefinedRef = nullptr;
        env->GetUndefined(&undefinedRef);
        result = reinterpret_cast<ani_object>(undefinedRef);
    }
    if ((status = env->Object_CallMethod_Void(call, method, error, result)) != ANI_OK) {
        SELECTION_HILOGE("Object_CallMethod_Void status: %{public}d", status);
        return false;
    }
    return true;
}

class EtsSelectionExtensionContext final {
public:
    explicit EtsSelectionExtensionContext(const std::shared_ptr<SelectionExtensionContext>& context) : context_(context)
    {
        SELECTION_HILOGI("EtsSelectionExtensionContext::EtsSelectionExtensionContext is called.");
    }
    EtsSelectionExtensionContext() = default;
    ~EtsSelectionExtensionContext() = default;

    static void Finalizer(ani_env *env, void* data, void* hint)
    {
        SELECTION_HILOGI("EtsSelectionExtensionContext::Finalizer is called.");
        std::unique_ptr<EtsSelectionExtensionContext>(static_cast<EtsSelectionExtensionContext*>(data));
    }

    static EtsSelectionExtensionContext *GetEtsAbilityContext(ani_env *env, ani_object aniObj)
    {
        SELECTION_HILOGD("EtsSelectionExtensionContext::GetEtsAbilityContext is called.");
        ani_class cls = nullptr;
        ani_long nativeContextLong;
        ani_field contextField = nullptr;
        ani_status status = ANI_ERROR;
        if (env == nullptr) {
            SELECTION_HILOGD("EtsSelectionExtensionContext::GetEtsAbilityContext, env is null");
            return nullptr;
        }
        if ((status = env->FindClass(CONTEXT_CLASS_NAME, &cls)) != ANI_OK) {
            SELECTION_HILOGE("Failed to find class, status : %{public}d", status);
            return nullptr;
        }
        if ((status = env->Class_FindField(cls, "nativeEtsContext", &contextField)) != ANI_OK) {
            SELECTION_HILOGE("Failed to find filed, status : %{public}d", status);
            return nullptr;
        }
        if ((status = env->Object_GetField_Long(aniObj, contextField, &nativeContextLong)) != ANI_OK) {
            SELECTION_HILOGE("Failed to get filed, status : %{public}d", status);
            return nullptr;
        }
        auto weakContext = reinterpret_cast<EtsSelectionExtensionContext *>(nativeContextLong);
        return weakContext;
    }

    static void StartAbility(ani_env *env, ani_object aniObj, ani_object wantObj, ani_object call)
    {
        SELECTION_HILOGD("EtsSelectionExtensionContext::StartAbility is called.");
        if (env == nullptr) {
            SELECTION_HILOGE("EtsSelectionExtensionContext::StartAbility env is null");
            return;
        }
        auto etsSelectionExtensionContext = EtsSelectionExtensionContext::GetEtsAbilityContext(env, aniObj);
        if (etsSelectionExtensionContext == nullptr) {
            SELECTION_HILOGE("null etsSelectionExtensionContext");
            return;
        }
        etsSelectionExtensionContext->OnStartAbility(env, aniObj, wantObj, call);
    }
    std::weak_ptr<SelectionExtensionContext> GetAbilityContext()
    {
        return context_;
    }

private:
    std::weak_ptr<SelectionExtensionContext> context_;

    void OnStartAbility(ani_env *env, ani_object aniObj, ani_object wantObj, ani_object callbackObj)
    {
        SELECTION_HILOGD("OnStartAbility");
        ani_object aniObject = nullptr;
        AAFwk::Want want;
        ErrCode errCode = ERR_OK;
        if (!AppExecFwk::UnwrapWant(env, wantObj, want)) {
            SELECTION_HILOGE("UnwrapWant failed");
            aniObject = EtsErrorUtil::CreateInvalidParamError(env, "UnwrapWant failed");
            AsyncCallback(env, callbackObj, aniObject, nullptr);
            return;
        }
        auto context = context_.lock();
        if (context == nullptr) {
            SELECTION_HILOGE("context is nullptr");
            errCode = static_cast<int32_t>(AbilityErrorCode::ERROR_CODE_INVALID_CONTEXT);
            aniObject = EtsErrorUtil::CreateError(env, static_cast<AbilityErrorCode>(errCode));
            AsyncCallback(env, callbackObj, aniObject, nullptr);
            return;
        }

        errCode = context->StartAbility(want);

        aniObject = EtsErrorUtil::CreateErrorByNativeErr(env, errCode);
        AsyncCallback(env, callbackObj, aniObject, nullptr);
    }
};

bool BindNativeMethods(ani_env *env, ani_class &cls)
{
    if (env == nullptr) {
        SELECTION_HILOGE("null env");
        return false;
    }
    ani_status status = ANI_ERROR;
    std::array functions = {
        ani_native_function { "nativeStartAbility",
            "C{@ohos.app.ability.Want.Want}C{@ohos.selectionInput.SelectionExtensionContext.AsyncCallbackWrapper}:",
            reinterpret_cast<void *>(EtsSelectionExtensionContext::StartAbility) },
    };
    if ((status = env->Class_BindNativeMethods(cls, functions.data(), functions.size())) != ANI_OK
        && status != ANI_ALREADY_BINDED) {
        SELECTION_HILOGE("bind method failed, status : %{public}d", status);
        return false;
    }
    ani_class cleanerCls = nullptr;
    status = env->FindClass(CLEANER_CLASS_NAME, &cleanerCls);
    if (status != ANI_OK || cleanerCls == nullptr) {
        SELECTION_HILOGE("Failed to find class, status : %{public}d", status);
        return false;
    }
    std::array CleanerMethods = {
        ani_native_function { "clean", nullptr, reinterpret_cast<void *>(EtsSelectionExtensionContext::Finalizer) },
    };
    if ((status = env->Class_BindNativeMethods(cleanerCls, CleanerMethods.data(), CleanerMethods.size())) != ANI_OK
        && status != ANI_ALREADY_BINDED) {
        SELECTION_HILOGE("bind method status : %{public}d", status);
        return false;
    }
    return true;
}


} // namespace

ani_object CreateEtsSelectionExtensionContext(ani_env *env, std::shared_ptr<SelectionExtensionContext> context)
{
    SELECTION_HILOGI("CreateEtsSelectionExtensionContext begin");
    if (env == nullptr || context == nullptr) {
        SELECTION_HILOGE("CreateEtsSelectionExtensionContext: null env or context");
        return nullptr;
    }
    ani_class cls = nullptr;
    ani_status status = ANI_ERROR;
    ani_method method = nullptr;
    ani_object contextObj = nullptr;
    if ((status = env->FindClass(SELECTION_EXTENSION_CONTEXT_CLASS_NAME, &cls)) != ANI_OK || cls == nullptr) {
        SELECTION_HILOGE("Failed to find class, status : %{public}d", status);
        return nullptr;
    }
    if (!BindNativeMethods(env, cls)) {
        SELECTION_HILOGE("Failed to BindNativeMethods");
        return nullptr;
    }
    if ((status = env->Class_FindMethod(cls, "<ctor>", "l:", &method)) != ANI_OK || method == nullptr) {
        SELECTION_HILOGE("Failed to find constructor, status : %{public}d", status);
        return nullptr;
    }
    std::unique_ptr<EtsSelectionExtensionContext> workContext =
        std::make_unique<EtsSelectionExtensionContext>(context);
    if (workContext == nullptr) {
        SELECTION_HILOGE("Failed to create etsSelectionExtensionContext");
        return nullptr;
    }
    auto serviceContextPtr = new (std::nothrow)
        std::weak_ptr<SelectionExtensionContext> (workContext->GetAbilityContext());
    if ((status = env->Object_New(cls, method, &contextObj, (ani_long)workContext.release())) != ANI_OK ||
        contextObj == nullptr) {
        SELECTION_HILOGE("Failed to create object, status : %{public}d", status);
        return nullptr;
    }
    if (!ContextUtil::SetNativeContextLong(env, contextObj, (ani_long)(serviceContextPtr))) {
        SELECTION_HILOGE("Failed to setNativeContextLong");
        return nullptr;
    }
    ContextUtil::CreateEtsBaseContext(env, cls, contextObj, context);
    CreateEtsExtensionContext(env, cls, contextObj, context, context->GetAbilityInfo());
    ani_ref *contextGlobalRef = new (std::nothrow) ani_ref;
    if (contextGlobalRef == nullptr) {
        SELECTION_HILOGE("new contextGlobalRef failed");
        return nullptr;
    }
    if ((status = env->GlobalReference_Create(contextObj, contextGlobalRef)) != ANI_OK) {
        SELECTION_HILOGE("GlobalReference_Create failed status: %{public}d", status);
        delete contextGlobalRef;
        return nullptr;
    }
    context->Bind(contextGlobalRef);
    return contextObj;
}
} // namespace OHOS::AbilityRuntime
