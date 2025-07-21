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

#include "selection_listener_impl.h"
#include "selection_log.h"
#include "selection_data_inner.h"
#include "selection_panel_manager.h"
#include "selection_ability.h"
#include "wm_common.h"
#include "window.h"

namespace OHOS {
namespace SelectionFwk {

static void CopySelectionData(const SelectionInfoData& src, SelectionInfo& dst)
{
    dst = src.data;
}

ErrCode SelectionListenerImpl::OnSelectionChange(const SelectionInfoData& selectionInfoData)
{
    SELECTION_HILOGI("Recveive selection data length: %{public}u", selectionInfoData.data.text.length());
    SelectionInfo selectionInfo;
    CopySelectionData(selectionInfoData, selectionInfo);
    if (selectionI_ == nullptr) {
        SELECTION_HILOGI("selectionI_ is nullptr");
        return 1;
    }
    selectionI_->OnSelectionEvent(selectionInfo);
    return 0;
}

ErrCode SelectionListenerImpl::FocusChange(const SelectionFocusChangeInfo& focusChangeInfo)
{
    SELECTION_HILOGI("Recveive FocusChange: %{public}s.", focusChangeInfo.ToString().c_str());
    if (!focusChangeInfo.isFocused_) {
        return NO_ERROR;
    }
    auto selectionAppPid = getpid();
    if (selectionAppPid == focusChangeInfo.pid_) {
        SELECTION_HILOGI("No need to hide or destory selection panel because window of selection app is focused.");
        return NO_ERROR;
    }

    auto& panelManager = SelectionPanelManager::GetInstance();
    if (!panelManager.FindWindowID(focusChangeInfo.windowId_)) {
        SELECTION_HILOGI("The focus window is not a selection window, hide or destroy selection panels.");
        panelManager.Dispose();
        return NO_ERROR;
    }

    return NO_ERROR;
}
} // namespace SelectionFramework
} // namespace OHOS