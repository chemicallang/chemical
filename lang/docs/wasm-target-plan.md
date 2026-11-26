# WebAssembly target plan (LLVM compiler + C emitter)

**Status:** approved for implementation · **Date:** 2026-11-26
**Goal:** make `wasm32-unknown-unknown` a real Chemical target for the LLVM
compiler, make C emitted by `TCCCompiler` compilable by any wasm-capable C
compiler, and prove it with a browser UI demo. The TinyCC backend itself is not
expected to emit wasm; it emits C, and that C is compiled with clang/wasm-ld.

This plan supersedes the "WASM is deferred" decision for the LLVM backend. The
JS backend is unchanged and remains the direct browser-as-JS path; this plan
adds the second browser path.

---

## 0. Verified baseline (2026-11-26)

Facts established before writing this plan:

| Check | Result |
|---|---|
| LLVM prebuilt has wasm | **Yes** — LLVM 22.1.8; `WebAssembly` is in `llvm-config --targets-built`; `liblldWasm.a`, `out/host/bin/wasm-ld` exist |
| LLVM emits wasm IR/object | **Yes** — `--target wasm32-unknown-unknown` emits `target triple = "wasm32-unknown-unknown"` and a valid wasm object |
| Compiler links a `.wasm` | **No** — host ELF flags (`crt1.o`, `-lc`, `-L/lib`, `-rpath`, `--enable-new-dtags`) and `ld.lld` (ELF) instead of `wasm-ld` |
| `std`/`cstd` build for wasm | **No** — no wasm platform layer; `std` fails on `get_stdout`/`get_stderr`/`get_errno` |
| Target data from a wasm triple | **No** — `init_target_data` does not set `wasm32`/`wasm64`; `if wasm32` is false |
| C ABI for wasm | **No** — `MIRABIKind::Unknown`; by-value extern aggregates rejected (`SAFE-FFI-003`) |
| C emitter (`TCCCompiler --emit-c`) | **Emits C today**, and trivial C compiles with clang → wasm-ld → runs in Node |
| Browser test tooling | **Available** — Playwright + Chromium under `lang/compiled/components-e2e`; Node 22 |
| Wasm-capable C compiler | **Available** — `cmake-build-debug/Compiler cc` (clang 22.1.8), plus system clang |
| Docs status | WASM deferred in `proposals/review.md`, `DECISIONS.md`, `language-goals.md`; this plan changes that for LLVM |

Manual proof that codegen is real:

```bash
out/host/bin/wasm-ld --no-entry --export-all object.o -o linked.wasm
node -e '... new WebAssembly.Instance(...) ... exports.main()'   # 42
```

The gap is **target plumbing, linking, runtime, platform layer, and browser host
ABI** — not the LLVM wasm code generator.

---

## 1. Target profiles

| Profile | Triple | Status in this wave | Use |
|---|---|---|---|
| Browser / freestanding | `wasm32-unknown-unknown` | **First-class** | UI library in the browser; DOM via `env` imports |
| WASI | `wasm32-wasi` / `wasm32-wasip1` | Later (W4) | CLI/server wasm, files/clock/random via WASI |
| Wasm64 | `wasm64-unknown-unknown` | Out of scope | later |
| Threads/atomics/SIMD/exceptions | — | Out of scope | later |

The first wave targets **freestanding browser wasm**: linear memory, an
allocator, a panic hook, and a small `env` host ABI. It does not pretend to
provide POSIX, files, sockets, processes, or threads.

---

## 2. Architecture

### 2.1 LLVM path

```
Chemical source
  → MIR
  → LLVM IR (WebAssembly target, already works)
  → wasm object
  → in-process wasm-ld (or out/host/bin/wasm-ld)
  → app.wasm
  → JS host (Node / browser)
```

Required compiler changes:

1. **Target data** (`ast/utils/GlobalFunctions.cpp`, `compiler/lab/TargetData.h`,
   `lang/libs/lab/src/target_data.ch`):
   - map `llvm::Triple::wasm32` / `wasm64` to `TargetData.wasm32` / `wasm64`;
   - set `little_endian = true`, `is64Bit` for wasm64;
   - do **not** mark wasm as `posix`/`isUnix`/`windows`/`linux`/`macos`;
   - accept `wasm32-wasi` and `wasm32-unknown-emscripten` as distinct profiles
     (WASI/emscripten source selection later).
2. **Source selection** (`lang/libs/cstd/chemical.mod`,
   `lang/libs/std/chemical.mod`):
   - `source "wasm" if wasm32`;
   - exclude host platform dirs for wasm: `source "posix" if !windows && !wasm32`,
     `source "win" if windows && !wasm32`, etc.;
   - keep the pure `src` tree shared.
3. **Linker** (`compiler/Codegen.cpp`):
   - `invoke_lld`: choose the `wasm-ld` driver when `triple.isWasm()`;
   - `lld_link_objects`: a wasm branch that does **not** pass host ELF flags;
     pass `--no-entry` (or `--export=...`), `--export-memory`,
     `--allow-undefined`/`--import-undefined` for `env` imports,
     `--stack-first`/`-z stack-size=...`, `--gc-sections`;
   - never pass `-l`/`-L` host libs or `-rpath`/`--enable-new-dtags`;
   - use the in-process `wasm-ld` driver (preferred; the LLD wasm library is
     already linked as `LLD_HAS_DRIVER(wasm)`), with `out/host/bin/wasm-ld` as a
     fallback;
   - output extension `.wasm`; keep `--out-ll-all`/`--out-obj` working.
4. **CLI/output**: `Compiler <app.mod> --target wasm32-unknown-unknown -o app.wasm`
   must build end-to-end without manual linking. Add a `--wasm-export`/mode only
   if needed; default export `main` plus `--export-all` for the UI demo.
5. **ABI**: keep `MIRABIKind::Unknown` fail-closed for by-value extern aggregates
   in this wave. The browser DOM bridge uses scalar `(ptr, len)` parameters, so
   it does not need wasm aggregate ABI. Define `MIRABIKind::Wasm32` in W4 for
   `string_view`/struct interop.
6. **C++ dependency**: hold the line that no backend reads the AST; wasm work
   stays in MIR/target/link layers.

### 2.2 C emitter path (`TCCCompiler`)

`TCCCompiler` does not emit wasm; it emits C. The acceptance is:

```bash
TCCCompiler app.mod --target wasm32-unknown-unknown --emit-c -o app.c
# then, with a wasm-capable C compiler:
clang --target=wasm32-unknown-unknown -nostdlib -c app.c -o app.o
clang --target=wasm32-unknown-unknown -nostdlib -c runtime.c -o runtime.o
wasm-ld --no-entry --export-all app.o runtime.o -o app.wasm
```

Required changes:

1. `TargetData` from the triple (same fix as 2.1.1) so `--target wasm32` selects
   `cstd/wasm`/`std/wasm` and does not select POSIX/Win.
2. `--emit-c` must not try to compile/link with TCC when the target is not
   native; emit C and exit (it already does for a single module; verify
   multi-module apps and the `-o .c` path).
3. Provide `scripts/wasm-c-build.sh` (or a documented two-step command) that:
   - compiles every emitted C file with `Compiler cc`/clang `--target=wasm32`;
   - compiles `lang/runtime/wasm/runtime.c`;
   - links with `out/host/bin/wasm-ld` (or `Compiler cc` with the right
     `-fuse-ld`/`-Wl` flags if it can find wasm-ld);
   - copies/embeds the JS host if needed.
4. Keep the generated C wasm-compatible: `size_t` is 32-bit on wasm32
   (`unsigned long`), no host-only headers, no ELF assumptions. Test a
   multi-module `std` app, not only a single file.
5. Add a C-path parity test: the same source produces the same observable
   behavior through LLVM and through C → clang → wasm.

### 2.3 Runtime C (`lang/runtime/wasm/runtime.c`)

A small freestanding runtime, linked into both paths. It provides the symbols
`cstd`/`std` expect and avoids a host libc/sysroot:

- **Allocator**: `malloc`/`free`/`realloc` over `__heap_base` +
  `__builtin_wasm_memory_grow(0, pages)`; a simple free list (not a bump-only
  allocator), with alignment and a null-return on OOM.
- **Memory/string**: `memcpy`, `memset`, `memmove`, `memcmp`, `strlen`, `strcmp`,
  `strncmp`, `strncpy` (or rely on clang builtins where safe).
- **Console/panic**: `abort` (`__builtin_trap`), `printf`/`snprintf`/`fwrite`/
  `fflush`/`get_stdout`/`get_stderr`/`get_errno` as minimal shims. Console output
  uses the host import `env.chc_log(ptr, len)`; `get_stdout` returns a stable
  sentinel, `get_errno` returns 0.
- **64-bit builtins** if the linker reports them: `__udivdi3`, `__umoddi3`,
  `__divdi3`, `__moddi3` (implement over 32-bit arithmetic) or link a wasm
  compiler-rt builtins archive if one is available. Do not leave unresolved
  `__multi3`/division symbols.
- **No OS APIs**: no files, sockets, processes, threads, clock, or locale.
- Add `link c "runtime.c" if wasm32` to `cstd/chemical.mod` (or the wasm app
  template) so the LLVM build links it automatically. For the C path,
  `scripts/wasm-c-build.sh` compiles it explicitly.

### 2.4 Browser host ABI and JS host

A browser instance is:

```
instantiate(app.wasm, {
  env: { memory_grow, chc_log, chc_abort, chc_wasm_dom_* },
  ...
})
```

Design rules:

- **Scalar ABI**: DOM/string imports take `(ptr: i32, len: i32)` or handle `i32`
  values, never by-value Chemical structs. This avoids the wasm C aggregate ABI
  in the first wave.
- **Handles**: JS keeps a `Map<u32, Node>`; handle 0 means null. Ids are never
  reused, so a stale handle cannot alias a different node.
- **Strings**: read from `memory` with `TextDecoder`; return borrowed bytes by
  copying into wasm only when the API requires an owned string.
- **Events**: JS registers a listener with `(handle, eventId)`; on an event it
  calls the exported `chc_wasm_on_event(handle, eventId)` (or an app-provided
  export). The browser library owns the dispatch table.
- **Exports**: `--export-all` for the demo is acceptable; later add explicit
  `--export=start`/`--export=...` flags.
- **Host file**: `lang/runtime/wasm/host.mjs`, plus a tiny `index.html`. Node can
  instantiate the same module with a mock DOM for unit tests.

### 2.5 Browser/DOM library

Add `lang/libs/browser` (or `wasm_dom`) as the wasm-side UI foundation:

```chemical
public namespace browser {
    public struct Node { private var id : u32 }
    public func document_body() : Node
    public func create_element(tag : string_view) : Node
    public func set_text(node : Node, text : string_view)
    public func append_child(parent : Node, child : Node)
    public func add_event_listener(node : Node, event_id : u32, handler : () => void)
    public func set_attribute(node : Node, name : string_view, value : string_view)
}
```

Implementation notes:

- The safe surface uses `string_view`/owned `string`; the raw `@extern
  @no_mangle` declarations underneath take scalars (`*char`, `size_t`, `u32`).
- The library is target-gated: `if wasm32` (or `option host = "wasm"`), so a
  native build includes declarations only or refuses calls.
- `MIRTargetSupport` must allow the wasm DOM symbol prefix for the wasm target
  (the existing `chx_dom_` rule blocks LLVM/C; either extend the rule with a
  wasm exception or use a distinct `chc_wasm_dom_` prefix). Prefer a distinct
  prefix to avoid accidentally enabling the JS-only `dom` module on native.
- Event handler storage: `std::unordered_map<u32, std::function<() => void>>` or a
  fixed table; exported `chc_wasm_on_event` dispatches. Handles are `u32`.
- The first demo only needs: document/body, create_element, set_text,
  append_child, add_event_listener, and a click event id.

### 2.6 UI demo and acceptance

Add `lang/compiled/wasm_ui/`:

- `chemical.mod`: `application wasm_ui`, `source "src"`, `import std`,
  `import browser`, `link c "runtime.c" if wasm32`.
- `src/main.ch`: `start()` builds a `div` containing a `button` and a `span`
  counter; clicking increments and updates the text; exports `start` and the
  event dispatch export.
- `index.html` + `host.mjs`: instantiate `app.wasm`, wire DOM handles/events,
  call `start`.
- Playwright test (reuse `lang/compiled/components-e2e` Playwright install):
  load the page, assert the initial counter, click, assert the updated counter,
  and fail on console errors/runtime traps.
- Node test: instantiate with a tiny DOM shim, call `start`, dispatch a click,
  assert the counter. This keeps CI independent of a browser when needed.

This demo is the definition of "the UI library can work in the browser using
wasm".

---

## 3. Milestones

### W0 — Target plumbing + freestanding link (no std)

- Fix `init_target_data` for wasm and mirror it in `TargetData`.
- Add wasm-ld driver selection + wasm link flags in `compiler/Codegen.cpp`.
- Build and run a trivial `main() -> 42` app end-to-end in Node.
- Add `lang/tests/wasm/` and `scripts/wasm-test.sh` (Node).
- Verify `--out-ll-all` and `--out-obj` still work; the object must be wasm.
- Do not change native linking.

**Exit:** `Compiler app.mod --target wasm32-unknown-unknown -o app.wasm`
succeeds; Node instantiates and `main()` returns 42.

### W1 — Freestanding runtime + wasm-safe std core

- Add `lang/runtime/wasm/runtime.c`; link it via `link c ... if wasm32`.
- Add `cstd/wasm` platform sources (malloc/free/realloc/mem*/str*/stdio shims,
  errno constants) and `std/wasm` (or target conditions) so `std` can build.
- Adjust `cstd`/`std` `chemical.mod` conditions so wasm does not select
  POSIX/Windows; exclude threads/process/fs/net/http/tls from the wasm profile.
- Make `std::string`, `std::vector`, `Option`, `Result`, `unordered_map`, and
  `std::mem` compile and run in wasm (a string/vector program in Node).
- Provide 64-bit division builtins if the link reports them.
- Document the wasm-safe std subset and exclusions.

**Exit:** A wasm app using `std::string`, `std::vector`, `Option`/`Result`, and a
short allocation/format loop builds with no manual objects and runs in Node with
the expected output.

### W2 — C emitter parity

- Make `TCCCompiler --target wasm32 --emit-c` select the wasm platform and emit
  C for a multi-module app.
- Add `scripts/wasm-c-build.sh` (clang `-c` per file + `wasm-ld` + runtime.c).
- Ensure the same source behaves identically through LLVM and through the C
  path.
- Add `--target wasm32` usage docs.

**Exit:** the W1 app, emitted as C by `TCCCompiler`, compiles with clang/wasm-ld
and runs in Node with identical observable output.

### W3 — Browser DOM bridge + UI demo

- Add the `browser`/`wasm_dom` library with scalar externs, handle table, and
  event dispatch.
- Allow the wasm DOM prefix in `MIRTargetSupport` for wasm only.
- Add `lang/runtime/wasm/host.mjs` + `index.html`.
- Add `lang/compiled/wasm_ui` counter demo.
- Add Playwright and Node tests; wire into `scripts/wasm-test.sh` (and a `--wasm`
  test flag if the suite layout warrants it).

**Exit:** Chromium (Playwright) loads the UI, the button click updates the
counter, no runtime trap/console error; Node unit test passes.

### W4 — Hardening / later profiles

- `wasm32-wasi` (WASI sysroot or a wasi host), files/clock/random.
- `MIRABIKind::Wasm32` C ABI for by-value aggregates and `string_view` interop.
- Memory64, SIMD, atomics/threads, exceptions — only if a real UI need appears.
- Debug info/source maps, size/perf measurements, cross-target CI.
- Reconcile `dom` and `browser` (share the JS `chx_dom_*` names if the C ABI
  supports them; otherwise keep the scalar wasm bridge).

---

## 4. Guardrails

- **Do not break native targets.** Run the existing TCC/LLVM suites after each
  milestone; wasm changes must be conditional on the triple.
- **No backend AST reads.** Target/link/runtime work stays in MIR/target layers.
- **No fake wasm support.** If a feature is unsupported (files, threads, full
  C ABI), fail with a clear diagnostic or document the wasm-safe subset; do not
  silently emit broken code.
- **No new dependencies in the core compiler.** The prebuilt LLVM 22.1.8,
  `wasm-ld`, and Playwright are already present. If a WASI sysroot is needed
  later, make it an optional downloaded resource, not a hard build dependency.
- **The C path must not use TCC for wasm.** TCC remains the C emitter; clang/
  wasm-ld are the wasm compilers.
- **Keep Windows/macOS/Linux native behavior identical.** Compare object sizes
  and run the relevant suites.
- **One logical commit per milestone/task**, with the wasm acceptance command in
  the message/STATUS.

---

## 5. Definition of done (this wave)

1. `Compiler <app.mod> --target wasm32-unknown-unknown -o app.wasm` builds
   end-to-end for a std-using UI app; Node and Chromium run it.
2. `TCCCompiler <app.mod> --target wasm32-unknown-unknown --emit-c` emits C
   that `scripts/wasm-c-build.sh` compiles to an equivalent wasm module.
3. The browser UI demo (button + counter) passes Playwright with no traps.
4. `cstd`/`std` wasm platform layer and the wasm-safe subset are documented.
5. Native `--tcc`, `--llvm`, `--libs`, `--safety`, `--negative`, `--plugins`
   suites remain green; no native target regressions.
6. `lang/docs/wasm-target-plan.md` (this file) records the final status and the
   W4 backlog.
