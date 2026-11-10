# 编码铁律

## 生成代码与接口边界

- **禁止**手改任何生成产物：`interfaces/idl/*.idl` 生成的 `*_proxy.cpp`/`*_stub.cpp`（位于 `${target_gen_dir}`）、`frameworks/ets/taihe/*/src` 的 Taihe 胶水。采用"改 `.idl` / 改源 → 重建再生成"。依据：`interfaces/idl/BUILD.gn`（`idl_gen_interface`）、AGENTS.md Do-not 清单。
- **禁止**给 `libselection_client.z.so` 加导出符号而不更新 `selection_client.versionscript`（只有匹配 `*SelectionClient*` 的符号可见，其余 `local:*` 隐藏）；同理 `service/selection_service.map` 是 SA so 的显式白名单，新增导出必须同步改 map。隐藏失败是静默的（链接"成功"但运行时找不到符号）。
- **禁止**在 `service/`（主服务）引入 `ability_runtime:ability_manager` 依赖——该依赖只允许出现在 `service/plugins/BUILD.gn`。主服务经 dlsym 函数指针调用插件（`service/BUILD.gn` 内有警告注释）。违反即破坏裁剪与 CFI 边界。
- **禁止**新增 `external_deps` 而不在 `bundle.json` 的 `component.deps.components` 中声明——构建直接失败；跨部件依赖属部件边界变更，先声明后使用。

## 并发与锁

- **禁止**在 `service/src/selection_input_monitor.cpp` 与 `utils/src/selection_timer.cpp` 加并发代码前不检查既有锁：这两处是高频变更区（git 近期提交记录），状态机已用 `std::atomic` 状态位 + `shared_mutex selectionInfoMutex_` 保护（`service/include/selection_input_monitor.h:99-103`），重复加锁或锁序不一致会死锁状态机。
- **禁止**跨线程直接触发 JS 回调，采用统一三段式：`EventHandler::PostTask(..., EventQueue::Priority::VIP)` → `JsCallbackHandler::Traverse`（NAPI，`common/callback_handler.cpp`）/ ANI 侧向量遍历；面板操作一律经 `JsPanel::jsQueue_`（`FFRTBlockQueue`，10s 超时）串行化。依据：`frameworks/js/napi/selection_ability/js_panel.cpp`、`common/ffrt_block_queue.h`。
- **禁止**在持有 `pluginMutex_`/`mtx_` 时做 dlopen/dlclose 或条件等待超过既有时长约定（插件卸载定时器 300000ms、剪贴板等待 110ms、扩展连接 5s）；异步等待用 `std::async`/`future`，不用忙轮询。
- **禁止**裸持有 uinput fd 而不套 `FdsanMark/FdsanClose`（`utils/include/selection_fdsan.h`，owner tag = GENERIC_00 + LOG_DOMAIN）；服务 `OnStart` 已设 `fdsan_set_error_level(FATAL)`，fd 泄漏会直接 fatal。

## 安全与权限

- **禁止**绕过或放宽 `SelectionAppValidator`：`RegisterListener/UnregisterListener/GetSelectionContent` 三个 IPC 入口必须校验 CallingTokenID → HapTokenInfo → bundleName 与 `sys.selection.app` 前缀一致（`service/src/selection_app_validator.cpp`），失败返回 `UNAUTHENTICATED_ERROR`。加白或跳过校验=任意应用可窃取选中文本。
- **禁止**改动 `etc/init/selection_service.cfg`（uid/gid/secon/permission/ACL）、`etc/para/selection.para.dac`（DAC 0775 sysselection）而不走安全评审；同样 `sys.selection.*` 默认值与 `sa_profile/8500.json` 的按需拉起项变更会影响真机加载时机。
- **禁止**在面板或内容链路新增权限申请而不核对现有 8 权限 + ACL（`INPUT_MONITORING`、`READ_PASTEBOARD`、`CONNECT_SELECTION_EXTENSION`、`SYSTEM_FLOAT_WINDOW` 等，见 `etc/init/selection_service.cfg`）。

## 构建加固（新增/拷贝 BUILD.gn 时必须保留）

- **禁止**丢块：所有产物必须携带完整 `sanitize = { boundary_sanitize/cfi/cfi_cross_dso/cfi_vcall_icall_only=true, debug=false, integer_overflow=true, ubsan=true }` + `branch_protector_ret = "pac_ret"`；共享库加 `ldflags = ["-Wl,--exclude-libs,ALL"]`（+ `-Wl,-z,now`）。干净构建即静态检查门——**修复方向是改代码，不是弱化 sanitize 块**。
- **禁止**让 `service/plugins/BUILD.gn` 的 CFI 配置偏离 `relational_store:native_rdb`：跨 DSO CFI 要求两侧一致（该 BUILD.gn 注释明确）。
- 新增 `declare_args()` 只放 `selection_service.gni`（现有唯一开关 `word_selection_feature_test_one`），不放各 BUILD.gn。

## 编码与风格

- **禁止**脱离统一设施自造轮子：日志用 `utils/include/selection_log.h` 的 `SELECTION_HILOG{F,E,W,I,D}` + `SELECTION_CHECK`（domain `0xD002901`，tag `SELECTION_SERVICE`）；错误码用 `utils/include/selection_errors.h` 枚举（`SelectionServiceError` 1..7、`SelectionConfigErrCode`、`PluginLoaderError` -100..-107）与 `common/selection_js_utils.h` 的 `SFErrorCode`（202/401/33600001..33600008），不自造裸数字。
- **禁止**在插件 so 内导出 C++ 符号：唯一导出面是 `plugin_exports.cpp` 的 `extern "C"` + `visibility("default")`，其余一律隐藏（`-fvisibility=hidden`）。
- **禁止**修改 JS 事件名白名单外的字符串直接比较：事件合法性走 `EventChecker::IsValidEventType`（`common/event_checker.cpp`：`selectionCompleted`/`destroyed`/`hidden`）。
- **禁止**提交未标注预留状态的未接线代码：`utils/src/selection_content_validator.cpp`、`selection_event_bus.cpp`、`selection_rate_limiter.cpp`、`selection_geometry_utils.cpp`、`common/data_converter/*` 均未接入任何 BUILD.gn——引用它们写"规划中能力"而非现行机制（各文件头注释已声明）。
- 约定：C++17、命名空间 `OHOS::SelectionFwk`（扩展模块 `OHOS::AbilityRuntime`）、服务代码 `using namespace MMI;`；Apache-2.0 版权头年份保持当前（生产文件 2025、测试 2026）；单测只用 `HWTEST_F(Fixture, Name, TestSize.LevelN)` 宏（gtest/gmock），私有成员经 `-Dprivate=public -Dprotected=public` 暴露。

## 测试与验证

- 单测目标 `selectionfwk_test`（`test/unittest:selection_manager_ut`）；fuzz（`selectioninputability_fuzzer`/`selectioninputlistener_fuzzer`）覆盖 IPC 入口；新增测试同步登记 `bundle.json` 的 `component.build.test`。
- 改动 `interfaces/inner_kits/selection_client/include/selection_client.h` 必须确认 versionscript 与 ABI；改动 `.idl` 必须确认重新生成的 proxy/stub 可编译。
- 无设备时明确说"构建/测试未执行"，不编造通过标记。
