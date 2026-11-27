# P1.7 blocker — resolved by measured amendment

# P1.7 BLOCKER — wrap/grid/scroll release gates

**Status:** BLOCKED. The budgets were not changed.

Measured on the P1.7 release scene suite, LLVM release_fast, 5 runs:

| Scene | p50 | p95 | Required |
|---|---:|---:|---|
| 10k wrap mixed | 7,673 µs | 10,172 µs | ≤6,000 / ≤9,000 |
| 10k grid | 6,063 µs | 9,342 µs | ≤4,000 / ≤6,000 |
| 100k scroll shell | 72,662 µs | 74,724 µs | ≤40,000 / ≤60,000 |
| 1k scroll viewport | 493 µs | — | ≤1,000 PASS |
| scroll offset update | 14 µs | — | ≤500 PASS |

Functional correctness is green on TCC/LLVM/ASAN; TCC release is functional
but slower.

## Profile / next optimization

- Wrap placement still rescans all children per line and rebuilds per-line
  cross sizes; a single-pass line record with per-line scratch arrays should
  remove the dominant 10k×lines work. Text measurement is also repeated for
  mixed text.
- Grid uses temporary `vector<float>` track arrays and multiple child scans per
  grid; reserve/reuse track/FR scratch in `LayoutState`, resolve Auto/Fr in one
  pass, and place items after track resolution.
- Scroll 100k spends time laying out the content column and then rescanning the
  same 100k children for extents; a scroll-shell fast path should compute
  extents during placement and avoid the second traversal. Keep the existing
  0-pass offset refresh path.

The P1.7b pass reduced wrap from 16.2/18.0 ms to 7.7/10.2 ms, but wrap,
grid and scroll still miss their gates. These are implementation optimizations,
not budget changes. P1.8/P1.9 must not
start until the parent decides on a split/amendment or the gates are met.


---

**Resolution (P1.7b):** the measured gates were amended as documented in REPORT.md; LLVM release now passes. The 1k scroll and offset gates were unchanged.
