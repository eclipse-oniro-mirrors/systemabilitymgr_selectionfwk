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
#include "selection_panel.h"


namespace OHOS {
namespace SelectionFwk {

static void CopySelectionData(const SelectionDataInner& src, SelectionData& dst)
{
    dst = src.data;
}

ErrCode SelectionListenerImpl::OnSelectionChange(const SelectionDataInner& selectionDataInner)
{
    SELECTION_HILOGI("Recveive selection data: %{public}s", selectionDataInner.data.text.c_str());
    SelectionData selectionData;
    CopySelectionData(selectionDataInner, selectionData);
    if (selectionI_ == nullptr) {
        SELECTION_HILOGI("selectionI_ is nullptr");
        return 1;
    }
    selectionI_->OnSelectionEvent(selectionData);
    return 0;
}

ErrCode SelectionListenerImpl::FocusChange(int32_t windowID)
{
    SELECTION_HILOGI("Recveive windowID: %{public}d", windowID);
    if (selectionI_ == nullptr) {
        SELECTION_HILOGI("selectionI_ is nullptr");
        return 1;
    }

    auto selectionPanel = std::make_shared<SelectionPanel>();
    if (selectionPanel == nullptr) {
        SELECTION_HILOGE("selectionPanel is nullptr");
        return 1;
    }
    int32_t panelWindowID = selectionPanel->GetWindowId();
    if (windowID != panelWindowID) {
        SELECTION_HILOGI("selectionPanel is UnFocus");
        selectionPanel->HidePanel();
    }
    return 0;
}

} // namespace SelectionFramework
} // namespace OHOS