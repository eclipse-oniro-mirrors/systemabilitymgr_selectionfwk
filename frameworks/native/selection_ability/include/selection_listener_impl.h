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

#ifndef SELECTION_LISTENER_IMPL_H
#define SELECTION_LISTENER_IMPL_H
#include "refbase.h"
#include "selection_listener_stub.h"
#include "selection_interface.h"

namespace OHOS {
namespace SelectionFwk {
class SelectionListenerImpl : public SelectionListenerStub {
public:
    SelectionListenerImpl(std::shared_ptr<SelectionInterface> selectionI) : selectionI_(selectionI) {}
    ~SelectionListenerImpl() override = default;
    ErrCode OnSelectionChange(const SelectionInfoData& SelectionInfoData) override;
    ErrCode FocusChange(const SelectionFocusChangeInfo& focusChangeInfo) override;

private:
    std::shared_ptr<SelectionInterface> selectionI_;
};
}
}
#endif // SELECTION_LISTENER_IMPL_H