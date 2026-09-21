# Async/Await — Remaining Work (Worked Around, Not Fixed)

**Purpose.** The async/await feature is shipped and integrated across `lang/libs`,
but several compiler and platform limitations were *worked around* rather than
fixed. This file is an **actionable worklist**: each item has a stable ID, the
symptom, the root cause location, the existing workaround, and a concrete
definition of done. An AI or human should be able to pick one item, fix it, and
verify it without re-deriving context.

**Companion docs**

- [`async-await-design.md`](./async-await-design.md) — normative language/runtime design.
- [`async-library-integration.md`](./async-library-integration.md) — assessment + rollout; §12 has the full blocker history (B1–B28). `B10/B11/B12/B14/B15/B16/B17/B19/B21/B22/B27/B28` are **fixed**; `B13` and `B18` are **by design**.

## Ground rules for every item

1. **Do not remove the library workaround until the compiler fix is green on the
   full matrix** (below). The workaround is the compatibility contract for code
   already shipped.
2. **Add a regression test** that fails before the fix and passes after. Prefer a
   minimal `lang/tests/async/` or `lang/tests/libs/async/` test, and (for
   codegen) assert the emitted IR / C too.
3. **Never change a synchronous API** (see `async-library-integration.md` §5).
4. Keep the fix in the compiler, not in the library, unless the item explicitly
   says otherwise.
5. Rebuild the relevant target first (`./scripts/build.sh --llvm` for `Compiler`,
   `--tcc` for `TCCCompiler`) — a stale binary silently ignores C++ changes.

### Verification matrix (run after a fix)

```bash
./scripts/test.sh --tcc  --async --libs --process --server --webview
./scripts/test.sh --llvm --async --libs --process --server --webview
./scripts/test.sh --tcc  --tls         # known flaky; the async tests matter
./scripts/test.sh --tcc                 # main
```

A fix is not done until the item's own repro passes on **both** TCC and LLVM and
the suite that exercises it stays green. (`--all` is for humans only; agents must
not run it.)

---

## Recommended order

| Order | ID | Title | Area | Effort |
|-------|----|-------|------|--------|
| ~~1~~ | ~~**B24**~~ | ~~Struct-typed async parameter field access (2c)~~ — ✅ FIXED | `2cASTVisitor.cpp` | S |
| ~~2~~ | ~~**B25**~~ | ~~Combinator await inside a spawned coroutine loses `Context` (LLVM)~~ — ✅ FIXED | `LLVMCoroutine.cpp` | M |
| ~~3~~ | ~~**B20**~~ | ~~Composite-generic field access in a generic body~~ — ✅ FIXED | symres/generics | L |
| 4 | **B15-W** | Async debug info disabled (LLVM `debug_complete`) | `LLVMCoroutine.cpp` | M |
| 5 | **AC** | Async closures not lowered | parser/symres/codegen | L |
| 6 | **TLS-VT** | TLS has no non-blocking transport | `tls` | L |
| 7 | **WIN-IOCP** | Windows async reactor is a stub | `async`/`net` win | L |
| 8 | **POSIX-EPOLL** | POSIX reactor is `select(2)` | `async` posix | M |

**B20, B23, B24, B25 and B26 are fixed** (kept below for reference). Items 4–8
are infrastructure/features/gaps. All are independent unless noted.

---

## B23 — A future whose payload is a *plain struct* corrupts it on LLVM — ✅ FIXED

- **Was:** `async func f() : Pair` (plain struct, no variant) driven by
  `block_on<Pair>(f())` yielded pointer halves instead of field values (e.g.
  `a=1929545688 b=32767`), on both the eager and suspending paths. Returning a
  *variable* was already correct.
- **Root cause (confirmed):** `Codegen::writeReturnStmtFor`'s coroutine branch
  (`compiler/backend/LLVM.cpp`) sent a `StructValue` through
  `value->llvm_value(...)` + `CreateStore`. In a function context
  `StructValue::llvm_value` returns its **alloca pointer**, so the pointer's two
  halves were stored into the frame's struct result field (opaque pointers make
  that `store ptr, ptr` legal, so the verifier did not catch it).
- **Fix:** the coroutine branch now byte-copies every struct-like return value
  into the frame result — `value->llvm_pointer(gen)`, materializing via
  `llvm_value` when it is null (a not-yet-allocated `StructValue`) — mirroring the
  non-coroutine aggregate-return path (`llvm_ret_value`/memcpy). Scalar returns
  keep the `llvm_value` + `implicit_cast` + store path.
- **Regression tests:** `lang/tests/async/struct_payload_test.ch`
  (`test_async_struct_payload_literal`, `..._suspend`, `..._destructible`) —
  covers the eager path, a real suspension, and a destructor-bearing struct
  (no double-free). Suite `--async` is 37/37 on TCC and LLVM.
- **The old workarounds may remain** (they are valid design/ABI choices, and
  `block_on`'s `poll_take_ready<T>` is still needed for B18): `net` futures keep
  carrying raw fds, `http` keeps its flat `*mut HttpResult` box.

---

## B26 — A large struct variant through a `FutureHandle` is corrupted on LLVM — ✅ FIXED

- **Was:** `block_on<Result<Response, std::string>>(http::get_async(...))`
  corrupted the result (string payload became a garbage pointer; destructor
  crashed), and in some configurations produced a *broken LLVM module*
  (`Global is external, but doesn't have external or weak linkage!` for a `tls`
  global). Small variants (`Result<int, std::string>`) and `fs`/`process`
  payloads were fine — the trigger was a **large struct inside the variant**
  (`Response` contains a `Body` with `std::string`/buffer state).
- **Resolution:** fixed by the **B23** result-store fix in
  `Codegen::writeReturnStmtFor` (the coroutine branch no longer stores an
  aggregate pointer into the frame result). The "broken LLVM module" half was the
  **B27** `current_function` leak (globals after an `async func`), also fixed.
- **Verification:** the original shape was re-tested with a faithful large
  variant (`Response` with nested `string`/`vector`/pointer/bool state) across
  every path — eager, suspended, `return await`, an awaited large variant inside
  another coroutine, `std::Result<...>`, and a bare
  `spawn_blocking<LargeVariant>` polled directly by `block_on`. All pass on TCC
  and LLVM. Regression tests:
  `lang/tests/async/variant_payload_test.ch` (4 tests; `--async` is 41/41 on both
  backends). The original corruption could not be re-triggered independently after
  B23/B27.
- **The `http` workaround stays** (`*mut HttpResult`, a flat heap box) — it is
  kept as a public API decision for its non-moving accessor API
  (`ok`/`status`/`body_view`), not because the payload is unsafe now.
- **Files:** `compiler/backend/LLVM.cpp` (B23 fix), `compiler/backend/LLVMCoroutine.cpp` (B27 fix).

---

## B24 — Field access on a struct-typed async parameter mis-lowers on 2c

- **Status:** Worked around. **Priority: Medium.**
- **Symptom:** `async func f(s : SomeStruct)` reading `s.field` emits
  `frame->slot->field` where the frame slot is stored **by value**
  (`struct SomeStruct slot;`) — a C compile error (`pointer expected`). Variant
  and pointer/primitive parameters are unaffected.
- **Root cause / where:** `preprocess/2c/2cASTVisitor.cpp` async lowering — for a
  by-value struct parameter the frame slot must be accessed with `.` not `->`
  (or struct parameters should be stored as pointers consistently). Look at the
  frame-slot access emission used by async bodies.
- **Current workaround:** pass raw handles (`Socket`, `int`, `*T`) into
  coroutine bodies and wrap synchronously outside. The `net` async entry points
  take `int` fds for this reason.
- **Definition of done:** an `async func` with a struct-typed parameter can read
  its fields on TCC. Add a test to `lang/tests/async/`.
- **Files:** `preprocess/2c/2cASTVisitor.cpp`.

---

## B25 — A spawned coroutine that awaits a combinator loses its `Context` on LLVM — ✅ FIXED

- **Was:** `async::spawn<int>(f())` where `f` awaits `async::timeout_or(...)` or
  `async::select(...)` crashed on LLVM: the combinator polled the inner future
  with a `Context` whose waker had a garbage `vtbl`, and `ready_poll`/`Waker::clone`
  jumped through it. Awaiting the same future directly from `block_on` worked.
- **Root cause (confirmed):** `gen_llvm_await` (`compiler/backend/LLVMCoroutine.cpp`)
  loaded the caller's `Context*` from the coroutine wrapper **once, before the
  await poll loop**, and reused it after every resume. The executor
  (`executor_poll_tasks`) passes a *fresh stack `Context`* on every poll; a resumed
  coroutine therefore forwarded a stale pointer to its child poll. The first
  (pre-suspend) poll used a valid `Context`, which is why direct `block_on`
  (single poll) was unaffected.
- **Fix:** reload the `Context*` from the wrapper inside the `await.loop` block so
  each iteration — including the resume path — uses the `Context` stored by
  `emit_poll_fn` at poll entry.
- **Regression tests:** `lang/tests/async/spawn_combinator_test.ch`
  (`test_async_spawn_await_timeout_or`, `test_async_spawn_await_select`). `--async`
  is 48/48 on TCC and LLVM.
- **Note:** `server::serve_coro` (`lang/libs/http/src/async.ch`) still uses the
  bounded blocking `accept` workaround; switching it to `timeout_or(accept_async(...))`
  is now possible but is a separate behavioural change (needs the `--tls` suite).
- **Files:** `compiler/backend/LLVMCoroutine.cpp`.

---

## B20 — Field access on a composite-generic type inside a generic body — ✅ FIXED

- **Was:** inside a generic function, accessing a field of a generic type
  instantiated with a *composite* argument resolved to the master's field type,
  not the substituted one:

  ```chemical
  public func <T> spawn_like(h : core::async::FutureHandle<T>) : int {
      var tt = malloc(sizeof(core::async::FutureTable<core::async::Unit>)) as *mut ...
      tt.poll = unit_ok   // error: '...Poll<Unit>' does not satisfy '...Poll<T>'
  }
  ```

- **Root cause (confirmed):** a generic type written in a generic body was always
  deferred (`SymResLinkBody::VisitGenericType` called only `instantiate_inline`,
  a no-op for structs). Because the signature pass also defers instantiations,
  `Box<int>`/`FutureTable<Unit>` still pointed at the master declaration while the
  body was linked, so member access resolved to the master's generic parameter
  (`T`). Parameter types were additionally never re-visited during the master body
  link (`SymResLinkBody::VisitFunctionParam` only declared the parameter).
- **Fix (`compiler/symres/SymResLinkBody.cpp`):**
  1. In `VisitGenericType`, for a **struct-like** generic (`GenericStructDecl`,
     `GenericUnionDecl`, `GenericVariantDecl`) whose arguments are all concrete and
     whose count matches, instantiate it during the generic-context link. Types
     that still mention a generic parameter (partially applied, e.g. `Box<T>`,
     `gen_point_existence_t34<() => T>`) stay deferred, so the `std`
     partially-applied path is unchanged. Interfaces, type aliases and generic
     functions also stay deferred (early instantiation broke interface dispatch /
     function references — the `json` library caught this).
  2. In `VisitFunctionParam`, visit a concrete parameter type during the generic
     body link so the parameter itself carries the instantiated member types.
  - `type_mentions_generic_param` recurses through generic args, pointers,
    references, arrays, function/closure types and `%runtime`/`%maybe_runtime`
    wrappers.
- **Definition of done met:** the minimal repro compiles;
  `FutureTable<Wrapper<int, int>>` (composite argument) is legal and its fields are
  substituted. The combinators still use the single-payload-type design — that was
  a deliberate ABI choice, and changing it is optional.
- **Regression tests:** `lang/tests/src/generic/basic.ch`
  (`test_native_generic_composite_field`, 3 cases) wired into
  `lang/tests/src/tests.ch`. TCC 2205/2205, LLVM 2206/2206, `--libs` 650/650,
  interpret 1814/1814.
- **Files:** `compiler/symres/SymResLinkBody.cpp`.

---

## B15-W — Async debug info is disabled on LLVM (`debug_complete`)

- **Status:** Workaround in place. **Priority: Medium.**
- **Symptom (before the workaround):** any `async func` generated invalid LLVM
  debug info in `--mode debug_complete` (`location requires a valid scope`,
  `local variable requires a valid scope`), because the coroutine body is emitted
  via `code_gen_no_scope` (no `DISubprogram` pushed) and cloned resume/destroy
  functions carry locations whose scope is the compile-unit file.
- **Current workaround:** `FunctionDeclaration::code_gen_body`
  (`ast/structures/FunctionDecl.cpp`) disables `gen.di` for the entire async
  lowering, so async bodies/frames/poll/drop emit **no** debug locations.
- **Definition of done:** async functions carry correct debug info in
  `debug_complete`; the coroutine split clones each get their own
  `DISubprogram`. Do not re-enable `gen.di` globally until the split carries
  per-clone scopes.
- **Files:** `compiler/backend/LLVMCoroutine.cpp`, `compiler/Codegen.*`,
  `ast/structures/FunctionDecl.cpp`.

---

## AC — Async closures are not lowered

- **Status:** Parsed, then **diagnosed** (`async closures are not yet supported;
  use a named async func instead`). **Priority: Medium (feature).**
- **Symptom:** `var f = async |x: int|() : int => { return await work(x) }` is a
  symres error on both backends.
- **Root cause / where:** an async closure carries a capture struct
  (`write_lambda_function`), so lowering needs a closure-specific
  frame/vtable emitter that stores the capture struct in the frame and binds
  `this` to it in `poll`. Diagnostic:
  `compiler/symres/SymResLinkBody.cpp::VisitLambdaFunction`.
- **Current workaround:** use a named `async func`.
- **Definition of done:** async closures compile and run on TCC and LLVM, with
  captures; remove the diagnostic and add tests.
- **Files:** `ast/values/LambdaFunction.cpp`, `compiler/async/AwaitNormalizePass.cpp`,
  `compiler/backend/LLVMCoroutine.cpp`, `preprocess/2c/2cASTVisitor.cpp`,
  `compiler/symres/SymResLinkBody.cpp`.

---

## TLS-VT — TLS has no non-blocking transport (async handshake is offloaded)

- **Status:** Deferred design. **Priority: Low–Medium (performance).**
- **Symptom:** the async TLS path is `spawn_blocking` over the synchronous
  handshake, so it overlaps work on the thread pool but is not a true coroutine
  state machine; concurrency is bounded by pool size.
- **Root cause / where:** `tls` calls `net::send_all`/`net::recv_all` directly
  (`lang/libs/tls/src/ssl.ch`, the transport read/write sites around the
  `ssl_write`/`ssl_read` paths). A non-blocking transport can only suspend if the
  handshake itself is a coroutine state machine.
- **Current workaround:** `lang/libs/tls/src/async.ch` offloads all work to
  `async::spawn_blocking`.
- **Definition of done:** introduce a transport vtable on `SSLContext` (function
  pointers + user data) whose **default is the current blocking implementation**
  (bit-identical sync behaviour), with an async path that returns `Poll`.
  This is the invasive one — guard it with the entire `--tls` suite.
- **Files:** `lang/libs/tls/src/ssl.ch`, `lang/libs/tls/src/types.ch`,
  `lang/libs/tls/src/async.ch`.

---

## WIN-IOCP — Windows async reactor is a stub

- **Status:** Stub. **Priority: Medium (Windows only).**
- **Symptom:** `lang/libs/async/win/reactor.ch` returns readiness immediately and
  a no-op poll; `net` async falls back to the thread pool on Windows.
- **Root cause / where:** `lang/libs/async/win/reactor.ch`. A real
  IOCP-backed reactor exists in skeleton form at `lang/libs/net/win/iocp.ch`
  (`CompletionPort`, `AsyncContext`, `async_recv`/`async_send`), already used by
  `http::server` on Windows.
- **Current workaround:** Windows `net` async uses the shared thread pool.
- **Definition of done:** wire `net`/`async` to IOCP on Windows so fd readiness
  is event-driven; keep the POSIX path unchanged. CI/manual verification on
  Windows required.
- **Files:** `lang/libs/async/win/reactor.ch`, `lang/libs/net/win/iocp.ch`,
  `lang/libs/net/win/platform_api.ch`.

---

## POSIX-EPOLL — POSIX reactor is `select(2)` (~1024 fd limit)

- **Status:** Deliberate v1. **Priority: Medium (scale).**
- **Symptom:** `lang/libs/async/posix/reactor.ch` uses `select(2)` (chosen over
  `poll(2)` to avoid a C-symbol clash with the `test` library), capping the
  reactor at `FD_SETSIZE` (~1024) fds and rescanning all fds each poll.
- **Root cause / where:** `lang/libs/async/posix/reactor.ch`
  (`reactor_platform_poll`).
- **Current workaround:** the executor blocks on the reactor while fds are
  registered; fine for current tests.
- **Definition of done:** add `epoll` (Linux) and `kqueue` (macOS/BSD)
  registration behind the same `reactor_register`/`reactor_poll` interface, with
  `select` kept as a portable fallback. No API change.
- **Files:** `lang/libs/async/posix/reactor.ch`, `lang/libs/async/src/reactor.ch`.

---

## Already decided (do **not** "fix")

- **B13** retention friction: calling a non-`public` function from a `public`
  generic is an error by design. Keep helpers `public` and use non-generic
  shims.
- **B18** moving a non-`Copy` generic value out of a pointed-to struct is
  correctly rejected. Use the non-moving idiom (`Option::take`/`memcpy`-out +
  empty replacement).
- `std::concurrent::Future<T>` vs `core::async::Future<T>` remain **separate**
  types; `spawn_blocking` is the bridge (`async-library-integration.md` §11.2).
