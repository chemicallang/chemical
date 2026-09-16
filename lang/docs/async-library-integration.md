# Async/Await Integration in `lang/libs` — Assessment & Rollout Plan

**Status:** assessment (no library code changed by this document).
**Date:** September 16, 2026.
**Companion:** [`async-await-design.md`](./async-await-design.md) — the normative
language/runtime design. This document does **not** re-litigate it; it applies it
to the shipped libraries and to the `--libs` test suite.

---

## 1. Purpose

Answer one question: **how should `async`/`await` be integrated into `lang/libs`
so every library benefits, without breaking existing APIs, and with tests that
run under `./scripts/test.sh --tcc --libs` (and `--llvm --libs`)?**

Audience: library authors, reviewers, and whoever schedules the work. The
guiding constraint, stated plainly: **existing synchronous APIs are a
compatibility surface and must keep working, unchanged, forever.** Async is
purely additive.

---

## 2. Executive summary

1. **The compiler is ready; the runtime is not.** `async func` / `await` are
   lowered on both the C and LLVM backends, and the cancellation guarantees are
   tested (`--async` is 33/33 on both backends). But `lang/libs/async` currently
   ships **only** `block_on` and the frame allocator hooks. There is **no
   executor, no reactor, no `spawn`, no timer, no `select`, no `spawn_blocking`**.
2. **Every shipped library is blocking today.** `net`, `tls`, `http`, `fs`,
   `process`, `environment`, `webview`, `window`, `server`, `minlsp`, `ide` are
   all synchronous. The pure-CPU libraries (`json`, `crypto`, `compression`,
   `archive`, `image`, `font`, `audio`, `regex`, `encoding`, `mime`, `path`,
   `uuid`, `bcrypt`, `osrand`, the parsers) have **no I/O to suspend on** and
   should stay synchronous.
3. **Integration must be additive and staged.** Add `_async` methods / parallel
   `Async*` types beside the existing sync ones; never change a signature.
   Tier 0 (runtime) must land before any library wrapper, because there is
   nothing to `await` against yet.
4. **`--libs` can start testing async immediately** — the deterministic,
   reactor-free parts (`block_on`, `await`, cancellation, `spawn` once it
   exists) need no sockets. Network/TLS/process async tests need the reactor and
   a real environment, so they belong in dedicated suites (`--tls`, `--process`)
   or in `--libs` behind a loopback/hermetic gate.
5. **Two concrete API collisions must be designed around, not "fixed" later:**
   `http::server::Server::serve_async` **already exists** (it returns
   `std.concurrent.Thread`), and `std::concurrent::Future<T>` collides by name
   with `core::async::Future<T>` (design blocker B5).

---

## 3. Current state (verified at this revision)

### 3.1 Compiler

| Capability | Status |
|---|---|
| `async func f(...) : T` → static type `FutureHandle<T>` | ✅ both backends |
| `await` in `async func` (eager and suspending) | ✅ both backends |
| Cancellation runs frame destructors exactly once | ✅ both backends |
| `async func main` → synchronous `int main` trampoline | ✅ both backends |
| Async closures (`async \|...\|`) | ❌ diagnosed, not lowered |
| Interpreter / comptime | ❌ eager/transparent only |

Evidence: `lang/tests/async/` (33 tests) via `./scripts/test.sh --async`.

### 3.2 Protocol (`lang/libs/core/async.ch`)

`Poll<T>` (`Ready`/`Pending`), `WakerVTable`, `Waker`, `Context`, `Future<T>`
(interface), `FutureTable<T>`, `FutureHandle<T>` (move-only, `@delete` drops the
frame), `Unit`, and the `@extern` frame hooks
`chemical_async_frame_alloc`/`_free`.

The protocol is compiler-facing and lives in `core`, which every module already
depends on (design D10). **No library needs to depend on `async` to use
`async func`** — only to get an executor.

### 3.3 Runtime (`lang/libs/async`)

| Component | Present? | Notes |
|---|---|---|
| `block_on<T>(handle) : T` | ✅ | bootstrap executor; parks ~1ms on `Pending` (not a busy spin) |
| `chemical_async_frame_alloc` / `_free` | ✅ | `malloc`/`free` default |
| `yield_now() : FutureHandle<Unit>` | ✅ | first hand-authored runtime future (no reactor) |
| `sleep(millis) : FutureHandle<Unit>` | ✅ | poll-driven clock future |
| `test::block_on<T>` | ✅ | deterministic executor for `--libs` tests |
| `spawn` / task queue / executor | ❌ | see §12 compiler blockers |
| `spawn_blocking` | ❌ | `ThreadPool` bridge ready; prototype works on TCC, blocked on LLVM by §12 B14 |
| `timeout` / `select` | ❌ | blocked by §12 (generic future tables) |
| Reactor (epoll/kqueue/IOCP) | ❌ | needed for async socket I/O |
| `channel` | ❌ | later phase |

**Landed (this revision).** `async::{yield_now, sleep, test::block_on}` and a
parking `block_on`; compiler fixes B11 (generic type identity) and B15 (async
works in `debug_complete`); library integration — additive
`fs::read_entire_file_async` / `write_text_file_async` / `atomic_write_async`
and `environment::{get_async, set_async, unset_async}` (v1 blocking bodies).
Tests: 6 async runtime + 3 fs + 1 environment, all under
`./scripts/test.sh --tcc --libs` **and** `--llvm --libs` (624/624 both), plus
`--async`/main green in `debug_complete`. Generic vtable, reactor,
`process` (B16), and true non-blocking `spawn_blocking` remain blocked by
B10/B12/B14/B16 (§12).

### 3.4 Libraries

| Library | Current API (must be preserved) | Blocking on | Async benefit | Tier |
|---|---|---|---|---|
| `net` | `dial`, `listen_addr`, `accept_socket`, `recv_all`, `send_all`, `close_socket`, `set_recv_timeout`, `set_keep_alive`; `Socket = usize` | connect/accept/read/write | many sockets on one thread | 1 |
| `tls` | `tls_connect`, `ssl_handshake`, `ssl_read`, `ssl_write`, `ssl_close_notify`, `ssl_free`, `ssl_set_socket`, `tls_accept` | full multi-RTT handshake + records, calling `net::send_all`/`recv_all` directly | overlap handshakes; async record I/O | 2 |
| `http` | `Client::request/get/post/put/patch/delete/head`; `Server::serve`, `Server::serve_async` (thread), `Body::read/read_to_string/read_exact/drain/close`; parsers `read_request_incremental`, `read_response_incremental` | dial → TLS → send → incremental read; worker-thread accept loop | `request_async`, per-connection tasks, streaming bodies | 3 |
| `fs` | `read_entire_file`, `atomic_write`, `read_to_buffer`, directory ops | disk syscalls | don't stall the executor | 4 |
| `process` | `execute`, `spawn`, `wait`, `try_wait`, `write_stdin`, `close_stdin`, `kill_process`, `is_running`, `sleep_ms` | child + pipes | awaitable child/pipes; `test_env` IPC | 4 |
| `environment` | get/set env | syscalls | `spawn_blocking` only | 4 |
| `webview`, `window` | UI-thread event loops (GTK3/WebKit2GTK) | host loop | background async work, post back to UI | 5 |
| `server`, `minlsp`, `ide` | socket/stdio loops over `http` | accept loops | same as `http` | 6 |
| `std.concurrent` | `Promise<T>`, `Future<T>`, `ThreadPool::submit`, `sleep_ms` | thread pool | substrate for `spawn_blocking` | 0 (interop) |
| `json`, `crypto`, `compression`, `archive`, `image`, `font`, `audio`, `regex`, `datetime`, `encoding`, `mime`, `path`, `uuid`, `bcrypt`, `osrand`, parsers | pure buffer/CPU | — | none directly; call via `spawn_blocking` if heavy | — |

Platform notes already in the tree:

- `net/posix/platform_api.ch` has `set_nonblocking`/`set_blocking` and only a
  **stub** `poll` comment ("use non-blocking accept instead"). No epoll.
- `net/win/iocp.ch` already has `CompletionPort`, `AsyncContext`, `async_recv`,
  `async_send` (callback based). `http::server` already uses it on Windows.
- `http::server::serve_async` **is not async/await** — it spawns the blocking
  accept loop on a `std.concurrent.Thread`. The name is taken.

### 3.5 Tests

- `lang/tests/libs/chemical.mod` imports 22 libs: `cstd`, `std`, `test`,
  `test_env`, `atomic`, `bcrypt`, `uuid`, `json`, `datetime`, `regex`, `fs`,
  `path`, `encoding`, `crypto`, `compression`, `osrand`, `mime`, `audio`,
  `font`, `archive`, `image`, `async`. **No `net`/`http`/`tls`/`process`/
  `webview`.**
- `lang/tests/libs/main.ch` dispatches `@test` functions via `test_runner`.
- `lang/tests/libs/async/` holds the deterministic runtime tests (6 tests),
  `lang/tests/libs/fs/async_test.ch` the fs wrapper tests (3 tests) and
  `lang/tests/libs/environment/async_test.ch` (1 test); suite total is now 624
  on both backends.
- `net`/`http` have **no dedicated suite**; `tls` → `--tls`, `process` →
  `--process`, `webview` → `--webview`.
- `fs` currently has 24 lib tests; `integration/` has 17 cross-library tests.

---

## 4. Goals and non-goals

**Goals**

- G1 — Every library that blocks gains an **additive** async path.
- G2 — No existing public signature, type, or behaviour changes.
- G3 — Async library tests run under `--libs` on **both** backends.
- G4 — CPU-only libraries are explicitly declared out of scope (not silently
  "un-integrated").
- G5 — The runtime (`async`) grows the minimum needed to make I/O libraries
  useful.

**Non-goals**

- Rewriting any library to be async-only.
- Changing the existing sync tests or their pass/fail contract.
- Adding `async` as a hard dependency of `core`/`std`/pure libs.
- Solving async closures or interpreter async here (they are compiler gaps that
  limit ergonomics; tracked in the design doc).

---

## 5. API-stability contract (the "do not break APIs" rule)

These rules are binding for every async integration PR.

1. **Additive only.** New methods get a new name. A method may never change its
   parameter list, return type, or semantics. `net::dial` stays `net::dial`.
2. **Reuse existing types.** Async entry points return the *same* result/value
   types (`std::Result`, `http::Response`, `std::string`) wrapped in a future —
   not new error or response types. This keeps call sites and docs valid.
3. **Naming convention (design F11).** Mirror the sync API:
   `X::foo` → `X::foo_async`; a distinct type gets an `Async` prefix
   (`net::AsyncSocket`). This makes the async surface discoverable and greppable.
4. **Avoid name collisions deliberately.** If a name is taken by a *different*
   meaning, do not overload it. Concretely: `http::Server::serve_async` already
   means "serve on a worker thread", so the coroutine accept loop must use a new
   name (`serve_tasks`/`run_async`) — see R1.
5. **The sync path remains first-class.** Async wrappers may not wrap,
   reimplement, or delete the sync implementation. Both share the same
   primitives (a shared `Transport`, the same parser). If a bug is fixed, it is
   fixed in the shared primitive, not in one path only.
6. **Deprecation is opt-in and delayed.** Nothing is marked
   `@deprecated("use foo_async")` until the async version has shipped, been
   tested under `--libs`, and been used by at least one real consumer. The sync
   version is never removed in the same window.
7. **Opt-in by import, not by global flag.** A library that gains async features
   depends on the `async` runtime module; a library that doesn't stays
   sync-only. No `CHEMICAL_ASYNC=1` switches.
8. **`Send`/thread-affinity is documented, not assumed** (design D7/D8). Every
   async type states which executor/thread it may be used on.

---

## 6. Target architecture for the libraries

```
        user code / libraries
   ┌───────────────────────────────────────────────┐
   │ http::Client::request_async   http::Server     │
   │ process::wait_async   fs::read_entire_file_async│
   └───────────────┬───────────────────────────────┘
                   │  await
   ┌───────────────▼───────────────────────────────┐
   │ tls::ssl_*_async  ──  Transport (read/write)  │
   └───────────────┬───────────────────────────────┘
                   │
   ┌───────────────▼───────────────────────────────┐
   │ net::AsyncSocket  (register fd + Waker)       │
   └───────────────┬───────────────────────────────┘
                   │
   ┌───────────────▼───────────────────────────────┐
   │ async runtime: executor, reactor, timers,     │
   │ spawn, select, spawn_blocking                 │
   │   reactor POSIX: epoll/kqueue                 │
   │   reactor Windows: reuse net::iocp            │
   │   spawn_blocking → std::concurrent::ThreadPool│
   └───────────────────────────────────────────────┘
```

Design principles:

- **Transport abstraction first.** `tls` currently calls `net::send_all` /
  `net::recv_all` directly. Introduce a transport vtable on `SSLContext`
  (function pointers + user data) whose **default is the current blocking
  implementation**, so sync behaviour is bit-identical. The async path installs
  transport callbacks that return `Poll`. This is the single most invasive
  change and should be a self-contained PR with the sync tests as guardian.
- **One `AsyncSocket`.** Wrap `net::Socket` (`usize`), do not replace it. It
  owns reactor registration and converts readiness into a `Waker` wake.
- **`spawn_blocking` is the universal escape hatch.** `fs`, `process`,
  `environment`, and any heavy CPU codecs wrap the existing
  `std::concurrent::ThreadPool::submit` (`lang/libs/std/src/concurrency/threadpool.ch`).
  This lets libraries gain awaitable APIs before the native reactor exists.
- **Cancellation is already guaranteed** by frame drop; async wrappers that own
  resources (`tls::SSLContext`, sockets) must rely on `@delete` exactly as the
  sync code does. This is a prerequisite, not an afterthought (design F7).

---

## 7. Library-by-library rollout

### Tier 0 — runtime (`lang/libs/async`) — **prerequisite**

Additive to a package that is already async-only, so no compatibility risk.
Deliver, in order:

- `async::test::block_on` — deterministic executor with no reactor (also usable
  in `@test`).
- `Executor` + task queue + `spawn<T>(FutureHandle<T>)`; per-thread default
  (design D7).
- `sleep`/`timeout` (timer wheel), `select`.
- `spawn_blocking<F, T>(f) : FutureHandle<T>` over `ThreadPool::submit`.
- `AsyncSocket`-supporting reactor (epoll/kqueue; IOCP reuse on Windows).
- Current-thread mode + `spawn_local` (for F8/UI libs).
- A `channel` (M6, optional for v1).

### Tier 1 — `net`

- Add `net::AsyncSocket` (`connect`/`accept`/`read`/`write`/`close`) and
  `dial_async`/`accept_async`/`recv_async`/`send_async`.
- Preserve every existing function untouched.
- POSIX: add the real `poll`/`epoll` registration (the current `poll` is a stub)
  and `set_nonblocking` (already present). Windows: reuse `net::iocp`.

### Tier 2 — `tls`

- Add `tls_connect_async`, `ssl_handshake_async`, `ssl_read_async`,
  `ssl_write_async`.
- Internals: replace the direct `net::send_all`/`recv_all` calls
  (`tls/src/ssl.ch:2100,2107`) with the transport vtable; the default vtable is
  the current blocking calls. `tls_connect` (`:5742`) keeps calling `net::dial`;
  async gets a separate entry point.
- Highest value: the handshake is multi-RTT, so overlapping it is the big win.

### Tier 3 — `http`

- Client: `Client::request_async` (+ `get_async`, `post_async`, …) returning a
  future of the **same** `std::Result<Response, std::string>`.
- Server: a real coroutine accept loop (`while run { var s = await
  listener.accept(); spawn(handle(s)) }`) using `select` for accept-vs-shutdown.
  **New name** — `serve_async` is taken (R1).
- Streaming: `Body::read_chunk_async` (design F12).

### Tier 4 — `fs`, `process`, `environment`

- `spawn_blocking` wrappers first (works before the reactor):
  `fs::read_entire_file_async`, `fs::atomic_write_async`,
  `process::wait_async`, `process::execute_async`, async pipe read/write.
- Keep the sync functions identical; share the same syscall helpers.
- **Landed (v1):** `fs::read_entire_file_async`, `fs::write_text_file_async`,
  `fs::atomic_write_async` (`lang/libs/fs/src/async.ch`);
  `environment::{get_async, set_async, unset_async}`
  (`lang/libs/environment/src/async.ch`). Both concrete-return `async func`s with
  blocking bodies; `fs`/`environment` now import `async`. They become true
  `spawn_blocking` wrappers once B10/B12/B14 land.
- **Blocked:** `process` async wrappers (`execute_async`/`wait_async`/…) — the
  result type `PR_Result` is a nested destructible aggregate, miscompiled by
  async returns (B16). No process async code is shipped.

### Tier 5 — `webview`, `window`

- `spawn_local` + a waker that posts to the UI thread. Not "await the UI"; run
  background async work and marshal results back.

### Tier 6 — `server`, `minlsp`, `ide`

- Adopt the `http` coroutine accept loop / stdio awaitable reads once Tier 3
  lands.

### Explicitly out of scope (stay synchronous)

`json`, `crypto`, `compression`, `archive`, `image`, `font`, `audio`, `regex`,
`datetime`, `encoding`, `mime`, `path`, `uuid`, `bcrypt`, `osrand`, and the
`html`/`css`/`js`/`md` parsers. They block on nothing; when a caller needs them
off the executor, wrap with `spawn_blocking`. `datetime` gains nothing — it
already has no I/O; `sleep` belongs to the runtime.

---

## 8. Test strategy for `--libs`

### 8.1 What can run under `--libs` today (no reactor)

Add `import async` to `lang/tests/libs/chemical.mod` and a
`lang/tests/libs/async/` directory. These tests are hermetic and deterministic:

| Test group | What it proves |
|---|---|
| `block_on` + eager `async func` | compiler→runtime round trip for value/struct/string results |
| `await` in loops/branches/match | control-flow lowering |
| hand-authored `Future` returning `Pending` N times | real suspend/resume |
| cancellation of a suspended future | frame drop runs destructors once |
| move of a `FutureHandle` | move-only semantics |
| `spawn` / `select` / `timeout` (once M2 lands) | executor correctness |
| `spawn_blocking` (once M2 lands) | thread-pool bridge |

These use the same hand-authored `Countdown`-style futures already proven in
`lang/tests/async/`, so no sockets or ports are needed. They must pass on
`--libs --tcc` **and** `--libs --llvm`.

### 8.2 What must be gated

Network/TLS/process async tests need loopback sockets, ports, certificates, and
timing. Rules:

- Bind to `127.0.0.1` with an ephemeral port (`:0`); never a fixed port.
- Put them behind an env gate (e.g. `CHEMICAL_TEST_NETWORK=1`) or in the
  dedicated suites (`--tls`, `--process`). `--libs` must stay runnable in a
  no-network sandbox.
- Prefer a test double: a `Transport` that returns `Pending` a fixed number of
  times, so `http::Client::request_async` logic is tested without a socket.

### 8.3 Cross-library integration

Extend `lang/tests/libs/integration/` with async combos:
`encoding + crypto` off the executor via `spawn_blocking`, `http + json` using
`request_async`, `fs + json` with `read_entire_file_async`. These prove the
additive APIs compose with the existing sync APIs.

### 8.4 CI matrix

Every async library test must run on both backends:
`./scripts/test.sh --tcc --libs` and `./scripts/test.sh --llvm --libs`.
Negative API-misuse tests (`await` outside async, missing `async` library)
already live in `--negative`.

---

## 9. Compatibility risk register

| # | Risk | Impact | Mitigation |
|---|---|---|---|
| **R1** | `http::Server::serve_async` already means "serve on a thread" | reusing the name silently changes behaviour | use a new name for the coroutine loop; keep the thread variant |
| **R2** | `std::concurrent::Future<T>` vs `core::async::Future<T>` (B5) | name confusion, accidental unification | never `using core::async::Future` unqualified in `std`; keep both; add an explicit `await` bridge if needed |
| **R3** | `net::Socket = usize` is a bare int | reactor needs per-fd state | wrap, don't replace: `AsyncSocket { fd, registration }` |
| **R4** | `tls` calls `net::send_all`/`recv_all` directly | transport swap is invasive | function-pointer transport on `SSLContext`; default = current sync calls; guard with all existing `--tls` tests |
| **R5** | `FutureHandle<T>` is move-only | accidental copies fail | document move-only; `block_on` consumes by value |
| **R6** | no `Send`/auto traits; per-thread executor (D7) | cross-thread migration unsound | document thread affinity; route cross-thread work through `spawn_blocking`/channels |
| **R7** | interpreter/comptime and async closures unsupported | library async code can't run in comptime; closures must be named | document; keep async out of interpreter-only paths |
| **R8** | adding `import async` to libs grows the dependency graph and binary | compile time / size | runtime is opt-in per package; `core` protocol stays dependency-free |

---

## 10. Sequencing

| Milestone | Deliverable | Unblocks | `--libs` coverage |
|---|---|---|---|
| **M0** | this assessment | — | — |
| **M1** | `async::test::block_on` + deterministic tests | hermetic async testing | async func / await / cancellation |
| **M2** | executor, `spawn`, timers, `select`, `spawn_blocking`, reactor | all I/O tiers | spawn/select/timeout/spawn_blocking |
| **M3** | `net::AsyncSocket` (+ POSIX poll, IOCP reuse) | TLS/HTTP | loopback (gated) |
| **M4** | `tls` transport vtable + `*_async` | HTTP client | `--tls` async tests |
| **M5** | `http` `request_async` + coroutine accept loop + streaming | server/minlsp/ide | integration (gated) |
| **M6** | `fs`/`process`/`environment` `spawn_blocking` wrappers | user apps | `--libs` |
| **M7** | `webview`/`window` `spawn_local` | UI async | `--webview` |

M1 is shippable immediately: the compiler already supports everything it needs,
and it changes **no** public API.

---

## 11. Open questions

1. **One `--libs` or a separate `--async-io` suite?** Network tests are not
   hermetic. Recommendation: keep hermetic async tests in `--libs`; put
   socket/TLS tests in the dedicated suites.
2. **Unify `std::concurrent::Future` with `core::async::Future`?** Recommendation:
   no; keep the thread-pool `Future` as the blocking primitive and expose
   `spawn_blocking` as the bridge.
3. **Executor API shape.** `spawn` returning a join `FutureHandle`, or
   fire-and-forget? `select` as a free function or a builder? Decide in M2.
4. **`Environment` async value.** Is `environment` worth async wrappers, or is
   `spawn_blocking` at the call site enough?
5. **TLS transport ownership.** Should the transport vtable live in `core`/`net`
   (shared with a future `io` abstraction) or stay inside `tls`?
6. **`--libs` import cost.** Adding `async` (and later `net`/`http`) to the lib
   suite increases build time; measure before making it unconditional.

---

## 12. Compiler blockers for hand-authored generic futures

Building the runtime exposed three concrete compiler gaps that block the
generic layers (`spawn_blocking<T>`, `timeout<T>`, the executor's awaitable
`JoinHandle<T>`, and any hand-authored `FutureTable<T>`). They are independent
of the runtime design and must be fixed in the compiler.

### B10 — Generic function *references* are not parsed or instantiated (HIGH)

`foo<int>` used as a value (not a call) does not instantiate `T`.

```chemical
public func <T> ident(x : T) : T { return x }
var g : (x : int) => int = ident<int>   // ERROR: value with type '(x : T) => T'
                                        // does not satisfy '(x : int) => int'
```

- Parser: `parser/utils/Expression.cpp:415-444` has a **commented-out** block for
  `ident<...>` in a chain; `parser/statements/AccessChain.cpp:405-434` consumes
  the generic list but only handles `{` (struct value) and `(` (call). A bare
  reference falls into `default` and **drops the generic arguments**.
- `VariableIdentifier` has no place to store generic arguments. Proper support
  needs a new representation + symres instantiation + codegen (LLVM/2c/interp).
- Without it, a runtime future cannot name a per-`T` poll function, so every
  generic future table must be built with inline non-capturing lambdas instead.

### B11 — Generic function *type* equality fails with a generic variant result (HIGH) — ✅ FIXED

An inline non-capturing lambda assigned to a `FutureTable<T>` field was rejected
even though the printed types were identical:

```chemical
vtbl.poll = (frame : *mut void, cx : *mut Context) => { ... Poll<T> ... }
// was: value with type '(frame : *mut void, cx : *mut Context) => Poll<T>'
//      does not satisfy type '(frame : *mut void, cx : *mut Context) => Poll<T>'
```

Root cause: `GenericType::is_same`/`satisfies` first called `canonical()` on the
other type, which can unwrap a `GenericType` to a `LinkedType`
(`BaseType::canonical`, case `Generic`), so the same-declaration branch was never
reached. Two fixes in `ast/types/GenericType.cpp`:

1. Prefer the direct `Generic` type; only fall back to `canonical()` for
   wrappers (literals/references/aliases).
2. Normalize `referenced->linked` to the owning generic declaration
   (`Generic*Decl`) via `canonical_generic_decl()`, since `Foo<T>` may reference
   the generic wrapper, its master implementation, or a concrete instantiation.

This makes `Foo<T> == Foo<T>` and `&Foo<T> == &Foo<T>` hold regardless of which
node each side references; the main suites stay green (2186/2187). It does not,
by itself, unblock the runtime because B12 and the LLVM function-typed-field
crash remain for hand-written generic vtables.

### B12 — Generic structs with generic function-typed fields are not specialized (2c) (MEDIUM)

```chemical
public struct FT<T> {
    var poll : (frame : *mut void, cx : *mut Context) => Poll<T>
}
@retained public func <T> make() : FT<T> {
    return FT<T> { poll : (frame, cx) => Poll.Pending<T>() }
}
// [2cTranslation] error: generic type parameter not specialized ...
```

Declaring/`zeroed`-ing `FT<int>` is fine; the failure needs a generic function
that **constructs the struct and assigns a lambda** to the function-typed field.

Root cause (instrumented): `link_lambda` sets the lambda's `returnType` to the
**shared master field** `FunctionType` (`Poll<FT_T>`). During instantiation,
`GenericInstantiator::make_gen_type_concrete` maps `FT_T -> make_T` (the outer
param) and does **not** follow the chain to the concrete `int`, because nested
instantiators created by `newGenericInstantiatorFrom` do **not** inherit the
parent's `active_type_map`. The shared master `FunctionType` is also mutated in
the `make<T>` master context (`FT_T -> make_T`), which then leaks into the
concrete instantiation. Fixing this needs either (a) chain/cross-instantiator
resolution of `active_type_map`, or (b) never sharing the master field
`FunctionType` with a lambda's expected type. Both are invasive; not attempted
here.

### B17 — Flaky LLVM compiler crash under parallel codegen with async modules (HIGH)

Running `./scripts/test.sh --llvm --libs` repeatedly is non-deterministic with
the async/environment wrappers present: sometimes 624/624, sometimes the compiler
aborts with `RUNTIME ERROR: invalid memory access` after emitting `lib_tests`.
This is a race in parallel module codegen (the same class as the `B12`-repro
LLVM crash), not in the generated program. The async modules increase the number
of generic instantiations emitted concurrently, raising the hit rate. Needs a
look at the parallel `ASTProcessor`/generic-instantiation synchronization.

### B13 — Retention friction for generic runtime code (LOW)

Calling a non-`public` (internal) function from a `public` generic declaration is
an error (`calling a non-retained function in a public generic declaration`). The
runtime worked around it by making helpers `public` and wrapping the
`ThreadPool::submit_void` call in a non-generic shim. Worth documenting for
library authors; not a hard blocker.

### B14 — Capturing lambda passed to a non-generic function inside a generic async func crashes LLVM (HIGH)

The following shape crashes the LLVM compiler (`RUNTIME ERROR: invalid memory
access` in `child_of_self_ptr`/`CreateGEP`, reached from
`ImplDefinition::code_gen_bodies`):

```chemical
@retained
public async func <T> run_on_pool(pool : *mut ThreadPool, f : std::function<() => T>) : T {
    var st = malloc(sizeof(BlockingState<T>)) as *mut BlockingState<T>
    new(st) BlockingState<T>()
    submit_blocking(pool, |st|() => {   // closure captures generic-typed state
        st.value = 99
        st.done = true
    })
    ...
}
```

Reproduced minimal variants: capturing *any* generic-typed state in a closure
that is passed to a non-generic function from inside a generic `async func`.
Removing the closure (direct assignment) compiles; a generic `async func` alone
compiles; a generic `async func` taking `std::function<() => T>` compiles. The
same prototype **compiles and runs on TCC**, so `spawn_blocking<T>` is blocked on
LLVM only.

**Consequence.** Until B10/B14 land, `spawn_blocking<T>` cannot ship (LLVM is
the default backend). The near-term path for `fs`/`process` stays the
`async func`-body pattern with direct (non-closure) state updates, or a native
reactor free of thread-pool closures.

### B15 — Async functions broke LLVM `debug_complete` (HIGH) — ✅ FIXED

Any `async func` generated invalid LLVM debug info in `--mode debug_complete`
(`location requires a valid scope`, `local variable requires a valid scope`),
because the coroutine body is emitted via `code_gen_no_scope` (no
`DISubprogram` pushed) and, once split, cloned resume/destroy functions carry
locations whose scope is the compile-unit file. This made it impossible to add
async wrappers to libraries in the main dependency graph (their `async func`s
are emitted even when unused), and `--async --mode debug_complete` failed.

Fix: `FunctionDeclaration::code_gen_body` disables `gen.di` for the entire async
lowering (`gen_llvm_async_fn`), so async bodies/frames/poll/drop emit no debug
locations. Async debug info can be restored once the coroutine split carries
per-clone `DISubprogram`s. Verified: `--async --mode debug_complete` 33/33 and
main `--mode debug_complete` 2187/2187.

### B16 — Async func returning a nested destructible aggregate is miscompiled (HIGH)

An `async func` that returns a deeply nested destructible struct and is called
from a **non-`main`** function crashes LLVM codegen (`RUNTIME ERROR: invalid
memory access`), and in the `--libs` test harness the same call instead
double-frees at runtime (`free(): double free detected in tcache 2`).

```chemical
async func execute_async(cfg : ProcessConfig) : PR_Result { return execute(cfg) }
// PR_Result = Result<ProcessResult, ProcessError>
// ProcessResult = { ProcessOutput{vector<u8>, vector<u8>}, ExitStatus{...}, bool }
```

Simpler returns are fine: `Result<vector<u8>, FsError>` (fs) and
`Result<UnitTy, ProcessError>` work. This blocks `process::execute_async` /
`wait_async` / `try_wait_async`, so Tier 4 `process` is not shipped. Root cause
is in the async return-value path for aggregate results and needs the same kind
of investigation as B9.

**Consequence.** Until B10/B12/B14 land, the runtime can only create
`FutureTable` entries for *concrete* `T` (as `yield_now`/`sleep` do with
`Unit`). A generic `async func` body *can* drive a generic state struct (this was
prototyped for `spawn_blocking<T>` and runs on TCC), but the thread-pool closure
path crashes LLVM (B14). The pragmatic near-term path is to keep generic
combinators (`spawn_blocking<T>`, `timeout<T>`) out of the runtime and implement
library async wrappers with compiler-lowered `async func` bodies instead (this
is now done for `fs`, §7 Tier 4).

---

## Appendix A — Additive API sketch (illustrative only)

```chemical
// runtime (lang/libs/async) — NEW, additive
public namespace async {
    public func <T> spawn(handle : core::async::FutureHandle<T>) : core::async::FutureHandle<T>
    public func <F, T> spawn_blocking(f : F) : core::async::FutureHandle<T>
    public func sleep_ms(ms : u64) : core::async::FutureHandle<core::async::Unit>
    public func <T> timeout(handle : core::async::FutureHandle<T>, ms : u64) : core::async::Poll<T>
    public namespace test {
        public func <T> block_on(handle : core::async::FutureHandle<T>) : T
    }
}

// net — NEW type, existing functions untouched
public namespace net {
    public struct AsyncSocket { /* fd + reactor registration */ }
    public func dial_async(addr : *char, port : uint) : core::async::FutureHandle<Socket>
    public func accept_async(s : Socket) : core::async::FutureHandle<Socket>
    public func recv_async(s : Socket, buf : *mut u8, cap : usize) : core::async::FutureHandle<int>
    public func send_async(s : Socket, p : *char, len : int) : core::async::FutureHandle<int>
}

// http — mirrors existing signatures, same result types
public namespace http {
    public struct Client {
        // existing: request(&self, rb : &RequestBuilder) : std::Result<Response, std::string>
        public func request_async(&self, rb : &RequestBuilder)
            : core::async::FutureHandle<std::Result<Response, std::string>>
    }
}

// fs — spawn_blocking bridge, same Result type
public func read_entire_file_async(path : *char)
    : core::async::FutureHandle<std::Result<std::vector<u8>, FsError>>
```

## Appendix B — Files inspected

- `lang/libs/core/async.ch`, `lang/libs/async/src/{main,frame,block_on}.ch`
- `lang/libs/net/{chemical.mod,src/main.ch,src/api.ch,posix/platform_api.ch,win/iocp.ch}`
- `lang/libs/tls/src/{ssl.ch,types.ch}`
- `lang/libs/http/src/{client.ch,server.ch,parser.ch,types.ch}`
- `lang/libs/fs/src/file_io.ch`, `lang/libs/process/src/process.ch`
- `lang/libs/std/src/concurrency/threadpool.ch`
- `lang/libs/components/src/Suspense.ch`
- `lang/tests/libs/chemical.mod`, `lang/tests/libs/main.ch`
- `lang/tests/build.lab`, `scripts/test.sh`
- `lang/docs/async-await-design.md` (§1.9, F1–F12, D1–D14)
- `lang/docs/library-ecosystem-analysis.md`
