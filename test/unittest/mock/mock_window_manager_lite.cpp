/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "window_manager_lite.h"

namespace OHOS {
namespace Rosen {

// UT 环境下没有可达的 sceneboard 服务，真实 IPC 会返回半构造的 FocusChangeInfo，
// 其 abilityToken_ (sptr) 在析构时触发 SEGV_MAPERR。
// 这里让 GetFocusWindowInfo 不走 IPC，保持 focusInfo 默认构造
// (windowId_ = INVALID_WINDOW_ID)，使 SelectionInputMonitor::HandleWindowFocused
// 走"窗口未聚焦"早返回分支，避免崩溃。
void WindowManagerLite::GetFocusWindowInfo(FocusChangeInfo& focusInfo, DisplayId displayId)
{
    (void)focusInfo;
    (void)displayId;
}

// ListWindowInfo 返回错误码，使 SaveSelectionStartInfo 跳过 bundleName 赋值，
// 避免访问 IPC 返回的 WindowInfo sptr。
WMError WindowManagerLite::ListWindowInfo(const WindowInfoOption& windowInfoOption,
    std::vector<sptr<WindowInfo>>& infos) const
{
    (void)windowInfoOption;
    (void)infos;
    return WMError::WM_DO_NOTHING;
}
} // namespace Rosen
} // namespace OHOS