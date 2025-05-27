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

#ifndef JS_SELECTION_EXTENSION_H
#define JS_SELECTION_EXTENSION_H
#include "selection_extension.h"

namespace OHOS::AbilityRuntime {
class JsRuntime;

class JsSelectionExtension : public SelectionExtension {
public:
    explicit JsSelectionExtension(JsRuntime& jsRuntime);
    virtual ~JsSelectionExtension() override;
    /**
     * @brief Create JsSelectionExtension.
     *
     * @param runtime The runtime.
     * @return The JsSelectionExtension instance.
     */
    static JsSelectionExtension* Create(const std::unique_ptr<Runtime>& runtime);
    /**
     * @brief Init the extension.
     *
     * @param record the extension record.
     * @param application the application info.
     * @param handler the extension handler.
     * @param token the remote token.
     */
    virtual void Init(const std::shared_ptr<AbilityLocalRecord>& record,
                      const std::shared_ptr<OHOSApplication>& application,
                      std::shared_ptr<AbilityHandler>& handler,
                      const sptr<IRemoteObject>& token) override;

    /**
     * @brief Called when this extension is started. You must override this function if you want to perform some
     *        initialization operations during extension startup.
     *
     * This function can be called only once in the entire lifecycle of an extension.
     * @param Want Indicates the {@link Want} structure containing startup information about the extension.
     */
    virtual void OnStart(const AAFwk::Want& want) override;

    /**
     * @brief Called when this Service extension is connected for the first time.
     *
     * You can override this function to implement your own processing logic.
     *
     * @param want Indicates the {@link Want} structure containing connection information about the Service extension.
     * @return Returns a pointer to the <b>sid</b> of the connected Service extension.
     */
    virtual sptr<IRemoteObject> OnConnect(const AAFwk::Want& want) override;

    /**
     * @brief Called when all abilities connected to this Service extension are disconnected.
     *
     * You can override this function to implement your own processing logic.
     *
     */
    virtual void OnDisconnect(const AAFwk::Want& want) override;

    /**
     * @brief Called when this extension enters the <b>STATE_STOP</b> state.
     *
     * The extension in the <b>STATE_STOP</b> is being destroyed.
     * You can override this function to implement your own processing logic.
     */
    virtual void OnStop() override;

private:
    napi_value CallObjectMethod(const char* methodName, const napi_value* argv = nullptr, size_t argc = 0);
    void GetSrcPath(std::string& srcPath);
    void BindContext(napi_env env, napi_value obj);

private:
    JsRuntime& jsRuntime_;
    std::unique_ptr<NativeReference> jsObj_;
};
} // namespace OHOS::AbilityRuntime

#endif // JS_SELECTION_EXTENSION_H