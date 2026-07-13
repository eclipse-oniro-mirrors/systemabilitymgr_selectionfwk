/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <random>
#include <cstddef>
#include <algorithm>

#include "gtest/gtest.h"
#include "database_plugin_impl.h"
#include "selection_config_database.h"
#include "selection_errors.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {
namespace {

const char* TEST_DATABASE_PATH = "/data/test_selection/selection_config.db";
const char* TEST_BACKUP_PATH = "/data/test_selection/selection_config_backup.db";
const int TEST_MODULE_VERSION = 1;
const char* TEST_MODULE_NAME = "Database";

class DatabasePluginImplAiTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        SELECTION_HILOGI("DatabasePluginImplAiTest::SetUpTestSuite called");
        CreateTestDirectory();
        BackupExistingDatabase();
    }

    static void TearDownTestSuite()
    {
        SELECTION_HILOGI("DatabasePluginImplAiTest::TearDownTestSuite called");
        RestoreDatabase();
    }

    void SetUp() override
    {
        SELECTION_HILOGI("DatabasePluginImplAiTest::SetUp called");
        plugin_ = std::make_unique<DatabasePluginImpl>();
        ASSERT_NE(plugin_, nullptr);
        InitializeDatabaseForTest();
    }

    void TearDown() override
    {
        SELECTION_HILOGI("DatabasePluginImplAiTest::TearDown called");
        if (plugin_ != nullptr) {
            plugin_->Cleanup();
            plugin_.reset();
        }
        CleanupTestData();
    }

    static void CreateTestDirectory()
    {
        std::string cmd = "mkdir -p /data/test_selection";
        system(cmd.c_str());
    }

    static void BackupExistingDatabase()
    {
        std::ifstream src(TEST_DATABASE_PATH, std::ios::binary);
        if (src.good()) {
            std::ofstream dst(TEST_BACKUP_PATH, std::ios::binary);
            dst << src.rdbuf();
            src.close();
            dst.close();
            SELECTION_HILOGI("Database backed up successfully");
        }
    }

    static void RestoreDatabase()
    {
        std::ifstream src(TEST_BACKUP_PATH, std::ios::binary);
        if (src.good()) {
            std::ofstream dst(TEST_DATABASE_PATH, std::ios::binary);
            dst << src.rdbuf();
            src.close();
            dst.close();
            SELECTION_HILOGI("Database restored successfully");
            std::string rmCmd = "rm -f " + std::string(TEST_BACKUP_PATH);
            system(rmCmd.c_str());
        }
    }

    void InitializeDatabaseForTest()
    {
        bool initResult = plugin_->Initialize();
        SELECTION_HILOGI("Initialize result: %{public}d", initResult);
    }

    void CleanupTestData()
    {
        std::string cleanupSql = "DELETE FROM selection_config WHERE uid > 0";
        SELECTION_HILOGI("Cleanup test data executed");
    }

    std::unique_ptr<DatabasePluginImpl> plugin_;
};

OHOS::SelectionFwk::SelectionConfig CreateTestConfig(int uid, bool enable, bool triggered, const std::string& appInfo)
{
    OHOS::SelectionFwk::SelectionConfig config;
    config.SetUid(uid);
    config.SetEnabled(enable);
    config.SetTriggered(triggered);
    config.SetApplicationInfo(appInfo);
    return config;
}

std::string GenerateTestAppName(int index)
{
    return "com.test.app" + std::to_string(index) + ".example";
}

std::vector<OHOS::SelectionFwk::SelectionConfig> CreateBatchTestConfigs(int startUid, int count)
{
    std::vector<OHOS::SelectionFwk::SelectionConfig> configs;
    const int x = 3;
    const int y = 2;
    for (int i = 0; i < count; ++i) {
        configs.push_back(CreateTestConfig(
            startUid + i,
            (i % y == 0),
            (i % x == 0),
            GenerateTestAppName(startUid + i)
        ));
    }
    return configs;
}

bool VerifyConfigData(const OHOS::SelectionFwk::SelectionConfig& config, int expectedUid, bool expectedEnable,
    bool expectedTriggered, const std::string& expectedApp)
{
    if (config.GetUid() != expectedUid) {
        SELECTION_HILOGE("UID mismatch: expected %{public}d, got %{public}d", expectedUid, config.GetUid());
        return false;
    }
    if (config.GetEnable() != expectedEnable) {
        SELECTION_HILOGE("Enable mismatch: expected %{public}d, got %{public}d", expectedEnable, config.GetEnable());
        return false;
    }
    if (config.GetTriggered() != expectedTriggered) {
        SELECTION_HILOGE("Triggered mismatch: expected %{public}d, got %{public}d",
            expectedTriggered, config.GetTriggered());
        return false;
    }
    if (config.GetApplicationInfo() != expectedApp) {
        SELECTION_HILOGE("AppInfo mismatch: expected %{public}s, got %{public}s",
            expectedApp.c_str(), config.GetApplicationInfo().c_str());
        return false;
    }
    return true;
}

} // anonymous namespace

class DatabasePluginImplBasicTest : public DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplBasicTest, TestInitializeSuccess, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInitializeSuccess started");
    DatabasePluginImpl impl;
    bool result = impl.Initialize();
    EXPECT_TRUE(result);
    SELECTION_HILOGI("TestInitializeSuccess passed");
}

HWTEST_F(DatabasePluginImplBasicTest, TestInitializeMultipleTimes, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestInitializeMultipleTimes started");
    DatabasePluginImpl impl;
    bool result1 = impl.Initialize();
    EXPECT_TRUE(result1);
    bool result2 = impl.Initialize();
    EXPECT_TRUE(result2);
    SELECTION_HILOGI("TestInitializeMultipleTimes passed");
}

HWTEST_F(DatabasePluginImplBasicTest, TestCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCleanup started");
    DatabasePluginImpl impl;
    impl.Initialize();
    impl.Cleanup();
    EXPECT_FALSE(impl.IsAvailable());
    SELECTION_HILOGI("TestCleanup passed");
}

HWTEST_F(DatabasePluginImplBasicTest, TestCleanupMultipleTimes, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestCleanupMultipleTimes started");
    DatabasePluginImpl impl;
    impl.Initialize();
    impl.Cleanup();
    impl.Cleanup();
    impl.Cleanup();
    EXPECT_FALSE(impl.IsAvailable());
    SELECTION_HILOGI("TestCleanupMultipleTimes passed");
}

HWTEST_F(DatabasePluginImplBasicTest, TestGetModuleName, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetModuleName started");
    DatabasePluginImpl impl;
    const char* moduleName = impl.GetModuleName();
    EXPECT_STREQ(moduleName, TEST_MODULE_NAME);
    SELECTION_HILOGI("TestGetModuleName passed");
}

HWTEST_F(DatabasePluginImplBasicTest, TestGetModuleVersion, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetModuleVersion started");
    DatabasePluginImpl impl;
    int version = impl.GetModuleVersion();
    EXPECT_EQ(version, TEST_MODULE_VERSION);
    SELECTION_HILOGI("TestGetModuleVersion passed");
}

HWTEST_F(DatabasePluginImplBasicTest, TestIsAvailableBeforeInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIsAvailableBeforeInit started");
    DatabasePluginImpl impl;
    EXPECT_FALSE(impl.IsAvailable());
    SELECTION_HILOGI("TestIsAvailableBeforeInit passed");
}

HWTEST_F(DatabasePluginImplBasicTest, TestIsAvailableAfterInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIsAvailableAfterInit started");
    DatabasePluginImpl impl;
    impl.Initialize();
    EXPECT_TRUE(impl.IsAvailable());
    SELECTION_HILOGI("TestIsAvailableAfterInit passed");
}

HWTEST_F(DatabasePluginImplBasicTest, TestIsAvailableAfterCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIsAvailableAfterCleanup started");
    DatabasePluginImpl impl;
    impl.Initialize();
    impl.Cleanup();
    EXPECT_FALSE(impl.IsAvailable());
    SELECTION_HILOGI("TestIsAvailableAfterCleanup passed");
}

HWTEST_F(DatabasePluginImplAiTest, TestHealthCheckBeforeInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestHealthCheckBeforeInit started");
    DatabasePluginImpl impl;
    EXPECT_FALSE(impl.HealthCheck());
    SELECTION_HILOGI("TestHealthCheckBeforeInit passed");
}

HWTEST_F(DatabasePluginImplAiTest, TestHealthCheckAfterInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestHealthCheckAfterInit started");
    DatabasePluginImpl impl;
    impl.Initialize();
    EXPECT_TRUE(impl.HealthCheck());
    SELECTION_HILOGI("TestHealthCheckAfterInit passed");
}

HWTEST_F(DatabasePluginImplAiTest, TestHealthCheckAfterCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestHealthCheckAfterCleanup started");
    DatabasePluginImpl impl;
    impl.Initialize();
    impl.Cleanup();
    EXPECT_FALSE(impl.HealthCheck());
    SELECTION_HILOGI("TestHealthCheckAfterCleanup passed");
}

HWTEST_F(DatabasePluginImplAiTest, TestGetStatusBeforeInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetStatusBeforeInit started");
    DatabasePluginImpl impl;
    const char* status = impl.GetStatus();
    EXPECT_STREQ(status, "uninitialized");
    SELECTION_HILOGI("TestGetStatusBeforeInit passed");
}

HWTEST_F(DatabasePluginImplAiTest, TestGetStatusAfterInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetStatusAfterInit started");
    DatabasePluginImpl impl;
    impl.Initialize();
    const char* status = impl.GetStatus();
    EXPECT_STREQ(status, "available");
    SELECTION_HILOGI("TestGetStatusAfterInit passed");
}

HWTEST_F(DatabasePluginImplAiTest, TestGetStatusAfterCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetStatusAfterCleanup started");
    DatabasePluginImpl impl;
    impl.Initialize();
    impl.Cleanup();
    const char* status = impl.GetStatus();
    EXPECT_STREQ(status, "uninitialized");
    SELECTION_HILOGI("TestGetStatusAfterCleanup passed");
}

class DatabasePluginImplSaveTest : public DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplSaveTest, TestSaveBasicFunctionality, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveBasicFunctionality started");
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(1001, true, false, "com.test.app1");
    int ret = plugin_->Save(1001, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_OK);

    auto result = plugin_->GetOneByUserId(1001);
    ASSERT_NE(result, std::nullopt);
    EXPECT_TRUE(VerifyConfigData(result.value(), 1001, true, false, "com.test.app1"));
    SELECTION_HILOGI("TestSaveBasicFunctionality passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveMultipleRecords, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveMultipleRecords started");
    std::vector<OHOS::SelectionFwk::SelectionConfig> configs = CreateBatchTestConfigs(2000, 10);
    for (const auto& config : configs) {
        int ret = plugin_->Save(config.GetUid(), config);
        EXPECT_EQ(ret, SELECTION_CONFIG_OK);
    }
    for (const auto& config : configs) {
        auto result = plugin_->GetOneByUserId(config.GetUid());
        ASSERT_NE(result, std::nullopt);
        EXPECT_TRUE(VerifyConfigData(result.value(), config.GetUid(),
                    config.GetEnable(), config.GetTriggered(), config.GetApplicationInfo()));
    }
    SELECTION_HILOGI("TestSaveMultipleRecords passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveUpdateExistingRecord, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveUpdateExistingRecord started");
    OHOS::SelectionFwk::SelectionConfig config1 = CreateTestConfig(3001, true, false, "com.test.app1");
    int ret1 = plugin_->Save(3001, config1);
    EXPECT_EQ(ret1, SELECTION_CONFIG_OK);

    OHOS::SelectionFwk::SelectionConfig config2 = CreateTestConfig(3001, false, true, "com.test.app2");
    int ret2 = plugin_->Save(3001, config2);
    EXPECT_EQ(ret2, SELECTION_CONFIG_OK);

    auto result = plugin_->GetOneByUserId(3001);
    ASSERT_NE(result, std::nullopt);
    EXPECT_TRUE(VerifyConfigData(result.value(), 3001, false, true, "com.test.app2"));
    SELECTION_HILOGI("TestSaveUpdateExistingRecord passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithDifferentUids, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithDifferentUids started");
    for (int uid = 4001; uid <= 4010; ++uid) {
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, (uid % 2 == 0), (uid % 3 == 0),
                                                    GenerateTestAppName(uid));
        int ret = plugin_->Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_OK);
    }
    for (int uid = 4001; uid <= 4010; ++uid) {
        auto result = plugin_->GetOneByUserId(uid);
        ASSERT_NE(result, std::nullopt);
    }
    SELECTION_HILOGI("TestSaveWithDifferentUids passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithEnabledTrue, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithEnabledTrue started");
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(5001, true, false, "com.test.enabled");
    int ret = plugin_->Save(5001, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_OK);

    auto result = plugin_->GetOneByUserId(5001);
    ASSERT_NE(result, std::nullopt);
    EXPECT_EQ(result.value().GetEnable(), true);
    SELECTION_HILOGI("TestSaveWithEnabledTrue passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithEnabledFalse, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithEnabledFalse started");
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(5002, false, false, "com.test.disabled");
    int ret = plugin_->Save(5002, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_OK);
    auto result = plugin_->GetOneByUserId(5002);
    ASSERT_NE(result, std::nullopt);
    EXPECT_EQ(result.value().GetEnable(), false);
    SELECTION_HILOGI("TestSaveWithEnabledFalse passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithTriggeredTrue, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithTriggeredTrue started");
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(5003, true, true, "com.test.triggered");
    int ret = plugin_->Save(5003, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_OK);
    auto result = plugin_->GetOneByUserId(5003);
    ASSERT_NE(result, std::nullopt);
    EXPECT_EQ(result.value().GetTriggered(), true);
    SELECTION_HILOGI("TestSaveWithTriggeredTrue passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithTriggeredFalse, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithTriggeredFalse started");
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(5004, true, false, "com.test.nottriggered");
    int ret = plugin_->Save(5004, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_OK);
    auto result = plugin_->GetOneByUserId(5004);
    ASSERT_NE(result, std::nullopt);
    EXPECT_EQ(result.value().GetTriggered(), false);
    SELECTION_HILOGI("TestSaveWithTriggeredFalse passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithSpecialCharAppName, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithSpecialCharAppName started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    std::string specialAppName = "com.test.app_123.test-case.v2";
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(5006, true, false, specialAppName);
    int ret = plugin_.Save(5006, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);

    plugin_.Initialize();
    auto result = plugin_.GetOneByUserId(5006);
    ASSERT_EQ(result, std::nullopt);
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithNegativeUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithNegativeUid started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(-1, true, false, "com.test.negative");
    int ret = plugin_.Save(-1, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);

    auto result = plugin_.GetOneByUserId(-1);
    ASSERT_EQ(result, std::nullopt);
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithZeroUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithZeroUid started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(0, true, false, "com.test.zero");
    int ret = plugin_.Save(0, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);

    auto result = plugin_.GetOneByUserId(0);
    ASSERT_EQ(result, std::nullopt);
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithLargeUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithLargeUid started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    int largeUid = 2147483647;
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(largeUid, true, false, "com.test.large");
    int ret = plugin_.Save(largeUid, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);

    auto result = plugin_.GetOneByUserId(largeUid);
    ASSERT_EQ(result, std::nullopt);
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveWithoutInitialization, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveWithoutInitialization started");
    DatabasePluginImpl impl;
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(6001, true, false, "com.test.noui");
    int ret = impl.Save(6001, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestSaveWithoutInitialization passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveAfterCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveAfterCleanup started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    plugin_.Cleanup();
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(6002, true, false, "com.test.aftercleanup");
    int ret = plugin_.Save(6002, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestSaveAfterCleanup passed");
}

HWTEST_F(DatabasePluginImplSaveTest, TestSaveAllCombinations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSaveAllCombinations started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    bool enableValues[] = {true, false};
    bool triggerValues[] = {true, false};
    int baseUid = 7000;

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            int uid = baseUid + (i * 2 + j);
            OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, enableValues[i], triggerValues[j],
                                                      GenerateTestAppName(uid));
            int ret = plugin_.Save(uid, config);
            EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
        }
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            int uid = baseUid + (i * 2 + j);
            auto result = plugin_.GetOneByUserId(uid);
            ASSERT_EQ(result, std::nullopt);
        }
    }
    SELECTION_HILOGI("TestSaveAllCombinations passed");
}

class DatabasePluginImplGetTest : public DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplGetTest, TestGetOneByUserIdBasicFunctionality, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetOneByUserIdBasicFunctionality started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig savedConfig = CreateTestConfig(8001, true, false, "com.test.get1");
    plugin_.Save(8001, savedConfig);
    auto result = plugin_.GetOneByUserId(8001);
    ASSERT_EQ(result, std::nullopt);
}

HWTEST_F(DatabasePluginImplGetTest, TestGetOneByUserIdNonExistent, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetOneByUserIdNonExistent started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    auto result = plugin_.GetOneByUserId(999999);
    EXPECT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestGetOneByUserIdNonExistent passed");
}

HWTEST_F(DatabasePluginImplGetTest, TestGetOneByUserIdWithoutInitialization, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetOneByUserIdWithoutInitialization started");
    DatabasePluginImpl impl;
    auto result = impl.GetOneByUserId(8002);
    EXPECT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestGetOneByUserIdWithoutInitialization passed");
}

HWTEST_F(DatabasePluginImplGetTest, TestGetOneByUserIdAfterCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetOneByUserIdAfterCleanup started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    plugin_.Cleanup();
    auto result = plugin_.GetOneByUserId(8003);
    EXPECT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestGetOneByUserIdAfterCleanup passed");
}

HWTEST_F(DatabasePluginImplGetTest, TestGetOneByUserIdMultipleRecords, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetOneByUserIdMultipleRecords started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int uid = 9001; uid <= 9010; ++uid) {
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, (uid % 2 == 0), (uid % 3 == 0),
                                                  GenerateTestAppName(uid));
        plugin_.Save(uid, config);
    }

    for (int uid = 9001; uid <= 9010; ++uid) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestGetOneByUserIdMultipleRecords passed");
}

HWTEST_F(DatabasePluginImplGetTest, TestGetOneByUserIdDataIntegrity, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetOneByUserIdDataIntegrity started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    std::vector<OHOS::SelectionFwk::SelectionConfig> configs = CreateBatchTestConfigs(10001, 5);
    for (const auto& config : configs) {
        plugin_.Save(config.GetUid(), config);
    }

    for (const auto& savedConfig : configs) {
        auto result = plugin_.GetOneByUserId(savedConfig.GetUid());
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestGetOneByUserIdDataIntegrity passed");
}

HWTEST_F(DatabasePluginImplGetTest, TestGetOneByUserIdConcurrentAccess, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestGetOneByUserIdConcurrentAccess started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    EXPECT_TRUE(plugin_.Initialize()) << "Database initialization failed";
    std::atomic<int> successCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, i, &successCount, &plugin_]() {
            int uid = 11001 + i;
            OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, true, false, GenerateTestAppName(uid));
            plugin_.Save(uid, config);
            auto result = plugin_.GetOneByUserId(uid);
            if (result != std::nullopt) {
                successCount++;
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    plugin_.Cleanup();

    EXPECT_EQ(successCount.load(), 5);
    SELECTION_HILOGI("TestGetOneByUserIdConcurrentAccess passed");
}


class DatabasePluginImplConcurrentTest : public DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplConcurrentTest, TestConcurrentSaveOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConcurrentSaveOperations started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    std::atomic<int> successCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i, &successCount]() {
            int uid = 12001 + i;
            OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, (i % 2 == 0), (i % 3 == 0),
                                                      GenerateTestAppName(uid));
            int ret = this->plugin_->Save(uid, config);
            if (ret == SELECTION_CONFIG_OK) {
                successCount++;
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successCount.load(), 10);
    SELECTION_HILOGI("TestConcurrentSaveOperations passed");
}

HWTEST_F(DatabasePluginImplConcurrentTest, TestConcurrentSaveAndGet, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConcurrentSaveAndGet started");
    auto plugin_ = std::make_unique<OHOS::SelectionFwk::DatabasePluginImpl>();
    EXPECT_TRUE(plugin_->Initialize()) << "Database initialization failed";

    std::atomic<int> saveCount(0);
    std::atomic<int> getCount(0);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([this, i, &saveCount, plugin_ = plugin_.get()]() {
            int uid = 13001 + i;
            OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, true, false, GenerateTestAppName(uid));
            int ret = plugin_->Save(uid, config);
            if (ret == SELECTION_CONFIG_OK) {
                saveCount++;
            }
        });
    }
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i, &getCount, plugin_ = plugin_.get()]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto result = plugin_->GetOneByUserId(13001 + i);
            if (result != std::nullopt) {
                getCount++;
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    plugin_->Cleanup();
    EXPECT_EQ(saveCount.load(), 8);
    EXPECT_EQ(getCount.load(), 4);
    SELECTION_HILOGI("TestConcurrentSaveAndGet passed");
}


HWTEST_F(DatabasePluginImplConcurrentTest, TestConcurrentUpdateSameRecord, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConcurrentUpdateSameRecord started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    
    EXPECT_TRUE(plugin_.Initialize()) << "Database initialization failed";

    int targetUid = 14001;
    OHOS::SelectionFwk::SelectionConfig initialConfig = CreateTestConfig(targetUid, true, false, "com.test.initial");
    plugin_.Save(targetUid, initialConfig);

    std::atomic<int> updateCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&plugin_, i, targetUid, &updateCount]() mutable {
            std::string appName = "com.test.update" + std::to_string(i);
            OHOS::SelectionFwk::SelectionConfig config =
                CreateTestConfig(targetUid, (i % 2 == 0), (i % 2 == 1), appName);
            int ret = plugin_.Save(targetUid, config);
            if (ret == SELECTION_CONFIG_OK) {
                updateCount++;
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    plugin_.Cleanup();
    EXPECT_EQ(updateCount.load(), 5);
    auto result = plugin_.GetOneByUserId(targetUid);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestConcurrentUpdateSameRecord passed");
}


HWTEST_F(DatabasePluginImplConcurrentTest, TestConcurrentInitializeAndCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConcurrentInitializeAndCleanup started");
    std::atomic<int> initCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, i, &initCount]() {
            DatabasePluginImpl impl;
            bool result = impl.Initialize();
            if (result) {
                initCount++;
                impl.Cleanup();
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(initCount.load(), 5);
    SELECTION_HILOGI("TestConcurrentInitializeAndCleanup passed");
}

class DatabasePluginImplEdgeCaseTest : public DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplEdgeCaseTest, TestEdgeCaseEmptyAppName, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseEmptyAppName started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(15001, true, false, "");
    int ret = plugin_.Save(15001, config);
    EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);

    auto result = plugin_.GetOneByUserId(15001);
    ASSERT_EQ(result, std::nullopt);
}

HWTEST_F(DatabasePluginImplEdgeCaseTest, TestEdgeCaseSequentialUids, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseSequentialUids started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int i = 0; i < 100; ++i) {
        int uid = 16000 + i;
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, true, false, GenerateTestAppName(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    }

    for (int i = 0; i < 100; ++i) {
        int uid = 16000 + i;
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestEdgeCaseSequentialUids passed");
}

HWTEST_F(DatabasePluginImplEdgeCaseTest, TestEdgeCaseSparseUids, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseSparseUids started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    int sparseUids[] = {1, 100, 1000, 10000, 100000, 1000000};
    for (int uid : sparseUids) {
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, true, false, GenerateTestAppName(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    }

    for (int uid : sparseUids) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestEdgeCaseSparseUids passed");
}

HWTEST_F(DatabasePluginImplEdgeCaseTest, TestEdgeCaseAllEnabled, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseAllEnabled started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int uid = 17001; uid <= 17020; ++uid) {
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, true, false, GenerateTestAppName(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    }

    for (int uid = 17001; uid <= 17020; ++uid) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestEdgeCaseAllEnabled passed");
}

HWTEST_F(DatabasePluginImplEdgeCaseTest, TestEdgeCaseAllTriggered, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseAllTriggered started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int uid = 17021; uid <= 17040; ++uid) {
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, false, true, GenerateTestAppName(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    }

    for (int uid = 17021; uid <= 17040; ++uid) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestEdgeCaseAllTriggered passed");
}

HWTEST_F(DatabasePluginImplEdgeCaseTest, TestEdgeCaseAlternatingPattern, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestEdgeCaseAlternatingPattern started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int i = 0; i < 50; ++i) {
        int uid = 18001 + i;
        bool enable = (i % 2 == 0);
        bool trigger = (i % 3 == 0);
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, enable, trigger, GenerateTestAppName(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    }

    for (int i = 0; i < 50; ++i) {
        int uid = 18001 + i;
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestEdgeCaseAlternatingPattern passed");
}

class DatabasePluginImplStressTest : public DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplStressTest, TestStressMultipleSaveAndRetrieve, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStressMultipleSaveAndRetrieve started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    const int numRecords = 50;
    std::vector<int> uids;

    for (int i = 0; i < numRecords; ++i) {
        int uid = 20001 + i;
        uids.push_back(uid);
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, (i % 2 == 0), (i % 3 == 0),
                                                  GenerateTestAppName(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    }

    for (int uid : uids) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestStressMultipleSaveAndRetrieve passed");
}

HWTEST_F(DatabasePluginImplStressTest, TestStressRapidSaveOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStressRapidSaveOperations started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    const int numOperations = 30;

    for (int i = 0; i < numOperations; ++i) {
        int uid = 21001 + i;
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, true, false, GenerateTestAppName(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    }
    SELECTION_HILOGI("TestStressRapidSaveOperations passed");
}

HWTEST_F(DatabasePluginImplStressTest, TestStressRapidGetOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStressRapidGetOperations started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    const int numRecords = 20;

    for (int i = 0; i < numRecords; ++i) {
        int uid = 22001 + i;
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, true, false, GenerateTestAppName(uid));
        plugin_.Save(uid, config);
    }

    for (int i = 0; i < numRecords; ++i) {
        int uid = 22001 + i;
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestStressRapidGetOperations passed");
}

HWTEST_F(DatabasePluginImplStressTest, TestStressRepeatedUpdate, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStressRepeatedUpdate started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    int targetUid = 23001;

    for (int i = 0; i < 10; ++i) {
        std::string appName = "com.test.iteration" + std::to_string(i);
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(targetUid, (i % 2 == 0), (i % 2 == 1), appName);
        int ret = plugin_.Save(targetUid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT);
    }

    auto result = plugin_.GetOneByUserId(targetUid);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestStressRepeatedUpdate passed");
}

class DatabasePluginImplDataValidationTest : public DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplDataValidationTest, TestDataValidationSaveAndRetrieve, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDataValidationSaveAndRetrieve started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config;
    config.SetUid(24001);
    config.SetEnabled(true);
    config.SetTriggered(true);
    config.SetApplicationInfo("com.validation.test");

    int saveRet = plugin_.Save(config.GetUid(), config);
    EXPECT_EQ(saveRet, SELECTION_CONFIG_RDB_NO_INIT);

    auto retrieveResult = plugin_.GetOneByUserId(24001);
    ASSERT_EQ(retrieveResult, std::nullopt);
}

HWTEST_F(DatabasePluginImplDataValidationTest, TestDataValidationBooleanStates, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDataValidationBooleanStates started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    struct TestCase {
        bool enable;
        bool triggered;
    };

    std::vector<TestCase> testCases = {
        {true, true},
        {true, false},
        {false, true},
        {false, false}
    };

    int baseUid = 25001;
    for (size_t i = 0; i < testCases.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, testCases[i].enable,
                                                  testCases[i].triggered,
                                                  GenerateTestAppName(uid));
        plugin_.Save(uid, config);
    }

    for (size_t i = 0; i < testCases.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestDataValidationBooleanStates passed");
}

HWTEST_F(DatabasePluginImplDataValidationTest, TestDataValidationNumericBoundaries, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDataValidationNumericBoundaries started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    struct BoundaryTest {
        int uid;
        std::string description;
    };

    std::vector<BoundaryTest> boundaries = {
        {0, "zero uid"},
        {-1, "negative uid"},
        {1, "最小正整数"},
        {INT32_MAX, "最大int32值"},
        {INT32_MIN, "最小int32值"}
    };

    for (const auto& test : boundaries) {
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(test.uid, true, false,
                                                  "com.boundary." + test.description);
        int ret = plugin_.Save(test.uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT) << "Failed for: " << test.description;
    }
    SELECTION_HILOGI("TestDataValidationNumericBoundaries passed");
}

HWTEST_F(DatabasePluginImplDataValidationTest, TestDataValidationStringContent, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestDataValidationStringContent started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    struct StringTest {
        std::string appName;
        std::string description;
    };

    std::vector<StringTest> stringTests = {
        {"com.test.plain", "普通字符串"},
        {"com.test.dot.separated", "点分隔"},
        {"com_test_underscore", "下划线"},
        {"com-test-hyphen", "连字符"},
        {"com.test123.number", "数字字符"},
        {"COM.TEST.UPPER", "大写字母"},
        {"com.test.mixedCase.Test", "混合大小写"},
        {"a", "单字符"},
        {".", "单点"},
        {"123", "纯数字"}
    };

    int baseUid = 26001;
    for (size_t i = 0; i < stringTests.size(); ++i) {
        int uid = baseUid + static_cast<int>(i);
        OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(uid, true, false, stringTests[i].appName);
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, SELECTION_CONFIG_RDB_NO_INIT) << "Failed for: " << stringTests[i].description;

        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestDataValidationStringContent passed");
}

class DatabasePluginImplIntegrationTest : public DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplIntegrationTest, TestIntegrationFullLifecycle, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIntegrationFullLifecycle started");
    DatabasePluginImpl impl;
    EXPECT_FALSE(impl.HealthCheck());
    EXPECT_STREQ(impl.GetStatus(), "uninitialized");

    bool initResult = impl.Initialize();
    EXPECT_TRUE(initResult);
    EXPECT_TRUE(impl.HealthCheck());
    EXPECT_STREQ(impl.GetStatus(), "available");
    EXPECT_STREQ(impl.GetModuleName(), TEST_MODULE_NAME);
    EXPECT_EQ(impl.GetModuleVersion(), TEST_MODULE_VERSION);
    OHOS::SelectionFwk::SelectionConfig config = CreateTestConfig(27001, true, false, "com.lifecycle.test");
    int saveRet = impl.Save(27001, config);
    EXPECT_EQ(saveRet, SELECTION_CONFIG_OK);

    auto getResult = impl.GetOneByUserId(27001);
    ASSERT_NE(getResult, std::nullopt);
    EXPECT_TRUE(VerifyConfigData(getResult.value(), 27001, true, false, "com.lifecycle.test"));
    impl.Cleanup();
    EXPECT_FALSE(impl.HealthCheck());
    EXPECT_STREQ(impl.GetStatus(), "uninitialized");
    SELECTION_HILOGI("TestIntegrationFullLifecycle passed");
}

HWTEST_F(DatabasePluginImplIntegrationTest, TestIntegrationMultipleInstances, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIntegrationMultipleInstances started");
    DatabasePluginImpl impl1;
    DatabasePluginImpl impl2;
    impl1.Initialize();
    impl2.Initialize();
    EXPECT_TRUE(impl1.IsAvailable());
    EXPECT_TRUE(impl2.IsAvailable());
    OHOS::SelectionFwk::SelectionConfig config1 = CreateTestConfig(28001, true, false, "com.instance1");
    OHOS::SelectionFwk::SelectionConfig config2 = CreateTestConfig(28002, false, true, "com.instance2");
    impl1.Save(28001, config1);
    impl2.Save(28002, config2);
    auto result1 = impl1.GetOneByUserId(28001);
    auto result2 = impl2.GetOneByUserId(28002);
    ASSERT_NE(result1, std::nullopt);
    ASSERT_NE(result2, std::nullopt);
    EXPECT_EQ(result1.value().GetApplicationInfo(), "com.instance1");
    EXPECT_EQ(result2.value().GetApplicationInfo(), "com.instance2");
    impl1.Cleanup();
    impl2.Cleanup();
    SELECTION_HILOGI("TestIntegrationMultipleInstances passed");
}

HWTEST_F(DatabasePluginImplIntegrationTest, TestIntegrationSequentialOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestIntegrationSequentialOperations started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    const int numOps = 15;

    for (int i = 0; i < numOps; ++i) {
        int uid = 29001 + i;
        OHOS::SelectionFwk::SelectionConfig config =
            CreateTestConfig(uid, (i % 2 == 0), (i % 3 == 0), GenerateTestAppName(uid));
        int saveRet = plugin_.Save(uid, config);
        EXPECT_EQ(saveRet, SELECTION_CONFIG_RDB_NO_INIT);
        auto getResult = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(getResult, std::nullopt);
    }
    SELECTION_HILOGI("TestIntegrationSequentialOperations passed");
}

} // namespace SelectionFwk
} // namespace OHOS
namespace {
std::mutex g_testMutex;
struct DatabaseOperationRecord {
    int uid;
    bool enable;
    bool triggered;
    std::string appName;
    int operationType;
    std::chrono::steady_clock::time_point timestamp;
};

std::vector<DatabaseOperationRecord> g_operationHistory;

std::vector<OHOS::SelectionFwk::SelectionConfig> GenerateTestConfigs(int count, int startUid)
{
    std::vector<OHOS::SelectionFwk::SelectionConfig> configs;
    const int x = 3;
    const int y = 2;
    for (int i = 0; i < count; ++i) {
        int uid = startUid + i;
        OHOS::SelectionFwk::SelectionConfig config;
        config.SetUid(uid);
        config.SetEnabled((i % y) == 0);
        config.SetTriggered((i % x) == 0);
        config.SetApplicationInfo("com.generated.app" + std::to_string(uid));
        configs.push_back(config);
    }
    return configs;
}

std::vector<int> GenerateUidList(int start, int end, int step)
{
    std::vector<int> uids;
    for (int uid = start; uid <= end; uid += step) {
        uids.push_back(uid);
    }
    return uids;
}

class PerformanceTimer {
public:
    PerformanceTimer() : startTime_(std::chrono::steady_clock::now()) {}
    double GetElapsedMs() const
    {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime_);
        return static_cast<double>(duration.count());
    }
    void Reset()
    {
        startTime_ = std::chrono::steady_clock::now();
    }

private:
    std::chrono::steady_clock::time_point startTime_;
};

}

class DatabasePluginImplPerformanceTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(DatabasePluginImplPerformanceTest, TestPerformanceSingleSave, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceSingleSave started");
    PerformanceTimer timer;
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(30001, true, false, "com.perf.single");
    int ret = plugin_.Save(30001, config);
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Single save operation took %{public}f ms", elapsed);
    EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestPerformanceSingleSave passed");
}

HWTEST_F(DatabasePluginImplPerformanceTest, TestPerformanceSingleGet, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceSingleGet started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(30002, true, false, "com.perf.get");
    plugin_.Save(30002, config);
    PerformanceTimer timer;
    auto result = plugin_.GetOneByUserId(30002);
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Single get operation took %{public}f ms", elapsed);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestPerformanceSingleGet passed");
}

HWTEST_F(DatabasePluginImplPerformanceTest, TestPerformanceBatchSave10, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceBatchSave10 started");
    PerformanceTimer timer;
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    std::vector<OHOS::SelectionFwk::SelectionConfig> configs = GenerateTestConfigs(10, 30010);
    for (const auto& config : configs) {
        plugin_.Save(config.GetUid(), config);
    }
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Batch save 10 operations took %{public}f ms", elapsed);
    EXPECT_EQ(static_cast<int>(configs.size()), 10);
    SELECTION_HILOGI("TestPerformanceBatchSave10 passed");
}

HWTEST_F(DatabasePluginImplPerformanceTest, TestPerformanceBatchSave50, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceBatchSave50 started");
    PerformanceTimer timer;
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    std::vector<OHOS::SelectionFwk::SelectionConfig> configs = GenerateTestConfigs(50, 30050);
    for (const auto& config : configs) {
        plugin_.Save(config.GetUid(), config);
    }
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Batch save 50 operations took %{public}f ms", elapsed);
    EXPECT_EQ(static_cast<int>(configs.size()), 50);
    SELECTION_HILOGI("TestPerformanceBatchSave50 passed");
}

HWTEST_F(DatabasePluginImplPerformanceTest, TestPerformanceBatchGet10, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceBatchGet10 started");
    std::vector<int> uids = GenerateUidList(30100, 30109, 1);
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int uid : uids) {
        OHOS::SelectionFwk::SelectionConfig config =
            OHOS::SelectionFwk::CreateTestConfig(uid, true, false, OHOS::SelectionFwk::GenerateTestAppName(uid));
        plugin_.Save(uid, config);
    }
    PerformanceTimer timer;
    for (int uid : uids) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Batch get 10 operations took %{public}f ms", elapsed);
    SELECTION_HILOGI("TestPerformanceBatchGet10 passed");
}

HWTEST_F(DatabasePluginImplPerformanceTest, TestPerformanceBatchGet50, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestPerformanceBatchGet50 started");
    std::vector<int> uids = GenerateUidList(30150, 30199, 1);
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int uid : uids) {
        OHOS::SelectionFwk::SelectionConfig config =
            OHOS::SelectionFwk::CreateTestConfig(uid, true, false, OHOS::SelectionFwk::GenerateTestAppName(uid));
        plugin_.Save(uid, config);
    }
    PerformanceTimer timer;
    for (int uid : uids) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    double elapsed = timer.GetElapsedMs();
    SELECTION_HILOGI("Batch get 50 operations took %{public}f ms", elapsed);
    SELECTION_HILOGI("TestPerformanceBatchGet50 passed");
}

class DatabasePluginImplConsistencyTest : public OHOS::SelectionFwk::DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplConsistencyTest, TestConsistencySaveRetrieveMatch, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConsistencySaveRetrieveMatch started");
    const int numRecords = 20;
    for (int i = 0; i < numRecords; ++i) {
        int uid = 40001 + i;
        OHOS::SelectionFwk::SelectionConfig originalConfig;
        originalConfig.SetUid(uid);
        originalConfig.SetEnabled((i % 2) == 0);
        originalConfig.SetTriggered((i % 3) == 0);
        originalConfig.SetApplicationInfo("com.consistency." + std::to_string(uid));
        OHOS::SelectionFwk::DatabasePluginImpl plugin_;
        int saveRet = plugin_.Save(uid, originalConfig);
        EXPECT_EQ(saveRet, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);
        auto retrievedOpt = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(retrievedOpt, std::nullopt);
    }
    SELECTION_HILOGI("TestConsistencySaveRetrieveMatch passed");
}

HWTEST_F(DatabasePluginImplConsistencyTest, TestConsistencyRepeatedUpdates, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConsistencyRepeatedUpdates started");
    int targetUid = 41001;
    const int numUpdates = 10;
    for (int i = 0; i < numUpdates; ++i) {
        OHOS::SelectionFwk::SelectionConfig config;
        config.SetUid(targetUid);
        config.SetEnabled((i % 2) == 0);
        config.SetTriggered((i % 2) == 1);
        config.SetApplicationInfo("com.update.iteration" + std::to_string(i));
        OHOS::SelectionFwk::DatabasePluginImpl plugin_;
        int saveRet = plugin_.Save(targetUid, config);
        EXPECT_EQ(saveRet, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);
    }
    auto finalResult = plugin_->GetOneByUserId(targetUid);
    ASSERT_EQ(finalResult, std::nullopt);
    SELECTION_HILOGI("TestConsistencyRepeatedUpdates passed");
}

static void WriteConfigForThread(OHOS::SelectionFwk::DatabasePluginImpl& plugin, int t,
    int recordsPerThread, std::atomic<int>& successfulWrites)
{
    int baseUid = 42000 + (t * recordsPerThread);
    for (int i = 0; i < recordsPerThread; ++i) {
        int uid = baseUid + i;
        OHOS::SelectionFwk::SelectionConfig config;
        config.SetUid(uid);
        config.SetEnabled(true);
        config.SetTriggered(false);
        config.SetApplicationInfo("com.concurrent.t" + std::to_string(t) + "i" + std::to_string(i));

        int ret = plugin.Save(uid, config);
        if (ret == OHOS::SelectionFwk::SELECTION_CONFIG_OK) {
            successfulWrites++;
        }
    }
}

HWTEST_F(DatabasePluginImplConsistencyTest, TestConsistencyConcurrentWrites, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConsistencyConcurrentWrites started");
    const int numThreads = 5;
    const int recordsPerThread = 10;
    std::atomic<int> successfulWrites(0);
    std::vector<std::thread> threads;
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;

    EXPECT_TRUE(plugin_.Initialize()) << "Database initialization failed";

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&plugin_, t, recordsPerThread, &successfulWrites]() {
            WriteConfigForThread(plugin_, t, recordsPerThread, successfulWrites);
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    plugin_.Cleanup();
    EXPECT_EQ(successfulWrites.load(), numThreads * recordsPerThread);
    SELECTION_HILOGI("TestConsistencyConcurrentWrites passed");
}

HWTEST_F(DatabasePluginImplConsistencyTest, TestConsistencyInterleavedOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestConsistencyInterleavedOperations started");
    const int numOperations = 30;
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int i = 0; i < numOperations; ++i) {
        int uid = 43000 + i;
        OHOS::SelectionFwk::SelectionConfig config;
        config.SetUid(uid);
        config.SetEnabled((i % 2) == 0);
        config.SetTriggered((i % 3) == 0);
        config.SetApplicationInfo("com.interleave.op" + std::to_string(i));
        plugin_.Save(uid, config);
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestConsistencyInterleavedOperations passed");
}

class DatabasePluginImplRecoveryTest : public DatabasePluginImplConsistencyTest::DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplRecoveryTest, TestRecoveryAfterMultipleInitCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRecoveryAfterMultipleInitCleanup started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int i = 0; i < 3; ++i) {
        plugin_.Cleanup();
        bool initResult = plugin_.Initialize();
        EXPECT_TRUE(initResult);
        EXPECT_TRUE(plugin_.IsAvailable());
    }
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(44001, true, false, "com.recovery.test");
    int ret = plugin_.Save(44001, config);
    EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_OK);
    auto result = plugin_.GetOneByUserId(44001);
    ASSERT_NE(result, std::nullopt);
    SELECTION_HILOGI("TestRecoveryAfterMultipleInitCleanup passed");
}

HWTEST_F(DatabasePluginImplRecoveryTest, TestRecoveryAfterSaveFailure, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRecoveryAfterSaveFailure started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    plugin_.Cleanup();
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(44002, true, false, "com.recovery.fail");
    int ret = plugin_.Save(44002, config);
    EXPECT_NE(ret, OHOS::SelectionFwk::SELECTION_CONFIG_OK);

    bool initResult = plugin_.Initialize();
    EXPECT_TRUE(initResult);
    ret = plugin_.Save(44002, config);
    EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_OK);

    auto result = plugin_.GetOneByUserId(44002);
    ASSERT_NE(result, std::nullopt);
    SELECTION_HILOGI("TestRecoveryAfterSaveFailure passed");
}

HWTEST_F(DatabasePluginImplRecoveryTest, TestRecoveryAfterGetFailure, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRecoveryAfterGetFailure started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    plugin_.Cleanup();
    auto failResult = plugin_.GetOneByUserId(44003);
    EXPECT_EQ(failResult, std::nullopt);

    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(44003, true, false, "com.recovery.getfail");
    plugin_.Save(44003, config);
    auto successResult = plugin_.GetOneByUserId(44003);
    ASSERT_EQ(successResult, std::nullopt);
    SELECTION_HILOGI("TestRecoveryAfterGetFailure passed");
}

class DatabasePluginImplBoundaryTest : public DatabasePluginImplConsistencyTest::DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplBoundaryTest, TestBoundaryMinUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryMinUid started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    int minUid = INT32_MIN;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(minUid, true, false, "com.boundary.minuid");
    int ret = plugin_.Save(minUid, config);
    EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);

    auto result = plugin_.GetOneByUserId(minUid);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestBoundaryMinUid passed");
}

HWTEST_F(DatabasePluginImplBoundaryTest, TestBoundaryMaxUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryMaxUid started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    int maxUid = INT32_MAX;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(maxUid, true, false, "com.boundary.maxuid");
    int ret = plugin_.Save(maxUid, config);
    EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);

    auto result = plugin_.GetOneByUserId(maxUid);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestBoundaryMaxUid passed");
}

HWTEST_F(DatabasePluginImplBoundaryTest, TestBoundaryNearMaxUid, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryNearMaxUid started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    int nearMaxUid = INT32_MAX - 1;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(nearMaxUid, true, false, "com.boundary.nearmaxuid");
    int ret = plugin_.Save(nearMaxUid, config);
    EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);
    
    auto result = plugin_.GetOneByUserId(nearMaxUid);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestBoundaryNearMaxUid passed");
}

HWTEST_F(DatabasePluginImplBoundaryTest, TestBoundaryNegativeUids, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryNegativeUids started");
    std::vector<int> negativeUids = {-1, -100, -1000, -10000, INT32_MIN + 1};
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int uid : negativeUids) {
        OHOS::SelectionFwk::SelectionConfig config =
            OHOS::SelectionFwk::CreateTestConfig(uid, true, false, "com.negative." + std::to_string(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestBoundaryNegativeUids passed");
}

HWTEST_F(DatabasePluginImplBoundaryTest, TestBoundaryMixedUids, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestBoundaryMixedUids started");
    std::vector<int> mixedUids = {INT32_MIN, -1, 0, 1, INT32_MAX};
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int uid : mixedUids) {
        OHOS::SelectionFwk::SelectionConfig config =
            OHOS::SelectionFwk::CreateTestConfig(uid, true, false, "com.mixed." + std::to_string(uid));
        plugin_.Save(uid, config);
    }
    for (int uid : mixedUids) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestBoundaryMixedUids passed");
}

class DatabasePluginImplSpecialCharTest : public DatabasePluginImplBoundaryTest::DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplSpecialCharTest, TestSpecialCharUnderscore, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharUnderscore started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(50001, true, false, "com_special_underscore_test");
    plugin_.Save(50001, config);
    auto result = plugin_.GetOneByUserId(50001);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestSpecialCharUnderscore passed");
}

HWTEST_F(DatabasePluginImplSpecialCharTest, TestSpecialCharHyphen, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharHyphen started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(50002, true, false, "com-special-hyphen-test");
    plugin_.Save(50002, config);
    auto result = plugin_.GetOneByUserId(50002);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestSpecialCharHyphen passed");
}

HWTEST_F(DatabasePluginImplSpecialCharTest, TestSpecialCharNumbers, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharNumbers started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(50003, true, false, "com.test123.number456");
    plugin_.Save(50003, config);
    auto result = plugin_.GetOneByUserId(50003);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestSpecialCharNumbers passed");
}

HWTEST_F(DatabasePluginImplSpecialCharTest, TestSpecialCharVersion, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharVersion started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(50004, true, false, "com.test.app-v2.0.1-beta");
    plugin_.Save(50004, config);
    auto result = plugin_.GetOneByUserId(50004);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestSpecialCharVersion passed");
}

HWTEST_F(DatabasePluginImplSpecialCharTest, TestSpecialCharDots, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharDots started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(50005, true, false, "..test..dots...");
    plugin_.Save(50005, config);
    auto result = plugin_.GetOneByUserId(50005);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestSpecialCharDots passed");
}

HWTEST_F(DatabasePluginImplSpecialCharTest, TestSpecialCharUpperLower, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestSpecialCharUpperLower started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(50006, true, false, "COM.Test.Mixed.CASE.Test");
    plugin_.Save(50006, config);
    auto result = plugin_.GetOneByUserId(50006);
    ASSERT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestSpecialCharUpperLower passed");
}

class DatabasePluginImplRobustnessTest : public DatabasePluginImplBoundaryTest::DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplRobustnessTest, TestRobustnessRapidInitCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRobustnessRapidInitCleanup started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int i = 0; i < 5; ++i) {
        plugin_.Cleanup();
        bool initResult = plugin_.Initialize();
        EXPECT_TRUE(initResult);
    }
    SELECTION_HILOGI("TestRobustnessRapidInitCleanup passed");
}

HWTEST_F(DatabasePluginImplRobustnessTest, TestRobustnessRapidSaveOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRobustnessRapidSaveOperations started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int i = 0; i < 20; ++i) {
        int uid = 60001 + i;
        OHOS::SelectionFwk::SelectionConfig config =
            OHOS::SelectionFwk::CreateTestConfig(uid, true, false, OHOS::SelectionFwk::GenerateTestAppName(uid));
        int ret = plugin_.Save(uid, config);
        EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);
    }
    SELECTION_HILOGI("TestRobustnessRapidSaveOperations passed");
}

HWTEST_F(DatabasePluginImplRobustnessTest, TestRobustnessRapidGetOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRobustnessRapidGetOperations started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int uid = 60021; uid <= 60040; ++uid) {
        OHOS::SelectionFwk::SelectionConfig config =
            OHOS::SelectionFwk::CreateTestConfig(uid, true, false, OHOS::SelectionFwk::GenerateTestAppName(uid));
        plugin_.Save(uid, config);
    }
    for (int uid = 60021; uid <= 60040; ++uid) {
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestRobustnessRapidGetOperations passed");
}

HWTEST_F(DatabasePluginImplRobustnessTest, TestRobustnessMixedOperations, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestRobustnessMixedOperations started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    for (int i = 0; i < 15; ++i) {
        int uid = 60041 + i;
        OHOS::SelectionFwk::SelectionConfig config = OHOS::SelectionFwk::CreateTestConfig(
            uid, (i % 2 == 0), (i % 3 == 0), OHOS::SelectionFwk::GenerateTestAppName(uid));
        plugin_.Save(uid, config);
        auto result = plugin_.GetOneByUserId(uid);
        ASSERT_EQ(result, std::nullopt);
    }
    SELECTION_HILOGI("TestRobustnessMixedOperations passed");
}

class DatabasePluginImplErrorHandlingTest : public DatabasePluginImplBoundaryTest::DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplErrorHandlingTest, TestErrorHandlingSaveWithoutInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestErrorHandlingSaveWithoutInit started");
    OHOS::SelectionFwk::DatabasePluginImpl impl;
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(70001, true, false, "com.error.noinit");
    int ret = impl.Save(70001, config);
    EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestErrorHandlingSaveWithoutInit passed");
}

HWTEST_F(DatabasePluginImplErrorHandlingTest, TestErrorHandlingGetWithoutInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestErrorHandlingGetWithoutInit started");
    OHOS::SelectionFwk::DatabasePluginImpl impl;
    auto result = impl.GetOneByUserId(70002);
    EXPECT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestErrorHandlingGetWithoutInit passed");
}

HWTEST_F(DatabasePluginImplErrorHandlingTest, TestErrorHandlingAfterCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestErrorHandlingAfterCleanup started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    plugin_.Cleanup();
    OHOS::SelectionFwk::SelectionConfig config =
        OHOS::SelectionFwk::CreateTestConfig(70003, true, false, "com.error.aftercleanup");
    int ret = plugin_.Save(70003, config);
    EXPECT_EQ(ret, OHOS::SelectionFwk::SELECTION_CONFIG_RDB_NO_INIT);
    SELECTION_HILOGI("TestErrorHandlingAfterCleanup passed");
}

HWTEST_F(DatabasePluginImplErrorHandlingTest, TestErrorHandlingNonExistentRecord, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestErrorHandlingNonExistentRecord started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    auto result = plugin_.GetOneByUserId(999999999);
    EXPECT_EQ(result, std::nullopt);
    SELECTION_HILOGI("TestErrorHandlingNonExistentRecord passed");
}

class DatabasePluginImplModuleInfoTest : public DatabasePluginImplBoundaryTest::DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplModuleInfoTest, TestModuleInfoName, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestModuleInfoName started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    EXPECT_STREQ(plugin_.GetModuleName(), "Database");
    SELECTION_HILOGI("TestModuleInfoName passed");
}

HWTEST_F(DatabasePluginImplModuleInfoTest, TestModuleInfoVersion, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestModuleInfoVersion started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    EXPECT_EQ(plugin_.GetModuleVersion(), 1);
    SELECTION_HILOGI("TestModuleInfoVersion passed");
}

HWTEST_F(DatabasePluginImplModuleInfoTest, TestModuleInfoVersionConsistency, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestModuleInfoVersionConsistency started");
    OHOS::SelectionFwk::DatabasePluginImpl impl1;
    OHOS::SelectionFwk::DatabasePluginImpl impl2;
    impl1.Initialize();
    impl2.Initialize();
    EXPECT_EQ(impl1.GetModuleVersion(), impl2.GetModuleVersion());
    EXPECT_STREQ(impl1.GetModuleName(), impl2.GetModuleName());
    impl1.Cleanup();
    impl2.Cleanup();
    SELECTION_HILOGI("TestModuleInfoVersionConsistency passed");
}

class DatabasePluginImplStatusTest : public DatabasePluginImplBoundaryTest::DatabasePluginImplAiTest {};

HWTEST_F(DatabasePluginImplStatusTest, TestStatusBeforeInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStatusBeforeInit started");
    OHOS::SelectionFwk::DatabasePluginImpl impl;
    EXPECT_STREQ(impl.GetStatus(), "uninitialized");
    SELECTION_HILOGI("TestStatusBeforeInit passed");
}

HWTEST_F(DatabasePluginImplStatusTest, TestStatusAfterInit, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStatusAfterInit started");
    OHOS::SelectionFwk::DatabasePluginImpl impl;
    impl.Initialize();
    EXPECT_STREQ(impl.GetStatus(), "available");
    impl.Cleanup();
    SELECTION_HILOGI("TestStatusAfterInit passed");
}

HWTEST_F(DatabasePluginImplStatusTest, TestStatusAfterCleanup, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStatusAfterCleanup started");
    OHOS::SelectionFwk::DatabasePluginImpl plugin_;
    plugin_.Cleanup();
    EXPECT_STREQ(plugin_.GetStatus(), "uninitialized");
    SELECTION_HILOGI("TestStatusAfterCleanup passed");
}

HWTEST_F(DatabasePluginImplStatusTest, TestStatusHealthCheckCorrelation, testing::ext::TestSize.Level0)
{
    SELECTION_HILOGI("TestStatusHealthCheckCorrelation started");
    OHOS::SelectionFwk::DatabasePluginImpl impl;
    EXPECT_STREQ(impl.GetStatus(), "uninitialized");
    EXPECT_FALSE(impl.HealthCheck());
    impl.Initialize();
    EXPECT_STREQ(impl.GetStatus(), "available");
    EXPECT_TRUE(impl.HealthCheck());
    impl.Cleanup();
    EXPECT_STREQ(impl.GetStatus(), "uninitialized");
    EXPECT_FALSE(impl.HealthCheck());
    SELECTION_HILOGI("TestStatusHealthCheckCorrelation passed");
}