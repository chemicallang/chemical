> **Superseded by P1.9c.** The AA gate is now green through analytic coverage.
> The P1.9b helper and timing findings remain valid; see `../P1.9c/REPORT.md`.

# P1.9b — C SIMD raster spans: report

**Date:** 2026-11-28 · **Branch:** `feature/ui-v1` · **State:** helper green,
AA release gate still measured as blocked. No commit; P1.10 not started.

## 1. What landed

| File | Change |
|---|---|
| `lang/libs/ui/src/raster_simd.c` | In-tree, allocation-free C helper: clear, raw fill, solid/alpha span blend, rounded-rect coverage/AA, damage copy. SSE2 span/coverage path on x86-64; portable scalar fallback (`__TINYC__` and non-SSE2). Source **16,928 B**; compiled release object **10,224 B**. |
| `lang/libs/ui/chemical.mod` | `link c "src/raster_simd.c"`. |
| `lang/libs/ui/src/raster.ch` | Validates `ColorBuffer` size/stride/clip before C calls; uses C for clear, fill, solid rect, rounded-rect fill and damage copy; `Rasterizer.new_scalar` keeps the Chemical scalar path selectable. Text/image/icon/path/gradient/shadow remain counted no-ops. |
| `compiler/Codegen.cpp` | Release `link c` objects now compile with `-O2` (debug C remains `-O0`). Before this, clang was compiling the helper at `-O0`, which alone cost ~20×. This is a compiler-side build-flag fix, no public API change. |
| `lang/tests/ui/src/raster.ch` | Adds C-vs-Chemical pixel equality test on the torture scene; existing 500 fuzz/clip/seam/tile/clamp tests retained. |

The C helper performs no allocation. Its 2-D rounded-corner coverage uses the
same 16×16 sample count as `raster.ch`; a small static direct-mapped coverage
cache stores exact pure results for repeated fractional geometry (P3.9 can
give each raster worker its own cache later). The Chemical wrapper validates
`width/height/stride/pixel-size` and all clip/device rectangles before any C
call; the C entry points also reject invalid spans defensively.

## 2. Correctness

- Torture PNGs remain **byte-identical**; no golden was regenerated:
  - `torture_320x240.png` — `80a2a72bbd100fffc40061c29c228b8df547b5d722fef7fd3b48f5aea9b51f37`
  - `torture_640x480.png` — `0bad2f026d12fc7cd88c9e418cdb6d3286dcfe2640ff4ef90af0acce68e9f557`
- `simd_scalar` renders the torture scene through `Rasterizer.new` and
  `Rasterizer.new_scalar` and compares every pixel; both are identical.
- 500-case random rounded-rect/clip/radius fuzz passes.
- Clip boundary, adjacent-rect seam, full-vs-two-tile rounded/border seam,
  NaN/negative/off-surface clamps, and no-command-outside-clip tests pass.
- TCC (`raster_tcc.log`), LLVM (`raster_llvm.log`) and LLVM
  ASAN+UBSAN (`raster_asan.log`) pass with no sanitizer report.
- `ui-api-index --check`: 438 declarations, unchanged.

## 3. Release timing — LLVM, 5 runs

The test takes 5 repetitions inside each run; table values are that run's
internal p50/p95 in µs. Raw logs: `logs/raster_release_llvm_run1.log` … `run5.log`.

| Run | 10k solid p50/p95 | 10k AA p50/p95 | 1080p clear+fill p50/p95 | 200×120 damage p50/p95 |
|---|---:|---:|---:|---:|
| 1 | 936 / 1,665 | 35,509 / 35,900 | 306 / 1,166 | 1 / 6 |
| 2 | 917 / 1,338 | 35,766 / 36,124 | 293 / 1,001 | 1 / 6 |
| 3 | 938 / 1,485 | 37,392 / 37,538 | 370 / 1,100 | 1 / 8 |
| 4 | 915 / 1,453 | 35,636 / 36,209 | 320 / 1,207 | 1 / 6 |
| 5 | 893 / 1,548 | 35,589 / 36,017 | 313 / 1,150 | 1 / 7 |
| **median** | **917 / 1,485** | **35,636 / 36,124** | **313 / 1,150** | **1 / 6** |
| gate | 1,200 / 2,000 | 2,500 / 6,000 | 1,000 | 1,500 |
| verdict | **PASS** | **MISS** ~6.0× | **p50 PASS**; p95 >1 ms | **PASS** |

TCC release is functional only (no gate): solid **11,099 µs**, AA **362,956 µs**,
clear+fill **6,256 µs**, damage **2 µs** p50 (`raster_release_tcc.log`). TCC uses
the C scalar fallback because TinyCC does not provide the SSE2 path.

## 4. Residual limitations / gate decision

- The only unambiguous gate miss is **AA**: exact 16×16 rounded coverage costs
  **35.6 ms p50**. P0.1 measured 16-sample supersampling at 29.9–58.8 ms for the
  same class of scene, while its 5.1 ms SSE2 figure was for **analytic
  axis-aligned** coverage, not 16×16 rounded corners. The 2.5 ms AA target is
  already a documented miss.
- Solid is now **0.917 / 1.485 ms** median (gate 1.2 / 2.0) and passes. The
  per-run p95 range is 1.34–1.67 ms, so the p95 gate also passes on every run.
- Clear+fill p50 is **0.313 ms**; its p95 is noisy (1.00–1.21 ms) because the
  benchmark's 5 samples cross the 1 ms line under host frequency variation. The
  gate test currently checks p50; the p95 residual is recorded, not hidden.
- Damage is **1 / 8 µs** median, far below 1.5 ms.
- AA is not silently re-budgeted. `BLOCKER.md` proposes the parent decision:
  analytic rounded coverage with an approved golden update (to reach 6 ms), or
  an explicit new 16×16-supersample budget.

P1.10 was not started.
