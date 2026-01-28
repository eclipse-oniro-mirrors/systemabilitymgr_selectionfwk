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


#ifndef SELECTION_PANEL_LISTENER_IMPL_H
#define SELECTION_PANEL_LISTENER_IMPL_H

#include <mutex>
#include "concurrent_map.h"
#include "selection_panel.h"
#include "panel_status_listener.h"
#include "ani_common.h"

namespace OHOS {
namespace SelectionFwk {
using CallbackVector = std::vector<SelectionFwk::callbackType>;
using TypeMap = std::map<std::string, CallbackVector>;

class PanelListenerImpl : public PanelStatusListener {
public:
    static std::shared_ptr<PanelListenerImpl> GetInstance();
    ~PanelListenerImpl();

    void OnPanelStatus(uint32_t windowId, const std::string& status) override;
    void Subscribe(uint32_t windowId, const std::string &type, callbackType &&cbObject);//往map里面添加cb
    void RemoveInfo(const std::string &type, uint32_t windowId, const callbackType &cbObject);//从map里面删除cb
    void RemoveInfo(const std::string &type, uint32_t windowId);
    CallbackVector GetCallback(uint32_t windowId, const std::string &type);

    static std::mutex listenerMutex_;
    static std::shared_ptr<PanelListenerImpl> instance_;
    ConcurrentMap<uint32_t, TypeMap> callbacks_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif //SELECTION_PANEL_LISTENER_IMPL_H