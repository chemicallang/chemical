# P1.9c — analytic AA coverage: report

**Date:** 2026-11-28 · **Branch:** `feature/ui-v1` · **State:** green.
No commit; P1.10 not started.

## 1. Algorithm change

Exact 16×16 rounded-curve supersampling was replaced by an **analytic
rounded-box coverage approximation** in both raster paths:

- Solid rectangles keep exact integer coverage.
- Rounded rectangles use the signed-distance form
  `q = max(|p - center| - (half - radius), 0)`, with a linearized coverage
  `0.5 - (q·q - radius²) / (2·radius)` at corners.
- Straight horizontal/vertical edges use the equivalent linear distance
  `0.5 - (q - radius)`.
- The inner core is exactly 255. Coverage is clamped and converted to 8-bit
  without FMA-dependent rounding.
- The same formula is implemented in `lang/libs/ui/src/raster.ch` (Chemical
  scalar fallback) and `lang/libs/ui/src/raster_simd.c` (C scalar/SSE2 helper).
  `Rasterizer.new_scalar` still renders through the Chemical path.
- The rounded-rect C path blends the straight interior span with one constant
  coverage and only evaluates the analytic edge coverage outside it.

C helper: **14,614 B** source, **11,296 B** release object. It allocates nothing.
The P1.9b `-O2` release `link c` compile flag remains in `compiler/Codegen.cpp`.

## 2. Goldens — approved regeneration

The two P1.9b PNGs were regenerated because the coverage algorithm changed.
Both were inspected at 1× and 2×; no golden was changed silently.

| Artifact | Old P1.9b SHA-256 | New P1.9c SHA-256 |
|---|---|---|
| `torture_320x240.png` | `80a2a72bbd100fffc40061c29c228b8df547b5d722fef7fd3b48f5aea9b51f37` | `1d3050fd906f2055a67c58bd485f73b27404ba024a9a44cbfda1477f6b068a70` |
| `torture_640x480.png` | `0bad2f026d12fc7cd88c9e418cdb6d3286dcfe2640ff4ef90af0acce68e9f557` | `735b12d15812fd2648e0bcf8a522ccaabacb0b08c95ac8882130cb1d86a2d3e6` |

`lang/tests/ui/fixtures/raster/SHA256SUMS` was updated; the exact-byte test
compares against the new committed PNGs.

**Visual review:** the scene still has the dark slate field, opaque red block,
translucent blue rounded-rect, translucent green block, clipped yellow block,
half-opacity white rounded-rect, translated magenta block, and partly
off-surface 2px white border. The rounded corners are smooth at both 1× and 2×;
the clipped yellow boundary is exact and straight; the border edge is clean
where it leaves the canvas. There are no background seams, dark/colored fringes,
or unexpected alpha/color changes. The analytic coverage shifts only the
sub-pixel edge values relative to the old 16×16 samples.

## 3. Correctness

- `simd_scalar`: C/SSE2 and Chemical-scalar torture renders compare every pixel
  and are identical under TCC and LLVM.
- 500-case fuzz; clip boundary; adjacent-rect seam; full-vs-two-tile rounded /
  border seam; NaN/negative/off-surface clamps: PASS.
- TCC (`raster_tcc.log`), LLVM (`raster_llvm.log`), and LLVM ASAN+UBSAN
  (`raster_asan.log`): PASS.
- P1.6 layout, P1.7 layout_v2, view, style, display and contract regressions:
  PASS on TCC/LLVM as applicable.
- `ui-api-index --check`: 438 declarations, unchanged.
- `ui-verify`: self-test and P1.9c/P1.9 packet validation PASS.

## 4. Release timing — LLVM, 5 runs

Internal p50/p95 over 5 repetitions per run; medians below.

| Run | Solid 10k p50/p95 | AA 10k p50/p95 | 1080p clear+fill | Damage 200×120 |
|---|---:|---:|---:|---:|
| 1 | 930 / 2,109 | 4,392 / 4,575 | 289 / 315 | 1 / 6 |
| 2 | 1,047 / 1,446 | 4,657 / 5,035 | 333 / 541 | 1 / 6 |
| 3 | 949 / 1,474 | 4,609 / 4,769 | 373 / 982 | 1 / 7 |
| 4 | 1,000 / 1,396 | 4,467 / 4,534 | 397 / 532 | 1 / 12 |
| 5 | 922 / 1,390 | 4,295 / 4,913 | 305 / 813 | 1 / 6 |
| **median** | **949 / 1,446** | **4,467 / 4,769** | **333 / 541** | **1 / 6** |
| gate | 1,200 / 2,000 | 2,500 / 6,000 | 1,000 p50 | 1,500 |
| verdict | **PASS** | **PASS (≤6 ms)**; 2.5 ms target still a documented miss | **PASS** | **PASS** |

The solid p95 on run 1 was 2,109 µs; the other four runs were ≤1,474 µs and the
5-run median is 1,446 µs. The gate test checks p50 (930–1,047 µs) and passes.
AA is 4.30–4.66 ms, below the 6 ms hard gate; the 2.5 ms target remains a
documented miss. Clear+fill p50 is 0.289–0.397 ms and p95 is 0.315–0.982 ms.
Damage is 1 µs p50.

TCC release is functional only (no timing gate): solid **11,161 µs**, AA
**47,814 µs**, clear+fill **5,978 µs**, damage **2 µs** p50
(`raster_release_tcc.log`).

## 5. Residual limits

- Analytic rounded coverage is an SDF/linear-distance approximation, not exact
  area; this is the authorized contract change. The old 16×16 goldens are
  intentionally replaced.
- The 2.5 ms AA target is still missed (4.47 ms median); the hard 6 ms gate is
  met.
- One solid p95 run (2.109 ms) crossed 2.0 ms; the 5-run median p95 passes.
- TCC has no SSE2 path and is functional only.
- Text/image/icon/path/gradient/shadow commands remain counted no-ops.
- P1.10 was not started.
