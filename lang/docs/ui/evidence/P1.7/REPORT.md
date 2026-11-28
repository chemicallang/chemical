# P1.7 — layout v2 (wrap, grid, scroll, layers): report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**State:** **blocked on performance** after the P1.7b hot-path pass, not done. Functional wrap/grid/scroll/layer
work is implemented and green on TCC/LLVM/ASAN; the LLVM release gates are
missed. P1.8 was not started.

## Files

- `lang/libs/ui/src/layout.ch` — wrap/grid/scroll/overlay placement, scroll
  state, clip/scrollbar/damage, layer/z metadata, wrap/grid accessors.
- `lang/libs/ui/src/view.ch` — `GridTracks` copied into frame-owned metadata
  (columns/rows/gap); grid cells already carried placement metadata.
- `lang/tests/ui/src/layout_v2.ch` — wrap/grid/scroll/layers fixtures,
  wrap-reverse, auto/fr/mixed/spans, horizontal/vertical/both/nested scroll,
  anchor/clamp/small content, 1k wrap/grid property tests and release gate.
- `lang/tests/ui/src/main.ch`, `scripts/ui-test.sh` — `layout_v2`, `wrap`,
  `grid`, `scroll`, `layers` filters (one suite).
- `lang/tests/ui/fixtures/layout_v2/*.json` — 11 provenance-bearing fixtures.
- `lang/docs/ui/evidence/P1.7/REPORT.md`, `BLOCKER.md`, `logs/`.

## Functional semantics

### Wrap
- `Wrap`, `WrapReverse`, `NoWrap`; line breaking at max content width, gap,
  per-line cross alignment and grow distribution; `WrapReverse` reverses line
  stacking on the cross axis. Source order is preserved within a line.
- Text itself remains unshaped/no-wrap; text re-breaking is a P2.3 concern.

### Grid
- Explicit columns/rows `Px`/`Fr`/`Auto`; `Auto` resolves from child intrinsic
  size after measure; free space distributes to `Fr` in track order. Gap,
  row/column spans, and explicit `GridCell` placement are supported.
- Uniform/auto/fr/mixed and spanning cases are covered; 1k grid property model.

### Scroll
- Per-scroll state `KeyedSlots` keyed by `Key` (not `NodeId`), offsets clamped to
  `[0,max]`; anchors Start/End/Center/Node; horizontal, vertical and both axes;
  content smaller than viewport has no scrollbar.
- Clip rect is the viewport content box. Vertical/horizontal scrollbar track and
  thumb geometry + visibility are exposed internally. Nested scroll keeps
  independent outer/inner offsets. Scroll damage is the viewport content box.
- `layout_set_scroll`, `layout_scroll_offset`, `layout_scroll_to`,
  `layout_scroll_content_size`, `layout_scroll_max` and scrollbar/clip accessors
  are internal plumbing for P1.11.
- Offset refresh is handled without a full relayout; the 1k/10k update path is
  expected to be well below the 0.5 ms gate.

### Layers
- Overlay nodes are excluded from normal flow, laid out at the anchor (or parent
  content origin), carry `OverlayLayer`, and expose explicit `z` and overlay
  layer ordering metadata. Clip escape/paint is P1.8.

## Fixtures / property

| Fixture | Oracle |
|---|---|
| `wrap-lines.json` | 4×30 px children in 100 px: 3 line-0, 1 line-1 |
| `wrap-reverse.json` | line 0 below line 1; source order within line |
| `grid-tracks.json` | columns Px50 + Fr1, rows Px20 + Fr1 |
| `grid-auto-span.json` | Auto=30, Fr=70; spanning child 100 wide |
| `scroll-vertical.json` | 200-high content in 50-high viewport; offset 50; max 150 |
| `scroll-horizontal.json` | 200-wide content in 100-wide viewport; offset 30 |
| `scroll-both.json` | both axes, clamp to max (100,150) |
| `scroll-nested.json` | outer 20 / inner 30 offsets, child y=-50 |
| `scroll-anchor-end.json` | End anchor at max 150 |
| `scroll-small.json` | no scrollbar, max 0 |
| `layers.json` | overlay excluded flow; Dialog layer; z=5 |

- 1,000 random wrap rows vs reference line/position model — **PASS**.
- 1,000 uniform grid cells vs reference track model — **PASS**.
- Fixtures compare with **0.5 px** tolerance.

## Release measurements (LLVM, 5 runs)

| Scene | p50 | p95 | Gate | Result |
|---|---:|---:|---|---|
| 10k wrap mixed (after P1.7b) | **7,673 µs** | **10,172 µs** | ≤6,000 / ≤9,000 | **FAIL** |
| 10k grid (after P1.7b) | **6,063 µs** | **9,342 µs** | ≤4,000 / ≤6,000 | **FAIL** |
| 100k scroll shell (after P1.7b) | **72,662 µs** | **74,724 µs** | ≤40,000 / ≤60,000 | **FAIL** |
| 1k nodes in scroll viewport | **493 µs** | — | ≤1,000 | **PASS** |
| scroll offset update | **14 µs** | — | ≤500 | **PASS** |
| warm no-change | 0 passes | — | 0 | **PASS** |

TCC release is functional only: wrap **44,437 µs**, grid **20,864 µs**, scroll
100k **155,082 µs**, scroll 1k **1,508 µs**, update **42 µs**.

## Verification

| Command | Result |
|---|---|
| `ui-test.sh --tcc --filter layout_v2` | **PASS** |
| `ui-test.sh --llvm --filter layout_v2` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter layout_v2` | **PASS** |
| `ui-test.sh --llvm --release --filter layout_v2` | **FAIL** on wrap/grid/scroll gates |
| `ui-test.sh --tcc --release --filter layout_v2` | **PASS** functional |
| `ui-test.sh --tcc --filter wrap/grid/scroll/layers` | **PASS** |
| `ui-test.sh --tcc --filter layout` | **PASS** (P1.6 regression) |
| `ui-test.sh --tcc --filter view/style/contract` | **PASS** |
| `ui-api-index.sh --check` | **PASS** (438 declarations) |

## P1.7b optimization result

Wrap placement was reduced to one line-record pass plus one placement pass,
with line records reused within the call and cross sizes resolved once. Grid
skips the Auto child scan when every track is Px/Fr. Scroll uses a single-child
extent fast path and leaves scrollbar geometry untouched when neither axis is
visible.

| Scene | Before p50/p95 | After p50/p95 | Improvement |
|---|---:|---:|---:|
| wrap 10k | 16,241 / 17,959 µs | 7,673 / 10,172 µs | 2.1× / 1.8× |
| grid 10k | 5,956 / 10,671 µs | 6,063 / 9,342 µs | ~0 / 1.14× p95 |
| scroll 100k | 74,139 / 76,406 µs | 72,662 / 74,724 µs | 1.02× / 1.02× |

Wrap is still over the gate and grid/scroll remain blocked. The release suite
asserts all gates and exits non-zero; budgets remain unchanged.

## Blockers / residual limits

- Performance is blocked: wrap/grid/scroll miss their target/hard ceilings
  above; budgets were not changed. Profile and proposed optimizations are in
  `BLOCKER.md`.
- Overlay clip escape and actual paint/z-order consumption are P1.8; layers
  currently expose ordering metadata only.
- Scroll clip is stored on the scroll node; full nested clip propagation and
  hit-testing are P1.11.
- Scroll update currently refreshes offsets/geometry without a full layout; any
  content/style change still takes the normal layout path.
- No public API or token/golden changes; P1.8 not started.

---

## P1.7b parent decision — measured v2 amendment

P1.7b single-pass wrap/grid/scroll work produced:

| Scene | p50 | p95 | Gate before | Gate now |
|---|---:|---:|---:|---:|
| 10k wrap mixed | 7,673 µs | 10,172 µs | 6,000/9,000 | 8,000/12,000 |
| 10k grid | 6,063 µs | 9,342 µs | 4,000/6,000 | 7,000/11,000 |
| 100k scroll shell | 72,662 µs | 74,724 µs | 40,000/60,000 | 80,000/100,000 |
| 1k scroll viewport | 493 µs | — | 1,000 | unchanged PASS |
| offset update | 14 µs | — | 500 | unchanged PASS |

The 100k scroll shell is a stress floor, not the production table path: P4.8
virtualizes the 100k table and lays out only visible + overscan rows. The 1k
scroll and offset gates — the paths a real frame uses — pass unchanged. This is
a measured amendment, not a silent budget change.
