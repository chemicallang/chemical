# WebAssembly target (wasm32-unknown-unknown)

**Status:** first slice shipped (W0–W3) · **Date:** 2026-11-26
**Plan:** [`wasm-target-plan.md`](wasm-target-plan.md)

Chemical has a working **freestanding browser wasm** target. Both compiler
paths produce a runnable `.wasm`:

```
Chemical source
  → MIR
  ├─ LLVM backend  → wasm object → (in-process) wasm-ld → app.wasm
  └─ C emitter     → app.c      → clang -target wasm32 → wasm-ld → app.wasm
```

There is no hosted VM: memory is linear, allocation is malloc/free over
`__heap_base` + `memory.grow`, and the only host surface is a small `env`
import table.

## Quick start

```bash
# LLVM path: whole app, no manual link
cmake-build-debug/Compiler app/chemical.mod \
    --target wasm32-unknown-unknown -o app.wasm \
    --wasm-export start --wasm-export chc_wasm_on_event

# C-emitter path (TCCCompiler only translates; clang + wasm-ld compile/link)
cmake-build-debug/TCCCompiler app/chemical.mod \
    --target wasm32-unknown-unknown --emit-c -o app.c
scripts/wasm-c-build.sh app/chemical.mod app.wasm \
    --export start --export chc_wasm_on_event

# Run in Node (see lang/tests/wasm/run.mjs for the harness)
node lang/tests/wasm/run.mjs app.wasm --expect-rc 0
```

Exports default to `main`. Name more with `--wasm-export NAME` (repeatable) or
export everything with `--wasm-export-all`. `memory` is always exported.
`--wasm-export-all` is a demo/debug convenience: it keeps every defined symbol
(and therefore any unreachable OS-only helper that happens to live in `std`),
so release builds should list their exports explicitly.

The gate for all of the above:

```bash
./scripts/wasm-test.sh                 # W0–W3, Node + Chromium
./scripts/wasm-test.sh --skip-playwright
```

## Profiles

| Profile | Triple | Status |
|---|---|---|
| Browser / freestanding | `wasm32-unknown-unknown` | **Supported (this slice)** |
| WASI | `wasm32-wasi` / `wasm32-wasip1` | W4 backlog |
| wasm64, threads/atomics, SIMD, exceptions | — | W4 backlog / out of scope |

`wasm32` is little-endian, 32-bit, **not** POSIX/Unix/Windows. `if wasm32` in
`chemical.mod` selects the wasm sources; the host POSIX/Windows directories are
never selected for it. `TargetData.posix` is false, `size_t`/`long`/pointers are
32-bit, and `MIRModuleLayout.pointer_size` is 4.

## Runtime and host ABI

`lang/runtime/wasm/runtime.c` is linked automatically by `cstd`
(`link c ... if wasm32` on the LLVM path; explicitly by
`scripts/wasm-c-build.sh` on the C path). It provides:

- first-fit allocator with a coalescing free list (`malloc`, `free`, `realloc`,
  `calloc`) over `__heap_base` + `__builtin_wasm_memory_grow`;
- `memcpy`/`memmove`/`memset`/`memcmp`/`strlen`/`strcmp`/`strncmp`/`strncpy`/
  `strcpy`;
- `abort` (trap), `printf`/`snprintf`/`fwrite`/`fflush`/`putchar`/`puts`
  (minimal `%s %d %u %x %p %c` formatting);
- console bytes go to the host import `env.chc_log(ptr, len)`.

Host imports used by this slice:

| Import | Signature | Purpose |
|---|---|---|
| `env.chc_log` | `(i32 ptr, i32 len) -> ()` | console/stdout/stderr bytes |
| `env.chc_now_millis` | `() -> f64` | `std::now_milli` / time stubs (only if reached) |
| `env.chc_wasm_dom_body` | `() -> i32` | document body handle |
| `env.chc_wasm_dom_create_element` | `(i32,i32) -> i32` | tag pointer/length → handle |
| `env.chc_wasm_dom_set_text` | `(i32,i32,i32)` | handle, text ptr/len |
| `env.chc_wasm_dom_append_child` | `(i32,i32)` | parent, child handles |
| `env.chc_wasm_dom_set_attribute` | `(i32,i32,i32,i32,i32)` | handle, name ptr/len, value ptr/len |
| `env.chc_wasm_dom_add_event_listener` | `(i32,i32)` | handle, event id |

The module exports `chc_wasm_on_event(handle, event_id)`, which dispatches to
the Chemical listener table. Event ids: 1 click, 2 input, 3 change.

DOM handles are `u32`; `0` is null and ids are never reused. The bridge uses a
**scalar ABI only** — no by-value Chemical aggregates cross the boundary — so
the wasm C aggregate ABI (`MIRABIKind::Wasm32`) is not needed yet and remains
fail-closed (`MIRABIKind::Unknown`).

`lang/runtime/wasm/host.mjs` implements the host for Node and the browser;
`lang/runtime/wasm/serve.mjs` is a dependency-free static server for demos.

## Browser library

`lang/libs/browser` is the safe UI surface:

```chemical
public struct Node { /* opaque u32 handle */ }
public func document_body() : Node
public func create_element(tag : string_view) : Node
public func set_text(node : Node, text : string_view)
public func append_child(parent : Node, child : Node)
public func set_attribute(node : Node, name : string_view, value : string_view)
public func add_event_listener(node : Node, event_id : u32, handler : std::function<() => void>)
public func on_click(node : Node, handler : std::function<() => void>)
```

`lang/compiled/wasm_ui` is the proof: a `div` with a button and a counter span,
a `start()` export, a Node DOM-shim test, and a Chromium Playwright test.

## wasm-safe `cstd`/`std` subset

Selected automatically by `source "wasm" if wasm32`:

- `lang/libs/cstd/wasm`: base typedefs (`usize`), stdio sentinels
  (`get_stdout`/`get_stderr`/`get_stdin`), errno access + the POSIX codes `std`
  names (`EINVAL`, `EAGAIN`, …); errno is a no-op and reads 0.
- `lang/libs/std/wasm`: single-threaded `mutex`/`recursive_mutex`/`rwlock`/
  `CondVar` stubs (no-ops; there are no threads), and a clock that reads
  `env.chc_now_millis`.

Works and is tested: `std::string`, `std::string_view`, `std::vector`,
`std::Option`, `std::Result`, `std::unordered_map`, `std::mem`, formatting /
`printf`, `std::Box`, closures via `std::function`, and the `browser` bridge.

**Not part of this profile** (no host OS): files/`fs`, sockets/`net`, processes,
threads/threadpool, TLS/HTTP, `std::time` OS clocks beyond the host millisecond
clock, environment variables, locale, and dynamic loading. The declarations may
exist (so shared code compiles) but a call is only safe if it is not reachable
from an exported root; unreachable functions are removed by `--gc-sections`.
Native builds are unchanged and still select POSIX/Windows as before.

## Limitations / W4 backlog

- WASI (`wasm32-wasi`): files, clock, random, args/environ.
- `MIRABIKind::Wasm32` C aggregate ABI for by-value `@extern` structs and
  `string_view` interop; today the bridge is scalar-only.
- Explicit export lists instead of `--wasm-export-all` for releases; debug
  info/source maps; size/perf measurements and cross-target CI.
- Memory64, SIMD, atomics/threads, exceptions — only on real need.
- Reconcile the wasm `browser` bridge with the JS-only typed `dom`
  (`chx_dom_*`); keep the scalar bridge until a wasm C ABI exists.

## Tests

```bash
./scripts/wasm-test.sh
# W0 freestanding hello (main -> 42) in Node
# W1 std string/vector/map/Option/Result/format in Node
# W2 the same app through TCCCompiler --emit-c + clang + wasm-ld, identical output
# W3 wasm UI in a Node DOM shim and in Chromium via Playwright
```

The wasm suite is separate from `--tcc`/`--llvm`; it needs clang and wasm-ld
(the prebuilt `out/host/bin/wasm-ld` is enough).
