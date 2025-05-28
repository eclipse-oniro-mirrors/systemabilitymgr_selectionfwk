/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef SELECTION_IMF_PANEL_LISTENER_IMPL_H
#define SELECTION_IMF_PANEL_LISTENER_IMPL_H

#include <mutex>
#include <shared_mutex>
#include <thread>
#include <tuple>
#include <uv.h>

#include "concurrent_map.h"
#include "event_handler.h"
#include "selection_panel.h"
#include "callback_object.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "panel_status_listener.h"


namespace OHOS {
namespace SelectionFwk {

struct JsWindowSize {
    static napi_value Write(napi_env env, const WindowSize &nativeObject);
    static bool Read(napi_env env, napi_value jsObject, WindowSize &nativeObject);
};

struct JsKeyboardArea {
    static napi_value Write(napi_env env, const PanelAdjustInfo &nativeObject);
    static bool Read(napi_env env, napi_value jsObject, PanelAdjustInfo &nativeObject);
};

class PanelListenerImpl : public PanelStatusListener {
public:

struct UvEntry {
        WindowSize size;
        PanelAdjustInfo keyboardArea;
        std::shared_ptr<SelectionFwk::JSCallbackObject> cbCopy;
        explicit UvEntry(const std::shared_ptr<SelectionFwk::JSCallbackObject> &cb) : cbCopy(cb)
        {
        }
    };

    static std::shared_ptr<PanelListenerImpl> GetInstance();
    ~PanelListenerImpl();

    void SetEventHandler(std::shared_ptr<AppExecFwk::EventHandler> handler);
    std::shared_ptr<AppExecFwk::EventHandler> GetEventHandler();

    void OnPanelStatus(uint32_t windowId, bool isShow) override;
    void OnSizeChange(uint32_t windowId, const WindowSize &size) override;
    void OnSizeChange(uint32_t windowId, const WindowSize &size, const PanelAdjustInfo &keyboardArea,
        const std::string &event) override;

    std::shared_ptr<SelectionFwk::JSCallbackObject> GetCallback(uint32_t windowId, const std::string &type);

    static std::mutex listenerMutex_;
    static std::shared_ptr<PanelListenerImpl> instance_;
    mutable std::shared_mutex eventHandlerMutex_;
    std::shared_ptr<AppExecFwk::EventHandler> handler_;

    ConcurrentMap<uint32_t, std::map<std::string, std::shared_ptr<SelectionFwk::JSCallbackObject>>> callbacks_;
};
} // namespace SelectionFwk
} // namespace OHOS

#endif //SELECTION_IMF_PANEL_LISTENER_IMPL_H
