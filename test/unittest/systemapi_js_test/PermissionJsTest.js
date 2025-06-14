/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

import selectionManager from '@ohos.selectionInput.selectionManager'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index'

describe("SystemApiJsTest", function () {
    beforeAll(function () {
        console.info('beforeAll called');
    })

    afterAll(function () {
        console.info('AfterAll called');
    })

    /*
     * @tc.name:SystemApi_destroyPanel_001
     * @tc.desc:verify SystemApi of destroyPanel
     * @tc.type: FUNC
     */
    // it("SystemApi_destroyPanel_001", 0, done => {
    //     console.info('----------------------SystemApi_destroyPanel_001---------------------------');
    //     try {
    //         selectionManager.destroyPanel(selectionManager.panel);
    //         expect(false).assertTrue();
    //         done();
    //     } catch (err) {
    //         expect(err.code).assertEqual(SYSTEMAPI_DENIED_CODE);
    //         done();
    //     }
    // });
});