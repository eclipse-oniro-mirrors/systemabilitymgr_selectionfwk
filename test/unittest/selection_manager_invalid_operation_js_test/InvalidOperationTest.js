/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
import { describe, beforeAll, afterAll, it, expect } from 'deccjsunit/index'
import { PanelType } from '@ohos.selectionInput.SelectionPanel';

describe("SelectionManagerInvalidOperationJsTest", function () {
  beforeAll(function () {
    console.info('beforeAll called');
  })

  afterAll(function () {
    console.info('AfterAll called');
  })

  /*
   * @tc.number  selectionfwk_on_selectionCompleted_invalid_operation_001
   * @tc.name    Test should throw BusinessError 33600003 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_on_selectionCompleted_invalid_operation_001', 0, async function (done) {
    console.info('************* selectionfwk_on_selectionCompleted_invalid_operation_001*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', (info) => {
        console.info(`selectionfwk_on_selectionCompleted_invalid_operation_001 is failed`);
        expect(false).assertTrue();
      })
      expect(false).assertTrue();
    } catch(error) {
      console.info(`selectionfwk_on_selectionCompleted_invalid_operation_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(33600003);
    }
    console.info('************* selectionfwk_on_selectionCompleted_invalid_operation_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_invalid_operation_001
   * @tc.name    Test should throw BusinessError 33600003 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_invalid_operation_001', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_invalid_operation_001 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panelInfo = {
        panelType: PanelType.MENU_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      await selectionManager.createPanel({stageMode : false}, panelInfo)
      expect(false).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_createPanel_invalid_operation_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(33600003);
    }
    console.info('************* selectionfwk_createPanel_invalid_operation_001 Test end*************');
    done();
  });
});
