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

#ifndef ETS_SELECTION_EXTENSION_H
#define ETS_SELECTION_EXTENSION_H
#include "ani.h"
#include "selection_extension.h"
#include "ets_native_reference.h"

namespace OHOS::AbilityRuntime {
class ETSRuntime;

class EtsSelectionExtension : public SelectionExtension {
public:
    explicit EtsSelectionExtension(ETSRuntime& etsRuntime);
    virtual ~EtsSelectionExtension() override;
    /**
     * @brief Create EtsSelectionExtension.
     *
     * @param runtime The runtime.
     * @return The EtsSelectionExtension instance.
     */
    static EtsSelectionExtension* Create(const std::unique_ptr<Runtime>& runtime);
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

private:
    ani_ref CallObjectMethod(bool withResult, const char *name, const char *signature, ...);
    void GetSrcPath(std::string& srcPath);
    void BindContext(ani_env *env);
    ani_object CreateETSContext(ani_env *env, std::shared_ptr<SelectionExtensionContext> context);

private:
    ETSRuntime& etsRuntime_;
    std::unique_ptr<ETSNativeReference> etsObj_;
    std::shared_ptr<ETSNativeReference> shellContextRef_ = nullptr;
};
} // namespace OHOS::AbilityRuntime

#endif // JS_SELECTION_EXTENSION_H