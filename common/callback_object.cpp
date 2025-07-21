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

#include "callback_object.h"
#include "selection_log.h"

#include <uv.h>

namespace OHOS {
namespace SelectionFwk {
constexpr int32_t MAX_TIMEOUT = 2000;
JSCallbackObject::JSCallbackObject(napi_env env, napi_value callback, std::thread::id threadId,
    std::shared_ptr<AppExecFwk::EventHandler> jsHandler)
    : env_(env), threadId_(threadId), jsHandler_(jsHandler)
{
    napi_create_reference(env, callback, 1, &callback_);
}

JSCallbackObject::~JSCallbackObject()
{
    if (callback_ != nullptr) {
        if (threadId_ == std::this_thread::get_id()) {
            napi_delete_reference(env_, callback_);
            env_ = nullptr;
            return;
        }
        isDone_ = std::make_shared<BlockData<bool>>(MAX_TIMEOUT, false);
        std::string type = "~JSCallbackObject";
        auto eventHandler = jsHandler_;
        if (eventHandler == nullptr) {
            SELECTION_HILOGE("eventHandler is nullptr!");
            return;
        }
        auto task = [env = env_, callback = callback_, isDone = isDone_]() {
            napi_delete_reference(env, callback);
            bool isFinish = true;
            isDone->SetValue(isFinish);
        };
        eventHandler->PostTask(task, type);
        isDone_->GetValue();
    }
    env_ = nullptr;
}

bool JSCallbackObject::operator==(const JSCallbackObject& other) const
{
    if (other.env_ != env_ || other.threadId_ != threadId_) {
        return false;
    }

    napi_value thisValue;
    napi_value otherValue;
    napi_status status = napi_get_reference_value(env_, this->callback_, &thisValue);
    if (status != napi_ok) {
        return false;
    }
    status = napi_get_reference_value(env_, other.callback_, &otherValue);
    if (status != napi_ok) {
        return false;
    }

    bool result;
    status = napi_strict_equals(env_, thisValue, otherValue, &result);
    if (status != napi_ok) {
        return false;
    }

    return result;
}

JSMsgHandlerCallbackObject::JSMsgHandlerCallbackObject(napi_env env, napi_value onTerminated, napi_value onMessage)
    : env_(env), handler_(AppExecFwk::EventHandler::Current()), threadId_(std::this_thread::get_id())
{
    napi_create_reference(env, onTerminated, 1, &onTerminatedCallback_);
    napi_create_reference(env, onMessage, 1, &onMessageCallback_);
}

JSMsgHandlerCallbackObject::~JSMsgHandlerCallbackObject()
{
    if (threadId_ == std::this_thread::get_id()) {
        if (onTerminatedCallback_ != nullptr) {
            napi_delete_reference(env_, onTerminatedCallback_);
        }
        if (onMessageCallback_ != nullptr) {
            napi_delete_reference(env_, onMessageCallback_);
        }
        env_ = nullptr;
        return;
    }
    SELECTION_HILOGW("Thread id is not same, abstract destructor is run in muti-thread!");
    env_ = nullptr;
}

std::shared_ptr<AppExecFwk::EventHandler> JSMsgHandlerCallbackObject::GetEventHandler()
{
    std::lock_guard<std::mutex> lock(eventHandlerMutex_);
    return handler_;
}
} // namespace SelectionFwk
} // namespace OHOS
