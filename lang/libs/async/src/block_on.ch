// The single, explicit bridge from synchronous to asynchronous code
// (design D12). Blocking `block_on` drives a future handle to completion on the
// current thread, draining the executor's spawned tasks between polls, then
// drops the handle (which cancels/frees the frame).
//
// On `Pending` it polls each outstanding spawned task once and parks for a
// short interval (or until a waker notifies the executor) before re-polling.
// This makes clock-driven futures (`async::sleep`), thread-pool completions
// (`async::spawn_blocking`) and `spawn`ed tasks all make progress without every
// future owning a waker. A readiness reactor (Tier 1) replaces the timed park
// with real readiness wakeups.
public namespace async {

// Drive a SendFutureHandle (the result of safe async submission) to
// completion on the current thread.
public func <T> block_on_send(handle : core::async::SendFutureHandle<T>) : T {
    var owned = handle
    return block_on<T>(unsafe(owned.into_future()))
}

public func <'a, T> block_on(handle : core::async::FutureHandle<'a, T>) : T {
        unsafe {
        var e = executor()
        var cx = core::async::Context {
            waker : exec_make_waker(e)
        }
        var out : T = loop {
            var r = handle.vtbl_ptr().poll(handle.frame_ptr(), &raw mut cx)
            if(r is core::async::Poll.Ready) {
                // Extract through a pointer+memcpy rather than a variant pattern
                // binding: moving a struct `T` out of a pattern mis-lowers on LLVM
                // (the payload is treated as a pointer), see B18.
                break poll_take_ready<T>(&raw mut r)
            }
            executor_poll_tasks(e, 4096u)
            executor_wait(e, 1u)
            continue
        }
        return out

        }
    }

}
