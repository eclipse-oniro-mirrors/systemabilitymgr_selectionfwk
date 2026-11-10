# 业务实体知识

selectionfwk 部件核心业务实体的完整知识定义，涵盖静态概念、动态运行、规则约束与代码映射。

## 划词信息（SelectionInfo / SelectionInfoData）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 划词信息（SelectionInfo，IPC 载体 SelectionInfoData） |
| 实体定义 | 一次用户选词操作的事件描述：选区类型 + 几何坐标 + 来源窗口/应用标识；由 SA 生成并推送给已注册的划词扩展应用，是"识别完成"的唯一通知凭据 |
| 核心特征 | `SelectionInfo`（`common/selection_interface.h:33-46`）：`selectionType`（`SelectionType`：MOVE_SELECTION=1/DOUBLE_CLICKED_SELECTION=2/TRIPLE_CLICKED_SELECTION=3）、start/endDisplayX/Y、start/endWindowX/Y、displayId、windowId、bundleName；IPC 侧 `SelectionInfoData : Parcelable`（`common/selection_data_inner.h:29`，selectionType 以 Int8 序列化） |
| 类型/分类 | 按选区类型三类：滑动选词、双击选词（词）、三击选词（段） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 创建：状态机 `ProcessWordSelection`（`service/src/selection_input_monitor.cpp:376`）`SaveSelectionEndInfo` 按状态写入 selectionType 与终点坐标（起点在 WORD_BEGIN 的 `SaveSelectionStartInfo` 记录，bundleName 经 `WindowManager(Lite)::ListWindowInfo(windowId)` 反查）；分发：`FinishedWordSelected`（:596）完成校验链（无扩展连接则 `ConnectExtAbilityFromConfig` → 锁屏跳过 → bundleName 空告警 → `IsAppInBlocklist`（空实现）→ 开关校验 → `SetCanGetSelectionContentFlag(true)` → `selectionEventId_++`）后 `listener->OnSelectionChange(infoData)`；消费：扩展应用 JS `on('selectionCompleted')` 收到后调用 `getSelectionContent` 取真实文本 |
| 状态流转 | 不可变实体：创建即终态；消费后由 `selectionEventId_` 递增使旧事件失效（应用迟到请求返回 `INVALID_TIMING`） |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 只有触发方式判定通过才产生：ctrl 模式须等 Ctrl 键 UP 才进入 DONE 并分发；分发必须与 `CanGetSelectionContent` flag 置位同帧（先 flag 后 notify）；坐标来自 MMI PointerEvent 屏幕/窗口双坐标系 |
| 并发约束 | 写侧经 `selectionInfoMutex_`（shared_mutex）保护（`SaveSelectionTypeLocked`）；ID 侧 `selectionEventId_` 原子递增作为事件一致性凭据 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 输入状态机（产生者）；`BaseSelectionInputMonitor` 缓存区 |
| 下游影响 | 划词扩展应用（selectionCompleted 回调）；取词时序 flag（联锁） |
| 平级关联 | `SelectionFocusChangeInfo`（同文件另一事件载体）；`ISelectionListener`（分发通道） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionInfo`/`SelectionType`（common/selection_interface.h）、`SelectionInfoData`（common/selection_data_inner.h） |
| 核心方法 | `SaveSelectionStartInfo`/`SaveSelectionEndInfo`/`ProcessWordSelection`/`FinishedWordSelected`（selection_input_monitor.cpp）、`Write`（NAPI 字段映射，js_selection_engine_setting.cpp:575） |

## 划词配置（SelectionConfig / 双轨记录）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 划词配置（SelectionConfig；系统参数态 `sys.selection.*`；RDB 态 `selection_config` 表行） |
| 实体定义 | 服务行为参数集：开关、触发方式、扩展应用、用户绑定四项；同时以系统参数（全局快车道）与 RDB 行（用户级）两种形态持久化，运行期统一读内存镜像 |
| 核心特征 | `SelectionConfig`（`service/include/selection_config.h:24-41`）：`isEnabled_`、`isTriggered_`（true=ctrl/false=immediate）、`uid_`（默认 -1）、`applicationInfo_`（默认 `com.selection.selectionapplication/SelectionExtensionAbility`）；RDB 表（`service/plugins/src/database/`）：`id PK, enable, bundleName, trigger, shortcutKeys, uid UNIQUE, tokenId DEFAULT ''`，`DATABASE_OPEN_VERSION=2`（升级 SQL ALTER TABLE），`SecurityLevel::S1` |
| 类型/分类 | 三形态：内存镜像（`MemSelectionConfig` 单例，mutex 保护）、系统参数态、数据库态 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 写入：参数 watcher（`WatchEnableSwitch/WatchTriggerMode/WatchAppSwitch`，selection_service.cpp:631 起）感知变化 → 更新镜像 → `PersistSelectionConfig()`（未登录用户不落库）；同步：`SynchronizeSelectionConfig()`（:687）在 Init/USER_SWITCHED 时读双轨 → `SelectionConfigComparator::DoCompare`（selection_config_comparator.cpp:93-143）裁决 → `SyncConfigToSystem`（SetParameter）/`SyncConfigToDatabase`（经插件 `DatabasePluginImpl::Save`，Update 无行则 Insert，事务包裹）；用户绑定：uid = 前台账号（`GetForegroundOsAccountLocalId`，重试 50×100ms） |
| 状态流转 | DB 无记录 → 比较器 `Init` 以 sys 参数为默认回写 DB；双轨不一致 → 按 direction 单向覆盖（FromDbToSys/FromSysToDb）；衍生动作：`shouldStop`→`DisableSAService()+UnloadService()`、`shouldRestartApp`→`ReconnectExtAbility`、`shouldStart`→按需连接扩展 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 参数键固定四项（`sys.selection.switch/trigger/app/uid`，buffer 200、uid 长度 ≤11 且数字校验）；DAC `sysselection:sysselection:0775`；`app` 格式 `bundle/ability`（`ParseAppInfo` 按 `/` 拆分）；RDB 路径 `/data/service/el1/public/selection_service/selection_config.db` |
| 并发约束 | 镜像单例 mutex；比较器 Init 单次（构造期）；参数 watcher 回调在参数线程、DB 写在插件调用线程，比较器输出后统一在服务线程执行动作 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `SysSelectionConfigRepository`（参数态仓库）、`SelectionConfigDataBase`（RDB 态，经插件）、`MemSelectionConfig`（读侧） |
| 下游影响 | SA 按需加载（switch）、状态机触发分叉（trigger）、扩展连接目标（app）、用户配置隔离（uid） |
| 平级关联 | `SelectionAppValidator`（用 app 前缀校验调用方）；`SelectionExtensionAbilityConnection`（app 变化驱动重连） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionConfig`/`MemSelectionConfig`/`SelectionConfigComparator`/`ComparisionResult`（service/include/selection_config.h 等）；`SelectionConfigDataBase`（service/plugins/include/selection_config_database.h） |
| 核心方法 | `SynchronizeSelectionConfig`/`WatchParams`/`PersistSelectionConfig`（selection_service.cpp）；`DoCompare`/`Init`（selection_config_comparator.cpp）；`SysSelectionConfigRepository::Get/Set`（sys_selection_config_repository.cpp）；`DatabasePluginImpl::Save`（service/plugins/src/database/database_plugin.cpp:53-106） |

## 面板（SelectionPanel / PanelInfo）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 面板（SelectionPanel；描述体 PanelInfo/PanelType） |
| 实体定义 | 划词扩展应用进程内的 `WINDOW_TYPE_SELECTION` 浮动窗口及其封装；是扩展应用呈现翻译/摘要等内容的唯一窗口载体，创建在应用侧（ctx 为应用 Context），SA 只探测其可见性 |
| 核心特征 | `PanelType`（`MENU_PANEL=1`/`MAIN_PANEL=2`，panel_info.h:23-26）；`PanelInfo{panelType,x,y,width,height}`（:28-34）；窗口名映射 `{MAIN_PANEL→"SelectionMainFwkWindow", MENU_PANEL→"SelectionMenuFwkWindow"}`（selection_panel.cpp:41-44）；窗口模式 `WINDOW_MODE_FLOATING` |
| 类型/分类 | 菜单面板（MENU_PANEL，小浮窗）与主面板（MAIN_PANEL，内容窗）；JS 侧 `selectionInput.SelectionPanel` 模块仅导出 PanelType 枚举 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 创建：JS `createPanel(ctx, info)` → NAPI `JsSelectionEngineSetting::CreatePanel`（含 hiappevent 打点）→ `SelectionAbility::CreatePanel`（校验 SA 存在 + `SelectionAppValidator` + `HasPanelOfType` 去重）→ `Rosen::Window::Create` → 成功 `PushPanel` 入 `panels_` deque；使用：`SetUiContent`（NapiSetUIContent/AniSetUIContent + `SetTransparent(true)`）→ `ShowPanel`（等 UI 就绪 3×100ms）→ `StartMoving`（StartMoveWindow）/`MoveTo`（sceneboard 下 MoveWindowToGlobalDisplay）；回收：`HidePanel/DoHidePanel`、`DestroyPanel`（先 Hide 后 `window->Destroy()`）；焦点联动：`Dispose(winId)` 遍历 panels_（MENU_PANEL→Hide，其余→Destroy） |
| 状态流转 | 创建 → STATE_SHOWN ↔ STATE_HIDDEN → STATE_DESTROYED；状态经 `PanelStatusListener::OnPanelStatus(windowId, status)` 以 `"hidden"/"destroyed"` 字符串回调 JS（`on('hidden')`/`on('destroyed')`）；操作全部经 `JsPanel::jsQueue_`（FFRTBlockQueue，10s 超时）串行化 |
### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 同类型面板去重（`HasPanelOfType`）；创建失败上报 `SELECTION_PROCESS_ABNORMAL`（CREATE_PANEL_FAILED）；SA 侧 `IsAnySelectionPanelShowing()`（`GetVisibilityWindowInfo` 匹配配置的 bundle/ability）用于插件保活判定 |
| 并发约束 | 面板操作在应用侧 FFRTBlockQueue 串行；Show 等待 `isWaitSetUiContent_`（3×100ms 轮询）；窗口对象归应用进程，SA 不持有 |
### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `SelectionAbility`（管理器，panels_ deque）；WMS/Rosen（窗口实现） |
| 下游影响 | 扩展应用 UI；SA 插件卸载定时器（面板显示时保留插件） |
| 平级关联 | `SelectionFocusChangeInfo`（Dispose 触发源）；`PanelListenerImpl`（NAPI 回调注册表：windowId→type→callbacks，ConcurrentMap） |
### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionAbility`/`SelectionPanel`/`PanelStatusListener`/`WindowState`（frameworks/native/selection_ability/）；`JsPanel`/`PanelListenerImpl`（frameworks/js/napi/selection_ability/） |
| 核心方法 | `CreatePanel/DestroyPanel/ShowPanel/HidePanel/StartMoving/MoveTo/SetUiContent/Dispose`（selection_panel.cpp、selection_ability.cpp）；`IsAnySelectionPanelShowing`（selection_service.cpp:545-572） |

## 扩展连接（SelectionExtensionAbilityConnection / AbilityRuntimeInfo）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 扩展连接（到划词扩展应用的服务侧连接） |
| 实体定义 | SA 8500 与划词扩展应用之间的常驻 IPC 连接（ConnectAbility 语义）：连接成功即代表扩展应用在位，划词事件才能分发；连接目标由配置 `sys.selection.app` 决定 |
| 核心特征 | `SelectionExtensionAbilityConnection : AAFwk::AbilityConnectionStub`（selection_service.h:53-78）；连接态记录 `AbilityRuntimeInfo{userId, bundleName, abilityName}`（`connectedAbilityInfo`）；同步原语 `std::promise<void> connectPromise_/disconnectPromise_` + `wait_for`（`TIMEOUT_FOR_CONNECT_DISCONNECT=5s`） |
| 类型/分类 | 单连接（同时只有一个目标扩展）；连接/断开两阶段对称操作 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 建立：`DoConnectNewExtAbility`（selection_service.cpp:438）构造 Want(bundle, ability) → 插件 `abilityConnect_(&want, &connectInner_, userId)` → `WaitForConnect()`（5s；超时上报 `CONNECT_EXTENSION_TIMEOUT`）→ `OnAbilityConnectDone` 记录 RuntimeInfo；断开：`DoDisconnectCurrentExtAbility`（:482）对称（超时上报 DISCONNECT_TIMEOUT）；驱动时机：事件到来无连接时 `ConnectExtAbilityFromConfig`；配置变化（`WatchAppSwitch` → 持久化 + `DisconnectCurrentExtAbility`；比较器 `shouldRestartApp` → `ReconnectExtAbility`）；应用装卸（`WatchExtAbilityInstalled`：PACKAGE_ADDED/CHANGED） |
| 状态流转 | 未连接 → 连接中（promise 等待）→ 已连接（RuntimeInfo 有效）→ 断开中 → 未连接；`OnAbilityDisconnectDone` 清空 RuntimeInfo |
### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 连接必须经插件（AbilityManagerPlugin）发起，主服务不直连 ability_runtime；连接成功与插件卸载互斥（扩展连接时 5 分钟闲置不卸载插件）；userId 参与连接（多用户下连前台用户的扩展） |
| 并发约束 | `connectMutex_` 保护连接流程；promise/future 单次性——重复 Wait 会异常，连接前重置 |
### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | 划词配置（app 项即目标）、插件 so（abilityConnect 函数指针） |
| 下游影响 | 事件分发给扩展（连接是分发前提）；插件保活；hitrace ONCONNECT_EXTENSION |
| 平级关联 | `ISelectionListener` 注册（扩展进程侧反向注册给 SA） |
### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionExtensionAbilityConnection`/`AbilityRuntimeInfo`（selection_service.h） |
| 核心方法 | `DoConnectNewExtAbility`/`DoDisconnectCurrentExtAbility`/`WaitForConnect`/`WaitForDisconnect`/`ConnectExtAbilityFromConfig`/`ReconnectExtAbility`/`WatchAppSwitch`/`WatchExtAbilityInstalled`（selection_service.cpp）；插件实现 `AbilityManagerPluginImpl`（service/plugins/src/ability/ability_manager_plugin_impl.cpp:76/97） |

## 输入状态机（SelectInputState / SelectInputSubState）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 划词识别状态机（SelectionInputMonitor 的运行态实体） |
| 实体定义 | 由 MMI 键鼠事件流驱动的识别状态集合：把原始事件序列折叠为"滑动/双击/三击选词 + Ctrl 触发"的确定性判定，是划词识别的唯一裁决者 |
| 核心特征 | 主态 `SelectInputState`（selection_input_monitor.h:41-47）：`SELECT_INPUT_INITIAL → WORD_BEGIN → WAIT_LEFT_MOVE / WAIT_DOUBLE_CLICK → WAIT_TRIPLE_CLICK → WORD_END → DONE`；子态 `SelectInputSubState`（:49-58）：SUB_INITIAL/WAIT_POINTER_ACTION_BUTTON_DOWN/UP/WAIT_KEY_CTRL_DOWN/UP/WAIT_BUTTON_OR_CTRL_DOWN；原子字段 `curSelectState/subSelectState/lastClickTime`（selection_input_monitor.h:99-103） |
| 类型/分类 | 两条识别路径：滑动路径（WORD_BEGIN→WAIT_LEFT_MOVE）与点击路径（WORD_BEGIN→WAIT_DOUBLE_CLICK→WAIT_TRIPLE_CLICK） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 输入：`InputManager::AddMonitor(inputMonitor_)`（SA 3101 就绪回调后）→ `OnInputEvent(KeyEvent/PointerEvent/AxisEvent)`；状态迁移（selection_input_monitor.cpp）：`InputInitialProcess`（:347，左键 DOWN→WORD_BEGIN+记时+存起点）、`InputWordBeginProcess`（:361，MOVE→WAIT_LEFT_MOVE / UP→WAIT_DOUBLE_CLICK）、`InputWordLeftMoveProcess`（:398，UP→`ProcessWordSelection`）、`InputWordDoubleClickProcess`（:422，550ms 超时重置；第二击 UP→`ProcessWordSelection`；DONE 态微动 ≤10px→`JudgeTripleClick`（:411，300ms 窗口）→WAIT_TRIPLE_CLICK）、`ProcessWordSelection`（:376，写 selectionType + 终点；ctrl 模式→WORD_END 等 Ctrl（`ProcessInputEvent(KeyEvent)` :88 只认 KEYCODE_CTRL_LEFT/RIGHT，UP→DONE），immediate→DONE）；DONE 后 `FinishedWordSelected` 产出事件；非左键任意态 `ResetState()`（:156） |
| 状态流转 | 见主态链；异常恢复：超时（550/300ms）、非左键、Ctrl 重复键（IsRepeatKey 忽略）、锁屏/开关关闭 → ResetState 回 INITIAL |
### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 时间阈值 `DOUBLE_CLICK_TIME=550ms`/`TRIPLE_CLICK_TIME=300ms`；微动容差 `MAX_POSITION_CHANGE_OFFSET=10px`（`IsTinyMovement` :221）；三击识别要求两击间仅微动；Ctrl 触发只认左右 Ctrl 键码；文本获取不属状态机职责（DONE 后由取词链路处理） |
| 并发约束 | 状态字段 atomic、SelectionInfo 写入 shared_mutex；`OnInputEvent` 由 MMI 分发线程串行回调 |
### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | MMI InputManager（事件源）；`GetCtrlSelectFlag`（=trigger 配置，决定是否等 Ctrl） |
| 下游影响 | 划词信息（产出）；取词时序 flag（DONE 联动置位）；`gmock` 单测 `MockBaseSelectionInputMonitor` |
| 平级关联 | `HandleWindowFocused`（BUTTON_DOWN 时同流判焦点，FocusChangeInfo 产出） |
### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `SelectionInputMonitor`/`BaseSelectionInputMonitor`（实现 `MMI::IInputEventConsumer`） |
| 核心方法 | `OnInputEvent`/`ProcessInputEvent`/`InputInitialProcess`/`InputWordBeginProcess`/`InputWordLeftMoveProcess`/`InputWordDoubleClickProcess`/`JudgeTripleClick`/`ProcessWordSelection`/`ResetState`/`IsTinyMovement`（selection_input_monitor.cpp） |

## 划词监听器（ISelectionListener / 注册关系）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 划词监听器（扩展应用向 SA 注册的事件接收端） |
| 实体定义 | 划词扩展应用侧实现的 `ISelectionListener` IPC 对象：SA 持有并回调 OnSelectionChange/FocusChange；其注册本身就是"应用就绪接收事件"的声明，注册入口同时是权限校验点 |
| 核心特征 | 契约 `interfaces/idl/ISelectionListener.idl`：`OnSelectionChange([in] SelectionInfoData)`、`FocusChange([in] SelectionFocusChangeInfo)`；应用侧实现 `SelectionListenerImpl : SelectionListenerStub`（frameworks/native/selection_ability/include/selection_listener_impl.h:24，NAPI 注册）与 ETS 版（taihe/SelectionManager）；SA 侧持有 `listener_`（mutex 保护，`selection_service.h:205-221`） |
| 类型/分类 | NAPI 版（JS selectionManager 模块加载即 `Register()`）与 ANI 版（Taihe SelectionManager）两种宿主实现 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 注册：模块加载（js_selection_engine_setting.cpp:497-502）→ `SelectionClient::RegisterListener(SelectionListenerImpl)` → IPC 到 SA → `SelectionAppValidator::Validate()` 通过后存 listener_（失败 `UNAUTHENTICATED_ERROR`）；回调：事件/焦点到达 → SA 持 listener_ 发 IPC → 应用侧 `SelectionListenerImpl::OnSelectionChange/FocusChange`（:43）→ `EventHandler::PostTask(VIP)` + `JsCallbackHandler::Traverse` 到 JS；注销：模块卸载/进程退出（`UnregisterListener` 同入口校验） |
| 状态流转 | 未注册 → 已注册（listener_ 非空）→ 未注册；断链（扩展进程死亡）后 SA 下次事件重新走连接/注册流程 |
### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 单监听器（mutex 保护覆盖式持有）；注册/注销/取词三入口统一校验调用方身份；事件分发前置条件链在 `FinishedWordSelected`（连接/锁屏/黑名单/开关/flag） |
| 并发约束 | 分发在 IPC 线程；SA 与应用双向进程隔离，回调对象经 Binder 死亡通知回收 |
### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `ISelectionService`（注册通道）；`SelectionListenerImpl`（应用侧宿主）；`JSCallbackObject`/`ConcurrentMap` 回调注册表 |
| 下游影响 | JS `on('selectionCompleted')`/`on('hidden')`/`on('destroyed')`；扩展 Dispose 行为 |
| 平级关联 | `SelectionAppValidator`（准入）；频控（`getSelectionContent` 侧 500ms/50 次） |
### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `ISelectionListener`（idl）、`SelectionListenerImpl`（NAPI/ANI 两版）、`PanelListenerImpl`（面板状态回调，同类注册表模式） |
| 核心方法 | `RegisterListener`/`UnregisterListener`/`OnSelectionChange`/`FocusChange`（idl + selection_service.cpp:263 起 IPC 入口）；`SelectionListenerImpl::OnSelectionEvent`（js_selection_engine_setting.cpp:594-629） |
