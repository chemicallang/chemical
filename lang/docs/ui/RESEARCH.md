# Chemical UI — research findings and the decisions they forced

**Date:** 2026-11-26 · **Status:** evidence for the plan in
[`../proposals/0016-native-ui.md`](../proposals/0016-native-ui.md)

This is not a literature review. Each section ends with the **decision** it
produced, so a reviewer can see why the architecture is what it is. Links are
kept so later packets can re-check claims instead of trusting this summary.

---

## 1. Architecture: immediate vs retained vs hybrid

**What the field does**

- **Dear ImGui / egui (immediate mode).** Cheap to start, linear code, but the
  whole UI is rebuilt every frame, layout happens before sizes are known (the
  "layout paradox"), and there is no persistent semantic tree unless one is
  bolted on. Dear ImGui explicitly does not do bidi/shaping or accessibility.
  egui's own README quotes **1–2 ms/frame** typical CPU; a reported native demo
  with many windows hit **~120 ms** vs ~13 ms in-browser. Large scroll areas are
  the known cliff.
  <https://github.com/ocornut/imgui> · <https://github.com/emilk/egui> ·
  <https://github.com/emilk/egui/discussions/5587>
- **iced (Elm).** Excellent ownership fit: `State + Message + pure view +
  update(&mut State, Message)`. Costs: `Element = Box<dyn Widget>` per node
  (allocation + vtable), no unified layout model, and frequent API breaks
  (`Command`→`Task`, `Application` removal, `container::Appearance`→`Style`).
  <https://book.iced.rs/architecture.html> ·
  <https://github.com/iced-rs/iced/blob/master/CHANGELOG.md>
- **GPUI / Xilem / Slint (hybrid or compile-time).** GPUI calls itself hybrid:
  an element tree is rebuilt, layout uses Taffy, paint is per-primitive shaders,
  entity state is retained. Zed budgets **8.33 ms/frame at 120 Hz**. Xilem diffs
  typed views into a retained Masonry tree; its 2026 review cites trait/generic
  explosion and long compile times. Slint compiles a `.slint` DSL to native code,
  inlines constants, computes dirty regions, and fits in **<300 KiB RAM** on an
  MCU—but a 28k-line `.slint` file generated **44.5 MB / 446k lines** of Rust and
  made incremental builds ~42 s dev / ~271 s release.
  <https://zed.dev/blog/videogame> · <https://gpui.rs/> ·
  <https://github.com/linebender/xilem> ·
  <https://github.com/slint-ui/slint/issues/4082> ·
  <https://slint.dev/blog/porting-slint-to-microcontrollers.html>
- **Leptos/Dioxus (signals).** Fine-grained updates avoid VDOM cost, but
  `Copy` signals panic on disposed reads and overlapping borrows; Dioxus
  documents that a disposed read is "similar to use-after-free … not undefined
  behavior". Runtime borrow panics are incompatible with Chemical's no-panic
  direction. <https://dioxuslabs.com/learn/0.7/essentials/basics/signals> ·
  <https://book.leptos.dev/appendix_life_cycle.html>

**Decision (ADR-001, ADR-002, ADR-003).** A declarative, typed view function
builds a compact node table in an arena; a keyed reconciler updates a **retained
scene tree** with generational IDs. Mutable UI state (hover, focus, scroll, text
edit, animation) is owned by the runtime; app state is owned by the app and
mutated only in `update`. No immediate-mode full rebuild, no `Box<dyn Widget>` per
leaf, no user-visible signals or `Rc<RefCell>`.

---

## 2. Layout

Taffy's published benchmark (M1 Pro, layout only) is the best hard data:
**10k nodes 2.69 ms** (~0.27 µs/node), **100k deep auto-size 164 ms** (Yoga
2.35 s), **300 nodes at depth 100 in 367 µs** (Yoga >9 min). Real websites are
**3,000–10,000 nodes**. Deep intrinsic/auto sizing is the cliff.
<https://github.com/DioxusLabs/taffy>

**Decision (ADR-012).** Ship a bounded 3-pass constraints-down/sizes-up layout (intrinsic measure → resolve/distribute → text re-break/place) with a linear flex subset in v1 (row/column/stack/wrap, grow/shrink/basis, min/max,
aspect, gap, padding, align/justify), plus an explicit track grid added as a
separate, bounded solver. No CSS cascade, no arbitrary constraint solving, no
implicit deep auto-size explosions. Budgets: **10k simple ≤1.5/2.5 ms**, **10k mixed ≤4/6 ms**, **100k ≤30/45 ms**, single fixed-width label **≤50/100 µs**; warm no-change 0. Layout is a total function: NaN/Inf/
negative sizes clamp and emit a diagnostic.

---

## 3. Rendering and text

- **GPU is not free.** Flutter built Impeller because runtime shader compilation
  caused jank; GPU drivers and CI make pixel determinism hard. Skia is large;
  wgpu is large on wasm (switching to glow "can significantly reduce"
  binary/wasm size). Vello is impressive but still adds a large GPU stack.
  <https://docs.flutter.dev/perf/impeller> ·
  <https://docs.rs/eframe/latest/eframe/> · <https://github.com/linebender/vello>
- **CPU raster is viable and deterministic.** Slint's software renderer and MCU
  port prove it; CPU raster also gives bit-exact goldens, headless CI, remote
  sessions, and no driver bugs. GPUI's per-primitive SDF approach (rounded rects,
  closed-form shadows, alpha glyph atlas with up to 16 subpixel variants, one
  instanced glyph draw) is the right primitive vocabulary even on CPU.
- **Text is the hard part.** Shape/cache runs by (text, font, size, features,
  width); use an alpha glyph atlas; handle fallback, bidi, line breaking, IME,
  emoji, and re-shaping per keystroke. Parley (Fontique + HarfRust + Skrifa +
  ICU4X) and cosmic-text (HarfRust + swash) show the required components.
  <https://github.com/linebender/parley> · <https://github.com/pop-os/cosmic-text>

### 3.1 GPU: yes, but as a backend, not the architecture

- **GPU is not free.** Flutter built Impeller because runtime shader compilation
  caused jank; wgpu is large on wasm (switching to glow "can significantly
  reduce" binary/wasm size); Vello is powerful but adds a large GPU stack. GPU
  drivers and CI make pixel determinism harder. <https://docs.flutter.dev/perf/impeller> ·
  <https://docs.rs/eframe/latest/eframe/> · <https://github.com/linebender/vello>
- **A 2D UI GPU path is tractable.** Instanced quads/rounded rects, an alpha
  glyph atlas, image textures, scissor/clip, layer textures, and cached shadow
  sprites cover the primitive set. GPUI's per-primitive approach is the model.
  <https://zed.dev/blog/videogame>
- **API choice matters more than the vendor.** Vulkan is excellent on
  Linux/Windows but excludes macOS/WASM and Vulkan-less machines. A
  `RenderBackend` over Vulkan/Metal/D3D12/WebGPU (or SDL3 GPU/OpenGL as a
  pragmatic first backend) keeps the app API stable and the CPU fallback
  universal. SDL3 3.2 has a cross-platform GPU API; it is an optional dependency,
  not part of the core. <https://github.com/libsdl-org/SDL/releases/tag/release-3.2.0>
- **Device loss is normal.** TDR/GPU reset/adapter removal must recreate
  resources and fall back to CPU after repeated failure; never crash the app.
- **GPU must not own correctness.** CPU remains the reference for headless,
  CI, VMs, remote sessions, and goldens; GPU is tested with per-driver tolerance
  on lavapipe/SwiftShader/WARP plus a real adapter.

### 3.2 Fonts: measure the bytes before deciding

Measured TTF sizes (locally, Inter 4.x + system fonts):

| Font | Size |
|------|------|
| Inter UI subset (ASCII + Latin-1 + punctuation, static 400) | **37 KB** |
| Inter Latin subset (U+0000–00FF + punctuation, static 400) | **42 KB** |
| Inter static, one weight, full charset | **224–342 KB** |
| Inter variable, full | **880 KB** |
| Inter variable UI subset | **115 KB** |
| Liberation Sans Regular | **411 KB** |
| Noto Sans Regular | **513 KB** |
| Lato Regular | **657 KB** |
| DejaVu Sans | **760 KB** |
| Noto Color Emoji | **11 MB** |
| CJK fallback (Droid Sans Fallback) | **4 MB** |

Chosen optional preset: the **42 KB Latin subset** (complete Latin-1 + punctuation,
static 400) over the 37 KB ASCII-first variant; the 5 KB difference buys better
coverage and is still ~1.7% of a 2.5 MB binary.

Facts and constraints:

- Platform UI fonts are not the same: Windows Segoe UI/Segoe UI Variable, macOS
  SF Pro/Helvetica Neue, Linux a distribution-dependent mix (DejaVu, Liberation,
  Noto, Cantarell). Minimal containers may have none.
- Segoe UI and SF Pro are proprietary; use via OS APIs, do not redistribute.
  Inter is OFL; DejaVu/Liberation/Noto are permissive (check each).
- Relying on system fonts means metrics and screenshots differ per machine.
  Deterministic goldens must pin a font file (test fixture or app-pinned).
- A 42 KB Latin subset is **≈1.7%** of a 2.5 MB binary and ~20 KB gzip — cheap.
  A full multi-script/emoji set is 4–11 MB and an update/licensing burden. One
  subset does not solve CJK/Arabic/emoji.
- The compiler ships no font asset by default; `ui_fonts` downloads a
  hash-verified subset to the user cache, and an app may embed its own copy. A
  default `ui` import links 0 font bytes.
- Runtime download is a privacy/security surface: opt-in, HTTPS, SHA-256, size
  limit, atomic cache, license metadata, no silent network.
  <https://rsms.me/inter/> · <https://openfontlicense.org/> ·
  <https://fonts.google.com/noto>

**Decision (ADR-004, ADR-010, ADR-016, ADR-017).** CPU is the reference and
always-available path; GPU is an optional backend over the identical display
list (Vulkan/Metal/D3D12/WebGPU, with a CPU fallback and device-loss recovery).
No font is embedded by default: system UI stack + optional `ui_fonts`
pin/download/embed (42 KB Latin subset preset; 37 KB ASCII-only variant) + a test-only pinned font for
deterministic goldens. Full portable complex shaping and color emoji remain
later work; WASM uses browser text for complex scripts.

---

## 4. Accessibility is the machine-readable UI model

AccessKit is the reference design: a Chromium-derived schema (stable integer
IDs, roles, attributes, actions), incremental tree updates, and adapters for
Windows UIA, macOS NSAccessibility, Unix AT-SPI, Android, iOS. It is used by
egui, Slint, Xilem, Bevy, and Freya. egui 0.37 ships an `inspection` feature
that exposes a TCP port so an external tool can read the AccessKit tree, inject
input, and capture screenshots.
<https://accesskit.dev/how-it-works/> · <https://docs.rs/eframe/latest/eframe/>

**Decision (ADR-007).** The semantic tree is a first-class output of the same
layout pass that produces pixels—never a separate hand-maintained tree. Stable
IDs are shared by focus, input, a11y, tests, and the debug protocol. Platform
a11y bridges are adapters over this tree. This directly gives agents a
structured interface that is *more* reliable than screenshots.

---

## 5. What AI agents actually fail at

- **Completeness and interaction, not just pixels.** Design2Code found GPT-4V
  output "exchangeable" in 49% of cases but weak at recalling every element and
  at layout; Design2Code-HARD block-match fell to 56.6 for GPT-4o. DCGen names
  the failure classes: **element omission, element distortion, element
  misarrangement**.
  <https://salt-nlp.github.io/Design2Code/> · <https://arxiv.org/abs/2406.16386>
- **Naive self-revision is weak.** UICoder needed compilers + multimodal scoring
  to improve; Design2Code's self-revision gave limited gains.
  <https://arxiv.org/abs/2406.07739>
- **Interaction is the dominant failure.** Interaction2Code (127 pages, 374
  interactions) found interaction generation lags static generation and
  visually subtle interactions fail. WebGen-Bench's best agent passed **27.8%**
  of 647 functional tests; runtime telemetry during repair raised one harness
  from 67.7% to 76.2%.
  <https://arxiv.org/abs/2411.03292> · <https://arxiv.org/abs/2505.03733> ·
  <https://arxiv.org/abs/2610.04981>
- **Accessibility is not solved by prompting.** A 2026 repair study found
  syntactically valid fixes >99.7% of the time but **fewer than 26% fully
  resolved**; iterative refinement added 52% cost and 1.64× API usage with no
  outcome gain. <https://arxiv.org/abs/2605.27716> · <https://arxiv.org/abs/2503.15885>
- **Constraints work.** shadcn/ui's success is a small, composable, typed
  interface plus copyable source; Radix is fully typed and consistent; Chakra
  uses W3C design tokens; Tailwind's benefit is "designing with constraints".
  A 36-session experiment found agents given only a component MCP broke 20/20
  team rules; adding a short rules file got 9/9 clean runs. When the registry had
  an equivalent component, agents chose the registry default.
  <https://ui.shadcn.com/docs> · <https://www.radix-ui.com/primitives/docs/overview/introduction> ·
  <https://tr.designtokens.org/format/> ·
  <https://www.supernova.io/blog/shadcn-mcp-design-system>
- **Semantic snapshots beat screenshots for agents.** Playwright MCP and Chrome
  DevTools MCP both prefer accessibility snapshots because they are structured
  and deterministic. <https://github.com/microsoft/playwright-mcp> ·
  <https://developer.chrome.com/blog/chrome-devtools-mcp>

**Decision (ADR-005, ADR-007, ADR-008, ADR-013).** Build the semantic tree,
virtual clock, and debug protocol *before* the pretty widgets. Make the catalog
small, typed, token-only, and inspectable. Ship a generate → render → inspect →
assert → repair loop as part of the library. Constrain styling so the fast/
accessible path is the only easy path.

---

## 6. Validation and benchmarks

- **Published anchors.** egui 1–2 ms/frame; Tauri/iced/egui cold start
  **380/230/280 ms**, binary **5/17/18 MB**, input delay **2–3 frames**;
  empty-window private RSS GTK3 **5 MB**, Qt **17 MB**, Dear ImGui **15.3 MB**,
  Electron **74.6 MB**; Slint **260 KB RAM** on RP2040; GPUI 120 Hz budget
  **8.3 ms**; Flutter hot reload example **978 ms**.
  <https://lukaskalbertodt.github.io/2023/02/03/tauri-iced-egui-performance-comparison.html> ·
  <https://szibele.com/memory-footprint-of-gui-toolkits/> ·
  <https://zed.dev/blog/120fps>
- **Honest measurement.** Report build/layout, paint/encode, GPU, and present
  separately; p50/p95/p99/p99.9 and missed-vsync counts; never averages alone.
  Gate deterministic instruction counts or internal counters; wall clock is
  advisory with statistical tests. Input-to-photon truth is a photodiode rig;
  CI proxies are frame timestamps, not photons.
  <https://docs.flutter.dev/perf/metrics> · <https://click2photon.com/>
- **Determinism.** Geometry snapshots are platform-independent and catch most
  layout bugs. CPU goldens need pinned fonts, 1×/2× DPR, grayscale AA, hinting
  off, sRGB, no decorations; GPU goldens need per-OS baselines and tolerance.
  Flutter uses the Ahem font for tests; Slint embeds fonts in its snapshot
  runtime; egui_kittest uses per-OS thresholds.
  <https://api.flutter.dev/flutter/flutter_test/matchesGoldenFile.html> ·
  <https://docs.rs/egui_kittest/latest/egui_kittest/> ·
  <https://docs.rs/slint-snapshot/latest/slint_snapshot/>
- **Reusable conformance corpora.** WPT `css-flexbox` has **1,833** tests,
  `css-grid` **3,812**; Taffy has **6,696** Chrome-generated fixtures. Use them
  as geometry oracles, not as a runtime dependency.
  <https://github.com/web-platform-tests/wpt/tree/master/css/css-flexbox>

**Decision (ADR-008, ADR-011).** Ship `ui_test` + a deterministic headless
backend + golden/geometry/semantic snapshots + fuzz targets + a bench harness
with committed baselines and CI gates. Publish a comparison report only with
methodology and raw data; never claim "beats everything" from one scene.

---

## 7. Codebase facts that constrain the plan

Verified in this tree (2026-11-26):

| Fact | Consequence |
|------|-------------|
| `lang/libs/window` has Windows and Linux/GTK3 backends, **no macOS backend** | P3.4 must add a Cocoa host (and/or a `window` macOS backend) before macOS can ship. |
| `window` has **no drawable/present/damage** API; `window_run` blocks in the global GTK/Win32 loop | `HOST.md`/P3.1 must add a per-window presenter and frame trigger; P1.14 walking skeleton proves it before Stage 2. |
| `window::Event` exposes platform key codes (`VK`/`GDK keyval`), integer wheel delta, no IME preedit/commit; Linux has no `EVENT_CHAR`; Win32 truncates UTF-16 | P3.1/P3.5 must normalize keys, deliver whole UTF-8, add precise wheel/touchpad, IME, pointer-leave. |
| `window` installs a 1 ms async timer for the process lifetime | Idle "0 wakeups" is impossible until P3.9 replaces it; budget/test updated. |
| `font` parses only `glyf`, cmap 0/4/6, no composite/CFF/TTC/cmap-12/OS2/name/GSUB/GPOS; `GlyphOutline` has no contour ends | `TEXT.md` support matrix; P1.5b metrics + P2.1/P2.2 parser/raster work; no "system font magically works". |
| `std::vector::push`/`string::append`/`std::function`/`Image.new` panic on OOM; `UiError` strings allocate | `CONTRACTS.md` §7.3: allocation-free errors, fallible UI buffers, std blocker recorded; OOM tests use a child process. |
| TCC has no sanitizer support (`scripts/test.sh` refuses `--sanitize` without LLVM) | Sanitizer matrix: LLVM ASAN/UBSAN + C-emitted clang sanitizer; TCC never claimed instrumented. |
| `image` decodes/encodes PNG/BMP/PPM and blits | Reuse for PNG screenshots and image nodes. |
| `link c "file.c"` works and compiles C files to objects; macOS `.m` support unverified | P0.1 verifies `.m`; add support if needed. |
| `lang/libs/cstd` has a wasm runtime/host ABI with DOM create/text/attr/event only | P3.7 must extend the host ABI for canvas presentation, input, IME, clipboard, semantics. |
| `[no_panic]` exists per function but rejects calls to unannotated std functions; module defaults are pending | Full no-panic enforcement is a compiler/std dependency; v1 is free of panics for expected failures by construction + audit + fuzz; a reviewed whitelist covers unsafe-to-continue invariants. |
| Interfaces support `&mut self` and generic impls (`Read`, `Write`, `Hashable`) | `Application` + a generic `run` bounded on `Application` is feasible; P0.1 confirms. |
| `std` has `vector`, `string`, `Option`, `Result`, `Arc`, mutex/atomics, `std::function` | Runtime can be built without new language features. |
| `intrinsics::get_raw_location()` and `intrinsics::embed_file()` exist | Debug source locations and opt-in font/icon embedding are possible. |
| The compiler must stay <5 MB and the first release may be 7–15 MB | `chemical ui` CLI must be a tiny launcher + self-built Chemical tool, not a large C++ subsystem. |
| WASM target is `wasm32-unknown-unknown`, freestanding, scalar host ABI | UI wasm backend uses `present(ptr,w,h,stride)` + host imports; no threads/SIMD in v1. |

---

## 8. The gap we are filling

What practitioners praise: instant native startup, low memory, keyboard-first
apps, accessibility, hot reload, great defaults, composability, type safety,
deterministic tests, DevTools.

What they complain about: platform inconsistency, text quality, borrow-checker
friction, runtime panics from signals, API churn, CSS specificity, goldens that
flake, DevTools that only work for web, and frameworks that make a beautiful UI
hard without a designer.

What no mainstream native toolkit ships together:

1. a deterministic headless renderer that is the *same* renderer as production;
2. a semantic tree + input/time/screenshot protocol designed for agents;
3. compile-time + runtime aesthetic/a11y enforcement;
4. a retained renderer with immediate-mode-level startup and no idle cost;
5. one API that runs on Windows, Linux, macOS, and WASM with identical visuals.

That combination is the product.
