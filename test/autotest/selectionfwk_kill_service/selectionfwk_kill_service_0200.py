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


class selectionfwk_kill_service_0200(TestCase):
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
        Step("kill掉划词进程，验证是否重新拉起")
        Step("执行操作")
        driver = UiDriver.create(self.device1)

        Step("步骤1:查询划词进程和小艺扩展是否存在")
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.xxos.vassistant:selection")

        Step("步骤2:获取划词进程的PID")
        selection_pid = 0
        for line in process_result.strip().split('\n'):
            line = line.strip()
            if not line:
                continue
            if "selection_service" in line:
                # 提取第二个字段（PID）
                parts = line.split()
                if len(parts) >= 2:
                    selection_pid = int(parts[1])

        Step("步骤3:杀掉划词进程")
        driver.shell("kill %d" % selection_pid)

        Step("步骤4:验证是否重新拉起")
        driver.wait(2)
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.xxos.vassistant:selection")

        new_pid = 0
        for line in process_result.strip().split('\n'):
            line = line.strip()
            if not line:
                continue
            if "selection_service" in line:
                # 提取第二个字段（PID）
                parts = line.split()
                if len(parts) >= 2:
                    new_pid = int(parts[1])

        driver.Assert.not_equal(selection_pid, new_pid)

        Step("步骤5：验证划词弹窗")
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
            driver.input_text_by_keyboard("kill掉划词进程，验证是否重新拉起")

        driver.mouse_drag((1068, 386), (3063, 1710))
        driver.press_key(key_code=2072)
        driver.check_window_exist(WindowFilter().bundle_name("com.hxxxxi.xxos.vassistant"))
        driver.wait(1)
        driver.stop_app("com.hxxxxi.xxos.notepad")

    def teardown(self):
        Step("收尾工作")