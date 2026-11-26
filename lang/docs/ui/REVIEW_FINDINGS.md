# Chemical UI — adversarial review findings and resolutions

**Date:** 2026-11-27 · **Status:** all P0 findings resolved in the plan; P1s
resolved or explicitly scoped. This file is the audit trail for the six reviews
(architecture/perf, safety, AI usability, product completeness, sequencing,
text/i18n).

Legend: **Fixed** = contract/plan changed; **Scoped** = explicitly deferred with
a named release; **Verified** = P0.1 probe remains (no decision left to the
implementer).

---

## P0 — contract/blocker findings

| # | Finding | Resolution |
|---|---------|------------|
| 1 | Virtualised list impossible: `view` cannot see scroll offset; scrolling must rebuild | `CONTRACTS.md` §2: all state keyed by `Key`; `b.scroll_offset`; `ItemSource`/`virtual_list`; virtual scroll triggers one view; ordinary scroll paints only. `API.md` updated. |
| 2 | Responsive layout impossible: no viewport/scale in `view` | `FrameInfo`/`Viewport`/`Breakpoint` passed to `view`; `b.viewport()`, `b.frame()`, `UiCtx.viewport/scale`; resize/DPI triggers view. |
| 3 | Hover/focus/pressed cannot change visuals without rebuild | `b.state_style(node, WidgetState, Style)`; runtime resolves states + transitions; no view rebuild. |
| 4 | Frozen API structural holes: duplicate `Size`, no aspect/grid/absolute/overlay/slots/fragments, `Path`/`Gradient`/`ImageId` undefined | `Dim` vs `Size`; `Style.aspect/position/inset/grid_*`; `b.grid_cell/fragment/overlay/component/slot`; `Path`/`Gradient`/`ImageHandle` defined; `CanvasPaint`. |
| 5 | No host present/frame-trigger contract; GTK3 has no DIB/SHM present | `HOST.md` §2–3: `Presenter.acquire/present/resize/request_frame`, surface formats, damage, double buffer, units, per-platform mechanisms, `HostCaps`. |
| 6 | “Single-pass” layout false; 10k 1.5 ms faster than Taffy with more features | `CONTRACTS.md` §5: explicit 3-pass pipeline; named scenes; 10k simple 1.5/2.5 ms, 10k mixed 4/6 ms, 100k 30/45 ms; warm 0. |
| 7 | Raster determinism contradictory (sRGB vs integer, pixel centre, tiles) | `CONTRACTS.md` §6: sRGB-encoded UNORM integer formula, pixel-centre/half-open, 4×4 coverage, tile scissor, atlas R8/eviction rules. |
| 8 | Zero-alloc/OOM claims impossible with `std::vector/string` panics; no alloc counter | `CONTRACTS.md` §7.3: fallible UI buffers, OOM policy, linker-wrapped allocator, cold/steady split; std blocker recorded. |
| 9 | `font` cannot load system fonts (CFF/TTC/cmap12/composite), `GlyphOutline` lacks contours | `TEXT.md` §1 support matrix; P1.5b metrics fixture; `GlyphOutline.contour_ends`; platform bitmap fallback; P2.1/P2.2 scope. |
| 10 | Text cache key missing font identity/DPI/direction; `measure_text` contextless | `TEXT.md` §2: `TextLayoutKey` with font epoch/scale/width/direction/features; `TextContext.measure/measure_rich`; separate shaping/break caches. |
| 11 | `Key` holds borrowed label; `id()` undefined; `@direct_init` blocks additive fields | `Key` now `{hash,tag,id,label_id}` with interned labels; `key_pair` id=a/tag=b; scoped siblings; collision buckets; `UiEvent/Payload` lifetime-parameterized; additive fields via `@make`/`Other`/API index. |
| 12 | Multi-window/shutdown absent from `Application` | `Application` now gets `FrameInfo` (includes `WindowId`); `on_close`/`CloseAction`; `open_window/close_window/windows`; per-window tree/clock; P3.6. |
| 13 | WASM cannot share the same display list/text/visuals; host ABI missing | `HOST.md` §7 ABI; `TEXT.md` §5.1 BrowserCanvas vs PinnedFont; WASM tolerance-only; first layout waits for fonts; P3.7. |
| 14 | App error channel absent; error card unreachable | `Application` methods return `Result`; `b.error`/`ctx.report_error`; `UiError` codes/classes; error card contract; P1.13. |
| 15 | `unreachable`/traps in reused `image/font/window`; no-panic audit only UI layer | `CONTRACTS.md` §7.2 fatal criterion; `UI-PANIC-003` ban in UI + transitive deps; `PANIC_WHITELIST.md`; P1.13/P7.6. |
| 16 | OOM/error type allocates; vector/string/function abort | `UiError` allocation-free; `Limits`; fallible UI buffers; examples no unbounded growth; std blocker tracked. |
| 17 | Borrowed `string_view` in `Key`/`Payload`/`Semantics` can dangle | `UiEvent<'a>`/`Payload<'a>`; builder copies all inputs; retained/semantics own/intern text; `Key` interned; lifetimes compile-enforced. |
| 18 | PNG/TTF decode unchecked (bombs, overflow, no caps) | `Limits` caps; checked arithmetic; `try_resize`; decode gate; fuzz/differential tests; P2.8/P6.3. |
| 19 | Debug server/UI-thread race; no task/worker API | `CONTRACTS.md` §9: UI-thread-only, worker `EventSender`/`UserEvent`, bounded protocol queue, shutdown; P1.10/P5.1. |
| 20 | Panic whitelist untestable; Internal vs fatal undefined; arithmetic policy missing | `CONTRACTS.md` §7.2: objective fatal criterion; whitelist schema (invariant/proof/child-process probe/expiry); checked/saturating policy; P1.13/P7.6. |
| 21 | `Size` duplicate / `Painter.size()` wrong | `Dim` (axis sizing) vs `Size {w,h}`; all APIs updated. |
| 22 | EventKind payloads missing (key/scroll/resize/drop/IME/drag/user) | `CONTRACTS.md` §3 kind→payload table; typed payload structs; full `KeyCode`; local coords; P1.11. |
| 23 | No startup/task/timeout/async completion | `EventSender`/`UserEvent`, `set_timeout`, `request_frame`, `EventKind.Frame`; `Application::on_load`? Use `on_start`/state hooks in CONTRACTS §9 (P1.10). |
| 24 | No app `on_start`/async/state hooks for hot reload | `CONTRACTS.md` §9/§11: `on_start` optional companion, `EventSender`, `state.snapshot` hooks; P5.7. |
| 25 | Multi-window and charts in release gates but deferred | Multi-window is v1 (`FrameInfo`/P3.6); charts are v1 (`ui_charts`/P4.12); P7.8 CPU visualiser, GPU optional. |
| 26 | No platform shell: menus/dialogs/notifications/tray/lifecycle | `CONTRACTS.md` §11/P3.6/P8.2: `WindowSpec`, native menus/dialogs/notifications, `on_close`, `on_open_file`, `on_second_instance`; packaging. |
| 27 | No frame tick for Canvas animation | `request_animation_frame`/`EventKind.Frame`; `AnimationSpec` repeat/alternate in `CONTRACTS.md` §9; P2.10/P7.8. |
| 28 | Style cannot express absolute/grid/aspect/RTL/text align/wrap/ellipsis | `Style` additions in `API.md`/`CONTRACTS.md` §4; P1.6/P1.7. |
| 29 | No custom theme/high-contrast/density/shared web tokens | `Theme` setters, `ThemeMode.HighContrast`, `Density`, `DESIGN.md`; P1.5/P2.11. |
| 30 | No date/color picker; Dashboard needs date range | P4.4/P4.5 add Calendar/DateRange/Time/Color pickers; `datetime` wiring. |
| 31 | No drag/drop/touch/pen/wheel phase | `CONTRACTS.md` §3 drag/pointer payloads; HOST.md §4; P3.1/P4.8. |
| 32 | No shortcut registry/app undo | `CONTRACTS.md` §9: `bind_shortcut`/`CommandId`; app undo transactions; P1.11/P4.5. |
| 33 | No rich text/markdown/code editor/selectable text | `TextSpan` modifiers; `ui_md`/`CodeEditor` v1.1; read-only selectable text in P4.2; explicit scope. |
| 34 | Image formats thin (JPEG/WebP/GIF/SVG); one font weight | `ui_images` optional; P2.8; 42 KB preset single weight documented; multi-weight/variable in v1.1. |
| 35 | A11y missing relations/live/modal/secure/announce | `Semantics` additions + `UiCtx.announce`; P1.12/P3.8/P6.6. |
| 36 | No shipping/bundle/signing/update/crash story | P8.2 packaging contract: `chemical bundle`, asset manifest, installers, signing, update feed, crash reporter. |
| 37 | Test/debug cannot drive IME/drag/touch/multi-match | `CATALOG.md` §4 harness: `find_all/within`, IME/drag/touch, `wait_until`, `app` access; P5.2/P6.1. |
| 38 | `@make public struct`/`&[]` don't compile; snippets not compile-gated | API uses `@direct_init` + `@make func`; P0.2 compiles every declaration under `lang/tests/ui/contracts/`. |
| 39 | ASAN on TCC impossible; sanitizer plan fakeable | `BENCHMARKS.md` §5/§10: LLVM ASAN+UBSAN + C-emitted clang sanitizer; TCC never claimed; TSAN PR for protocol/workers. |
| 40 | No walking skeleton/performance prototype before deep investment | P0.1 raster prototype/go-no-go; P1.14 Linux walking skeleton before Stage 2; P3.2 hardens. |
| 41 | Test harness/bench/lint arrive after packets that need them | P0.3 ships `ui_test_core`, 3 bench scenes/baselines, minimal lint; P4.1 ships catalog/API index/lint; P5.9/P6.x harden. |
| 42 | API freeze not machine-checked | `ui_api_index.json` + CI diff gate; P0.2. |
| 43 | `gh` auth/org permission unprobed; publish before release audit | P0.1 probes `gh auth`/org; P7.9 creates **private** repo; P8.1 makes public after release audit. |
| 44 | Compiler size ceiling not in any gate | P8.1/BENCHMARKS: TCCCompiler ≤5 MB steady-state, first release 7–15 MB. |
| 45 | License/attribution audit missing | P8.1 + `TEXT.md` §7: fonts/icons/examples licenses, OFL notices, no proprietary fonts. |
| 46 | Config/asset paths CWD-relative break installed apps | P8.2 asset manifest/`resolve_asset`; HOST.md macOS app bundle. |
| 47 | Unbounded capture/screenshot/protocol params | `Limits` + `PROTOCOL.md` §1: 4 MiB/64 depth/5 s/100 rps, capture root, redaction. |
| 48 | Forgeable/stale handles (`ImageId`/`FontHandle`/`CanvasId`) | Private/generational/registry handles; `Option`/`Result`; placeholders; P1.2/P2.8. |
| 49 | `Key` collision/scope/equality unspecified; example broken | `CONTRACTS.md` §2: full tuple, collision buckets, sibling scope, order-sensitive pair, `id()`/`tag()`; example uses `id()`. |
| 50 | `settle`/`text_of`/`find_*` cannot report failure | `UiHarness` returns `Result`/`Option`; `find_all`/`within`; `CATALOG.md` §4. |

---

## P1 — resolved or explicitly scoped

| # | Finding | Resolution |
|---|---------|------------|
| 51 | GPU swapchain/persistent texture/partial present undefined | `BENCHMARKS.md` §2.6 + P3.10: persistent scene texture + blit, swapchain state, frames-in-flight, device loss, formats, shader source. |
| 52 | Atlas eviction vs “0 upload” vs 8 MB budget contradictory | `CONTRACTS.md` §6/TEXT §5: frame-boundary eviction, `atlas_generation`, R8 atlas, 16 MB budget, subpixel bins by size. |
| 53 | Idle 0 wakeups contradicted by 1 ms pump | `HOST.md` §2/§9: remove/replace pump, request_frame only; idle test after; P1.14/P3.9. |
| 54 | PSS/startup numbers inconsistent; GTK3 assumption | `BENCHMARKS.md` §1/§2.1: 960×640, two surfaces, startup breakdown; P0.1/P1.14 gate decides GTK3 vs raw X11. |
| 55 | Budget arithmetic inconsistent | `BENCHMARKS.md` §2.3/§2.4: cold/warm, named scenes, event→update 1 ms, event→present 10 ms, damage-only 4 ms. |
| 56 | Frame loop/coalescing/timestamp/timers unspecified | `CONTRACTS.md` §1.4/§9: one frame rules, virtual timestamps, `set_timeout`, `request_frame`, `clock.advance` 1/60. |
| 57 | Image/font loading synchronous | `CONTRACTS.md` §10: `ImageHandle`/states, async decode, cancellation, `fetch_font_async`; never decode in view. |
| 58 | Text input/IME/selection/controlled-uncontrolled ambiguous | `TEXT.md` §4: `TextEditState`, `set_text/selection`, controlled/uncontrolled, IME payload, secure redaction. |
| 59 | `Semantics` relations/modal/live/secure missing | `API.md` §8 + `TEXT.md` §6; P1.12/P3.8. |
| 60 | Key/handle stale/forgeable semantics | `CONTRACTS.md` §2; private/generational; tests. |
| 61 | Error taxonomy/recovery/source/retry missing | `CONTRACTS.md` §7.1 recovery table; `UiError`; error card. |
| 62 | Resource limits/cache budgets missing | `CONTRACTS.md` §8 `Limits`; `FrameStats`; tests. |
| 63 | Window host global loop/timer table unsafe | `HOST.md`/P3.1/P3.2: per-window session, tokenized timers, signal deregistration; ASAN/TSAN. |
| 64 | Retained text/reentrancy/std::function lifetime | `CONTRACTS.md` §3.2/§9; `HOST.md` §5; events copied/queued; callbacks `'static` or handle-scoped. |
| 65 | Protocol auth/path/limits/redaction | `PROTOCOL.md` §1; P5.1/P5.3. |
| 66 | Sanitizer/fuzz matrix fakeable | `BENCHMARKS.md` §5/§10; P6.3; negative seeded tests. |
| 67 | Freeze claims additive but types break | `ui_api_index.json`; `Other` + detail; companion interfaces; P0.2. |
| 68 | FFI UTF-16/path/integer widths | `HOST.md` §4/§9; `UiPath`/owned UTF-8; `i64/f64/usize`; P3.1. |
| 69 | Numeric policy incomplete | `CONTRACTS.md` §5/§7.3; `LayoutWarning` event; fuzz extreme values. |
| 70 | Painter unbalanced scopes | `CONTRACTS.md` §4.4: guards, max depth, auto-close, `paint` Result. |
| 71 | Event payload kind mapping/lifetimes | `CONTRACTS.md` §3; `UiEvent<'a>`; builder copies. |
| 72 | `Key` scope/collision | `CONTRACTS.md` §2. |
| 73 | Component catalog schema undefined | `CATALOG.md` §1; `ui_api_index.json`; P4.1. |
| 74 | Goldens self-certify omissions | `CATALOG.md` §5: ComponentSpec/PageSpec independent assertions; P6.2. |
| 75 | Image/icon async/lookup | `CONTRACTS.md` §10; `ui_images`; `ui_icons::find`; P2.8. |
| 76 | Multi-window/drag/shortcuts/close-veto | `CONTRACTS.md` §1.3/§3/§9; P3.6/P4.8. |
| 77 | Animation/theme scoping/invalidate ambiguity | `CONTRACTS.md` §4/§9; `state_style`, `animate`, `request_animation_frame`; P2.10. |
| 78 | Packaging/signing/updates | P8.2; `HOST.md` §8. |
| 79 | Responsive/200% text/RTL | `FrameInfo`, `Style` additions, `DESIGN.md`, P2.6/P4.7. |
| 80 | Date/color/rich text/charts gaps | P4.4/P4.5/P4.12; rich text/code editor explicitly v1.1. |

---

## Remaining open items (not contract holes)

1. **Branch:** `feature/ui-v1` vs `main` behind a flag (maintainer choice).
2. **Linux host:** GTK3 vs raw X11/Wayland for v1; P0.1/P1.14 measures and
   decides with a startup/PSS gate.
3. **`ui_lint`:** P4.1 minimal rules vs P5.9 CBI; the plan ships minimal early
   and adds CBI later.
4. **Text oracle dependency:** HarfBuzz/platform shaper approval is a P0.1
   dependency decision; portable Latin is the fallback.
5. **GPU first backend:** Vulkan/Metal/WebGPU is the recommended set; P0.1 may
   substitute OpenGL/SDL3 if it wins on size/startup/CI. GPU is v1.1.

Everything else that would affect a public signature, lifetime, layout, raster,
text, safety, or release gate is now written down in the normative docs.
