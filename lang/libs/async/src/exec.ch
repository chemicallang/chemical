// The task executor and `spawn` (design Section 12, D7).
//
// `block_on` is the bootstrap executor: it polls its own handle and, between
// polls, drains a process-wide task queue. `spawn(h)` submits a runtime future
// to that queue and returns a join handle that becomes `Ready` with the task's
// result.
//
// This is a cooperative "poll everything" executor, not a waker-scheduled one:
// every `block_on` iteration polls each outstanding task once and then parks
// the thread briefly (or until a waker notifies it). That is enough for the
// futures the runtime authors today (`yield_now`, `sleep`, `spawn_blocking`,
// `timeout`, `select`) and requires no per-task waker plumbing. A readiness
// reactor (Tier 1) will replace the park with real wakeups.
//
// Two shapes here are deliberately non-generic, to avoid a compiler bug (B20)
// where accessing a field of a *concretely instantiated* generic type from
// inside a generic function resolves to the master's field type:
//   * `TaskVTable` / `RawTask` type-erase a task without `FutureTable<Unit>`.
//   * the join result is pulled out with `memcpy`, not `std::replace`.
//
// Ownership: the executor owns the type-erased task frame (`SpawnTask<T>`) and
// frees it after it completes. The join handle and the task share a
// refcounted `JoinState<T>`; the last one out frees it. Dropping the join
// handle cancels the task (best effort): the task observes `cancelled`, reports
// `Ready`, and is reclaimed on the next executor pass.
public namespace async {

// -------------------------------------------------------------------------
// Canonical safe surface (raw executor state stays in `async::raw`)
// -------------------------------------------------------------------------

// Safe driver for native event loops: poll up to `max` queued tasks without
// exposing the executor's queue or synchronization state.
public func executor_drive(max : uint) : uint {
    unsafe {
        return raw::executor_poll_tasks(raw::executor(), max)
    }
}

// Safe submission for a runtime future whose frame the compiler can inspect.
// The call-site check (`SAFE-SEND-001`) requires the operand to be a direct
// async function call and verifies every frame local/parameter is Send and
// 'static. The implementation then crosses the existing unsafe boundary.
public func <T> spawn_send(handle : core::async::FutureHandle<'static, T>) : core::async::SendFutureHandle<T> where T : Send {
    var joined = unsafe(raw::spawn<T>(handle))
    return unsafe(core::async.future_handle_into_send<T>(&mut joined))
}

// Submit a future that already crossed the safe Send boundary (for example the
// result of `spawn_send`). The frame witness is carried by SendFutureHandle, so
// this is safe without re-inspecting an erased FutureHandle.
public func <T> spawn_send_handle(handle : core::async::SendFutureHandle<T>) : core::async::SendFutureHandle<T> where T : Send {
    var owned = handle
    var joined = unsafe(raw::spawn<T>(unsafe(owned.into_future())))
    return unsafe(core::async.future_handle_into_send<T>(&mut joined))
}

}
