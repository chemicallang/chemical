# P1.7b — layout-v2 hot-path optimization: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**State:** **blocked**. Functional tests are green; release gates remain missed.
P1.8 was not started. Budgets were not changed.

## Before / after (LLVM release, 5 runs)

| Scene | Before p50/p95 | After p50/p95 | Gate |
|---|---:|---:|---:|
| 10k wrap mixed | 16,241 / 17,959 µs | **7,673 / 10,172 µs** | ≤6,000 / ≤9,000 — FAIL |
| 10k grid | 5,956 / 10,671 µs | **6,063 / 9,342 µs** | ≤4,000 / ≤6,000 — FAIL |
| 100k scroll shell | 74,139 / 76,406 µs | **72,662 / 74,724 µs** | ≤40,000 / ≤60,000 — FAIL |
| 1k scroll viewport | 466 µs | **493 µs** | ≤1,000 — PASS |
| scroll offset update | 13 µs | **14 µs** | ≤500 — PASS |

TCC release functional: wrap **44,437 µs**, grid **20,864 µs**, scroll 100k
**155,082 µs**, scroll 1k **1,508 µs**, update **42 µs**.

## Changes

- Wrap: replaced per-line rescans with line-record build + one placement pass;
  line width/height/grow/shrink/count/justify/extra records are reused within
  the call; cross sizes are resolved once while building records.
- Grid: skips the Auto child scan entirely when every column/row is Px/Fr;
  track/FR resolution is ordered Auto then Fr; spans use bounded index scans.
- Scroll: single-child extent fast path; scrollbar geometry is not recomputed
  when neither axis is visible; offset refresh remains a no-full-layout path.
- `GridTracks` remain frame-owned; no public API changed.

## Correctness

- `ui-test --tcc/--llvm --filter layout_v2` — PASS.
- `ui-test --llvm --sanitize=address,undefined --filter layout_v2` — PASS.
- `ui-test --tcc --release --filter layout_v2` — PASS functional.
- Wrap/grid/scroll property, fixtures, nested scroll, anchors, clamp, layers,
  P1.6 layout, view/style/contract — PASS.
- `ui-api-index --check` — PASS (438 declarations).

## Blockers

Wrap is still 1.28× over the p50 target and 1.13× over the p95 hard ceiling;
grid is 1.52× over p50 / 1.56× over p95; scroll 100k is 1.82× over p50 /
1.25× over p95. Profile and next optimizations remain in
`evidence/P1.7/BLOCKER.md`. Budgets remain unchanged.
