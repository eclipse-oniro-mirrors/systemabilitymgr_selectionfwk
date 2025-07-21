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

#include "focus_change_listener.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
using namespace Rosen;
FocusMonitorManager &FocusMonitorManager::GetInstance()
{
    static FocusMonitorManager focusMonitorManager;
    return focusMonitorManager;
}

void FocusMonitorManager::RegisterFocusChangedListener(const FocusHandle &handle)
{
    if (focusListener_ != nullptr) {
        SELECTION_HILOGE("focusListener_ has been registered by others.");
        return;
    }

    focusListener_ = sptr<FocusChangedListener>::MakeSptr(handle);
    if (focusListener_ == nullptr) {
        SELECTION_HILOGE("failed to create focusListener_");
        return;
    }
#ifdef SCENE_BOARD_ENABLE
    WMError ret = WindowManagerLite::GetInstance().RegisterFocusChangedListener(focusListener_);
#else
    WMError ret = WindowManager::GetInstance().RegisterFocusChangedListener(focusListener_);
#endif
    SELECTION_HILOGI("register focus changed focusListener_ ret: %{public}d", ret);
}

void FocusMonitorManager::UnregisterFocusChangedListener()
{
    if (focusListener_ == nullptr) {
        SELECTION_HILOGE("focusListener_ is nullptr");
        return;
    }
#ifdef SCENE_BOARD_ENABLE
    WMError ret = WindowManagerLite::GetInstance().UnregisterFocusChangedListener(focusListener_);
#else
    WMError ret = WindowManager::GetInstance().UnregisterFocusChangedListener(focusListener_);
#endif
    SELECTION_HILOGI("Unregister focus changed focusListener_ ret: %{public}d", ret);
    focusListener_ = nullptr;
}
} // namespace SelectionFwk
} // namespace OHOS