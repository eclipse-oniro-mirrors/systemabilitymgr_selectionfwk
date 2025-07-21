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

#include "panel_listener_impl.h"

#include "selection_js_utils.h"
#include "callback_handler.h"
#include "util.h"

namespace OHOS {
namespace SelectionFwk {
std::shared_ptr<PanelListenerImpl> PanelListenerImpl::instance_{ nullptr };
std::mutex PanelListenerImpl::listenerMutex_;
std::shared_ptr<PanelListenerImpl> PanelListenerImpl::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(listenerMutex_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<PanelListenerImpl>();
        }
    }
    return instance_;
}

PanelListenerImpl::~PanelListenerImpl() {}

void PanelListenerImpl::SetEventHandler(std::shared_ptr<AppExecFwk::EventHandler> handler)
{
    std::unique_lock<decltype(eventHandlerMutex_)> lock(eventHandlerMutex_);
    handler_ = handler;
}

std::shared_ptr<AppExecFwk::EventHandler> PanelListenerImpl::GetEventHandler()
{
    std::shared_lock<decltype(eventHandlerMutex_)> lock(eventHandlerMutex_);
    return handler_;
}

void PanelListenerImpl::OnPanelStatus(uint32_t windowId, const std::string& status)
{
    auto eventHandler = GetEventHandler();
    if (eventHandler == nullptr) {
        SELECTION_HILOGE("eventHandler is nullptr!");
        return;
    }
    CallbackVector callBacks = GetCallback(windowId, status);
    if (callBacks.empty()) {
        SELECTION_HILOGE("callBacks is empty!");
        return;
    }
    SELECTION_HILOGI("OnPanelStatus status: %{public}s", status.c_str());
    auto task = [callBacks]() {
        SelectionFwk::JsCallbackHandler::Traverse(callBacks);
    };
    eventHandler->PostTask(task, status, 0, AppExecFwk::EventQueue::Priority::VIP);
}

CallbackVector PanelListenerImpl::GetCallback(uint32_t windowId, const std::string &type)
{
    CallbackVector callBackVector;
    callbacks_.ComputeIfPresent(windowId, [&type, &callBackVector](uint32_t id, TypeMap& callbacks) {
        if (callbacks.find(type) != callbacks.end()) {
            callBackVector = callbacks[type];
        }
        return !callbacks.empty();
    });
    return callBackVector;
}

napi_value JsWindowSize::Write(napi_env env, const WindowSize &nativeObject)
{
    napi_value jsObject = nullptr;
    napi_create_object(env, &jsObject);
    bool ret = SelectionFwk::JsUtil::Object::WriteProperty(env, jsObject, "width", nativeObject.width);
    ret = ret && SelectionFwk::JsUtil::Object::WriteProperty(env, jsObject, "height", nativeObject.height);
    return ret ? jsObject : SelectionFwk::JsUtil::Const::Null(env);
}

bool JsWindowSize::Read(napi_env env, napi_value jsObject, WindowSize &nativeObject)
{
    auto ret = SelectionFwk::JsUtil::Object::ReadProperty(env, jsObject, "width", nativeObject.width);
    ret = ret && SelectionFwk::JsUtil::Object::ReadProperty(env, jsObject, "height", nativeObject.height);
    return ret;
}

napi_value JsKeyboardArea::Write(napi_env env, const PanelAdjustInfo &nativeObject)
{
    napi_value jsObject = nullptr;
    napi_create_object(env, &jsObject);
    bool ret = SelectionFwk::JsUtil::Object::WriteProperty(env, jsObject, "top", nativeObject.top);
    ret = ret && SelectionFwk::JsUtil::Object::WriteProperty(env, jsObject, "bottom", nativeObject.bottom);
    ret = ret && SelectionFwk::JsUtil::Object::WriteProperty(env, jsObject, "left", nativeObject.left);
    ret = ret && SelectionFwk::JsUtil::Object::WriteProperty(env, jsObject, "right", nativeObject.right);
    return ret ? jsObject : SelectionFwk::JsUtil::Const::Null(env);
}

bool JsKeyboardArea::Read(napi_env env, napi_value jsObject, PanelAdjustInfo &nativeObject)
{
    bool ret = SelectionFwk::JsUtil::Object::ReadProperty(env, jsObject, "top", nativeObject.top);
    ret = ret && SelectionFwk::JsUtil::Object::ReadProperty(env, jsObject, "bottom", nativeObject.bottom);
    ret = ret && SelectionFwk::JsUtil::Object::ReadProperty(env, jsObject, "left", nativeObject.left);
    ret = ret && SelectionFwk::JsUtil::Object::ReadProperty(env, jsObject, "right", nativeObject.right);
    return ret;
}

void PanelListenerImpl::Subscribe(uint32_t windowId, const std::string &type,
    std::shared_ptr<JSCallbackObject> cbObject)
{
    callbacks_.Compute(windowId, [cbObject, &type](auto windowId, TypeMap& cbs) {
        if (cbs.find(type) == cbs.end()) {
            cbs.try_emplace(type, CallbackVector {cbObject});
            return !cbs.empty();
        }
        auto it = std::find_if(
            cbs[type].begin(), cbs[type].end(),
            [cbObject](const std::shared_ptr<JSCallbackObject>& vecCbObject) { return *vecCbObject == *cbObject; });
        if (it == cbs[type].end()) {
            cbs[type].emplace_back(cbObject);
            SELECTION_HILOGI("start to subscribe type: %{public}s of windowId: %{public}u.", type.c_str(), windowId);
        } else {
            SELECTION_HILOGI("type: %{public}s of windowId: %{public}u already subscribed.", type.c_str(), windowId);
        }
        return !cbs.empty();
    });
}

void PanelListenerImpl::RemoveInfo(const std::string &type, uint32_t windowId)
{
    callbacks_.ComputeIfPresent(windowId, [&type](auto windowId, TypeMap& cbs) {
        cbs.erase(type);
        return !cbs.empty();
    });
}

void PanelListenerImpl::RemoveInfo(const std::string &type, uint32_t windowId,
    std::shared_ptr<JSCallbackObject> cbObject)
{
    callbacks_.ComputeIfPresent(windowId, [&type, cbObject](auto windowId, TypeMap& cbs) {
        auto it = cbs.find(type);
        if (it == cbs.end()) {
            return !cbs.empty();
        }
        auto targetCallback = std::find_if(
            cbs[type].begin(), cbs[type].end(),
            [cbObject](std::shared_ptr<JSCallbackObject> vecCbObject) { return *vecCbObject == *cbObject; });
        if (targetCallback != cbs[type].end()) {
            cbs[type].erase(targetCallback);
        }

        return !cbs.empty();
    });
}

} // namespace SelectionFwk
} // namespace OHOS
