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

#ifndef SELECTION_FDSAN_H
#define SELECTION_FDSAN_H

#include <stdint.h>
#include <stdio.h>
#include "selection_log.h"

/*
 * fdsan declarations are in musl's <stdio.h>, not a standalone <fdsan.h>.
 * This wrapper provides inline functions for marking and closing fds
 * with a business-specific tag, following the fdsan接入指导.
 */

#ifndef FDSAN_OWNER_TYPE_GENERIC_00
#define FDSAN_OWNER_TYPE_GENERIC_00 static_cast<enum fdsan_owner_type>(128)
#endif

namespace OHOS::SelectionFwk {
inline void FdsanMark(int fd)
{
    fdsan_exchange_owner_tag(fd, 0,
        fdsan_create_owner_tag(FDSAN_OWNER_TYPE_GENERIC_00, LOG_DOMAIN));
}

inline void FdsanClose(int fd)
{
    fdsan_close_with_tag(fd,
        fdsan_create_owner_tag(FDSAN_OWNER_TYPE_GENERIC_00, LOG_DOMAIN));
}
} // namespace OHOS::SelectionFwk

#endif /* SELECTION_FDSAN_H */
