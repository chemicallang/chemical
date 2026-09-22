// Runtime edge cases for the async executor and combinators: starvation,
// deterministic timer races, and channel queue stress.
//
// Run under ./scripts/test.sh --libs (TCC and LLVM).
using namespace std;

var edge_done : int = 0;

// A task that yields `steps` times before returning, recording completion.
async func edge_task(steps : int, v : int) : int {
    var i = 0
    while(i < steps) {
        var u = await async::yield_now()
        i = i + 1
    }
    edge_done = edge_done + 1
    return v
}

// A task that is never ready within the test's lifetime.
async func edge_never() : int {
    var u = await async::sleep(100000u)
    return -1
}

// A never-ready task must not starve the ready ones (executor rotation).
async func edge_run() : int {
    var h0 = async::spawn<int>(edge_never())
    var h1 = async::spawn<int>(edge_task(3, 10))
    var h2 = async::spawn<int>(edge_task(3, 20))
    var a = await h1
    var b = await h2
    return a + b
}

@test
func test_executor_no_starvation(env : &mut TestEnv) {
    edge_done = 0;
    var v = async::block_on<int>(edge_run())
    if(v != 30) {
        env.error("ready tasks must complete despite a never-ready task")
    }
    if(edge_done < 2) {
        env.error("both ready tasks must have run to completion")
    }
}

async func edge_sleep_then(ms : u64, v : int) : int {
    var u = await async::sleep(ms)
    return v
}

// `select` must take whichever branch becomes ready first, not poll order.
@test
func test_select_picks_the_faster_timer(env : &mut TestEnv) {
    var start = std::now_milli()
    var v = async::block_on<int>(async::select<int>(edge_sleep_then(400u, 22), edge_sleep_then(1u, 11)))
    var elapsed = std::now_milli() - start
    if(v != 11) {
        env.error("select should take the branch that becomes ready first")
    }
    if(elapsed > 300i64) {
        env.error("select must not wait for the slower branch")
    }
}

@test
func test_timeout_or_sleep_fires(env : &mut TestEnv) {
    var v = async::block_on<int>(async::timeout_or<int>(edge_sleep_then(100000u, 5), 10u, -1))
    if(v != -1) {
        env.error("timeout_or should fall back when the inner timer is longer")
    }
}

@test
func test_timeout_or_sleep_completes(env : &mut TestEnv) {
    var v = async::block_on<int>(async::timeout_or<int>(edge_sleep_then(1u, 5), 10000u, -1))
    if(v != 5) {
        env.error("timeout_or should take the value when the inner timer wins")
    }
}

@test
func test_block_on_timeout_sleep(env : &mut TestEnv) {
    var ok = async::block_on_timeout<int>(edge_sleep_then(1u, 8), 10000u)
    if(ok is std::Option.None) {
        env.error("block_on_timeout should complete a short timer")
    }
    var no = async::block_on_timeout<int>(edge_sleep_then(100000u, 8), 10u)
    if(no is std::Option.Some) {
        env.error("block_on_timeout should give up on a long timer")
    }
}

// Many queued values must be delivered in order, then the closed channel must
// report the fallback exactly once.
async func edge_chan_sum(n : int) : int {
    var ch = async::channel<int>()
    var i = 0
    while(i < n) {
        ch.sender.send(i)
        i = i + 1
    }
    ch.sender.close()
    var total = 0
    var j = 0
    while(j < n) {
        total = total + await ch.receiver.recv_or(-1)
        j = j + 1
    }
    var after = await ch.receiver.recv_or(-999)
    if(after != -999) {
        return -1
    }
    return total
}

@test
func test_channel_many_values(env : &mut TestEnv) {
    var n = 100
    var expected = n * (n - 1) / 2
    var v = async::block_on<int>(edge_chan_sum(n))
    if(v != expected) {
        env.error("channel must deliver every queued value in order")
    }
}

// Zero-duration sleep and a large number of yields must still terminate.
@test
func test_sleep_zero_completes(env : &mut TestEnv) {
    var u = async::block_on<core::async::Unit>(async::sleep(0u))
}

@test
func test_many_yields(env : &mut TestEnv) {
    edge_done = 0;
    var v = async::block_on<int>(edge_task(50, 42))
    if(v != 42) {
        env.error("many yields should still complete")
    }
}

// ---- waker plumbing ---------------------------------------------------------

async func edge_quick(v : int) : int { return v }

// When both branches are already ready, `select` polls the first one first.
@test
func test_select_both_ready_takes_first(env : &mut TestEnv) {
    var v = async::block_on<int>(async::select<int>(edge_quick(1), edge_quick(2)))
    if(v != 1) {
        env.error("select should poll its first branch first")
    }
}

// A `recv_or` parked on an empty channel must be woken when a sender closes it,
// even though the receiver's own handle is still alive.
async func edge_close_wakes_recv() : int {
    var ch = async::channel<int>()
    var s = ch.sender.clone_sender()
    var h = async::spawn_blocking<bool>(|s|() => {
        std::concurrent.sleep_ms(20u)
        s.close()
        return true
    })
    var v = await ch.receiver.recv_or(-5)
    var u = await h
    return v
}

@test
func test_channel_close_wakes_pending_recv(env : &mut TestEnv) {
    var v = async::block_on<int>(edge_close_wakes_recv())
    if(v != -5) {
        env.error("closing the channel from another thread must wake a parked recv_or")
    }
}

// A channel send must still wake the executor while it is blocked in the fd
// reactor (a registered, never-ready fd keeps `executor_wait` on the reactor
// path; the condvar must still deliver the channel wakeup).
async func edge_mixed_channel_and_reactor() : int {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        return -1
    }
    // Registers fds[0] and is never ready (nothing writes), so the executor
    // parks in the reactor.
    var h0 = async::spawn<core::async::Unit>(async::readable(fds[0]))
    var ch = async::channel<int>()
    var s = ch.sender.clone_sender()
    var h = async::spawn_blocking<bool>(|s|() => {
        std::concurrent.sleep_ms(20u)
        s.send(42)
        return true
    })
    var v = await ch.receiver.recv_or(-1)
    var u = await h
    close(fds[0])
    close(fds[1])
    return v
}

@test
func test_channel_wakes_executor_on_reactor_path(env : &mut TestEnv) {
    var v = async::block_on<int>(edge_mixed_channel_and_reactor())
    if(v != 42) {
        env.error("a channel send must wake the executor even while it blocks on the reactor")
    }
}

// ---- nested spawn -----------------------------------------------------------

async func edge_inner(x : int) : int {
    var u = await async::yield_now()
    return x + 1
}

async func edge_outer(x : int) : int {
    var h = async::spawn<int>(edge_inner(x))
    var v = await h
    return v * 2
}

async func edge_nested() : int {
    var h = async::spawn<int>(edge_outer(20))
    return await h
}

@test
func test_spawn_nested(env : &mut TestEnv) {
    var v = async::block_on<int>(edge_nested())
    if(v != 42) {
        env.error("a spawn inside a spawn must complete")
    }
}

// ---- destructible fallback --------------------------------------------------

async func edge_slow_str() : std::string {
    var u = await async::sleep(100000u)
    return std::string("never")
}

@test
func test_timeout_or_destructible_fallback(env : &mut TestEnv) {
    var s = async::block_on<std::string>(async::timeout_or<std::string>(edge_slow_str(), 10u, std::string("fallback")))
    if(s.size() != 8u) {
        env.error("timeout_or should return its destructible fallback on timeout")
    }
}

// ---- already-ready children and variant payloads ----------------------------

@test
func test_timeout_or_ready_child(env : &mut TestEnv) {
    var v = async::block_on<int>(async::timeout_or<int>(edge_quick(9), 10000u, -1))
    if(v != 9) {
        env.error("timeout_or with an already-ready child must return its value")
    }
}

@test
func test_block_on_timeout_ready(env : &mut TestEnv) {
    var d = async::block_on_timeout<int>(edge_quick(9), 10000u)
    if(d is std::Option.None) {
        env.error("block_on_timeout on an already-ready handle should complete")
        return
    }
    var Some(v) = d else unreachable
    if(v != 9) {
        env.error("block_on_timeout already-ready value mismatch")
    }
}

// A `select` between a timer and a reactor-backed future: the fd must win.
async func edge_mixed_select_fd() : int {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        return -1
    }
    var w = fds[1]
    var h = async::spawn_blocking<bool>(|w|() => {
        std::concurrent.sleep_ms(10u)
        var c = 'x'
        write(w, &raw mut c as *void, 1u as ulong)
        return true
    })
    var v = await async::select<core::async::Unit>(async::sleep(100000u), async::readable(fds[0]))
    var u = await h
    var buf : [1]char
    read(fds[0], &raw mut buf[0] as *mut void, 1u as ulong)
    close(fds[0])
    close(fds[1])
    return 1
}

@test
func test_select_mixed_timer_and_fd(env : &mut TestEnv) {
    if(async::block_on<int>(edge_mixed_select_fd()) != 1) {
        env.error("select must take a ready reactor future over a long timer")
    }
}

async func edge_make_opt(x : int) : std::Option<int> {
    var u = await async::yield_now()
    if(x > 0) {
        return std::Option.Some<int>(x)
    }
    return std::Option.None<int>()
}

@test
func test_spawn_variant_payload(env : &mut TestEnv) {
    var d = async::block_on<std::Option<int>>(async::spawn<std::Option<int>>(edge_make_opt(5)))
    if(d is std::Option.None) {
        env.error("spawned Option payload should be Some")
        return
    }
    var Some(v) = d else unreachable
    if(v != 5) {
        env.error("spawned Option payload value mismatch")
    }
    var e = async::block_on<std::Option<int>>(async::spawn<std::Option<int>>(edge_make_opt(0)))
    if(!(e is std::Option.None)) {
        env.error("spawned Option payload should preserve None")
    }
}


