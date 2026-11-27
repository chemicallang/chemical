# P1.9 — CPU rasterizer v1: report

**Date:** 2026-11-28 · **Branch:** `feature/ui-v1` · **Host:** Linux x86-64,
AMD Ryzen 5 7430U (12T), CPU scaling at ~39% during the measured run.
**State:** functional/golden work complete; release performance is **blocked**
on the original raster gates (see `BLOCKER.md`). P1.10 was not started.

## 1. What landed

| File | Contents |
|---|---|
| `lang/libs/ui/src/raster.ch` | `ColorBuffer` (premultiplied RGBA8, stride, bounds checks, clear/fill/copy), `Rasterizer` over `DisplayList`, `RasterStats`, integer `div255` blend, 16×16 rounded-corner coverage, exact rect coverage, clip intersection, translation/scale transforms, layer/style opacity, damage copy, PNG save through `image`. |
| `lang/tests/ui/src/raster.ch` | Deterministic 1×/2× torture scenes + byte-exact goldens, 500-case random rounded-rect/clip/radius fuzz, adjacent-rect seam test, clip test, NaN/negative/off-surface clamp test, release performance probes. |
| `lang/tests/ui/fixtures/raster/` | `torture_320x240.png`, `torture_640x480.png`, `SHA256SUMS`. |
| `lang/libs/ui/chemical.mod` | Adds `image`/`path` imports for PNG goldens. No public API change. |
| `lang/tests/ui/chemical.mod`, `main.ch`, `scripts/ui-test.sh` | `raster` filter and release wiring. |

The rasterizer is internal (`ui_internal`); the frozen `Surface`/`Painter` API
is unchanged. There is no host, present, window, rasterizer public API, glyph
atlas, or text shaping in this packet.

## 2. Contract behavior implemented

- Buffer is **premultiplied RGBA8**. Source `Color` is premultiplied by its
  effective alpha before blending; the destination is
  `src + div255(dst * (255 - src_a))`, with integer `div255` and 8-bit clamps.
- Pixel centers are half-open: a rect covering `[x0,x1)` writes integer pixels
  whose centers are in the interval; adjacent rects at `x=16` have no gap and
  no double-covered seam pixel.
- Solid rects use exact coverage. Rounded rects/borders use 16×16 subpixel
  coverage near corners and exact interior coverage. Border coverage is
  `outer - inner`.
- Clip is intersected in absolute device coordinates before bounds are
  converted to integer pixel spans. A clipped command with an empty span is
  counted as clipped and writes nothing.
- `Transform2D` supports translation and scale (axis-aligned). A non-axis-
  aligned transform is skipped and counted; rotation is a P2.9 item.
- `LayerPush`/`LayerPop` and command opacity are reflected through each command's
  effective opacity; painter clip commands are represented by each command's
  resolved clip.
- Text, image, icon, path, gradient, and shadow commands are **not faked**:
  they increment `RasterStats.skipped` and emit no pixels until P2.2/P2.3/
  P2.7/P2.9.
- `RasterStats` exposes `commands/drawn/skipped/clipped/pixels` for tests and
  frame stats.
- `Rasterizer.render_tile(list, tile, clear)` applies an absolute-device tile
  scissor; `render` uses the whole surface as one tile. Adjacent tiles keep the
  same per-pixel coverage but split the clip, so rounded edges crossing a tile
  boundary are not double-blended and do not leave a seam. Parallel tile
  scheduling/frame pacing remains P3.9. The seam test compares a full render
  against two 16×16 tiles of a rounded/bordered scene.

## 3. Torture scene and visual review

Both PNGs are committed and are compared **byte-for-byte** by the test (not
only by hash). The 2× image is authored at doubled device coordinates, not a
post-resize of the 1× image.

| Artifact | Size | SHA-256 |
|---|---:|---|
| `torture_320x240.png` | 307,528 B | `80a2a72bbd100fffc40061c29c228b8df547b5d722fef7fd3b48f5aea9b51f37` |
| `torture_640x480.png` | 1,229,438 B | `0bad2f026d12fc7cd88c9e418cdb6d3286dcfe2640ff4ef90af0acce68e9f557` |

`SHA256SUMS` contains both entries. Hashes are x86-64 deterministic; macOS/ARM
golden reproduction is **BLOCKED** by the absence of that runner.

**Visual inspection (1× and 2×):** the scene shows a dark slate 24/32/48
background; an opaque red 120×70 block at (12,12); a translucent blue
rounded-rect (radius 16) at (150,18); a translucent green 200×100 block at
(20,100); a yellow block cropped exactly to the nested clip
`(30,110,120,70)`; a half-opacity white rounded-rect (radius 24) overlapping the
green/blue region; a magenta 60×24 block shifted by the translation transform;
and a 2px white rounded border at x≈300, partly outside the canvas. The 2×
image is the same scene at 640×480: quarter-circle corners are smooth, the
clipped yellow edge is straight and at the expected device boundary, the
translucent overlaps have no dark fringe or seam, and the off-surface border is
clipped cleanly. No text/glyph pixels or fake image/icon placeholders appear.

## 4. Functional verification

All commands run from the repo root; logs are in `evidence/P1.9/logs/`.

```bash
./scripts/ui-test.sh --tcc --filter raster                  # PASS
./scripts/ui-test.sh --llvm --filter raster                 # PASS
./scripts/ui-test.sh --llvm --sanitize=address,undefined --filter raster  # PASS
./scripts/ui-test.sh --tcc --release --filter raster        # PASS (functional; no timing gate)
./scripts/ui-test.sh --llvm --filter display                # PASS
./scripts/ui-test.sh --tcc --filter display                 # PASS
./scripts/ui-test.sh --llvm --filter layout_v2              # PASS
./scripts/ui-api-index.sh --check                           # PASS, 438 declarations
```

- Golden mode compares generated PNGs with committed PNG bytes; `--update-goldens`
  regenerated both fixtures and `SHA256SUMS` after the final 2× correction.
- Fuzz: 500 deterministic random rounded rects with random radius/alpha/clip;
  ASAN+UBSAN report no OOB, no use-after-free, no undefined arithmetic.
- Clip: red rect clipped to `[0,16)` and blue rect starting at `x=16`; pixel
  `(15,8)` is red, `(16,8)` is blue, and no blue leaks left of the clip.
- Seam: shared boundary at `x=16` has opaque coverage on both sides with no
  transparent/background gap.
- Edge clamp: an off-surface negative rect, a NaN-opacity command, and a
  translated-off-surface command render without panic or out-of-bounds writes;
  the in-bounds NaN-opacity command leaves the cleared pixel untouched.
- `RasterStats.skipped` observes text/image/icon/path/gradient/shadow commands;
  no fake glyphs or images are emitted.

## 5. Release performance (LLVM, 5 runs)

`./scripts/ui-test.sh --llvm --release --filter raster` was run 5 times
(one build, four `--no-build` runs). Values are **microseconds per frame**;
the test's internal p50/p95 is over 5 repetitions inside each run.

| Run | 10k solid p50/p95 | 10k AA p50/p95 | 1080p clear+fill p50/p95 | 200×120 damage p50/p95 |
|---|---:|---:|---:|---:|
| 1 | 43,475 / 43,507 | 82,051 / 82,417 | 39,719 / 40,017 | 343 / 351 |
| 2 | 43,358 / 43,572 | 82,088 / 83,390 | 39,720 / 39,744 | 341 / 347 |
| 3 | 43,518 / 43,977 | 82,226 / 85,275 | 39,901 / 40,124 | 340 / 351 |
| 4 | 43,502 / 43,670 | 82,101 / 82,823 | 39,964 / 40,268 | 343 / 366 |
| 5 | 43,880 / 44,878 | 83,362 / 83,893 | 40,132 / 40,373 | 347 / 352 |
| **median** | **43,502 / 43,670** | **82,101 / 83,390** | **39,901 / 40,124** | **343 / 351** |
| budget | 1,200 / 2,000 | 2,500 / 6,000 | 1,000 | 1,500 |
| verdict | **MISS** ~36× | **MISS** ~33× | **MISS** ~40× | **PASS** |

TCC release is functional only (no timing gate): solid 221,291 µs, AA
6,845,519 µs, clear+fill 131,978 µs, damage 1,988 µs (p50), all from
`logs/raster_release_tcc.log`. The TCC AA number is 6.85 s and is recorded for
completeness, not treated as a frame budget.

The original budgets in `BENCHMARKS.md` were **not** edited. The measured
failure and proposed correction are in `BLOCKER.md`.

## 6. Known limitations / follow-ups

- No C SIMD helper was added in this checkpoint; the scalar Chemical fallback
  is the measured path. P0.1 already measured that 10k AA quads are impossible
  in the 1.2 ms/2.5 ms budgets with scalar C (7.3–14.8 ms; SSE2 interior
  4.3–5.1 ms). The optional `link c` span/AA helper is the proposed P1.9b path.
- Rounded rect/border coverage is exact for axis-aligned 16×16 sampling, but
  border corners are an approximation of the outer-minus-inner shape.
- Rotation and skew are skipped (counted); only translation/scale are rasterized.
- Tile scissoring exists, but tile scheduling/parallelism and a damage-region
  rasterization API are P3.9. The 200×120 copy is a scalar damage blit and
  meets its gate.
- Text/image/icon/path/gradient/shadow commands are deliberate no-ops with
  counters. P2.2/P2.3/P2.7/P2.8/P2.9 own them.
- Goldens are exact on this x86-64 host. macOS/ARM deterministic goldens remain
  unimplemented/BLOCKED.
