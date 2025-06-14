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
import PanelInfo from '@ohos.selectionInput.SelectionPanel'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index'

describe("SystemApiJsTest", function () {
  panel_: selectionManager.Panel = undefined;
  beforeAll(function () {
    console.info('beforeAll called');
  })

  afterAll(function () {
    console.info('AfterAll called');
  })

  /*
   * @tc.name:SystemApi_createPanel_001
   * @tc.desc:verify SystemApi of createPanel
   * @tc.type: FUNC
   */
  it('SystemApi_createPanel_001', 0, async function (done) {
    console.info('************* SystemApi_createPanel_001 Test start*************');
    try {
      let panelInfo = {
        panelType: 1,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      selectionManager.createPanel(this.context, panelInfo).then((panel) => {
        console.info('SystemApi_createPanel_001 promise success');
        this.panel_ = panel;
        expect(true).assertTrue();
        done();
      }).catch((error) => {
        console.info(`SystemApi_createPanel_001 result: ${JSON.stringify(error)}`);
        expect().assertFail();
        done();
      })
    } catch (error) {
      console.info(`SystemApi_createPanel_001 result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
    console.info('************* SystemApi_createPanel_001 Test end*************');
  });

  /*
   * @tc.name:SystemApi_hide_001
   * @tc.desc:verify SystemApi of hide
   * @tc.type: FUNC
   */
  it('SystemApi_hide_001', 0, async function (done) {
    console.info('************* SystemApi_hide_001 Test start*************');
    try {
      this.panel_.hide().then(() => {
        console.info('SystemApi_hide_001 promise success');
        expect(true).assertTrue();
        done();
      }).catch((error) => {
        console.info(`SystemApi_hide_001 result: ${JSON.stringify(error)}`);
        expect().assertFail();
        done();
      })
    } catch (error) {
      console.info(`SystemApi_hide_001 result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
    console.info('************* SystemApi_hide_001 Test end*************');
  });

  /*
   * @tc.name:SystemApi_startMoving_001
   * @tc.desc:verify SystemApi of startMoving
   * @tc.type: FUNC
   */
  it('SystemApi_startMoving_001', 0, async function (done) {
    console.info('************* SystemApi_startMoving_001 Test start*************');
    try {
      this.panel_.startMoving().then(() => {
        console.info('SystemApi_startMoving_001 promise success');
        expect(true).assertTrue();
        done();
      }).catch((error) => {
        console.info(`SystemApi_startMoving_001 result: ${JSON.stringify(error)}`);
        expect().assertFail();
        done();
      })
    } catch (error) {
      console.info(`SystemApi_startMoving_001 result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
    console.info('************* SystemApi_startMoving_001 Test end*************');
  });

  /*
   * @tc.name:SystemApi_moveTo_001
   * @tc.desc:verify SystemApi of moveTo
   * @tc.type: FUNC
   */
  it('SystemApi_moveTo_001', 0, async function (done) {
    console.info('************* SystemApi_moveTo_001 Test start*************');
    try {
      let x = 100;
      let y = 100;
      this.panel_.moveTo(x, y).then(() => {
        console.info('SystemApi_moveTo_001 promise success');
        expect(true).assertTrue();
        done();
      }).catch((error) => {
        console.info(`SystemApi_moveTo_001 result: ${JSON.stringify(error)}`);
        expect().assertFail();
        done();
      })
    } catch (error) {
      console.info(`SystemApi_moveTo_001 result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
    console.info('************* SystemApi_moveTo_001 Test end*************');
  });


});