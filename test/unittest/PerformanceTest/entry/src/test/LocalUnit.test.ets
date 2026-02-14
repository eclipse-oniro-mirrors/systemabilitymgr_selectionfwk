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

import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { selectionManager, PanelInfo, PanelType, SelectionExtensionAbility } from '@kit.BasicServicesKit';
import systemDateTime from '@ohos.systemDateTime';
import { abilityDelegatorRegistry } from '@kit.TestKit';

// Preconditions:
// hdc shell param set sys.selection.switch on
// hdc shell param set sys.selection.app com.selection.selectionapplication/SelectionExtAbility

let standardTime = 10 * 1000 * 1000;
export default function localUnitTest() {
  describe('localUnitTest', () => {
    // Defines a test suite. Two parameters are supported: test suite name and test suite function.
    beforeAll(() => {
      // Presets an action, which is performed only once before all test cases of the test suite start.
      // This API supports only one parameter: preset action function.
    });
    beforeEach(() => {
      // Presets an action, which is performed before each unit test case starts.
      // The number of execution times is the same as the number of test cases defined by **it**.
      // This API supports only one parameter: preset action function.
    });
    afterEach(() => {
      // Presets a clear action, which is performed after each unit test case ends.
      // The number of execution times is the same as the number of test cases defined by **it**.
      // This API supports only one parameter: clear action function.
    });
    afterAll(() => {
      // Presets a clear action, which is performed after all test cases of the test suite end.
      // This API supports only one parameter: clear action function.
    });
    /*
     * @tc.number  selectionfwk_performance_on_selectionCompleted_001
     * @tc.name    Test performance of on('selectionCompleted', callback).
     * @tc.desc    Function test
     * @tc.level   0
     */
    it('selectionfwk_performance_on_off_selectionCompleted_001', 0, async() => {
      console.info('************* selectionfwk_performance_on_off_selectionCompleted_001*************');
      // if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      //   console.info('can not use SystemCapability.SelectionInput.Selection');
      //   expect(true).assertTrue();
      //   return;
      // }

      let startTimeNs = 0;
      let endTimeNs = 0;
      let timeSumOfOn = 0;
      let counterOfOn = 0;
      let timeSumOfOff = 0;
      let counterOfOff = 0;
      let callback = (info: selectionManager.SelectionInfo) => {
        console.info('selectionfwk_performance_on_off_selectionCompleted_001: callback');
      };
      for (let i = 0; i < 300; i++) {
        try {
          startTimeNs = systemDateTime.getTime(true);
          selectionManager.on('selectionCompleted', callback);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfOn += endTimeNs - startTimeNs;
          counterOfOn++;

          startTimeNs = systemDateTime.getTime(true);
          selectionManager.off('selectionCompleted', callback);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfOff += endTimeNs - startTimeNs;
          counterOfOff++;
        } catch (error) {
          console.info(`selectionfwk_performance_on_off_selectionCompleted_001 throw error: ${JSON.stringify(error)}`);
        }
      }

      let averageTimeOfOn = timeSumOfOn/counterOfOn;
      let averageTimeOfOff = timeSumOfOff/counterOfOff;
      expect(averageTimeOfOn <= standardTime && averageTimeOfOff <= standardTime).assertTrue();

      console.info(`averageTime: On:${averageTimeOfOn}ns, Off:${averageTimeOfOff}ns`);
      console.info('************* selectionfwk_performance_on_off_selectionCompleted_001 Test end*************');
    });

    /*
   * @tc.number  selectionfwk_performance_create_destroy_panel_001
   * @tc.name    Test performance of createpanel and destroypanel.
   * @tc.desc    Function test
   * @tc.level   0
   */
    it('selectionfwk_performance_create_destroy_panel_001', 0, async () => {
      console.info('************* selectionfwk_performance_create_destroy_panel_001*************');
      // if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      //   console.info('can not use SystemCapability.SelectionInput.Selection');
      //   expect(true).assertTrue();
      //   return;
      // }
      let panelInfo: PanelInfo = {
        panelType: PanelType.MENU_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
      abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
      let context = abilityDelegator.getAppContext();
      context.stageMode = true;
      let startTimeNs = 0;
      let endTimeNs = 0;
      let timeSumOfCreate = 0;
      let counterOfCreate = 0;
      let timeSumOfDestroy = 0;
      let counterOfDestroy = 0;

      for (let i = 0; i < 300; i++) {
        try {
          startTimeNs = systemDateTime.getTime(true);
          let panelTemp = await selectionManager.createPanel(context, panelInfo);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfCreate += endTimeNs - startTimeNs;
          counterOfCreate++;
          console.info(`testapitime---${i}, createpanel: ${endTimeNs - startTimeNs}`);

          startTimeNs = systemDateTime.getTime(true);
          await selectionManager.destroyPanel(panelTemp);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfDestroy += endTimeNs - startTimeNs;
          counterOfDestroy++;
          console.info(`testapitime---${i}, createpanel: ${endTimeNs - startTimeNs}`);
        } catch (error) {
          console.info(`selectionfwk_performance_create_destroy_panel_001 throw error: ${JSON.stringify(error)}`);
        }
      }

      let averageTimeOfCreate = timeSumOfCreate/counterOfCreate;
      let averageTimeOfDestroy = timeSumOfDestroy/counterOfDestroy;
      expect(averageTimeOfCreate <= standardTime && averageTimeOfDestroy <= standardTime).assertTrue();

      console.info(`averageTime: CreatePanel:${averageTimeOfCreate}ns, DestroyPanel:${averageTimeOfDestroy}ns`);
      console.info('************* selectionfwk_performance_create_destroy_panel_001 Test end*************');
    });

    /*
     * @tc.number  selectionfwk_performance_on_off_destroyed_001
     * @tc.name    Test performance of on('destroyed', callback) and off('destroyed', callback).
     * @tc.desc    Function test
     * @tc.level   0
     */
    it('selectionfwk_performance_on_off_destroyed_001', 0, async () => {
      console.info('************* selectionfwk_performance_on_off_destroyed_001*************');
      // if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      //   console.info('can not use SystemCapability.SelectionInput.Selection');
      //   expect(true).assertTrue();
      //   return;
      // }

      let panelInfo: PanelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
      abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
      let context = abilityDelegator.getAppContext();
      context.stageMode = true;
      let panelTemp = await selectionManager.createPanel(context, panelInfo);
      let startTimeNs = 0;
      let endTimeNs = 0;
      let timeSumOfOn = 0;
      let counterOfOn = 0;
      let timeSumOfOff = 0;
      let counterOfOff = 0;
      let callback = () => {
        console.info('selectionfwk_performance_on_off_destroyed_001: callback');
      };
      for (let i = 0; i < 300; i++) {
        try {
          startTimeNs = systemDateTime.getTime(true);
          panelTemp.on('destroyed', callback);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfOn += endTimeNs - startTimeNs;
          counterOfOn++;

          startTimeNs = systemDateTime.getTime(true);
          panelTemp.off('destroyed', callback);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfOff += endTimeNs - startTimeNs;
          counterOfOff++;
        } catch (error) {
          console.info(`selectionfwk_performance_on_off_destroyed_001 throw error: ${JSON.stringify(error)}`);
        }
      }

      let averageTimeOfOn = timeSumOfOn/counterOfOn;
      let averageTimeOfOff = timeSumOfOff/counterOfOff;
      expect(averageTimeOfOn <= standardTime && averageTimeOfOff <= standardTime).assertTrue();

      await selectionManager.destroyPanel(panelTemp);
      console.info(`averageTime: on_destoryed:${averageTimeOfOn}ns, off_destoryed:${averageTimeOfOff}ns`);
      console.info('************* selectionfwk_performance_on_off_destroyed_001 Test end*************');
    });

    /*
     * @tc.number  selectionfwk_performance_on_off_hidden_001
     * @tc.name    Test performance of on('hidden', callback) and off('hidden', callback).
     * @tc.desc    Function test
     * @tc.level   0
     */
    it('selectionfwk_performance_on_off_hidden_001', 0, async () => {
      console.info('************* selectionfwk_performance_on_off_hidden_001*************');
      // if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      //   console.info('can not use SystemCapability.SelectionInput.Selection');
      //   expect(true).assertTrue();
      //   return;
      // }

      let panelInfo: PanelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
      abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
      let context = abilityDelegator.getAppContext();
      context.stageMode = true;
      let panelTemp = await selectionManager.createPanel(context, panelInfo);
      let startTimeNs = 0;
      let endTimeNs = 0;
      let timeSumOfOn = 0;
      let counterOfOn = 0;
      let timeSumOfOff = 0;
      let counterOfOff = 0;
      let callback = () => {
        console.info('selectionfwk_performance_on_off_hidden_001: callback');
      };

      for (let i = 0; i < 300; i++) {
        try {
          startTimeNs = systemDateTime.getTime(true);
          panelTemp.on('hidden', callback);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfOn += endTimeNs - startTimeNs;
          counterOfOn++;

          startTimeNs = systemDateTime.getTime(true);
          panelTemp.off('hidden', callback);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfOff += endTimeNs - startTimeNs;
          counterOfOff++;
        } catch (error) {
          console.info(`selectionfwk_performance_on_off_hidden_001 throw error: ${JSON.stringify(error)}`);
        }
      }

      let averageTimeOfOn = timeSumOfOn/counterOfOn;
      let averageTimeOfOff = timeSumOfOff/counterOfOff;
      expect(averageTimeOfOn <= standardTime && averageTimeOfOff <= standardTime).assertTrue();

      await selectionManager.destroyPanel(panelTemp);
      console.info(`averageTime: on_hidden:${averageTimeOfOn}ns, off_hidden:${averageTimeOfOff}ns`);
      console.info('************* selectionfwk_performance_on_off_hidden_001 Test end*************');
    });

    /*
     * @tc.number  selectionfwk_performance_hide_panel_001
     * @tc.name    Test performance of hide().
     * @tc.desc    Function test
     * @tc.level   0
     */
    it('selectionfwk_performance_hide_panel_001', 0, async () => {
      console.info('************* selectionfwk_performance_hide_panel_001*************');
      // if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      //   console.info('can not use SystemCapability.SelectionInput.Selection');
      //   expect(true).assertTrue();
      //   return;
      // }

      let panelInfo: PanelInfo = {
        panelType: PanelType.MENU_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
      abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
      let context = abilityDelegator.getAppContext();
      context.stageMode = true;
      let panelTemp = await selectionManager.createPanel(context, panelInfo);
      let startTimeNs = 0;
      let endTimeNs = 0;
      let timeSumOfHide = 0;
      let counterOfHide = 0;

      for (let i = 0; i < 300; i++) {
        try {
          startTimeNs = systemDateTime.getTime(true);
          await panelTemp.hide();
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfHide += endTimeNs - startTimeNs;
          counterOfHide++;
        } catch (error) {
          console.info(`selectionfwk_performance_hide_panel_001 throw error: ${JSON.stringify(error)}`);
        }
      }

      let averageTimeOfHide = timeSumOfHide/counterOfHide;
      expect(averageTimeOfHide <= standardTime).assertTrue();

      await selectionManager.destroyPanel(panelTemp);
      console.info(`averageTime: hide:${averageTimeOfHide}ns`);
      console.info('************* selectionfwk_performance_hide_panel_001 Test end*************');
    });

    /*
     * @tc.number  selectionfwk_performance_move_001
     * @tc.name    Test performance of movetoglobaldisplay.
     * @tc.desc    Function test
     * @tc.level   0
     */
    it('selectionfwk_performance_move_001', 0, async () => {
      console.info('************* selectionfwk_performance_move_001*************');
      // if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      //   console.info('can not use SystemCapability.SelectionInput.Selection');
      //   expect(true).assertTrue();
      //   return;
      // }

      let panelInfo: PanelInfo = {
        panelType: PanelType.MAIN_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
      abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
      let context = abilityDelegator.getAppContext();
      context.stageMode = true;
      let panelTemp = await selectionManager.createPanel(context, panelInfo);
      let startTimeNs = 0;
      let endTimeNs = 0;
      let timeSumOfStartMoving = 0;
      let counterOfStartMoving = 0;
      let timeSumOfMoveTo = 0;
      let counterOfMoveTo = 0;

      for (let i = 0; i < 300; i++) {
        try {
          startTimeNs = systemDateTime.getTime(true);
          await panelTemp.startMoving();
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfStartMoving += endTimeNs - startTimeNs;
          counterOfStartMoving++;

          startTimeNs = systemDateTime.getTime(true);
          await panelTemp.moveToGlobalDisplay(1, 1);
          endTimeNs = systemDateTime.getTime(true);
          timeSumOfMoveTo += endTimeNs - startTimeNs;
          counterOfMoveTo++;
        } catch (error) {
          console.info(`selectionfwk_performance_move_001 throw error: ${JSON.stringify(error)}`);
        }
      }

      let avgrageTimeOfStartMoving = timeSumOfStartMoving/counterOfStartMoving;
      let averageTimeOfMoveTo = timeSumOfMoveTo/counterOfMoveTo;
      expect(avgrageTimeOfStartMoving <= standardTime && averageTimeOfMoveTo <= standardTime).assertTrue();

      await selectionManager.destroyPanel(panelTemp);
      console.info(`averageTime: startmoving:${avgrageTimeOfStartMoving}ns, movetoglobaldisplay:${averageTimeOfMoveTo}ns`);
      console.info('************* selectionfwk_performance_move_001 Test end*************');
    });

    /*
     * @tc.number  selectionfwk_performance_setuicontent_show_001
     * @tc.name    Test performance of setuicontent() and show().
     * @tc.desc    Function test
     * @tc.level   0
     */
    it('selectionfwk_performance_setuicontent_show_001', 0, async () => {
      console.info('************* selectionfwk_performance_setuicontent_show_001*************');
      // if (!canIUse('SystemCapability.SelectionInput.Selection')) {
      //   console.info('can not use SystemCapability.SelectionInput.Selection');
      //   expect(true).assertTrue();
      //   return;
      // }

      let panelInfo: PanelInfo = {
        panelType: PanelType.MENU_PANEL,
        x: 0,
        y: 0,
        width: 100,
        height: 100
      }
      let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
      abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
      let context = abilityDelegator.getAppContext();
      context.stageMode = true;
      let panelTemp = await selectionManager.createPanel(context, panelInfo);
      let startTimeNs = 0;
      let endTimeNs = 0;
      let timeSumOfSetUiContent = 0;
      let counterOfSetUiContent = 0;
      let timeSumOfShow = 0;
      let counterOfShow = 0;

      for (let i = 0; i < 300; i++) {
        try {
          startTimeNs = systemDateTime.getTime(true);
          await panelTemp.setUiContent('page/Index');
          endTimeNs = systemDateTime.getTime(true);
        } catch (error) {
          endTimeNs = systemDateTime.getTime(true);
          console.info(`selectionfwk_performance_setuicontent_show_001 throw error: ${error}`);
        }
        timeSumOfSetUiContent += endTimeNs - startTimeNs;
        counterOfSetUiContent++;

        try {
          startTimeNs = systemDateTime.getTime(true);
          await panelTemp.show();
          endTimeNs = systemDateTime.getTime(true);
        } catch (error) {
          endTimeNs = systemDateTime.getTime(true);
          console.info(`selectionfwk_performance_setuicontent_show_001 throw error: ${error}`);
        }
        timeSumOfShow += endTimeNs - startTimeNs;
        counterOfShow++;
      }

      let averageTimeOfSetUiContent = timeSumOfSetUiContent/counterOfSetUiContent;
      let averageTimeOfShow = timeSumOfShow/counterOfShow;
      expect(averageTimeOfSetUiContent <= standardTime && averageTimeOfShow <= standardTime).assertTrue();

      await selectionManager.destroyPanel(panelTemp);
      console.info(`averageTime: setuicontent:${averageTimeOfSetUiContent}ns, show:${averageTimeOfShow}ns`);
      console.info('************* selectionfwk_performance_setuicontent_show_001 Test end*************');
    });
  });
}