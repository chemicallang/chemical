# P1.10 — headless host, app loop, PNG: report

**Date:** 2026-11-28 · **Branch:** `feature/ui-v1` · **State:** green.
No commit; P1.11 not started.

## 1. Pipeline

`lang/libs/ui/src/app.ch` now provides the P1.10 public app surface:

- `Application`, `CloseAction`, `Options.make()` with limits/defaults,
  `FrameInfo`/`Viewport`, `UiCtx`, `FrameStats`, `Screenshot`.
- `run_headless(app, options) -> Result<Unit, UiError>`.
- `run_headless_shot(app, options, out_path) -> Result<Screenshot, UiError>`
  for the deterministic shot path used by `ui-shot.sh`; the public `run_headless`
  entry has no PNG side effect.

The loop is:

1. Validate options (`width/height`, scale, font scale, limits, max frames).
2. Load the pinned font path if supplied; otherwise use the empty text context.
3. Build a logical viewport, fixed virtual clock, light/dark/high-contrast
   theme, and fixed theme mode.
4. `on_start` → reset view builder → `view` → `reconcile` → `layout` →
   `display_build` → CPU raster → optional PNG.
5. Call `update` with a synthetic deterministic tick, then honour
   `request_view`/`request_quit`; repeat up to `max_frames` (default one frame
   for headless so there is no hidden loop).
6. `on_close`; no sleeps, no wall-clock reads in the scene, no input callbacks.

`ui_host` now has `run_with` (headless only) and a minimal `run` argv entry;
window/native presentation remains P1.14/P3.x. `UiCtx` exposes
`request_view`, `invalidate`, `frame`, `viewport`, `scale`, `text_context`,
`theme`, `set_theme`, `request_frame`, `request_quit`, `now_ms`, and `stats`.
Expected failures return typed `UiError`; no panic/unwrap/unreachable was added.

PNG output goes through `image`/`raster_save_png`; `scripts/ui-shot.sh` now
compiles the small in-tree shot app, renders the built-in 3-node fixture, and
writes a real PNG. It no longer exits 78. `STUBS.md` was updated.

## 2. PNG fixtures and visual review

Committed under `lang/tests/ui/fixtures/headless/`:

| Artifact | Size | SHA-256 |
|---|---:|---|
| `scene_3node.png` | 256,283 B | `938cbea4a7ee1ecb95a51441dcd947f0ac70309a4b74d27efc5a04cedeb5a1a6` |
| `scene_1k.png` | 2,458,493 B | `3aaea5f64e8e60a798d4306a607257bb09a87f5b4a08be514cae86955b3fe8a0` |

`SHA256SUMS` contains both entries. The tests compare **bytes exactly**; no
tolerance was used. `scene_3node.png` is 320×200; `scene_1k.png` is 960×640.

**Visual review (inspected at full size):** the 3-node scene is a light surface
with three rounded blocks — blue 120×80, red 90×60, green 160×50 — laid out
left-to-right at the top with clean rounded corners and no fringes. The 1k
scene is the same theme with a 48-column wrap of 1,000 16×10 tiles: blue/light
secondary alternation, consistent gaps, a straight grid edge, and the rest of
the 960×640 surface unchanged. There are no seams, clipped tile artifacts,
unexpected alpha changes, or missing raster regions. Text is deliberately not
rasterized (P2.2), so the pinned font affects layout metrics only; the fixtures
contain no fake glyphs.

`ui-shot.sh --headless --theme light --scale 1 --out /tmp/ui-shot-final.png`
produces exactly the 3-node fixture hash. Dark 2× produced a 640×400 PNG with
the same scene and dark token colors.

## 3. Correctness

- Headless `--filter headless` TCC, LLVM, and LLVM ASAN+UBSAN: PASS.
- 3-node and 1k exact PNG fixtures: PASS; deterministic two-run bytes/hash:
  PASS.
- Start/view/layout/display/raster stats: `commands > 0`, 4+ nodes,
  `paint_bytes == w*h*4`.
- Invalid width, missing pinned font, and app `view` failure return typed
  `UiError` (`InvalidArgument`, `Font`, `Layout`): PASS.
- Max frames, synthetic update tick, state update and `request_quit`: PASS.
- Scale 2 renders the expected physical 320×200 surface for a 160×100 logical
  viewport: PASS.
- P1.6 layout, P1.7 layout_v2, view, style, theme, display, raster and contract
  regressions: PASS on TCC/LLVM as applicable.
- `ui-api-index --check`: 438 declarations, unchanged.
- `ui-verify`: self-test and P1.10 packet validation PASS.

## 4. Performance

LLVM release, `--filter headless` (5 internal runs per scenario; includes font
load, view/layout/display/raster, and PNG encode):

| Scenario | p50 | p95 |
|---|---:|---:|
| 3-node, 960×640 | **86.70 ms** | **86.82 ms** |
| 1k tile scene, 960×640 | **90.15 ms** | **90.29 ms** |

TCC release is functional: 3-node **531.31 ms**, 1k **549.64 ms** p50.
The 220 ms figure is the P1.14 GTK window budget; it does not apply to this
headless path. This is a measured first-frame/shot number, not a cold-process
claim. A separate `/usr/bin/time -v` run of the shot binary (320×200, TCC debug
build) reported wall **0.26 s** and max RSS **27,416 KiB**; PSS was not measured
and is not claimed against the 12 MB empty-window budget.

## 5. Files

- `lang/libs/ui/src/app.ch`
- `lang/libs/ui/src/theme.ch` (Shadow storage packed into primitive elevation
  lanes to avoid a C declaration-order bug; `theme.elevation()` API unchanged)
- `lang/libs/ui/src/geometry.ch` (Shadow declaration moved earlier)
- `lang/libs/ui_host/src/host.ch`
- `lang/tests/ui/src/headless.ch`, `main.ch`, `chemical.mod`
- `lang/tests/ui/shot/{chemical.mod,src/main.ch}`
- `lang/tests/ui/fixtures/headless/{scene_3node.png,scene_1k.png,SHA256SUMS}`
- `scripts/ui-shot.sh`, `scripts/ui-test.sh`, `lang/docs/ui/STUBS.md`
- `lang/docs/ui/evidence/P1.10/REPORT.md` + `logs/`

## 6. Limitations

- No window, input, focus, IME, semantics, multi-window, or native present;
  those are P1.11/P1.14/P3.x.
- Headless has no event source. `update` receives a synthetic deterministic
  tick; P1.11 supplies real events.
- Text/image/icon/path/gradient/shadow raster commands remain deferred to
  P2.2/P2.3/P2.7/P2.8/P2.9; the pinned font drives layout only.
- `UiCtx.text_context` is a non-owning empty context in this checkpoint because
  the runtime owns the pinned `TextContext` in the layout context; apps that
  need a borrowed owned text context get the production harness in P6.1.
- `Options.theme_override` is exposed and honored (`Option<Theme>`); the theme
  mode and explicit `UiCtx.set_theme` paths are also implemented.
- PSS/cold-process startup is not a gate for headless; the measured RSS is
  recorded separately and not presented as the 12 MB GTK budget.
