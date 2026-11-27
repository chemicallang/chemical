# Chemical UI — frozen v1 contracts

**Status:** contract draft for approval · **Date:** 2026-11-27
**Rule:** once packet P0.2 is committed, this file plus the normative companions
is the source of truth. A public signature, semantic rule, or protocol method may
change only by an explicit amendment approved by the parent before code changes.
Silent drift is a review failure.

**Normative companions (read before coding):**
[`CONTRACTS.md`](CONTRACTS.md) (freeze amendments: frame/window, keys,
virtualization, events/lifetimes, view construction, layout pipeline, raster,
errors/panic/OOM, limits, threads, images/fonts, host/protocol/catalog hooks),
[`TEXT.md`](TEXT.md), [`HOST.md`](HOST.md), [`PROTOCOL.md`](PROTOCOL.md),
[`CATALOG.md`](CATALOG.md), [`DESIGN.md`](DESIGN.md). If this file conflicts with
`CONTRACTS.md`, `CONTRACTS.md` wins until P0.2 folds it in.

Everything below is the target API. Implementation packets may leave a method
unimplemented behind a documented `NotImplemented` error, but may not invent a
different public shape. P0.2 must compile every declaration, not "where
possible". Function/constructor **bodies are elided** for readability; structs
that expose fields are `@direct_init`, and structs with a `@make` constructor
show the constructor signature. Internal fields may be added only if they do not
change construction or equality semantics.

---

## 0. Packages and dependency direction

| Package | Role | May import |
|---------|------|------------|
| `ui` | Pure core: geometry, color, arena, view builder, tree, reconciler, layout, paint list, rasterizer, text, input, semantics, app model, headless run. No OS calls. | `std`, `cstd`, `image`, `font` |
| `ui_host` | Target facade: picks `ui_native` or `ui_web` and starts the loop. | `ui`, `ui_native`, `ui_web`, `ui_debug` |
| `ui_native` | Desktop hosts: Linux (GTK3 host), Windows (Win32), macOS (Cocoa). Window, input, IME, clipboard, present, a11y bridges; optional `ui_gpu_*`. | `ui`, `window`, `fs`, `osrand` |
| `ui_web` | `wasm32-unknown-unknown` host: canvas presenter, DOM event source, IME, clipboard, semantics DOM overlay; optional `ui_gpu_webgpu`. | `ui`, `browser` |
| `ui_kit` | Design system + components + page patterns. | `ui`, `ui_icons`, `ui_images` |
| `ui_images` | Optional JPEG (v1) / WebP/GIF/SVG (v1.1) decode + image cache; PNG core. | `ui`, `image` |
| `ui_charts` | v1 charts (line/bar/area/scatter/pie/heatmap) on Canvas + a11y table fallback. | `ui`, `ui_kit` |
| `ui_icons` | Curated vector icon path data (ISC/MIT). | `std` only |
| `ui_fonts` | Optional font cache/download/pin (`fetch`, SHA-256, atomic writes, OFL preset). Not imported by `ui`. | `std`, `fs`, `http`/`net` (native), `browser` (wasm) |
| `ui_gpu` | Optional GPU backend interface/resource cache; CPU is always available. | `ui` |
| `ui_gpu_vulkan` | Vulkan backend (Linux/Windows). | `ui`, `ui_gpu` |
| `ui_gpu_metal` | Metal backend (macOS). | `ui`, `ui_gpu` |
| `ui_gpu_webgpu` | WebGPU/WebGL2 backend (WASM). | `ui`, `ui_gpu`, `browser` |
| `ui_debug` | JSON-RPC debug server, discovery file, audit, capture/replay, inspector overlay. | `ui`, `json`, `net`, `fs`, `osrand` |
| `ui_test` | Headless harness: semantic queries, input, clock, goldens, geometry snapshots. | `ui`, `image`, `test` |
| `ui_cli` | `chemical ui …` client (self-built Chemical app). | `json`, `net`, `image`, `process`, `fs` |
| `ui_lint` | Optional CBI compile-time rules (keyed lists, tokens, a11y). | `compiler` |

**Dependency direction is one-way.** `ui` never imports a host, a widget, or the
debug server. `ui_kit` never imports a host. Hosts never contain widget logic.


**Namespace ownership (normative).** All declarations in §1–§9 and §11 belong to
`ui` unless a fence explicitly opens another namespace (`ui_host`, `ui_images`,
`ui_icons`, `ui_fonts`, `ui_test`). A fence may omit the `public namespace ui {`
wrapper for readability; the package namespace is implicit. `main`, the app
template, and the `button(...)` shape are **sample-only** and are not frozen
declarations. `ui_kit` props/components are catalog-owned (P4); `TableSource`,
`ItemSource`, and the data-view props in `CATALOG.md` are the exception because
`ViewBuilder`/`TableSource` are part of the frozen core.

App template (**sample-only, not a frozen declaration**):

```chemical sample
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

@direct_init
public struct FrameInfo {
    var window : WindowId
    var viewport : Viewport
    var scale : float
    var text_scale : float
    var theme : ThemeMode
    var reduced_motion : bool
    var locale : string_view
    var direction : Direction
    var frame : u64
    var now_ms : double
    var focused : bool
    var occluded : bool
}

@direct_init
public struct Viewport {
    var size : Size
    var safe_area : Insets
    var breakpoint : Breakpoint
}

public interface Application {
    // Called once per window before the first view; may spawn work via ctx.
    func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx)
        : Result<std::Unit, UiError>

    // Pure read of app state. Builds/reuses the view for this frame.
    func view(&self, frame : &FrameInfo, b : &mut ViewBuilder)
        : Result<NodeId, UiError>

    // The only place app state may mutate in response to input.
    func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx)
        : Result<std::Unit, UiError>

    // Custom painting for Canvas nodes; return Ok(()) if unused.
    func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter)
        : Result<std::Unit, UiError>

    // Return Allow to close; Veto keeps the window open. Required.
    func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction
}

public enum CloseAction { Allow, Veto }

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
    var limits : Limits                 // Options.make() sets Limits.make()
    var theme_override : Option<Theme> = Option.None<Theme>()
    var locale : string_view = "en-US"
    var max_fps : u8 = 0            // 0 = display refresh
    var reveal_secrets : bool = false

    // Rendering. CPU is the reference; GPU is an optional accelerator.
    var renderer : Renderer = Renderer.Auto
    var gpu_backend : GpuBackend = GpuBackend.Auto
    var gpu_prefer : GpuPreference = GpuPreference.Auto
    var allow_software_gpu : bool = false
    var gpu_frames_in_flight : u8 = 2

    // Typography. No font is embedded by default. An empty stack means the
    // platform UI stack; see ui_fonts for the optional download/pin package.
    var font_stack : FontStack       // Options.make() sets FontStack.system()
    var font_file : string_view = ""       // optional app-pinned font
    var font_cache : string_view = ""      // default: platform cache dir
    var font_size_scale : float = 1.0

    @make func make() : Options
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

`ui_host` selects `ui_native` or `ui_web` at build time; both implement the
host entry and drive the core loop. A headless build uses
`ui::run_headless` directly.

### 1.2 Supporting types

```chemical
@direct_init
public struct Point {
    var x : float
    var y : float
    public func zero() : Point
    public func new(x : float, y : float) : Point
    public func add(&self, other : Point) : Point
    public func sub(&self, other : Point) : Point
    public func scale(&self, s : float) : Point
    public func length(&self) : float
    public func distance(&self, other : Point) : float
    public func lerp(&self, other : Point, t : float) : Point
    public func is_finite(&self) : bool
}
@direct_init
public struct Rect {
    var x : float
    var y : float
    var w : float
    var h : float
    public func zero() : Rect
    public func new(x : float, y : float, w : float, h : float) : Rect
    public func from_ltrb(left : float, top : float, right : float, bottom : float) : Rect
    public func x2(&self) : float
    public func y2(&self) : float
    public func contains(&self, p : Point) : bool
    public func intersects(&self, other : Rect) : bool
    public func intersection(&self, other : Rect) : Option<Rect>
    public func union(&self, other : Rect) : Rect
    public func inset(&self, i : Insets) : Rect
    public func offset(&self, dx : float, dy : float) : Rect
    public func area(&self) : float
    public func is_finite(&self) : bool
}
@direct_init
public struct Transform2D {
    // Canvas2D order [a, b, c, d, tx, ty]:
    // x' = a*x + c*y + tx, y' = b*x + d*y + ty.
    var m : [6]float
    public func identity() : Transform2D
    public func translate(tx : float, ty : float) : Transform2D
    public func scale(sx : float, sy : float) : Transform2D
    public func rotate(radians : float) : Transform2D
    // self ∘ other: apply other first, then self.
    public func multiply(&self, other : Transform2D) : Transform2D
    public func transform_point(&self, p : Point) : Point
    public func is_finite(&self) : bool
}
@direct_init public struct Shadow { var x : float; var y : float; var blur : float; var spread : float; var color : Color }
@direct_init public struct TextSpan {
    var text : string_view; var role : TextRole; var color : ColorToken
    var weight : FontWeight; var italic : bool; var family : FontFamily
    var features : TextFeatures; var link : Option<string_view>; var decoration : u8
}
@direct_init public struct GridTracks { var columns : &[GridTrack]; var rows : &[GridTrack]; var gap : Space }
public variant GridTrack { Px(value : float) Fr(value : float) Auto() }
@direct_init public struct Duration { var ms : float }
public enum Easing { Linear, EaseIn, EaseOut, EaseInOut, Spring }
public enum MotionToken { Fast, Base, Slow, Spring }
public enum AnimProperty { Opacity, TranslateX, TranslateY, Scale, Rotate, Background, Width, Height }
@direct_init public struct AnimationSpec {
    var duration : Duration; var easing : Easing; var delay : Duration
    var repeat : u32; var alternate : bool
}
public enum Cursor { Arrow, IBeam, Hand, Wait, Crosshair, Move, ResizeNS, ResizeEW, ResizeNESW, ResizeNWSE, NotAllowed }
public enum ImageFit { Contain, Cover, Fill, None }
public enum FontFamily { Ui, Mono, System }
public struct FontSpec {
    var family : FontFamily; var family_name : string_view
    var file : string_view; var fallback : FontFamily
}
@direct_init public struct FontFallback { var script : u32; var families : &[string_view] }
@direct_init public struct FontStack {
    var ui : &[string_view]; var mono : &[string_view]
    var fallback : &[FontFallback]
    @make func system() : FontStack
}
public struct IconId { /* private registry handle */ }
public struct FontHandle { /* private generational */ }
public struct ImageHandle { /* private generational */ }
@direct_init public struct FontCacheEntry { var family : string_view; var path : string_view; var sha256 : string_view; var bytes : u64; var license : string_view; var source_url : string_view; var version : string_view }
public variant FontError {
    NotFound(family : string_view)
    InvalidFormat()
    LicenseRequired()
    DownloadFailed()
    CacheFailed()
    OutOfMemory()
}
public variant ImageSource { Path(path : string_view) Bytes(bytes : &[u8]) Handle(h : ImageHandle) Placeholder() }

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
public struct WindowId { /* private index+generation; host-assigned */ }
public enum ThemeMode { Light, Dark, HighContrast, System }
public enum Density { Comfortable, Compact }
public enum KeyCode {
    Unknown, Tab, Enter, Escape, Space, Backspace, Delete, Insert,
    Left, Right, Up, Down, Home, End, PageUp, PageDown,
    A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
    Digit0,Digit1,Digit2,Digit3,Digit4,Digit5,Digit6,Digit7,Digit8,Digit9,
    Minus,Equal,LeftBracket,RightBracket,Backslash,Semicolon,Quote,Comma,Period,Slash,Backquote,
    F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,F15,F16,F17,F18,F19,F20,F21,F22,F23,F24,
    ShiftLeft,ShiftRight,ControlLeft,ControlRight,AltLeft,AltRight,MetaLeft,MetaRight,CapsLock,
    Numpad0,Numpad1,Numpad2,Numpad3,Numpad4,Numpad5,Numpad6,Numpad7,Numpad8,Numpad9,
    NumpadAdd,NumpadSubtract,NumpadMultiply,NumpadDivide,NumpadDecimal,NumpadEnter,
    PrintScreen,ScrollLock,Pause,ContextMenu,MediaPlayPause,MediaStop,MediaNext,MediaPrev,
    VolumeUp,VolumeDown,VolumeMute
}

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
    var event_to_update_us : double
    var update_to_submit_us : double
    var submit_to_present_us : double
    var nodes : u32
    var visited : u32
    var relaid_out : u32
    var layout_nodes : u32
    var virtual_rebuilds : u32
    var virtual_extent_calls : u32
    var virtual_index_builds : u32
    var commands : u32
    var glyphs : u32
    var text_measured : u32
    var text_cache_hits : u32
    var tiles_painted : u32
    var full_repaints : u32
    var paint_bytes : u64
    var scroll_blit_bytes : u64
    var scroll_blit_us : double
    var allocs : u32
    var allocs_cold : u32
    var alloc_bytes_high_water : u64
    var settle_frames : u32
    var gpu_upload_bytes : u64
    var gpu_draw_calls : u32
}
```

The full key set and the `Rect`/`Point` helpers are defined in `ui::geom` and
`ui::input`; P1.1 adds the value methods above without renaming the types.
`Rect` is half-open: `contains` includes `x/y` and excludes `x+w/y+h`; empty
rectangles (`w <= 0 || h <= 0`) never intersect or contain. `Rect.new` clamps
negative/non-finite extents to zero; `from_ltrb` normalizes the corners.
`Transform2D` uses Canvas2D order `[a,b,c,d,tx,ty]`; `multiply` returns
`self ∘ other` (other applies first). Non-finite inputs/results clamp to identity/
zero where the value may be passed to layout or paint.

### 1.3 Rendering backend

The public API never names Vulkan, Metal, or D3D12 types. It exposes capability
and selection only:

```chemical
public namespace ui {
    public func gpu_info() : GpuInfo
    public func canvas_id(name : string_view) : Result<CanvasId, UiError>
    public func release_canvas(id : CanvasId)
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
- `GpuInfo` is for diagnostics/telemetry, not for branching app UI; its device/driver strings are static/host-owned and safe to retain.
- A GPU backend consumes the **same display list** as the CPU renderer: the app
  never writes shaders or GPU code.

### 1.4 Font services

No font is embedded in the `ui` core or in every app binary. The default is a
platform UI font stack; an app can pin a file, embed a small subset, or fetch an
OFL font into a cache. Golden tests pin a test font.

```chemical
public variant ImageState { Missing() Loading() Ready(value : ImageHandle) Failed(err : UiError) }
public namespace ui_images {
    public func image(path : string_view) : ImageState
    public func image_bytes(bytes : &[u8], hash : u64) : ImageState
    public func state(h : ImageHandle) : ImageState
    public func release(h : ImageHandle)
}

public namespace ui_icons {
    public func find(name : string_view) : Option<IconId>
    public func list() : &[string_view]
}

public namespace ui_fonts {
    // Platform UI stack: system-ui, Segoe UI, SF Pro Text, Inter, Noto Sans,
    // DejaVu Sans, sans-serif (the actual list is platform-specific).
    public func system_stack() : FontStack

    public func add_file(path : string_view) : Result<FontHandle, FontError>
    public func add_bytes(bytes : &[u8], name : string_view) : Result<FontHandle, FontError>
    public func set_default(handle : FontHandle)
    public func default_handle() : FontHandle

    // Optional network/cache flow. Verifies SHA-256 and atomic-writes.
    public func fetch(url : string_view, sha256 : string_view) : Result<FontHandle, FontError> // blocking; never in view/update
    public func fetch_async(url : string_view, sha256 : string_view) : FontRequest
    public func cache_dir() : string_view
    public func list_cached() : &[FontCacheEntry]

    // Recommended OFL preset: a Latin subset (~42 KB TTF, chosen over the
    // 37 KB ASCII-first variant for complete Latin-1 coverage). This is opt-in;
    // it downloads to the cache unless the app embedded it.
    public func use_inter_latin() : Result<FontHandle, FontError>
}
public struct FontRequest { /* private; completion via FontReady */ }
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

### 1.5 Shared value types

```chemical
public enum Breakpoint { Sm, Md, Lg, Xl }
@direct_init public struct MenuItem {
    var id : CommandId; var label : string_view; var shortcut : Option<Shortcut>
    var enabled : bool; var checked : bool; var separator : bool
}
@direct_init public struct Menu { var title : string_view; var items : &[MenuItem] }
@direct_init public struct FileDialogOptions {
    var title : string_view; var filters : &[string_view]
    var multiple : bool; var directory : bool; var save : bool
}
@direct_init public struct Notification {
    var title : string_view; var body : string_view; var icon : Option<ImageHandle>
}
@direct_init public struct WindowSpec {
    var title : string_view; var size : Size; var min_size : Size; var max_size : Size
    var position : Option<Point>; var resizable : bool; var decorated : bool
    var always_on_top : bool; var transparent : bool; var start_visible : bool
}
@direct_init public struct CanvasPaint {
    var node : NodeId; var key : Key; var id : CanvasId
    var bounds : Rect; var scale : float
}
public struct TimerId { /* private host-assigned */ }
public struct CommandId { private var value : u32 }
@direct_init public struct Shortcut { var code : KeyCode; var mods : Modifiers }
@direct_init public struct SourceLocation { var file : string_view; var line : u32; var col : u32 }
public enum RetryHint { Never, Immediate, Backoff }
@direct_init public struct ErrorDetail {
    var code : u32; var message : &'static str; var context : &'static str
}

@direct_init public struct UserEvent {
    var tag : u64; var a : i64; var b : i64; var text : string_view
}
public struct EventSender { /* Send + Sync + Clone; holds a refcounted queue */
    // Copies text fallibly into the queue; after shutdown returns Cancelled.
    public func post(&self, ev : UserEvent) : Result<std::Unit, UiError>
    public func wake(&self)
}

@direct_init public struct Limits {
    var max_view_nodes : u32; var max_retained_nodes : u32
    var max_text_bytes : u64; var max_image_pixels : u64; var max_image_bytes : u64
    var max_font_bytes : u64; var max_atlas_bytes : u64
    var max_layer_bytes : u64; var max_gpu_bytes : u64
    var max_display_commands : u32; var max_protocol_bytes : u32
    var max_capture_frames : u32; var max_screenshot_pixels : u64
    @make func make() : Limits
}

@direct_init public struct Range { var start : u32; var end : u32 }
public variant ImeTestEvent {
    Preedit(text : string_view, caret : u32, anchor : u32, replace : Range)
    Commit(text : string_view, replace : Range)
    Cancel()
}
@direct_init public struct ImeComposition {
    var preedit : string_view; var caret : u32; var anchor : u32
    var replace : Range; var candidate : Option<Rect>
}
@direct_init public struct TextEditState {
    var text : string_view; var anchor : u32; var caret : u32
    var composition : Option<ImeComposition>; var secure : bool
}

public variant PathCommand {
    MoveTo(p : Point) LineTo(p : Point) QuadTo(c : Point, p : Point)
    CubicTo(c1 : Point, c2 : Point, p : Point) Close()
}
@direct_init public struct Path { var commands : vector<PathCommand>; var fill_rule : u8 }
@direct_init public struct GradientStop { var offset : float; var color : Color }
public variant Gradient { Linear(start : Point, end : Point, stops : vector<GradientStop>) Radial(center : Point, radius : float, stops : vector<GradientStop>) }
public struct ScopeGuard { /* private; drop pops scope */ }
public struct ClipGuard { /* private; drop closes */ }
public struct LayerGuard { /* private; drop closes */ }
public struct TransformGuard { /* private; drop closes */ }

@direct_init public struct Screenshot {
    var path : string_view; var width : u32; var height : u32; var hash : u64
}
@direct_init public struct GoldenOptions {
    var max_diff_pixels : u32
    var yiq : float
    var masks : &[Rect]
    @make func make() : GoldenOptions
}
@direct_init public struct GoldenResult {
    var passed : bool; var diff_path : string_view; var new_path : string_view
}
@direct_init public struct Query {
    var role : Option<Role>; var name : Option<string_view>
    var key : Option<Key>; var test_id : Option<string_view>
    var text : Option<string_view>; var component : Option<string_view>
    var state : Option<WidgetState>; var within : Option<NodeId>
    var all : bool; var limit : u32
}
// A component root + named slots. Private; created by `ViewBuilder.component`.
public struct Component { /* private root + named slots */ }

// Virtualized list source (CATALOG.md §3). `virtual_list` calls only visible
// + overscan items; extents must be finite and [0, max].
public interface ItemSource {
    func count(&self) : size_t
    func key(&self, index : size_t) : Key
    func item(&self, b : &mut ViewBuilder, index : size_t) : NodeId
    func extent(&self, index : size_t) : float
}

public enum SelectionMode { None, Single, Multiple }
@direct_init public struct TableColumn {
    var id : string_view; var title : string_view; var width : Dim
    var align : TextAlign; var sortable : bool
}
public variant SortSpec { Ascending(column : string_view) Descending(column : string_view) }
public interface TableSource {
    func count(&self) : size_t
    func row_key(&self, index : size_t) : Key
    func row_extent(&self, index : size_t) : float
    func cell(&self, b : &mut ViewBuilder, row : size_t, column : string_view) : NodeId
    func sort(&mut self, spec : SortSpec)
    func filter(&mut self, query : string_view)
}
public struct ComponentSpec { /* generated from ComponentMeta; see CATALOG.md */ }
public struct ComponentMeta { /* catalog-owned; see CATALOG.md */ }
public struct PageSpec { /* generated page composition spec */ }
public struct AuditReport { /* findings: code, severity, node_id, source, fix */ }
```

`Screenshot`, `GoldenResult`, `AuditReport`, and `ComponentSpec` are expanded in
`CATALOG.md`/`PROTOCOL.md`. `UiCtx`/`UiHarness` use these exact names.

### 1.6 Host presenter contract

These types are frozen from `HOST.md` §2. They are public so a host backend can
implement `Presenter`; the core renderer only consumes the surface/damage.

```chemical
public struct SurfaceDesc {
    var width : u32
    var height : u32
    var stride : u32
    var format : SurfaceFormat
}

public enum SurfaceFormat { Bgra8Premul, Rgba8Premul, Rgba8SrgbPremul }

public struct Surface<'a> {
    var desc : SurfaceDesc
    var bytes : usize
    var data : *mut u8
}

public interface Presenter {
    func <'a> acquire(&'a mut self, desc : SurfaceDesc) : Result<Surface<'a>, UiError>
    func present(&mut self, damage : &[Rect]) : Result<std::Unit, UiError>
    func resize(&mut self, width : u32, height : u32, scale : float) : Result<std::Unit, UiError>
    func request_frame(&mut self)
    func set_cursor(&mut self, c : Cursor)
    func set_ime_rect(&mut self, r : Rect)
}

@direct_init
public struct HostCaps {
    var partial_present : bool; var vsync : bool; var multi_window : bool
    var ime : bool; var clipboard_images : bool; var drop_files : bool
    var native_menus : bool; var notifications : bool; var a11y : bool
}
```

`HostCaps` is exposed through `app.info`; `Presenter` is implemented by
`ui_host` (not by app code). `Surface` is lifetime-bound and cannot be retained.

---

## 2. Identity: `NodeId`, `Key`, `CanvasId`

```chemical
public struct NodeId {          // private construction; frame-local
    private var index : u32
    private var generation : u32
}

public struct Key {                 // private fields; constructors only
    private var hash : u64
    private var tag_value : u64
    private var id_value : u64

    public func equals(&self, label : string_view) : bool
    public func equals_key(&self, other : Key) : bool
    public func id(&self) : u64
    public func tag(&self) : u64
}

public func key(label : string_view) : Key
public func key_of(label : string_view, id : u64) : Key
public func key_pair(label : string_view, a : u64, b : u64) : Key

public struct CanvasId { /* private registry handle */ }
```

Semantics (full version in `CONTRACTS.md` §2):

- `NodeId` is a **frame-local view handle** with private construction. `(0,0)`
  invalid; generation wrap retires a slot (`ResourceExhausted`), never aliases.
- `Key` is the **cross-frame identity** for all runtime state. It holds only
  `(hash, tag, id)`; constructors are the only creation path. `key(label)` hashes
  the bytes (best-effort debug label table, no correctness dependency);
  `key_of` sets `id`; `key_pair` sets `id=a, tag=b` with an order-sensitive
  hash. `id()` returns `id`, `tag()` returns `tag`. `equals(label)` hashes and
  compares; `equals_key` compares the full tuple. No `starts_with` (use
  `id`/`tag`).
- Key scope is immediate siblings; components/overlays/fragments have their own
  scope. Duplicate keys are a diagnostic, never silent state theft.
- **All** hover/focus/pressed/scroll/text/selection/animation/custom state is
  keyed by `Key`, not `NodeId`.
- `CanvasId`, `ImageHandle`, `IconId`, `FontHandle` are private/registry-checked;
  stale handles paint placeholders and report once.

---

## 3. View construction

```chemical
public struct ViewBuilder {
    // Context (also available through the FrameInfo passed to view).
    public func frame(&self) : &FrameInfo
    public func viewport(&self) : Viewport
    public func scroll_offset(&self, key : Key) : Point

    // Containers (layout is set by Style; the method is a shorthand for display/direction).
    public func row(&mut self, key : Key, style : Style) : NodeId
    public func column(&mut self, key : Key, style : Style) : NodeId
    public func stack(&mut self, key : Key, style : Style) : NodeId
    public func wrap(&mut self, key : Key, style : Style) : NodeId
    public func grid(&mut self, key : Key, style : Style, tracks : GridTracks) : NodeId
    public func scroll(&mut self, key : Key, style : Style, axis : Axis, anchor : ScrollAnchor = ScrollAnchor.Start()) : NodeId
    public func canvas(&mut self, key : Key, style : Style, canvas : CanvasId) : NodeId
    public func fragment(&mut self, key : Key) : NodeId          // display: contents
    public func scope(&mut self, key : Key) : ScopeGuard
    public func scoped_key(&self, parent : Key, label : string_view) : Key
    public func overlay(&mut self, key : Key, layer : OverlayLayer,
                        anchor : Option<NodeId>) : NodeId
    public func component(&mut self, key : Key, name : string_view) : Component
    public func slot(&mut self, c : Component, name : string_view) : NodeId

    // Leaves.
    public func text(&mut self, key : Key, value : string_view, style : Style) : NodeId
    public func rich_text(&mut self, key : Key, spans : &[TextSpan], style : Style) : NodeId
    public func image(&mut self, key : Key, src : ImageSource, style : Style) : NodeId
    public func icon(&mut self, key : Key, icon : IconId, style : Style) : NodeId
    public func virtual_list<S : ItemSource>(&mut self, key : Key, style : Style,
                                             axis : Axis, source : &S) : NodeId

    // Tree structure.
    public func child(&mut self, parent : NodeId, child : NodeId)
    public func children(&mut self, parent : NodeId, kids : &[NodeId])
    public func insert_child(&mut self, parent : NodeId, index : u32, child : NodeId)

    // Per-node properties. style() is a field-wise overlay.
    public func style(&mut self, node : NodeId, style : Style)
    public func state_style(&mut self, node : NodeId, state : WidgetState, style : Style)
    public func command(&mut self, node : NodeId, cmd : CommandId)
    public func semantics(&mut self, node : NodeId, sem : Semantics)
    public func test_id(&mut self, node : NodeId, id : string_view)
    public func focusable(&mut self, node : NodeId, tab_index : i32)
    public func grid_cell(&mut self, node : NodeId, col : u16, row : u16,
                          col_span : u16, row_span : u16)
    public func animate(&mut self, node : NodeId, prop : AnimProperty, to : float,
                        duration : Duration, easing : Easing)
    public func animate_exit(&mut self, node : NodeId, prop : AnimProperty, to : float,
                             duration : Duration, easing : Easing)
    public func animate_spec(&mut self, node : NodeId, prop : AnimProperty, to : float,
                             spec : AnimationSpec)
    public func animate_exit_spec(&mut self, node : NodeId, prop : AnimProperty, to : float,
                                  spec : AnimationSpec)
    public func transform(&mut self, node : NodeId, t : Transform2D)
    public func error(&mut self, node : NodeId, err : UiError)
}
```

Rules:

- A node is created once; `child`/`children` append. Cycles are rejected with a
  diagnostic (never an infinite loop).
- The builder arena resets at the start of every `view`; `NodeId`s are
  frame-local. Public code caches `Key`s, never `NodeId`s.
- The builder **copies** every string/slice (text, labels, test ids, spans,
  component names, image bytes) into the frame arena. Retained nodes, semantics,
  a11y, and protocol snapshots own/intern their text.
- `Style` is a value type; no global style state, no cascade. `style()` overlays
  fields; `merge()` is explicit.
- `state_style` resolves hover/focus/pressed/disabled without a `view` rebuild.
- `virtual_list` calls `ItemSource` only for visible + overscan rows. Scrolling
  it triggers one `view`; ordinary scroll does not.
- `fragment` has no layout box; `overlay` escapes clips into the window overlay
  root with layer ordering; `component`/`slot` is the only content injection.

Component pattern (**sample-only, not a frozen declaration**; the `ui_kit`
component surface is catalog-owned and lands in P4):

```chemical sample
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
public enum Radius { None, XS, SM, MD, LG, XL, Full }
public enum Elevation { None, Low, Medium, High }
public enum TextRole { Display, H1, H2, H3, H4, Body, BodyStrong, Caption, Code, Label }
public enum FontWeight { Regular, Medium, Semibold, Bold }
public enum Overflow { Visible, Clip, Scroll }
public enum Align { Start, Center, End, Stretch, Baseline }
public enum Justify { Start, Center, End, SpaceBetween, SpaceAround, SpaceEvenly }
public enum Axis { Horizontal, Vertical, Both }

@direct_init
public struct Color {
    var r : u8
    var g : u8
    var b : u8
    var a : u8
    public func new(r : u8, g : u8, b : u8) : Color
    public func rgba(r : u8, g : u8, b : u8, a : u8) : Color
    // Byte order is 0xRRGGBBAA: red in the high byte.
    public func from_u32(v : u32) : Color
    public func to_u32(&self) : u32
    public func with_alpha(&self, alpha : u8) : Color
    public func premultiply(&self) : Color
    public func unpremultiply(&self) : Color
    public func lerp(&self, other : Color, t : float) : Color
    // Integer sRGB-encoded source-over compositing (CONTRACTS.md §6).
    public func over(&self, dst : Color) : Color
    public func relative_luminance(&self) : f64
    public func contrast_ratio(&self, other : Color) : f64
    public func is_opaque(&self) : bool
    public func to_premul_u32(&self) : u32
}

`Color.from_u32`/`to_u32` use `0xRRGGBBAA` (red high byte). Premultiply,
unpremultiply and `over` are integer sRGB-u8 operations; `over` is source-over
using `div255` and returns exact `dst` for a transparent source and exact `src`
for an opaque source. Luminance/contrast are WCAG sRGB relative-luminance values
in f64 and ignore alpha. A translucent premultiply round-trip is exact only within
the information lost at low alpha (`255/a + 1` channel tolerance).

public variant Dim {
    Auto()
    Px(value : float)
    Percent(value : float)
    Fill()          // grow to the parent's main-axis remaining space
}

@direct_init
public struct Size {
    var width : float
    var height : float
    public func zero() : Size
    public func new(width : float, height : float) : Size
    public func add(&self, other : Size) : Size
    public func sub(&self, other : Size) : Size
    public func max(&self, other : Size) : Size
    public func min(&self, other : Size) : Size
    public func area(&self) : float
    public func is_finite(&self) : bool
}

public enum Direction { Auto, Ltr, Rtl }
public enum TextAlign { Start, Center, End, Justify }
public enum WrapMode { NoWrap, Wrap, WrapReverse }
public enum TextOverflow { Clip, Ellipsis }
public variant Position { Relative() Absolute(top : Dim, right : Dim, bottom : Dim, left : Dim) }
public enum WidgetState { Hover, Focus, Pressed, Disabled, Selected, Checked, Invalid }
public enum OverlayLayer { Tooltip, Popover, Menu, Dialog, Toast }
public variant ScrollAnchor { Start() End() Center() Node(key : Key) }

@direct_init
public struct Insets {
    var left : float
    var top : float
    var right : float
    var bottom : float
    public func zero() : Insets
    public func all(v : float) : Insets
    public func symmetric(horizontal : float, vertical : float) : Insets
    public func horizontal(v : float) : Insets
    public func vertical(v : float) : Insets
    public func is_finite(&self) : bool
}

@direct_init
public struct Border { var width : float; var color : ColorToken }

public struct Style {
    public func new() : Style

    public func padding(&self, p : Insets) : Style
    public func padding_all(&self, s : Space) : Style
    public func gap(&self, s : Space) : Style
    public func width(&self, s : Dim) : Style
    public func height(&self, s : Dim) : Style
    public func min_width(&self, px : float) : Style
    public func max_width(&self, px : float) : Style
    public func grow(&self, f : float) : Style
    public func shrink(&self, f : float) : Style
    public func basis(&self, s : Dim) : Style
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
    public func aspect(&self, ratio : float) : Style
    public func position(&self, p : Position) : Style
    public func inset(&self, i : Insets) : Style
    public func grid_column(&self, col : u16, span : u16) : Style
    public func grid_row(&self, row : u16, span : u16) : Style
    public func direction(&self, d : Direction) : Style
    public func text_style(&self, t : TextStyle) : Style
    public func text_align(&self, a : TextAlign) : Style
    public func wrap(&self, w : WrapMode) : Style
    public func max_lines(&self, n : u16) : Style
    public func text_overflow(&self, o : TextOverflow) : Style
    public func tab_size(&self, spaces : u8) : Style
    public func merge(&self, overlay : Style) : Style
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
    public func density(&self) : Density
    public func text_scale(&self) : float
    public func motion(&self, m : MotionToken) : Duration
    public func set_color(&mut self, t : ColorToken, c : Color)
    public func set_space(&mut self, s : Space, px : float)
    public func set_radius(&mut self, r : Radius, px : float)
    public func set_text_style(&mut self, role : TextRole, t : TextStyle)
    public func set_density(&mut self, d : Density)
    public func disabled_opacity(&self) : float
    public func set_text_scale(&mut self, scale : float)
}
```

### 4.2 Layout semantics (normative)

- **Sizing.** `Auto` = intrinsic content size; `Px` = exact; `Percent` = fraction
  of the parent's content box, except inside a scroll where it resolves against
  the viewport main axis; `Fill` = take the remaining main-axis space
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
  viewport, and stores the offset in runtime state keyed by `Key` (never `NodeId`).
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
    Focus, Blur, Change, Submit, Select, SelectionChanged, TextChanged, Open, Close,
    Drop, DragBegin, DragOver, DragLeave, DragDrop,
    Resize, DpiChange, Timer, Frame, Command, Custom, ImageReady, ImageFailed, FontReady,
    FileChosen, OpenFile, SecondInstance
}

@direct_init
public struct Modifiers {
    var shift : bool; var ctrl : bool; var alt : bool; var meta : bool; var caps : bool
}

@direct_init public struct PointerPayload {
    var button : u8; var press_count : u8; var pointer_kind : u8
    var pressure : float; var tilt_x : float; var tilt_y : float
}
@direct_init public struct ScrollPayload {
    var dx : float; var dy : float; var phase : u8   // Begin/Update/End/Inertia
}
@direct_init public struct KeyPayload {
    var code : KeyCode; var repeat : bool; var text : Option<string_view>
}
@direct_init public struct TextPayload { var text : string_view }
@direct_init public struct ImePayload {
    var preedit : string_view; var commit : string_view
    var caret : u32; var anchor : u32
    var replace : Range
    var candidate : Option<Rect>
}
@direct_init public struct SizePayload { var width : float; var height : float; var scale : float }
@direct_init public struct ScalePayload { var scale : float }
@direct_init public struct FilesPayload { var paths : &[string_view]; var text : Option<string_view> }
@direct_init public struct DragPayload {
    var payload : UserEvent; var x : float; var y : float
    var target : Option<Key>; var accepted : bool
}
@direct_init public struct TimerPayload { var id : TimerId }
@direct_init public struct FramePayload { var now_ms : double; var dt_ms : double }
@direct_init public struct ValuePayload {
    var kind : u8; var text : string_view; var number : double
    var range : Range
    var selected : bool; var checked : bool
}
@direct_init public struct FocusPayload { var focused : bool }
@direct_init public struct IndexPayload { var index : u32; var selected : bool }
@direct_init public struct CommandPayload { var id : CommandId }
@direct_init public struct ImagePayload { var handle : ImageHandle }
@direct_init public struct FontPayload { var handle : FontHandle }
@direct_init public struct TextChangePayload { var range : Range; var inserted : string_view }
@direct_init public struct SelectionPayload { var range : Range; var selected : bool }
@direct_init public struct ErrorPayload { var err : UiError }
@direct_init public struct UserPayload {
    var tag : u64; var a : i64; var b : i64; var text : string_view
}

public variant Payload<'a> {
    None()
    Pointer(value : PointerPayload)
    Scroll(value : ScrollPayload)
    Key(value : KeyPayload)
    Text(value : TextPayload)
    Ime(value : ImePayload)
    Size(value : SizePayload)
    Scale(value : ScalePayload)
    Files(value : FilesPayload)
    Drag(value : DragPayload)
    Timer(value : TimerPayload)
    Frame(value : FramePayload)
    Value(value : ValuePayload)
    Selection(value : SelectionPayload)
    TextChange(value : TextChangePayload)
    Focus(value : FocusPayload)
    Index(value : IndexPayload)
    Command(value : CommandPayload)
    Image(value : ImagePayload)
    Font(value : FontPayload)
    Error(value : ErrorPayload)
    User(value : UserPayload)
}

@direct_init
public struct UiEvent<'a> {
    var window : WindowId
    var key : Key
    var kind : EventKind
    var payload : Payload<'a>
    var modifiers : Modifiers
    var local_x : float
    var local_y : float
    var timestamp_ms : double
    var frame : u64
}

public struct UiCtx {
    public func frame_info(&self) : &FrameInfo
    public func viewport(&self) : Viewport
    public func scale(&self) : float
    public func text_scale(&self) : float
    public func set_text_scale(&mut self, scale : float)

    public func request_view(&mut self)
    public func invalidate(&mut self, key : Key)
    public func request_paint(&mut self)
    public func request_quit(&mut self)
    public func request_frame(&mut self)
    public func request_animation_frame(&mut self)
    public func set_timeout(&mut self, ms : u32) : TimerId
    public func cancel_timeout(&mut self, id : TimerId)

    public func text(&self, key : Key) : &str
    public func take_text(&mut self, key : Key) : string
    public func text_context(&self) : &TextContext
    public func text_state(&self, key : Key) : Option<&TextEditState>
    public func set_text(&mut self, key : Key, text : string_view) : Result<std::Unit, UiError>
    public func set_selection(&mut self, key : Key, anchor : u32, caret : u32)
    public func select_all(&mut self, key : Key)
    public func ime_rect(&mut self, key : Key, rect : Rect)
    public func is_focused(&self, key : Key) : bool
    public func focus(&mut self, key : Key)
    public func scroll_offset(&self, key : Key) : Point
    public func set_scroll(&mut self, key : Key, offset : Point)
    public func scroll_to(&mut self, key : Key)

    public func clipboard(&self) : string_view
    public func set_clipboard(&mut self, text : string_view)
    public func begin_drag(&mut self, source : Key, payload : UserEvent)
    public func accept_drop(&mut self, target : Key, accepted : bool)
    public func open_url(&mut self, url : string_view) : Result<std::Unit, UiError>

    public func theme_mode(&self) : ThemeMode
    public func set_theme_mode(&mut self, mode : ThemeMode)
    public func set_theme(&mut self, theme : Theme)
    public func theme(&self) : &Theme
    public func density(&self) : Density
    public func set_density(&mut self, d : Density)
    public func disabled_opacity(&self) : float

    public func now_ms(&self) : double
    public func frame(&self) : u64
    public func stats(&self) : FrameStats
    public func sender(&self) : EventSender

    public func window(&self) : WindowId
    public func open_window(&mut self, spec : WindowSpec) : Result<WindowId, UiError>
    public func close_window(&mut self, window : WindowId)
    public func windows(&self) : &[WindowId]
    public func window_size(&self, window : WindowId) : Option<Size>
    public func set_window_size(&mut self, window : WindowId, size : Size)
    public func focus_window(&mut self, window : WindowId)
    public func set_window_title(&mut self, window : WindowId, title : string_view)
    public func set_menu(&mut self, window : WindowId, menu : &[Menu]) : Result<std::Unit, UiError>
    public func open_file_dialog(&mut self, opts : FileDialogOptions) : Result<std::Unit, UiError>
    public func notify(&mut self, n : Notification) : Result<std::Unit, UiError>
    public func set_tray(&mut self, menu : &[Menu]) : Result<std::Unit, UiError>  // v1.1

    public func announce(&mut self, live : u8, text : string_view)
    public func bind_shortcut(&mut self, s : Shortcut, cmd : CommandId)
        : Result<std::Unit, UiError>
    public func unbind_shortcut(&mut self, s : Shortcut)
    public func report_error(&mut self, key : Key, err : UiError)
}
```

Full kind→payload mapping, lifetimes, IME, drag, and window rules are in
`CONTRACTS.md` §3 and `HOST.md` §4. `Payload` values are borrowed for the
`update` call only.

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
    var family_name : string_view
    var size : float
    var weight : FontWeight
    var italic : bool
    var line_height : float       // multiplier
    var letter_spacing : float
    var features : TextFeatures
    var tab_size : u8
}
@direct_init
public struct TextFeatures {
    var ligatures : bool; var tabular_numbers : bool; var slashed_zero : bool
    var stylistic_set : u8; var raw_tags : &[u32]
}
@direct_init
public struct TextLayoutMetrics {
    var width : float; var height : float
    var first_baseline : float; var last_baseline : float
    var line_gap : float; var lines : u32; var truncated : bool
    var cap_height : float; var x_height : float
}
@direct_init
public struct TextConstraints {
    var max_width : float; var max_lines : u16
    var wrap : WrapMode; var overflow : TextOverflow
    var tab_size : u8; var direction : Direction
}
public struct TextContext {
    public func measure(&self, text : string_view, style : TextStyle,
                        c : TextConstraints) : TextLayoutMetrics
    public func measure_rich(&self, spans : &[TextSpan], style : TextStyle,
                             c : TextConstraints) : TextLayoutMetrics
}
```

- The full architecture, file-format matrix, shaping tiers, cache key, bidi,
  Unicode tables, IME/editing, secure redaction, WASM backend, and determinism
  are in [`TEXT.md`](TEXT.md). `measure_text` is not a free function.
- No font is embedded by default. Tests pin a font **stack**; system/browser
  text is tolerance-only. Color emoji is v1.1; tofu + `FontWarning` in v1.
- `TextSpan` gains `weight`, `italic`, `family`, `features`, `link`, and
  `decoration`; `rich_text` is measured by `measure_rich`.

---

## 7. Painter (custom Canvas content)

```chemical
public struct Painter {
    public func size(&self) : Size
    public func theme(&self) : &Theme
    public func color(&self, t : ColorToken) : Color

    public func push_clip(&mut self, r : Rect, radius : float) : ClipGuard
    public func push_opacity(&mut self, o : float) : LayerGuard
    public func push_transform(&mut self, t : Transform2D) : TransformGuard
    // guards auto-close on return; max depth 16 → Internal

    public func rect(&mut self, r : Rect, color : Color)
    public func rounded_rect(&mut self, r : Rect, radius : float, color : Color)
    public func border(&mut self, r : Rect, radius : float, width : float, color : Color)
    public func line(&mut self, a : Point, b : Point, width : float, color : Color)
    public func path(&mut self, path : &Path, color : Color)
    public func gradient_rect(&mut self, r : Rect, g : Gradient)
    public func shadow(&mut self, r : Rect, radius : float, s : Shadow)
    public func image(&mut self, img : ImageHandle, r : Rect, fit : ImageFit)
    public func text(&mut self, text : string_view, pos : Point, style : TextStyle, color : Color)
}
```

`Path` is a command list (move/line/quad/cubic/close); `Gradient` is
linear/radial with stops. Painting is immediate into the display list, clipped
by the Canvas node and any guards; no tree mutation, no IO, no clock reads.
`paint` receives `CanvasPaint { node, key, id, bounds, scale }` and returns
`Result`; an unbalanced guard is auto-closed and reported. Unknown `CanvasId`/
`ImageHandle` paints a placeholder and reports once.

---

## 8. Semantics and accessibility

```chemical
public enum Role {
    Unknown, Button, Checkbox, Radio, Switch, Slider, TextField, MultilineTextField,
    Combobox, Listbox, Option, Spinbutton, SearchBox,
    Heading, Text, Image, Link, List, ListItem, Table, Grid, Row, Cell, ColumnHeader,
    Tree, TreeItem, Tab, TabList, TabPanel, Menu, MenuItem, MenuBar, MenuButton,
    Toolbar, Banner, Main, Navigation,
    Dialog, AlertDialog, Alert, Tooltip, ProgressBar, Status, Group, Form, Label,
    Separator, ScrollView, Canvas, Custom
}

@direct_init
public struct Semantics {
    var role : Role
    var label : string_view
    var description : string_view
    var value : string_view
    var alt : string_view
    var lang : string_view
    var placeholder : string_view
    var error : string_view
    var states : SemState
    var actions : SemAction
    var level : u8
    var pos_in_set : u32
    var set_size : u32
    var row_index : u32
    var column_index : u32
    var selected_index : i32
    var labelled_by : Option<NodeId>
    var described_by : Option<NodeId>
    var controls : Option<NodeId>
    var owns : Option<NodeId>
    var active_descendant : Option<NodeId>
    var has_popup : bool
    var secure : bool
}

@direct_init
public struct SemState {
    var focusable : bool; var focused : bool; var disabled : bool
    var checked : bool; var selected : bool; var expanded : bool
    var busy : bool; var invalid : bool; var readonly : bool
    var required : bool; var pressed : bool; var hidden : bool
    var modal : bool; var live : u8          // Off/Polite/Assertive
    var multiselectable : bool
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
public enum ErrorClass { Expected, AppBug, Internal, Fatal }
public enum ErrorCode {
    Platform, Surface, Font, Image, Layout, InvalidKey, StaleNode, OutOfMemory,
    Protocol, Io, Timeout, NotImplemented, Unsupported, NotFound, PermissionDenied,
    InvalidArgument, Decode, ResourceExhausted, Cancelled, Busy, WouldBlock, DuplicateKey,
    StaleKey, ThreadAffinity, DeviceLost, Internal, Other
}
@direct_init
public struct UiError {                // private fields; @make only
    private var code_value : ErrorCode
    private var class_value : ErrorClass
    private var detail_value : ErrorDetail   // static/inline, never a borrowed dynamic view
    private var source_value : Option<SourceLocation>
    private var retry_value : RetryHint
    @make func make(code : ErrorCode, message : &'static str) : UiError
    public func code(&self) : ErrorCode
    public func class(&self) : ErrorClass
    public func retry_hint(&self) : RetryHint
    public func detail(&self) : &ErrorDetail
    public func source(&self) : Option<SourceLocation>
    public func with_source(&self, loc : SourceLocation) : UiError
    public func with_retry(&self, hint : RetryHint) : UiError
}

public func classify(code : ErrorCode) : ErrorClass
public func <T> ok(value : T) : Result<T, UiError>
public func <T> err(error : UiError) : Result<T, UiError>
public func <T, U> map(r : Result<T, UiError>, f : (x : T) => U) : Result<U, UiError>
```

`UiError.make` never creates `Fatal`: `classify` maps app codes to `Expected` or
`Internal` only; `Fatal` is reserved for a private runtime helper. Default retry
hints are `Backoff` for `Io/Timeout/Busy/WouldBlock/DeviceLost` and `Never`
otherwise. `detail()` returns the inline static detail; `with_source`/`with_retry`
copy the value without allocation. The file view in `SourceLocation` is a
borrowed view and must be static/host-owned if retained by debug tooling.

The full code/recovery table and allocation-free detail format are in
`CONTRACTS.md` §7. `Other` + `ErrorDetail.code` keeps the enum extensible. Error cards are keyed
runtime subtrees with role `Alert`, user-safe text, optional Retry, focus
restore, and one log entry.

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

The full schema, auth/limits/redaction, CLI commands/exit codes, missing
methods (`viewport.set`, `hit_test`, `focus.get`, `scroll.get`, `tree.diff`,
`wait.idle`, `error.list`, `gpu.*`, `fonts.*`), and WASM transport are in
[`PROTOCOL.md`](PROTOCOL.md). The **frozen** method/CLI name list is
[`contracts/protocol.json`](contracts/protocol.json); P5.1 commits the full
`protocol.schema.json`.

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
    public func app(&self) : &A
    public func app_mut(&mut self) : &mut A
    public func size(&self) : Size
    public func set_size(&mut self, w : float, h : float)
    public func set_theme(&mut self, mode : ThemeMode)
    public func set_reduced_motion(&mut self, on : bool)
    public func set_scale(&mut self, scale : float)

    public func frame(&mut self) : Result<FrameStats, UiError>
    public func advance(&mut self, ms : double) : Result<FrameStats, UiError>
    public func settle(&mut self) : Result<u32, UiError>     // frames; Timeout if unstable
    public func wait_until(&mut self, pred : (x : &A) => bool, timeout_ms : u32) : Result<std::Unit, UiError>

    public func find_role(&self, role : Role) : Option<NodeId>
    public func find_name(&self, name : string_view) : Option<NodeId>
    public func find_test_id(&self, id : string_view) : Option<NodeId>
    public func find_all(&self, q : Query) : &[NodeId]
    public func within(&self, parent : NodeId, q : Query) : &[NodeId]
    public func focused(&self) : Option<NodeId>
    public func scroll_offset(&self, key : Key) : Point
    public func by_key(&self, key : Key) : Option<NodeId>
    public func semantics(&self, id : NodeId) : Option<&Semantics>
    public func bounds(&self, id : NodeId) : Option<Rect>
    public func text_of(&self, id : NodeId) : Option<&str>

    public func click(&mut self, id : NodeId) : Result<std::Unit, UiError>
    public func double_click(&mut self, id : NodeId) : Result<std::Unit, UiError>
    public func hover(&mut self, id : NodeId) : Result<std::Unit, UiError>
    public func drag(&mut self, from : NodeId, to : NodeId) : Result<std::Unit, UiError>
    public func pointer(&mut self, kind : EventKind, x : float, y : float) : Result<std::Unit, UiError>
    public func scroll(&mut self, id : NodeId, dx : float, dy : float) : Result<std::Unit, UiError>
    public func key(&mut self, code : KeyCode, mods : Modifiers) : Result<std::Unit, UiError>
    public func type_text(&mut self, text : string_view) : Result<std::Unit, UiError>
    public func ime(&mut self, ev : ImeTestEvent) : Result<std::Unit, UiError>

    public func screenshot(&self, path : string_view) : Result<Screenshot, UiError>
    public func assert_golden(&self, path : string_view, opts : GoldenOptions) : Result<GoldenResult, UiError>
    public func assert_semantics(&self, expected : &ComponentSpec) : Result<std::Unit, UiError>
    public func assert_no_overflow(&self) : Result<std::Unit, UiError>
    public func geometry_snapshot(&self) : string          // JSON
    public func semantics_snapshot(&self) : string         // JSON
    public func audit(&self) : AuditReport
}
```

Full harness rules, `Query`, `ComponentSpec`, `GoldenOptions`, and the state
matrix are in [`CATALOG.md`](CATALOG.md) §4. `GoldenOptions` defaults to
bit-exact (`max_diff_pixels = 0`, `yiq = 0`); a tolerance must be explicit.
Golden updates require `--update-goldens` plus parent review of old/new/diff.

---

## 12. Component catalog conventions

Every `ui_kit` component exports:

1. a `Props` struct with an `@make` constructor, typed token-only fields, and defaults;
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
- `ui_api_index.json` is generated/committed; CI fails on a removed/renamed
  symbol, changed signature/default, or changed `Props`/slot. This is the
  machine-checked freeze, not prose.
- `UiError` uses `ErrorCode` + `Other` + detail code so new expected failures do not
  break exhaustive matches. `Application` extensions use optional companion
  interfaces (`Lifecycle`, `Stateful`, `TaskHost`) or default methods where the
  language supports them; adding a required method to `Application` is a major
  change.
- Additive changes are allowed in a minor release (new methods, new components,
  new protocol methods, new semantics roles) **only if** the API index diff is
  additive and construction/defaults remain compatible.
- Breaking changes require a new major package (`ui2`) or an explicit
  parent-approved amendment here plus a migration note; they are not allowed to
  land silently during the plan.
- Freeze is effective at P0.2; the first app (P1.14 walking skeleton) exercises
  the contract before Stage 2. Internal names are prefixed `internal_` or live in
  `ui::internal`; they are not part of the contract.

---

## 14. Amendments

| Date | Amendment | Approved by |
|------|-----------|-------------|
| — | Initial contract | — |
