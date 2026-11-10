# 术语表

## 划词（word selection / selectionfwk）
用户在任意应用中选中文本（滑动/双击/三击）后，系统捕获该文本并交给划词扩展应用处理的全流程。部件名 `selectionfwk`（历史 GN 变量 `word_selection_part_name="word_selection"` 与 `part_name="selectionfwk"` 并存，属命名遗留）。syscap `SystemCapability.SelectionInput.Selection`。

## SA 8500 / selection_service 进程
本子系统的 SystemAbility（id 8500，宏 `SELECTION_FWK_SA_ID`，定义于 samgr `system_ability_definition.h:337`）。进程名 `selection_service`，由 `sa_main` 按 `etc/init/selection_service.cfg` 拉起：ondemand、importance -20、uid/gid `sysselection`、APL `system_basic`、SELinux `u:r:selection_service:s0`。按需加载：参数 `sys.selection.switch=on` 或事件 `usual.event.USER_SWITCHED`；`switch=off` 或自卸载（`UnloadSystemAbility(8500)`）时退出。单进程单实例，多用户在实例内按前台 userId 切换配置。

## 划词扩展应用（SelectionExtensionAbility / 扩展类型 31）
承接划词事件并渲染智能面板的扩展应用（默认 `com.selection.selectionapplication/SelectionExtensionAbility`）。以 ExtensionAbility 形态存在：模块加载器 `SelectionExtensionModuleLoader` 注册 `type="31"`、`name="SelectionExtensionAbility"`；服务端经插件 `AbilityManagerClient::ConnectAbility(Want)` 建立连接（5 秒超时）。当前扩展由系统参数 `sys.selection.app` 指定，无动态注册/发现机制。

## 划词信息（SelectionInfo / SelectionInfoData）
一次划词事件的载体：`SelectionType`（`MOVE_SELECTION=1`、`DOUBLE_CLICKED_SELECTION=2`、`TRIPLE_CLICKED_SELECTION=3`）+ 起/止 display/window 坐标 + displayId/windowId/bundleName（`common/selection_interface.h`）；IPC 侧由 `SelectionInfoData : Parcelable` 包装（`common/selection_data_inner.h`，selectionType 以 Int8 序列化）。经 `ISelectionListener::OnSelectionChange` 推给已注册的划词应用。

## 焦点变化信息（SelectionFocusChangeInfo）
面板外窗口获焦的通知载体：windowId/displayId/pid/uid/windowType/isFocused + `FocusChangeSource`（`WindowManager` 或 `InputManager` 双来源）。WMS 路径来自 `FocusMonitorManager` 注册的 `IFocusChangedListener`；MMI 路径在 BUTTON_DOWN 时经 `GetFocusWindowInfo` 判定。应用侧收到后执行 `Dispose`（MENU_PANEL 隐藏、其余销毁）。

## 面板（Panel / MENU_PANEL / MAIN_PANEL）
划词扩展应用进程内的 `WINDOW_TYPE_SELECTION` + `WINDOW_MODE_FLOATING` 浮动窗口。两种类型：`MENU_PANEL=1`（菜单窗 "SelectionMenuFwkWindow"）、`MAIN_PANEL=2`（主窗 "SelectionMainFwkWindow"）。生命周期 `CreatePanel→SetUiContent→Show/Hide/StartMoving/MoveTo→DestroyPanel`；状态 `STATE_SHOWN/HIDDEN/DESTROYED` 经 `PanelStatusListener` 以 `"shown"/"hidden"/"destroyed"` 字符串回调 JS。面板窗口创建在扩展应用进程内（`createPanel(ctx, info)` 传入应用 Context），SA 只用 `IsAnySelectionPanelShowing()` 探测保活。

## 触发方式（trigger / ctrl / immediate）
取词时机配置 `sys.selection.trigger`：`ctrl`（默认）——状态机识别完选区后停在 WORD_END，等用户按 Ctrl（`KEYCODE_CTRL_LEFT/RIGHT`）才 DONE 并通知；`immediate`——识别完立即 DONE。内存镜像 `MemSelectionConfig::GetTriggered()` 读取。

## 输入状态机（SelectInputState / SelectInputSubState）
划词识别的核心模型（`service/include/selection_input_monitor.h:41-58`）：主态 `INITIAL → WORD_BEGIN → WAIT_LEFT_MOVE / WAIT_DOUBLE_CLICK → WAIT_TRIPLE_CLICK → WORD_END → DONE`；子态跟踪按键与指针细节（CTRL_DOWN/UP、BUTTON_DOWN/UP）。关键阈值：双击间隔 `DOUBLE_CLICK_TIME=550ms`、三击 `TRIPLE_CLICK_TIME=300ms`、微动判定 `MAX_POSITION_CHANGE_OFFSET=10px`（`IsTinyMovement`）。非左键一律 `ResetState()`。

## Ctrl+C 注入（Selection VKeyboard / uinput）
取词手段：经 `/dev/uinput` 创建虚拟键盘设备 `"Selection VKeyboard"`，按 `KEY_LEFTCTRL↓ → KEY_C↓ → KEY_C↑ → KEY_LEFTCTRL↑`（各步 SYN、间隔 1ms/60ms）序列注入，触发目标应用执行系统复制（`service/plugins/src/pasteboard/selection_pasteboard_manager.cpp:185-247`）。fd 用 `FdsanMark/FdsanClose` 保护。

## 剪贴板一次性订阅（SubscribeDisposableObserver / PLAIN_TEXT）
取词回执通道：`PasteboardClient::SubscribeDisposableObserver(observer, windowId, DisposableType::PLAIN_TEXT, 2000*3)`——按 windowId 限定、只要纯文本、上限 6000 字节（2000 字符×3 字节）；服务在注入前订阅、`cv_.wait_for(110ms)` 等待 `OnTextReceived` 回调后写入全局内容并 notify。错误码映射 `PB_ERR_OUT_OF_RANGE=5→CONTENT_OUT_OF_RANGE`、`PB_ERR_CANNOT_GET_CONTENT=7→CANNOT_GET_CONTENT`（应用禁止复制）、超时→`GET_CONTENT_TIMEOUT`。

## 空白校验（IsAllWhitespace）
剪贴板回调侧对取到文本的过滤：覆盖 ASCII 空白与全套 Unicode 空白/零宽字符（`\u00A0/\u1680/\u180E/U+2000-200F/U+2028-202E/U+205F-2064/\u3000/\uFEFF`），全空白视为无效内容（`selection_pasteboard_manager.cpp:66`）。6000 字节上限同函数簇体现；`utils/src/selection_content_validator.cpp` 的 `CONTENT_MAX_BYTES=6000` 是未接线的预留实现。

## 划词配置（SelectionConfig / 双轨持久化）
四个配置项：`isEnabled_`（开关）、`isTriggered_`（触发方式）、`applicationInfo_`（扩展应用）、`uid_`（用户绑定，默认 -1）。持久化双轨：系统参数 `sys.selection.{switch,trigger,app,uid}`（快速/全局）+ RDB `/data/service/el1/public/selection_service/selection_config.db` 表 `selection_config`（用户级，S1，`DATABASE_OPEN_VERSION=2`）。同步由 `SelectionConfigComparator::DoCompare` 裁决方向（`FromDbToSys`/`FromSysToDb`）与衍生动作（`shouldStop/shouldStart/shouldRestartApp`）。

## 配置比较器（SelectionConfigComparator）
双轨同步的单点裁决器（`service/src/selection_config_comparator.cpp:93-143`）：输入 sys 参数快照 + DB 记录，输出 `ComparisionResult{direction, shouldCreate, shouldStop, shouldStart, shouldRestartApp, selectionConfig}`；DB 无记录时以 sys 参数为默认并回写。`Init`/USER_SWITCHED/参数 watcher 均经它同步。

## 插件 so（libselection_plugins_impl.z.so）
pasteboard/RDB/ability 三个重依赖的解耦产物：主服务 dlopen + dlsym 9 个 C 函数（`service/include/selection_service.h:159-203`），唯一导出面 `service/plugins/src/plugin_exports.cpp`（`extern "C"` + `visibility("default")`）。闲置 5 分钟（`PLUGIN_UNLOAD_TIMEOUT_MS=300000`）自动 dlclose（面板显示或扩展连接时保留），dlclose 前等 50ms。

## IDL proxy/stub（ISelectionService / ISelectionListener）
IPC 契约：`ISelectionService`（RegisterListener/UnregisterListener/GetSelectionContent）与 `ISelectionListener`（OnSelectionChange/FocusChange），由 `idl_tool` 的 `idl_gen_interface` 从 `interfaces/idl/*.idl` 生成到 `${target_gen_dir}`，禁止手改。

## inner-api（libselection_client.z.so）
面向系统应用的 C++ 客户端（`interfaces/inner_kits/selection_client`，`platformsdk` 标签）：`SelectionClient::GetInstance().GetSelectionContent()` → `GetSystemAbility(8500)` → IPC。versionscript 只导出 `*SelectionClient*`；源码与 `frameworks/native/selection_client` 共享。

## Taihe FFI / ANI
`selectionManager`/`selectionPanel` 模块的方舟原生接口（ANI）实现路径：`frameworks/ets/taihe/*/idl/*.taihe` 经 `taihe_ffi_gen` 生成 C++ 胶水（`selectionManager_taihe_native.z`），boot abc 落 `/system/framework/selectionManager.abc` 等。区别于 NAPI（`frameworks/js/napi`）：同一 JS API 的两条运行时通道。

## 扩展连接（SelectionExtensionAbilityConnection）
SA 侧 `AAFwk::AbilityConnectionStub` 实现：`OnAbilityConnectDone` 记录 `AbilityRuntimeInfo{userId,bundleName,abilityName}`，配合 `std::promise` + 5s `wait_for` 同步等待连接结果（`WaitForConnect`）。断链对称（`DoDisconnectCurrentExtAbility`）。超时上报 `CONNECT_EXTENSION_TIMEOUT`/`DISCONNECT_EXTENSION_TIMEOUT`。

## sceneboard（window_manager_use_sceneboard）
全局 GN 开关：开→依赖 `libwm_lite` + 定义 `SCENE_BOARD_ENABLE`（运行时走 `MoveWindowToGlobalDisplay` 等分支）；关→依赖 `libwm`。涉及 window_manager 的 BUILD.gn 与运行时代码必须保持双分支一致。

## 取词频控（MAX_CALLS_PER_WINDOW）
JS 层 `getSelectionContent` 的滑动窗口限频：500ms 内最多 50 次调用，超出抛 `EXCEPTION_TOO_FREQUENT(33600004)`（`frameworks/js/napi/selection_ability/js_selection_engine_setting.cpp`）。防止应用无限轮询取词。

## 取词时序 flag（CanGetSelectionContent / selectionEventId_)
防竞态机制：划词事件完成后 `SetCanGetSelectionContentFlag(true)` 且 `selectionEventId_++`；应用请求取词时校验 flag 与时机（`INVALID_TIMING`），取词期间再比对 eventId 是否变化（变化则恢复 flag 并拒绝），保证一次事件只允许一次取词。

## SELECTIONFWK 事件域
hisysevent domain（`hisysevent-SELECTIONFWK.yaml`）：`SELECTION_PROCESS_ABNORMAL`（FAULT：FAIL_REASON `SelectFailedReason` 0..4 = 注入失败/文本接收失败/面板创建失败/连接超时/断开超时 + BUNDLE_NAME/ERROR_CODE）、`SELECTION_STATISTIC`（STATISTIC：12 小时周期上报触发/失败计数后清零）。另有 hiappevent `api_diagnostic`（SDK_OCG）记录 createPanel API 级时延。

## hisysevent 定时器（SelectionFwkTimer / 12h）
`utils/src/selection_timer.cpp` 包装 `Utils::Timer("selectionFwkTimer", -1)` 独立定时器线程：承载插件 5 分钟闲置卸载与 hisysevent 12 小时统计两个周期任务（`SELECTION_HISYSEVENT_REPORT_TIME = 12*60*60*1000`）。

## 应用校验器（SelectionAppValidator）
IPC 入口准入：`IPCSkeleton::GetCallingTokenID()` → `AccessTokenKit::GetHapTokenInfo` → 调用方 bundleName 与 `sys.selection.app` 前缀（`/` 截断）比对，用于 Register/Unregister/GetSelectionContent 三入口；应用侧另有 framework 版（模块加载时 `SetValid()` 置位）。区别于进程权限：进程级 8 个 `ohos.permission.*` 在 `selection_service.cfg`。

## 预留实现（未接线代码）
仓内存在但未接入任何 BUILD.gn 的规划能力：`utils/src/selection_content_validator.cpp`（6000 字节校验）、`selection_event_bus.cpp`、`selection_rate_limiter.cpp`、`selection_geometry_utils.cpp`、`common/data_converter/*`。写文档/引用时必须标注"规划中"，不可当作现行机制。
