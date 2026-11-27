# Chemical UI — one-page plan

**Status:** draft, reviewed and corrected · **Date:** 2026-11-27
**Full design:** [`../proposals/0016-native-ui.md`](../proposals/0016-native-ui.md)
**Execution:** [`EXECUTION.md`](EXECUTION.md) · **Contracts:** [`API.md`](API.md) +
[`CONTRACTS.md`](CONTRACTS.md) · **Text:** [`TEXT.md`](TEXT.md) ·
**Host:** [`HOST.md`](HOST.md) · **Protocol:** [`PROTOCOL.md`](PROTOCOL.md) ·
**Catalog/tests:** [`CATALOG.md`](CATALOG.md) · **Design:** [`DESIGN.md`](DESIGN.md) ·
**Gates:** [`BENCHMARKS.md`](BENCHMARKS.md) · **Evidence:** [`RESEARCH.md`](RESEARCH.md)

---

## Mission

> Build `ui`: a native, memory-safe, AI-first UI toolkit that ships with the
> Chemical compiler. It must make the performant, accessible, and visually
> consistent implementation the path of least resistance; make every pixel and
> interaction explainable and testable without a human; and run with the same
> semantics/layout on Linux, Windows, macOS, WASM, and headless. Native headless
> visuals are bit-exact with a pinned font; system-font, GPU, and browser-text
> paths are tolerance-only and say so.
>
> Declarative typed views → retained keyed tree → one display list consumed by
> a deterministic CPU reference renderer and an optional GPU backend (Vulkan/
> Metal/D3D12/WebGPU) → semantic tree + JSON-RPC/MCP debug protocol. No panics for expected failures,
> no UB, no `unsafe` in the safe surface, no per-frame allocations in steady
> state, no relayout of clean subtrees. No font is embedded by default.

---

## The decisions that define it

1. **Retained, keyed tree — not immediate mode.** Stable IDs, incremental work,
   one semantic model for pixels/a11y/tests.
2. **Runtime owns UI state; app owns the model.** `view(&self)`, `update(&mut
   self)`, semantic events, no `Rc<RefCell>`/signal panics.
3. **Generational `NodeId` + explicit `Key`.** Stale handles fail closed; list
   state cannot attach to the wrong row.
4. **CPU reference + optional GPU backend.** One display list feeds both; CPU
   gives bit-exact goldens (with a pinned font), GPU gives 4K/120 and heavy
   effects; GPU failure falls back to CPU, never to a crash.
5. **Typed tokens + typed style; no CSS cascade.** Predictable for agents and
   reviewers; theme swap is data.
6. **No new syntax/DSL in v1.** Plain Chemical functions/structs/interfaces;
   source maps, diagnostics, compile time, and size stay under control.
7. **Semantic tree is a first-class output.** Roles/names/states/actions/bounds/
   source locations derived from the same layout pass as paint.
8. **Virtual clock + headless + protocol from day one.** Animations, tests, and
   debugging are deterministic; no retrofitting.
9. **Thin hosts, one core.** Linux/Windows/macOS/WASM/headless implement only
   window/input/present/a11y; layout, text, paint, and semantics are shared.
10. **Freeze, then optimize.** `API.md` is the contract; changes require an
    approved amendment; internals stay fluid.
11. **System fonts first; no default font payload.** The theme uses the platform
    UI stack. Apps can pin a file, fetch an OFL font into a cache, or embed the
    chosen **42 KB** Latin subset (37 KB ASCII-only variant also available).
    Goldens pin a test font; system-font goldens are per-platform with tolerance.
12. **GPU ships in v1.1, not v1.** Vulkan (Linux/Windows), Metal (macOS), WebGPU/
    WebGL2 (WASM); CPU fallback always. P0.1 validates; "Vulkan" is a backend,
    not the API.
13. **Hello binary: 2.5 MB target / 3.5 MB hard ceiling** (LLVM, stripped, CPU,
    no font). Size is a gate with a per-module breakdown.
14. **Four AI-written public example apps** (settings, dashboard, chat, media)
    are a release gate, pushed to `chemicallang/ui-examples` with `gh`.
15. **No panic button.** Expected failures return `Result`/placeholders; panic
    is a reviewed last resort only when continuing is unsafe; no `unwrap`
    convenience APIs; no unwinding.

---

## v1 capability

- **Core:** view/tree/reconcile, flex/stack/wrap/grid/scroll layout, display
  list, CPU rasterizer, text shaping/editing/IME, input/focus/hit testing,
  semantics, animation, multi-window, error boundaries, typed-error cards.
- **Platforms:** Linux (X11/Wayland), Windows, macOS, WASM canvas + DOM
  semantics, headless. Multi-window, close-veto, native menus/dialogs, and
  notifications in v1; tray is v1.1.
- **Rendering:** CPU reference; `RenderBackend` interface and GPU track
  (Vulkan/Metal/D3D12/WebGPU) available as an optional v1.1 accelerator.
- **Text/fonts:** system UI stack by default (0 bytes embedded); optional
  `ui_fonts` cache/download/pin; 42 KB Inter Latin subset if an app wants
  identical typography; test-only pinned stack for goldens. Formats/shaping/
  bidi/IME per `TEXT.md`; WASM browser text is tolerance-only.
- **Kit:** primitives, layout, navigation, buttons/toggles, text inputs,
  selects/menus/popovers/tooltips/command palette, dialogs/sheets/toasts,
  virtual list/table/tree (100k), **charts**, forms, feedback, page patterns;
  light/dark/compact/high-contrast.
- **Agent tooling:** debug server (snapshot/find/inspect/act/clock/animation/
  screenshot/overlay/capture/replay/state/audit/perf/logs), `chemical ui` CLI,
  MCP adapter, in-app inspector, agent skill/catalog, generation eval harness.
- **Validation:** semantic/geometry/golden tests, fuzz + sanitizers, bench gates,
  cross-platform CI, four public example apps, published comparison report.
- **Public examples:** settings, dashboard, chat, media in
  `github.com/chemicallang/ui-examples`, built/benchmarked with both TCC and LLVM
  (LLVM for benchmarks), with screenshots and READMEs; written by AI agents at
  the end and pushed via `gh`.

**Later:** mobile, full portable complex shaping/color emoji, variable-font
rendering, SVG subset, video, tray, live reload, remote
rendering. GPU is a parallel track (P3.10–P3.15); move it before the kit if v1
must ship GPU.

---

## Stages and commits

| Stage | Packets | Outcome |
|-------|---------|---------|
| 0. Decisions | P0.1–P0.4 | Verified facts, frozen contracts, green scaffolding/scripts, exact design tokens |
| 1. Core | P1.1–P1.14 | Headless UI + first real-window walking skeleton |
| 2. Text & visual | P2.1–P2.11 | System-first typography, icons, images, effects, animation, design system |
| 3. Platforms | P3.1–P3.9 | Linux/Windows/macOS/WASM hosts, IME, a11y, damage/pacing |
| 3G. GPU (parallel) | P3.10–P3.15 | Vulkan/Metal/WebGPU backends, parity, device loss, GPU benchmarks |
| 4. Kit | P4.1–P4.12 | Complete catalog + charts + page patterns + guidelines |
| 5. Agent tooling | P5.1–P5.9 | Debug server, CLI/MCP, inspector, audit, eval, lint |
| 6. Validation | P6.1–P6.7 | Harness, goldens, fuzz, benches, CI matrix, comparison |
| 7. Examples + hardening | P7.1–P7.9 | Settings/dashboard/chat/media apps, hardening, docs/claims, public `ui-examples` repo |
| 8. Release | P8.1–P8.2b | Gate audit, bundles, updates/crash, final validation |

One packet = one subagent session = at least one reviewed commit. The parent
re-runs the packet's commands, opens the screenshots, checks the benchmark JSON,
and rejects stubs/skips/weakened tests before committing.

---

## Hard invariants

- No `panic`, `unwrap`/`expect`, `unreachable`, unchecked indexing, or `unsafe`
  in the safe surface. Panic is a reviewed last resort only when continuing is
  unsafe; expected failures return `Result`/placeholders.
- Zero heap allocations per frame after warm-up.
- No relayout of clean subtrees; no full repaint on hover/typing/scroll.
- Lists require keys; large data uses virtualisation.
- Chrome uses tokens only; raw colors/pixels are audited.
- CPU path always works; GPU is optional and falls back, never a correctness
  dependency.
- 0 embedded font bytes by default; system-first, opt-in pin/download/embed;
  goldens pin a font for bit-exactness.
- Every visual packet produces semantic + geometry + golden + overlay artifacts
  that a human/parent inspects.
- Every performance claim links to a scenario, machine, commit, and raw JSON.
- APIs freeze at P0.2; changes require an approved amendment.

Budgets (reference 1080p laptop, release): cold start ≤120 ms p50, empty PSS
≤12 MB, 1k-node dashboard p99 ≤6 ms, 100k-row scroll p99 ≤8 ms, 10k-node layout
≤1.5 ms, 0 allocs/frame, hello binary ≤2.5 MB target / 3.5 MB hard ceiling (0 bytes font), WASM hello ≤350 KB
brotli, optional pinned Latin subset ≈42 KB. Full
table and methodology: [`BENCHMARKS.md`](BENCHMARKS.md).

---

## Start here

1. Maintainer approves [`0016-native-ui.md`](../proposals/0016-native-ui.md).
2. Parent launches **P0.1** (feasibility/compiler probes; no library API
   changes).
3. P0.3a creates the branch/libraries/scripts; P0.2 freezes [`API.md`](API.md)
   + [`CONTRACTS.md`](CONTRACTS.md); P0.3b builds the test core; P0.4 signs the
   exact tokens/reference screens.
4. Proceed packet by packet through [`EXECUTION.md`](EXECUTION.md); update
   [`STATUS.md`](STATUS.md) after every commit.
