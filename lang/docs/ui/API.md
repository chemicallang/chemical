# Chemical UI — frozen v1 contracts

**Status:** contract draft for approval · **Date:** 2026-11-26
**Rule:** once packet P0.2 is committed, this file is the source of truth. A
public signature, semantic rule, or protocol method may change only by an
explicit amendment section at the bottom of this file, approved by the parent
agent before code changes. Silent drift is a review failure.

Everything below is the target API. Implementation packets may leave a method
unimplemented behind a documented `NotImplemented` error, but may not invent a
different public shape. Function/constructor **bodies are elided** for
readability; `@make` structs have a matching constructor, and `@direct_init`
structs expose their fields. Fields shown are public contract; internal fields
may be added only if they do not change construction or equality semantics.

---

## 0. Packages and dependency direction

| Package | Role | May import |
|---------|------|------------|
| `ui` | Pure core: geometry, color, arena, view builder, tree, reconciler, layout, paint list, rasterizer, text, input, semantics, app model, headless run. No OS calls. | `std`, `cstd`, `image`, `font` |
| `ui_host` | Target facade: picks `ui_native` or `ui_web` and starts the loop. | `ui`, `ui_native`, `ui_web`, `ui_debug` |
| `ui_native` | Desktop hosts: Linux (GTK3 host), Windows (Win32), macOS (Cocoa). Window, input, IME, clipboard, present, a11y bridges; optional `ui_gpu_*`. | `ui`, `window`, `fs`, `osrand` |
| `ui_web` | `wasm32-unknown-unknown` host: canvas presenter, DOM event source, IME, clipboard, semantics DOM overlay; optional `ui_gpu_webgpu`. | `ui`, `browser` |
| `ui_kit` | Design system + components + page patterns. | `ui`, `ui_icons` |
| `ui_icons` | Curated vector icon path data (ISC/MIT). | `std` only |
| `ui_fonts` | Optional font cache/download/pin (`fetch`, SHA-256, atomic writes, OFL preset). Not imported by `ui`. | `std`, `fs`, `http`/`net` (native), `browser` (wasm) |
| `ui_gpu` | Optional GPU backend interface/resource cache; CPU is always available. | `ui` |
| `ui_gpu_vulkan` | Vulkan backend (Linux/Windows). | `ui`, `ui_gpu` |
| `ui_gpu_metal` | Metal backend (macOS). | `ui`, `ui_gpu` |
| `ui_gpu_webgpu` | WebGPU/WebGL2 backend (WASM). | `ui`, `ui_gpu`, `browser` |
| `ui_debug` | JSON-RPC debug server, discovery file, audit, capture/replay, inspector overlay. | `ui`, `json`, `net`, `fs`, `osrand` |
| `ui_test` | Headless harness: semantic queries, input, clock, goldens, geometry snapshots. | `ui`, `image`, `test` |
| `ui_cli` | `chemical ui …` client (self-built Chemical app). | `json`, `net`, `image`, `process`, `fs` |
| `ui_charts` | Charts on the Canvas primitive (later release). | `ui`, `ui_kit` |
| `ui_lint` | Optional CBI compile-time rules (keyed lists, tokens, a11y). | `compiler` |

**Dependency direction is one-way.** `ui` never imports a host, a widget, or the
debug server. `ui_kit` never imports a host. Hosts never contain widget logic.

App template:

```chemical
import ui
import ui_kit
import ui_host

public func main(argc : int, argv : **char) : int {
    var app = App()
    return ui_host::run(&mut app, argc, argv)
}
```

---

## 1. App model

```chemical
public namespace ui {

public interface Application {
    // Pure read of app state. Builds/reuses the view for this frame.
    func view(&self, b : &mut ViewBuilder) : NodeId

    // The only place app state may mutate in response to input.
    func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx)

    // Custom painting for Canvas nodes; leave the body empty if unused.
    func paint(&self, canvas : CanvasId, p : &mut Painter)
}

public func <A> run_headless(
    app : &mut A,
    options : Options
) : Result<std::Unit, UiError> where A : Application

} // namespace ui
```

Rules:

- `view` is called only when the runtime is dirty (first frame, resize, theme
  change, or `ctx.request_view()`). It must not mutate app state.
- Built-in widget interactions (hover, focus, scroll, caret, selection, IME
  preedit) update **runtime-owned** state and mark only paint/layout dirty. They
  do not call `view`.
- `update` receives semantic events. If it mutates model state that affects the
  view, it must call `ctx.request_view()`. This is explicit so typing in a text
  field does not rebuild the entire UI.
- `paint` is only called for `Canvas` nodes and must be deterministic for a
  given state; it must not allocate unboundedly or read the clock directly.
- No user callback runs while the tree is structurally mutated. Events are
  dispatched after layout and before the next optional rebuild.

### 1.1 Options and runtime flags

```chemical
@make
public struct Options {
    var title : string_view = "Chemical"
    var width : i32 = 960
    var height : i32 = 640
    var min_width : i32 = 320
    var min_height : i32 = 240
    var theme : ThemeMode = ThemeMode.System
    var scale : float = 0.0          // 0 = system DPI
    var headless : bool = false
    var debug : bool = false
    var debug_port : u16 = 0       // 0 = ephemeral
    var fixed_time : bool = false
    var reduced_motion : bool = false
    var vsync : bool = true
    var max_frames : u32 = 0       // 0 = unlimited; headless/testing

    // Rendering. CPU is the reference; GPU is an optional accelerator.
    var renderer : Renderer = Renderer.Auto
    var gpu_backend : GpuBackend = GpuBackend.Auto
    var gpu_prefer : GpuPreference = GpuPreference.Auto
    var allow_software_gpu : bool = false
    var gpu_frames_in_flight : u8 = 2

    // Typography. No font is embedded by default. An empty stack means the
    // platform UI stack; see ui_fonts for the optional download/pin package.
    var font_stack : &[string_view] = &[]
    var font_file : string_view = ""       // optional app-pinned font
    var font_cache : string_view = ""      // default: platform cache dir
    var font_size_scale : float = 1.0
}
```

`ui_host::run` parses and applies these app-level flags from `argv` (they are
removed before the app sees the remaining arguments):

| Flag | Meaning |
|------|---------|
| `--ui-debug[=PORT]` | Start the debug server (also `CHEMICAL_UI_DEBUG=1`). |
| `--ui-headless` | No window; render into memory; useful for CI. |
| `--ui-shot=PATH` | Render one frame, write PNG, exit. |
| `--ui-frame=N` | Frame to render with `--ui-shot` (default: first settled frame). |
| `--ui-at=MS` | Virtual time at which to render (implies fixed clock). |
| `--ui-size=WxH` | Override logical viewport. |
| `--ui-scale=F` | Override DPI scale (e.g. `2`). |
| `--ui-theme=light\|dark\|system` | Theme override. |
| `--ui-motion=off` | Force reduced motion. |
| `--ui-fixed-time` | Freeze the virtual clock. |
| `--ui-frames=N` | Headless frame count before exit. |
| `--ui-pid-file=PATH` | Discovery file path override. |
| `--ui-renderer=auto\|cpu\|gpu` | Force a renderer; `auto` chooses GPU when available and the scene/window warrants it. |
| `--ui-gpu-backend=auto\|vulkan\|metal\|d3d12\|webgpu\|opengl` | Force a GPU backend; `auto` picks the platform default. |
| `--ui-gpu-prefer=auto\|discrete\|integrated\|software` | Adapter preference. |
| `--ui-allow-software-gpu` | Allow lavapipe/SwiftShader/WARP for tests and VMs. |
| `--ui-font=PATH` | Pin a specific font file (also `CHEMICAL_UI_FONT`). |
| `--ui-font-family=NAME` | Start the fallback stack with a named family. |
| `--ui-font-cache=DIR` | Font cache directory for downloaded fonts. |
| `--ui-font-scale=F` | Multiply all text sizes (accessibility testing). |

Environment: `CHEMICAL_UI_DEBUG`, `CHEMICAL_UI_LOG` (`error|warn|info|debug|trace`),
`CHEMICAL_UI_BACKEND` (`native|web|headless`), `CHEMICAL_UI_RENDERER`,
`CHEMICAL_UI_GPU_BACKEND`, `CHEMICAL_UI_FONT`, `CHEMICAL_UI_FONT_CACHE`.

The target facade:

```chemical
public namespace ui_host {
    public func <A> run(app : &mut A, argc : int, argv : **char) : int where A : ui::Application
    public func <A> run_with(app : &mut A, options : ui::Options) : Result<std::Unit, ui::UiError> where A : ui::Application
}
```

`ui_host` selects `ui_native` or `ui_web` at build time; both call
`ui::run_with`. A headless build uses `ui::run_headless` directly.

### 1.2 Supporting types

```chemical
@direct_init public struct Point { var x : float; var y : float }
@direct_init public struct Rect { var x : float; var y : float; var w : float; var h : float }
@direct_init public struct Transform2D { var m : [6]float }
@direct_init public struct Shadow { var x : float; var y : float; var blur : float; var spread : float; var color : ColorToken }
@direct_init public struct TextSpan { var text : string_view; var role : TextRole; var color : ColorToken }
@direct_init public struct GridTracks { var columns : &[GridTrack]; var rows : &[GridTrack]; var gap : Space }
public variant GridTrack { Px(value : float) Fr(value : float) Auto() }
@direct_init public struct Duration { var ms : float }
public enum Easing { Linear, EaseIn, EaseOut, EaseInOut, Spring }
public enum AnimProperty { Opacity, TranslateX, TranslateY, Scale, Rotate, Background, Width, Height }
public enum Cursor { Arrow, IBeam, Hand, Wait, Crosshair, Move, ResizeNS, ResizeEW, ResizeNESW, ResizeNWSE, NotAllowed }
public enum ImageFit { Contain, Cover, Fill, None }
public enum FontFamily { Ui, Mono, System, Named }
public struct FontSpec { var family : FontFamily; var file : string_view; var fallback : FontFamily }
public struct ImageId { var value : u32 }
public struct IconId { var value : u32 }
public struct FontHandle { var value : u32 }
@direct_init public struct FontCacheEntry { var family : string_view; var path : string_view; var sha256 : string_view; var bytes : u64 }
public variant FontError {
    NotFound(family : string_view)
    InvalidFormat(message : string)
    LicenseRequired(message : string)
    DownloadFailed(message : string)
    CacheFailed(message : string)
    OutOfMemory()
}
public variant ImageSource { Path(path : string_view) Bytes(bytes : &[u8]) Placeholder() }

public enum Renderer { Auto, Cpu, Gpu }
public enum GpuBackend { Auto, Vulkan, Metal, D3D12, WebGpu, OpenGl }
public enum GpuPreference { Auto, Discrete, Integrated, Software }
@direct_init
public struct GpuInfo {
    var available : bool
    var backend : GpuBackend
    var device : string_view
    var driver : string_view
    var dedicated : bool
    var max_texture : u32
    var supports_compute : bool
    var software : bool
}
public struct WindowId { var value : u32 }
public enum ThemeMode { Light, Dark, System }
public enum KeyCode { Tab, Enter, Escape, Space, Backspace, Delete, Left, Right, Up, Down, Home, End, PageUp, PageDown, A, B, C /* … full set */, F1, F2, F12 }

@direct_init
public struct FrameStats {
    var frame : u64
    var renderer : Renderer
    var gpu_backend : GpuBackend
    var build_us : double
    var layout_us : double
    var paint_us : double
    var gpu_us : double
    var present_us : double
    var nodes : u32
    var relaid_out : u32
    var commands : u32
    var glyphs : u32
    var tiles_painted : u32
    var full_repaints : u32
    var allocs : u32
    var gpu_upload_bytes : u64
    var gpu_draw_calls : u32
}
```

The full key set and the `Rect`/`Point` helpers are defined in `ui::geom` and
`ui::input`; packets may add helper methods but not rename the types.

### 1.3 Rendering backend

The public API never names Vulkan, Metal, or D3D12 types. It exposes capability
and selection only:

```chemical
public namespace ui {
    public func gpu_info() : GpuInfo
    public func active_renderer() : Renderer
    public func force_renderer(r : Renderer) : Result<std::Unit, UiError>
}
```

- `Renderer.Auto` means: CPU in headless/deterministic mode; otherwise use a
  **linked** GPU backend when available and the window is large enough to
  benefit, with CPU fallback on init failure, device loss, or repeated GPU
  errors.
- GPU backends are opt-in packages: importing `ui_gpu_vulkan`/`ui_gpu_metal`/
  `ui_gpu_webgpu` registers it and links its code. `ui_host` does not import a
  GPU backend by default, so a CPU-only app links zero GPU bytes. `Gpu` with no
  linked backend returns a typed `Unavailable`/falls back to CPU.
- GPU is never required for correctness. A device loss recreates resources; if
  it fails twice, the runtime switches to CPU, emits an error event, and keeps
  the app alive.
- `GpuInfo` is for diagnostics/telemetry, not for branching app UI.
- A GPU backend consumes the **same display list** as the CPU renderer: the app
  never writes shaders or GPU code.

### 1.4 Font services

No font is embedded in the `ui` core or in every app binary. The default is a
platform UI font stack; an app can pin a file, embed a small subset, or fetch an
OFL font into a cache. Golden tests pin a test font.

```chemical
public namespace ui_fonts {
    // Platform UI stack: system-ui, Segoe UI, SF Pro Text, Inter, Noto Sans,
    // DejaVu Sans, sans-serif (the actual list is platform-specific).
    public func system_stack() : &[string_view]

    public func add_file(path : string_view) : Result<FontHandle, FontError>
    public func add_bytes(bytes : &[u8], name : string_view) : Result<FontHandle, FontError>
    public func set_default(handle : FontHandle)
    public func default_handle() : FontHandle

    // Optional network/cache flow. Verifies SHA-256 and atomic-writes.
    public func fetch(url : string_view, sha256 : string_view) : Result<FontHandle, FontError>
    public func cache_dir() : string_view
    public func list_cached() : &[FontCacheEntry]

    // Recommended OFL preset: a Latin subset (~42 KB TTF, chosen over the
    // 37 KB ASCII-first variant for complete Latin-1 coverage). This is opt-in;
    // it downloads to the cache unless the app embedded it.
    public func use_inter_latin() : Result<FontHandle, FontError>
}
```

Rules:

- System fonts are used through the OS API where required and are **never
  redistributed** by Chemical. Downloaded fonts must be permissively licensed
  (OFL/Apache/BSD) and are verified by hash.
- An app that embeds a font does so explicitly (`embed_file` or an asset); the
  library does not link a font into every app.
- `--ui-font=PATH` pins a file for deterministic screenshots; `--ui-font-family`
  starts a custom fallback stack. The debug protocol reports resolved font
  names, paths, and hashes.
- Missing glyphs fall back through the stack; if nothing covers the codepoint,
  render a tofu glyph and emit a `FontWarning` — never panic.
- Determinism is **conditional on a pinned font**. With system fonts, goldens
  are per-platform with tolerance (see `BENCHMARKS.md`); with a pinned font,
  headless goldens are bit-exact.

---

## 2. Identity: `NodeId`, `Key`, `CanvasId`

```chemical
@direct_init
public struct NodeId {
    var index : u32
    var generation : u32
}

@direct_init
public struct Key {
    var hash : u64
    var tag : u64
    var label : string_view

    public func equals(&self, label : string_view) : bool
    public func starts_with(&self, prefix : string_view) : bool
    public func id(&self) : u64
}

public func key(label : string_view) : Key
public func key_of(label : string_view, id : u64) : Key
public func key_pair(label : string_view, a : u64, b : u64) : Key

public struct CanvasId { var value : u32 }
```

Semantics:

- `NodeId` is a generational handle. `(0, 0)` is invalid. Freeing a node bumps
  its generation; a stale handle resolves to `None`, never to a different node.
- `Key` is the cross-frame identity used by reconciliation and tests. `key()` is
  FNV-1a over the bytes plus a stable tag; `key_of()` adds an integer identity
  (database id, index of a stable record, etc.). `label` is for diagnostics and
  protocol queries only. Labels passed to `key*` must have static lifetime
  (literals); dynamic lists use `key_of("todo", id)` and the integer id, not a
  formatted string.
- **Lists and any stateful node that can move must be keyed.** `ui_kit::list`
  and `ui_kit::virtual_list` require a `Key` per item. There is no implicit
  index key for list items.
- `CanvasId` names an app-owned painting routine; multiple Canvas nodes may
  share an id and are distinguished by `NodeId`.

---

## 3. View construction

```chemical
public struct ViewBuilder {
    // Containers (layout is set by Style; the method is a shorthand for display/direction).
    public func row(&mut self, key : Key, style : Style) : NodeId
    public func column(&mut self, key : Key, style : Style) : NodeId
    public func stack(&mut self, key : Key, style : Style) : NodeId
    public func wrap(&mut self, key : Key, style : Style) : NodeId
    public func grid(&mut self, key : Key, style : Style, tracks : GridTracks) : NodeId
    public func scroll(&mut self, key : Key, style : Style, axis : Axis) : NodeId
    public func canvas(&mut self, key : Key, style : Style, canvas : CanvasId) : NodeId

    // Leaves.
    public func text(&mut self, key : Key, value : string_view, style : Style) : NodeId
    public func rich_text(&mut self, key : Key, spans : &[TextSpan], style : Style) : NodeId
    public func image(&mut self, key : Key, src : ImageSource, style : Style) : NodeId
    public func icon(&mut self, key : Key, icon : IconId, style : Style) : NodeId

    // Tree structure.
    public func child(&mut self, parent : NodeId, child : NodeId)
    public func children(&mut self, parent : NodeId, kids : &[NodeId])
    public func insert_child(&mut self, parent : NodeId, index : u32, child : NodeId)

    // Per-node properties.
    public func style(&mut self, node : NodeId, style : Style)
    public func semantics(&mut self, node : NodeId, sem : Semantics)
    public func test_id(&mut self, node : NodeId, id : string_view)
    public func focusable(&mut self, node : NodeId, tab_index : i32)
    public func animate(&mut self, node : NodeId, prop : AnimProperty, to : float, duration : Duration, easing : Easing)
    public func transform(&mut self, node : NodeId, t : Transform2D)

    // Begin/end a reusable component that owns child construction.
    public func component(&mut self, key : Key, name : string_view) : NodeId
}
```

Rules:

- A node is created once; `child`/`children` append. Cycles are rejected with a
  diagnostic (never an infinite loop).
- The builder arena is reset at the start of every `view`; all `NodeId`s from a
  previous `view` are invalid as *view* handles but remain valid as *retained*
  handles. Public code should not cache view `NodeId`s; cache `Key`s instead.
- Text values are copied into the view arena for the duration of reconciliation;
  they do not need to outlive `view`.
- `Style` is a value type; there is no mutable global style state and no
  cascade.

Component pattern (plain functions, no trait objects, no macro):

```chemical
public func button(
    b : &mut ViewBuilder,
    key : Key,
    props : ButtonProps
) : NodeId
```

---

## 4. Style and tokens

```chemical
public enum ColorToken {
    Transparent,
    Surface, SurfaceRaised, SurfaceSunken, SurfaceOverlay,
    OnSurface, OnSurfaceMuted, OnSurfaceFaint,
    Primary, OnPrimary, PrimaryHover, PrimaryPressed,
    Secondary, OnSecondary,
    Danger, OnDanger, Success, OnSuccess, Warning, OnWarning,
    Border, BorderStrong, BorderFocus,
    FocusRing, Selection, Scrim,
    Chart1, Chart2, Chart3, Chart4, Chart5, Chart6
}

public enum Space { None, XXS, XS, SM, MD, LG, XL, XXL, XXXL }
public enum Radius { None, XS, SM, MD, LG, Full }
public enum Elevation { None, Low, Medium, High }
public enum TextRole { Display, H1, H2, H3, H4, Body, BodyStrong, Caption, Code, Label }
public enum FontWeight { Regular, Medium, Semibold, Bold }
public enum Overflow { Visible, Clip, Scroll }
public enum Align { Start, Center, End, Stretch, Baseline }
public enum Justify { Start, Center, End, SpaceBetween, SpaceAround, SpaceEvenly }
public enum Axis { Horizontal, Vertical, Both }

@direct_init
public struct Color { var r : u8; var g : u8; var b : u8; var a : u8 }

public variant Size {
    Auto()
    Px(value : float)
    Percent(value : float)
    Fill()          // grow to the parent's main-axis remaining space
}

@direct_init
public struct Insets { var left : float; var top : float; var right : float; var bottom : float }

@direct_init
public struct Border { var width : float; var color : ColorToken }

@make
public struct Style {
    public func new() : Style

    public func padding(&self, p : Insets) : Style
    public func padding_all(&self, s : Space) : Style
    public func gap(&self, s : Space) : Style
    public func width(&self, s : Size) : Style
    public func height(&self, s : Size) : Style
    public func min_width(&self, px : float) : Style
    public func max_width(&self, px : float) : Style
    public func grow(&self, f : float) : Style
    public func shrink(&self, f : float) : Style
    public func basis(&self, s : Size) : Style
    public func align_items(&self, a : Align) : Style
    public func justify(&self, j : Justify) : Style
    public func align_self(&self, a : Align) : Style
    public func background(&self, c : ColorToken) : Style
    public func foreground(&self, c : ColorToken) : Style
    public func border(&self, b : Border) : Style
    public func radius(&self, r : Radius) : Style
    public func shadow(&self, e : Elevation) : Style
    public func opacity(&self, f : float) : Style
    public func clip(&self, on : bool) : Style
    public func overflow(&self, o : Overflow) : Style
    public func text(&self, role : TextRole) : Style
    public func weight(&self, w : FontWeight) : Style
    public func cursor(&self, c : Cursor) : Style
    public func z(&self, z : i32) : Style
}
```

Rules:

- Chrome styling uses **tokens only**. `ui_kit` props do not accept `Color` or
  pixel values for colors/spacing; they accept `ColorToken`, `Space`, `Radius`,
  `Elevation`, `TextRole`. Raw `Color` is for data visualisation and user
  content, via `ui::raw`.
- `Style.padding`/`width` accept exact floats for layout primitives; the kit
  does not expose them in component props. `ui audit` flags raw values in
  component code outside `Canvas` painting.
- Dark theme is a token-table swap; components never branch on dark mode.
- 8-pt spacing scale; 4-pt half-step allowed only for optical alignment.
- Focus ring, disabled opacity, and reduced-motion behavior are theme tokens.

### 4.1 Theme

```chemical
public struct Theme {
    public func light() : Theme
    public func dark() : Theme
    public func high_contrast() : Theme
    public func color(&self, t : ColorToken) : Color
    public func space(&self, s : Space) : float
    public func radius(&self, r : Radius) : float
    public func text(&self, r : TextRole) : TextStyle
    public func elevation(&self, e : Elevation) : Shadow
    public func set_color(&mut self, t : ColorToken, c : Color)
    public func set_font(&mut self, role : TextRole, f : FontSpec)
}
```

### 4.2 Layout semantics (normative)

- **Sizing.** `Auto` = intrinsic content size; `Px` = exact; `Percent` = fraction
  of the parent's content box; `Fill` = take the remaining main-axis space
  (equivalent to `grow: 1` with `basis: 0`).
- **Main axis.** Children are measured in order; fixed children first, then
  `grow` distributes remaining space proportional to `grow`; `shrink` is used
  only when the total exceeds the container.
- **Cross axis.** `Stretch` gives the child the container's cross size;
  `Start/Center/End` use the child's intrinsic size; `Baseline` aligns first
  text baselines.
- **Padding** is inside the border and outside children. **Gap** is between
  children only.
- **Stack** places children at the same origin; later children paint on top;
  `align`/`justify` apply to the stack as a whole; per-child `align_self`
  positions an individual layer.
- **Scroll** measures children with unbounded main-axis max, clips to the
  viewport, and stores the offset in runtime state keyed by `NodeId`.
- **Grid** uses explicit tracks (`Px`, `Fr`, `Auto`) and explicit row/column
  placement; implicit placement is left-to-right, top-to-bottom. No CSS
  auto-placement heuristics.
- **Overflow** defaults to `Clip` for scroll and `Visible` elsewhere. Visible
  overflow still contributes to hit testing only inside the parent's clip
  unless `overflow: Visible` and the parent is a `Stack`.
- **Invalid input** (NaN, Inf, negative size) clamps to a finite value and emits
  a `LayoutWarning`; it never panics.

---

## 5. Events, input, and context

```chemical
public enum EventKind {
    Click, DoubleClick, ContextMenu,
    Press, Release, PointerMove, PointerEnter, PointerLeave,
    Scroll, KeyDown, KeyUp, TextInput, ImePreedit, ImeCommit,
    Focus, Blur, Change, Submit, Select, Open, Close,
    Drop, Resize, DpiChange, Timer, Command, Custom
}

@direct_init
public struct Modifiers {
    var shift : bool; var ctrl : bool; var alt : bool; var meta : bool; var caps : bool
}

public variant Payload {
    None()
    Bool(value : bool)
    Int(value : i64)
    UInt(value : u64)
    Float(value : double)
    Text(value : string_view)
    Range(start : float, end : float)
    Index(value : u32)
    Point(x : float, y : float)
}

@direct_init
public struct UiEvent {
    var window : WindowId
    var key : Key
    var kind : EventKind
    var payload : Payload
    var modifiers : Modifiers
    var local_x : float
    var local_y : float
    var timestamp_ms : double
    var frame : u64
}

public struct UiCtx {
    public func request_view(&mut self)
    public func invalidate(&mut self, key : Key)
    public func request_paint(&mut self)
    public func request_quit(&mut self)

    public func text(&self, key : Key) : string_view
    public func is_focused(&self, key : Key) : bool
    public func focus(&mut self, key : Key)
    public func scroll_to(&mut self, key : Key)

    public func clipboard(&self) : string_view
    public func set_clipboard(&mut self, text : string_view)
    public func open_url(&mut self, url : string_view) : Result<std::Unit, UiError>

    public func theme_mode(&self) : ThemeMode
    public func set_theme_mode(&mut self, mode : ThemeMode)
    public func theme(&self) : &Theme

    public func now_ms(&self) : double
    public func frame(&self) : u64
    public func stats(&self) : FrameStats
    public func window(&self) : WindowId
    public func set_window_title(&mut self, title : string_view)
}
```

Event routing:

- Hit testing walks the retained tree back-to-front inside clips; the deepest
  interactive node wins.
- `KeyDown`/`KeyUp` use normalized `KeyCode` values, not platform codes. Text
  input arrives as `TextInput`/`ImeCommit`, never as synthetic key codes.
- Capture phase (root → target) then bubble (target → root). A handler may stop
  propagation only through runtime-owned widget state, not a public callback.
- `Click` is synthesized only when press and release hit the same interactive
  node and the pointer did not leave; `DoubleClick` uses the OS threshold.
- Keyboard activation: Space/Enter on focused buttons, arrows within groups,
  Escape closes overlays, Tab follows the focus order. Every component's
  contract is in its catalog entry.
- Focus is a single runtime-owned `Option<NodeId>`, validated every frame. A
  stale focus target falls back to the nearest focusable ancestor, then the
  window root.
- `string_view` values returned by `UiCtx` (`text`, `clipboard`) and event
  payloads are valid until the next runtime mutation. Copy them (e.g.
  `to_string()`) if the app needs to retain them.

---

## 6. Text

```chemical
@direct_init
public struct TextStyle {
    var family : FontFamily
    var size : float
    var weight : FontWeight
    var line_height : float       // multiplier
    var letter_spacing : float
    var features : u32          // bit flags: tabular numbers, ligatures, etc.
}

@direct_init
public struct TextMetrics {
    var width : float
    var height : float
    var baseline : float
    var lines : u32
    var truncated : bool
}

public func measure_text(text : string_view, style : TextStyle, max_width : float) : TextMetrics
```

- Measurement is cached by `(text bytes, style id, max_width)` with a bounded
  LRU. Editing re-shapes only the affected paragraph.
- v1 shaping coverage: Latin/Greek/Cyrillic with kerning and standard ligatures,
  grapheme clusters, UAX #14 line breaking, ellipsis, tabs, and bidi paragraph
  direction. Complex scripts use a platform shaper when available; otherwise a
  visible tofu glyph plus a `FontWarning`.
- No font is embedded by default; the theme uses the platform UI stack. Apps can
  pin a file (`--ui-font`), fetch an OFL font into a cache, or embed a subset.
  Tests pin a small font fixture. System fallback is explicit, never ambient in
  deterministic test mode; missing glyphs render tofu + a warning, never panic.
- Text rendering is alpha-mask glyphs in an atlas with subpixel positioning;
  color emoji is v1.1.

---

## 7. Painter (custom Canvas content)

```chemical
public struct Painter {
    public func size(&self) : Size
    public func clip_rect(&mut self, r : Rect)
    public func clip_rounded(&mut self, r : Rect, radius : float)
    public func push_opacity(&mut self, opacity : float)
    public func pop_layer(&mut self)
    public func push_transform(&mut self, t : Transform2D)
    public func pop_transform(&mut self)

    public func rect(&mut self, r : Rect, color : Color)
    public func rounded_rect(&mut self, r : Rect, radius : float, color : Color)
    public func border(&mut self, r : Rect, radius : float, width : float, color : Color)
    public func line(&mut self, a : Point, b : Point, width : float, color : Color)
    public func path(&mut self, path : &Path, color : Color)
    public func gradient_rect(&mut self, r : Rect, g : Gradient)
    public func shadow(&mut self, r : Rect, radius : float, s : Shadow)
    public func image(&mut self, img : ImageId, r : Rect, fit : ImageFit)
    public func text(&mut self, text : string_view, pos : Point, style : TextStyle, color : Color)
}
```

Rules: painting is immediate into the current display list, clipped by the
Canvas node's bounds by default; no tree mutation, no IO, no clock reads.
`Path` is a small command list (move/line/quad/cubic/close) for icons and charts.

---

## 8. Semantics and accessibility

```chemical
public enum Role {
    Unknown, Button, Checkbox, Radio, Switch, Slider, TextField, MultilineTextField,
    Heading, Text, Image, Link, List, ListItem, Table, Row, Cell, Tree, TreeItem,
    Tab, TabList, TabPanel, Menu, MenuItem, MenuBar, Dialog, AlertDialog, Tooltip,
    ProgressBar, Status, Group, Form, Label, Separator, ScrollView, Canvas, Custom
}

@direct_init
public struct Semantics {
    var role : Role
    var label : string_view
    var description : string_view
    var value : string_view
    var states : SemState     // semantic states
    var actions : SemAction   // available actions
    var level : u8            // headings
    var pos_in_set : u32
    var set_size : u32
}

@direct_init
public struct SemState {
    var focusable : bool; var focused : bool; var disabled : bool
    var checked : bool; var selected : bool; var expanded : bool
    var busy : bool; var invalid : bool; var readonly : bool
    var required : bool; var pressed : bool; var hidden : bool
}

@direct_init
public struct SemAction {
    var press : bool; var focus : bool; var set_value : bool
    var scroll_into_view : bool; var expand : bool; var collapse : bool
    var increment : bool; var decrement : bool
}
```

- Every interactive node must have a non-empty accessible name. Components
  derive it from visible text, `label`, or an explicit `a11y_label`; icon-only
  controls require `a11y_label` at the type level.
- The semantic tree is produced from the same retained nodes and layout boxes
  as paint. It is not a parallel tree.
- `ui audit` fails on: missing name/role, duplicate names in a group, contrast
  below 4.5:1 for text (3:1 for large text), targets below 24×24 logical px,
  focus order that skips interactive nodes, and modal focus escapes.
- Platform bridges: Linux AT-SPI, Windows UIA, macOS NSAccessibility, WASM DOM
  overlay. They consume incremental `SemanticsUpdate` values.

---

## 9. Errors, panic policy, and no-panic enforcement

```chemical
public variant UiError {
    Platform(message : string)
    Surface(message : string)
    Font(message : string)
    Image(message : string)
    Layout(message : string)
    InvalidKey(message : string)
    StaleNode()
    OutOfMemory()
    Protocol(message : string)
    Io(message : string)
    Timeout()
    NotImplemented(feature : string)
    Internal(message : string)     // invariant breach detected; recoverable path
}
```

**Expected failures are values.** Public APIs return `Result<T, UiError>` (or
`Option<T>`) where failure is possible. The library adds no `unwrap`/`expect`/
`panic` convenience methods; callers use `?`, pattern matching, `unwrap_or`, or
an explicit fallback. Allocation, image/font decode, layout, stale IDs, IO,
protocol garbage, and missing resources are all expected failures and must not
panic.

**Panic is a last resort, not an error channel.** Chemical has no unwinding and
no try/catch, so a panic is process-fatal. The library may abort only when
continuing would be memory-unsafe or corrupt state beyond recovery — and that
must be a reviewed, private `ui::internal::fatal` call with a clear diagnostic,
a test proving the state is otherwise unrepresentable, and a parent-approved
exception. Panic is never used for:

- missing/corrupt font, image, or file;
- invalid layout values (NaN/Inf/negative) or overflow;
- stale `NodeId`/`Key`, missing node, or protocol error;
- allocation failure (degrade/return `OutOfMemory`);
- a widget misuse that can be reported as `Internal`/`InvalidKey`;
- user/app bugs where an error card or fallback is possible.

**Error boundaries are typed-error boundaries.** The runtime replaces a failed
subtree with a standard error card and keeps the window alive for `UiError`.
A panic is a bug and is not caught; it is reported by the crash/debug path and
is a release-blocking defect.

Rules:

- No `panic`, `unreachable`, unchecked indexing, or `unsafe` in shipped safe
  paths. Arithmetic that can overflow uses saturating/checked helpers.
- Debug builds may use assertions for programmer contracts; release builds must
  return `UiError.Internal` and render a safe fallback where possible.
- `ui_lint` (P5.9) bans `unwrap`/`expect`/`panic`/`unreachable` in
  `lang/libs/ui*` and in app UI code. Tests may use assertions; examples must
  demonstrate proper error handling, not `unwrap`.
- Full compiler-enforced `[no_panic]` is gated on std effect annotations
  (tracked in the proposal's blockers). Until then, a no-panic audit, fuzz
  corpus, and panic-review whitelist are release gates.
- The panic whitelist lives at `lang/docs/ui/PANIC_WHITELIST.md` and is part of
  the release audit: every allowed abort site names the invariant, why
  continuing is unsafe, and the test that keeps it unreachable.

---

## 10. Debug protocol `ui.debug.v1`

Transport: newline-delimited JSON-RPC 2.0 over TCP on `127.0.0.1`. Discovery:
the app writes `.chemical/ui/<pid>.json` (mode 0600) containing
`{ "pid":…, "port":…, "token":"…", "started_ms":…, "title":"…" }`. Every
request must include the token (except `hello`, which carries it). No `eval`, no
arbitrary code execution, no filesystem writes outside declared capture paths.

WASM has no listening sockets. `ui_web` therefore exposes the same JSON-RPC
methods as `globalThis.__chemicalUi.call(method, params)` (a Promise) so
Playwright/MCP can drive it with `page.evaluate`, and optionally as a WebSocket
client to a `chemical ui relay` process on the host. The CLI normalizes all
three transports (TCP, DOM bridge, WebSocket) behind one client.

Request shape:

```json
{"jsonrpc":"2.0","id":7,"method":"tree.snapshot","params":{"depth":0,"include_layout":true}}
```

Core methods:

| Method | Params | Returns |
|--------|--------|---------|
| `hello` | `{token}` | `{api:"1", pid, platform, backend, theme}` |
| `app.info` | `{}` | size, scale, theme, renderer, GPU backend/device, font stack + hashes, build, uptime, frame count |
| `gpu.info` | `{}` | `{available, backend, device, driver, dedicated, max_texture, supports_compute, software}` |
| `gpu.set` | `{renderer?, backend?, prefer?}` | `{}` (forces a switch; CPU fallback on failure) |
| `fonts.list` | `{}` | resolved fonts, fallback chain, paths/hashes, missing glyph counts |
| `fonts.add` | `{path}` or `{url, sha256}` | `{handle, family, sha256}` |
| `fonts.use` | `{handle}` or `{family}` | `{}` |
| `tree.snapshot` | `{depth?, root?, include_layout?, include_offscreen?}` | semantic tree with `node_id`, `key`, `test_id`, role/name/value/states/actions, bounds, source `{file,line,col}`, component, children |
| `tree.find` | `{role?, name?, key?, test_id?, text?}` | `[node_id]` |
| `node.inspect` | `{node_id}` | props, resolved tokens, style, layout box + constraints, focus, hit path, component stack, animation state, semantics, source |
| `node.highlight` | `{node_id, color?, ttl_ms?}` | draw overlay in next frames |
| `input.pointer` | `{type, x, y, button?, modifiers?, pointer_id?}` | `{}` |
| `input.key` | `{action:"down"\|"up", code, modifiers?}` | `{}` |
| `input.text` | `{text}` | `{}` |
| `input.ime` | `{type:"preedit"\|"commit"\|"cancel", text?, cursor?}` | `{}` |
| `input.scroll` | `{x, y, dx, dy}` | `{}` |
| `action.invoke` | `{node_id, action}` (`focus`, `press`, `scroll_into_view`, `expand`, `collapse`, `set_value`) | `{}` |
| `clock.get` | `{}` | `{now_ms, paused, rate}` |
| `clock.set` | `{now_ms}` | `{}` |
| `clock.pause` / `clock.resume` | `{}` | `{}` |
| `clock.advance` | `{ms}` | `{}` (renders intermediate frames) |
| `clock.rate` | `{rate}` | `{}` |
| `frame.step` | `{frames?, dt_ms?}` | `{frame, stats}` |
| `animation.list` | `{}` | running/queued animations with node, property, start, duration, easing, progress, value, target |
| `animation.seek` | `{node_id, prop, ms}` | `{}` |
| `animation.rate` | `{rate}` | `{}` |
| `screenshot` | `{node_id?, region?, scale?, background?, at_time?, include_overlay?}` | `{png_base64, width, height, frame, hash}` |
| `overlay.set` | `{boxes?, layout?, text?, focus?, damage?, fps?, hit?}` | `{}` |
| `capture.start` | `{path, fps?, max_frames?, include_input?}` | `{capture_id}` |
| `capture.stop` | `{capture_id}` | `{frames, path}` |
| `state.snapshot` | `{}` | implementation-defined JSON (tree state + opt-in app state) |
| `state.restore` | `{state}` | `{}` |
| `audit.run` | `{level?}` | structured findings: a11y, contrast, tap targets, overflow, tokens, missing keys, huge non-virtualised lists |
| `perf.stats` | `{reset?}` | `{renderer, build_us, layout_us, paint_us, gpu_us, present_us, nodes, commands, glyphs, tiles_painted, allocs, gpu_upload_bytes, gpu_draw_calls, frame, p50/p99 ring}` |
| `log.tail` | `{n?, level?}` | `[{ts, level, category, message, source}]` |
| `events.subscribe` | `{kinds?}` | notifications with deltas |
| `catalog.get` | `{component?}` | component descriptors (props schema, slots, variants, examples, a11y contract) |
| `tokens.get` | `{}` | resolved token table + source |
| `shutdown` | `{}` | `{}` |

Notifications: `tree.changed`, `frame`, `error`, `log`, `animation.finished`.

The same methods are exposed as MCP tools by `chemical ui mcp` (one high-level
tool per workflow, not one per method):

- `ui_snapshot` → tree + focus + layout summary
- `ui_find` / `ui_inspect`
- `ui_act` (click/type/key/scroll/focus by key or test id)
- `ui_clock` (pause/seek/step/rate)
- `ui_screenshot` (returns path + hash)
- `ui_capture` (frames + input)
- `ui_audit`
- `ui_perf` (CPU + GPU frame stats)
- `ui_gpu` (info/force/fallback)
- `ui_fonts` (list/pin/add)
- `ui_catalog`

---

## 11. Test harness (`ui_test`)

```chemical
public struct UiHarness<'a, A> {
    public func new(app : &'a mut A, options : Options) : Result<UiHarness<'a, A>, UiError> where A : Application
    public func size(&self) : Size
    public func set_size(&mut self, w : float, h : float)
    public func set_theme(&mut self, mode : ThemeMode)
    public func set_reduced_motion(&mut self, on : bool)
    public func scale(&self) : float

    public func frame(&mut self)
    public func advance(&mut self, ms : double)
    public func settle(&mut self)                 // bounded: error if not stable in N frames

    public func find_role(&self, role : Role) : Option<NodeId>
    public func find_name(&self, name : string_view) : Option<NodeId>
    public func find_test_id(&self, id : string_view) : Option<NodeId>
    public func by_key(&self, key : Key) : Option<NodeId>
    public func semantics(&self, id : NodeId) : Semantics
    public func bounds(&self, id : NodeId) : Rect
    public func text_of(&self, id : NodeId) : string_view

    public func click(&mut self, id : NodeId)
    public func double_click(&mut self, id : NodeId)
    public func pointer(&mut self, kind : EventKind, x : float, y : float)
    public func scroll(&mut self, id : NodeId, dx : float, dy : float)
    public func key(&mut self, code : KeyCode, mods : Modifiers)
    public func type_text(&mut self, text : string_view)

    public func screenshot(&self, path : string_view) : Result<std::Unit, UiError>
    public func assert_golden(&self, path : string_view, tolerance : float) : Result<std::Unit, UiError>
    public func geometry_snapshot(&self) : string          // JSON
    public func semantics_snapshot(&self) : string         // JSON
    public func audit(&self) : AuditReport
}
```

Golden policy: headless + CPU rasterizer + pinned test font + fixed DPI/time/theme
are bit-exact. Geometry and semantics snapshots are the primary behavioral
oracles; pixels are the appearance oracle. Golden updates require an explicit
`--update-goldens` and parent review of the diff.

---

## 12. Component catalog conventions

Every `ui_kit` component exports:

1. a `Props` struct with `@make`, typed token-only fields, and defaults;
2. a constructor `func name(b, key, props) : NodeId`;
3. a `ComponentMeta` descriptor (name, description, props JSON schema, slots,
   variants, required a11y name, keyboard contract, examples, "when to use /
   when not");
4. at least two examples (one minimal, one realistic);
5. semantic, geometry, and golden tests for every state.

Prop naming: `variant`, `size`, `disabled`, `loading`, `selected`, `checked`,
`error`, `icon`, `trailing_icon`, `a11y_label`, `test_id`, `tooltip`. No
component-specific synonyms for these. A `Props` struct never accepts a raw
`Color`/pixel value for chrome.

---

## 13. Versioning and freeze

- Package version starts at `1.0.0`; `ui::API_VERSION = 1`.
- Frozen before v1: package names, `Application`, `ViewBuilder` method set,
  `Key`/`NodeId` semantics, `Style`/token names, layout semantics in §4.2,
  `UiEvent`/`Payload`, `UiError`, component naming/prop conventions, and the
  debug protocol `ui.debug.v1`.
- Additive changes are allowed in a minor release (new methods, new components,
  new protocol methods, new semantics roles).
- Breaking changes require a new major package (`ui2`) or an explicit
  parent-approved amendment here plus a migration note; they are not allowed to
  land silently during the plan.
- Internal names are prefixed `internal_` or live in `ui::internal`; they are
  not part of the contract.

---

## 14. Amendments

| Date | Amendment | Approved by |
|------|-----------|-------------|
| — | Initial contract | — |
