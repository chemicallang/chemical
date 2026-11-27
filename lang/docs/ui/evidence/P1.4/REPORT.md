# P1.4 — reconciler and retained tree: report

> **Historical (superseded by P1.4b).** The original 10k reconcile gate was
> missed at LLVM p50 1,852 µs; P1.4b met it at 714 µs p50 / 916 µs p95. This
> file preserves the initial blocked result and correctness evidence.

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**Status:** correctness/differential/state tests are green; the 10k reorder release
gate is **BLOCKED** at LLVM p50 1,852 µs vs 1,000 µs. P1.4 is not marked done;
see `BLOCKER.md`.

No layout, paint, or text work was added.

## Files

Added:

- `lang/libs/ui/src/reconcile.ch` — internal `RetainedTree`, compact retained
  nodes, `StateStore` (hover/focus/pressed/scroll/text/animation/custom), keyed
  sibling diff, `KeyIndex`, `ReconcileStats`, and internal accessors.
- `lang/tests/ui/src/reconcile.ch` — 10k-operation differential model, 10k
  reorder/state test with release measurement, duplicate/stale/remove/path tests.
- `lang/docs/ui/evidence/P1.4/REPORT.md`, `BLOCKER.md`, `logs/`.

Modified:

- `lang/libs/ui/src/view.ch` — internal `reset_view_builder`; node fast-index
  cleanup used by the reconciler tests.
- `lang/libs/ui/src/arena.ch` — `clear` free-list ordering so a reused builder
  starts at index 1.
- `lang/tests/ui/src/main.ch`, `scripts/ui-test.sh` — `reconcile` filter.
- `STATUS.md`, `STATUS.json`, `packets/P1.4.json` (blocked state).

## Differential / state results

- 10,000 deterministic randomized operations against a reference key sequence:
  create, update, remove, swap/move, reorder, duplicate keys, state updates, stale/forged access. Tree
  child order and keyed custom state matched the model on every operation.
- 10,000-child reorder: all keys preserved, state for key `r:0` preserved across
  repeated reorders, no recreations; `retained_live` matched expected count.
- Duplicate fresh sibling keys return `UiError(DuplicateKey)`; first key wins,
  second is not attached, and first text/state is not stolen.
- Forged/zero `NodeId` returns `None`; removing the subtree drops keyed state;
  a second empty reconcile removes nothing further (`removed_total` stable).
- Unkeyed nodes use positional fallback and have distinct, stable structural
  paths while position is unchanged.

## Commands / results

| Command | Result |
|---|---|
| `ui-test.sh --tcc --filter reconcile` | **PASS** |
| `ui-test.sh --llvm --filter reconcile` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter reconcile` | **PASS** |
| `ui-test.sh --llvm --release --filter reconcile` | **FAIL**, p50 1,852 µs > 1,000 µs |
| `ui-test.sh --tcc --release --filter reconcile` | **PASS** functional |
| `ui-api-index.sh --check` | **PASS** |
| `ui-test.sh --tcc/--llvm --filter contract/design` | **PASS** |
| `ui-test.sh --tcc --filter keys/arena` | **PASS** |
| `ui-gen-theme.sh --check` / `ui-gen-design.sh --check` | **PASS** |

## Residual limits

- 10k reorder reconcile p50 is 1.85 ms LLVM; the gate is 1.0 ms. The measured
  cause and next optimization are in `BLOCKER.md`; TCC is functional only.
- State text uses the retained `StateStore` owned string; no layout/text shaping.
- Retained metadata currently mirrors style id, text, test id, and semantics label;
  layout/paint consumption is later.
- Structural-path semantics for unkeyed nodes are positional and therefore change
  when an unkeyed sibling is inserted/reordered; keyed nodes are key-stable.
- Duplicate-key detection is sibling-scoped in the reconciler; builder-wide debug
  checks remain debug-only.
- macOS/Windows CI remain BLOCKED as previously recorded.
