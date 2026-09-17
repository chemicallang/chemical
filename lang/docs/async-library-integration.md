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
| `block_on<T>(handle) : T` | ✅ | bootstrap executor; also drains the task queue and parks (condvar, ~1ms) on `Pending` |
| `chemical_async_frame_alloc` / `_free` | ✅ | `malloc`/`free` default |
| `yield_now() : FutureHandle<Unit>` | ✅ | first hand-authored runtime future (no reactor) |
| `sleep(millis) : FutureHandle<Unit>` | ✅ | poll-driven clock future |
| `test::block_on<T>` | ✅ | deterministic executor for `--libs` tests |
| `spawn<T>` + task queue + executor | ✅ | `lang/libs/async/src/exec.ch` — process-wide polling executor; join handle, cancellation; `spawn_local` alias |
| `spawn_blocking<T>` | ✅ | `lang/libs/async/src/blocking.ch` — submits to a shared lazily-created `std::concurrent::ThreadPool`; cancellation-safe; see B19 for capture/result limits |
| `select<T>` / `timeout_or<T>` / `block_on_timeout<T>` | ✅ | `lang/libs/async/src/combinators.ch` — same-payload-typed combinators (B20); no `Result`/`Either`-typed vtable |
| Reactor / fd readiness | ✅ | `lang/libs/async/src/reactor.ch` + `posix/reactor.ch` (`select(2)`) / `win/reactor.ch` stub — `readable`/`writable`/`AsyncFd`; epoll/kqueue and `net` wiring are Tier 1 |
| `channel<T>` (mpsc) | ✅ | `lang/libs/async/src/channel.ch` — `send`/`close`/`try_recv`/`recv_or`/`clone_sender`; linked-list queue |
| Executor wake hook | ✅ | `ExecWakeHook` + `executor_set_wake_hook` (`lang/libs/async/src/exec.ch`) — lets a native event loop post off-thread wakeups to itself (Tier 5) |

**Landed.** Runtime: `async::{yield_now, sleep, test::block_on}`, a parking
`block_on`, an executor with `spawn<T>`/`spawn_local<T>`, the `select` /
`timeout_or` / `block_on_timeout` combinators, `channel<T>`, the fd reactor
(`readable`/`writable`/`AsyncFd`), and `spawn_blocking<T>` (F4/F5) — a
cancellation-safe bridge over a shared, lazily-created
`std::concurrent::ThreadPool` (`lang/libs/async/src/blocking.ch`).
Compiler fixes B10, B11, B12, B14, B15, B16, B17, B19, B21, B22 plus a 2c
nested-lambda fix (a lambda declared inside an `async func` no longer inherits
the coroutine state, so its `return` is emitted as an ordinary synchronous
return — the 2c analogue of B14). B20 (a vtable whose type argument is a
composite generic) is worked around, not fixed.

Library integration (all additive):
- `fs::{read_entire_file_async, write_text_file_async, atomic_write_async}` —
  bodies offload the syscall to `spawn_blocking`, so disk I/O no longer stalls
  the executor.
- `environment::{get_async, set_async, unset_async}` — direct bodies (cheap
  syscalls; avoids capturing a `string_view` into a worker thread).
- `process::{execute_async, spawn_async, wait_async}` — offload to
  `spawn_blocking`; `try_wait_async`, `kill_process_async`,
  `write_stdin_async`, `close_stdin_async`, `is_running_async` keep direct
  bodies (quick syscalls); `sleep_ms_async` is timer-backed and genuinely
  suspends.

Tests: `--libs` on **both** backends — 6 async runtime + 10 executor/combinator +
5 channel + 3 reactor + 3 fs + 1 environment + 8 `spawn_blocking` = **650/650**;
`--process` (dedicated suite) — 6 async tests, **127/127** on both backends;
`--async` (dedicated; includes the loopback `net` test) **34/34**; `--tls`
(dedicated; includes the async TLS handshake and async HTTP loopback tests)
**573 TCC / ~570 passed** (2 pre-existing `BAD_SIGNATURE` failures plus harness
flakiness under parallel dispatch; the async tests pass in isolation on LLVM);
`--webview` (dedicated; includes the 5 Tier 5 UI-async tests) **47/47** on both
backends; `--server` (dedicated; the 2 Tier 6 file-server tests) **2/2** on both
backends; main suite green (2190 TCC / 2191 LLVM); interpret 1811/1811.

### 3.4 Libraries

| Library | Current API (must be preserved) | Blocking on | Async benefit | Tier |
|---|---|---|---|---|
| `net` | `dial`, `listen_addr`, `accept_socket`, `recv_all`, `send_all`, `close_socket`, `set_recv_timeout`, `set_keep_alive`; `Socket = usize` | connect/accept/read/write | many sockets on one thread | 1 (**done**: `AsyncSocket`, `dial_async`/`accept_async`/`recv_async`/`send_async`) |
| `tls` | `tls_connect`, `ssl_handshake`, `ssl_read`, `ssl_write`, `ssl_close_notify`, `ssl_free`, `ssl_set_socket`, `tls_accept` | full multi-RTT handshake + records, calling `net::send_all`/`recv_all` directly | overlap handshakes; async record I/O | 2 (**done**: `tls_connect_async`/`ssl_handshake_async`/`ssl_read_async`/`ssl_write_async`/`tls_accept_async`) |
| `http` | `Client::request/get/post/put/patch/delete/head`; `Server::serve`, `Server::serve_async` (thread), `Body::read/read_to_string/read_exact/drain/close`; parsers `read_request_incremental`, `read_response_incremental` | dial → TLS → send → incremental read; worker-thread accept loop | `request_async`, per-connection tasks, streaming bodies | 3 (**done**: `request_async`/`get_async`/…, `serve_coro`, `read_chunk_async`) |
| `fs` | `read_entire_file`, `atomic_write`, `read_to_buffer`, directory ops | disk syscalls | don't stall the executor | 4 |
| `process` | `execute`, `spawn`, `wait`, `try_wait`, `write_stdin`, `close_stdin`, `kill_process`, `is_running`, `sleep_ms` | child + pipes | awaitable child/pipes; `test_env` IPC | 4 |
| `environment` | get/set env | syscalls | `spawn_blocking` only | 4 |
| `webview`, `window` | UI-thread event loops (GTK3/WebKit2GTK) | host loop | background async work, resume on UI | 5 (**done**: `spawn_local` + `window_pump`/`window_run_async` + executor wake hook) |
| `server` | file-server runtime: `Server::serve` (blocking accept loop) | accept loop | coroutine accept loop | 6 (**done**: `serve_files_async` + `main --async`) |
| `minlsp`, `ide` | **compiler-side / CBI** (analyzer hooks), not runtime | — | — | out of scope |
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
  `--process`, `webview` (Tier 5 UI async) → `--webview`, `server` (Tier 6
  coroutine file server) → `--server`.
- `lang/tests/process/src/async_test.ch` holds the `process` async wrapper tests
  (5 tests) in the dedicated `--process` suite (126/126 on TCC and LLVM).
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
  in `@test`). ✅
- `Executor` + task queue + `spawn<T>(FutureHandle<T>)`; process-wide default
  (design D7). ✅ (`lang/libs/async/src/exec.ch`)
- `sleep` ✅; `timeout`/`select` ✅ as `combinators.ch` — `select<T>(a, b)`
  (homogeneous race), `timeout_or<T>(h, millis, fallback)` (cancels on timeout),
  and the synchronous `block_on_timeout<T>`. They return the child's payload
  type directly, because a vtable whose argument is a *composite* generic
  (`FutureTable<Result<T, E>>`) is not yet supported (B20).
- `spawn_blocking<T>(f : std::function<() => T>) : FutureHandle<T>` over
  `ThreadPool::submit` — ✅ landed (`lang/libs/async/src/blocking.ch`).
- `AsyncSocket`-supporting reactor. ✅ `lang/libs/async/src/reactor.ch` with
  `posix/reactor.ch` (`select(2)`, chosen over `poll` to avoid a C-symbol
  clash with the `test` library) and a `win/reactor.ch` stub. `readable(fd)` /
  `writable(fd)` / `AsyncFd` are awaitable readiness futures; the executor
  blocks on the reactor while fds are registered. epoll/kqueue and the `net`
  wiring are Tier 1.
- Current-thread mode + `spawn_local` (for F8/UI libs). ✅ `spawn_local` submits
  to the process-wide executor; the UI-thread driver + wake hook landed with
  Tier 5 (`window_run_async`/`window_pump`).
- A `channel` (M6). ✅ `lang/libs/async/src/channel.ch` — multi-producer /
  single-consumer, `send`/`close`/`try_recv`/`recv_or`/`clone_sender`. `recv_or`
  carries the caller's fallback for the closed case because a bare
  `recv() : FutureHandle<Option<T>>` needs a composite-generic vtable (B20).

### Tier 1 — `net` — ✅ DONE

`lang/libs/net/src/async.ch` (new), additive — every synchronous function is
untouched. `net` now imports `core` + `async`.

- `net::AsyncSocket` (`valid`/`raw`/`close`/`read`/`write`/`accept`).
- `async_listener(addr, port) : AsyncSocket` (sync bind+listen),
  `dial_async(addr, port)`, `accept_async(listener_fd)`, `recv_async(fd, buf,
  cap)`, `send_async(fd, data, len)`.
- POSIX: `accept`/`recv`/`send` set the socket non-blocking and retry after
  `await async::readable`/`writable` (EAGAIN/EINTR), so a single executor drives
  many sockets. Connect and accept are offloaded to the thread pool (connect is
  an inherently blocking kernel call). `set_nonblocking` already existed.
- Windows: the whole API falls back to the thread pool (its reactor is a stub);
  real IOCP integration is still open.
- **Futures carry raw fds (`int`), not `AsyncSocket`.** This started as a B23
  workaround (now fixed); it is kept as the ABI because `int` handles are the
  simplest payload. `AsyncSocket` is a synchronous wrapper over the fd.
- Tests: `lang/tests/async/net_test.ch` (loopback echo) in the dedicated
  `--async` suite (not `--libs`, which stays hermetic).
- Still open for a follow-up: an epoll/kqueue registration for scale (the
  reactor is `select(2)`, ~1024 fds) and the Windows IOCP path.

### Tier 2 — `tls` — ✅ DONE

`lang/libs/tls/src/async.ch` (new), additive — the synchronous API is unchanged.
`tls` now depends on `async` (see the build-system note below).

- `tls_connect_async(ssl, host, port)`, `ssl_handshake_async(ssl)`,
  `ssl_read_async(ssl, buf, len)`, `ssl_write_async(ssl, data, len)`,
  `tls_accept_async(sock, cert, key)`.
- All offload their blocking work to the runtime thread pool via
  `async::spawn_blocking` (the handshake is multi-RTT and record I/O blocks),
  so the executor keeps running and many handshakes overlap, bounded by pool
  size. Pointer/int payloads (B23 is fixed; B24 still applies to 2c struct
  parameters).
- **Deferred:** the transport vtable (`net::send_all`/`recv_all` →
  function pointers on `SSLContext`). A non-blocking transport can only suspend
  if the handshake itself is a coroutine state machine, which the synchronous
  library is not structured for; `spawn_blocking` gives the v1 concurrency.
  The direct calls remain at `tls/src/ssl.ch:2099,2106`.
- Tests: `lang/tests/tls/src/async_test.ch` (real TLS 1.3 handshake + request
  over the async API) in the dedicated `--tls` suite. TCC full suite:
  572 tests / 570 pass (2 pre-existing `BAD_SIGNATURE` failures); the async
  test passes on both backends (verified in isolation on LLVM — the LLVM full
  `--tls` run is flaky under parallel test dispatch).

> **Build-system note:** `lang/libs/tls/` has a hand-written `build.lab` that
> lists module dependencies explicitly; adding a dependency to `chemical.mod`
> alone is not enough — it must also be added to `build.lab` (deps array +
> span count). This is why `tls` initially could not see the `async` namespace.
- Highest value: the handshake is multi-RTT, so overlapping it is the big win.

### Tier 3 — `http` — ✅ DONE

`lang/libs/http/src/async.ch` (new), additive — the synchronous client/server
are unchanged. `http` now imports `core` + `async`.

- **Client:** `request_async`/`get_async`/`post_async`/`put_async`/`patch_async`/
  `delete_async`/`head_async`. Each offloads the whole blocking exchange to the
  pool. They resolve with a `*mut HttpResult` — a flat, heap-allocated
  `{ ok, status, status_text, headers, body, error }` box with non-moving
  accessors (`is_ok`/`status_code`/`body_view`/`error_view`). This is *not* a
  future of `Result<Response, std::string>` as originally sketched: a large
  result variant returned through a `FutureHandle` is corrupted on LLVM (B26),
  and the flat box also avoids moving a destructible payload out of a variant
  (B18). The caller owns the box (`delete`).
- **Server:** `server::serve_coro(srv, port)` — a coroutine accept loop (R1:
  `serve_async` is the thread variant). It observes `Server::run` for shutdown
  via a bounded blocking `accept` on the pool; a spawned coroutine cannot await
  a combinator (`timeout_or`/`select`) on LLVM today (B25). Each accepted
  connection is handled on the pool (`spawn_blocking`), so the loop never blocks
  on a handler.
- **Streaming:** `read_chunk_async(b, dst, cap)` / `body_read_async` /
  `read_to_string_async` (design F12) offload `Body::read`.
- Tests: `lang/tests/tls/src/http_async_test.ch` (coroutine server + async
  client loopback) in the dedicated `--tls` suite; passes on both backends
  (LLVM verified in isolation).

### Tier 4 — `fs`, `process`, `environment`

- `spawn_blocking` wrappers first (works before the reactor):
  `fs::read_entire_file_async`, `fs::atomic_write_async`,
  `process::wait_async`, `process::execute_async`, async pipe read/write.
- Keep the sync functions identical; share the same syscall helpers.
- **Landed (true `spawn_blocking`):** `fs::read_entire_file_async`,
  `fs::write_text_file_async`, `fs::atomic_write_async`
  (`lang/libs/fs/src/async.ch`) now `await async::spawn_blocking(...)`, so the
  disk I/O runs on the pool and the executor keeps running. `fs` imports `async`.
  Tests: 3 in `lang/tests/libs/fs/async_test.ch` (630/630 on both backends).
- **Landed (v1 direct bodies):** `environment::{get_async, set_async,
  unset_async}` (`lang/libs/environment/src/async.ch`) — symmetric awaitable
  surface; kept direct because the syscalls are cheap and an async body would
  capture a caller `string_view` into a worker thread.
- **Landed (true `spawn_blocking`):** `process::{execute_async, spawn_async,
  wait_async}` (`lang/libs/process/src/async.ch`) now `await
  async::spawn_blocking(...)`, capturing the `ProcessConfig`/child pointer into
  the task and moving the nested `ProcessResult` back out of the shared state.
  `wait_async` reuses `wait` rather than polling `try_wait`, because POSIX
  `try_wait` reaps without capturing stdout/stderr; this preserves the
  synchronous result exactly (including wait-after-kill). The remaining process
  entry points (`try_wait_async`, `kill_process_async`, `write_stdin_async`,
  `close_stdin_async`, `is_running_async`) are quick non-blocking syscalls and
  keep direct bodies; `sleep_ms_async` is timer-backed and genuinely suspends.
  Tests: 6 in `lang/tests/process/src/async_test.ch` (127/127 both backends).
- **Done:** `fs`, `environment` and `process` are all integrated. The remaining
  Tier 4 work is only the reactor-backed I/O (Tiers 1–3).

### Tier 5 — `webview`, `window` — ✅ DONE

The async executor is a cooperative poll loop; a GUI app is parked in the native
event loop (`gtk_main` / `GetMessage`), so `async::spawn_local` alone would never
be polled. Tier 5 couples the executor to the native loop so a UI-spawned task is
polled on the UI thread — and therefore the continuation after an `await` runs on
the UI thread too. No callback types, no "await the UI": background work runs on
`spawn_blocking` and the resume happens on the UI thread.

- `async::Executor` gained a wake hook
  (`ExecWakeHook`, `executor_set_wake_hook`); `exec_wake` runs it outside the
  executor lock. A native loop installs one so an off-thread completion wakes the
  loop immediately instead of waiting for the next tick
  (`lang/libs/async/src/exec.ch`).
- `window::{window_pump, window_run_async}` (`lang/libs/window/src/async.ch`,
  new). `window_pump` polls the executor and drains pending native events once;
  `window_run_async` installs the wake hook + a 1 ms executor tick and runs the
  native loop. The platform hooks live in `posix/linux.ch` (GTK `g_idle_add` /
  `g_timeout_add` / `gtk_main_iteration`) and `win/win.ch` (`WM_NULL` wake /
  `SetTimer` tick / `PeekMessageW` drain).
- `window` now imports `core` + `async`; `webview::{webview_run_async}` mirrors
  `webview_run` for apps whose bridge handlers await background work.
- Usage: submit with `async::spawn_local(future())`, then run the loop with
  `window_run_async` (or `webview_run_async`). Blocking work must still be
  wrapped in `async::spawn_blocking`; the executor tick then resumes the task on
  the UI thread.
- Tests: `lang/tests/webview/src/async_tests.ch` (5 tests) in the dedicated
  `--webview` suite — an eager and a suspending `async func` in the same module,
  `window_pump` drives the suspended coroutine to completion, the join result is
  readable, cancellation of dropped handles is clean, and `window_run_async`
  drives the executor from a real GTK loop. **47/47 on TCC and LLVM.**
- **B27** (an `async func` leaving `current_function` set, so globals declared
  after it in the same file were emitted without an initializer and crashed
  LLVM) was found here and **fixed** in `gen_llvm_async_fn`. The tests use both
  an eager and a suspending `async func` in the same module.

### Tier 6 — `server` — ✅ DONE (`minlsp`/`ide` out of scope)

**`minlsp` and `ide` are compiler-side / CBI consumers, not runtime libraries.**
`ide` imports `compiler` and declares `interface LSPAnalyzers`; `minlsp` is a set
of token-type declarations; both are consumed by the
`html_ide`/`js_ide`/`css_ide`/`md_ide`/`universal_ide` CBI plugins and run inside
the analyzer pass on the compiler/LSP thread. They have no sockets or disk I/O of
their own to suspend on, so the runtime async layer does not apply to them (and
adding `import async` to a CBI plugin would only grow the plugin). They are
removed from the runtime-async rollout.

**`server` is the runtime file-server library.** Its synchronous `Server::serve`
blocks the calling thread in an accept loop (`serve_non_iocp`); IOCP is used on
Windows. Tier 6 adds the additive coroutine path:

- `server::serve_files_async(srv, root, port)` (`lang/libs/server/src/async.ch`,
  new) — adds the same static-file routes as `main` and drives the Tier 3
  `server::serve_coro` accept loop. Drive it with
  `async::block_on<int>(...)` or submit it with `async::spawn<int>(...)` and stop
  it with `srv.shutdown()`.
- `main --async` selects it (the synchronous `serve` path stays the default).
- `server` now imports `core` + `async`.
- **Two pre-existing `server`/`http` bugs fixed while wiring this up (B28):**
  `Server::start` and `FileServer::serve_http` compared a `size_t` `find()`
  result against `-1u` (32-bit), which on 64-bit is *never* equal to `NPOS`. As a
  result `Server::start` read out of bounds for any `addr` without a `':'`
  (including the documented default `0.0.0.0`) and `serve_http` returned `403`
  for every path. Both now compare against `std::NPOS`.
- Tests: `lang/tests/server/` (`@test
  INT_server_async_file_get`/`INT_server_async_404`) in a dedicated `--server`
  suite. **2/2 on TCC and LLVM.**

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
| **M2** | executor, `spawn`, timers, `select`, `spawn_blocking` ✅, reactor | all I/O tiers | spawn/select/timeout/spawn_blocking |
| **M3** | `net::AsyncSocket` (+ POSIX poll, IOCP reuse) | TLS/HTTP | loopback (gated) |
| **M4** | `tls` transport vtable + `*_async` | HTTP client | `--tls` async tests |
| **M5** | `http` `request_async` + coroutine accept loop + streaming | server/minlsp/ide | integration (gated) |
| **M6** | `fs`/`process`/`environment` `spawn_blocking` wrappers (additive `*_async` signatures landed; bodies become non-blocking here) | user apps | `--libs` / `--process` |
| **M7** | `webview`/`window` `spawn_local` + UI-thread executor ✅ | UI async | `--webview` 47/47 |
| **M8** | `server::serve_files_async` + `main --async` ✅ (`minlsp`/`ide` are CBI, out of scope) | runtime file server | `--server` 2/2 |

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

Building the runtime exposed concrete compiler gaps that block the generic
layers (`spawn_blocking<T>`, `timeout<T>`, the executor's awaitable
`JoinHandle<T>`, and any hand-authored `FutureTable<T>`). They are independent
of the runtime design and must be fixed in the compiler. B10, B11, B12, B14,
B15, B16, B17, B19, B21, B22 and B23 are **fixed**; B13 and B18 are
by-design/documented; and B20 (composite generic arguments in a generic body),
B24 (field access on a struct-typed async parameter), B25 (a spawned coroutine
awaiting a combinator) and B26 (a large struct variant through a future) are
**worked around** — the runtime keeps composite generics out of vtable types,
keeps large structs out of future payloads (futures carry ints/pointers), keeps
struct parameters out of coroutine bodies, and avoids combinator awaits inside
spawned coroutines. B27 (globals after an `async func`) and B28 (`size_t find()`
vs `-1u` in the server runtime) were found later and are **fixed**.

> **Remaining (worked around, not fixed) → actionable worklist:**
> [`async-remaining-work.md`](./async-remaining-work.md). It lists each pending
> item (B20, B24, B25, B26, async debug info, async closures, the TLS transport
> vtable, the Windows IOCP reactor, and the POSIX epoll/kqueue reactor) with
> symptom, root-cause location, current workaround, and a definition of done, in
> a recommended fix order. B23 was fixed from that list (see below).

### B10 — Generic function *references* are not parsed or instantiated (HIGH) — ✅ FIXED

`foo<int>` used as a value (not a call) did not instantiate `T`:

```chemical
public func <T> ident(x : T) : T { return x }
var g : (x : int) => int = ident<int>   // was: value with type '(x : T) => T'
                                        // does not satisfy '(x : int) => int'
```

- Parser: `parser/statements/AccessChain.cpp` consumed the generic list for a
  bare reference then fell into `default` and **dropped the generic arguments**
  (`parser/utils/Expression.cpp` had a commented-out block for the same reason).
- `VariableIdentifier` had nowhere to store generic arguments.

Fix, three layers:

1. **Representation** (`ast/values/VariableIdentifier.h`): a bare reference now
   stores its arguments in `std::vector<TypeLoc> generic_list` (copied by
   `VariableIdentifier::copy`). Calls/struct values keep theirs on the
   `FunctionCall`/`StructValue` as before.
2. **Parser** (`parser/statements/AccessChain.cpp`): when `ident<...>` is not
   followed by `(` or `{`, the parsed arguments are attached to the last
   identifier instead of being discarded.
3. **Symres** (`compiler/symres/SymResLinkBody.cpp`
   `link_generic_func_reference`, and `GenericInstantiator::relink_identifier` /
   `instantiate_generic_func_reference` for generic bodies): the argument types
   are linked, then — unless inside a generic body, where instantiation is
   deferred — the generic function is instantiated via
   `GenericFuncDecl::register_generic_args` and the identifier is relinked to the
   concrete `FunctionDeclaration`. For a reference in a **global initializer**
   (which the body pass skips), the arguments are linked in
   `TopLevelLinkSignature::VisitVariableIdentifier` and registered in
   `GenericInstantiationPass::VisitVariableIdentifier` (with
   `VisitAccessChain` refreshing the wrapping chain's type). Codegen needed no
   change: a concrete function reference already lowers correctly.

Verified on TCC **and** LLVM: `ident<int>` in a non-generic body, in a global
initializer, passed as an argument, `ns::ident<int>` (namespaced), and `ident<T>`
/ `ns::ident<T>` inside a generic function (including multiple parameters).
Regression tests in `lang/tests/src/generic/basic.ch`. Full matrix green (main
2189/2190, libs 624/624, async 33/33, interpret 1811/1811).

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

### B12 — Generic structs with generic function-typed fields are not specialized (2c) (MEDIUM) — ✅ FIXED

```chemical
public struct FT<T> {
    var poll : (frame : *mut void, cx : *mut Context) => Poll<T>
}
@retained public func <T> make() : FT<T> {
    return FT<T> { poll : (frame, cx) => Poll.Pending<T>() }
}
// was: [2cTranslation] error: generic type parameter not specialized ...
```

Declaring/`zeroed`-ing `FT<int>` was fine; the failure needed a generic function
that **constructs the struct and assigns a lambda** to the function-typed field.

Root cause (instrumented): `link_lambda` sets the lambda's `returnType` to the
**master member's** `FunctionType` return type, which is written in terms of the
*container's* generic parameters (`FT_T`). During `make<int>` instantiation the
current instantiator's `active_type_map` only contains the *function's*
parameters (`make_T -> int`); the container is specialized by a nested
instantiator whose map is discarded. `FT_T` was therefore never replaced and
reached 2c unspecialized.

Fix (`compiler/generics/GenericInstantiator.cpp::VisitStructValue`): visit the
value's referenced type first (so a generic container is specialized before its
initializers are visited), then — once the concrete container is known — activate
that container's instantiation (container params → concrete args) in the current
`active_type_map` while visiting the field initializers, restoring the previous
mapping afterwards. This replaces the container's parameters inside function
typed initializers (lambdas) with the concrete types.

Verified: the minimal repro and a real `core::async::FutureTable<T>` built from a
generic function (both TCC and LLVM), a regression test in
`lang/tests/src/generic/basic.ch` (confirmed to fail before the fix), and the full
matrix green (main 2187/2188, libs 624/624, async 33/33).

### B17 — Order-dependent LLVM crash declaring a comptime-const `StructValue` (HIGH) — ✅ FIXED

`./scripts/test.sh --llvm --libs` was flaky (sometimes 624/624, sometimes the
compiler aborted). It was **not** parallel-codegen related (it still happened
with `-j 1`); the build.lab file-order seed randomizes module order, and the
crash depended on that order. Reproduced deterministically with
`FILE_ORDER_SEED=17491728074826263121`.

Root cause: `VarInitStatement::code_gen_external_declare` materialized a
non-string comptime constant by calling `initializer_value(gen)`. For a
`StructValue` initializer that reaches `StructValue::llvm_allocate` →
`IRBuilder::CreateAlloca`, which is invalid during external declaration (no
function/insert block) and segfaults in `BasicBlock::getDataLayout`. It only
triggered when the module referencing the constant was processed before the
module defining it.

Fix (`ast/statements/VarInit.cpp`): only cache the value when there is a
function context (`gen.current_function != nullptr && insert block != nullptr`);
otherwise leave `llvm_ptr` null so the constant is materialized (inlined) at its
use site, as intended. Verified: the crashing seed passes 5/5 and `--libs
--llvm` passes 8/8 with random seeds; full matrix green (main 2186/2187,
main `debug_complete` 2187, libs 624/624, async 33/33).

### B13 — Retention friction for generic runtime code (LOW)

Calling a non-`public` (internal) function from a `public` generic declaration is
an error (`calling a non-retained function in a public generic declaration`). The
runtime worked around it by making helpers `public` and wrapping the
`ThreadPool::submit_void` call in a non-generic shim. Worth documenting for
library authors; not a hard blocker.

### B18 — Moving a non-`Copy` generic value out of a pointed-to struct (LOW / by design)

Not a B12 case, but discovered while validating the runtime's generic vtable. A
generic `FutureTable<T>` poll that reads a `T` field out of the heap frame is
rejected during generic-instantiation symres:

```chemical
var st = frame as *mut FrameState<T>
return core::async::Poll.Ready<T>(st.value)   // moves `st.value` out of the pointee
// error: cannot move this value without re-initializing memory, use std::replace ...
```

The move check is correct — for a `T` with a destructor this would double-free
the field when the frame is dropped. It triggers because the parameter has no
`COPY_BIT` (the master generic body is not specialized with concrete `Copy`
bits). Options for the runtime, not the compiler: bound the parameter
(`<T>` where the concrete types are `Copy`), read through a pointer in an
`unsafe` block, or have the poll mark the payload as taken before returning it.
Documented so the executor's `JoinHandle<T>`/`spawn_blocking<T>` use the
non-moving idiom.

### B19 — `spawn_blocking` with destructible by-value captures / nested aggregate results — ✅ FIXED

Found while converting the Tier 4 wrappers to `spawn_blocking`. Three distinct
bugs were involved.

**1. Generic instantiation dedup dropped type arguments.** `get_iteration_for`
(`ast/utils/ASTUtils.cpp`) compared instantiation args with
`canonical()->is_same(canonical())`. `BaseType::canonical()` unwraps a
`GenericType` to a `LinkedType` referring to the generic's *master* declaration,
so any two instantiations of the same generic compared equal. Concretely,
`FutureHandle<Result<ProcessResult, E>>`, `FutureHandle<Result<ChildProcess, E>>`
and `FutureHandle<Result<UnitTy, E>>` all deduped to one `FutureHandle`
instantiation, so async functions lowered to the wrong `Poll<T>`/`FutureTable<T>`
(`cannot convert 'Result__cgs__30' to 'int'`). Fix: a
`canonicalize_instantiation_arg` helper that unwraps type aliases but keeps (stops
at) a `GenericType` so its arguments still participate in the comparison.

**2. Duplicate module processing.** An app importing both `std` and `process`
could compile the same logical module through two `LabModule` instances (its
`chemical.mod` and its generated `build.lab`), parsing every source twice and
emitting duplicate generic instantiations (`struct ... already defined`). Fix:
`flatten_dedupe_sorted` now de-duplicates by `scope:name`
(`compiler/lab/LabBuildCompiler.cpp`).

**3. `spawn_blocking` result buffer + `std::function` capture.** The old
implementation stored the worker result in a separate `malloc(sizeof(T))` and had
the worker closure capture the submitted `std::function`. Two problems: the
result buffer's first 16 bytes came back overwritten (allocator metadata) and
tearing down the task double-freed the closure's captured `ProcessConfig`
(`std::function` heap captures are shallow-copied). Fix: `BlockingState<T>` now
owns both the task (`task : std::function<() => T>`) and the result
(`value : std::Option<T>`); the submitted closure captures only the state pointer
(a small inline capture), and poll `take`s the value out. No separate result
buffer, no `std::function` capture. (`lang/libs/async/src/blocking.ch`.)

With all three fixed, `process::execute_async`/`spawn_async`/`wait_async` offload
to `spawn_blocking`. Verified: main 2190/2190 (TCC) 2191/2191 (LLVM), interpret
1811/1811, `--libs` 630/630 (TCC and LLVM), `--process` 126/126 (TCC and LLVM),
`--async` 33/33.

### B20 — Field access on a concrete generic type inside a generic body (MEDIUM — worked around)

Found building the Tier 0 executor. Inside a generic function, accessing a field
of a generic type instantiated with a *composite* argument resolves to the
master's field type, not the substituted one:

```chemical
public func <T> spawn_like(h : core::async::FutureHandle<T>) : int {
    var tt = malloc(sizeof(core::async::FutureTable<core::async::Unit>)) as *mut ...FutureTable<...Unit>
    tt.poll = unit_ok           // error: '...Poll<Unit>' does not satisfy '...Poll<T>'
}
```

The type reported is `Poll<T>` — the master `FutureTable` parameter — because in
a generic body `SymResLinkBody::VisitGenericType` only calls `instantiate_inline`
(the full `instantiate` is deferred to the caller's monomorphization), so
`referenced->linked` stays the master and member accesses link against
unsubstituted members. It also affects calls: calling a generic function with a
type argument that contains the caller's generic parameter (or any composite
generic) from within a generic function returned the callee's parameter type
(`GenericFuncDecl::instantiate_call` canonicalized explicit args with
`BaseType::canonical()`, which unwraps an uninstantiated `Foo<X>` to the master
`Foo<T>`). **Partially fixed:** `GenericFuncDecl::instantiate_call` now preserves
`GenericType` arguments. **Not fixed:** the field-access resolution; making
`VisitGenericType` instantiate in a generic context aborts (`unexpected generic
type parameter usage`) on `std` partially-applied generics.

**Workarounds used by the runtime** (and the rule for library authors):
- type-erase task/executor bookkeeping through *non-generic* vtable structs
  (`TaskVTable`, `RawTask` in `exec.ch`);
- write vtable types with a bare generic parameter (`FutureTable<T>`,
  `FutureTable<R>`), never a composite of parameters
  (`FutureTable<Result<T, E>>`). This is why `select`/`timeout_or` return the
  child payload type directly rather than `Either<A, B>`/`Result<T, TimedOut>`.

### B21 — `await` of a *stored* future handle double-dropped it (HIGH) — ✅ FIXED

`var j = spawn(...); var v = await j` destroyed the handle twice. The 2c
lowering emitted `frame.__chx_child_i = j;` as a plain copy and left `j`'s frame
drop flag set, so the child (which the await drops when it resolves) and the
local both dropped the same frame → `invalid memory access` in
`FutureHandle.delete`. Awaiting a *temporary* (`await spawn(...)`) was unaffected
(the temporary has no drop flag). Fix (2c): after storing the child, clear the
consumed operand's frame drop flag via `set_moved_ref_drop_flag`
(`preprocess/2c/2cASTVisitor.cpp`, `emit_async_await_var_init`). Fix (LLVM):
after memcpy-ing the child handle into the frame, null the source handle's
`{frame, vtbl}` fields so its destructor is a no-op
(`compiler/backend/LLVMCoroutine.cpp`, `gen_llvm_await`). Tests:
`lang/tests/libs/async/exec_test.ch::test_async_spawn_await_inside_async`.

### B22 — LLVM `coro.destroy` re-runs the coroutine body after completion (HIGH) — ✅ FIXED

Found building the Tier 0 fd reactor. An `async func` that awaited a future
completing on the second poll and held a small local array crashed on LLVM when
the completed handle was dropped: the custom `__drop` called `coro.destroy`, and
LLVM's split destroy path re-entered the coroutine body (deref'ing a null child
slot). The 2c backend and direct `block_on(future)` (no enclosing coroutine)
were unaffected.

**Fix:** a *completed* coroutine (wrapper state `0xFFFFFFFF`) has no live locals
and does not need its coroutine frame destroyed. `emit_drop_fn`
(`compiler/backend/LLVMCoroutine.cpp`) now routes the `0xFFFFFFFF` case to a
`drop.free` block that frees the coroutine and wrapper frames directly, and
reserves `coro.destroy` for the not-started (`0`) and suspended (`site+1`)
cases.

**Along the way:** `FunctionCall::infer_generic_args` (`ast/values/FunctionCall.cpp`)
indexed `func->params[arg_offset]` without checking `arg_offset < params.size()`,
which aborted the compiler on a call whose argument list is longer than the
parameter list. Guarded. Tests: `lang/tests/libs/async/reactor_test.ch`.

### B23 — A future whose payload is a *plain struct* corrupts it on LLVM (HIGH) — ✅ FIXED

An `async func f() : Pair` (plain struct) driven by `block_on<Pair>(f())` yielded
pointer halves instead of the field values on LLVM (e.g. `a=1929545688 b=32767`),
on both the eager path and the real suspension path. 2c was always correct.

- Variant payloads were fine: `block_on<Result<...>>` / `Option<...>` worked
  everywhere (`fs`/`process` async funcs), as did `spawn_blocking<std::string>`
  and a non-coroutine function returning `Poll<Pair>`. The bug was specific to an
  `async func` (coroutine) whose `T` is a non-variant struct. Returning a
  *variable* was also correct — only `return Pair { ... }` (a `StructValue`)
  corrupted.
- **Root cause (confirmed):** `Codegen::writeReturnStmtFor`'s coroutine branch
  (`compiler/backend/LLVM.cpp`) sent a `StructValue` through
  `value->llvm_value(...)` + `CreateStore`. In a function context
  `StructValue::llvm_value` returns its **alloca pointer**, so the pointer's two
  halves were stored into the frame's struct result field. Opaque pointers make
  `store ptr, ptr` legal, so the verifier did not catch it.
- **Fix:** the coroutine branch now byte-copies every struct-like return value
  into the frame result — `value->llvm_pointer(gen)`, materializing via
  `llvm_value` when it is null (a not-yet-allocated `StructValue`) — mirroring
  the non-coroutine aggregate-return path. Scalars keep the
  `llvm_value`/`implicit_cast`/store path.
- **Regression tests:** `lang/tests/async/struct_payload_test.ch` (eager,
  suspending, and destructor-bearing struct). `--async` is 37/37 on TCC and LLVM.
- The old `net` raw-fd / `http` flat-box workarounds are kept as valid ABI
  choices; `block_on`'s `poll_take_ready<T>` is still needed for B18.

### B24 — Field access on a struct-typed async parameter mis-lowers on 2c (MEDIUM, worked around)

An `async func f(s : SomeStruct)` that reads `s.field` in its body emits
`frame->slot->field` where the frame slot is stored *by value*
(`struct SomeStruct slot;`) — a C compile error (`pointer expected`). Variant and
pointer/primitive parameters are unaffected.

- **Workaround:** pass raw handles (`Socket`, `int`, `*T`) into coroutine bodies
  and wrap them synchronously outside. The `net` async entry points take `int`
  fds for this reason.
- **To fix (compiler):** the 2c async lowering should emit `.field` for
  by-value frame slots (or store struct parameters as pointers consistently).

### B25 — A spawned coroutine that awaits a combinator loses its `Context` on LLVM (HIGH, worked around)

`async::spawn<int>(f())` where `f` awaits `async::timeout_or(...)` /
`async::select(...)` crashes on LLVM: the combinator polls the inner future with
a `Context` whose waker has a garbage `vtbl`, and `ready_poll`/`Waker::clone`
jumps through it. Minimal repro: a spawned coroutine awaiting
`timeout_or<Unit>(readable(fd), ms, Unit)`. The same future awaited directly from
`block_on` works, and `await spawn_blocking` inside a spawned coroutine works.

- **Workaround:** do not await a combinator inside a coroutine that is itself
  polled as a task. `server::serve_coro` uses a bounded blocking `accept` on the
  pool instead of `timeout_or(accept_async(...))`.
- **To fix:** the coroutine `__cx`/`Context` forwarding in
  `compiler/backend/LLVMCoroutine.cpp` (the `Context*` passed to a coroutine
  polled by another future is not preserved when it forwards to a child poll).

### B26 — A large `Result<Response, std::string>` through a `FutureHandle` is corrupted on LLVM (HIGH, worked around)

`block_on<Result<Response, std::string>>(http::get_async(...))` corrupted the
result (the string payload became a garbage pointer and the destructor crashed),
and in some configurations produced a *broken LLVM module*
(`Global is external, but doesn't have external or weak linkage!` for a `tls`
global). Small variants (`Result<int, std::string>`) and `fs`/`process`
`Result<...>` payloads are fine — the trigger is a large struct inside the
variant (`Response` contains a `Body` with `std::string`/buffer state).

- **Workaround:** the async client returns `*mut HttpResult` — a pointer to a
  flat heap box, populated on the pool thread from the `Result`. Pointer
  payloads are unaffected.
- **To fix:** the LLVM async result storage/payload lowering for a large
  struct-shaped `inner_ty` (wrapper `result` field + `Poll<T>` payload load),
  `compiler/backend/LLVMCoroutine.cpp`.

### B27 — `async func` left `current_function` set → invalid globals after it (HIGH) — ✅ FIXED

Found implementing Tier 5 (the `webview` async tests), then reproduced in the
Tier 6 `server` suite as an intermittent LLVM crash: a large module with an
`async func` followed (in the same file) by module-level `var`s SIGSEGVs in
`AlwaysInlinerPass` → `llvm::isInlineViable`:

```
Thread 1 "Compiler" received signal SIGSEGV
#0 llvm::isInlineViable(llvm::Function&) [clone .cold]
#1 (anonymous namespace)::AlwaysInlineImpl(...)
#2 llvm::AlwaysInlinerPass::run(...)
#5 save_as_file_type  compiler/Codegen.cpp:1613
```

**Root cause:** `gen_llvm_async_fn` (`compiler/backend/LLVMCoroutine.cpp`) set
`gen.current_function = ramp` and then restored `prev_func` — but `prev_func` was
captured *after* installing the ramp, so it restored the ramp itself. Every
module-level statement compiled after an `async func` in the same file therefore
ran with a non-null `current_function`. `VarInitStatement::code_gen` branches on
`current_function == nullptr` to decide between a global and a local, so those
globals skipped `global->setInitializer(...)` and were emitted with **no
initializer**:

```llvm
@webview_tests_g_ui_await_value = internal global i32      ; invalid: non-extern global with no initializer
```

`llvm-as`/`opt` rejects that ("global variable reference must have pointer
type" on the *next* line), and the always-inliner crashes on the malformed
module. It only needed a `var` after an async func; the "two coroutines" and
"large module" symptoms were incidental.

**Fix:** `gen_llvm_async_fn` captures the caller's `current_function` at entry
and restores it on every exit path (eager frame, coroutine, early `return false`).
`VarInitStatement::code_gen` also zero-initializes a non-extern global if its
initializer value is null, so a missing initializer can never again emit invalid
IR. Verified: the two-coroutine `webview_tests` repro compiles and its 5 Tier 5
tests pass on LLVM, and the `server` suite no longer flakes.

### B28 — `size_t find()` compared against `-1u` in the server runtime (MEDIUM) — ✅ FIXED

Surfaced by the Tier 6 file server. `std::string::find` returns `size_t` and
signals "not found" with `NPOS` (`(0 as size_t) - 1`, all-ones on 64-bit), but two
call sites compared it against `-1u` (`UINT_MAX`, `0xFFFFFFFF`):

```chemical
var pos = self.cfg.addr.find(":")          // http/src/server.ch, Server::start
if (pos != -1u) { ... substring(0, pos) ... }
if (path.find("..") != -1u) { ... 403 ... } // http/src/static.ch, serve_http
```

On a 64-bit target `NPOS != 0xFFFFFFFF`, so the branch was taken for *every*
string: `Server::start` called `substring(0, NPOS)` and read out of bounds for
any `addr` without a `':'` (the documented default `0.0.0.0`), and `serve_http`
returned `403 Forbidden` for every path. Both now compare against `std::NPOS`.
The synchronous and coroutine paths were equally affected, so the fix is
independent of Tier 6; it simply made the async file-server tests possible.
(`client.ch` already used `std::NPOS` correctly.)

### B14 — Capturing lambda inside a generic async func crashed LLVM (HIGH) — ✅ FIXED

A capturing lambda created inside an `async func` (e.g. the `spawn_blocking`
prototype) crashed the LLVM compiler:

```chemical
@retained
public async func <T> run_on_pool(pool : *mut ThreadPool, f : std::function<() => T>) : T {
    var st = malloc(sizeof(BlockingState<T>)) as *mut BlockingState<T>
    new(st) BlockingState<T>()
    submit_blocking(pool, |st|() => {   // capturing lambda
        st.value = 99
        st.done = true
    })
    ...
}
```

Root cause (found via `llvm::verifyModule` before the pass pipeline): the nested
function generated for the lambda inherited the enclosing async function's
`Codegen::redirect_return` (the ramp's `coro.final` block) and
`Codegen::current_coro`. Its `ret` was therefore emitted as
`br label %coro.final` — a branch to a basic block in a *different* function.
The verifier reported `Referring to a basic block in another function!` and the
optimization pipeline later crashed in `BranchProbabilityInfo`.

Fix (`compiler/Codegen.cpp::create_nested_function`): save, clear
(`redirect_return = nullptr; current_coro = nullptr`), and restore both around
the nested body, so nested functions/lambdas never inherit the enclosing
coroutine state. Verified on LLVM **and** TCC (`v=99`, exit 0), with no
regressions (main 2186/2187, main `debug_complete` 2187, async 33/33, libs
624/624).

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

### B16 — `block_on` double-freed a destructible variant payload (HIGH) — ✅ FIXED

Root cause found by emitting the 2c translation of an async function returning a
`std::vector<u8>` and reading it with `async::block_on`. In `block_on`:

```chemical
var r = handle.vtbl.poll(handle.frame, &raw mut cx)
if(r is core::async::Poll.Ready) {
    var Ready(value) = r else unreachable
    break value
}
```

`var Ready(value) = r` binds `value` to the payload and `break value` **moves**
it out, but the generated code still destroyed `r` afterwards; `Poll<T>::delete`
then freed the (already-moved) payload and freed its heap buffer again. A
synchronous `var Ok(v) = r` did not hit this because the pattern variable binds
as a *reference* into `r` (only `r` owns the value). `std::string` hid it via SSO.

Two fixes, both needed:

1. **Symres return move-check** (`SymResLinkBody::VisitReturnStmt`): an async
   function's declared type is `FutureHandle<T>`, but the body returns inner `T`.
   Unwrap with `func->inner_return_type()` before linking/`mark_moved_value`, so
   `return local_struct` from an async func compiles instead of erroring with
   *"unknown value being moved..."*.
2. **Move-tracking for destructured payloads:**
   - `SymResLinkBody::VisitBreakStmt` now records a move for a destructible
     loop-result value (identifier or access chain bound by a pattern match),
     and `mark_moved_id` also marks the pattern's source expression
     (`PatternMatchIdentifier::matchExpr->expression`) as moved.
   - 2c: `set_moved_ref_drop_flag` clears the pattern source's drop flag, and
     `writeBreakStmtFor` wraps a moved reference so the flag is cleared.
   - LLVM: `Value::set_drop_flag_for_ref` clears the pattern source's drop flag,
     and `BreakStatement::code_gen` clears the drop flags of a moved break value.

Verified on LLVM **and** TCC: bare `std::vector<u8>` and
`std::Result<std::vector<u8>, int>` returns from async funcs via `block_on` no
longer double-free; no regressions (main 2186/2187, main `debug_complete` 2187,
async 33/33, libs 624/624). This unblocks `process` async wrappers.

**Consequence.** With B10 and B12 fixed, the runtime can construct generic
`FutureTable<T>` entries and generic combinators (`spawn_blocking<T>`,
`timeout<T>`, `JoinHandle<T>`) using either inline non-capturing lambdas or
generic function references (`ident<int>`, `ns::ident<T>`). Library async
wrappers continue to use compiler-lowered `async func` bodies
(done for `fs` and `process`, §7 Tier 4). Watch
B18 when the future payload is a non-`Copy` `T` owned by the frame.

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
