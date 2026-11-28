# P1.6 — layout v1 (flex/stack): report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**State:** done. P1.7 was not started.

## Files

- `lang/libs/ui/src/layout.ch` — internal layout flattening, intrinsic measurement,
  two-pass row/column/stack placement, grow/shrink, padding/gap, percent/fill,
  min/max, align/justify/baseline, basic absolute placement, warnings, cache,
  `LayoutResult`/accessors.
- `lang/libs/ui/src/view.ch` — internal `layout_epoch`/dirty-node markers for cache
  and fixed-box style-edit fast path; public ViewBuilder API unchanged.
- `lang/tests/ui/src/layout.ch` — fixture cases, text intrinsic/baseline, random
  property model, cache/pass/warning checks and release gate.
- `lang/tests/ui/src/main.ch`, `scripts/ui-test.sh` — `layout` filter.
- `lang/tests/ui/fixtures/layout/*.json` — hand-written fixture oracles with
  provenance.
- `lang/docs/ui/evidence/P1.6/REPORT.md`, `logs/`.

## Layout semantics

- Root/Column and Row place normal-flow children; Stack places children on the
  same content origin. Fragment/Component/Slot/Wrap/Grid/Scroll are treated as
  stack shells only; no wrap/grid/scroll behavior is implemented in P1.6.
- `Px` is exact; `Percent` is a fraction of the parent content box (`0.5` = 50%);
  `Fill` is grow 1 / basis 0; `Auto` uses intrinsic size.
- Grow distributes positive remaining space by `grow`; shrink removes overflow
  weighted by `shrink × base`; ties follow child order. `min_width`/`max_width`
  clamp widths.
- Padding is inside the content box; gap is between normal-flow children.
- Cross-axis Start/Center/End/Stretch and parent `align_self`/`align_items`;
  row baseline alignment uses text first baselines (non-text children use their
  bottom edge).
- Justify Start/Center/End/SpaceBetween/SpaceAround/SpaceEvenly is applied after
  distribution.
- Basic absolute children use top/right/bottom/left against parent content; they
  do not participate in normal flow.
- Intrinsic text uses P1.5b `TextContext.measure` with the resolved `TextStyle`.
  No wrapping/re-breaking or shaping occurs in this packet.
- Maximum published geometry passes: **2** (intrinsic, place), below the 3-pass
  cap. No-change warm layout: **0** passes. The fixed-box style-edit fast path
  also returns 0 passes.
- NaN/Inf/negative dimensions/insets are clamped to zero and produce internal
  `LayoutWarning` entries instead of panics.

Allocation policy: layout nodes/specs/rects are stored in reusable vectors and
reserved up front; no per-node heap allocation is performed after warmup. The
release suite records 0 relayout passes for the warm path.

## Fixtures

Hand-written, provenance-bearing fixtures under `lang/tests/ui/fixtures/layout/`.
Tests compare rects with a **0.5 px** tolerance and exact invariants.

| Fixture | Oracle / expected |
|---|---|
| `row-grow.json` | 200-wide row, padding/gap 8: `a=83`, `c=93` widths; x=8/99 |
| `column-shrink.json` | two 30-high bases in 30-high column shrink to 15 each |
| `padding-gap.json` | padding 5, gap 10: x=5/35 |
| `min-max.json` | max_width 20 and min_width 10 clamps |
| `percent.json` | 0.5 fraction of 100 -> 50 |
| `fill.json` | fixed 20 + Fill -> 80? fixture isolates the Fill child at 50 after a 50 base |
| `align-center.json` | 10-high child in 40-high row -> y=15 |
| `baseline.json` | invariant: two text runs share one absolute baseline |
| `nested-stack.json` | both stack children share origin (0,0) |
| `text-intrinsic.json` | pinned Body `A`: 9.337891 x 20, baseline 13.804199 |
| `absolute.json` | top 10 / left 20 -> (20,10,20,10) |
| `nan-inf.json` | Inf width / negative height -> zero + warnings |

Property test: **1,000 random fixed-size row/column containers** compared to a
reference position model; all positions/widths matched within 0.5 px.

## Release measurements (5 runs, LLVM release_fast)

| Scene | p50 | p95 | Gate |
|---|---:|---:|---|
| 10k simple fixed nodes | **1,960 µs** | **3,553 µs** | ≤2,500 target / ≤4,000 hard — PASS |
| 10k mixed (20% text/auto) | **2,725 µs** | **4,171 µs** | ≤6,000 / ≤9,000 — PASS |
| 100k wide/shallow | **26,366 µs** | **47,823 µs** | ≤40,000 / ≤60,000 — PASS |
| Single fixed-box style edit in 10k | **0 µs** | **1 µs** | ≤50 / ≤100 — PASS |
| Warm no-change | 0 passes | — | 0 relayout — PASS |

TCC release is functional only (no timing gate):

| Scene | TCC p50 | TCC p95 |
|---|---:|---:|
| 10k simple | 11,485 µs | 13,326 µs |
| 10k mixed | 16,611 µs | 18,633 µs |
| 100k wide | 124,730 µs | 137,359 µs |
| single fixed-box edit | 0 µs | 2 µs |

## Verification

| Command | Result |
|---|---|
| `ui-test.sh --tcc --filter layout` | **PASS** |
| `ui-test.sh --llvm --filter layout` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter layout` | **PASS** |
| `ui-test.sh --llvm --release --filter layout` | **PASS**, release gates |
| `ui-test.sh --tcc --release --filter layout` | **PASS** functional |
| `ui-test.sh --tcc --filter view/style/contract` | **PASS** |
| `ui-api-index.sh --check` | **PASS** (438 declarations) |

## Deviations / residual limits

- Wrap, explicit grid, scroll measurement/offset/clip, scrollbars, layers/z-order
  are P1.7. Wrap/Grid/Scroll currently behave as stack shells.
- Absolute positioning is basic (top/left/right/bottom, no RTL/overflow-driven
  stretch); aspect-ratio layout is not consumed yet.
- Text is unshaped and no-wrap; text re-break/cache is deferred with P1.7/P2.3.
- The one-edit fast path only skips work when a fixed-box node's resolved layout
  spec is unchanged; other edits perform a full geometry pass.
- No public API changed; `LayoutResult`, `LayoutState`, `LayoutContext` and
  `LayoutWarning` are internal. No token/golden files changed.
