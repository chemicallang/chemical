// Regression tests for B25: a coroutine that awaits a combinator
// (`timeout_or` / `select`) while it is itself polled as a *spawned task* must
// keep a valid `Context` across suspensions.
//
// The LLVM lowering cached the `Context*` in the coroutine body before the
// await loop and reused it after every resume. The executor passes a fresh
// stack `Context` on each poll, so the resumed coroutine forwarded a stale
// pointer to its child poll; the combinator then read a garbage waker `vtbl`
// and jumped through it. Awaiting the same combinator directly from `block_on`
// was unaffected (one poll, no stale pointer).
//
// These run on both backends (the bug was LLVM-only, but TCC must stay green).

async func b25_timeout_waiter(rd : int) : int {
    // `readable(rd)` on an idle pipe is pending, so the coroutine really
    // suspends and then resumes when the timeout fires. The combinator clones
    // the waker, which is where the stale `Context` used to blow up.
    var u = await async::timeout_or<core::async::Unit>(async::readable(rd), 20u, core::async::Unit { })
    return 7
}

async func b25_select_waiter(rd : int) : int {
    // `sleep` wins the race; `select` copies the waker while suspended.
    var a = async::readable(rd)
    var b = async::sleep(10u)
    var u = await async::select<core::async::Unit>(a, b)
    return 9
}

@test
public func test_async_spawn_await_timeout_or(env : &mut TestEnv) {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        env.error("async::pipe failed")
        return
    }
    var h = async::spawn<int>(b25_timeout_waiter(fds[0]))
    var v = async::block_on<int>(h)
    if(v != 7) {
        env.error("spawned coroutine awaiting timeout_or did not complete")
    }
}

@test
public func test_async_spawn_await_select(env : &mut TestEnv) {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        env.error("async::pipe failed")
        return
    }
    var h = async::spawn<int>(b25_select_waiter(fds[0]))
    var v = async::block_on<int>(h)
    if(v != 9) {
        env.error("spawned coroutine awaiting select did not complete")
    }
}
