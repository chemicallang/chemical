# Chemical UI — host, present, input, and platform contract

**Status:** normative, v1 · **Date:** 2026-11-27
**Companions:** [`CONTRACTS.md`](CONTRACTS.md) §1/§9, [`API.md`](API.md) §1,
[`EXECUTION.md`](EXECUTION.md) P3.1–P3.9.

The core is pure Chemical. Hosts own only windowing, surfaces, input, IME,
clipboard, present, native menus/dialogs/notifications, and a11y bridges. No host
may contain layout/widget/text logic.

---

## 1. Units and coordinates

- Layout, hit testing, style, events: **logical points** (`float`).
- Surfaces, rasterization, OS input: **physical pixels** (`i32`).
- Exactly one conversion boundary: `physical = floor(logical * scale)` (and
  `scale = physical / logical` from the OS). Input divides/rounds at the same
  boundary. Never mix units in one API.
- Rect convention: half-open `[x, x+w) × [y, y+h)`; pixel centres at +0.5.
- `Viewport.size` is logical; `Surface.width/height` physical; `stride` bytes.

---

## 2. Presenter / surface ABI — FROZEN

```chemical
public struct SurfaceDesc {
    var width : u32          // physical
    var height : u32
    var stride : u32         // bytes per row, >= width*bpp
    var format : SurfaceFormat
}

public enum SurfaceFormat {
    Bgra8Premul,             // Win32 DIB, X11 XShm, Cocoa BGRA
    Rgba8Premul,             // Wayland wl_shm, WASM ImageData
    Rgba8SrgbPremul          // optional GPU readback
}

public interface Presenter {
    // OS-backed buffer; valid until present/next acquire. Do not retain.
    func acquire(&mut self, desc : SurfaceDesc) : Result<Surface, UiError>
    func present(&mut self, damage : &[Rect]) : Result<std::Unit, UiError>
    func resize(&mut self, width : u32, height : u32, scale : float) : Result<std::Unit, UiError>
    func request_frame(&mut self)        // maps to queue_draw / InvalidateRect / setNeedsDisplay / rAF
    func set_cursor(&mut self, c : Cursor)
    func set_ime_rect(&mut self, r : Rect)
}
```

Rules:

- `acquire` returns a writable OS-backed buffer (XShmImage, `wl_shm` pool,
  `CreateDIBSection`, `CGContext`/`CALayer` backing, WASM `ImageData`). The
  rasterizer writes directly into it; no extra full-frame copy on the CPU path.
- Double-buffer: two full-size surfaces (front/back) plus damage tracking. A
  1080p RGBA8 surface is 8.3 MB; two is 16.6 MB, so the **empty-window PSS budget
  is measured at 960×640** (2.46 MB each), not 1080p. 4K uses a GPU/persistent
  scene texture; document the memory.
- `present(damage)` presents only the damaged rects where the platform supports
  it; otherwise it copies the full buffer and says so in `HostCaps`.
- `request_frame` is the only wake path. It must not spin. Idle means no timer,
  no rAF, no repaint.
- A host that cannot provide partial present reports `HostCaps.partial_present =
  false`; the runtime then uses full-surface damage policy.
- On resize/DPI change: drain input, recreate buffers, deliver `Resize`/
  `DpiChange`, request one view + full repaint. Swapchain out-of-date/occluded/
  minimized states are explicit; no present while minimized.
- On close: deliver `on_close`; if vetoed, keep the window; if allowed, stop
  input, cancel timers/animations/async image/font work, drain protocol, destroy
  surfaces, then exit. No callbacks after shutdown.

`HostCaps { partial_present, vsync, multi_window, ime, clipboard_images,
drop_files, native_menus, notifications, a11y }` is exposed through `app.info`;
apps degrade explicitly.

---

## 3. Per-platform mechanisms (v1)

| Platform | Window | Surface/present | Frame trigger | Input | IME |
|----------|--------|-----------------|---------------|-------|-----|
| Linux GTK3 | `GtkWindow` + `GtkDrawingArea` | `cairo_image_surface_create_for_data` on the drawing area's draw signal; XShm on X11 if used | `gtk_widget_queue_draw_area` | GDK events + raw XInput | `GtkIMContext` |
| Linux raw X11/Wayland | XCB/xdg-shell + `wl_shm` | XShm / `wl_shm` buffer + damage | `XSendEvent`/`wl_surface_damage_buffer` + frame callback | XInput2/libinput | `zwp_text_input_v3`/GTK fallback |
| Windows | Win32 `HWND` | `CreateDIBSection` + `BitBlt`/`StretchDIBits` | `InvalidateRect`/`WM_PAINT` | `WM_POINTER`/`WM_INPUT` | `WM_IME_*` + Imm32 |
| macOS | `NSWindow` + `NSView`/`CAMetalLayer` | `CGContext`/`CALayer.contents` (CPU) or Metal drawable (GPU) | `setNeedsDisplay`/`CVDisplayLink` | `NSEvent`/trackpad | `NSTextInputClient` |
| WASM | `<canvas>` + DOM overlay | `ImageData`/`putImageData` (damage rects) or WebGPU texture | `requestAnimationFrame` | pointer/key/wheel/textarea | hidden textarea composition |
| headless | none | memory buffer | manual frame | synthetic | mock |

`lang/libs/window` is not sufficient as-is: it blocks in `window_run`, has one
global loop, a 1 ms async pump, and no present/damage. P3.1/P3.2 either extend it
with a per-window session API or add a UI-specific host. The old functions may
remain for compatibility; the UI host must not use `window_run` as its frame
loop.

---

## 4. Input contract

- Deliver normalized `KeyCode`, not platform VK/keyval. Full UTF-8 text is
  delivered as `TextInput`/`ImeCommit`; never truncate to one byte.
- Pointer: physical → logical at the boundary; capture on press; hover/leave;
  pressure/tilt/pointer kind; wheel dx/dy + phase (Begin/Update/End/Inertia).
- Drag/drop: begin/over/leave/drop with payload (files/text/internal record),
  coordinates, modifiers; file paths are owned UTF-8 (platform conversion in the
  host).
- Keyboard: repeat flag, modifiers (shift/ctrl/alt/meta/caps), dead keys, layout;
  no synthetic text from key codes.
- IME: preedit/commit/cancel, caret/anchor, replacement range, candidate rect;
  host sets the candidate rect; no lost/duplicated characters.
- Clipboard: text now; HTML/image/file-list MIME in v1.1. Large/async paste
  posts a `UserEvent`.
- Timers/animations: virtual-clock driven; host wake only via `request_frame`.

---

## 5. Threading

- UI thread owns `Application`, tree, layout, paint, raster, present. These are
  `!Send + !Sync`.
- Worker pool only for decode/hash/download; results are owned `UserEvent`s via
  `EventSender`; the UI thread drains.
- Debug protocol thread parses bounded commands, then enqueues them to the UI
  thread. No tree/surface access off-thread. Snapshot/screenshot run on the UI
  thread.
- If a render thread is added, it consumes an immutable `Send` display-list
  snapshot + resources; it never touches app/tree. v1 is single-threaded.
- Shutdown: stop input → cancel timers/animations/workers → drain protocol →
  join workers (bounded) → destroy surfaces → exit. No use-after-free on close.

---

## 6. Accessibility bridge

- The host consumes incremental `SemanticsUpdate` values from the core:
  node add/remove/update, role/name/value/states/actions, bounds, relations.
- Linux: AT-SPI2 via D-Bus (`libatspi` C shim); Windows: UIA COM shim; macOS:
  `NSAccessibility`; WASM: DOM overlay with ARIA nodes positioned over the
  canvas.
- Actions (press/focus/set_value/scroll_into_view) are validated against the
  current semantic node; stale IDs are rejected, not redirected.
- Secure text is redacted. Live regions/announcements and modal state are
  required (see CONTRACTS §7/API §8).

---

## 7. WASM host ABI (to add in P3.7)

Current `env` imports are DOM-only. Add scalar imports:

| Import | Signature | Purpose |
|--------|-----------|---------|
| `chc_canvas_create` | `(w,h) -> u32` | offscreen/main canvas |
| `chc_canvas_present` | `(ptr,w,h,stride,x,y,dw,dh)` | damaged RGBA8 region |
| `chc_canvas_resize` | `(w,h,scale) -> ()` | DPI/resize |
| `chc_raf` | `() -> ()` | request animation frame |
| `chc_pointer` | `(kind,x,y,button,mods,pointer_id) -> ()` | pointer/touch |
| `chc_key` | `(kind,code,mods,repeat) -> ()` | key |
| `chc_text` | `(ptr,len) -> ()` | UTF-8 text |
| `chc_ime` | `(kind,ptr,len,caret,anchor,rx,ry,rw,rh) -> ()` | preedit/commit/candidate |
| `chc_clipboard_get/set` | `() -> ptr/len` / `(ptr,len)` | text |
| `chc_dpi` | `() -> f64` | scale |
| `chc_a11y_update` | `(ptr,len)` | DOM semantics overlay JSON |
| `chc_font_fetch` | `(ptr,len) -> ()` | async font |

All pointers are linear-memory offsets with explicit lengths. No by-value
aggregates cross the boundary. `host.mjs` owns the ImageData copy and DOM
overlay. Browser text is a separate backend (TEXT.md §5.1); WASM goldens are
tolerance-only.

---

## 8. macOS specifics

- `window` has no macOS backend today. P3.4 adds a Cocoa host (preferred) or a
  `window` backend; `link c` with `.m` must be verified in P0.1. If `.m` cannot
  be compiled, use a `.c` shim through the Objective-C runtime.
- Main-thread UI only; app menu/Quit/Preferences; `applicationShouldTerminate`
  maps to `on_close` (veto supported); `NSTextInputClient` for IME; `NSEvent`
  for input; `NSView` layer backing/CGImage for CPU present; Metal for GPU.
- App bundle/Info.plist/assets resolve via `resolve_asset`, not CWD.

---

## 9. Host tests

- Headless host implements every interface and passes the full suite.
- Real-window smoke per platform: create, present a known frame, resize, DPI,
  input, IME, clipboard, close-veto, multi-window, device loss (GPU).
- Xvfb + llvmpipe, Weston headless, Windows WARP, macOS CPU, Chromium
  SwiftShader; real hardware nightly.
- Idle test: 60 s, zero timer/rAF wakeups beyond the OS event wait; verify with
  `strace`/powertop/ETW.
- Shutdown test: close during update/paint/timer/protocol request; ASAN/TSAN.
