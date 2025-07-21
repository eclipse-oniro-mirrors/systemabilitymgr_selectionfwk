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

#ifndef SELECTION_FOCUS_MONITOR_MANAGER_H
#define SELECTION_FOCUS_MONITOR_MANAGER_H

#include <functional>
#include "focus_change_info.h"
#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif

namespace OHOS {
namespace SelectionFwk {
using FocusHandle = std::function<void(const sptr<Rosen::FocusChangeInfo> &focusChangeInfo, bool)>;
class FocusMonitorManager {
public:
    static FocusMonitorManager &GetInstance();
    void RegisterFocusChangedListener(const FocusHandle &handle);
    void UnregisterFocusChangedListener();

private:
    FocusMonitorManager() = default;
private:
    sptr<OHOS::Rosen::IFocusChangedListener> focusListener_ = nullptr;
};
} // namespace SelectionFwk
} // namespace OHOS

#endif // SELECTION_FOCUS_MONITOR_MANAGER_H
