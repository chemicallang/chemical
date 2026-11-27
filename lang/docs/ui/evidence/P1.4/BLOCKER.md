# P1.4 BLOCKER — 10k reconcile gate

> **Historical — resolved by P1.4b.** The optimized reconciler now runs at
> LLVM p50 714 µs / p95 916 µs; this file records the original profile only.

**Status:** BLOCKED on the 10k reconcile gate. The budget was not changed.

Required gate: **LLVM release 10k reorder reconcile p50 ≤1,000 µs**; TCC functional
only. Duplicate/stale/remove/state differential tests and debug/ASAN are green.

## Measured result (5 runs, release_fast)

| Backend | p50 | p95 |
|---|---:|---:|
| LLVM release | **1,852 µs** | **2,228 µs** |
| TCC release | 17,248 µs | 19,769 µs (functional) |

Raw logs: `logs/release_reconcile_llvm.log`, `logs/release_reconcile_tcc.log`.

## Profile / cause

The 10k reorder path does two node-metadata passes per matched child:
`reconcile_children` calls `sync_node`, then `set_children` calls `sync_node` again;
it also builds `retained_ids`, `fresh_ids`, `matched`, and the new-order vectors and
rebuilds the sibling KeyIndex every reconcile. Debug TCC/LLVM and ASAN runs pass;
the differential model and state preservation are correct.

The remaining work is to remove the duplicate sync pass and reuse the order/key
scratch storage, or make the reorder path a specialized fast path. The 1 ms gate
remains in force; no budget amendment was made.
