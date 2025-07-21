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

#include "js_selection_extension_context.h"
#include "js_error_utils.h"
#include "js_extension_context.h"
#include "js_runtime_utils.h"
#include "selection_js_utils.h"
#include "napi_common_want.h"
#include "selection_log.h"

namespace OHOS::AbilityRuntime {
using namespace OHOS::SelectionFwk;

namespace {
constexpr int32_t INDEX_ZERO = 0;
constexpr int32_t ERROR_CODE_ONE = 1;
constexpr size_t ARGC_ONE = 1;

class JsSelectionExtensionContext final {
public:
    explicit JsSelectionExtensionContext(const std::shared_ptr<SelectionExtensionContext>& context) : context_(context)
    {
        SELECTION_HILOGI("JsSelectionExtensionContext::JsSelectionExtensionContext is called.");
    }
    JsSelectionExtensionContext() = default;
    ~JsSelectionExtensionContext() = default;

    static void Finalizer(napi_env env, void* data, void* hint)
    {
        SELECTION_HILOGI("JsSelectionExtensionContext::Finalizer is called.");
        std::unique_ptr<JsSelectionExtensionContext>(static_cast<JsSelectionExtensionContext*>(data));
    }

    static napi_value StartAbility(napi_env env, napi_callback_info info)
    {
        GET_CB_INFO_AND_CALL(env, info, JsSelectionExtensionContext, OnStartAbility);
    }

private:
    std::weak_ptr<SelectionExtensionContext> context_;

    napi_value OnStartAbility(napi_env env, size_t argc, napi_value* argv)
    {
        SELECTION_HILOGI("SelectionExtensionContext OnStartAbility.");
        // only support one params
        PARAM_CHECK_RETURN(env, argc == ARGC_ONE, "number of param should in 1", TYPE_NONE, CreateJsUndefined(env));
        PARAM_CHECK_RETURN(env, JsUtil::GetType(env, argv[0]) == napi_object, "param want type must be Want", TYPE_NONE,
                           JsUtil::Const::Null(env));
        decltype(argc) unwrapArgc = 0;
        AAFwk::Want want;
        OHOS::AppExecFwk::UnwrapWant(env, argv[INDEX_ZERO], want);
        SELECTION_HILOGI("%{public}s bundleName: %{public}s abilityName: %{public}s.", __func__,
                         want.GetBundle().c_str(), want.GetElement().GetAbilityName().c_str());
        unwrapArgc++;
        napi_value lastParam = argc > unwrapArgc ? argv[unwrapArgc] : nullptr;
        napi_value result = nullptr;
        std::unique_ptr<NapiAsyncTask> napiAsyncTask = CreateEmptyAsyncTask(env, lastParam, &result);
        auto asyncTask = [weak = context_, want, unwrapArgc, env, task = napiAsyncTask.get()]() {
            SELECTION_HILOGI("startAbility start.");
            auto context = weak.lock();
            if (context == nullptr) {
                SELECTION_HILOGW("context is released.");
                task->Reject(env, CreateJsError(env, ERROR_CODE_ONE, "Context is released"));
                delete task;
                return;
            }
            ErrCode errcode = (unwrapArgc == 1) ? context->StartAbility(want) : ERR_INVALID_VALUE;
            if (errcode == 0) {
                task->Resolve(env, CreateJsUndefined(env));
            } else {
                task->Reject(env, CreateJsErrorByNativeErr(env, errcode));
            }
            delete task;
        };
        if (napi_send_event(env, asyncTask, napi_eprio_high) != napi_status::napi_ok) {
            napiAsyncTask->Reject(env, CreateJsError(env, ERROR_CODE_ONE, "send event failed"));
        } else {
            napiAsyncTask.release();
        }
        return result;
    }
};
} // namespace

napi_value CreateJsSelectionExtensionContext(napi_env env, std::shared_ptr<SelectionExtensionContext> context)
{
    SELECTION_HILOGI("CreateJsSelectionExtensionContext begin");
    std::shared_ptr<OHOS::AppExecFwk::AbilityInfo> abilityInfo = nullptr;
    if (context) {
        abilityInfo = context->GetAbilityInfo();
    }

    napi_value objValue = CreateJsExtensionContext(env, context, abilityInfo);
    std::unique_ptr<JsSelectionExtensionContext> jsContext = std::make_unique<JsSelectionExtensionContext>(context);
    napi_wrap(env, objValue, jsContext.release(), JsSelectionExtensionContext::Finalizer, nullptr, nullptr);

    const char* moduleName = "JsSelectionExtensionContext";
    BindNativeFunction(env, objValue, "startAbility", moduleName, JsSelectionExtensionContext::StartAbility);
    return objValue;
}
} // namespace OHOS::AbilityRuntime
