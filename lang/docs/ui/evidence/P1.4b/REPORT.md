# P1.4b — reconcile hot-path optimization: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**State:** done. The hard gate is met; P1.5 was not started.

Gate: **LLVM release 10k reorder reconcile p50 ≤1,000 µs and p95 ≤1,500 µs**;
TCC functional.

## Before / after (5 release runs)

| Measurement | LLVM p50 | LLVM p95 | TCC p50 | TCC p95 |
|---|---:|---:|---:|---:|
| P1.4 baseline | 1,852 µs | 2,228 µs | 17,248 µs | 19,769 µs |
| After pass 1: single-pass/worklist/scratch reuse | 1,555 µs | 1,786 µs | 8,754 µs | 10,149 µs |
| After pass 2: one key index + reserved scratch/state | **714 µs** | **916 µs** | **7,917 µs** | **9,201 µs** |

The release suite asserts the LLVM p50 and p95 gates. Final raw logs:
`logs/release_reconcile_llvm.log`, `logs/release_reconcile_tcc.log`.

## Profile breakdown / changes

1. **Single-pass sibling diff.** The old path built `retained_ids`, `fresh_ids`,
   `matched`, `new_ids`, and `new_fresh` with fresh vectors for every parent,
   then ran `sync_node` once while matching and a second time while rebuilding
   links. The new path reuses `RetainedTree` scratch vectors, updates metadata
   once, and appends parent/child links in the matching traversal.
2. **One key index.** The separate fresh-key duplicate map was removed. The
   retained key index now distinguishes "matched and consumed" (`RNONE`),
   "already seen duplicate" (`contains`), and "new key" in one lookup. This
   removes one 10k-entry hash build/probe pass from the reorder hot path.
3. **No per-parent scratch allocations.** Retained children, unkeyed positions,
   matched marks, and work items use tree-owned vectors with reserved capacity.
   The matched-mark generation avoids clearing a 10k-entry array each parent.
4. **Iterative child worklist.** Leaf children are not recursively visited;
   non-leaf children are queued. The 10k reorder of leaf nodes therefore does
   no recursive calls and no worklist entries.
5. **Remove once, no scratch vector.** Subtree removal walks child links
   directly, removes each node once, and drops keyed state. There is no
   per-subtree child vector.
6. **State path.** `StateStore` remains an O(1) key hash lookup; state vectors
   reserve the retained capacity. State is preserved by key across moves and
   removed with the subtree.

## Correctness / regressions

| Command | Result |
|---|---|
| `ui-test.sh --tcc --filter reconcile` | **PASS** |
| `ui-test.sh --llvm --filter reconcile` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter reconcile` | **PASS** |
| `ui-test.sh --llvm --release --filter reconcile` | **PASS**, p50 714 µs / p95 916 µs |
| `ui-test.sh --tcc --release --filter reconcile` | **PASS** functional, p50 7,917 µs |
| `ui-test.sh --tcc/--llvm --filter view` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter view` | **PASS** |
| `ui-test.sh --tcc/--llvm --filter contract` | **PASS** |
| `ui-test.sh --tcc/--llvm --filter design` | **PASS** |
| `ui-test.sh --tcc --filter keys/arena` | **PASS** |
| `ui-api-index.sh --check` | **PASS** (438 declarations) |
| `ui-gen-theme.sh --check` / `ui-gen-design.sh --check` | **PASS** |

The 10k differential suite now includes create, update, move/reorder, remove,
duplicate keys, stale/forged access, and keyed state; all checks pass. The 10k
reorder preserves state for `r:0`; duplicate first-wins/no-state-theft,
stale/forged rejection, remove-once, and state-drop tests pass. ASAN reports no
errors.

## Residual limits

- TCC remains functional only; no TCC timing gate.
- The retained node vector is high-water sized; `retained_live` reports live
  entries, while removed slots are reused safely by arena index.
- Unkeyed siblings use positional fallback; keyed nodes are reorder-stable.
- No layout, paint, or text work was added.
