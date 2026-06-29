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


class selectionfwk_lock_screen_0100(TestCase):
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
        Step("验证锁屏时划词服务是否正常")
        Step("执行操作")
        driver = UiDriver.create(self.device1)

        Step("步骤1:查询划词进程和小艺扩展是否存在")
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.xxos.vassistant:selection")

        Step("步骤2:获取进程的PID")
        extension_pid = 0
        selection_pid = 0
        for line in process_result.strip().split('\n'):
            line = line.strip()
            if not line:
                continue
            if "com.hxxxxi.xxos.vassistant:selection" in line:
                # 提取第二个字段（PID）
                parts = line.split()
                if len(parts) >= 2:
                    extension_pid = int(parts[1])
            if "selection_service" in line:
                # 提取第二个字段（PID）
                parts = line.split()
                if len(parts) >= 2:
                    selection_pid = int(parts[1])

        Step("步骤3:锁屏")
        driver.ScreenLock.lock()

        Step("步骤4:查询划词进程和小艺扩展是否存在")
        process_result = driver.shell("ps -ef | grep selection")
        driver.Assert.contains(process_result, "selection_service")
        driver.Assert.contains(process_result, "com.hxxxxi.xxos.vassistant:selection")

        Step("步骤5:查询进程号是否相同")
        new_extension_pid = 0
        new_selection_pid = 0
        for line in process_result.strip().split('\n'):
            line = line.strip()
            if not line:
                continue
            if "com.hxxxxi.xxos.vassistant:selection" in line:
                # 提取第二个字段（PID）
                parts = line.split()
                if len(parts) >= 2:
                    new_extension_pid = int(parts[1])
            if "selection_service" in line:
                # 提取第二个字段（PID）
                parts = line.split()
                if len(parts) >= 2:
                    new_selection_pid = int(parts[1])

        driver.Assert.equal(extension_pid, new_extension_pid)
        driver.Assert.equal(selection_pid, new_selection_pid)

        Step("步骤6:解锁")
        driver.ScreenLock.unlock()

    def teardown(self):
        Step("收尾工作")