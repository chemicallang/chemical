# P1.9 — raster performance BLOCKER

**Status:** functional/golden P1.9 is green; the original **release raster gates**
are not met by the scalar Chemical path. The budgets remain unchanged.

## Measured result (LLVM release, 5 runs)

| Scenario | Original gate (p50 / p95) | P1.9 median (p50 / p95) | Verdict |
|---|---:|---:|---|
| 10k solid/opaque quads (≤16×8) | 1.2 / 2.0 ms | **43.50 / 43.67 ms** | MISS (~36×) |
| 10k AA quads (≤16×8) | 2.5 / 6.0 ms | **82.10 / 83.39 ms** | MISS (~33×) |
| 1080p clear + opaque fill | 1.0 ms | **39.90 / 40.12 ms** | MISS (~40×) |
| 200×120 damage blit | 1.5 ms | **0.343 / 0.351 ms** | PASS |

Raw runs: `logs/raster_release_llvm_run1.log` … `run5.log`.
TCC release is functional only: solid 221,291 µs, AA 6,845,519 µs,
clear+fill 131,978 µs, damage 1,988 µs (`logs/raster_release_tcc.log`).

## Why the scalar path misses

The current rasterizer is correctness-first scalar Chemical:

- every pixel goes through safe `ColorBuffer.pixel`/`put` bounds checks;
- every blend performs integer divide-by-255 arithmetic and opacity work;
- rounded edges use 16×16 coverage sampling for edge pixels;
- there is no span batching, SIMD, or opaque-pixel fast path;
- the test invokes the renderer from Chemical with per-command dispatch.

The 1080p clear+fill figure is the clearest signal: even a full-buffer opaque
operation is ~40 ms in this path, so this is not an algorithm-budget dispute;
it is missing hot-loop work. P0.1's standalone C probe measured the necessary
scale for the same scenarios: clear 0.13–0.16 ms, fill 0.30–0.38 ms, 10k solid
1.0–1.9 ms, 10k AA 7.3–14.8 ms scalar and 4.3–5.1 ms SSE2. P0.1 also recorded
that the original 1.2 ms AA gate is impossible scalar and needs a span/SIMD
renderer; see `../P0.1/REPORT.md` §4. P1.9 did **not** add that helper, so the
measured scalar fallback is far above even the P0.1 scalar reference.

## Proposed parent decision (no silent budget change)

Preferred: **split performance into P1.9b** and keep the functional/golden P1.9
result:

1. Add a small `link c` SIMD helper for fill and span/AA hot loops, with the
   current Chemical scalar path retained as fallback. The helper must link and
   run under both TCC and LLVM and must not add a third-party runtime.
2. Keep the current damage gate (P1.9 already passes it: ≤0.351 ms p95).
3. If the helper reaches the P0.1 measurements, use those measured, stated
   budgets rather than the impossible scalar target:
   - 10k solid: ≤2.0 ms p95 (1.2 ms target only on a boosted CPU);
   - 10k AA: ≤15 ms scalar / ≤6 ms SSE2; 2.5 ms only with AVX2 + span;
   - 1080p clear+fill: ≤1 ms;
   - damage: ≤1.5 ms.
4. If the SSE2 helper lands at P0.1's ~8.3 ms AA, amend the AA gate to
   **9 ms p50 / 12 ms p95** only after parent approval. Do not apply this
   amendment silently.

If the parent does not authorize P1.9b/C SIMD, the alternative is an explicit
new scalar gate (for example solid ≤50 ms p50, AA ≤90 ms p50, clear+fill
≤45 ms p50). That is **not recommended**: it would accept a ~35–45× regression
against the stated paint budgets and would not meet the “extreme performance”
language goal. In either case P1.9 stays `blocked` on performance until a
decision is recorded.

## Not done

- P1.10 (host/app loop/present/window) was not started.
- No text shaping, glyph atlas, pixels for text/image/icon/path/gradient/shadow,
  parallel tile scheduling, or rotation rasterization was added.
