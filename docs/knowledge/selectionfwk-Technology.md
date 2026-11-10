# 技术术语知识

selectionfwk 部件核心技术框架和机制的知识定义，涵盖价值认知、原理、使用方式和代码关联。

## SA 8500 按需生命周期与延迟初始化

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 划词是低频能力，常驻进程浪费内存；同时其依赖（MMI/WMS/公共事件）在开机早期顺序不定，直接初始化会因依赖未就绪而失败 |
| 技术目标 | 服务零成本闲置（按需加载/卸载），且对三个上游 SA 的初始化只在它们就绪后发生 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 注册与拉起：`SystemAbility::MakeAndRegisterAbility(SelectionService::GetInstance().GetRefPtr())`（selection_service.cpp:55），构造 `SystemAbility(8500, true)`；`sa_profile/8500.json` 定义 start-on-demand（param `sys.selection.switch=on`、commonevent `usual.event.USER_SWITCHED`）、stop-on-demand（`switch=off`）、`BootStartPhase`、`auto-restart:true`、`run-on-create:false`。启动序：`OnStart`（:850）→ fdsan FATAL → `WatchParameter("bootevent.boot.completed", WatchBootCompleted)`（失败立即 Init）→ `Publish(GetInstance())` → `StartHisyseventTimer()`；`Init`（:419）→ Comparator::Init → `SynchronizeSelectionConfig()`（若 `shouldStop` → `DisableSAService()+UnloadService()`）→ `RegisterSystemAbilityStatusChangeListener()` → `WatchParams()`。延迟初始化：`InitSystemAbilityChangeHandlers`（:872）订阅 MMI(3101)/WMS(4606)/CES(3299) 就绪回调，分别做 `InputMonitorInit()`（AddMonitor）、`InitFocusChangedMonitor()`、`SubscribeSysEventReceiver()`。退出：`OnStop`（:864）→ `Shutdown()`（逐项反注册）→ `UnloadPluginSo()`；主动卸载 `UnloadService()` 调 `samgr->UnloadSystemAbility(8500)`（:1074-1088） |
| 关键流程 | 进程身份由 `etc/init/selection_service.cfg` 提供：ondemand、importance -20、uid/gid `sysselection`、APL `system_basic`、secon `u:r:selection_service:s0`、8 权限 + ACL；on-start job 建 `/data/service/el1/public/selection_service`（0770 sysselection） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 面向系统：参数 `sys.selection.*`（默认值 `etc/para/selection.para`，DAC 见 `selection.para.dac`）；面向开发者：`SelectionClient`（inner-api）；用户开关=设置 `sys.selection.switch`（off 时 SA 自卸载） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionService : SystemAbility, SelectionServiceStub`（service/include/selection_service.h） |
| 核心成员 | `SELECTION_FWK_SA_ID=8500`；`SystemAbilityStatusChangeListener`（就绪回调表） |
| 核心方法 | `OnStart/OnStop/Init/Shutdown/UnloadService/InitSystemAbilityChangeHandlers/InputMonitorInit/WatchBootCompleted`（selection_service.cpp） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 按需三件套（sa_profile/8500.json + selection_service.cfg + selection.para(.dac)）耦合变更；SA id 宏在 samgr 侧定义（system_ability_definition.h:337），不可本地改 |
| 并发约束 | Init 可能由参数回调或 SA 回调触发，`initMutex_` 保护；Shutdown 与 Init 竞态由 `isShutdown_` 原子位挡 |
| 安全约束 | 进程 uid/secon/权限清单变更需安全评审；`sys.selection.*` DAC 只允许 sysselection 组写 |

## 划词识别状态机（事件→选区类型）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 系统无法直接得知"用户选中了文本"：MMI 只给原始键鼠事件流，必须在线识别单击滑动/双击/三击三种选词手势，并按触发方式决定何时通知 |
| 技术目标 | 用确定性状态机把事件流折叠为选区类型（MOVE/DOUBLE/TRIPLE），正确处理超时、微动、非左键、Ctrl 触发等边界 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 状态机见实体文档；本机制补充分发前置链（`FinishedWordSelection`，selection_input_monitor.cpp:596）：无扩展连接→`ConnectExtAbilityFromConfig()` + `ResetPluginUnloadTimer()` → 锁屏跳过 → bundleName 空告警 → `IsAppInBlocklist`（空）→ 开关关跳过 → `SetCanGetSelectionContentFlag(true)` → `selectionEventId_++` → `listener->OnSelectionChange`。起点信息在 WORD_BEGIN 经 `WindowManager(Lite)::ListWindowInfo(windowId)` 反查 bundleName（:254-300）；焦点伴随判定 `HandleWindowFocused`（:560，BUTTON_DOWN 时 `GetFocusWindowInfo`，点击窗即焦点窗则发 FocusChangeInfo） |
| 关键流程 | 触发分叉：`GetCtrlSelectFlag()`（=trigger 配置）为真→WORD_END 等 Ctrl（KeyEvent 只认 `KEYCODE_CTRL_LEFT/RIGHT`，重复键忽略，UP→DONE）；为假→直接 DONE。三击窗口由双击 DONE 态微动（≤10px）延续 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无对外独立 API；行为参数化项=trigger 配置（ctrl/immediate）；单测入口 `selection_input_monitor_test.cpp`（含 `_ctrl_test.cpp`，gmock `MockBaseSelectionInputMonitor`） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionInputMonitor`/`BaseSelectionInputMonitor`（MMI::IInputEventConsumer） |
| 核心成员 | `DOUBLE_CLICK_TIME=550ms`、`TRIPLE_CLICK_TIME=300ms`、`MAX_POSITION_CHANGE_OFFSET=10px`、`MAX_PASTERBOARD_TEXT_LENGTH=2000`、`BYTES_PER_CHINESE_CHAR=3` |
| 核心方法 | `OnInputEvent`/各 `Input*Process`/`JudgeTripleClick`/`ProcessWordSelection`/`FinishedWordSelection`/`ResetState`/`HandleWindowFocused`（selection_input_monitor.cpp） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 只处理左键；无键鼠（纯触屏）不产生划词（autotest `selectionfwk_touch_screen` 验证）；坐标系依赖 displayId/扩展屏 |
| 并发约束 | MMI 单分发线程回调；状态原子量 + shared_mutex 写坐标 |
| 安全约束 | 不适用（纯事件识别，无数据面） |

## 划词取词链路（Ctrl+C 注入 + 剪贴板一次性订阅）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 系统没有全局"选中文本"接口：用户选中的文本只存在于目标应用内部，只有触发目标应用自己的"复制"行为才能把文本送进剪贴板 |
| 技术目标 | 在应用请求 `getSelectionContent` 时，服务端模拟 Ctrl+C 复制、经剪贴板按窗口一次性订阅捕获文本，完成校验过滤后同步返回，全程限时 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 请求侧（SA）：`SelectionService::GetSelectionContent`（selection_service.cpp:263）→ `SelectionAppValidator::Validate()`（失败 `UNAUTHENTICATED_ERROR`）→ `CanGetPasteboardContent()` 时机不对（`INVALID_TIMING`）→ `inputMonitor_->GetSelectionContent()`（selection_input_monitor.cpp:636）：`AddSelectionCount()` → `SetCanGetSelectionContentFlag(false)` → 记 eventId → `GetPasteboardContent(content, windowId, bundleName)`（缓存 2000 字符×3+1，:1263-1286，经插件函数指针 `PasteboardGetSelectionContent`）→ 期间 eventId 变化则恢复 flag 返回 `INVALID_TIMING`。插件侧（`SelectionPasteboardManager::GetSelectionContent`，selection_pasteboard_manager.cpp:314）：`PasteboardClient::SubscribeDisposableObserver(pasteboardObserver_, windowId, DisposableType::PLAIN_TEXT, 2000*3)` → `std::async` 执行 `InjectCtrlC()` → `cv_.wait_for(110ms)`（`MAX_DELAY_WAIT_FOR_PB`）。注入实现：`InitUidev()`（:185）打开 `/dev/uinput`、UI_SET_EVBIT(EV_KEY)、UI_SET_KEYBIT(KEY_LEFTCTRL/KEY_C)，设备名 "Selection VKeyboard"；`InjectCtrlC()`（:239）序列 `CTRL↓→SYN→C↓→SYN→C↑→SYN→60ms→CTRL↑→SYN`；fd 用 `FdsanMark/FdsanClose`。回执：`OnTextReceived`（:111）errCode≠0→`ReportShowPanelFailed(TEXT_RECEIVE_FAILED)`；`IsAllWhitespace`（:66，ASCII + `\u00A0/\u1680/\u180E/U+2000-200F/U+2028-202E/U+205F-2064/\u3000/\uFEFF`）全空白视为无效 → 写全局 `g_selectionContent` + `cv_.notify_one()`。错误映射（:282）：`PB_ERR_OUT_OF_RANGE=5→CONTENT_OUT_OF_RANGE`、`PB_ERR_CANNOT_GET_CONTENT=7→CANNOT_GET_CONTENT`（应用禁止复制）、超时→`GET_CONTENT_TIMEOUT`、其余→`INVALID_DATA`（`SelectionServiceError`，utils/include/selection_errors.h:38-46） |
| 关键流程 | 频控（JS 层）：`JsSelectionEngineSetting::GetSelectionContent` 滑动窗口 500ms/50 次（`MAX_CALLS_PER_WINDOW/WINDOW_MS`），超出抛 `EXCEPTION_TOO_FREQUENT(33600004)` |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 应用侧 JS `getSelectionContent(): Promise<string>`（selectionManager 模块，NAPI 与 Taihe 双通道）；系统侧 `SelectionClient::GetInstance().GetSelectionContent()`（inner-api，IPC 到 SA） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionPasteboardManager`/`SelectionPasteboardDisposableObserver`（service/plugins/src/pasteboard/） |
| 核心成员 | `g_selectionContent`/`g_pasteBoardErrorCode`（全局 + mtx_/cv_）、`injectCtrlCFuture_`、`self_weak_` |
| 核心方法 | `GetSelectionContent`（SA :263 与 monitor :636 两级）、`GetPasteboardContent`（selection_service.cpp:1263）、`InitUidev/InjectCtrlC/OnTextReceived/IsAllWhitespace/PasteBoardErrorCodeToSelectionService`（selection_pasteboard_manager.cpp） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 订阅长度 2000 字符（×3 字节）即 6000 字节上限的现行实现（`selection_content_validator.cpp` 的 `CONTENT_MAX_BYTES=6000` 未接线，属预留）；等待窗 110ms；不可复制应用取词失败；一次事件仅允许一次取词（flag+eventId 联锁） |
| 并发约束 | 注入在 `std::async` 线程，回执在剪贴板回调线程，经 `cv_` 汇合；`weak_ptr` 防回执时对象已析构 |
| 安全约束 | 需要 `READ_PASTEBOARD` 进程权限；`CANNOT_GET_CONTENT` 尊重应用复制禁令；注入仅发生在用户已选中场景（时序 flag 前置） |

## 面板管理机制（浮窗生命周期与串行化）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 扩展应用需要一个紧贴划词位置的系统级浮窗呈现内容；而浮窗的创建/显示/移动/销毁若与 JS 线程、UI 加载竞速会产生黑窗、闪窗与悬挂回调 |
| 技术目标 | 提供进程内确定性顺序的面板生命周期 API，UI 加载完成后再显示，操作串行化，状态可靠回传 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 实体生命周期见实体文档；本机制补充三件套：①UI 就绪同步——`ShowPanel` 等待 `isWaitSetUiContent_`（`MAXWAITTIMES=3 × WAITTIME=100000us`）；②操作串行化——所有面板操作封装为 `JsEvent`（SHOW/HIDE/SET_UI_CONTENT/MOVE_TO/START_MOVING 等，js_panel.h:31-44）经 `FFRTBlockQueue<JsEventInfo> jsQueue_`（`MAX_WAIT_TIME=10ms`）FIFO 执行；③回调注册表——`PanelListenerImpl` 单例 `ConcurrentMap<uint32_t, map<string, vector<shared_ptr<JSCallbackObject>>>> callbacks_`（windowId→事件名→回调），事件经 `EventHandler::PostTask(..., VIP)` + `JsCallbackHandler::Traverse` 分发（panel_listener_impl.cpp:51-68）；ANI 版 `PanelListenerImpl`（taihe/SelectionManager）同步遍历。焦点联动（服务端双源）：WMS `IFocusChangedListener`（`FocusMonitorManager`）与 MMI BUTTON_DOWN 判定产生 `SelectionFocusChangeInfo` → 应用侧 `FocusChange` → `Dispose(windowId_)`。服务端探测：`IsAnySelectionPanelShowing()`（`GetVisibilityWindowInfo` 匹配配置 bundle/ability，selection_service.cpp:545-572）用于插件保活。sceneboard 分支：`SCENE_BOARD_ENABLE` 下 `MoveTo` 走 `MoveWindowToGlobalDisplay`（跨 display） |
| 关键流程 | 失败上报：面板创建失败（selection_panel.cpp:69）、连接/断开超时（selection_service.cpp:473/511）、注入失败（selection_pasteboard_manager.cpp:367）、文本接收失败（:116）→ `SELECTION_PROCESS_ABNORMAL`；hiappevent：createPanel 全路径打点（入参校验失败/执行失败/成功） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | JS：`createPanel(ctx, info): Promise<Panel>`、`destroyPanel(panel)`；`Panel`：`setUiContent(path)`、`show()/hide()`、`startMoving()`、`moveTo(x,y)`（sceneboard 另有 `moveToGlobalDisplay`）、`on/off('hidden'|'destroyed')`（NAPI `JsPanel`，js_panel.cpp:48-57；Taihe IDL 同构）。面板类型枚举经 `selectionInput.SelectionPanel` 模块导出 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionPanel`/`PanelStatusListener`（native）；`JsPanel`/`PanelListenerImpl`（NAPI/ANI） |
| 核心成员 | 窗口名映射表、`panels_` deque（SelectionAbility）、`jsQueue_` |
| 核心方法 | `CreatePanel/SetUiContent/ShowPanel/HidePanel/StartMoving/MoveTo/DestroyPanel/Dispose/IsAnySelectionPanelShowing` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 面板窗口属扩展应用进程（createPanel 的 ctx 决定）；SA 不持有面板对象，仅窗口可见性探测；`SYSTEM_FLOAT_WINDOW` 权限由服务配置承载 |
| 并发约束 | 应用侧操作串行（FFRTBlockQueue 10ms）；回调 VIP 队列不阻塞 IPC 线程 |
| 安全约束 | createPanel 走 `SelectionAppValidator`（framework 版，模块加载 `SetValid()` 置位）；窗口类型固定 `WINDOW_TYPE_SELECTION`，不可挪作他用 |

## 插件解耦与自动卸载机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | pasteboard、RDB、ability_runtime 是重依赖（CFI/链接面/裁剪面），若编进主服务 so，主服务无法裁剪且 CFI 配置耦合 |
| 技术目标 | 重依赖下沉到独立插件 so，主服务以最小面（9 个 C 函数指针）调用，闲置即卸载回收内存 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 加载：`dlopen("libselection_plugins_impl.z.so")` + dlsym 9 个函数（`PLUGIN_SO_PATH` 与指针表见 selection_service.h:159-203：pasteboard 取词、ability 连接/断开、DB 读写等），`pluginMutex_`（shared_mutex）保护；卸载：`PLUGIN_UNLOAD_TIMEOUT_MS=300000` 定时器（`ResetPluginUnloadTimer/OnPluginUnloadTimer`，:1193-1207）——到期时若面板在显（`IsAnySelectionPanelShowing`）则直接保留插件（不重置定时器）；若扩展已连接（`HasExtAbilityConnection`）则先 `DisconnectCurrentExtAbility()` 断开扩展连接，随后无条件 `UnloadPluginSo()`（dlclose 前 `CLEANUP_DELAY_TIME=50ms` 等待在途调用）；导出面：`plugin_exports.cpp` `extern "C"` + `visibility("default")`，其余符号 hidden；CFI：plugins/BUILD.gn 配置与 `relational_store:native_rdb` 一致并 `-Wl,-z,now` |
| 关键流程 | 事件到来时 `ResetPluginUnloadTimer()` 续命；`OnStop` 强制 `UnloadPluginSo()`；调用约定=函数指针直调，无 IPC 开销 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 内部机制；新插件函数=在 `plugin_exports.cpp` 增导出 + 主服务头文件增函数指针类型 + dlsym 表项，三处同步 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionService`（持有者）；`SelectionPasteboardManager`/`AbilityManagerPluginImpl`/`SelectionConfigDataBase`/`DatabasePluginImpl`（插件内实现） |
| 核心成员 | `PLUGIN_SO_PATH`、`PLUGIN_UNLOAD_TIMEOUT_MS=300000`、`CLEANUP_DELAY_TIME=50` |
| 核心方法 | `LoadPluginSo/UnloadPluginSo/ResetPluginUnloadTimer/OnPluginUnloadTimer`（selection_service.cpp）；`plugin_exports.cpp`（service/plugins/src/） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 插件 so 与主服务 ABI 强耦合（函数指针表），升级需同版本；插件内部可自有线程（如 pasteboard 的 async 注入） |
| 并发约束 | dlsym/dlclose 与调用并发由 shared_mutex 读写锁保护；50ms 延迟关闭窗口覆盖在途调用 |
| 安全约束 | 只从受控路径 dlopen；`PluginLoaderError` -100..-107（utils/include/selection_errors.h）；禁止插件内导出 C++ 符号 |

## 配置双轨同步机制（参数 + RDB 比较器）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 配置既要全局快速生效（系统参数），又要按用户隔离（多用户各一份）；两轨并存必然漂移，重启/切用户后以谁为准需要确定性规则 |
| 技术目标 | 单点比较器裁决两轨差异并给出动作集（覆盖方向、是否启停服务、是否重连扩展），用户切换时自动装载对应用户配置 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 数据面见"划词配置"实体；同步触发点：`Init`（启动）、USER_SWITCHED（切用户）、参数 watcher（实时变更）。流程：取前台 userId（`GetForegroundOsAccountLocalId`，重试 50×100ms，selection_service.cpp:667-680）→ 读 sys 参数 + `DatabasePluginImpl` 按 uid 读行（无记录用默认并回写）→ `SelectionConfigComparator::DoCompare`（selection_config_comparator.cpp:93-143）输出 `ComparisionResult{direction, shouldCreate, shouldStop, shouldStart, shouldRestartApp, selectionConfig}` → 按方向写回（`SyncConfigToSystem`：SetParameter；`SyncConfigToDatabase`：经插件事务写）→ 执行动作（shouldStop→`DisableSAService()+UnloadService()`；shouldRestartApp→`ReconnectExtAbility`）。运行读侧统一 `MemSelectionConfig`（mutex 保护 delegate），状态机/连接/校验器都从这里取 |
| 关键流程 | 参数 watcher（`WatchEnableSwitch/WatchTriggerMode/WatchAppSwitch`，:631）实时镜像到内存并 `PersistSelectionConfig()`（未登录用户不落库） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 设置面：`sys.selection.*` 参数（DAC 0775 sysselection）；代码面：`MemSelectionConfig::Get*()`、`SysSelectionConfigRepository::Get/Set*`；测试面：`selection_config_test/comparator_test/database_test/sys_selection_config_repository_test.cpp`、mock 版 `mock_parameter.cpp` |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionConfigComparator`/`ComparisionResult`/`MemSelectionConfig`/`SysSelectionConfigRepository`/`SelectionConfigDataBase` |
| 核心成员 | 键常量 `sys.selection.switch/trigger/app/uid`；`BUFFER_LEN=200`、`SELECTION_MAX_UID_LENGTH=11`、`DATABASE_OPEN_VERSION=2`、`DEFAULT_UID=-1` |
| 核心方法 | `SynchronizeSelectionConfig`/`WatchParams`/`WatchAppSwitch`/`PersistSelectionConfig`（selection_service.cpp）；`DoCompare`/`Init`（selection_config_comparator.cpp）；`GetOneByUserId`/`Save`（database_plugin.cpp） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | RDB 依赖 `/data` 分区就绪与插件加载；参数写入受 DAC 限制；`shouldStop` 一旦触发 SA 即自卸载（开关 off 场景） |
| 并发约束 | 比较器 Init 一次性；同步流程在服务线程串行；DB 事务由插件内保证原子 |
| 安全约束 | RDB `SecurityLevel::S1`；uid 输入做数字与范围校验（防参数注入）；参数默认值/按需行为变更需升级评审 |

## IPC 契约与接口层（IDL / inner-api / NAPI / Taihe）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 同一能力要服务四类消费方：系统应用（C++）、JS/ArkTS 应用（NAPI）、方舟 ANI 通道（Taihe FFI）、以及跨进程协议演进（proxy/stub 生成） |
| 技术目标 | 契约单源（.idl/.taihe），生成物不可手改，四层各自收敛导出面 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 契约层：`interfaces/idl/ISelectionService.idl`（Register/Unregister/GetSelectionContent）+ `ISelectionListener.idl`（OnSelectionChange/FocusChange，sequenceable 引 `common/selection_data_inner..SelectionInfoData/SelectionFocusChangeInfo`），`idl_gen_interface` 生成 proxy/stub 到 `${target_gen_dir}`。inner-api：`interfaces/inner_kits/selection_client`（与 `frameworks/native/selection_client` 共源，`SelectionClient::GetInstance().GetSelectionContent()` → `GetSystemAbility(8500)` → `iface_cast<ISelectionService>`），versionscript 仅 `*SelectionClient*`、`innerapi_tags=["platformsdk"]`。NAPI 层（`frameworks/js/napi/`）：`selection_manager`（=selectionManager 模块，js_selection_engine_setting.cpp:484-526 导出 on/off/getSelectionContent/createPanel/destroyPanel + SelectionType）、`selection_panel`（仅 PanelType）、`selection_extension_ability/_context`（JS 实现内嵌 abc 字节码）；异步框架 `AsyncCall`（FFRT 线程安全）；`EventChecker` 事件名白名单。Taihe 层（`frameworks/ets/taihe/`）：`SelectionManager/idl/ohos.selectionInput.selectionManager.taihe`（onSelectionComplete/offSelectionComplete/@promise getSelectionContent/createPanel/destroyPanel + interface Panel 全方法）与 `SelectionPanel/idl`，`taihe_ffi_gen` 生成胶水（`selectionManager_taihe_native.z`，boot abc → `/system/framework/selectionManager.abc` 等）；ANI 回调常量 `callbackType_SelectionComplete="selectionComplete"` 等（ani_common.h:33-35） |
| 关键流程 | 频控/校验在 NAPI 层前置（500ms/50 次、入参 PanelInfo 解析 `JsSelectionUtils::GetValue`）；hiappevent 打点挂 createPanel |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | JS/ETS：`selectionManager`（on/off/getSelectionContent/createPanel/destroyPanel）、`Panel` 方法集、`selectionInput.SelectionPanel`（PanelType）、`SelectionExtensionAbility/Context`（onConnect/onDisconnect/startAbility）；系统侧：`SelectionClient`（inner-api） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `ISelectionService`/`ISelectionListener`（idl）；`SelectionClient`；`JsSelectionEngineSetting`/`JsPanel`/`EtsSelectionEngineSetting` |
| 核心成员 | 模块名常量（`selectionInput.selectionManager` 等）；d.ts 位于大仓 `interface/sdk-js/api/@ohos.selectionInput.*.d.ts`（本仓无） |
| 核心方法 | idl 声明全集（interfaces/idl/）；`SelectionClient::GetInstance()/GetSelectionContent()`（frameworks/native/selection_client/selection_client.cpp:32/:46） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 生成物不可手改（proxy/stub、taihe src 部分）；无 CJ/Cangjie 通道（ArkTS/ANI+NAPI 覆盖）；`frameworks/js/napi/selection_framework.map` global 为空（全部 local） |
| 并发约束 | NAPI 回调经 VIP 队列；`AsyncCall` FFRT 线程安全；`ReqMsgHandle` 类似单线程约束由 AsyncCall 封装消化 |
| 安全约束 | inner-api 变更需兼容性评审（platformsdk）；IDL 改动必须同步考虑跨版本 proxy/stub 兼容 |

## 权限校验机制（SelectionAppValidator）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | SA 8500 会把用户选中文本（敏感数据）发给扩展应用，任何应用都能注册监听/取词的话构成隐私泄露通道 |
| 技术目标 | 所有事件入口只对"配置指定的那一个划词扩展应用"放行，以调用方 TokenID 不可伪造为准 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 服务端版（`service/src/selection_app_validator.cpp`）：`IPCSkeleton::GetCallingTokenID()` → `Security::AccessToken::AccessTokenKit::GetHapTokenInfo(callingTokenId, hapTokenInfoRes)` 取调用方 bundleName → 与 `sys.selection.app` 的 bundle 前缀（`GetBundleNameFromSys`（定义于 `service/src/selection_app_validator.cpp:63`）按 `/` 截断）比对；强制点=`RegisterListener`/`UnregisterListener`/`GetSelectionContent` 三个 IPC 入口（失败 `UNAUTHENTICATED_ERROR`）。framework 版（`frameworks/native/selection_ability/src/selection_app_validator.cpp`）：模块加载时 `SetValid()` 置位（js_selection_engine_setting.cpp:501），用于 createPanel 侧。进程级补充：服务进程自身 8 权限 + ACL（INPUT_MONITORING 等，selection_service.cfg），参数 DAC（selection.para.dac） |
| 关键流程 | 校验失败既拒绝注册也拒绝取词 → 非目标应用拿不到 SelectionInfo 也拿不到文本 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无对外 API；切换目标扩展应用=`sys.selection.app` 参数变更（服务侧自动重连新目标，校验基准同步更新） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionAppValidator`（服务端版 + framework 版） |
| 核心成员 | `UNAUTHENTICATED_ERROR`（utils/include/selection_errors.h） |
| 核心方法 | `Validate()`（两版本）；`GetBundleNameFromSys`（selection_app_validator.cpp）/`ParseAppInfo`（selection_common.cpp） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 依赖 access_token 服务可用；bundleName 反查依赖 WMS 窗口信息（事件链） |
| 并发约束 | 校验在 IPC 入口线程同步执行，无状态 |
| 安全约束 | **禁止**放宽/跳过校验；`sys.selection.app` 指向谁就是唯一合法消费者，变更该参数等同变更数据流向，需评审 |

## 可观测性机制（hisysevent / hiappevent / hitrace）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 取词链路横跨注入、剪贴板、连接、面板四个故障域，需要分域归因与长期用量统计；对外 API 需要时延诊断 |
| 技术目标 | 系统侧 FAULT 即时上报 + 12 小时用量统计；应用侧 API 级时延打点；关键链路同步 trace |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | hisysevent（domain `SELECTIONFWK`，`hisysevent-SELECTIONFWK.yaml`）：`SELECTION_PROCESS_ABNORMAL`（FAULT/CRITICAL/preserve：FAIL_REASON=SelectFailedReason 0..4、BUNDLE_NAME、ERROR_CODE）——上报点：面板创建失败（selection_panel.cpp:69）、连接/断开超时（selection_service.cpp:473/511）、注入失败（selection_pasteboard_manager.cpp:367）、文本接收失败（:116）；`SELECTION_STATISTIC`（STATISTIC：SELECTION_TRIGGER_COUNT/FAILED_COUNT）——`AddSelectionCount()`（每次取词）+ `AddFailCount()`（每次失败），`StartHisyseventTimer()` 注册 12h 定时器（`SELECTION_HISYSEVENT_REPORT_TIME=12*60*60*1000`，sysevent/hisysevent_adapter.cpp:16）周期上报后清零。hiappevent：`SelectionApiEventReporter`（hiappevent_agent/）：一次性 `AppEventProcessorMgr::AddProcessor(config{name="ha_app_event", configName="SDK_OCG"})`；`WriteEndEvent(result, errCode)` 写 `("api_diagnostic","api_exec_end",BEHAVIOR)`（trans_id/api_name/sdk_name="BasicServicesKit"/begin/end/result/error_code）；挂接点=createPanel（js_selection_engine_setting.cpp:263-283）。hitrace：`SelectionMethodSyncTrace`（common/selectionmethod_trace.cpp，TraceTaskId 如 ONCONNECT_EXTENSION） |
| 关键流程 | 统计定时器与插件卸载定时器共用 `SelectionFwkTimer`（Utils::Timer 独立线程） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 事件消费面：HiView（SELECTIONFWK 域两个事件）；修改事件字段=改 `hisysevent-SELECTIONFWK.yaml`（DFX 消费方依赖字段名，需评审） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `HisyseventAdapter`（sysevent/）、`SelectionApiEventReporter`（hiappevent_agent/）、`SelectionMethodSyncTrace`（common/） |
| 核心成员 | `SelectFailedReason{INJECT_CTRLC_FAILED=0, TEXT_RECEIVE_FAILED, CREATE_PANEL_FAILED, CONNECT_EXTENSION_TIMEOUT, DISCONNECT_EXTENSION_TIMEOUT}`（sysevent/hisysevent_adapter.h:26-32） |
| 核心方法 | `ReportShowPanelFailed`/`AddSelectionCount`/`AddFailCount`/`StartHisyseventTimer`/`WriteEndEvent` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 12h 统计依赖 SA 常驻期内定时器（SA 卸载即丢当期计数）；preserve 级事件落盘 |
| 并发约束 | 计数原子；上报在定时器线程/失败现场线程直接调用 |
| 安全约束 | 事件不含选中文本本身（仅 bundleName/错误码）；hiappevent trans_id 为随机数非用户标识 |
