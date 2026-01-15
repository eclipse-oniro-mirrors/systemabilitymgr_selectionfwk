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

#include <optional>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "common_event_support.h"
#include "db_selection_config_repository.h"
#include "parameter.h"
#include "selection_config.h"
#include "iselection_listener.h"
#include "iservice_registry.h"
#include "selection_service.h"
#include "selection_app_validator.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace SelectionFwk {

using namespace testing::ext;
using ::testing::Return;
using ::testing::Mock;

class MockSelectionService : public SelectionService {
public:
    MOCK_METHOD(bool, CheckUserLoggedIn, (), (override));
};

class SelectionServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    sptr<ISelectionListener> listener_;

private:
    sptr<IRemoteObject> GetSelectionSystemAbility();
};

void SelectionServiceTest::SetUpTestCase()
{
    std::cout << "SelectionServiceTest SetUpTestCase" << std::endl;
}

void SelectionServiceTest::TearDownTestCase()
{
    std::cout << "SelectionServiceTest TearDownTestCase" << std::endl;
}

void SelectionServiceTest::SetUp()
{
    std::cout << "SelectionServiceTest SetUp" << std::endl;
}

void SelectionServiceTest::TearDown()
{
    std::cout << "SelectionServiceTest TearDown" << std::endl;
}

sptr<IRemoteObject> SelectionServiceTest::GetSelectionSystemAbility()
{
    std::cout << "SelectionServiceTest GetSelectionSystemAbility" << std::endl;
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        std::cerr << "SelectionServiceTest systemAbilityManager == nullptr" << std::endl;
        return nullptr;
    }

    sptr<IRemoteObject> systemAbility = nullptr;
    systemAbility = systemAbilityManager->GetSystemAbility(SELECTION_FWK_SA_ID);
    if (systemAbility == nullptr) {
        std::cerr << "SelectionServiceTest systemAbility == nullptr" << std::endl;
        return nullptr;
    }

    return systemAbility;
}

/**
 * @tc.name: SelectionService001
 * @tc.desc: test ConnectNewExtAbility
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService001, TestSize.Level0)
{
    SelectionService::GetInstance()->DisconnectCurrentExtAbility();
    int32_t ret = SelectionService::GetInstance()->ConnectNewExtAbility("a", "b");
    ASSERT_NE(ret, 0);
    SelectionService::GetInstance()->DisconnectCurrentExtAbility();
}

/**
 * @tc.name: SelectionService002
 * @tc.desc: test ReconnectExtAbility
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService002, TestSize.Level0)
{
    SelectionService::GetInstance()->DisconnectCurrentExtAbility();
    int32_t ret = SelectionService::GetInstance()->ReconnectExtAbility("a", "b");
    ASSERT_EQ(ret, 0);
    SelectionService::GetInstance()->DisconnectCurrentExtAbility();
}

/**
 * @tc.name: SelectionService003
 * @tc.desc: test HandleCommonEvent
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService003, TestSize.Level0)
{
    AAFwk::Want want;
    want.SetElementName("com.selection.selectionapplication", "SelectionExtensionAbility");
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
    EventFwk::CommonEventData data;
    data.SetWant(want);
    SelectionService::GetInstance()->HandleCommonEvent(data);
    bool flag = SelectionService::GetInstance()->GetScreenLockedFlag();
    ASSERT_TRUE(flag);

    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    data.SetWant(want);
    SelectionService::GetInstance()->HandleCommonEvent(data);

    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED);
    data.SetWant(want);
    SelectionService::GetInstance()->HandleCommonEvent(data);

    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED);
    data.SetWant(want);
    SelectionService::GetInstance()->HandleCommonEvent(data);

    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    data.SetWant(want);
    SelectionService::GetInstance()->HandleCommonEvent(data);
    flag = SelectionService::GetInstance()->GetScreenLockedFlag();
    ASSERT_FALSE(flag);
}

/**
 * @tc.name: SelectionService004
 * @tc.desc: test WatchExtAbilityInstalled
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService004, TestSize.Level0)
{
    auto obj = SelectionService::GetInstance();
    ASSERT_NE(obj, nullptr);
    MemSelectionConfig::GetInstance().SetApplicationInfo("a");
    SelectionService::GetInstance()->WatchExtAbilityInstalled("a", "b");

    const std::string applicationInfo("com.selection.selectionapplication/SelectionExtensionAbility");
    MemSelectionConfig::GetInstance().SetApplicationInfo(applicationInfo);
    SelectionService::GetInstance()->WatchExtAbilityInstalled(
        "com.selection.selectionapplication", "SelectionExtensionAbility");
    std::string appInfo = MemSelectionConfig::GetInstance().GetApplicationInfo();
    ASSERT_EQ(appInfo, applicationInfo);
    SelectionService::GetInstance()->WatchExtAbilityInstalled("a", "b");
}

/**
 * @tc.name: SelectionService005
 * @tc.desc: test RegisterListener
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService005, TestSize.Level0)
{
    auto remote = GetSelectionSystemAbility();
    sptr<ISelectionListener> listener = iface_cast<ISelectionListener>(remote);
    ASSERT_NE(listener, nullptr);
    listener_ = listener;

    int32_t ret = SelectionService::GetInstance()->RegisterListener(listener);
    ASSERT_EQ(ret, 2);
}

/**
 * @tc.name: SelectionService006
 * @tc.desc: test UnregisterListener
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService006, TestSize.Level0)
{
    int ret = SelectionService::GetInstance()->UnregisterListener(listener_);
    ASSERT_EQ(ret, 0);
    auto listener = SelectionService::GetInstance()->GetListener();
    ASSERT_EQ(listener, nullptr);
}

/**
 * @tc.name: SelectionService007
 * @tc.desc: test IsCurrentSelectionApp
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService007, TestSize.Level0)
{
    bool resultValue;
    ASSERT_EQ(SelectionService::GetInstance()->IsCurrentSelectionApp(101, resultValue), 0);
}

/**
 * @tc.name: SelectionService008
 * @tc.desc: test Dump Other
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService008, TestSize.Level0)
{
    std::vector<std::u16string> args = {u"-x"};
    int fd = 1;
    ASSERT_EQ(SelectionService::GetInstance()->Dump(fd, args), 0);

    std::vector<std::u16string> args2 = {u"-x", u"-b"};
    ASSERT_EQ(SelectionService::GetInstance()->Dump(fd, args2), 0);
}

/**
 * @tc.name: SelectionService009
 * @tc.desc: test Dump HELP
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService009, TestSize.Level0)
{
    std::vector<std::u16string> args = {u"-h"};
    int fd = 1;
    ASSERT_EQ(SelectionService::GetInstance()->Dump(fd, args), 0);
}

/**
 * @tc.name: SelectionService010
 * @tc.desc: test Dump ALL
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService010, TestSize.Level0)
{
    std::vector<std::u16string> args = {u"-a"};
    int fd = 1;
    ASSERT_EQ(SelectionService::GetInstance()->Dump(fd, args), 0);
}

/**
 * @tc.name: SelectionService011
 * @tc.desc: test service start and stop
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService011, TestSize.Level0)
{
    SelectionService testSA(SELECTION_FWK_SA_ID, true);
    SelectionService::GetInstance()->OnStart();
    SelectionService::GetInstance()->Init();
    SelectionService::GetInstance()->InputMonitorInit();
    SelectionService::GetInstance()->InputMonitorInit();
    SelectionService::GetInstance()->WatchParams();

    int ret = SetParameter("sys.selection.uid", "1010");
    ASSERT_EQ(ret, 0);
    ret = SetParameter("sys.selection.switch", "off");
    ASSERT_EQ(ret, 0);
    SelectionService::GetInstance()->SynchronizeSelectionConfig();
    SelectionService::GetInstance()->OnStop();
    SelectionService::GetInstance()->OnStop();
    SetParameter("sys.selection.switch", "on");
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionService012
 * @tc.desc: test SynchronizeSelectionConfig
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService012, TestSize.Level0)
{
    auto selectionConfig = DbSelectionConfigRepository::GetInstance()->GetOneByUserId(100);
    ASSERT_NE(selectionConfig, std::nullopt);

    auto store = SelectionConfigDataBase::GetInstance()->store_;
    SelectionConfigDataBase::GetInstance()->store_ = nullptr;
    SelectionService::GetInstance()->SynchronizeSelectionConfig();
    SelectionConfigDataBase::GetInstance()->store_ = store;

    int ret = SetParameter("sys.selection.uid", "1011");
    std::string appInfo = selectionConfig->GetApplicationInfo();
    ret = SetParameter("sys.selection.app", appInfo.c_str());
    ASSERT_EQ(ret, 0);
    SelectionService::GetInstance()->SynchronizeSelectionConfig();

    ret = SetParameter("sys.selection.app", "a");
    ASSERT_EQ(ret, 0);
    SelectionService::GetInstance()->SynchronizeSelectionConfig();

    ret = SetParameter("sys.selection.switch", "off");
    ASSERT_EQ(ret, 0);
    SelectionService::GetInstance()->SynchronizeSelectionConfig();

    ret = SetParameter("sys.selection.switch", "on");
    ASSERT_EQ(ret, 0);
    SelectionService::GetInstance()->SynchronizeSelectionConfig();

    SelectionConfigDataBase::GetInstance()->store_ = nullptr;
    ret = SetParameter("sys.selection.switch", "off");
    ASSERT_EQ(ret, 0);
    SelectionService::GetInstance()->SynchronizeSelectionConfig();
    ret = SetParameter("sys.selection.switch", "on");
    ASSERT_EQ(ret, 0);
    SelectionService::GetInstance()->SynchronizeSelectionConfig();
    SelectionConfigDataBase::GetInstance()->store_ = store;

    ret = SetParameter("sys.selection.uid", "100");
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: SelectionService013
 * @tc.desc: test methods with non-nullptr connectInner_
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService013, TestSize.Level0)
{
    auto connectInner = SelectionService::GetInstance()->connectInner_;
    SelectionService::GetInstance()->connectInner_ = sptr<SelectionExtensionAbilityConnection>::MakeSptr(1001);
    ASSERT_NE(SelectionService::GetInstance()->connectInner_, nullptr);

    SelectionService::GetInstance()->connectInner_->connectedAbilityInfo = {100, "a", "b"};
    int ret = SelectionService::GetInstance()->ReconnectExtAbility("a", "b");
    ASSERT_EQ(ret, 0);

    SelectionService::GetInstance()->DoDisconnectCurrentExtAbility();

    MemSelectionConfig::GetInstance().SetApplicationInfo("a");
    SelectionService::GetInstance()->WatchExtAbilityInstalled("a", "b");
    SelectionService::GetInstance()->connectInner_ = connectInner;
}

/**
 * @tc.name: SelectionService014
 * @tc.desc: test OnAbilityConnectDone
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService014, TestSize.Level0)
{
    auto extensionConnection = sptr<SelectionExtensionAbilityConnection>::MakeSptr(1001);
    ASSERT_NE(extensionConnection, nullptr);
    AppExecFwk::ElementName elementName("TestDeviceId", "TestBundleName", "TestAbilityName");
    sptr<IRemoteObject> remoteObject;
    extensionConnection->OnAbilityConnectDone(elementName, remoteObject, 0);
    ASSERT_EQ(extensionConnection->connectPromise_, nullptr);

    extensionConnection->WaitForConnect();
    ASSERT_NE(extensionConnection->connectPromise_, nullptr);
    extensionConnection->OnAbilityConnectDone(elementName, remoteObject, 0);
    ASSERT_EQ(extensionConnection->connectPromise_, nullptr);

    extensionConnection->WaitForConnect();
    ASSERT_NE(extensionConnection->connectPromise_, nullptr);
}

/**
 * @tc.name: SelectionService015
 * @tc.desc: test OnAbilityDisconnectDone
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService015, TestSize.Level0)
{
    auto connection = sptr<SelectionExtensionAbilityConnection>::MakeSptr(1002);
    ASSERT_NE(connection, nullptr);
    AppExecFwk::ElementName element("TestDeviceId", "TestBundleName", "TestAbilityName");
    connection->OnAbilityDisconnectDone(element, 0);
    ASSERT_EQ(connection->disconnectPromise_, nullptr);

    connection->WaitForDisconnect();
    ASSERT_NE(connection->disconnectPromise_, nullptr);
    MemSelectionConfig::GetInstance().SetEnabled(false);
    connection->OnAbilityDisconnectDone(element, 0);
    ASSERT_EQ(connection->disconnectPromise_, nullptr);

    connection->WaitForDisconnect();
    ASSERT_NE(connection->disconnectPromise_, nullptr);
    MemSelectionConfig::GetInstance().SetEnabled(true);
    connection->needReconnectWithException = false;
    connection->OnAbilityDisconnectDone(element, 0);
    ASSERT_EQ(connection->disconnectPromise_, nullptr);

    connection->WaitForDisconnect();
    ASSERT_NE(connection->disconnectPromise_, nullptr);
    MemSelectionConfig::GetInstance().SetEnabled(true);
    connection->needReconnectWithException = true;
    MemSelectionConfig::GetInstance().SetApplicationInfo("a/b");
    connection->OnAbilityDisconnectDone(element, 0);
    ASSERT_EQ(connection->disconnectPromise_, nullptr);

    connection->WaitForDisconnect();
    ASSERT_NE(connection->disconnectPromise_, nullptr);
    MemSelectionConfig::GetInstance().SetEnabled(true);
    connection->needReconnectWithException = true;
    MemSelectionConfig::GetInstance().SetApplicationInfo("TestBundleName/TestAbilityName");
    connection->OnAbilityDisconnectDone(element, 0);
    ASSERT_EQ(connection->disconnectPromise_, nullptr);

    connection->WaitForDisconnect();
    ASSERT_NE(connection->disconnectPromise_, nullptr);
}

/**
 * @tc.name: SelectionService016
 * @tc.desc: test GetSelectionContent
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService016, TestSize.Level0)
{
    std::string selectionContent;
    auto ret = SelectionService::GetInstance()->GetSelectionContent(selectionContent);
    ASSERT_NE(ret, 0);

    auto inputMonitor = SelectionService::GetInstance()->inputMonitor_;
    SelectionService::GetInstance()->inputMonitor_ = nullptr;
    ret = SelectionService::GetInstance()->GetSelectionContent(selectionContent);
    ASSERT_EQ(ret, 1);

    SelectionService::GetInstance()->inputMonitor_ = inputMonitor;
    SelectionService::GetInstance()->inputMonitor_->SetCanGetSelectionContentFlag(false);
    ret = SelectionService::GetInstance()->GetSelectionContent(selectionContent);
    ASSERT_EQ(ret, 4);

    SelectionService::GetInstance()->inputMonitor_->SetCanGetSelectionContentFlag(true);
    ret = SelectionService::GetInstance()->GetSelectionContent(selectionContent);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: SelectionService017
 * @tc.desc: test HandleFocusChanged
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService017, TestSize.Level0)
{
    sptr<Rosen::FocusChangeInfo> focusChangeInfo;

    auto listener = SelectionService::GetInstance()->listener_;
    SelectionService::GetInstance()->listener_ = nullptr;
    SelectionService::GetInstance()->HandleFocusChanged(focusChangeInfo, true);

    auto remote = GetSelectionSystemAbility();
    sptr<ISelectionListener> selectionListener = iface_cast<ISelectionListener>(remote);
    ASSERT_NE(selectionListener, nullptr);
    SelectionService::GetInstance()->listener_ = selectionListener;
    SelectionService::GetInstance()->HandleFocusChanged(focusChangeInfo, false);

    focusChangeInfo = new Rosen::FocusChangeInfo();
    ASSERT_NE(focusChangeInfo, nullptr);
    SelectionService::GetInstance()->HandleFocusChanged(focusChangeInfo, true);
    SelectionService::GetInstance()->listener_ = listener;
}

/**
 * @tc.name: SelectionService018
 * @tc.desc: test methods with mock failed
 * @tc.type: FUNC
 */
HWTEST_F(SelectionServiceTest, SelectionService018, TestSize.Level0)
{
    MockSelectionService mockObj;
    EXPECT_CALL(mockObj, CheckUserLoggedIn()).WillRepeatedly(Return(false));

    mockObj.PersistSelectionConfig();
    mockObj.SynchronizeSelectionConfig();

    sptr<ISelectionListener> listener;
    int ret = mockObj.RegisterListener(listener);
    ASSERT_EQ(ret, 3);

    ret = mockObj.UnregisterListener(listener);
    ASSERT_EQ(ret, 3);

    bool resultValue;
    ret = mockObj.IsCurrentSelectionApp(1001, resultValue);
    ASSERT_EQ(ret, 3);

    std::string selectionContent;
    ret = mockObj.GetSelectionContent(selectionContent);
    ASSERT_EQ(ret, 3);
}
}
}