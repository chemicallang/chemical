# P1.8 — display list and Painter: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**State:** **blocked on the command-build gate**, not done. RenderBackend/damage,
20-scene serialization, command/order, clip/transform/layer and Painter tests are
green on TCC/LLVM/ASAN. P1.9 was not started. No public API was added beyond
API.md.

## Files

- `lang/libs/ui/src/display.ch` — `DisplayList`, ordered `DisplayCommand`
  records, `DisplayState`, layout-tree flattening/emit, culling, clip/transform/
  layer stacks, overlay emission, public `Painter`/`Path`/`Gradient`/
  `CanvasPaint`/guard types, internal accessors.
- `lang/tests/ui/src/display.ch` — command kinds/order/count, style/theme
  resolution smoke, clip/transform/layer stack, text/image/icon/shadow/path/
  gradient/line Painter recording, RenderBackend/damage, 20-scene
  serialization, determinism, release build gate.
- `lang/tests/ui/fixtures/display/*.dl.txt` — 20 committed stable serializations.
- `lang/libs/ui/src/display.ch` — `RenderBackend` interface and recording CPU
  adapter; `display_damage`; `display_serialize`.
- `lang/tests/ui/src/main.ch`, `scripts/ui-test.sh` — `display` filter.
- `lang/docs/ui/evidence/P1.8/REPORT.md`, `BLOCKER.md`, `logs/`.

## Command table

Layout tree emission emits, in paint order:

| Group | Command |
|---|---|
| shadow | `Shadow` (rect/radius/offset/blur/color) |
| background | `Rect` or `RoundedRect` |
| border | `Border` (rect/radius/width/color) |
| content | `Text` (text/style/baseline), `Image`, `Icon` |
| scroll | `ClipPush` / `ClipPop`; scrollbar geometry is metadata (paint is P1.9) |
| transform | `TransformPush` / `TransformPop` with composed 2D transform |
| opacity | `LayerPush` / `LayerPop` with clamped opacity |
| overlay | `LayerPush`/`LayerPop` carrying `OverlayLayer` and explicit `z` |
| Painter | `Rect`, `RoundedRect`, `Border`, `Line`, `Path`, `Gradient`, `Shadow`, `Image`, `Text` + clip/layer/transform push/pop |

The Painter records into the same `DisplayList`; it never rasterizes or mutates
the tree. `ClipGuard`/`LayerGuard`/`TransformGuard` auto-pop on scope exit.

## Transform / clip semantics

- Transform composition is `parent ∘ node` via `Transform2D.multiply`; commands
  carry the composed transform. Culling uses the transformed rect's bounding box.
- Clip is the intersection of the inherited clip and scroll/overflow clips;
  `ClipPush` carries the resulting rect. Overlay nodes escape the ancestor clip
  and emit at the window viewport with layer/z metadata.
- Rect geometry is retained in layout coordinates plus the composed transform;
  no rasterizer/pixel convention is consumed in P1.8.
- NaN/Inf/negative rect/opacity values clamp through the geometry/display helpers.

## Verification

| Command | Result |
|---|---|
| `ui-test --tcc --filter display` | **PASS** |
| `ui-test --llvm --filter display` | **PASS** |
| `ui-test --llvm --sanitize=address,undefined --filter display` | **PASS** |
| `ui-test --llvm --release --filter display` | **FAIL** on 0.3 ms build gate |
| `ui-test --tcc --release --filter display` | **PASS** functional |
| `ui-test --tcc --filter layout/layout_v2/view` | **PASS** |
| `ui-api-index.sh --check` | **PASS** (438 declarations) |

Release measure: **display 10k commands p50 2,291 µs / p95 5,494 µs** (LLVM
release, 5 runs); the original gate is ≤300 µs. TCC release functional is
**15,566 µs p50 / 19,017 µs p95**.

## Blockers / residual limits

- Command build remains over the 0.3 ms gate. `DisplayState` now caches the last
  resolved paint and the measured result is below the proposed **3 ms p50 /
  7 ms p95** amendment; parent approval is required before changing the gate.
  See `BLOCKER.md`; budgets unchanged.
- RenderBackend is a recording CPU adapter only; P1.9 owns rasterization.
- Overlay z/layer metadata is emitted, but actual paint order/z sorting and
  rasterization are P1.8/P1.9 boundaries. No pixels are produced.
- Scrollbar geometry is available from layout but not painted in this packet.
- No public API changes beyond the frozen API; P1.9 not started.

---

## P1.8b parent decision — measured display gate

The 20-scene serialization, `RenderBackend`/`RecordingBackend`, and damage tests
are complete. After `PaintProps` caching the 10k-command build is **2,291 µs p50 /
5,494 µs p95** LLVM release; TCC is functional at 15,566 µs.

The original 0.3 ms was a pre-implementation stretch. Parent amendment:
**LLVM ≤3 ms p50 / ≤7 ms p95; TCC functional**. The 20-scene exact fixtures,
clip/damage invariants, and Painter tests are unchanged. No pixel/raster work.
