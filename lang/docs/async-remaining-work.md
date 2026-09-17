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
| 1 | **B26** | Large struct variant through `FutureHandle` corrupts (LLVM) | `LLVMCoroutine.cpp` | M |
| 2 | **B24** | Struct-typed async parameter field access (2c) | `2cASTVisitor.cpp` | S |
| 3 | **B25** | Combinator await inside a spawned coroutine loses `Context` (LLVM) | `LLVMCoroutine.cpp` | M |
| 4 | **B20** | Composite-generic field access in a generic body | symres/generics | L |
| 5 | **B15-W** | Async debug info disabled (LLVM `debug_complete`) | `LLVMCoroutine.cpp` | M |
| 6 | **AC** | Async closures not lowered | parser/symres/codegen | L |
| 7 | **TLS-VT** | TLS has no non-blocking transport | `tls` | L |
| 8 | **WIN-IOCP** | Windows async reactor is a stub | `async`/`net` win | L |
| 9 | **POSIX-EPOLL** | POSIX reactor is `select(2)` | `async` posix | M |

**B23 is fixed** (kept below for reference). Items 1–4 are pure compiler bugs;
5 is infrastructure; 6–9 are features/gaps. All are independent unless noted.

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

## B26 — A large struct variant through a `FutureHandle` is corrupted on LLVM

- **Status:** Worked around. **Priority: High.** (Related to the now-fixed B23 — same result-storage neighbourhood; start by checking whether the B23 fix covers part of it.)
- **Symptom:** `block_on<Result<Response, std::string>>(http::get_async(...))`
  corrupted the result (string payload became a garbage pointer; destructor
  crashed), and in some configurations produced a *broken LLVM module*
  (`Global is external, but doesn't have external or weak linkage!` for a `tls`
  global). Small variants (`Result<int, std::string>`) and `fs`/`process`
  payloads are fine — the trigger is a **large struct inside the variant**
  (`Response` contains a `Body` with `std::string`/buffer state).
- **Root cause / where:** `compiler/backend/LLVMCoroutine.cpp` — LLVM async
  result storage / payload lowering for a large struct-shaped `inner_ty` (the
  wrapper `result` field + the `Poll<T>` payload load). Same result-storage code
  path the B23 fix touched, but a *variant/large* payload — re-check after B26's
  own repro.
- **Current workaround:** `lang/libs/http/src/async.ch` returns `*mut HttpResult`
  — a flat heap box populated on the pool thread from the `Result`
  (`is_ok`/`status_code`/`body_view`/`error_view`; caller `delete`s it). Pointer
  payloads are unaffected.
- **Definition of done:** `block_on<Result<Response, std::string>>(...)` works on
  LLVM without corruption and without emitting an invalid module. Add a test
  using a large struct variant payload. (Prefer keeping the flat box as a public
  API decision if desired, but the compiler must not corrupt it.)
- **Files:** `compiler/backend/LLVMCoroutine.cpp`.

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

## B25 — A spawned coroutine that awaits a combinator loses its `Context` on LLVM

- **Status:** Worked around. **Priority: High.**
- **Symptom:** `async::spawn<int>(f())` where `f` awaits `async::timeout_or(...)`
  or `async::select(...)` crashes on LLVM: the combinator polls the inner future
  with a `Context` whose waker has a garbage `vtbl`; `ready_poll`/`Waker::clone`
  jumps through it. Minimal repro: a spawned coroutine awaiting
  `timeout_or<Unit>(readable(fd), ms, Unit)`. The same future awaited directly
  from `block_on` works, and `await spawn_blocking` inside a spawned coroutine
  works.
- **Root cause / where:** `compiler/backend/LLVMCoroutine.cpp` — the coroutine
  `__cx`/`Context` forwarding: the `Context*` passed to a coroutine that is
  itself polled by another future is not preserved when it forwards to a child
  poll.
- **Current workaround:** never await a combinator inside a task polled on the
  executor. `server::serve_coro` (`lang/libs/http/src/async.ch`) uses a bounded
  blocking `accept` on the pool instead of `timeout_or(accept_async(...))`.
- **Definition of done:** a spawned coroutine awaiting `select`/`timeout_or`
  completes correctly on LLVM. Add an executor test.
- **Files:** `compiler/backend/LLVMCoroutine.cpp`, `compiler/async/AwaitNormalizePass.cpp`.

---

## B20 — Field access on a composite-generic type inside a generic body

- **Status:** Partially fixed (`GenericFuncDecl::instantiate_call` now preserves
  `GenericType` args); the **field-access** half remains. **Priority: Medium.**
- **Symptom:** inside a generic function, accessing a field of a generic type
  instantiated with a *composite* argument resolves to the master's field type,
  not the substituted one:

  ```chemical
  public func <T> spawn_like(h : core::async::FutureHandle<T>) : int {
      var tt = malloc(sizeof(core::async::FutureTable<core::async::Unit>)) as *mut ...
      tt.poll = unit_ok   // error: '...Poll<Unit>' does not satisfy '...Poll<T>'
  }
  ```

  The reported type is the master `FutureTable` parameter (`Poll<T>`) because
  `SymResLinkBody::VisitGenericType` only calls `instantiate_inline` in a generic
  body, so `referenced->linked` stays the master.
- **Root cause / where:** `compiler/symres/SymResLinkBody.cpp::VisitGenericType`
  (and the member-access linking that consumes `referenced->linked`). Making it
  instantiate in a generic context previously aborted with
  `unexpected generic type parameter usage` on `std` partially-applied generics
  — that is the real problem to solve (partially-applied generics).
- **Current workaround:** type-erase bookkeeping through non-generic vtable
  structs (`TaskVTable`, `RawTask` in `exec.ch`); vtable/state types use a bare
  generic parameter (`FutureTable<T>`), never a composite of parameters
  (`FutureTable<Result<T, E>>`). This is why `select`/`timeout_or` return the
  child payload type directly instead of `Either`/`Result`.
- **Definition of done:** the minimal repro compiles; `FutureTable<Result<T, E>>`
  is legal; `select`/`timeout_or` can return a proper `Either`/`Result` if
  desired. Keep the `std` partially-applied-generic path green.
- **Files:** `compiler/symres/SymResLinkBody.cpp`,
  `compiler/generics/GenericInstantiator.cpp`,
  `ast/types/GenericType.cpp`.

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
