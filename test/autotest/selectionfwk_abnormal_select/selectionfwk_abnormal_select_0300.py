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


class selectionfwk_abnormal_select_0300(TestCase):
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
        Step("验证选中超过6000字符不弹窗（1个汉字对应3个字符）")
        Step("执行操作")
        driver = UiDriver.create(self.device1)

        Step("步骤1:查询划词进程和小艺扩展是否存在")
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.xxos.vassistant:selection")

        Step("步骤2:打开备忘录")
        driver.start_app("com.hxxxxi.xxos.notepad")

        Step("步骤3:点击同意隐私协议")
        component = driver.find_component(BY.text("同意"))
        if component:
            driver.touch(BY.text("同意"))

        Step("步骤4:新建备忘录")
        driver.touch(BY.key("createNoteTop"))

        Step("步骤5:点击放大备忘录")
        display_width, display_height = driver.get_display_size()
        window_width, window_height = driver.get_window_size()
        if display_width != window_width:
            driver.touch(BY.key("EnhanceMaximizeBtn"))

        Step("步骤6:输入文字")
        for i in range(100):
            driver.input_text_by_keyboard("验证超六千字符不弹窗验证超六千字符不弹窗")

        Step("步骤7:尝试选中文字")
        driver.mouse_drag((1068, 386), (3071, 1866))

        Step("步骤8:按下ctrl")
        driver.press_key(key_code=2072)

        Step("步骤9:检查是否成功弹窗")
        driver.check_window_exist(WindowFilter().bundle_name("com.hxxxxi.xxos.vassistant"))
        driver.wait(1)

        Step("步骤10:聚焦到文本，取消高亮")
        driver.touch((1100, 1849))

        Step("步骤10:输入文字致使超过6000字符")
        driver.input_text_by_keyboard("a")

        Step("步骤11:重新划词，验证是否会弹窗")
        driver.mouse_drag((1068, 386), (3071, 1866))
        driver.check_window_exist(WindowFilter().bundle_name("com.hxxxxi.xxos.vassistant"), expect_exist=False)
        driver.wait(1)

        Step("步骤10:关闭备忘录")
        driver.stop_app("com.hxxxxi.xxos.notepad")

    def teardown(self):
        Step("收尾工作")