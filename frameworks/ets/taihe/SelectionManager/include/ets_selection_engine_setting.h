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

#ifndef ETS_SELECTION_ENGINE_SETTING_H
#define ETS_SELECTION_ENGINE_SETTING_H

#include "ani_common.h"
#include "selection_interface.h"
#include "iremote_stub.h"
#include "iselection_listener.h"
namespace OHOS {
namespace SelectionFwk {
class EtsSelectionEngineSetting : public SelectionInterface {
public:
    static void Subscribe(const std::string &type, callbackTypePara &&cb);
    static void UnSubscribe(const std::string &type, const callbackTypePara &cb);
    static void UnSubscribe(const std::string &type);
    int32_t OnSelectionEvent(const SelectionInfo &selectionInfo);

private:
    EtsSelectionEngineSetting() = default;
    static SFErrorCode Register();
    static std::shared_ptr<EtsSelectionEngineSetting> GetEtsSelectionEngineSetting();
    void RegisterListener(const std::string &type, callbackTypePara &&cb);
    void UnRegisterListener(const std::string &type, const callbackTypePara &cb);
    void UnRegisterListener(const std::string &type);
    static SFErrorCode RegisterListenerToService(std::shared_ptr<EtsSelectionEngineSetting> &selectionEnging);

private:
    std::map<std::string, std::vector<callbackTypePara>> EtsCbMap_;
    static std::mutex selectionMutex_;
    static std::shared_ptr<EtsSelectionEngineSetting> selectionDelegate_;
    std::recursive_mutex mutex_;
    static sptr<ISelectionListener> listenerStub_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif //ETS_SELECTION_ENGINE_SETTING_H