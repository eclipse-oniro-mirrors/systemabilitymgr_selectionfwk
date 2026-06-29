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


class selectionfwk_switch_0500(TestCase):
    def __init__(self, configs):
        self.TAG = self.__class__.__name__
        TestCase.__init__(self, self.TAG, configs)

    def setup(self):
        Step("预置工作:初始化2in1开始................." + self.devices[0].device_sn)
        driver = UiDriver.create(self.device1)
        wake = driver.Screen.is_on()
        time.sleep(0.5)
        if wake:
            driver.ScreenLock.unlock()
        else:
            driver.Screen.wake_up()
            driver.ScreenLock.unlock()
        driver.shell("power-shell timeout -o 86400000")

    def process(self):
        Step("测试设置开关划词服务")
        Step("执行操作")
        driver = UiDriver.create(self.device1)

        Step("步骤1:打开设置")
        driver.start_app("com.hxxxxi.xxos.settings")

        Step("步骤2:搜索智慧划词")
        driver.touch(BY.key("SettingsHome_PCSearch"))
        driver.input_text_by_keyboard("划词")
        driver.touch(BY.text("智慧划词"))

        Step("步骤3:关闭智慧划词")
        driver.touch(BY.key("Setting.system_select_words.system_words_switch_group.selection_service_switch.result"))
        driver.wait(20)

        Step("步骤4:验证是否关闭成功")
        process_result = driver.shell("ps -ef | grep selection")
        keyword1 = "selection_service"
        keyword2 = "com.hxxxxi.xxos.vassistant:selection"
        is_exist = False
        if keyword1 in process_result or keyword2 in process_result:
            is_exist = True
        else:
            is_exist = False
        driver.Assert.equal(is_exist, False)

        Step("步骤5:重新打开划词开关")
        driver.touch(BY.key("Setting.system_select_words.system_words_switch_group.selection_service_switch.result"))
        driver.wait(2)

        Step("步骤6:验证是否成功开启")
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.xxos.vassistant:selection")
        driver.stop_app("com.hxxxxi.xxos.settings")

        Step("步骤7：验证划词弹窗")
        driver.start_app("com.hxxxxi.xxos.notepad")
        component = driver.find_component(BY.text("同意"))
        if component:
            driver.touch(BY.text("同意"))

        display_width, display_height = driver.get_display_size()
        window_width, window_height = driver.get_window_size()
        if display_width != window_width:
            driver.touch(BY.key("EnhanceMaximizeBtn"))

        driver.touch(BY.key("createNoteTop"))
        for i in range(2):
            driver.input_text_by_keyboard("自动化测试123abc")

        driver.mouse_drag((1068, 386), (3063, 1710))
        driver.press_key(key_code=2072)
        driver.check_window_exist(WindowFilter().bundle_name("com.hxxxxi.xxos.vassistant"))
        driver.wait(1)
        driver.stop_app("com.hxxxxi.xxos.notepad")

    def teardown(self):
        Step("收尾工作")