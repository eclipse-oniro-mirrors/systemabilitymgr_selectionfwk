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


  /*
  * @tc.number  selection_panel_test_off_Hide_001
  * @tc.name    Test whether the register the callback of the panel method is valid.
  * @tc.desc    Function test
  * @tc.level   2
  */
    it('selection_panel_test_off_Hide_001', 0, async function (done) {
    console.info('************* selection_panel_test_off_Hide_001*************');
    try {
      this.panel_.off('hidden', () => {
        console.info(`panel on hidde success`);
        expect(true).assertTrue();
        done();
      });
    } catch(error) {
      console.info(`panel on hidden fail result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
  });

  /*
  * @tc.number  selection_panel_test_off_Hide_002
  * @tc.name    Test whether the register the callback of the panel method is valid.
  * @tc.desc    Function test
  * @tc.level   2
  */
    it('selection_panel_test_off_Hide_001', 0, async function (done) {
    console.info('************* selection_panel_test_off_Hide_002*************');
    try {
      this.panel_.off('hidden');
    } catch(error) {
      console.info(`panel on hidden fail result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
  });

  /*
  * @tc.number  selection_panel_test_on_destroyed_001
  * @tc.name    Test whether the register the callback of the panel method is valid.
  * @tc.desc    Function test
  * @tc.level   2
  */
    it('selection_panel_test_on_destroyed_001', 0, async function (done) {
    console.info('************* selection_panel_test_on_destroyed_001*************');
    try {
      this.panel_.on('destroyed', () => {
        console.info(`panel on hide success`);
        expect(true).assertTrue();
        done();
      });
    } catch(error) {
      console.info(`panel on destroyed fail result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
  });

  /*
  * @tc.number  selection_panel_test_off_destroyed_001
  * @tc.name    Test whether the register the callback of the panel method is valid.
  * @tc.desc    Function test
  * @tc.level   2
  */
    it('selection_panel_test_off_destroyed_001', 0, async function (done) {
    console.info('************* selection_panel_test_off_destroyed_001*************');
    try {
      this.panel_.off('destroyed', () => {
        console.info(`panel on destroyed success`);
        expect(true).assertTrue();
        done();
      });
    } catch(error) {
      console.info(`panel on destroyed fail result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
  });

  /*
  * @tc.number  selection_panel_test_off_destroyed_002
  * @tc.name    Test whether the register the callback of the panel method is valid.
  * @tc.desc    Function test
  * @tc.level   2
  */
    it('selection_panel_test_off_destroyed_002', 0, async function (done) {
    console.info('************* selection_panel_test_off_destroyed_002*************');
    try {
      this.panel_.off('destroyed');
    } catch(error) {
      console.info(`panel on "hidden fail result: ${JSON.stringify(error)}`);
      expect().assertFail();
      done();
    }
  });

    /*
     * @tc.name:selection_panel_method_destroyPanel_001
     * @tc.desc:verify method of destroyPanel
     * @tc.type: FUNC
     */
    it('selection_panel_method_destroyPanel_001', 0, async function (done) {
      console.info('************* selection_panel_method_destroyPanel_001 Test start*************');
      try {
        selectionManager.destroyPanel(this.panel_).then(() => {
          console.info('selection_panel_method_destroyPanel promise success');
          expect(true).assertTrue();
          done();
        }).catch((error) => {
          console.info(`selection_panel_method_destroyPanel result: ${JSON.stringify(error)}`);
          expect().assertFail();
          done();
        })
      } catch (error) {
        console.info(`selection_panel_method_destroyPanel result: ${JSON.stringify(error)}`);
        expect().assertFail();
        done();
      }
      console.info('************* selection_panel_method_destroyPanel Test end*************');
    });
});