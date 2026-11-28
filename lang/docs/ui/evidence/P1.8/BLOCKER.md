# P1.8 BLOCKER — 10k command-build gate

**Status:** BLOCKED. The 0.3 ms budget was not changed.

LLVM release_fast, 5 runs:

| Scene | p50 | p95 | Gate |
|---|---:|---:|---:|
| 10k display commands | **2,291 µs** | **5,494 µs** | ≤300 µs |

TCC release is functional only (15,566 µs p50 / 19,017 µs p95).

Functional command order, culling, transform/clip/layer nesting, Painter
recording, determinism and ASAN are green.

## Proposed parent amendment (not applied)

The measured result is **2,291 µs p50 / 5,494 µs p95**. With the PaintProps
cache and command path settled, the realistic gate is **3 ms p50 / 7 ms p95**
on LLVM release; TCC remains functional only. This requires parent approval and
was not silently changed.

## Profile / proposed optimization

- Flattening and per-node `style_resolve` dominate; then each node constructs a
  large `DisplayCommand` and pushes into a vector.
- Next work: cache compact paint properties per node/theme epoch, split the
  command into a fixed-size packed record + side tables, pre-size the command
  buffer, and add a fixed-background fast path that avoids the full resolved
  style. None of these changes a budget or public API.
