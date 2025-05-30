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

#ifndef JS_SELECTION_ENGINE_SETTING_H
#define JS_SELECTION_ENGINE_SETTING_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "callback_object.h"
#include "iremote_stub.h"
#include "iselection_listener.h"
#include "iselection_service.h"
#include "napi/native_api.h"
#include "refbase.h"
#include "selection_interface.h"
#include "util.h"
#include "async_call.h"
#include "selection_panel.h"
#include "panel_info.h"
#include "context.h"

namespace OHOS {
namespace SelectionFwk {
class JsSelectionEngineSetting : public SelectionInterface{
public:
    JsSelectionEngineSetting() = default;
    ~JsSelectionEngineSetting() = default;
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value InitProperty(napi_env env, napi_value exports);
    static napi_value GetSelectionAbility(napi_env env, napi_callback_info info);
    static napi_value Subscribe(napi_env env, napi_callback_info info);
    static napi_value UnSubscribe(napi_env env, napi_callback_info info);
    static napi_value CreatePanel(napi_env env, napi_callback_info info);
    static napi_value DestroyPanel(napi_env env, napi_callback_info info);
    int32_t OnSelectionEvent(const SelectionData &selectionData);

private:
    struct SelectionEntry {
        std::vector<std::shared_ptr<JSCallbackObject>> vecCopy;
        std::string type;
        SelectionData selectionData;
        SelectionEntry(const std::vector<std::shared_ptr<JSCallbackObject>> &cbVec, const std::string &type)
            : vecCopy(cbVec), type(type)
        {
        }
    };

    struct PanelContext : public AsyncCall::Context {
        PanelInfo panelInfo = PanelInfo();
        std::shared_ptr<SelectionPanel> panel = nullptr;
        std::shared_ptr<OHOS::AbilityRuntime::Context> context = nullptr;
        PanelContext() : Context(nullptr, nullptr){};
        PanelContext(InputAction input, OutputAction output) : Context(std::move(input), std::move(output)){};

        napi_status operator()(napi_env env, size_t argc, napi_value *argv, napi_value self) override
        {
            CHECK_RETURN(self != nullptr, "self is nullptr", napi_invalid_arg);
            return Context::operator()(env, argc, argv, self);
        }
        napi_status operator()(napi_env env, napi_value *result) override
        {
            if (status_ != napi_ok) {
                output_ = nullptr;
                return status_;
            }
            return Context::operator()(env, result);
        }
    };

    static napi_value GetSEInstance(napi_env env, napi_callback_info info);
    static napi_value JsConstructor(napi_env env, napi_callback_info cbinfo);
    static napi_value Write(napi_env env, const SelectionData &selectionData);
    static napi_status GetContext(napi_env env, napi_value in, std::shared_ptr<OHOS::AbilityRuntime::Context> &context);
    static std::shared_ptr<JsSelectionEngineSetting> GetJsSelectionEngineSetting();
    void RegisterListener(napi_value callback, std::string type, std::shared_ptr<JSCallbackObject> callbackObj);
    void UnRegisterListener(napi_value callback, std::string type);
    sptr<ISelectionService> GetSelectionSystemAbility();
    static std::shared_ptr<AppExecFwk::EventHandler> GetEventHandler();
    using EntrySetter = std::function<void(SelectionEntry &)>;
    std::shared_ptr<SelectionEntry> GetEntry(const std::string &type, EntrySetter entrySetter = nullptr);

private:
    static const std::string KDS_CLASS_NAME;
    static thread_local napi_ref KDSRef_;
    std::map<std::string, std::vector<std::shared_ptr<JSCallbackObject>>> jsCbMap_;
    static std::mutex selectionMutex_;
    static std::shared_ptr<JsSelectionEngineSetting> selectionDelegate_;
    std::recursive_mutex mutex_;
    sptr<ISelectionListener> listenerStub_ { nullptr };
    sptr<ISelectionService> abilityManager_ { nullptr };
    static std::mutex eventHandlerMutex_;
    static std::shared_ptr<AppExecFwk::EventHandler> handler_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif //JS_SELECTION_ENGINE_SETTING_H
