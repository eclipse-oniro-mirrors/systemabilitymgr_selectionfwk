#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from devicetest.core.test_case import TestCase, Step
from devicetest.utils.file_util import get_resource_path
from hypium import *
from hypium.uiexplorer import *


class selectionfwk_user_switch_0100(TestCase):
    def __init__(self, configs):
        self.TAG = self.__class__.__name__
        TestCase.__init__(self, self.TAG, configs)

    def setup(self):
        Step("预置工作:初始化手机开始................." + self.devices[0].device_sn)
        driver = UiDriver.create(self.device1)
        # 解锁屏幕
        wake = driver.Screen.is_on()
        time.sleep(0.5)
        if wake:
            driver.touch((100, 200))
        else:
            driver.Screen.wake_up()
            driver.touch((100, 200))
        driver.shell("power-shell timeout -o 86400000")
    def process(self):
        Step("验证A用户和B用户来回切换，能够保留自身之前的设置")
        Step("执行操作")

        Step("步骤1：用户A设置为ctrl触发、划词开、扩展进程为小艺")
        driver = UiDriver.create(self.device1)
        driver.shell("param set sys.selection.switch on")
        driver.shell("param set sys.selection.trigger ctrl")
        #param set sys.selection.trigger "immediate"
        driver.shell("param set sys.selection.app com.hxxxxi.hxxs.vassistant/MiniMenuServiceExtAbility")
        #com.selection.selectionapplication/SelectionExtensionAbility

        Step("查看划词服务进程")
        Step("查询划词进程和小艺扩展是否存在")
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.hxxs.vassistant:selection")

        Step("查询当前的触发方式")
        trigger_result = driver.shell("param get | grep selection")
        driver.Assert.contains(trigger_result, "ctrl")
        driver.Assert.contains(trigger_result, "on")

        # 切换到id为101的用户

        Step("步骤2、切换到用户B")

        driver.User.switch(user_id=106)
        driver.touch((100, 200))

        Step("用户A设置为直接触发、划词开、扩展进程为小艺")
        driver = UiDriver.create(self.device1)
        driver.shell("param set sys.selection.switch on")
        driver.shell("param set sys.selection.trigger immediate")
        #param set sys.selection.trigger "immediate"
        driver.shell("param set sys.selection.app com.hxxxxi.hxxs.vassistant/MiniMenuServiceExtAbility")
        #com.selection.selectionapplication/SelectionExtensionAbility

        Step("查看划词服务进程")
        Step("查询划词进程和小艺扩展是否存在")
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.hxxs.vassistant:selection")

        Step("查询当前的触发方式")
        trigger_result = driver.shell("param get | grep selection")
        driver.Assert.contains(trigger_result, "immediate")
        driver.Assert.contains(trigger_result, "on")

        Step("步骤3、切换回用户A")

        driver.User.switch(user_id=100)
        driver.touch((100, 200))

        Step("查看划词服务进程")
        Step("查询划词进程和小艺扩展是否存在")
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.hxxs.vassistant:selection")

        Step("查询当前的触发方式")
        trigger_result = driver.shell("param get | grep selection")
        driver.Assert.contains(trigger_result, "ctrl")
        driver.Assert.contains(trigger_result, "on")
    def teardown(self):
        Step("收尾工作")