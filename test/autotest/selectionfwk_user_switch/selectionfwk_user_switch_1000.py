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


class selectionfwk_user_switch_1000(TestCase):
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
        Step("多用户下，关闭划词开关，划词扩展停止运行")
        Step("执行操作")

        Step("步骤1：用户A关闭划词开关")
        driver = UiDriver.create(self.device1)
        process_result = driver.shell("param set sys.selection.switch off")
        driver.Assert.match_regexp(process_result, r"^(?!.*selection_service).*")
        driver.Assert.match_regexp(process_result, r"^(?!.*com.hxxxxi.hxxs.vassistant).*")

        Step("步骤2、切换到用户B")

        driver.User.switch(user_id=106)
        driver.touch((100, 200))

        Step("步骤1：用户A关闭划词开关")
        driver = UiDriver.create(self.device1)
        process_result = driver.shell("param set sys.selection.switch off")
        driver.Assert.match_regexp(process_result, r"^(?!.*selection_service).*")
        driver.Assert.match_regexp(process_result, r"^(?!.*com.selection.selectionapplication).*")

        Step("步骤3、切换回用户A")

        driver.User.switch(user_id=100)
        driver.touch((100, 200))

        Step("收尾工作")