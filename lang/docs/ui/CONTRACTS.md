# Chemical UI — normative contract amendments

**Status:** normative, must be merged into the frozen v1 contract before P0.2
**Date:** 2026-11-27
**Precedence:** if this file conflicts with `API.md`, this file wins until P0.2
folds the decisions into `API.md`. P0.2 must make every declaration here
compile; prose is not enough.

These amendments exist because six independent reviews found places where an
implementing AI would otherwise invent behavior. They are decisions, not
options. Anything marked **FROZEN** cannot change without a recorded amendment.

---

## 1. Frame, app, and window model — FROZEN

### 1.1 FrameInfo

Every `view` and `paint` call receives the frame context. This fixes responsive
layout, theme access in paint, locale, reduced motion, and multi-window.

```chemical
@direct_init
public struct FrameInfo {
    var window : WindowId
    var viewport : Viewport          // logical size + scale + safe area
    var scale : float                // devicePixelRatio
    var theme : ThemeMode            // resolved Light/Dark/HighContrast
    var reduced_motion : bool
    var locale : string_view         // BCP-47, e.g. "en-US"
    var direction : Direction        // Ltr | Rtl | Auto-resolved
    var frame : u64
    var now_ms : double              // virtual monotonic time
    var focused : bool
    var occluded : bool
}

@direct_init
public struct Viewport {
    var size : Size                  // logical points (float)
    var safe_area : Insets           // notch/status bar in logical points
    var breakpoint : Breakpoint      // Sm | Md | Lg | Xl
}

public enum Breakpoint { Sm, Md, Lg, Xl }   // <640, 640–1023, 1024–1439, ≥1440
```

`view` is invoked on resize, scale change, theme/locale/reduced-motion change,
and explicit `request_view`. It is **not** invoked on hover/focus/press, and not
on ordinary scroll — except for a virtualised container, whose scroll must
produce a new visible window (see §2).

### 1.2 Application interface — FROZEN

```chemical
public interface Application {
    // Called once per window before the first view; may spawn work via ctx.
    func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx)
        : Result<std::Unit, UiError>

    // Pure read of app state + FrameInfo. May mark a subtree failed with b.error.
    func view(&self, frame : &FrameInfo, b : &mut ViewBuilder)
        : Result<NodeId, UiError>

    // The only place app model state may mutate.
    func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx)
        : Result<std::Unit, UiError>

    // Custom Canvas painting; read-only. CanvasPaint identifies the node.
    func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter)
        : Result<std::Unit, UiError>

    // Return Allow to close; Veto keeps the window open. Required; return Allow.
    func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction
}

public enum CloseAction { Allow, Veto }
```

Rules:

- A returned `UiError` from `view`/`update`/`paint` renders a global error card
  for that window; the app stays alive. It never aborts.
- `b.error(node_or_parent, err)` renders a keyed error card for a subtree.
- `view`/`paint` may not mutate app state. `update` is the only mutation point.
- `on_close` may mutate the model (save a draft) and return `Veto`; the host
  must not destroy the window until a later close returns `Allow`.
- Every window has its own retained tree, layout, damage, focus, scroll, and
  animation clock. Theme, font cache, atlases, and images are shared.

### 1.3 Multi-window — FROZEN for v1

```chemical
@direct_init
public struct WindowSpec {
    var title : string_view
    var size : Size
    var min_size : Size
    var max_size : Size
    var position : Option<Point>
    var resizable : bool
    var decorated : bool
    var always_on_top : bool
    var transparent : bool
    var start_visible : bool
}

public struct WindowId { /* private index+generation */ }   // opaque, private

public struct UiCtx {
    public func window(&self) : WindowId
    public func open_window(&mut self, spec : WindowSpec) : Result<WindowId, UiError>
    public func close_window(&mut self, window : WindowId)
    public func windows(&self) : &[WindowId]
    public func window_size(&self, window : WindowId) : Option<Size>
    public func set_window_title(&mut self, window : WindowId, title : string_view)
    public func set_window_size(&mut self, window : WindowId, size : Size)
    public func focus_window(&mut self, window : WindowId)
}
```

The initial window is created by `ui_host::run` from `Options`. Closing the last
window ends the loop. Multi-window is in v1; P3.6 implements it. If a platform
cannot support it, `open_window` returns `UiError.Unsupported` and the app must
degrade — it may not silently fake a window.

### 1.4 Frame loop — FROZEN

One frame is:

1. Drain the host event queue in order, coalescing pointer-move/scroll per node
   and keeping key/text/IME order.
2. Dispatch `update` for each semantic event; multiple events may be coalesced
   into one `view` only when the runtime can prove no observable ordering loss.
3. Run at most one `view`/reconcile, using the final event state.
4. Run one layout pass for dirty subtrees.
5. Build one display list + semantics delta + damage tiles.
6. Rasterise (CPU) or submit (GPU), then present once.
7. Increment `frame`; record `FrameStats`.

`timestamp_ms` on `UiEvent` is virtual monotonic time at frame start, paused
when the debug clock is paused. `UiCtx.now_ms()` returns the same domain. OS
wall-clock time is never used in layout, paint, or animation.

---

## 2. Identity, keys, virtualization — FROZEN

### 2.1 Key

```chemical
public struct Key {                 // private fields; constructors only
    private var hash : u64
    private var tag : u64
    private var id : u64

    public func id(&self) : u64
    public func tag(&self) : u64
    public func equals(&self, label : string_view) : bool
    public func equals_key(&self, other : Key) : bool
}

public func key(label : string_view) : Key          // hash(label), tag=0, id=0
public func key_of(label : string_view, id : u64) : Key   // id=id
public func key_pair(label : string_view, a : u64, b : u64) : Key // id=a, tag=b
```

- `Key` holds only `(hash, tag, id)` and has private fields; constructors are
  the only creation path. Labels are **not stored** in `Key`; a best-effort
  debug table maps hash→label for protocol display and may drop entries without
  affecting identity. Dynamic labels are allowed; the debug table is bounded
  and never a correctness dependency.
- Equality is the full tuple `(hash, tag, id)`; `key_pair` uses an
  order-sensitive hash of `label:a:b`, not XOR.
- **Key scope is immediate siblings.** A component/overlay/fragment gets its own
  scope. Use `b.scope(key)` or `b.scoped_key(parent, label)` for repeated
  internal labels. Duplicate keys in the same scope are a compile-time/runtime
  diagnostic (`DuplicateKey`), never silent state theft.
- `NodeId` is **frame-local** (view handle). `Key` is the cross-frame identity
  for all runtime state: hover, focus, pressed, scroll, text, selection,
  animation, custom widget state. `NodeId` is never stored across frames.
- `NodeId` is private-construction, `{ index:u32, generation:u32 }`; generation
  wraps cause slot retirement + `ResourceExhausted`, never aliasing. All lookups
  return `Option`/`Result`.
- `ImageHandle`, `FontHandle`, `IconId`, `CanvasId` are private, generational or
  registry-checked handles. Public constructors do not exist; APIs return
  `Result`/`Option`. Stale handles paint a placeholder and report once.

### 2.2 Scroll and virtualization

```chemical
public struct ViewBuilder {
    public func scroll_offset(&self, key : Key) : Point
    public func viewport(&self) : Viewport
    public func frame(&self) : &FrameInfo

    // Runtime-owned virtual list. The source is called only for visible items
    // plus overscan. Scrolling a virtual list requests one view rebuild.
    public func virtual_list<S : ItemSource>(
        &mut self, key : Key, style : Style, axis : Axis, source : &S
    ) : NodeId
}

public interface ItemSource {
    func count(&self) : size_t
    func key(&self, index : size_t) : Key
    func item(&self, b : &mut ViewBuilder, index : size_t) : NodeId
    func extent(&self, index : size_t) : float   // logical px; fixed or measured
}
```

Rules:

- Ordinary `scroll` updates offset and paints only (blit + expose).
- A `virtual_list` scroll triggers **one** `view` for the visible window; no
  offscreen row is built. This is the only allowed scroll→view path and must be
  counted in `FrameStats.virtual_rebuilds`.
- Scroll offsets are logical points, preserved by key across reorder, clamped on
  resize/content shrink, and anchored to the top-left unless `anchor` is set.
- `List`, `Table`, `Tree`, `Grid` use this contract. A non-virtualised list above
  `UI-LIST-001` (200 items) is an audit error.
- `extent(index)` must be O(1): fixed extents or a maintained measured-extent
  prefix index (updated incrementally); the runtime never scans all rows per
  frame. Expose `virtual_extent_calls`/`virtual_index_builds` counters.
- Hostile `ItemSource` is contained: `count()` clamps to `Limits.max_view_nodes`;
  `extent()` must be finite/≥0 and sums use checked saturating math; only visible
  + overscan items are requested; recursion depth ≤32; duplicate `key(index)`
  emits a diagnostic and an error card. The builder never loops on NaN/Inf.

---

## 3. Events, payloads, and lifetimes — FROZEN

```chemical
public struct UiEvent<'a> {
    var window : WindowId
    var key : Key
    var kind : EventKind
    var payload : Payload<'a>
    var modifiers : Modifiers
    var local_x : float      // logical px, relative to target border box
    var local_y : float
    var timestamp_ms : double
    var frame : u64
}
```

`local_x/local_y` are logical points relative to the target node's border box;
`-1` means not applicable. Pointer coordinates outside the node are still
reported during capture.

### 3.1 Kind → payload table (normative)

| Kind | Payload |
|------|---------|
| `Click`, `DoubleClick`, `ContextMenu` | `Pointer { button, press_count, pointer_kind }` |
| `Press`, `Release` | `Pointer { button, pointer_kind }` |
| `PointerMove`, `PointerEnter`, `PointerLeave` | `Pointer { pointer_kind, pressure, tilt }` |
| `Scroll` | `Scroll { dx, dy, phase }` where phase = Begin/Update/End/Inertia |
| `KeyDown`, `KeyUp` | `Key { code : KeyCode, repeat : bool, text : Option<string_view> }` |
| `TextInput` | `Text { text }` |
| `ImePreedit` | `Ime { preedit, caret, anchor, replace : Range, candidate : Option<Rect> }` |
| `ImeCommit` | `Ime { commit, caret, replace : Range }` |
| `Focus`, `Blur` | `Focus { focused }` (`FocusPayload`) |
| `Change` | `Value { kind, text, number, range, selected, checked }` |
| `Submit` | `Value { text }` (one `ValuePayload`) |
| `Select` | `Index { index, selected }` (`IndexPayload`) |
| `Open`, `Close` | `None` |
| `Resize` | `Size { width, height, scale }` |
| `DpiChange` | `Scale { scale }` |
| `Drop` | `Files { paths : &[string_view], text : Option<string_view> }` |
| `DragBegin/Over/Leave/DragDrop` | `Drag { payload, x, y, target, accepted }` |
| `Timer` | `Timer { id }` |
| `Frame` | `Frame { now_ms, dt_ms }` |
| `Command` | `Command { id }` |
| `FileChosen` | `Files { paths, text }` |
| `OpenFile` | `Files { paths, text }` |
| `SecondInstance` | `User { tag, a, b, text }` |
| `ImageReady` | `Image { handle }` |
| `ImageFailed` | `Error { err }` |
| `FontReady` | `Font { handle }` |
| `Custom` | `User { tag, a, b, text }` |

`Payload<'a>` is a variant with these structs; `None()` for no data. A key code
set is fully enumerated in `ui::input` (letters, digits, F1–F24, navigation,
editing, modifiers, media keys); no `/* … */` placeholder in the frozen API.

### 3.2 Ownership

- `UiEvent<'a>` and its borrowed strings/slices are valid **only for the
  `update` call**. They cannot be stored; the lifetime parameter makes that a
  compile error. To retain, copy (`to_string`, `to_owned`).
- `ViewBuilder` copies every string/slice it receives (text, labels, test ids,
  spans, component names) into the frame arena. **Image bytes are the exception:**
  `ImageSource.Bytes` is copied synchronously into the image cache's owned
  allocation under `Limits.max_image_bytes` and the worker owns that buffer; it
  never points into the frame arena. Retained nodes, semantics, a11y, and
  protocol snapshots own/intern text; they never read the view arena after
  reconciliation.
- `UiCtx.text(key)` returns `&str` tied to the `&UiCtx` borrow; copy to retain.
- `UiCtx.text_state` returns `Option<&TextEditState>` and `UiHarness.semantics`
  returns `Option<&Semantics>`, both tied to the receiver borrow. They cannot be
  stored past the call. `GpuInfo` device/driver strings are static/host-owned;
  `Screenshot`/`GoldenResult` paths point at caller-provided buffers.
- `EventSender.post` copies the `UserEvent.text` view into an owned queue entry
(fallible, `Limits`) and returns `Cancelled` after shutdown; it never retains the
caller view. See §9.
- Every API that **retains** text copies/interns it synchronously: `run` copies
  `Options` strings; `Theme.set_*` copies family names; `UiCtx.set_window_title`,
  `set_menu`, `notify`, `open_file_dialog`, `announce`, `set_text` copy/validate
  before returning; `WindowSpec`/`Menu`/`Notification` are copied on submission.
  No retained API stores a caller `string_view`. Copy failure returns
  `ResourceExhausted` where the signature allows a `Result`; void setters leave
  the old value and log `Internal`.

---

## 4. View construction — FROZEN

### 4.1 Dimensions

```chemical
public variant Dim { Auto() Px(value : float) Percent(value : float) Fill() }
@direct_init public struct Size { var width : float; var height : float }
```

`Style.width/height/basis` take `Dim`; `Painter.size()`, `UiHarness.size()`,
`Viewport.size`, `Resize` payloads take `Size`. There is no overloaded `Size`
enum.

### 4.2 Style additions (all token-only in the kit)

```chemical
public struct Style {
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

public variant Position { Relative() Absolute(top : Dim, right : Dim, bottom : Dim, left : Dim) }
public enum Direction { Auto, Ltr, Rtl }
public enum TextAlign { Start, Center, End, Justify }
public enum WrapMode { NoWrap, Wrap, WrapReverse }
public enum TextOverflow { Clip, Ellipsis }
```

`b.style(node, style)` is a **field-wise overlay**, not a blind replace: it
preserves unspecified fields. `Style.merge` is explicit. `b.state_style(node,
WidgetState, Style)` sets hover/focus/pressed/disabled/selected visuals resolved
by the runtime without calling `view`.

### 4.3 Composition, slots, fragments, overlays

```chemical
public struct ViewBuilder {
    public func fragment(&mut self, key : Key) : NodeId       // display: contents
    public func component(&mut self, key : Key, name : string_view) : Component
    public func slot(&mut self, c : Component, name : string_view) : NodeId
    public func overlay(&mut self, key : Key, layer : OverlayLayer, anchor : Option<NodeId]) : NodeId
    public func grid_cell(&mut self, node : NodeId, col : u16, row : u16, col_span : u16, row_span : u16)
    public func state_style(&mut self, node : NodeId, state : WidgetState, style : Style)
    public func error(&mut self, node : NodeId, err : UiError)
}

public struct Component { /* root + named slots, private */ }
public enum OverlayLayer { Tooltip, Popover, Menu, Dialog, Toast }
public enum WidgetState { Hover, Focus, Pressed, Disabled, Selected, Checked, Invalid }
```

- A fragment has no layout box; its children participate in the parent.
- A component exposes named slots; `slot` returns a node that the caller fills.
  Catalog entries list slot names and required/optional status.
- Overlay layers are children of a per-window overlay root, **outside clips**.
  Stacking order: Tooltip < Popover/Menu < Dialog < Toast. `Style.z` orders
  within a layer. Modal layers trap focus and mark the background inert.
- `animate_exit` keeps the node alive until the exit animation finishes, then
  removes it; `animate` is one-shot and may repeat/alternate via its spec.
- `ImageHandle` is obtained through `ui_images`/`ui::raw::register_image`; `Path` and
  `Gradient` are defined in `Painter` (line/quad/cubic/close; linear/radial with
  stops). Canvas identity is `CanvasPaint { node, key, id, bounds, scale }`.

### 4.4 Painter / Canvas

```chemical
@direct_init
public struct CanvasPaint {
    var node : NodeId
    var key : Key
    var id : CanvasId
    var bounds : Rect
    var scale : float
}

public struct Painter {
    public func size(&self) : Size
    public func theme(&self) : &Theme
    public func color(&self, t : ColorToken) : Color
    public func push_clip(&mut self, r : Rect, radius : float) : ClipGuard
    public func push_opacity(&mut self, o : float) : LayerGuard
    public func push_transform(&mut self, t : Transform2D) : TransformGuard
    // guards auto-close on function return; max depth 16, then Internal error
}
```

`paint` returns `Result`; an early return with an unbalanced guard is
auto-closed and reported. Unknown `CanvasId`/`ImageHandle` paints a placeholder and
reports once.

---

## 5. Layout — FROZEN algorithm and honest gates

The pipeline is:

1. **Intrinsic measure**: for each dirty subtree compute min/max content size.
   Text measures at available width; `Auto` main size needs this pass.
2. **Constraint resolution**: resolve `Auto/Px/Percent/Fill`, min/max, aspect,
   grid tracks, wrap lines.
3. **Distribution**: grow/shrink, justify/align, gap/padding, absolute/grid
   placement.
4. **Text re-break**: shape/break at final width if the width changed; reuse if
   the `TextLayoutKey` matches.
5. **Place**: assign border boxes, baselines, clip/scroll extents.

Maximum **3 passes** for flex/wrap/grid; text may re-break once. No unbounded
iteration. Grow distributes remaining space proportional to `grow`; shrink is
weighted by `shrink × base`; ties break by child order. `Fill` = `grow:1,
basis:0`. `Percent` inside a scroll resolves against the **viewport** main axis,
never an unbounded parent. Grid `Auto` resolves after intrinsic measure, then
`Fr` distributes remaining space; ties break by track order. `Baseline` on a
non-text child uses its bottom edge. Zero/negative/NaN/Inf clamp to zero and emit
a `LayoutWarning` event. Fixtures: `lang/tests/ui/fixtures/layout/*.json` with
expected rects + provenance (WPT/Taffy subset) and a 0.5 px oracle; committed
before P1.6.

Named gates (see `BENCHMARKS.md` for scene JSON):

| Scene | Target | Hard ceiling |
|-------|--------|--------------|
| 10k simple nodes (fixed sizes, no text) cold | 1.5 ms | 2.5 ms |
| 10k mixed nodes (20% text, auto, wrap) cold | 4 ms | 6 ms |
| 100k wide/shallow nodes cold | 30 ms | 45 ms |
| Single fixed-width label change in 10k | 50 µs | 100 µs |
| No-change warm frame | 0 relayout | 0 |

---

## 6. Raster and color — FROZEN

- **Color space:** premultiplied **sRGB-encoded 8-bit UNORM** blending, matching
  Canvas2D and a GPU UNORM framebuffer. This is not linear-light blending; the
  docs must stop saying "sRGB-correct" without qualification. Linear-light
  gradients are a later opt-in.
- **Blend:** `out = src + div255(dst * (255 - src.a))`, `div255(x) = (x + 128 +
  ((x + 128) >> 8)) >> 8`; integer, no float; alpha 0..255.
- **Coverage:** 8-bit coverage from analytic area or 16×16 subsampling for
  curves; solid rects use exact integer coverage. Fixed-point, no FMA.
- **Pixel convention:** pixel `(i,j)` covers `[i,i+1) × [j,j+1)`, sample at the
  centre; fill rule non-zero; abutting rects must not overlap by more than 0.
- **Tiles:** coverage/AA is computed in absolute device coordinates; a tile is a
  scissor, never a local coordinate origin. No seams at tile boundaries.
- **Text:** alpha-only atlas (R8), grayscale AA, hinting off, subpixel bins:
  4 bins for size < 20 px, 1 bin for ≥ 20 px; baseline snapped to 1/64 px;
  atlas pages grow without invalidating existing pages; eviction/repack only at
  frame boundaries; `atlas_generation` bumps only on eviction/repack and
  invalidates tile/layer caches (counted as a full repaint).
- **GPU parity:** GPU consumes the same display list and uses a UNORM target;
  GPU goldens are per-driver tolerance, never cross-machine equality.
- **sRGB LUT:** no LUT in the reference path; data-viz/linear gradients may use
  a LUT later behind an explicit option.

---

## 7. Errors, panic, OOM — FROZEN

### 7.1 UiError

```chemical
public enum ErrorClass { Expected, AppBug, Internal, Fatal }
public enum ErrorCode {
    Platform, Surface, Font, Image, Layout, InvalidKey, StaleNode, OutOfMemory,
    Protocol, Io, Timeout, NotImplemented, Unsupported, NotFound, PermissionDenied,
    InvalidArgument, Decode, ResourceExhausted, Cancelled, Busy, WouldBlock, DuplicateKey,
    StaleKey, ThreadAffinity, DeviceLost, Internal, Other
}
@direct_init
public struct UiError {
    private var code : ErrorCode
    private var class : ErrorClass
    private var detail : ErrorDetail   // static/inline only
    private var source : Option<SourceLocation>
    private var retry : RetryHint
    @make func make(code : ErrorCode, message : &'static str) : UiError
}
```

`UiError` is **allocation-free** on the failure path (inline/fixed detail, static
strings, numeric codes). `Other` + `ErrorDetail.code` keeps the enum extensible without
breaking exhaustive matches. The error card is a keyed runtime subtree with
role `Alert`, user-safe message, optional Retry, focus restore, and one log
entry; internal details only in debug.

Recovery is code-driven:

| Code | Class | Default recovery |
|------|-------|------------------|
| `NotFound`, `Unsupported`, `PermissionDenied`, `InvalidArgument`, `Decode`, `Font`, `Image`, `Layout`, `InvalidKey`, `StaleKey`, `DuplicateKey` | Expected | error card/placeholder; user may edit/retry |
| `Io`, `Timeout`, `Busy`, `Cancelled`, `WouldBlock` | Expected | retry with backoff or show retry |
| `OutOfMemory`, `ResourceExhausted` | Expected | placeholder/truncate; never grow |
| `Protocol`, `Surface`, `DeviceLost` | Expected/Internal | recreate/fallback; report |
| `ThreadAffinity`, `StaleNode` | Internal | safe fallback + log; fix the caller |
| `Internal` | Internal | error card + log; fix the caller |
| `Fatal` | Fatal | abort with diagnostic; whitelist only |

`UiError.make` never sets `Fatal`; any app-supplied code is downgraded to
`Internal`/`Expected` by class. Only `ui::internal::fatal` may abort.

### 7.2 Panic criterion

An abort is permitted only when the next operation would:

- read/write outside an allocation,
- double-drop/double-free,
- race on UI state, or
- corrupt a live handle table beyond recovery.

Everything else returns `UiError.Internal` and a safe fallback. Panic is never
used for expected IO/decode/layout/OOM/stale/misuse. Every allowed site is in
`PANIC_WHITELIST.md` with: ID, callsite, invariant, mechanical proof, test that
keeps it unreachable, owner, expiry, approval. A child-process probe may reach
it; the normal suite never does.

### 7.3 Arithmetic/OOM

- Public boundaries reject NaN/Inf/negative/overflow; saturating/checked math
  with defined clamps. Division by zero returns `Internal`/clamp, never abort.
- Float→int casts are range-checked; `--ui-scale`, sizes, timers, protocol
  numbers are bounded.
- UI-owned buffers use `try_reserve`/`try_push`/`try_new`; OOM returns
  `OutOfMemory` and a placeholder. `std::vector::push`/`string::append` still
  panic on OOM in std (known blocker); UI code must use fallible forms and lint
  `UI-ALLOC-001` flags infallible growth in hot paths. Examples must not grow
  unbounded collections without a limit.
- `FrameStats.allocs` is measured by a linker-wrapped allocator in the bench
  build; steady-state gate excludes documented cold/high-water growth.

---

## 8. Resource limits — FROZEN defaults

```chemical
@direct_init
public struct Limits {
    var max_view_nodes : u32 = 200000
    var max_retained_nodes : u32 = 200000
    var max_text_bytes : u64 = 64 << 20
    var max_image_pixels : u64 = 64 << 20       // 64 MP
    var max_image_bytes : u64 = 256 << 20
    var max_font_bytes : u64 = 32 << 20
    var max_atlas_bytes : u64 = 16 << 20
    var max_layer_bytes : u64 = 128 << 20
    var max_gpu_bytes : u64 = 160 << 20
    var max_display_commands : u32 = 200000
    var max_protocol_bytes : u32 = 4 << 20
    var max_capture_frames : u32 = 10000
    var max_screenshot_pixels : u64 = 64 << 20
}
```

`Options.limits` carries them. Exceeding a limit returns
`UiError.ResourceExhausted(resource, limit)` and degrades to a placeholder; it
never aborts. Per-window + global accounting appears in `FrameStats`.

---

## 9. Threads, tasks, timers — FROZEN

- UI state (`Application`, `UiCtx`, `ViewBuilder`, `Painter`, tree) is
  **UI-thread-only**, `!Send + !Sync`. Entry points check thread id in debug.
- Workers are for decode/hash/download only. Results are posted as owned
  `UserEvent` values through a `EventSender`; the UI thread drains them.
- Debug protocol runs on a protocol thread but only parses into a bounded
  command queue; all tree/surface access and snapshots happen on the UI thread.
- A render thread, if added, consumes an immutable `Send` display-list snapshot;
  it never touches the tree. v1 is single-threaded.

```chemical
public struct EventSender {   // Send + Sync + Clone
    public func post(&self, ev : UserEvent) : Result<std::Unit, UiError>
    public func wake(&self)
}
@direct_init public struct UserEvent {
    var tag : u64; var a : i64; var b : i64; var text : string_view
}
public struct UiCtx {
    public func sender(&self) : EventSender
    public func set_timeout(&mut self, ms : u32) : TimerId
    public func cancel_timeout(&mut self, id : TimerId)
    public func request_frame(&mut self)
    public func request_animation_frame(&mut self)
}
```

`EventKind.Frame` is delivered only when a frame was requested/animating.
`clock.advance(ms)` steps in fixed 1/60 s frames unless `dt_ms` is given. Timers
are virtual-clock based, cancelled on window close, and never fire after
shutdown.

---

## 10. Images and fonts — FROZEN

### 10.1 Images

```chemical
public struct ImageHandle { /* private generational */ }
public enum ImageState { Missing, Loading, Ready(ImageHandle), Failed(UiError) }
public func image(path : string_view) : ImageState
public func image_bytes(bytes : &[u8], hash : u64) : ImageState
public func image_release(h : ImageHandle)
public func image_state(h : ImageHandle) : ImageState
```

Path loaders use one `fs::read_limited(path, max, flags)` that checks a regular
file, `O_NOFOLLOW|O_NONBLOCK`, size ≤ limit, `try_reserve`, and maps failures to
`Io`/`ResourceExhausted`. `font_from_file`/`image(path)` pass `Limits` and never
buffer an unbounded file; FIFO/symlink/`/dev/zero` tests are mandatory.

`view` never decodes. `b.image(key, ImageSource)` accepts `Path`, `Bytes`
(copied into the image cache, then queued), `Handle`, or `Placeholder`; the node
paints the current state.
Decode runs on a worker with `Limits` caps, posts `ImageReady`/`ImageFailed`,
and cancels when the keyed node is removed. Cache is a bounded LRU with
per-image/global byte accounting. `ui_images` adds JPEG/WebP/GIF/SVG support
(optional package) without changing the core contract. `Painter.image` takes
`ImageHandle`, not a forgeable `u32`.

### 10.2 Fonts

```chemical
public struct FontHandle { /* private generational */ }
public struct FontStack { var ui : &[string_view]; var mono : &[string_view]; var fallback : &[string_view] }
public func system_stack() : FontStack
public func font_from_file(path : string_view) : Result<FontHandle, UiError>
public func font_state(h : FontHandle) : FontState
public func fetch_async(url : string_view, sha256 : string_view) : FontRequest
```

`fetch` is blocking and must never be called from `view`/`update`; use
`fetch_async` + `FontReady` event, or preload at startup. The compiler ships
no font. The optional 42 KB preset is downloaded/cached or embedded by the app.
Missing glyphs render tofu + a `FontWarning` event and a coverage report; they
never panic. `FontCacheEntry` carries `license`, `source_url`, `version`.
Detailed text rules: [`TEXT.md`](TEXT.md).

---

## 11. Host, present, protocol, catalog, packaging

- **Host/present contract:** [`HOST.md`](HOST.md). It defines surface formats,
  present/damage, request-frame, logical↔physical conversion, IME, a11y, macOS,
  WASM, and shutdown. P3.1 must implement it; P3.2–P3.4 cannot invent it.
- **Protocol and CLI:** [`PROTOCOL.md`](PROTOCOL.md). It defines the JSON-RPC
  schema, auth, limits, redaction, missing methods, CLI commands/exit codes,
  WASM bridge, and conformance tests.
- **Catalog/test harness:** [`CATALOG.md`](CATALOG.md). It defines
  `ComponentMeta`, slots, props schema, `ui_api_index.json`, state matrix,
  `ui_test` assertions, goldens, and the API-diff gate.
- **Design tokens:** [`DESIGN.md`](DESIGN.md) is normative; P1.5 uses those exact
  values. P2.11 may polish but may not invent a new token set.
### 11.1 Platform shell (freeze before P3.6)

```chemical
public struct MenuItem {
    var id : CommandId; var label : string_view; var shortcut : Option<Shortcut>
    var enabled : bool; var checked : bool; var separator : bool
}
public struct Menu { var title : string_view; var items : &[MenuItem] }
public struct FileDialogOptions {
    var title : string_view; var filters : &[string_view]
    var multiple : bool; var directory : bool; var save : bool
}
public struct Notification {
    var title : string_view; var body : string_view; var icon : Option<ImageHandle>
}
public struct UiCtx {
    public func set_menu(&mut self, window : WindowId, menu : &[Menu])
        : Result<std::Unit, UiError>
    public func open_file_dialog(&mut self, opts : FileDialogOptions)
        : Result<std::Unit, UiError>          // result via Files event
    public func notify(&mut self, n : Notification) : Result<std::Unit, UiError>
    // tray is v1.1; notify is v1
    public func set_tray(&mut self, menu : &[Menu]) : Result<std::Unit, UiError>
    // results arrive as FileChosen / OpenFile / SecondInstance events
}
```

Native menu/quit/preferences, file dialogs, notifications, and open-file/
second-instance delivery (as `FileChosen`/`OpenFile`/`SecondInstance` events) are
v1 platform-shell features (P3.6); tray is v1.1. A host that
cannot support one returns `UiError.Unsupported`; the app degrades visibly.

- **Packaging:** P8.2a/P8.2b must define `chemical bundle` targets (Linux AppImage/deb,
  Windows MSI/MSIX, macOS `.app`/notarization, WASM static), an asset manifest
  + `resolve_asset`, app icons/licenses, signing, update feed, and crash
  reporting. CWD-relative asset paths are forbidden in bundled apps.

---

## 12. Packet impact summary

P0.2 must fold these into `API.md` and compile every declaration. The minimum
new/changed packets are:

- **P0.1** committed probes + raster prototype + `gh auth`/org probe + font/text
  probes + allocation counting.
- **P0.2** contract compilation, API index, API diff gate.
- **P0.3b** `ui_test_core`, `ui_bench` 3 scenes/baselines, minimal lint,
  `STATUS.json`, stub ledger, macOS CI job.
- **P0.4** `DESIGN.md` exact token sheet + 3 reference screens.
- **P1.5b** font metrics + committed fixture before layout.
- **P1.8** split display list/CPU painter from `RenderBackend` minimal.
- **P1.14** Linux walking skeleton before Stage 2.
- **P2.5a/b/c** per-platform shapers; **P3.2a/b** X11/GTK + Wayland.
- **P4.12** charts (v1, because Dashboard requires them).
- **P5.4a/b** CLI + MCP/relay/DOM bridge.
- **P7.8a/b** Media app + benchmark/readme.
- **P8.2a/b** bundle/installers + final validation.

See `EXECUTION.md` for the authoritative packet list and gates.
