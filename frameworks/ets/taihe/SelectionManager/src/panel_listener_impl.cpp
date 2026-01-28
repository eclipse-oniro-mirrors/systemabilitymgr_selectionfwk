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
#include "util.h"

namespace OHOS {
namespace SelectionFwk {
std::shared_ptr<PanelListenerImpl> PanelListenerImpl::instance_{ nullptr };
std::mutex PanelListenerImpl::listenerMutex_;
std::shared_ptr<PanelListenerImpl> PanelListenerImpl::GetInstance()
{
    SELECTION_HILOGI("PanelListenerImpl::GetInstance");
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(listenerMutex_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<PanelListenerImpl>();
        }
    }
    return instance_;
}

PanelListenerImpl::~PanelListenerImpl() {}

void PanelListenerImpl::OnPanelStatus(uint32_t windowId, const std::string& status)
{
    CallbackVector callBacks = GetCallback(windowId, status);
    if (callBacks.empty()) {
        SELECTION_HILOGE("callBacks is empty!");
        return;
    }
    SELECTION_HILOGI("OnPanelStatus status: %{public}s", status.c_str());
    UndefinedType_t callbackType = UndefinedType_t::make_undefined();
    for (auto &cb : callBacks) {
        cb(callbackType);
    }
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

void PanelListenerImpl::Subscribe(uint32_t windowId, const std::string &type,
    callbackType &&cbObject)
{
    callbacks_.Compute(windowId, [cbObject, &type](auto windowId, TypeMap& cbs) {
        if (cbs.find(type) == cbs.end()) {
            cbs.try_emplace(type, CallbackVector {std::move(cbObject)});
            return !cbs.empty();
        }
        auto it = std::find_if(
            cbs[type].begin(), cbs[type].end(),
            [cbObject](const callbackType& vecCbObject) { return vecCbObject == cbObject; });
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
    const callbackType &cbObject)
{
    callbacks_.ComputeIfPresent(windowId, [&type, cbObject](auto windowId, TypeMap& cbs) {
        auto it = cbs.find(type);
        if (it == cbs.end()) {
            return !cbs.empty();
        }
        auto targetCallback = std::find_if(
            cbs[type].begin(), cbs[type].end(),
            [cbObject](callbackType vecCbObject) { return vecCbObject == cbObject; });
        if (targetCallback != cbs[type].end()) {
            cbs[type].erase(targetCallback);
        }

        return !cbs.empty();
    });
}

} // namespace SelectionFwk
} // namespace OHOS
