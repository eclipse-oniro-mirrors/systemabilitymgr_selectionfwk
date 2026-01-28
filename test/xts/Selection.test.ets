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

'use static'
import { describe, it, expect, TestType, Size, Level, beforeAll} from "../../../hypium/index";
import hilog from '@ohos.hilog'
import Utils from './Util.test';
import { PanelInfo, PanelType } from '@ohos.selectionInput.SelectionPanel';
import selectionManager from '@ohos.selectionInput.selectionManager';
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';

// Preconditions:
// hdc shell param set sys.selection.switch on
// hdc shell param set sys.selection.app bundlename/MainAbility

let domain: int = 0x0000; //日志标识,0x0000作为测试框架的业务标识
let tag: string = 'testTag'; //日志标识字符串,作为tag标识当前runner类下的测试行为

export default function selectionTest() {

    describe("selectionTest", (): void => {
        hilog.info(domain, tag, '%{public}s', 'describe start');
        /*
         * @tc.number: selectionfwk_on_selectionCompleted_003
         * @tc.name: Test should success, when parameters are valid.
         * @tc.desc:  Function test
         * @tc.level: LEVEL0
         * @tc.size: MEDIUMTEST
         * @tc.type: FUNCTION
        */
        it("selectionfwk_on_selectionCompleted_001", TestType.FUNCTION|Size.MEDIUMTEST|Level.LEVEL0, async (done: () => void): Promise<void> => {
            console.info('************* selectionfwk_on_selectionCompleted_001*************');
            try {
                 selectionManager.onSelectionCompleted((info: selectionManager.SelectionInfo) => {
                     console.info(`selectionfwk_on_selectionCompleted_001 callback enter`);
                     expect(info != null).assertTrue();

                 });
                console.info('************* selectionfwk_on_selectionCompleted_001 Test end*************');
                done();
             } catch(error) {
                 console.error(`Failed to query device. Code is ${error.code}, message is ${error.message}`);
                 console.info('************* selectionfwk_on_selectionCompleted_001 Test end*************');
                 done();
             }
        })

        /*
         * @tc.number  selectionfwk_createPanel_002
         * @tc.name    Test should throw BusinessError 401 when parameters are invalid.
         * @tc.desc    Function test
         * @tc.level   0
        */
        it('selectionfwk_createPanel_001', TestType.FUNCTION|Size.MEDIUMTEST|Level.LEVEL0, async (done: () => void): Promise<void> => {
            console.info('************* selectionfwk_createPanel_001 Test start*************');
            let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
            abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
            let context = abilityDelegator.getAppContext();
            context.stageMode= true;
            try {
                let panelInfo: PanelInfo = {
                    panelType: PanelType.MENU_PANEL,
                    x: 0,
                    y: 0,
                    width: 100,
                    height: 100
                }
                let panel: selectionManager.Panel = await selectionManager.createPanel(context, panelInfo)
                if (panel !== undefined) {
                    await selectionManager.destroyPanel(panel);
                    expect(true).assertTrue();
                    console.info('************* selectionfwk_createPanel_001 Test end*************');
                    done();
                }
                done();
            } catch (error) {
                console.info(`selectionfwk_createPanel_001 throw error: [${error.code}, ${error.message}]`);
                expect(false).assertTrue();
                console.info('************* selectionfwk_createPanel_001 Test end*************');
                done();
            }
        });


        /*
     * @tc.number  selectionfwk_panel_setUiContent_001
     * @tc.name    verify ERROR code 401 of setUiContent.
     * @tc.desc    Function test
     * @tc.level   0
     */
        it('selectionfwk_panel_setUiContent_001', 0, async (done: () => void): Promise<void> =>  {
            console.info('************* selectionfwk_panel_setUiContent_001 Test start*************');
            let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
            abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
            let context = abilityDelegator.getAppContext();
            context.stageMode = true;
            let panelInfo: PanelInfo = {
                panelType: PanelType.MAIN_PANEL,
                x: 0,
                y: 0,
                width: 100,
                height: 100
            }

            let panel: selectionManager.Panel | undefined = undefined;
            let bDestroyed = false;
            try {
                panel = await selectionManager.createPanel(context, panelInfo);
                if (panel !== undefined) {
                    await selectionManager.destroyPanel(panel);
                    bDestroyed = true;
                    await panel.setUiContent('page1');
                    console.info(`selectionfwk_panel_setUiContent_001 promise failed`);
                    expect(false).assertTrue();
                }
            } catch (error) {
                console.info(`selectionfwk_panel_setUiContent_001 result: [${error.code}, ${error.message}]`);
                expect(error.code).assertEqual(33600002);
            } finally {
                if (!bDestroyed) {
                    if (panel !== undefined) {
                        await selectionManager.destroyPanel(panel);
                    }
                }
                console.info('************* selectionfwk_panel_setUiContent_001 Test end*************');
                done();
            }
        });

        /*
     * @tc.number  selectionfwk_panel_moveTo_001
     * @tc.name    verify function of the panel moveTo.
     * @tc.desc    Function test
     * @tc.level   0
     */
        it('selectionfwk_panel_moveTo_001', 0, async (done: () => void): Promise<void> => {
            console.info('************* selectionfwk_panel_moveTo_001 Test start*************');
            let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
            abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
            let context = abilityDelegator.getAppContext();
            context.stageMode = true;
            let panelInfo: PanelInfo = {
                panelType: PanelType.MENU_PANEL,
                x: 0,
                y: 0,
                width: 100,
                height: 100
            }
            let panel: selectionManager.Panel | undefined = undefined;
            try {
                panel = await selectionManager.createPanel(context, panelInfo);
                if (panel !== undefined) {
                    await panel.moveTo(20, 20);
                    console.info(`selectionfwk_panel_moveTo_001 promise success`);
                    await selectionManager.destroyPanel(panel);
                    panel = undefined;
                    expect(true).assertTrue();
                }
            } catch (error) {
                console.info(`selectionfwk_panel_moveTo_001 throw error: [${error.code}, ${error.message}]`);
                expect(false).assertTrue();
            } finally {
                if (panel !== undefined) {
                    await selectionManager.destroyPanel(panel);
                }
                console.info('************* selectionfwk_panel_moveTo_001 Test end*************');
                done();
            }
        });

        /*
     * @tc.number  selectionfwk_panel_moveTo_002
     * @tc.name    verify ERROR code 33600002 of the panel hide.
     * @tc.desc    Function test
     * @tc.level   0
     */
        it('selectionfwk_panel_moveTo_002', 0, async (done: () => void): Promise<void> => {
            console.info('************* selectionfwk_panel_moveTo_002 Test start*************');
            let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
            abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
            let context = abilityDelegator.getAppContext();
            context.stageMode = true;
            let panelInfo: PanelInfo = {
                panelType: PanelType.MENU_PANEL,
                x: 0,
                y: 0,
                width: 100,
                height: 100
            }
            let panel: selectionManager.Panel | undefined = undefined;
            let bDestroyed = false;
            try {
                panel = await selectionManager.createPanel(context, panelInfo);
                if (panel !== undefined) {
                    await selectionManager.destroyPanel(panel);
                    bDestroyed = true;
                    await panel.moveTo(20, 20);
                    expect(false).assertTrue();
                }
            } catch (error) {
                console.info(`selectionfwk_panel_moveTo_002 throw error: [${error.code}, ${error.message}]`);
                expect(error.code).assertEqual(33600002);
            } finally {
                if (!bDestroyed) {
                    if (panel !== undefined) {
                        await selectionManager.destroyPanel(panel);
                    }
                }
                console.info('************* selectionfwk_panel_moveTo_002 Test end*************');
                done();
            }
        });

        /*
     * @tc.number  selectionfwk_panel_hide_001
     * @tc.name    verify ERROR code 33600002 of the panel hide.
     * @tc.desc    Function test
     * @tc.level   0
     */
        it('selectionfwk_panel_hide_001', 0, async (done: () => void): Promise<void> => {
            console.info('************* selectionfwk_panel_hide_001 Test start*************');
            let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
            abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
            let context = abilityDelegator.getAppContext();
            context.stageMode = true;
            let panelInfo: PanelInfo = {
                panelType: PanelType.MENU_PANEL,
                x: 0,
                y: 0,
                width: 100,
                height: 100
            }
            let panel: selectionManager.Panel | undefined = undefined;
            let bDestroyed = false;
            try {
                panel = await selectionManager.createPanel(context, panelInfo);
                if (panel !== undefined) {
                    await selectionManager.destroyPanel(panel);
                    bDestroyed = true;
                    await panel.hide();
                    expect(false).assertTrue();
                }
            } catch (error) {
                console.info(`selectionfwk_panel_hide_001 throw error: [${error.code}, ${error.message}]`);
                expect(error.code).assertEqual(33600002);
            } finally {
                if (!bDestroyed) {
                    if (panel !== undefined) {
                        await selectionManager.destroyPanel(panel);
                    }
                }
                console.info('************* selectionfwk_panel_hide_001 Test end*************');
                done();
            }

        });

        /*
     * @tc.number  selectionfwk_panel_startmoving_001
     * @tc.name    verify ERROR code 33600002 of the panel hide.
     * @tc.desc    Function test
     * @tc.level   0
     */
        it('selectionfwk_panel_startmoving_001', 0, async (done: () => void): Promise<void> => {
            console.info('************* selectionfwk_panel_startmoving_001 Test start*************');
            let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
            abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
            let context = abilityDelegator.getAppContext();
            context.stageMode = true;
            let panelInfo: PanelInfo = {
                panelType: PanelType.MENU_PANEL,
                x: 0,
                y: 0,
                width: 100,
                height: 100
            }
            let panel: selectionManager.Panel | undefined = undefined;
            let bDestroyed = false;
            try {
                panel = await selectionManager.createPanel(context, panelInfo);
                if (panel !== undefined) {
                    await selectionManager.destroyPanel(panel);
                    bDestroyed = true;
                    await panel.startMoving();
                    expect(false).assertTrue();
                }
            } catch (error) {
                console.info(`selectionfwk_panel_startmoving_001 throw error: [${error.code}, ${error.message}]`);
                expect(error.code).assertEqual(33600002);
            } finally {
                if (!bDestroyed) {
                    if (panel !== undefined) {
                        await selectionManager.destroyPanel(panel);
                    }
                }
                console.info('************* selectionfwk_panel_startmoving_001 Test end*************');
                done();
            }

        });

        /*
     * @tc.number  selectionfwk_panel_show_001
     * @tc.name    verify ERROR code 33600002 of the panel hide.
     * @tc.desc    Function test
     * @tc.level   0
     */
        it('selectionfwk_panel_show_001', 0, async (done: () => void): Promise<void> => {
            console.info('************* selectionfwk_panel_show_001 Test start*************');
            let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
            abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
            let context = abilityDelegator.getAppContext();
            context.stageMode = true;
            let panelInfo: PanelInfo = {
                panelType: PanelType.MAIN_PANEL,
                x: 0,
                y: 0,
                width: 100,
                height: 100
            }
            let panel: selectionManager.Panel | undefined = undefined;
            let bDestroyed = false;
            try {
                panel = await selectionManager.createPanel(context, panelInfo);
                if (panel !== undefined) {
                    await selectionManager.destroyPanel(panel);
                    bDestroyed = true;
                    await panel.show();
                    expect(false).assertTrue();
                }

            } catch (error) {
                console.info(`selectionfwk_panel_show_001 throw error: [${error.code}, ${error.message}]`);
                expect(error.code).assertEqual(33600002);
            } finally {
                if (!bDestroyed) {
                    if (panel !== undefined) {
                        await selectionManager.destroyPanel(panel);
                    }
                }
                console.info('************* selectionfwk_panel_show_001 Test end*************');
                done();
            }

        });

        /*
     * @tc.number  selectionfwk_destroyPanel_001
     * @tc.name    verify method of destroyPanel.
     * @tc.desc    Function test
     * @tc.level   0
     */
        it('selectionfwk_destroyPanel_001', 0, async (done: () => void): Promise<void> => {
            console.info('************* selectionfwk_destroyPanel_001 Test start*************');
            let abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;
            abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
            let context = abilityDelegator.getAppContext();
            context.stageMode = true;
            let panelInfo: PanelInfo = {
                panelType: PanelType.MAIN_PANEL,
                x: 0,
                y: 0,
                width: 100,
                height: 100
            }
            try {
                let panel: selectionManager.Panel = await selectionManager.createPanel(context, panelInfo);
                if (panel !== undefined) {
                    await selectionManager.destroyPanel(panel);
                    console.info(`selectionfwk_destroyPanel_001 promise success`);
                    expect(true).assertTrue();
                    console.info('************* selectionfwk_destroyPanel_001 Test end*************');
                    done();
                }
            } catch (error) {
                console.info(`selectionfwk_destroyPanel_001 throw error: [${error.code}, ${error.message}]`);
                expect(false).assertTrue();
                console.info('************* selectionfwk_destroyPanel_001 Test end*************');
                done();
            }
        });
    })

}