# P1.14 — Linux walking skeleton (real GTK3 window): report

**Date:** 2026-11-28 · **Branch:** `feature/ui-v1` · **State:** green (real window works).
No commit; P2.1 not started.

## 1. Host implementation

`lang/libs/ui_native/src/gtk.ch` creates a real GTK3 window through the existing
`window::Window` safe handle, adds a `GtkDrawingArea`, and drives the same
`ViewBuilder → reconcile → layout_run → display_build → Rasterizer` pipeline
used by P1.10. The draw callback converts the raster's premultiplied RGBA8
pixels to BGRA8 and presents them through an ARGB32 cairo image surface.

Frame flow:

```text
Window.new (logical options size)
  → native_handle() = GtkWidget*
  → gtk_drawing_area_new + gtk_container_add
  → draw signal
     → app.view/update on the GTK thread
     → layout/display/raster into ColorBuffer
     → premultiplied RGBA8 → BGRA8
     → cairo_image_surface_create_for_data(ARGB32)
     → cairo_set_source_surface + cairo_paint
     → optional cairo_surface_write_to_png
  → gtk_widget_queue_draw on input/resize
```

Input is connected both through `window::Window.on_event/on_resize/on_close`
and GTK event signals on the drawing area/toplevel. `window::Event` values are
mapped to the P1.11 `UiEvent` shape (`Press`/`Release`/`PointerMove`/`Scroll`/
`KeyDown`/`KeyUp`), then `Application.update` runs on the GTK thread. Escape
quits; close destroys/quits the loop; resize updates the allocation and
reinitializes the logical viewport/buffer. There is **no 1 ms pump** in this
host: draws happen only for expose/input/resize/test-plan timers.

`ui_host::run_with` selects this host when `Options.headless == false`; the
headless path remains unchanged. No display/GTK produces typed
`UiError.Platform`. The real-window test is guarded by `DISPLAY`/
`WAYLAND_DISPLAY` and reports `BLOCKED` without them.

## 2. GTK / cairo externs

`lang/libs/ui_native/src/gtk.ch` declares only the symbols it calls; no GTK
headers were available or used:

| API | Symbols |
|---|---|
| GTK/new | `gtk_init_check`, `gtk_drawing_area_new`, `gtk_container_add`, `gtk_widget_set_hexpand`, `gtk_widget_set_vexpand`, `gtk_widget_queue_draw`, `gtk_widget_get_allocated_width`, `gtk_widget_get_allocated_height`, `gtk_window_resize` |
| GTK/window reuse | `gtk_widget_show_all`, `gtk_widget_get_window`, `gtk_widget_add_events`, `g_signal_connect_data`, `g_timeout_add` (through `window`) |
| GDK test | `gdk_test_simulate_button` |
| cairo | `cairo_image_surface_create_for_data`, `cairo_set_source_surface`, `cairo_paint`, `cairo_surface_flush`, `cairo_surface_destroy`, `cairo_surface_write_to_png` |
| test timing | libc `clock` |

`lang/libs/ui_native/chemical.mod` adds:

```text
import window
link ":libcairo.so.2" if linux
```

The existing window library supplies GTK/GDK/glib/gobject links. No new
production dependency is introduced beyond cairo (the required present target).

## 3. Measurements

Measured on the real window under the existing `DISPLAY=:0` session. Startup is
`clock()` from before `gtk_init_check` through the first draw/present. PSS is
`VmRSS` at first present. Idle CPU is process CPU time from first present to the
test-plan quit; the three frames are first-present, click, and resize.

| Build | start→first present | PSS | frames | idle CPU (520 ms) |
|---|---:|---:|---:|---:|
| TCC debug | 284 µs | 75,560 KB | 3 | 108.910 ms |
| LLVM debug | 250 µs | 76,004 KB | 3 | 85.101 ms |
| TCC release | 310 µs | 75,000 KB | 3 | 115.508 ms |
| LLVM release | 187 µs | 74,800 KB | 4 | 59.217 ms |
| LLVM ASAN/UBSAN | 549 µs | 210,740 KB | 3 | 88.438 ms |

The 220 ms startup / 12 MB PSS gates are **not claimed**. The measured PSS is
dominated by GTK/pango/fontconfig and is recorded as-is. Idle has no periodic
app frame pump; the remaining CPU is GTK's own loop/compositor work.

## 4. Screenshots

The test plan saves the exact cairo surface after `cairo_paint` using
`cairo_surface_write_to_png` inside the real draw callback. This is the cursor
surface presented to GTK; it is not an external compositor crop.

| File | SHA-256 | Review |
|---|---|---|
| `before.png` | `9e81c6e015d5310fabd382ff52fa8fea0542c9cde3b4a5af543474ccea6d2f20` | 640×480 physical surface; red 160×80 button at top-left on light surface |
| `after_click.png` | `15de3fd94020ce4f1a9145bd40402a37f4c30c997bfe4c19be431e9f0d5ba8b0` | same size; button is blue after the queued press/release toggle |
| `after_resize.png` | `169db810c7248a5edc23131188a1d94aa6806980ca152263fc6f31fb7b9f0252` | resized 420×300 surface; blue button remains toggled |

The logical request was 320×240; GTK's scale factor produces a 640×480
physical first surface, which is what the rasterizer sees. No text is drawn;
P2.2 owns glyph rasterization.

## 5. Input and resize

- The unit test `gtk_tests::mapping` checks `window::Event` → `UiEvent`
  normalization, scale division, and GDK Escape (`0xff1b`) mapping without
  opening a window.
- The real smoke test reads the saved surfaces and asserts click dimensions are
  unchanged and the resize surface is smaller; `app.clicks == 1` and
  `toggled == true`.
- `GDK_BACKEND=wayland` returned `gdk_test_simulate_button == 0`; under X11 it
  returned 1 without delivering a routed GTK signal. The deterministic test plan
  therefore uses a test-only direct event injection through the same host
  `gtk_host_event` queue; the GTK area/toplevel handlers are installed for real
  user input. This is the only automation workaround in the packet.
- Esc is handled in `gtk_host_event`; close is handled by
  `Window.on_close`/`window_quit`; resize is an actual `gtk_window_resize` and
  drives a new allocation, viewport, buffer and draw.

## 6. Tests

PASS:

- `ui-test --tcc --filter gtk` (real window + unit mapping)
- `ui-test --llvm --filter gtk`
- `ui-test --llvm --sanitize=address,undefined --filter gtk` with
  `ASAN_OPTIONS=detect_leaks=0` (GTK/pango/fontconfig leak reports are external;
  no host-code ASAN/UBSAN finding)
- `ui-test --tcc --release --filter gtk`
- `ui-test --llvm --release --filter gtk`
- `ui-test --tcc --filter gtk` with no display → BLOCKED, green
- P1.10 headless, P1.11 input, P1.9 raster, P1.12 semantics, P1.13 errors TCC regressions
- `ui-lint` 0 findings; `ui-api-index --check` 438 declarations

Logs are in `lang/docs/ui/evidence/P1.14/logs/`.

## 7. Files

- `lang/libs/ui_native/src/gtk.ch` (new GTK/cairo host)
- `lang/libs/ui_native/chemical.mod` (window import, cairo link)
- `lang/libs/ui_host/src/host.ch`, `chemical.mod` (host selection/fallback)
- `lang/libs/ui/src/app.ch` (`ui_ctx_new` and accessors for the host)
- `lang/tests/ui/src/gtk.ch` (unit mapping + real-window smoke)
- `lang/tests/ui/src/main.ch`, `chemical.mod`, `scripts/ui-test.sh`
- `lang/tests/ui/fixtures/gtk/{before,after_click,after_resize}.png` + `SHA256SUMS`
- `lang/docs/ui/evidence/P1.14/REPORT.md` + `logs/`

## 8. Limitations

- No text/glyph path (P2.2); no menus, IME, multi-window, or full `UiHarness`.
- The screenshot is the presented cairo surface, not a compositor/window-manager
  crop. A display with no X/Wayland (or broken GTK) returns typed Platform and
  the test is BLOCKED.
- GDK test input was not reliable on this Wayland/Xwayland session, so automated
  click proof uses a test-only direct queue injection; real GTK input handlers
  are connected but cannot be claimed as automated end-to-end input proof.
- The GTK window scales 320×240 logical to a 640×480 physical surface here; the
  host currently renders at physical allocation and reports actual dimensions.
- PSS is ~75 MB, not the 12 MB goal. No 220 ms/12 MB gate claim.
- No damage-region present path; each draw repaints the full surface.
