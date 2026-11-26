# Chemical UI — native, safe, AI-first UI toolkit

**Chemical · Proposal 0016**

> A native, memory-safe UI library that makes the performant and accessible
> implementation the path of least resistance for an AI agent, renders
> deterministically on every target, and exposes a first-class machine-readable
> view of the UI for inspection, testing, and debugging.

**Status:** draft, awaiting maintainer approval · **Area:** Libraries · UI ·
[review index](review.md) · **Execution plan:** [`../ui/EXECUTION.md`](../ui/EXECUTION.md)

---

## Goal fit

| Requirement | How this proposal satisfies it |
|-------------|-------------------------------|
| **0. Extreme performance** | No per-frame tree allocation, no relayout of clean subtrees, retained keyed tree, tile damage tracking, deterministic CPU reference renderer plus an optional GPU backend (Vulkan/Metal/D3D12/WebGPU) over the same display list, cached glyph/atlas/shadow resources, explicit virtualisation, and hard budgets with CI gates (`BENCHMARKS.md`). Class-leading startup, memory, 100k-row scrolling, and input latency on measured CPU scenarios; 4K/120 and heavy effects on the GPU track. |
| **1. Complete memory safety** | Runtime owns all mutable UI state; app owns the model; cross-frame references are generational `NodeId`s, never pointers; no `unsafe` in the safe surface; no `Rc<RefCell>`/signals/panic-on-borrow; stale handles fail closed; all decoding and layout are bounded and validated. |
| **2. Enforceable no-panic policy** | Typed `UiError` everywhere, total layout function, fallible allocation at boundaries, typed-error subtree containment, fault-injection tests, fuzz targets, `[no_panic]` wherever std permits, and a reviewed panic whitelist only for states where continuing would be unsafe. No `unwrap`/`expect` convenience APIs. Full compiler enforcement is a tracked dependency on std effect annotations, not a hidden gap. |
| **3. Runs everywhere** | One pure-Chemical core over a thin `Host` interface: Linux (X11/Wayland), Windows, macOS, and `wasm32-unknown-unknown` (canvas + DOM semantics), plus headless as a first-class target. Same layout, paint list, semantics, and goldens on every backend. |
| **Toolchain constraint** | Everything lives in `lang/libs/ui*` and ships with the compiler. No new runtime dependency for the core. Packages are split so unused widgets do not bloat apps; the `chemical ui` CLI is a tiny launcher plus a self-built Chemical tool, not a second large C++ subsystem. Binary-size budgets are release gates. |

---

## 1. Problem

Chemical has a strong web stack (`#html`/`#css`/`#universal`/`page`) and a
`webview`/`window` layer, but no native UI toolkit. That leaves four gaps:

1. **Native apps are either web views or hand-built platform code.** The
   webview route inherits browser memory, a JS runtime, DOM layout, and a
   second language for UI; the raw-window route has no widgets, no text editing,
   no accessibility, and no design system. Neither is a foundation for complex
   desktop apps.
2. **Native toolkits are hostile to AI agents.** GTK/Qt are enormous, borrow-
   checker-hostile or C-oriented, stringly typed, and have no machine-readable
   inspection protocol designed for an agent. Their debuggers assume a human
   clicking around. Their default aesthetics are inconsistent across platforms.
3. **Existing Rust toolkits each trade away something we cannot trade.**
   Immediate mode (egui/Dear ImGui) rebuilds and lays out every frame, lacks a
   persistent semantic tree, and has weak text/IME/a11y. Signals
   (Dioxus/Leptos) move borrow errors to runtime panics. Fully typed view trees
   (Xilem) stress compile time. `.slint`-style codegen produces huge generated
   files. None ships a deterministic renderer + agent protocol + enforced design
   system together.
4. **Agents cannot verify what they built.** They can generate a screen that
   compiles and looks plausible while omitting elements, breaking interaction,
   or failing accessibility. Research shows static fidelity is improving but
   interaction and completeness remain the dominant failures, and naive
   screenshot self-revision gives limited gains. The library must make the
   correct result *checkable*: semantic tree, geometry, input replay, time
   control, screenshots, audit, and performance counters.

The opportunity is not "another widget toolkit". It is the first native UI
library whose **primary user is an agent** and whose **primary output is a
verifiable UI**, not just pixels.

---

## 2. The brief, tightened

> Build `ui`: a native, memory-safe, AI-first UI toolkit that ships with the
> Chemical compiler. It must make the performant, accessible, and visually
> consistent implementation the path of least resistance; make every pixel and
> every interaction explainable and testable without a human; and run with the
> same semantics and visuals on Windows, Linux, macOS, and WASM.
>
> The library is declarative (typed view functions → a retained, keyed tree),
> owns all mutable UI state, uses generational handles instead of pointers, and
> renders through a deterministic CPU rasterizer behind a backend-neutral
> display list (GPU is an optional accelerator, never a correctness
> dependency). It exposes a first-class semantic tree and a local JSON-RPC debug
> protocol (snapshot, inspect, act, clock, capture, audit, perf) alongside
> screenshots. Every feature is gated by semantic, geometry, and visual tests
> plus hard performance budgets. No panics for expected failures, no UB, no `unsafe` in the safe
> surface, no per-frame allocation in steady state, no relayout of clean
> subtrees.
>
> The library must be broad enough for real apps — scrolling, virtualised data,
> text editing, IME, overlays, menus, drag, keyboard navigation, accessibility,
> theming, animation, multi-window — and extensible enough that an agent can
> build any 2D UI from primitives without escaping the safe API. It must not be
> limited by its own component set.

### Must

- **Safe by construction**: no memory bugs, no crashes, no runtime borrow
  panics, no unchecked indexing in shipped paths.
- **Fast by construction**: the easy API is the fast API; performance cliffs are
  absent or diagnosed.
- **AI-first**: semantic + protocol + headless + time control + audit are part of
  the library, not an afterthought.
- **Beautiful by default**: one excellent design system, complete states,
  tokens, page patterns, and objective audits.
- **Everywhere**: Linux, Windows, macOS, WASM, headless; identical layout and
  semantics.
- **Stable**: APIs freeze before v1; no repeated redesign after implementation.
- **Validated**: benchmarks, goldens, fuzzing, sanitizers, cross-platform CI,
  dogfood apps, and an agent-generation evaluation.

### Must not

- Be a browser/webview wrapper for the native path.
- Require a UI DSL or a second type system in v1.
- Depend on a GPU or a specific driver for correctness.
- Expose raw platform handles or `unsafe` in the public API.
- Use per-frame full-tree layout, per-node `Box<dyn Widget>`, or an implicit
  signal graph that can panic.
- Ship a "small demo kit" that cannot build a real app.

---

## 3. Principles

1. **The runtime owns UI state; the app owns the model.** Hover, focus, scroll,
   caret, selection, IME preedit, and animation live in the runtime. App data
   mutates only in `update`.
2. **Identity is explicit.** Every stateful/moving node has a `Key`; every
   cross-frame reference is a generational `NodeId`. No pointers in the safe
   surface.
3. **Declarative, not immediate.** The view is a description; the retained tree
   and layout survive frames. Unchanged subtrees are not rebuilt or relaid out.
4. **One display list, many backends and hosts.** Layout, paint list, text, and
   semantics are backend-agnostic; CPU and GPU consume the same list. Hosts only
   create surfaces, deliver input, and present pixels.
5. **Determinism is a feature.** Virtual clock, a pinned font, integer blend
   math, fixed-point coverage, seeded input, and headless rendering make a frame
   reproducible; system-font and GPU runs declare their tolerances.
6. **The semantic tree is the UI's public model.** Pixels, a11y, tests, and the
   debug protocol project the same tree; they never diverge.
7. **Fail closed, degrade gracefully.** Stale IDs, missing fonts, bad images,
   invalid layout, allocation failure, and protocol garbage produce typed errors
   and a safe fallback, never a crash.
8. **The fast path is the only path.** Tokens, virtualisation, cached text,
   damage tracking, and zero-allocation frames are defaults, not opt-ins.
9. **Beauty is a system, not a style.** A small token set, a complete component
   catalog, page patterns, and audits beat endless styling freedom.
10. **No new syntax before it pays.** Plain Chemical functions, structs, and
    interfaces. An optional macro may be added later only if it compiles to the
    same API and improves diagnostics.
11. **Freeze, then optimize.** Public contracts land before implementation;
    internals stay fluid. Breaking changes require an explicit amendment.
12. **Measure or it did not happen.** No performance or visual claim without a
    reproducible command, raw data, and a reviewed artifact.

---

## 4. Architecture

```
┌──────────────────────────────────────────────────────────────────────┐
│ App (user)                                                           │
│   model : AppState                                                   │
│   view(&self, &mut ViewBuilder) -> NodeId                            │
│   update(&mut self, &UiEvent, &mut UiCtx)                            │
│   paint(&self, CanvasId, &mut Painter)                               │
└───────────────▲──────────────────────────────────────────────────────┘
                │ semantic events / commands
┌───────────────┴──────────────────────────────────────────────────────┐
│ ui core (pure Chemical, no OS)                                       │
│  ViewBuilder → arena → Reconciler → retained NodeTree (NodeId, Key)  │
│       │                    │                    │                    │
│       │                    ├─ Layout (flex/grid/scroll/stack)        │
│       │                    ├─ Semantics tree (roles/states/actions)  │
│       │                    └─ Animation timeline (virtual clock)     │
│       ▼                    ▼                    ▼                    │
│  DisplayList ────────► CPU Rasterizer ────────► Surface (RGBA8)      │
│  (rects/text/paths)    (deterministic AA)                            │
│                                                                      │
│  Input router · focus · hit testing · text engine · image/font cache │
└───────────────▲──────────────────────────────────────────────────────┘
                │ Surface, InputEvent, Ime, Clipboard, A11yUpdate
┌───────────────┴──────────────────────────────────────────────────────┐
│ Hosts (thin)                                                         │
│  ui_native: Linux GTK3/X11/Wayland · Windows Win32 · macOS Cocoa     │
│  ui_web:    wasm32 canvas + DOM semantics overlay                    │
│  headless:  memory surface + virtual input (tests/CI/debug)          │
└───────────────▲──────────────────────────────────────────────────────┘
                │ JSON-RPC / MCP / CLI
┌───────────────┴──────────────────────────────────────────────────────┐
│ ui_debug + ui_test + ui_cli                                          │
│  snapshot · find · inspect · act · clock · capture · audit · perf    │
└──────────────────────────────────────────────────────────────────────┘
```

### 4.1 App loop

1. Host delivers input/timer/redraw events.
2. Runtime routes input (hit test, focus, widget state). Built-in widgets
   update runtime state and request only the necessary layout/paint work.
3. Semantic events go to `Application::update`. The app mutates its model and
   optionally calls `ctx.request_view()`.
4. If dirty, `view` builds a compact arena of view nodes.
5. Reconciler applies the view to the retained tree by `Key`: reuse, update,
   move, create, remove. Removed nodes run their drop logic exactly once.
6. Layout processes dirty nodes only (with relayout boundaries).
7. Semantics, display list, and damage tiles are produced from the same tree.
8. Rasterizer paints damaged tiles; host presents; stats are recorded.

### 4.2 View and reconciliation

- `ViewBuilder` allocates nodes from a per-frame arena; after warm-up a frame
  does **zero** heap allocations for the tree.
- Containers are explicit parent handles (`b.column(key, style)`,
  `b.child(parent, child)`); no closures, no stack discipline, no macros. This
  is easy for agents to generate and easy to debug.
- Reconciliation is keyed: a `Key` is `(label hash, optional integer id)`. Lists
  and any movable stateful node require keys; index keys are never implicit.
- Node state survives move/reorder by key; removal bumps the generation so stale
  handles fail closed.
- `view` is pure (`&self`); no hidden mutation, no IO, no layout reads.

### 4.3 Layout

- Constraints-down / sizes-up, single pass for non-wrapping containers.
- v1: row, column, stack, wrap, explicit-track grid, scroll, layers; sizing
  (`Auto/Px/Percent/Fill`), grow/shrink/basis, min/max, aspect, gap, padding,
  align/justify/baseline.
- No CSS cascade, no specificity, no arbitrary constraint solving. Typed
  values; invalid input clamps and reports.
- Dirty subtrees only; relayout boundary when a parent's size does not depend on
  the child.
- Targets: 10k nodes ≤1.5 ms, 100k ≤20 ms, single node ≤30 µs.

### 4.4 Paint and raster

- Display list of primitives: solid/rounded rect, border, shadow, text run,
  image, path, gradient, clip, opacity/transform layer.
- One deterministic CPU reference rasterizer: premultiplied RGBA8, fixed-point
  coverage, integer blending, sRGB-correct, no FMA-dependent rounding. Same
  bytes on x86-64 and ARM when the font is pinned.
- Cached resources: glyph atlas, shadow sprites (9-slice), gradient ramps,
  decoded images, layer buffers.
- Damage: 64×64 tiles, dirty propagation from changed bounds; scroll uses a
  blit + exposed strips; no full repaint on hover/typing.
- **GPU is a first-class optional backend over the same display list**, never a
  correctness dependency. Backends: Vulkan (Linux/Windows), Metal (macOS),
  WebGPU/WebGL2 (WASM), with D3D12 optional. The app never writes shaders; the
  display list is consumed by CPU or GPU. Device loss is contained and falls
  back to CPU; headless CPU remains the reference for tests.
- GPU resource model: instanced quads/rounded rects, alpha glyph atlas, image
  textures, clip/scissor, layer textures, cached shadow sprites/SDF, pipeline
  cache; no re-upload of clean atlases. Shaders are small, precompiled
  (SPIR-V/WGSL/MSL or a build-time cross-compile), and cached on disk.
- The GPU track is parallel (P3.10–P3.15). If v1 must ship GPU, those packets
  move before the kit; otherwise v1 ships CPU + the backend interface and v1.1
  ships the production GPU backend.

### 4.5 Text and fonts

- **System-first typography:** no font is embedded in the core or linked into
  every app. The default theme uses the platform UI stack (Segoe UI/SF Pro/
  Inter/Noto/DejaVu/sans-serif); apps can pin a file, download an OFL font into a
  cache, or embed a small subset explicitly.
- `ui_fonts` (optional package): `add_file`, `add_bytes`, `fetch(url, sha256)`,
  atomic cache writes, license metadata, hash verification, size limits;
  `use_inter_latin()` preset (≈42 KB TTF UI subset, OFL).
- The compiler ships **no font asset by default**. `ui_fonts` stores a URL +
  SHA-256 manifest and downloads to the user cache; an app can embed its own
  copy with `embed_file`/asset packaging. A default `ui` import links 0 font
  bytes, and the compiler distribution does not grow by a bundled font.
- Deterministic goldens pin a test font; system-font runs use per-platform
  tolerance. The debug protocol reports resolved fonts, paths, hashes, and
  missing-glyph counts.
- Portable font provider, metrics, fallback chain, test font fixture.
- v1 shaping: Latin/Greek/Cyrillic, kerning/ligatures, grapheme clusters,
  UAX #14 line breaking, ellipsis, tabs, bidi paragraph direction.
- Complex scripts: platform shapers (DirectWrite/CoreText/HarfBuzz) return glyph
  IDs and positions; our atlas rasterizes them from the same font file. WASM
  uses the browser text stack (Canvas2D/FontFace) for complex scripts and may
  emit canvas text ops instead of glyph runs in v1.
- Editing core: caret, selection, word/line navigation, undo/redo, clipboard,
  password, validation, horizontal scroll, IME preedit/commit.
- Cache shaped runs and measured paragraphs; editing re-shapes only the affected
  paragraph.

### 4.6 Input, focus, accessibility

- Normalized keys, pointer/touchpad with precise deltas, text and IME events,
  drag/drop, focus/blur, DPI and resize.
- Hit testing respects clip, transform, z-order, and pointer capture.
- One focus value (`Option<NodeId>`) validated each frame; Tab order follows the
  logical tree; roving focus inside composites; stale focus falls back
  deterministically.
- Capturing/bubbling is implemented by the runtime; user code receives semantic
  events, not raw callbacks that could re-enter the tree.
- The semantic tree (roles, names, values, states, actions, bounds, source
  location) is derived from the same nodes as paint. Platform bridges: AT-SPI,
  UIA, NSAccessibility, WASM DOM overlay. It is also the primary test/debug
  model.

### 4.7 Theming and components

- Typed tokens: semantic colors, 8-pt spacing, radii, elevation, typography,
  motion, focus ring, density, high contrast.
- `Style` is a typed value; no cascade, no global mutable style. Components
  accept tokens only for chrome.
- `ui_kit` ships primitives, layout, navigation, inputs, overlays, data views
  (virtualised), forms, feedback, and page patterns.
- Components are plain functions + `Props` structs + `ComponentMeta`; no trait
  objects, no codegen per element. Type erasure exists only at explicit
  extension points.
- Custom visuals use the `Canvas` primitive + `Painter`; custom interaction uses
  Canvas events + app messages. This is the escape hatch that keeps arbitrary
  2D UIs possible without letting agents bypass the design system for chrome.

### 4.8 Debug and agent tooling

- `ui_debug` runs only when enabled; localhost + token + discovery file.
- Methods: tree snapshot/find, node inspect (props, resolved tokens, layout,
  focus, hit path, component stack, source, animation), input injection,
  action invoke, virtual clock control, frame stepping, animation seek,
  screenshot, overlay, capture/replay, state snapshot/restore, audit, perf
  stats, structured logs, catalog/tokens.
- `chemical ui` CLI: `run`, `tree`, `inspect`, `click`, `type`, `key`, `shot`,
  `record`, `replay`, `diff`, `audit`, `bench`, `test`, `mcp`, `catalog`.
- MCP adapter exposes high-level workflows (snapshot, act, clock, screenshot,
  capture, audit, perf, catalog), not one tool per RPC method.
- In-app inspector (F12): tree, boxes, tokens, focus, event log, fps graph,
  pick-by-pointer, copy key/test id.
- Headless mode is identical to production except present; a screenshot is a
  deterministic frame, not a platform screenshot.

### 4.9 Testing and validation

- `ui_test` harness: semantic queries, input injection, virtual clock, frame
  stepping, geometry/semantics snapshots, golden images, audit.
- Goldens: headless CPU, pinned test font, fixed DPI/time/theme, bit-exact;
  system-font runs are per-platform with tolerance; 1×/2×; all component
  states/themes/densities/text scales.
- Fuzz: layout, style, events, fonts, images, protocol; invariants include no
  panic, finite geometry, paint inside clip, idempotent layout.
- Bench: scenarios in `BENCHMARKS.md`, counter gates in CI, wall-clock nightly,
  comparison report with raw data.
- Dogfood: settings, dashboard, chat/IDE apps built only with the library.
- Agent eval: 12 prompts scored by compile + semantic/interaction tests + audit
  + visual rubric + budgets.

---

## 5. Decisions

| ID | Decision | Why |
|----|----------|-----|
| ADR-001 | Declarative builder + retained keyed tree, not immediate mode | Immediate mode relayouts every frame, has no stable semantic identity, and weak a11y/text; retained keyed trees give stable IDs, incremental work, and testability. |
| ADR-002 | Runtime-owned UI state; app-owned model; messages/events | Avoids `Rc<RefCell>`/signal panics and re-entrant mutation; matches iced/Elm's ownership fit without per-node boxing. |
| ADR-003 | Generational `NodeId` + explicit `Key` | Prevents dangling references and state attaching to the wrong row after reorder; stale handles fail closed. |
| ADR-004 | CPU reference + optional GPU backend over the same display list | CPU gives reproducible tests, headless CI, remote/VM support, and no driver dependency; GPU gives 4K/120 and heavy effects. Neither is a correctness dependency on the other. |
| ADR-005 | Typed tokens + typed style; no CSS cascade | Predictable for agents and reviewers, no specificity wars, no silent string typos; theme swap is data. |
| ADR-006 | No new syntax/DSL in v1 | AI writes code; plain Chemical keeps source maps, diagnostics, compile time, and binary size under control. |
| ADR-007 | Semantic tree is a first-class output | One model for pixels, a11y, tests, debug; agents get structure, not just screenshots. |
| ADR-008 | Virtual clock + headless + protocol from day one | Animations/tests/debug become deterministic; no retrofitting. |
| ADR-009 | Thin host interface; GTK3/Win32/Cocoa/WASM hosts | Core stays portable; platforms own only window/input/present/a11y. |
| ADR-010 | Portable Latin text + platform shapers for complex scripts | Own the deterministic path; do not reimplement HarfBuzz/ICU in v1. |
| ADR-011 | Split packages (`ui`, `ui_kit`, `ui_host`, …) | Unused widgets do not bloat apps; keeps the compiler/toolchain budget. |
| ADR-012 | Layout: single-pass flex + explicit grid; no CSS | Predictable cost; avoids deep auto-size cliffs; still expressive. |
| ADR-013 | Virtualisation and keys are API requirements | Prevents the classic agent failure of 10k unvirtualised rows and lost state. |
| ADR-014 | Typed-error boundaries + a minimal, reviewed panic whitelist + fault injection | Expected failures must not abort; panic is only for unsafe-to-continue invariants, and no `unwrap`/`expect` is offered. No unwinding means this must be structural. |
| ADR-015 | API freeze before v1 | Agents and humans should not re-learn the library every release. |
| ADR-016 | GPU backends are Vulkan/Metal/D3D12/WebGPU behind `RenderBackend`; CPU fallback always exists | "Vulkan" is a backend, not the API. A single abstraction keeps apps portable, supports macOS/WASM, and allows device-loss fallback; no GPU-only UI feature. |
| ADR-017 | System-first fonts; no default embedded font; opt-in pinned/downloaded font | Zero font payload by default; avoids licensing/update burden and multi-MB CJK/emoji assets; the chosen optional preset is the **42 KB** Latin subset (37 KB ASCII-only variant remains available); tests pin a font for determinism. |
| ADR-018 | GPU ships in v1.1 as an optional track; first production backends are Vulkan (Linux/Windows), Metal (macOS), WebGPU/WebGL2 (WASM), CPU fallback | Keeps v1 focused while making GPU a real, portable backend. The P0.1 spike may substitute OpenGL/SDL3 only if it wins on startup/size/CI; "Vulkan" is a backend, not the API. |
| ADR-019 | Hello binary: **2.5 MB target, 3.5 MB hard ceiling** (LLVM, stripped, CPU only, no font) | Aggressive but achievable vs 17–18 MB Rust toolkits; a 42 KB font is ~1.7%. Size is a release gate with a per-module breakdown, not a vague aspiration. |
| ADR-020 | Four AI-written public example apps in `chemicallang/ui-examples` are a release gate | Synthetic tests do not prove usability; real apps prove the API, beauty, performance, both backends, and the agent workflow. They are written at the end and pushed with `gh`. |

---

## 6. API sketch

The full contract is in [`API.md`](../ui/API.md). A minimal app:

```chemical
import std
import ui
import ui_kit
import ui_host

using std::vector
using std::string

public variant Filter { All() Active() Done() }

public struct Todo {
    var id : u64
    var title : string
    var done : bool
}

public struct App {
    var todos : vector<Todo>
    var filter : Filter
    var next_id : u64

    public func view(&self, b : &mut ui::ViewBuilder) : ui::NodeId {
        var root = b.column(ui::key("root"), ui_kit::page_style())

        b.child(root, ui_kit::heading(b, ui::key("title"),
            ui_kit::HeadingProps.make("Todos", ui::TextRole.H1)))

        var list = b.column(ui::key("list"), ui_kit::stack_style())
        for(var i = 0u; i < self.todos.size(); i++) {
            var t = self.todos.get_ptr(i)
            var row = ui_kit::list_item(b, ui::key_of("todo", t.id), ui_kit::ListItemProps.make(t.id))
            b.child(row, ui_kit::checkbox(b, ui::key_pair("done", t.id, 0),
                ui_kit::CheckboxProps.make(t.done).label(t.title)))
            b.child(list, row)
        }
        b.child(root, list)

        var input = ui_kit::text_input(b, ui::key("new"),
            ui_kit::TextInputProps.make("New todo").placeholder("What needs doing?"))
        b.child(root, input)

        b.child(root, ui_kit::button(b, ui::key("add"),
            ui_kit::ButtonProps.make("Add", ui::key("action.add")).variant(ui_kit::ButtonVariant.Primary)))

        return root
    }

    public func update(&mut self, ev : &ui::UiEvent, ctx : &mut ui::UiCtx) {
        if(ev.key.equals("action.add") && ev.kind == ui::EventKind.Click) {
            var text = ctx.text(ui::key("new"))
            if(text.size() > 0) {
                self.todos.push(Todo { id : self.next_id, title : text.to_string(), done : false })
                self.next_id = self.next_id + 1
                ctx.request_view()
            }
        } else if(ev.key.starts_with("done")) {
            var id = ev.key.id()
            for(var i = 0u; i < self.todos.size(); i++) {
                var t = self.todos.get_ptr(i)
                if(t.id == id) { t.done = !t.done }
            }
            ctx.request_view()
        }
    }

    public func paint(&self, canvas : ui::CanvasId, p : &mut ui::Painter) { }
}

public func main(argc : int, argv : **char) : int {
    var app = App()
    return ui_host::run(&mut app, argc, argv)
}
```

What this sketch demonstrates:

- No per-node heap allocation or `Box<dyn>`; no closures; no macro.
- App state is plain Chemical; the runtime owns editing/scroll/focus state.
- Lists are keyed by record identity; a reorder preserves checkbox state.
- Input is semantic; the app asks for a rebuild only when its model changed.
- Custom painting is available but does not leak into chrome.

---

## 7. Safety model

| Hazard | Prevention | Verification |
|--------|------------|--------------|
| Use-after-free / dangling node | Generational `NodeId`; arena never returns a live pointer; removal bumps generation | 1M randomized arena/fuzz test; stale-handle tests |
| State attached to wrong list row | Explicit `Key` required for list items; state stored by key, not position | Reorder/insert/remove differential tests |
| Runtime borrow panic | No `Rc<RefCell>`/signals; runtime owns state; events are values | Static audit + API review |
| Re-entrant mutation during paint/layout | User callbacks run outside tree mutation; paint is read-only | Event-loop tests |
| Panic on expected failure | Total layout, checked math, bounded decodes, typed errors, typed-error boundary | Fault injection + fuzz + panic-whitelist audit |
| OOM abort | Fallible allocation at unbounded boundaries; degrade to placeholder | OOM injection tests |
| Dangling platform handles | Opaque `WindowId`; no raw handles in safe API | API audit |
| Protocol abuse | Localhost + token + 0600 discovery; no `eval`; bounded params | Security review + fuzz |
| `unsafe` creep | `ui::raw` only; audited; lint forbids it in app/kit code | `ui_lint` + grep gate |
| Data race | Single-threaded UI; `!Send`/`!Sync` on UI state; worker results via messages | Compiler + TSAN |
| Visual/a11y divergence | Semantics + paint derived from one tree/layout pass | Semantic vs pixel tests |
| Golden rot | Deterministic renderer + pinned fonts/time; review workflow | CI hash + parent approval |

Full compiler-enforced `[no_panic]` for every function is currently blocked by
std effect annotations and module defaults. The plan does not pretend otherwise:
it ships an expected-failure-free-of-panic implementation, a static audit, a
panic whitelist, and fuzzing, and adds `[no_panic]` annotations as the std work
lands. Panic is not an error channel: with no unwinding, an abort is a bug or a
last-resort safety action, never a substitute for `Option`/`Result`.

### 7.1 Panic is a scalpel, not an API

- **No panic button in the public surface.** The library exposes no `unwrap`,
  `expect`, `panic`, or `unreachable`-style convenience. Callers handle
  `Option`/`Result` or pick an explicit fallback.
- **Expected failures stay values.** Missing font/glyph, corrupt image, invalid
  layout, stale ID, IO error, protocol garbage, allocation failure, and widget
  misuse all return `UiError` (or a safe placeholder) and keep the app alive.
- **Panic only when continuing is unsafe.** A private `ui::internal::fatal` may
  abort for an invariant breach that would otherwise cause memory unsafety or
  corrupt state beyond recovery. Every site is reviewed, documented, and paired
  with a test that makes the state unrepresentable. It is not used for bugs that
  can be reported as `Internal`.
- **No unwinding, no try/catch.** Typed-error boundaries contain `UiError`
  subtrees; they cannot catch a panic. A panic is process-fatal and is a
  release-blocking defect, reported by the crash/debug path.
- **Debug vs release.** Assertions may abort debug builds for programmer
  contracts. Release builds must return `UiError.Internal` and render a safe
  fallback where it is safe to continue.
- **Lint and audit.** `ui_lint` bans `unwrap`/`expect`/`panic`/`unreachable` in
  the library and app UI code; tests may assert; examples must show error
  handling. The release audit reviews the panic whitelist line by line.
- **No `unwrap` in examples.** Example apps demonstrate `Result`/`Option`
  handling, error cards, and fallbacks — not shortcuts that teach agents to
  sprinkle aborts.

---

## 8. Performance model

The performance claim is **specific and measured**, not universal:

- **Zero work when idle**: no timers, no repaints, no allocations.
- **Incremental by default**: typing/hover/scroll touch only the affected nodes
  and tiles; no full-tree layout.
- **No per-frame allocation**: arena view + cached text/shapes/atlas.
- **Virtualised data**: 100k rows at 60/120 fps with bounded memory.
- **Fast startup**: no JS runtime, no DOM, no GPU init for the CPU reference
  path; GPU init is cached and falls back.
- **Scales with the machine**: one display list; CPU for compatibility/CI,
  Vulkan/Metal/D3D12/WebGPU for 4K/120 and heavy effects. No GPU-only UI
  features.
- **Bit-exact headless with a pinned font**: the same CPU renderer powers tests,
  CI, and screenshots; system-font runs use per-platform tolerance.

Budgets (reference 1080p laptop, release): cold start ≤120 ms p50, empty PSS
≤12 MB, 1k-node dashboard p99 ≤6 ms, 100k-row scroll p99 ≤8 ms, 10k-node layout
≤1.5 ms, 0 allocations/frame, hello binary ≤2.5 MB target / 3.5 MB hard with 0 font bytes, WASM hello
≤350 KB brotli, optional pinned Latin subset ≈42 KB. GPU: 4K 60/120, ≤80 ms
device init, ≤64 MB GPU memory, device-loss recovery ≤250 ms. The full table,
scenarios, statistics, and CI policy are in
[`BENCHMARKS.md`](../ui/BENCHMARKS.md).

The README may only brag after the release gate has raw data. When a competitor
wins a scenario, the report says so. That is what makes the win credible.

---

## 9. AI-first properties

### 9.1 Forcing performant code

- The builder is arena-based; there is no per-node allocation API to misuse.
- Lists require keys and expose virtualisation as the default data view; a
  non-virtualised 10k list is an audit error.
- Text editing, scrolling, focus, selection, and animation are runtime-owned;
  agents do not reimplement them (and cannot accidentally make them O(n²)).
- `view` is pure; there is no layout read or blocking IO in the view API.
- Styles are values; tokens prevent per-widget style recomputation and
  accidental theme branches.
- `ui_lint` rejects raw chrome styles, missing accessible names, unkeyed lists,
  blocking calls in `view`, and `unsafe` in UI code.
- `ui audit` catches overflow, huge static lists, contrast, tap targets, focus
  escapes, and raw values at runtime.

**Failure → mechanism map**

| Observed agent failure | Mechanism that prevents or catches it |
|------------------------|----------------------------------------|
| Omits hover/focus/disabled/error states | Every component ships a state matrix; the gallery golden fails if a state is missing. |
| Builds a 10k-row `Column` | `VirtualList` is the default data view; `audit` errors on huge non-virtualised lists. |
| Keys list items by index | List APIs require a `Key` derived from record identity; lint flags index/position keys. |
| Uses raw hex/pixels for chrome | Kit props accept tokens only; `ui_lint`/`audit` flag raw values outside `Canvas`. |
| Forgets accessible names | Icon-only components require `a11y_label` at the type level; audit fails missing names. |
| Reimplements text editing / scroll | Runtime-owned widgets; the API does not expose a "build your own editor" path for standard inputs. |
| Blocks the UI thread in `view` | `view` has no IO/socket/worker API; `[no_blocking]`/lint checks where available. |
| Rebuilds the whole tree per keystroke | Typing updates runtime text state + paint only; `view` is called only on `request_view`. |
| Reads layout during view | Layout access is not in `ViewBuilder`; only tests/debug can read boxes. |
| Hardcodes light/dark branches | Components use tokens; `ui audit` flags theme conditionals/raw colors. |
| Animates on wall-clock time | Animations are declarative on the virtual clock; direct clock reads are not in the animation API. |
| Verifies only with a screenshot | Semantic tree + geometry + audit + perf are the primary test/debug channels; screenshots are secondary. |
| Suppresses a failure with unwrap/try-catch | No `unwrap`/`expect`/panic convenience; no unwind/try-catch; errors are typed values and typed-error boundaries report them. |
| Grows a list/string until OOM | Fallible growth at boundaries; audit/lint flags unbounded patterns; OOM degrades to a placeholder. |

### 9.2 Forcing beautiful UIs

- One design system with a real type ramp, spacing scale, elevation, focus ring,
  density, dark/high-contrast, and motion tokens.
- Complete components with every state, not a demo set.
- Page patterns (dashboard, settings, auth, chat, master-detail, data table,
  empty/loading/error) that agents compose instead of inventing layout.
- A written `GUIDELINES.md` with do/don't screenshots and an objective audit
  (alignment, hierarchy, contrast, line length, density, radii).
- An agent evaluation harness scores generated screens on compile, behavior,
  audit, visual rubric, and budget; the catalog/docs improve from failures.

The objective checklist (enforced by `ui audit` where possible):

| Rule | Threshold |
|------|-----------|
| Spacing on the 8-pt scale (4-pt only for optical alignment) | error |
| Body text contrast | ≥4.5:1 (≥3:1 for ≥24 px or ≥19 px bold) |
| Interactive target | ≥24×24 logical px, ≥44×44 preferred |
| Line length | ≤75 characters (content column ≤65ch default) |
| Heading order | exactly one H1; no skipped levels |
| Radii/elevation/motion | token values only; ≤3 elevation levels per screen |
| Status color | never the only signal (icon/text too) |
| States | empty, loading, error, disabled, focus defined for every data view |
| Responsive | 640/1024/1440 breakpoints tested; no horizontal overflow |
| Motion | 120–240 ms for UI transitions; reduced-motion path required |
| Focus | always visible; never hidden by overflow or color |
| Density | comfortable/compact both tested at 100%/200% text |

### 9.3 Debugging without screenshots

- `tree.snapshot` / `tree.find` / `node.inspect`: roles, names, values, states,
  actions, bounds, style, tokens, focus, hit path, component stack, source
  location.
- `perf.stats`: build/layout/paint/present time, nodes, commands, glyphs, tiles,
  allocations, frame ring.
- `log.tail` / `events.subscribe`: structured events, no log scraping.
- `clock.*` / `frame.step` / `animation.*`: pause, seek, step, inspect curves.
- `audit.run`: a11y, contrast, tap targets, overflow, tokens, huge lists.
- `state.snapshot` / `capture` / `replay`: reproduce a bug deterministically.
- Screenshots and overlays are available when pixels matter, but never the only
  interface.

### 9.4 Screenshots, visuals, animations

- `chemical ui shot` in headless mode returns a deterministic PNG; on a GPU
  window it reads back the presented texture (tolerance applies).
- `overlay.set` draws layout/text/focus/damage/fps overlays into the capture.
- `capture.start` records frame PNGs + timestamped input; `replay` reproduces
  frame hashes.
- `animation.seek` lets an agent inspect any point in an animation without
  waiting or racing wall-clock time.
- `diff` compares two PNGs with a threshold and writes a diff image.
- `gpu.info`/`perf.stats` and `fonts.list` make renderer and typography
  differences explicit in the capture metadata.

### 9.5 Public example apps

Four polished apps are written **at the end** by AI agents, using only the
public library, and published to **`github.com/chemicallang/ui-examples`**:

1. **Settings** — forms, validation, dialogs, search, keyboard, dark/light,
   200% text, a11y.
2. **Dashboard** — 100k-row virtual table, charts, filters, resizing,
   empty/loading/error states.
3. **Chat** — virtual message list, IME composer, command palette, tabs, split
   panes, notifications.
4. **Media** — album grid, 10k-track playlist, context menus, drag-reorder,
   animated Canvas visualiser, GPU path.

The repo has a root README (what each app demonstrates, screenshots, quick
start), a per-app README (features, screenshots, architecture, build/run with
**TCC and LLVM**, LLVM recommended for benchmarks, benchmark table/raw JSON),
`BENCHMARKS.md`, `bench/`, and an MIT license. The implementing agent creates
and pushes it with `gh` and verifies a fresh clone. These apps are the final
"is it actually usable?" gate; they are benchmarked, audited, screenshot-tested,
and must not use `unwrap`/`panic`/`unsafe`.

---

## 10. Compiler, plugin, and platform work

The plan is library-first, but the following are explicit dependencies. P0.1
verifies each and P0.2 freezes the fallback.

| Area | Needed | Risk | Fallback |
|------|--------|------|----------|
| macOS windowing | Cocoa `NSWindow`/`NSView` host; `.m` shim via `link c` | `window` has no macOS backend; `.m` support unverified | Add a `window` macOS backend or a UI-specific host; if `.m` fails, compile a `.c` shim using the Objective-C runtime API |
| IME | Preedit/commit/candidate rect on GTK, Win32, Cocoa, DOM | Missing in `window` today | Extend `window` events or host directly; mock IME tests until platform integration lands |
| Input normalization | Cross-platform key codes, precise wheel/touchpad, pointer leave, modifiers | `window` exposes platform codes and integer delta | New `ui::input` normalization + host extensions |
| WASM host ABI | Canvas/WebGPU present, rAF, pointer/key/text/IME, clipboard, DPI, semantics DOM, font fetch | Current ABI is DOM-only, scalar | Extend `lang/runtime/wasm` + `host.mjs` + compiler import table; keep scalar |
| Text shaping | DirectWrite/CoreText/HarfBuzz shims | System deps; not portable | Portable Latin in v1; complex scripts degrade to tofu + warning if shim absent; WASM uses Canvas2D |
| Fonts | System enumeration, cache/download, hash verification, WASM `FontFace` | OS APIs/permissions; no font on minimal Linux | System stack + optional pinned 42 KB subset + tofu/warning; test font fixture |
| GPU backend | Vulkan (Linux/Windows), Metal (macOS), WebGPU/WebGL2 (WASM), D3D12 optional; SPIR-V/WGSL/MSL shaders | Driver/CI matrix, shader toolchain, device loss | `RenderBackend` interface, CPU fallback, one production backend first; software adapters in CI |
| Accessibility | AT-SPI/UIA/NSAccessibility shims | Platform APIs are large | Ship the semantic tree + protocol first; bridges follow; never fake a11y |
| No-panic effects | `[no_panic]` on std + module defaults | Current checker rejects unannotated std calls | Panic-free implementation + audit + fuzz; add annotations incrementally |
| Dead-code elimination | Unused `ui_kit` components must not bloat apps | TCC may link all functions | Split packages/files; measure per-app size; add linker GC if needed |
| Debug CLI | Self-built `ui_cli` cached by the compiler launcher | Adds a small C++ launcher | If self-build is too slow, ship a tiny C++ client for the core RPC subset |
| `ui_lint` | CBI plugin reading UI AST | CBI plugin maintenance | Runtime `ui audit` already covers most rules; lint can land later |

No compiler change is allowed to hide a library design flaw. If a dependency
blocks a packet, the subagent must stop and document it (see `EXECUTION.md`
§0.6).

---

## 11. Alternatives considered

| Alternative | Why rejected |
|-------------|--------------|
| **Webview/HTML for native** | Two languages, JS runtime, DOM layout/memory, poor startup/memory, no native a11y contract, and it does not solve agent inspection (it only moves it to Chrome DevTools). |
| **GTK/Qt widgets** | Enormous dependency, inconsistent visuals across platforms, C/GObject or C++ ownership, no deterministic headless reference, and no AI-first protocol. |
| **Pure immediate mode (egui/ImGui)** | Full relayout, no stable semantic tree, weak text/IME/a11y, and state attaches by ID heuristics. |
| **Signals/`Rc<RefCell>`** | Moves borrow errors to runtime panics, incompatible with no-panic and with stale-node safety. |
| **Fully typed view trees (Xilem-style)** | Compile-time cost and trait/generic complexity at app scale; type erasure is needed at component boundaries anyway. |
| **`.slint`-style DSL + codegen** | A second type system, huge generated files, compile-time and binary-size cost, and source-map/debug friction. |
| **Vulkan-only GPU** | Simple story on Linux/Windows, but excludes macOS/WASM and Vulkan-less machines; `RenderBackend` with CPU fallback is more scalable. |
| **Bundled UI font by default** | A 42 KB Latin subset (37 KB ASCII-only) is cheap, but it does not solve CJK/emoji (4–11 MB), weights (≈150–350 KB), licensing/updates, or brand fonts; ship 0 bytes by default and make pinning/download opt-in. |
| **GPU-only renderer** | Driver bugs, shader compilation jank, CI nondeterminism, WASM/VM reach; CPU is the reference and fallback, GPU is an accelerator. |
| **Canvas-only WASM** | Loses accessibility, selection, native text input, and SEO; the DOM semantics overlay is required. |
| **A web-only component kit** | Does not satisfy native performance, startup, memory, or offline requirements. |

---

## 12. Scope

### v1 (this plan)

- Core: view/tree/reconcile/layout/display list/CPU reference raster/text/input/
  focus/semantics/animation/error model, plus the `RenderBackend` interface used
  by CPU and GPU.
- Platforms: Linux (X11/Wayland via one host), Windows, macOS, WASM canvas +
  DOM semantics, headless.
- Text: system-first fonts (0 bytes embedded by default); Latin/Greek/Cyrillic
  shaping, bidi paragraph direction, line breaking, IME, editing; complex
  scripts via system shapers; optional `ui_fonts` pin/download/embed.
- Kit: primitives, layout, navigation, inputs, overlays, virtualised data
  views, forms, feedback, page patterns; light/dark/compact/high-contrast.
- Agent tooling: debug protocol v1, CLI, MCP, inspector, audit, capture/replay,
  state snapshot, agent docs/skill, eval harness.
- Validation: goldens, geometry/semantics tests, fuzz/sanitizers, bench gates,
  cross-platform CI, four public example apps, comparison report.

### GPU track (parallel; v1.1 by default)

- P3.10–P3.15: backend selection/resources, Vulkan (Linux/Windows), Metal
  (macOS), WebGPU/WebGL2 (WASM), parity/device loss, benchmarks/CI.
- Move before the kit if v1 must ship a GPU backend. The public API and app code
  do not change; CPU fallback is always tested.

### Later (additive)

- Mobile (Android/iOS), Wayland-native/raw X11 hosts for smaller footprint,
  full portable complex shaping/color emoji, variable-font rendering, SVG
  subset, charts library, video, native tray/notifications, live code hot
  reload, remote/network rendering, plugin widgets, RTL polish, i18n/locale
  formatting, more platforms (FreeBSD), and a `ui2` only if a semantic break is
  unavoidable.

### Explicit non-goals

- 3D/game engine, video editor timeline, browser engine, or document editor.
- A visual designer/canvas app (the inspector is a debugger, not a design tool).
- A second language or DSL.
- Replacing the web stack; `ui` is for native apps and WASM app surfaces, while
  `#html`/`#universal` remain the web-page stack. They share design tokens.

---

## 13. Milestones

The detailed packet list, exit criteria, evidence requirements, and parent/
subagent loop are in [`../ui/EXECUTION.md`](../ui/EXECUTION.md). Summary:

| Stage | Packets | Outcome |
|-------|---------|---------|
| 0. Decisions | P0.1–P0.3 | Verified facts, frozen contracts, green scaffolding + verify scripts |
| 1. Core | P1.1–P1.13 | Headless UI: view/tree/layout/paint/raster/input/semantics/errors |
| 2. Text & visual | P2.1–P2.11 | System-first typography, icons, images, effects, animation, design system |
| 3. Platforms | P3.1–P3.9 | Linux/Windows/macOS/WASM hosts, IME, a11y, damage/pacing |
| 3G. GPU (parallel) | P3.10–P3.15 | Vulkan/Metal/WebGPU backends, parity, device loss, GPU benchmarks |
| 4. Kit | P4.1–P4.11 | Complete component catalog + page patterns + guidelines |
| 5. Agent tooling | P5.1–P5.9 | Debug server, CLI/MCP, inspector, audit, eval, lint |
| 6. Validation | P6.1–P6.7 | Harness, goldens, fuzz, benches, CI matrix, comparison |
| 7. Examples + hardening | P7.1–P7.9 | Four polished apps (settings/dashboard/chat/media), hardening, docs/claims, public `chemicallang/ui-examples` repo |
| 8. Release | P8.1–P8.2 | Gate audit, packaging, templates, final validation |

Each packet is a commit (or a small commit series) with evidence. Stages merge to
`main` only at their gates.

---

## 14. Acceptance criteria

The proposal is complete when all of the following are true on the release
commit:

1. **Capability**: the four example apps (settings, dashboard, chat, media)
   build and pass their interaction, semantic, audit, and golden tests using
   only `ui`/`ui_kit`; no app reaches into `ui::raw` for chrome.
2. **Safety**: ASAN/UBSAN/TSAN clean; 24h fuzz clean; fault injection produces
   typed errors/error cards; no `unwrap`/`expect`/`unreachable` in library or
   examples; every panic-whitelist site is justified and tested; no `unsafe` in
   the safe surface.
3. **Performance**: every CPU budget in `BENCHMARKS.md` met on the reference
   profile; GPU budgets met when the GPU track is in scope; counters within 2%
   of baseline; zero steady-state allocations; idle CPU/wakeups zero.
4. **Visual**: every component/state/theme/DPI has reviewed goldens; bit-exact
   headless on x86-64 and ARM **with a pinned font**; system-font goldens are
   per-platform with tolerance; GPU readbacks within tolerance; overlay captures
   reviewed.
5. **Accessibility**: semantic audit clean; keyboard-only suite passes;
   AT-SPI/UIA/NSAccessibility smoke tests pass; manual screen-reader checklist
   signed.
6. **Agent tooling**: an agent can, from a cold app, list the tree, inspect a
   node, click/type, pause and seek an animation, capture a replay, run an
   audit, inspect GPU/font state, and read perf stats — all through the CLI/MCP,
   without screenshots.
7. **AI eval**: 12-task generation eval meets the baseline success/quality
   threshold defined in P5.8/P7.7.
8. **Portability**: Linux X11 + Wayland, Windows, macOS, WASM, and headless
   green in CI; the same scene yields the same layout/semantics on all; CPU
   fallback works when GPU init/device fails.
9. **Stability**: API.md frozen and amended only through the recorded process;
   no known breaking change queued for v1.
10. **Size**: hello-world and dogfood binary/WASM budgets met with 0 embedded
    font bytes; unused kit components are eliminated or documented; an optional
    pinned Latin subset stays within ~42 KB.
11. **GPU (if in scope)**: every primitive has a CPU and GPU path; device loss
    recovers or falls back; no GPU-only UI feature; per-driver tolerances and
    software-adapter CI.
12. **Public examples**: `github.com/chemicallang/ui-examples` exists, contains
    the four apps, each README builds/runs with TCC and LLVM (LLVM recommended
    for benchmarks), screenshots are present, benchmarks reproduce, and the repo
    was verified from a fresh clone.

---

## 15. Risks and open questions

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| Scope is a multi-month toolkit, not a library | High | Stage gates; vertical slices; four public example apps prove value early; defer mobile/live reload; GPU is a parallel track |
| Text shaping/bidi is deeper than expected | High | Portable Latin first; system shapers for complex; tests; explicit tofu fallback |
| macOS host and IME require compiler work | Medium | P0.1 spike; Cocoa shim via `link c`; `window` backend fallback |
| CPU raster misses 4K/120 or heavy blur | Medium | Budgets split by scenario; damage tracking; GPU track; no false universal claim |
| GPU backend matrix is large (Vulkan/Metal/D3D12/WebGPU) | High | P0.1 spike compares native set vs SDL3 GPU vs OpenGL; `RenderBackend` keeps CPU fallback; ship one production backend first (Vulkan or OpenGL), add the rest |
| GPU driver/device-loss bugs | Medium | Software-adapter CI, device-loss recreation, CPU fallback after 2 failures, per-driver tolerance |
| Shader build/CI toolchain grows | Medium | Precompiled SPIR-V/WGSL/MSL; commit artifacts; build-time only, never user compile |
| System fonts differ or are absent (minimal Linux/containers) | High | Platform stack + fallback; optional pinned 42 KB font; tofu/warning instead of crash; no-font test container |
| Runtime font download is a privacy/security surface | Medium | Opt-in only, HTTPS, SHA-256, size limit, atomic cache, license metadata, no silent network |
| WASM canvas/DOM bridge grows | Medium | Scalar ABI, small host surface, Playwright tests, bundle budget |
| Compile time/binary size regress | Medium | Split packages, measure per packet, DCE check, no codegen per element, 0 font bytes by default |
| Design system becomes a straitjacket | Medium | Canvas escape hatch + additive components; tokens are data; theme overrides |
| Agent docs/catalog drift from code | High | Generate catalog from `ComponentMeta`; docs are a test artifact |
| Goldens flake across platforms | Medium | CPU reference, pinned font for bit-exact, geometry first, per-OS tolerance for system fonts/present/GPU |
| Debug server becomes a security hole | Low | localhost, token, 0600, no eval, bounded params, security review |
| Public API churn after release | High if rushed | Contract freeze at P0.2, additive policy, dogfood before v1, amendment log |
| No-panic enforcement blocked by std | Known | Audit + fuzz now; annotate incrementally; track in STATUS blockers |

Open questions for the maintainer:

1. Is `feature/ui-v1` the right long-lived branch, or should packets land
   directly on `main` behind a build flag?
2. Is GTK3 acceptable as the v1 Linux host dependency (as Flutter uses), or is a
   raw X11/Wayland host required before v1?
3. Should `ui_lint` be a compiler plugin at v1, or can runtime `ui audit` ship
   first?

Resolved: GPU is v1.1 with Vulkan/Metal/WebGPU (P0.1 may swap in OpenGL/SDL3 if
it wins); fonts are system-first with a 42 KB optional Latin preset; hello is
2.5 MB target / 3.5 MB hard; four public examples are a release gate.

---

## 16. Dependencies and unblocks

**Depends on:** MIR consolidation (stable backends), the existing `font`,
`image`, `window`, `std`, `json`, `net`, and `test` libraries, WASM host work
(P3.7), and the safety effect work only for full no-panic enforcement.

**Unblocks:** native desktop apps written in Chemical; a shared design system
between native and web; agent-first app development; a credible native
performance story; future GPU/mobile backends over the same display list.

---

## 17. Evidence and references

Research summary and decisions: [`../ui/RESEARCH.md`](../ui/RESEARCH.md).
Primary sources include:

- egui / Dear ImGui: <https://github.com/emilk/egui> ·
  <https://github.com/ocornut/imgui> ·
  <https://github.com/emilk/egui/discussions/5587>
- iced: <https://book.iced.rs/architecture.html> ·
  <https://github.com/iced-rs/iced>
- GPUI / Zed: <https://gpui.rs/> · <https://zed.dev/blog/videogame>
- Xilem: <https://github.com/linebender/xilem>
- Slint: <https://slint.dev/> ·
  <https://slint.dev/blog/porting-slint-to-microcontrollers.html> ·
  <https://github.com/slint-ui/slint/issues/4082>
- Taffy: <https://github.com/DioxusLabs/taffy>
- Vello: <https://github.com/linebender/vello>
- Parley / cosmic-text: <https://github.com/linebender/parley> ·
  <https://github.com/pop-os/cosmic-text>
- AccessKit: <https://accesskit.dev/how-it-works/>
- Design2Code: <https://salt-nlp.github.io/Design2Code/>
- DCGen: <https://arxiv.org/abs/2406.16386>
- UICoder: <https://arxiv.org/abs/2406.07739>
- Interaction2Code: <https://arxiv.org/abs/2411.03292>
- WebGen-Bench: <https://arxiv.org/abs/2505.03733>
- shadcn/ui: <https://ui.shadcn.com/docs> ·
  <https://ui.shadcn.com/docs/mcp>
- Radix: <https://www.radix-ui.com/primitives/docs/overview/introduction>
- W3C design tokens: <https://tr.designtokens.org/format/>
- Playwright MCP: <https://github.com/microsoft/playwright-mcp>
- Chrome DevTools MCP: <https://developer.chrome.com/blog/chrome-devtools-mcp>
- Flutter goldens: <https://api.flutter.dev/flutter/flutter_test/matchesGoldenFile.html>
- egui_kittest: <https://docs.rs/egui_kittest/latest/egui_kittest/>
- GUI memory study: <https://szibele.com/memory-footprint-of-gui-toolkits/>
- Rust GUI startup comparison:
  <https://lukaskalbertodt.github.io/2023/02/03/tauri-iced-egui-performance-comparison.html>
