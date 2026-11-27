# P1.2 — arena, generational IDs, keys: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**Status:** all P1.2 commands green; P1.2 marked `done`; parent commit/tag pending.

No view tree/reconciler, Style, layout, or renderer was implemented. The P1.2
arena/keys are internal machinery; only the minimal `NodeId` accessors were added
to the public surface.

---

## 1. Files

Added:

- `lang/libs/ui/src/arena.ch` — `NodeId`, internal `NodeArena`, `FrameArena`.
- `lang/libs/ui/src/keys.ch` — `Key`, FNV-1a 64 constructors, bounded debug table,
  internal `KeyScope`/`KeyedSlots`.
- `lang/tests/ui/src/arena.ch` — 1M-operation fuzz, wrap/reuse/clear, frame arena.
- `lang/tests/ui/src/keys.ch` — 10M key check, collisions, duplicate scope,
  debug-table overflow.
- `lang/docs/ui/evidence/P1.2/REPORT.md` + `logs/`.

Modified:

- `lang/docs/ui/API.md` — `NodeId.index/generation/is_valid` only.
- `lang/tests/ui/contracts/src/main.ch` — matching NodeId declarations.
- `lang/docs/ui/ui_api_index.json` — regenerated baseline (438 declarations).
- `lang/tests/ui/src/main.ch` — `arena`/`keys` filters; test-core `NodeId`
  qualified to avoid colliding with the now-real UI handle.
- `scripts/ui-test.sh` — `--filter arena` and `--filter keys`.
- `lang/docs/ui/STATUS.md`, `STATUS.json`, `packets/P1.2.json`.

## 2. NodeArena

- `NodeId` fields are private; `(0,0)` is invalid and generation 0 is never
  issued. Slot indices are 1-based; index 0 is the sentinel.
- `NodeArena.new(max_nodes)` reserves backing storage once. `insert` reuses the
  free list or appends an unallocated slot; `remove` bumps the generation and
  returns the slot to the free list.
- Stale/forged handles: `get`/`contains` return `None`/false; `remove` returns
  `UiError(StaleNode)`.
- At generation `0xFFFFFFFF`, `remove` retires the slot to generation 0, does not
  free it, and returns `UiError(ResourceExhausted)`. The retired slot cannot
  alias. `clear` invalidates all live handles; a later insert bumps generation.
- Capacity is bounded by `max_nodes`; a full arena returns
  `UiError(ResourceExhausted)`. `reserve` checks the bound and pre-reserves the
  backing vectors. No per-node heap growth occurs after `new`/`reserve`.
- `FrameArena` is a bump allocator over a preallocated `vector<u8>`:
  `alloc_offset`/`alloc` align with checked size arithmetic, `slice`/`bytes`
  expose used bytes, `reset` reuses the block while preserving `high_water`, and
  overflow returns `ResourceExhausted`.

## 3. Key

- `Key` stores only `(hash, tag, id)`; labels are not retained in the key.
- `key(label)` = FNV-1a 64 over bytes, tag=0, id=0.
- `key_of(label,id)` = label hash, tag=0, id=id.
- `key_pair(label,a,b)` is order-sensitive: label, `0xFF`, little-endian `a`,
  `0xFF`, little-endian `b`; stores tag=b, id=a.
- `equals(label)` hashes the label; `equals_key` compares the full tuple.
- The debug label table is a fixed `64 × 32-byte` thread-local array. It drops
  entries when full; identity never depends on it and `key()` does no
  correctness allocation.
- `ui_internal::KeyScope` is a chained hash table (bucket `unordered_map<u64,u32>`
  + key/next vectors) with an O(1) average insert/lookup. A second equal key
  returns `UiError(DuplicateKey)` without mutating state; same-hash/different-id
  keys are kept in the same chain and distinguished by `equals_key`.
- `ui_internal::KeyedSlots` uses the same chained table for `i64` state slots.
  Removal is deferred to P1.4 where retained state ownership is defined; P1.2
  only tests insert/update/lookup.
- `ui_internal::key_with_hash` is the test-only same-hash constructor; it is not
  public API and cannot forge a safe `Key` from outside `ui_internal`.

## 4. Results

`arena` fuzz: **1,000,000** randomized insert/remove/lookup operations on an
8192-slot arena, deterministic xorshift64 seed. It checks live-count parity,
stale handles after removal, stale `remove` codes, free-list reuse, and final
drain/leak count. Separately: clear invalidation, generation-wrap retirement, and
frame-arena alignment/overflow/reset/no-growth.

`keys`: **10,000,000** distinct `key_of("row", i)` tuples are generated and
checked (`id == i`, repeat equality); `key_pair` swap is rejected; same-hash
collision keys coexist; duplicate `KeyScope` insertion returns `DuplicateKey`;
128 debug-label insertions leave a 64-entry cap without changing identity.

| Command | Result |
|---|---|
| `ui-test.sh --tcc --filter arena` | **PASS** (`arena ops=1000000 capacity=8192`) |
| `ui-test.sh --llvm --filter arena` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter arena` | **PASS** |
| `ui-test.sh --tcc --filter keys` | **PASS** (`10m distinct=10000000 debug_cap=64`) |
| `ui-test.sh --llvm --filter keys` | **PASS** |
| `ui-test.sh --llvm --sanitize=address,undefined --filter keys` | **PASS** |
| `ui-api-index.sh --check` | **PASS** (438 declarations; 0 duplicates) |
| `ui-test.sh --tcc/--llvm --filter contract` | **PASS** |
| `ui-test.sh --tcc/--llvm --filter design` | **PASS** |
| `ui-gen-theme.sh --check` | **PASS** |
| `ui-gen-design.sh --check` | **PASS** |
| `ui-verify.sh --self-test` | **PASS** |
| `ui-verify.sh --packet P1.2` | **PASS** |

Both instrumented runs are clean under LLVM ASAN/UBSAN.

## 5. API additions

Only `NodeId` gained public accessors: `index()`, `generation()`, `is_valid()`.
`NodeId` construction remains private (`make_internal` is non-public and only the
arena uses it). `Key` already had its frozen API; no public key/arena methods
were added. The internal arena/key-scope types live in `ui_internal` and are not
in `API.md`/`ui_api_index.json`.

## 6. Residual limits

- The debug label table is deliberately lossy and bounded (64 entries × 32 bytes);
  IDs/hashes remain correct after drops.
- `NodeArena` currently stores slot metadata only; P1.3 adds the view-node payload
  and uses this handle shape. `KeyedSlots` removal is deferred to P1.4.
- `FrameArena` and `NodeArena` reserve their backing vectors on construction;
  OOM at construction is still subject to the std vector allocation blocker
  (steady-state operations do not allocate/grow).
- `FrameArena.alloc` returns a raw pointer for the future view builder; callers
  must respect the requested lifetime/alignment. No view tree is built here.
- macOS/Windows CI remain BLOCKED as previously recorded.
