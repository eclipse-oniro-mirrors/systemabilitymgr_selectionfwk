# AGENTS.md — selectionfwk

OpenHarmony **Selection Service** subsystem (`@ohos/selectionfwk`, part_name `selectionfwk`, subsystem `systemabilitymgr`). Implements SystemAbility **ID 8500** (`selection_service` process) that globally captures user-selected text and drives selection extension panels. Not buildable standalone — lives inside the OpenHarmony source tree at `foundation/systemabilitymgr/selectionfwk`.

This file routes an agent to the right files; it is not an architecture reference. When a section says "read X", read X before editing.

## Build

The build is GN + Ninja driven by the top-level OpenHarmony `build.sh`, run from the **workspace root** (the OpenHarmony tree root, not this repo):

- Full (after editing any `BUILD.gn`): `./build.sh --product-name rk3568 --ccache`
- Full (no `BUILD.gn` changes): `./build.sh --product-name rk3568 --ccache --fast-rebuild`
- Component only: `./build.sh --product-name rk3568 --ccache --build-target selectionfwk`
- Tests only: `./build.sh --product-name rk3568 --build-target selectionfwk_test`

GN entry: `bundle.json` registers the component; `selection_service.gni` (imported by every `BUILD.gn`) defines `selection_fwk_root_path` and `system_type = "default"`. Add new `declare_args()` switches (e.g. `word_selection_feature_test_one`) in `selection_service.gni`, not in `BUILD.gn` files.

There is no separate lint step. Static analysis (CFI, UBSan, integer_overflow, boundary_sanitize) is enforced at **build time** via the `sanitize` block in every production target — a clean build is the lint gate.

## Layout / what builds what

| Path | Output | Notes |
|---|---|---|
| `service/` | `libselection_service.z.so` | Core SA. `service/src`, `service/include`, `service/focus_monitor`, `service/plugins` |
| `service/plugins/` | `libselection_plugins_impl.z.so` + `selection_config_static` | Decoupled pasteboard / database / ability plugins. `plugin_exports.cpp` exports `extern "C"` symbols with explicit `visibility("default")` (everything else hidden) |
| `frameworks/native/selection_ability` | `selection_ability` | Native selection-ability impl |
| `frameworks/native/selection_extension` | `selection_extension_ability_native` | Native extension impl |
| `frameworks/native/selection_client` | (compiled into inner_kits) | Source shared with `interfaces/inner_kits/selection_client` |
| `interfaces/inner_kits/selection_client` | `libselection_client.z.so` | **Public inner-api**; `version_script = selection_client.versionscript`; `innerapi_tags = ["platformsdk"]` |
| `frameworks/js/napi/selection_ability` | `selectionmanager_napi` | The `selectionManager` JS module |
| `frameworks/js/napi/selection_panel` | `selectionpanel_napi` | The `selectionPanel` JS module |
| `frameworks/js/napi/selection_extension_ability` / `_context` | `selectionextensionactivity_napi` / `...context_napi` | |
| `frameworks/ets/ets/` | `selection_extension_ability_etc` | ETS API `.ets` declaration files |
| `frameworks/ets/taihe/` | `selection_taihe_group` | **Taihe FFI codegen** (dep `taihe_ffi_gen`); IDL sources under `SelectionManager/idl`, `SelectionPanel/idl` |
| `interfaces/idl/` | `selection_service_proxy/stub`, `selection_listener_proxy/stub` | IDL codegen via `idl_tool` (`idl_gen_interface`); generated `*_proxy.cpp`/`*_stub.cpp` land in `${target_gen_dir}` — do not edit |
| `common/` | `libselection_common.a` (static) | Shared utils, NAPI helpers |
| `sa_profile/8500.json` | SA profile | SA 8500, `libselection_service.z.so`, boot phase `BootStartPhase`, **start-on-demand** on `sys.selection.switch=on` and `usual.event.USER_SWITCHED` |
| `etc/init/selection_service.cfg` | init config | uid/gid `sysselection`, SELinux `u:r:selection_service:s0`, `apl: system_basic`; creates `/data/service/el1/public/selection_service` |
| `etc/para/selection.para` | system params | `sys.selection.switch`, `sys.selection.trigger`, `sys.selection.app`, `sys.selection.uid`, `sys.selection.timeout` |
| `sysevent/` / `hiappevent_agent/` | hisysevent / hiappevent adapters | Domain `SELECTIONFWK` (see `hisysevent-SELECTIONFWK.yaml`) |
| `utils/` | `selection_timer` | Linked into service + plugins |

Frequent-change paths (by recent commit frequency): `service/src`, `service/include`, `service/plugins`, `frameworks/native`, `frameworks/js`, `test/unittest`. Timer state / lock-safety changes recur often in `service/src/selection_input_monitor.cpp` and `utils/src/selection_timer.cpp` — check for existing lock guards before adding concurrency code there.

## Where to look for common tasks

| Task | First files to read |
|---|---|
| Add/change a selection service behavior | `service/include/selection_service.h`, `service/src/selection_service.cpp` |
| Input event handling / state machine | `service/include/selection_input_monitor.h`, `service/src/selection_input_monitor.cpp` |
| Pasteboard / DB / ability plugin logic | `service/plugins/src/{database,pasteboard,ability}/`, `service/plugins/include/` |
| Public C++ inner-API surface | `interfaces/inner_kits/selection_client/include/selection_client.h`, `frameworks/native/selection_client/selection_client.cpp` |
| IPC wire format / proxy-stub | `interfaces/idl/ISelectionService.idl`, `ISelectionListener.idl` (then `${target_gen_dir}/*_proxy.cpp`) |
| JS/ETS API (selectionManager / selectionPanel) | `frameworks/js/napi/selection_ability/`, `frameworks/js/napi/selection_panel/`, `frameworks/ets/ets/` |
| SA on-demand / boot / SELinux / permissions | `sa_profile/8500.json`, `etc/init/selection_service.cfg`, `etc/para/selection.para(.dac)` |
| DFX (hisysevent / hiappevent) | `sysevent/hisysevent_adapter.{h,cpp}`, `hiappevent_agent/`, `hisysevent-SELECTIONFWK.yaml` |
| Config persistence / user-switch | `service/src/sys_selection_config_repository.cpp`, `service/src/selection_config_comparator.cpp` |
| Tests for any of the above | `test/unittest/<matching>_test.cpp` |

## Knowledge routing

### Task-based routing
- **Public API change** (anything touching `libselection_client.z.so`): read `interfaces/inner_kits/selection_client/selection_client.versionscript` first — only symbols matching `*SelectionClient*` are exported; everything else is `local:*`. A new public symbol that does not match this pattern will be silently hidden.
- **IPC protocol / cross-process change**: edit the `.idl` in `interfaces/idl/`, never the generated `*_proxy.cpp`/`*_stub.cpp`. Rebuild to regenerate.
- **Permission / SELinux / on-demand behavior**: read `sa_profile/8500.json`, `etc/init/selection_service.cfg`, `etc/para/selection.para(.dac)` together — they are coupled (switch param + common event + SA profile + init uid/gid/secon).
- **DFX / fault attribution**: read `hisysevent-SELECTIONFWK.yaml` for the event schema (`SELECTION_PROCESS_ABNORMAL`, `SELECTION_STATISTIC`) before adding/renaming a hisysevent field; domain is `SELECTIONFWK`.
- **Window manager dep**: see the sceneboard conditional below before adding any `window_manager:*` dep.
- **Plugin boundary change**: the SA must stay decoupled from `ability_runtime:ability_manager` — read the comment in `service/BUILD.gn` and route that dep through `service/plugins/` instead.

### Path-based routing
- Editing anything under `service/plugins/` → also re-check `service/plugins/BUILD.gn` CFI config (must match `relational_store:native_rdb`).
- Editing `frameworks/ets/taihe/*/idl/` → expect regenerated glue under `frameworks/ets/taihe/*/src`; treat `src` as partly generated.
- Editing `interfaces/idl/*.idl` → regenerate happens at build time into `${target_gen_dir}`; do not commit generated proxy/stub.
- Adding a new `BUILD.gn` for a shared lib → copy the full `sanitize` + `cflags_cc` + `ldflags` block from an existing target (see "Build hardening").

### Vocabulary routing
| Term | Meaning / where to read more |
|---|---|
| SA 8500 | SystemAbility ID 8500 = this service. Profile: `sa_profile/8500.json`. |
| `selection_service` (process) | The SA's runtime process name, runs under uid/gid `sysselection`. |
| APL `system_basic` | Access permission level of the process — `etc/init/selection_service.cfg`. Higher than normal apps; governs which permissions the process can hold. |
| inner_kits / innerapi | The public C++ API surface (`interfaces/inner_kits/selection_client`), tagged `platformsdk` and version-scripted. Changes need compatibility review. |
| IDL proxy/stub | IPC codegen from `interfaces/idl/*.idl` via `idl_gen_interface`. Outputs in `${target_gen_dir}`. |
| Taihe FFI | C/Rust-style FFI codegen (dep `taihe_ffi_gen`) for the `selectionManager`/`selectionPanel` JS modules. IDL in `frameworks/ets/taihe/*/idl/`. |
| sceneboard | Global GN switch `window_manager_use_sceneboard`; picks `libwm_lite` vs `libwm` and defines `SCENE_BOARD_ENABLE`. |
| CFI / UBSan / integer_overflow | Build-time sanitizers in every `sanitize` block. CFI cross-DSO requires matching configs across DSO boundaries. |
| `sys.selection.*` | System parameters driving switch/trigger/app/uid/timeout. Defaults in `etc/para/selection.para`. |
| `BootStartPhase` | SA boot phase for 8500; starts on-demand, not at init. |
| `usual.event.USER_SWITCHED` | Common event that also triggers SA 8500 load (multi-user config switch). |

## Tests

Tests are registered in `bundle.json` under `component.build.test` and run via the OpenHarmony test runner.

- Unit tests — `test/unittest/`: `ohos_unittest("selection_service_unit_test")`, group `selection_manager_ut`. **gtest/gmock only**, uses `HWTEST_F(Fixture, Name, TestSize.LevelN)` macro (not raw `TEST_F`). Private/protected members are exposed via `-Dprivate=public -Dprotected=public` cflags.
- Mock tests — `test/unittest/mock/`: separate `ohos_unittest("selection_service_unit_mock_test")` (uses local `mock_parameter.cpp`).
- Performance test — `test/unittest/PerformanceTest/`: `ohos_app("performanceTest")`, a separate HAP, not part of the gtest suite.
- Fuzz tests — `test/fuzztest/{selectioninputability_fuzzer,selectioninputlistener_fuzzer}`: group `selection_service_fuzztest`, `ohos_fuzztest(...)` with `module_out_path = "selectionfwk/selectionfwk"`.
- XTS — `test/xts/*.test.ets` (declarative API conformance).
- Autotests — `test/autotest/`: scenario-driven Python `.py` + `.json` pairs (e.g. `selectionfwk_normal_select`, `selectionfwk_kill_service`, `selectionfwk_user_switch`, `selectionfwk_lock_screen`). Run on-device, not via GN unittest.

To run a focused test target from the workspace root:
```
./build.sh --product-name rk3568 --build-target selectionfwk_test
# then run the specific ohos_unittest binary on device, e.g.:
hdc shell /data/local/tmp/selection_service_unit_test
```

## Constraints and boundaries

### Do not
- Do **not** hand-edit generated files: `${target_gen_dir}/*_proxy.cpp`, `${target_gen_dir}/*_stub.cpp` (from `interfaces/idl/*.idl`), and `frameworks/ets/taihe/*/src` glue (from `taihe_ffi_gen`). Change the `.idl`/source and rebuild.
- Do **not** re-add `ability_runtime:ability_manager` to `service/BUILD.gn` — the SA is deliberately decoupled; that dep belongs in `service/plugins/` only. A comment in `service/BUILD.gn` warns about this.
- Do **not** drop the `sanitize` / `branch_protector_ret = "pac_ret"` / `cflags_cc` visibility / `ldflags` `--exclude-libs,ALL` blocks when copy-pasting a new `BUILD.gn` for a production target (see "Build hardening").
- Do **not** add a new exported symbol to `libselection_client.z.so` without confirming it matches `*SelectionClient*` in `selection_client.versionscript` (or updating the versionscript deliberately).
- Do **not** modify the SA's exported C++ surface casually — `service/selection_service.map` is an explicit symbol whitelist (`SelectionService::*`, `BaseSelectionInputMonitor::*`, `SelectionConfig::*`, etc.). New exports need a map update.
- Do **not** change `sys.selection.*` defaults in `etc/para/selection.para` or the SA on-demand profile (`sa_profile/8500.json`) without escalation — it changes when SA 8500 loads on real devices and affects user-switch behavior.
- Do **not** bypass the `window_manager_use_sceneboard` branch when adding window_manager deps; runtime code paths are guarded by `SCENE_BOARD_ENABLE`.

### Ask before
- Any change to `interfaces/inner_kits/selection_client/include/selection_client.h` (public inner-api, `platformsdk`) — needs API compatibility review.
- Any change to `etc/init/selection_service.cfg` (uid/gid/secon/permissions) or `etc/para/selection.para.dac` (DAC) — security/permission boundary.
- Any change to `hisysevent-SELECTIONFWK.yaml` event schema — DFX consumers depend on field names/types.
- Any new `external_deps` entry — must be in `bundle.json` `component.deps.components` already, or the build will fail; adding a new component dep is a cross-component boundary change.

### Invariants
- C++17, namespace `OHOS::SelectionFwk`; service code uses `using namespace MMI;`.
- License header: Apache 2.0 with copyright line — keep the year current; production files 2025, tests 2026.
- PC/2-in-1 + external keyboard/mouse only; selection content capped at **6,000 bytes**; no cross-device use.
- Plugins export only via `plugin_exports.cpp` `extern "C"` + `visibility("default")`; everything else stays hidden.

## Build hardening — keep consistent when adding/editing libs

All production targets set (do **not** drop these when copy-pasting a new `BUILD.gn`):
```
branch_protector_ret = "pac_ret"
sanitize = {
  boundary_sanitize = true
  cfi = true
  cfi_cross_dso = true
  cfi_vcall_icall_only = true
  debug = false
  integer_overflow = true
  ubsan = true
}
```
Plus `cflags_cc` with `-fvisibility=hidden`/`-fvisibility-inlines-hidden` and `ldflags` with `-Wl,--exclude-libs,ALL` for shared libs. The plugins `BUILD.gn` documents why: CFI config must match `native_rdb` to avoid cross-DSO CFI failures, and `ldflags += [ "-Wl,-z,now" ]` is required so CFI type-checks complete at load time.

CFI blocklist for tests lives at `test/unittest/ipc_blocklist.txt` — add new test-only source paths there if they trip CFI (gtest/gmock and `test/unittest/*` are already excluded).

## Conditional: sceneboard window manager

Several `BUILD.gn` files branch on the global `window_manager_use_sceneboard`:
```
if (window_manager_use_sceneboard) {
  external_deps += [ "window_manager:libwm_lite" ]
  defines += [ "SCENE_BOARD_ENABLE" ]
} else {
  external_deps += [ "window_manager:libwm" ]
}
```
Keep this branch in sync when adding window_manager deps; `SCENE_BOARD_ENABLE` guards runtime code paths.

## Codegen to be aware of

- **IDL** (`interfaces/idl/*.idl` → `*_proxy.cpp`/`*_stub.cpp` in `${target_gen_dir}`): never hand-edit generated files; change the `.idl` and rebuild.
- **Taihe FFI** (`frameworks/ets/taihe/*/idl/`): the `taihe_ffi_gen` component generates glue for `SelectionManager`/`SelectionPanel`. Treat `frameworks/ets/taihe/*/src` partly as generated.

## When editing SA on-demand behavior

`sa_profile/8500.json` controls start/stop on demand via `sys.selection.switch` and the `usual.event.USER_SWITCHED` common event. `etc/para/selection.para` sets defaults; `etc/para/selection.para.dac` sets DAC permissions. Changing switch defaults here affects when SA 8500 loads on real devices.

## Verification loop (how to prove the task is done)

1. Build the component clean: `./build.sh --product-name rk3568 --ccache --build-target selectionfwk` — a clean build is the sanitizer/static-analysis gate; if it fails on CFI/UBSan, fix the code, do not weaken the `sanitize` block.
2. If you added a test or touched `service/`/`service/plugins/`/`interfaces/`, build and run tests: `./build.sh --product-name rk3568 --build-target selectionfwk_test`, then `hdc shell /data/local/tmp/selection_service_unit_test` on device.
3. If you touched a public header under `interfaces/inner_kits/selection_client/include/`, confirm `selection_client.versionscript` still matches the exported symbols and there is no ABI break.
4. If you touched `interfaces/idl/*.idl`, confirm the regenerated proxy/stub builds (it lands in `${target_gen_dir}`).

### Done definition
A task is done when: (a) `--build-target selectionfwk` succeeds with no new sanitizer errors; (b) relevant `*_test.cpp` for the touched path builds and passes; (c) generated code (IDL/Taihe) was regenerated, not hand-edited; (d) no `Do not` rule was violated; (e) any public-API, permission, or on-demand change flagged an `Ask before` item.

### Final response expectations
Report: the files changed (with `file:line` for the key edits), the exact build/test commands run and their pass/fail result, which `Do not`/`Ask before` items were considered, and any `Ask before` item that needs human escalation.

### Fallback if validation cannot run
If the OpenHarmony tree is not available (cannot run `build.sh`), say so explicitly, do not claim success, and at minimum: compile-check the changed `BUILD.gn` for the required `sanitize`/`visibility`/`ldflags` blocks, verify no generated file was hand-edited, and flag that on-device build/test validation is still pending.

## External OpenCode config

None — no `opencode.json`, `CLAUDE.md`, or `.cursorrules` exist in this repo at time of writing.
