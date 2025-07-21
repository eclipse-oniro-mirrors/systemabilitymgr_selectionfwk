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
import { describe, beforeAll, afterAll, beforeEach, afterEach, it, expect } from 'deccjsunit/index'
import { PanelType } from '@ohos.selectionInput.SelectionPanel';

describe("SelectionManagerJsTest", function () {
  beforeAll(function () {
    console.info('beforeAll called');
  })

  afterAll(function () {
    console.info('afterAll called');
  })

  beforeEach(function () {
    console.info('beforeEach called');
  });

  afterEach(function () {
    console.info('afterEach called');
  });

  async function createSelectionPanel()
  {
    let panelInfo = {
      panelType: PanelType.MENU_PANEL,
      x: 0,
      y: 0,
      width: 100,
      height: 100
    }
    return await selectionManager.createPanel({stageMode : false}, panelInfo);
  }

  async function destroySelectionPanel(panel)
  {
    await selectionManager.destroyPanel(panel);
  }

  /*
   * @tc.number  selectionfwk_on_selectionCompleted_001
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_on_selectionCompleted_001', 0, async function (done) {
    console.info('************* selectionfwk_on_selectionCompleted_001*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompletedtest', (info) => {
        console.info(`selectionfwk_on_selectionCompleted_001 is failed`);
        expect(false).assertTrue();
        done();
      });
      console.info(`selectionfwk_on_selectionCompleted_001 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_on_selectionCompleted_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_on_selectionCompleted_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_on_selectionCompleted_002
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_on_selectionCompleted_002', 0, async function (done) {
    console.info('************* selectionfwk_on_selectionCompleted_002*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted');
      console.info(`selectionfwk_on_selectionCompleted_002 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_on_selectionCompleted_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_on_selectionCompleted_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_on_selectionCompleted_003
   * @tc.name    Test should success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_on_selectionCompleted_003', 0, async function (done) {
    console.info('************* selectionfwk_on_selectionCompleted_003*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', (info) => {
        console.info(`selectionfwk_on_selectionCompleted_003 callback enter`);
        expect(true).assertTrue();
        done();
      });
      console.info(`selectionfwk_on_selectionCompleted_003 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_on_selectionCompleted_003 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    console.info('************* selectionfwk_on_selectionCompleted_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_on_selectionCompleted_004
   * @tc.name    Test 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_on_selectionCompleted_004', 0, async function (done) {
    console.info('************* selectionfwk_on_selectionCompleted_004*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on(null, (info) => {
        console.info(`selectionfwk_on_selectionCompleted_004 callback enter`);
        done();
      });
      console.info(`selectionfwk_on_selectionCompleted_004 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_on_selectionCompleted_004 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_on_selectionCompleted_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_on_selectionCompleted_005
   * @tc.name    Test 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_on_selectionCompleted_005', 0, async function (done) {
    console.info('************* selectionfwk_on_selectionCompleted_005*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on(undefined, (info) => {
        console.info(`selectionfwk_on_selectionCompleted_005 callback enter`);
        done();
      });
      console.info(`selectionfwk_on_selectionCompleted_005 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_on_selectionCompleted_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_on_selectionCompleted_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_on_selectionCompleted_006
   * @tc.name    Test 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_on_selectionCompleted_006', 0, async function (done) {
    console.info('************* selectionfwk_on_selectionCompleted_006*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', null);
      console.info(`selectionfwk_on_selectionCompleted_006 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_on_selectionCompleted_006 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_on_selectionCompleted_006 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_on_selectionCompleted_007
   * @tc.name    Test 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_on_selectionCompleted_007', 0, async function (done) {
    console.info('************* selectionfwk_on_selectionCompleted_007*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', undefined);
      console.info(`selectionfwk_on_selectionCompleted_007 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_on_selectionCompleted_007 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_on_selectionCompleted_007 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_off_selectionCompleted_001
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_off_selectionCompleted_001', 0, async function (done) {
    console.info('************* selectionfwk_off_selectionCompleted_001*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.off('selectionCompletedtest', (info) => {
        console.info(`selectionfwk_off_selectionCompleted_001 is failed`);
        done();
      });
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_off_selectionCompleted_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_off_selectionCompleted_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_off_selectionCompleted_002
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_off_selectionCompleted_002', 0, async function (done) {
    console.info('************* selectionfwk_off_selectionCompleted_002*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.off();
      console.info(`selectionfwk_off_selectionCompleted_002 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_off_selectionCompleted_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_off_selectionCompleted_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_off_selectionCompleted_003
   * @tc.name    Test should success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_off_selectionCompleted_003', 0, async function (done) {
    console.info('************* selectionfwk_off_selectionCompleted_003*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', (info) => {
        console.info(`selectionfwk_on_selectionCompleted_003 callback enter`);
        done();
      });
      selectionManager.off('selectionCompleted');
      console.info(`selectionfwk_off_selectionCompleted_003 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_off_selectionCompleted_003 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    console.info('************* selectionfwk_off_selectionCompleted_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_off_selectionCompleted_004
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_off_selectionCompleted_004', 0, async function (done) {
    console.info('************* selectionfwk_off_selectionCompleted_004*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', (info) => {
        console.info(`selectionfwk_off_selectionCompleted_004 callback enter`);
        done();
      });
      selectionManager.off(null);
      console.info(`selectionfwk_off_selectionCompleted_004 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_off_selectionCompleted_004 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_off_selectionCompleted_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_off_selectionCompleted_005
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_off_selectionCompleted_005', 0, async function (done) {
    console.info('************* selectionfwk_off_selectionCompleted_005*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', (info) => {
        console.info(`selectionfwk_off_selectionCompleted_005 callback enter`);
        done();
      });
      selectionManager.off(undefined);
      console.info(`selectionfwk_off_selectionCompleted_005 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_off_selectionCompleted_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_off_selectionCompleted_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_off_selectionCompleted_006
   * @tc.name    Test return success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_off_selectionCompleted_006', 0, async function (done) {
    console.info('************* selectionfwk_off_selectionCompleted_006*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', (info) => {
        console.info(`selectionfwk_off_selectionCompleted_006 callback enter`);
        done();
      });
      selectionManager.off('selectionCompleted', null);
      console.info(`selectionfwk_off_selectionCompleted_006 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_off_selectionCompleted_006 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    console.info('************* selectionfwk_off_selectionCompleted_006 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_off_selectionCompleted_007
   * @tc.name    Test return sucess, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_off_selectionCompleted_007', 0, async function (done) {
    console.info('************* selectionfwk_off_selectionCompleted_007*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      selectionManager.on('selectionCompleted', (info) => {
        console.info(`selectionfwk_off_selectionCompleted_007 callback enter`);
        done();
      });
      selectionManager.off('selectionCompleted', undefined);
      console.info(`selectionfwk_off_selectionCompleted_007 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_off_selectionCompleted_007 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    console.info('************* selectionfwk_off_selectionCompleted_007 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_001
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_001', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_001 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      await selectionManager.createPanel({stageMode : false});
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_002
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_002', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_002 Test start*************');
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
      await selectionManager.createPanel(selectionManager.Context, panelInfo);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_003
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_003', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_003 Test start*************');
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
      await selectionManager.createPanel({stageMode : true}, panelInfo);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_003 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_004
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_004', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_004 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panelTemp;
    try {
      let panelInfo = {
        panelType: PanelType.MENU_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      await selectionManager.createPanel({stageMode : false}, panelInfo)
      .then((panel) => {
        panelTemp = panel;
        expect(true).assertTrue();
      })
      await destroySelectionPanel(panelTemp);
    } catch (error) {
      console.info(`selectionfwk_createPanel_004 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    console.info('************* selectionfwk_createPanel_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_005
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_005', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_005 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      let panelTemp = await selectionManager.createPanel({stageMode : false}, panelInfo);
      await destroySelectionPanel(panelTemp);
    } catch (error) {
      console.info(`selectionfwk_createPanel_005 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    console.info('************* selectionfwk_createPanel_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_006
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_006', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_006 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: -1,
        y: 0,
        width: 100,
        height: 100
      }
      await selectionManager.createPanel({stageMode : false}, panelInfo);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_006 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_006 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_007
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_007', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_007 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: -1,
        width: 100,
        height: 100
      }
      await selectionManager.createPanel({stageMode : false}, panelInfo);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_007 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_007 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_008
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_008', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_008 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: 0,
        width: -1,
        height: 100
      }
      await selectionManager.createPanel({stageMode : false}, panelInfo)
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_008 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_008 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_009
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_009', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_009 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: -1
      }
      await selectionManager.createPanel({stageMode : false}, panelInfo)
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_009 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_009 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_010
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_010', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_010 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      await selectionManager.createPanel({stageMode : false}, null);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_010 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_010 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_010
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_011', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_011 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      await selectionManager.createPanel({stageMode : false}, undefined);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_011 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_011 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_012
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_012', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_012 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      await selectionManager.createPanel(null, panelInfo)
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_012 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_012 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_createPanel_013
   * @tc.name    verify method of createPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_createPanel_013', 0, async function (done) {
    console.info('************* selectionfwk_createPanel_013 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      await selectionManager.createPanel(undefined, panelInfo)
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_createPanel_013 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_createPanel_013 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_destroyPanel_001
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_destroyPanel_001', 0, async function (done) {
    console.info('************* selectionfwk_destroyPanel_001 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      await selectionManager.destroyPanel();
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_destroyPanel_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_destroyPanel_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_destroyPanel_002
   * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_destroyPanel_002', 0, async function (done) {
    console.info('************* selectionfwk_destroyPanel_002 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      let panel = 1;
      await selectionManager.destroyPanel(panel);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_destroyPanel_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_destroyPanel_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_destroyPanel_003
   * @tc.name    verify method of destroyPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_destroyPanel_003', 0, async function (done) {
    console.info('************* selectionfwk_destroyPanel_003 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await selectionManager.destroyPanel(panel);
      console.info(`selectionfwk_destroyPanel_003 promise success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_destroyPanel_003 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    console.info('************* selectionfwk_destroyPanel_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_destroyPanel_004
   * @tc.name    verify method of destroyPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_destroyPanel_004', 0, async function (done) {
    console.info('************* selectionfwk_destroyPanel_004 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      await selectionManager.destroyPanel(null);
      console.info(`selectionfwk_destroyPanel_004 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_destroyPanel_004 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_destroyPanel_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_destroyPanel_005
   * @tc.name    verify method of destroyPanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_destroyPanel_005', 0, async function (done) {
    console.info('************* selectionfwk_destroyPanel_005 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    try {
      await selectionManager.destroyPanel(undefined);
      console.info(`selectionfwk_destroyPanel_005 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_destroyPanel_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    console.info('************* selectionfwk_destroyPanel_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selection_panel_setUiContent_001
   * @tc.name    verify ERROR code 401 of setUiContent.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selection_panel_setUiContent_001', 0, async function (done) {
    console.info('************* selection_panel_setUiContent_001 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
     await panel.setUiContent();
      console.info(`selection_panel_setUiContent_001 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selection_panel_setUiContent_001 result: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selection_panel_setUiContent_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selection_panel_setUiContent_002
   * @tc.name    verify ERROR code 401 of setUiContent.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selection_panel_setUiContent_002', 0, async function (done) {
    console.info('************* selection_panel_setUiContent_002 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.setUiContent(20);
      console.info(`selection_panel_setUiContent_002 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selection_panel_setUiContent_002 result: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selection_panel_setUiContent_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selection_panel_setUiContent_003
   * @tc.name    verify ERROR code 33600002 of setUiContent.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selection_panel_setUiContent_003', 0, async function (done) {
    console.info('************* selection_panel_setUiContent_003 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    await destroySelectionPanel(panel);
    try {
      await panel.setUiContent('pages/index/index');
      expect().assertFail();
    } catch (error) {
      console.info(`selection_panel_setUiContent_003 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(33600002);
    }
    console.info('************* selection_panel_setUiContent_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selection_panel_setUiContent_004
   * @tc.name    verify ERROR code 401 of setUiContent.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selection_panel_setUiContent_004', 0, async function (done) {
    console.info('************* selection_panel_setUiContent_004 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.setUiContent(null);
      expect().assertFail();
    } catch (error) {
      console.info(`selection_panel_setUiContent_004 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selection_panel_setUiContent_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selection_panel_setUiContent_005
   * @tc.name    verify ERROR code 401 of setUiContent.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selection_panel_setUiContent_005', 0, async function (done) {
    console.info('************* selection_panel_setUiContent_005 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.setUiContent(undefined);
      expect().assertFail();
    } catch (error) {
      console.info(`selection_panel_setUiContent_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selection_panel_setUiContent_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_moveTo_001
   * @tc.name    verify ERROR code 401 of the panel moveTo.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_moveTo_001', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_001 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(20);
      console.info(`selectionfwk_panel_moveTo_001 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_moveTo_002
   * @tc.name    verify ERROR code 401 of the panel moveTo.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_moveTo_002', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_002 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(20, '20');
      console.info(`selectionfwk_panel_moveTo_002 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_moveTo_003
   * @tc.name    verify ERROR code 33600002 the panel moveTo.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_moveTo_003', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_003 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    await destroySelectionPanel(panel);
    try {
      await panel.moveTo(20, 20);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_003 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(33600002);
    }
    console.info('************* selectionfwk_panel_moveTo_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_moveTo_004
   * @tc.name    verify method of the panel moveTo.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_moveTo_004', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_004 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(50, 50);
      console.info(`selectionfwk_panel_moveTo_004 promise success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_004 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_moveTo_005
   * @tc.name    verify method of the panel moveTo.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_moveTo_005', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_005 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(-1, 50);
      console.info(`selectionfwk_panel_moveTo_005 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_moveTo_006
   * @tc.name    verify method of the panel moveTo.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_moveTo_006', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_006 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(50, -1);
      console.info(`selectionfwk_panel_moveTo_006 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_006 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_006 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_moveTo_007
   * @tc.name    verify method of the panel moveTo.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_moveTo_007', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_007 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(null, 50);
      console.info(`selectionfwk_panel_moveTo_007 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_007 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_007 Test end*************');
    done();
  });

  /*
  * @tc.number  selectionfwk_panel_moveTo_008
  * @tc.name    verify method of the panel moveTo.
  * @tc.desc    Function test
  * @tc.level   0
  */
  it('selectionfwk_panel_moveTo_008', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_008 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(undefined, 50);
      console.info(`selectionfwk_panel_moveTo_008 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_008 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_008 Test end*************');
    done();
  });

  /*
  * @tc.number  selectionfwk_panel_moveTo_009
  * @tc.name    verify method of the panel moveTo.
  * @tc.desc    Function test
  * @tc.level   0
  */
  it('selectionfwk_panel_moveTo_009', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_009 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(50, null);
      console.info(`selectionfwk_panel_moveTo_009 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_009 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_009 Test end*************');
    done();
  });

  /*
  * @tc.number  selectionfwk_panel_moveTo_010
  * @tc.name    verify method of the panel moveTo.
  * @tc.desc    Function test
  * @tc.level   0
  */
  it('selectionfwk_panel_moveTo_010', 0, async function (done) {
    console.info('************* selectionfwk_panel_moveTo_010 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.moveTo(50, undefined);
      console.info(`selectionfwk_panel_moveTo_010 promise success`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_moveTo_010 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_moveTo_010 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_show_001
   * @tc.name    verify ERROR code 33600002 of the panel show.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_show_001', 0, async function (done) {
    console.info('************* selectionfwk_panel_show_001 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    await destroySelectionPanel(panel);
    try {
      await panel.show();
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_show_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(33600002);
    }
    console.info('************* selectionfwk_panel_show_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_destroyed_001
   * @tc.name    verify ERROR code 401 of the panel on destroyed.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_destroyed_001', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_destroyed_001*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyedTest', (info) => {
        console.info(`selectionfwk_panel_on_destroyed_001 is failed`);
        done();
      });
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_destroyed_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_destroyed_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_destroyed_002
   * @tc.name    verify ERROR code 401 of the panel on destroyed.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_destroyed_002', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_destroyed_002*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed');
      console.info(`selectionfwk_panel_on_destroyed_002 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_destroyed_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_destroyed_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_destroyed_003
   * @tc.name    Test should success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_destroyed_003', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_destroyed_003*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed', (info) => {
        console.info(`selectionfwk_panel_on_destroyed_003 callback enter`);
        done();
      });
      console.info(`selectionfwk_panel_on_destroyed_003 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_on_destroyed_003 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_destroyed_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_destroyed_004
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_destroyed_004', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_destroyed_004*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on(null, (info) => {
        console.info(`selectionfwk_panel_on_destroyed_004 callback enter`);
        done();
      });
      console.info(`selectionfwk_panel_on_destroyed_004 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_destroyed_004 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_destroyed_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_destroyed_005
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_destroyed_005', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_destroyed_005*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on(undefined, (info) => {
        console.info(`selectionfwk_panel_on_destroyed_005 callback enter`);
        done();
      });
      console.info(`selectionfwk_panel_on_destroyed_005 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_destroyed_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_destroyed_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_destroyed_006
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_destroyed_006', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_destroyed_006*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed', null);
      console.info(`selectionfwk_panel_on_destroyed_006 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_destroyed_006 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_destroyed_006 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_destroyed_007
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_destroyed_007', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_destroyed_007*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed', undefined);
      console.info(`selectionfwk_panel_on_destroyed_007 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_destroyed_007 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_destroyed_007 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_destroyed_001
   * @tc.name    verify ERROR code 401 of the panel off destroyed.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_destroyed_001', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_destroyed_001*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.off('destroyedTest', (info) => {
        console.info(`selectionfwk_panel_off_destroyed_001 is failed`);
        done();
      });
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_off_destroyed_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_destroyed_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_destroyed_002
   * @tc.name    verify ERROR code 401 of the panel on destroyed.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_destroyed_002', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_destroyed_002*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.off(() => {
        console.info(`selectionfwk_panel_off_destroyed_002 is failed`);
        expect().assertFail();
      })
    } catch (error) {
      console.info(`selectionfwk_panel_off_destroyed_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_destroyed_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_destroyed_003
   * @tc.name    Test should success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_destroyed_003', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_destroyed_003*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed', (info) => {
        console.info(`selectionfwk_panel_off_destroyed_003 callback enter`);
        done();
      });
      panel.off('destroyed');
      console.info(`selectionfwk_panel_off_destroyed_003 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_off_destroyed_003 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_destroyed_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_destroyed_004
   * @tc.name    Test reurn 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_destroyed_004', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_destroyed_004*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed', (info) => {
        console.info(`selectionfwk_panel_off_destroyed_004 callback enter`);
        done();
      });
      panel.off(null);
      console.info(`selectionfwk_panel_off_destroyed_004 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_off_destroyed_004 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_destroyed_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_destroyed_005
   * @tc.name    Test reurn 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_destroyed_005', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_destroyed_005*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed', (info) => {
        console.info(`selectionfwk_panel_off_destroyed_005 callback enter`);
        done();
      });
      panel.off(undefined);
      console.info(`selectionfwk_panel_off_destroyed_005 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_off_destroyed_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_destroyed_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_destroyed_006
   * @tc.name    Test return success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_destroyed_006', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_destroyed_006*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed', (info) => {
        console.info(`selectionfwk_panel_off_destroyed_006 callback enter`);
        done();
      });
      panel.off('destroyed', null);
      console.info(`selectionfwk_panel_off_destroyed_006 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_off_destroyed_006 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_destroyed_006 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_destroyed_007
   * @tc.name    Test return success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_destroyed_007', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_destroyed_007*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('destroyed', (info) => {
        console.info(`selectionfwk_panel_off_destroyed_007 callback enter`);
        done();
      });
      panel.off('destroyed', undefined);
      console.info(`selectionfwk_panel_off_destroyed_007 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_off_destroyed_007 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_destroyed_007 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_hidden_001
   * @tc.name    verify ERROR code 401 of the panel on hidden.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_hidden_001', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_hidden_001*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hiddenTest', (info) => {
        console.info(`selectionfwk_panel_on_hidden_001 is failed`);
        done();
      });
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_hidden_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_hidden_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_hidden_002
   * @tc.name    verify ERROR code 401 of the panel on hidden.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_hidden_002', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_hidden_002*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden');
      console.info(`selectionfwk_panel_on_hidden_002 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_hidden_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_hidden_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_hidden_003
   * @tc.name    Test should success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_hidden_003', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_hidden_003*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden', (info) => {
        console.info(`selectionfwk_panel_on_hidden_003 callback enter`);
        done();
      });
      console.info(`selectionfwk_panel_on_hidden_003 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_on_hidden_003 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_hidden_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_hidden_004
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_hidden_004', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_hidden_004*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on(null, (info) => {
        console.info(`selectionfwk_panel_on_hidden_004 callback enter`);
        done();
      });
      console.info(`selectionfwk_panel_on_hidden_004 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_hidden_004 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_hidden_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_hidden_005
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_hidden_005', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_hidden_005*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on(undefined, (info) => {
        console.info(`selectionfwk_panel_on_hidden_005 callback enter`);
        done();
      });
      console.info(`selectionfwk_panel_on_hidden_005 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_hidden_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_hidden_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_hidden_006
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_hidden_006', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_hidden_006*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden', null);
      console.info(`selectionfwk_panel_on_hidden_006 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_hidden_006 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_hidden_006 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_on_hidden_007
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_on_hidden_007', 0, async function (done) {
    console.info('************* selectionfwk_panel_on_hidden_007*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden', undefined);
      console.info(`selectionfwk_panel_on_hidden_007 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_on_hidden_007 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_on_hidden_007 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_hidden_001
   * @tc.name    verify ERROR code 401 of the panel off hidden.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_hidden_001', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_hidden_001*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.off('hiddenTest', (info) => {
        console.info(`selectionfwk_panel_off_hidden_001 is failed`);
        done();
      });
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_off_hidden_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_hidden_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_hidden_002
   * @tc.name    verify ERROR code 401 of the panel on hidden.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_hidden_002', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_hidden_002*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.off(() => {
        console.info(`selectionfwk_panel_off_hidden_002 is failed`);
        expect().assertFail();
      })
    } catch (error) {
      console.info(`selectionfwk_panel_off_hidden_002 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_hidden_002 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_hidden_003
   * @tc.name    Test should success, when parameters are valid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_hidden_003', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_hidden_003*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden', (info) => {
        console.info(`selectionfwk_panel_off_hidden_003 callback enter`);
        done();
      });
      panel.off('hidden');
      console.info(`selectionfwk_panel_off_hidden_003 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_off_hidden_003 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_hidden_003 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_hidden_004
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_hidden_004', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_hidden_004*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden', (info) => {
        console.info(`selectionfwk_panel_off_hidden_004 callback enter`);
        done();
      });
      panel.off(null);
      console.info(`selectionfwk_panel_off_hidden_004 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_off_hidden_004 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_hidden_004 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_hidden_005
   * @tc.name    Test return 401, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_hidden_005', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_hidden_005*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden', (info) => {
        console.info(`selectionfwk_panel_off_hidden_005 callback enter`);
        done();
      });
      panel.off(undefined);
      console.info(`selectionfwk_panel_off_hidden_005 is failed`);
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_off_hidden_005 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(401);
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_hidden_005 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_hidden_006
   * @tc.name    Test return success, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_hidden_006', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_hidden_006*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden', (info) => {
        console.info(`selectionfwk_panel_off_hidden_006 callback enter`);
        done();
      });
      panel.off('hidden', null);
      console.info(`selectionfwk_panel_off_hidden_006 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_off_hidden_006 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_hidden_006 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_off_hidden_007
   * @tc.name    Test return success, when parameters are invalid.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_off_hidden_007', 0, async function (done) {
    console.info('************* selectionfwk_panel_off_hidden_007*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      panel.on('hidden', (info) => {
        console.info(`selectionfwk_panel_off_hidden_007 callback enter`);
        done();
      });
      panel.off('hidden', undefined);
      console.info(`selectionfwk_panel_off_hidden_007 is success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_off_hidden_007 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_off_hidden_007 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_hide_001
   * @tc.name    verify ERROR code 33600002 of the panel hide.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_hide_001', 0, async function (done) {
    console.info('************* selectionfwk_panel_hide_001 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    await destroySelectionPanel(panel);
    try {
      await panel.hide();
      expect().assertFail();
    } catch (error) {
      console.info(`selectionfwk_panel_hide_001 throw error: ${JSON.stringify(error)}`);
      expect(error.code).assertEqual(33600002);
    }
    console.info('************* selectionfwk_panel_hide_001 Test end*************');
    done();
  });

  /*
   * @tc.number  selectionfwk_panel_hide_002
   * @tc.name    verify method of the panel hide.
   * @tc.desc    Function test
   * @tc.level   0
   */
  it('selectionfwk_panel_hide_002', 0, async function (done) {
    console.info('************* selectionfwk_panel_hide_002 Test start*************');
    if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      console.info('can not use SystemCapability.SelectionInput.Selection');
      expect(true).assertTrue();
      done();
      return;
    }
    let panel = await createSelectionPanel();
    try {
      await panel.hide();
      console.info(`selectionfwk_panel_hide_002 promise success`);
      expect(true).assertTrue();
    } catch (error) {
      console.info(`selectionfwk_panel_hide_002 throw error: ${JSON.stringify(error)}`);
      expect().assertFail();
    }
    await destroySelectionPanel(panel);
    console.info('************* selectionfwk_panel_hide_002 Test end*************');
    done();
  });
});
