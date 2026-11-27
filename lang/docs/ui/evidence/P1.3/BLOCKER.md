# P1.3b BLOCKER — release view-build gate

**Status:** BLOCKED. The required release gate is **not met**. No budget change
was made; the gate remains LLVM flat 100k p50 ≤5,000 µs and TCC flat 100k p50
≤10,000 µs.

## Before / after (release_fast)

Parent audit before this packet:

| Backend | Flat 100k | Deep 10k×10 |
|---|---:|---:|
| LLVM | 117,818 µs | 117,400 µs |
| TCC | 209,360 µs | 244,269 µs |

After compact P1.3b node/side-table/style-pool changes, 5 runs (warm-up is the
first run; p50/p95 from the five recorded runs):

| Backend | Flat p50 | Flat p95 | Deep p50 | Deep p95 |
|---|---:|---:|---:|---:|
| LLVM release | **10,190 µs** | **15,601 µs** | **12,729 µs** | **12,874 µs** |
| TCC release | **69,670 µs** | **75,369 µs** | **118,047 µs** | **121,315 µs** |

Raw runs are in `logs/release_view_llvm.log` and
`logs/release_view_tcc.log`.

## What changed

- Base `ViewNode`: **96 bytes**, down from the prior ~500-byte value; style,
  semantics, animation, error, span, grid, overlay, component and other rare
  fields are in side tables.
- Style pooling: the 100k test uses **1 pool entry, 100,000 hits, 0 misses**.
  Default style is the fast path (`style.set_mask == 0`), no field-by-field
  comparison.
- Duplicate-key probing is now **debug-builder-only** (`new_view_builder_debug`);
  the release builder has no `KeySet` work. Structural duplicate tests still run
  in debug mode.
- Frame-local text remains in `FrameArena`; node/span/style arrays are reserved
  at construction; no per-node `malloc` path after warm-up.

## Profile (same release binary)

| Probe | LLVM | TCC |
|---|---:|---:|
| `NodeArena.insert` only, 100k | 648 µs | 4,579 µs |
| `fragment` add only, 100k | 6,771 µs | 14,365 µs |
| `text` only, 100k | 10,830 µs | 35,261 µs |
| `text` + `child(root)`, 100k | 14,060 µs | 63,162 µs |

The base add path already exceeds the 5 ms LLVM gate before style/text work:
the remaining cost is `ViewBuilder.add_node` / compact-node append and generated
call overhead, not `NodeArena` alone (arena insert is only 648 µs of the 7.2 ms).
The TCC generated-C path is additionally 2.3–5× slower on the same operations;
TCC add-only (16.6 ms) already exceeds its 10 ms gate.

Style-by-value adds roughly 4.7 ms LLVM / 20.2 ms TCC over add-only, and
`child(root)` adds another ~0.6 ms LLVM / 29 ms TCC.

## Required next action

P1.3b remains BLOCKED pending a profile-guided lowering of the builder hot path:
remove/replace the heavy by-value `Style` argument copy or specialize the
frame-builder append path, and reduce TCC-generated call/append overhead. Do not
raise the gate or claim P1.3b done.

**Reproduce:**

```bash
./scripts/ui-test.sh --llvm --release --filter view   # expected exit 1, BLOCKER line
./scripts/ui-test.sh --tcc --release --filter view    # expected exit 1, BLOCKER line
```

Debug structural `view` tests remain green; ASAN/UBSAN debug remains green.
