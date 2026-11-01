// Async closures (`async |captures|(params) => ...`).
//
// A closure future owns its capture environment. The call consumes the
// callable (`FnOnce`), the ramp moves the environment into the coroutine
// frame, and normal completion/cancellation releases it exactly once.

async func async_closure_add_one(v : int) : int {
    return v + 1
}

@test
func test_async_closure_capture_await(env : &mut TestEnv) {
    var x = 40
    var s = std::string("hello")
    var f = async |x, s|(y : int) => {
        var z = await async_closure_add_one(x)
        var n = s.size() as int
        // 1 + 41 + 5 - 5
        return y + z + n - 5
    }
    if(async::block_on<int>(f(1)) != 42) {
        env.error("capturing async closure should resume with its captured state")
    }
}

@test
func test_async_closure_noncapturing(env : &mut TestEnv) {
    var f = async ||(y : int) => {
        var z = await async_closure_add_one(y)
        return z
    }
    if(async::block_on<int>(f(41)) != 42) {
        env.error("non-capturing async closure should await and return 42")
    }
}

@test
func test_async_closure_explicit_fn_once(env : &mut TestEnv) {
    var x = 40
    var f : std::FnOnce<(y : int) => core::async::FutureHandle<'static, int>> = async |x|(y : int) => {
        var z = await async_closure_add_one(x)
        return y + z
    }
    if(async::block_on<int>(f(1)) != 42) {
        env.error("async closure assigned to an explicit FnOnce should return 42")
    }
}


func async_closure_run_param(
    f : std::FnOnce<(y : int) => core::async::FutureHandle<'static, int>>
) : int {
    return async::block_on<int>(f(1))
}

@test
func test_async_closure_as_fn_once_parameter(env : &mut TestEnv) {
    var x = 40
    var f = async |x|(y : int) => {
        var z = await async_closure_add_one(x)
        return y + z
    }
    if(async_closure_run_param(f) != 42) {
        env.error("an async closure should cross an explicit FnOnce parameter")
    }
}


func <F> async_closure_get_future(
    f : F, x : int
) : core::async::FutureHandle<'static, int> where F : core::ops::FnOnce<(y : int) => core::async::FutureHandle<'static, int>> {
    return f(x)
}

@test
func test_async_closure_generic_fn_once_bound(env : &mut TestEnv) {
    var x = 40
    var f = async |x|(y : int) => {
        var z = await async_closure_add_one(x)
        return y + z
    }
    var future = async_closure_get_future(f, 1)
    if(async::block_on<int>(future) != 42) {
        env.error("an async closure should satisfy a generic FnOnce bound")
    }
}


@test
func test_async_closure_noncapturing_explicit_fn_once(env : &mut TestEnv) {
    var f : std::FnOnce<(y : int) => core::async::FutureHandle<'static, int>> = async ||(y : int) => {
        return y + 1
    }
    if(async::block_on<int>(f(41)) != 42) {
        env.error("a non-capturing async closure should cross an explicit FnOnce target")
    }
}

@direct_init
struct AsyncClosureHolder {
    var f : std::FnOnce<(y : int) => core::async::FutureHandle<'static, int>>
}

@test
func test_async_closure_stored_in_field(env : &mut TestEnv) {
    var x = 40
    var h = AsyncClosureHolder { f : async |x|(y : int) => {
        var z = await async_closure_add_one(x)
        return y + z
    } }
    var future = h.f(1)
    if(async::block_on<int>(future) != 42) {
        env.error("a field-stored async closure should move its capture into the future")
    }
}

var async_closure_drops : int = 0;

@direct_init
struct AsyncClosureDropCounter {
    var value : int
    @delete
    func delete(&mut self) {
        unsafe { async_closure_drops = async_closure_drops + 1 }
    }
}

@test
func test_async_closure_dropped_before_poll(env : &mut TestEnv) {
    unsafe {
        var before = async_closure_drops
        {
            var d = AsyncClosureDropCounter { value : 7 }
            var f = async |d|() => {
                return d.value
            }
            var future = f()
            // never polled: the future owns the capture and must release it
        }
        if(async_closure_drops != before + 1) {
            env.error("an unpolled async closure future must drop its captures exactly once")
        }
    }
}

struct AsyncClosureCountdown {
    var remaining : int
    var value : int
}

func async_closure_countdown_poll(
    frame : *mut void, cx : *mut core::async::Context
) : core::async::Poll<int> {
    unsafe {
        var f = frame as *mut AsyncClosureCountdown
        if(f.remaining > 0) {
            f.remaining = f.remaining - 1
            return core::async::Poll.Pending<int>()
        }
        return core::async::Poll.Ready<int>(f.value)
    }
}

func async_closure_countdown_drop(frame : *mut void) { }

func async_closure_make_countdown(
    remaining : int, value : int
) : core::async::FutureHandle<int> {
    unsafe {
        var f = unsafe(malloc(sizeof(AsyncClosureCountdown))) as *mut AsyncClosureCountdown
        f.remaining = remaining
        f.value = value
        var t = unsafe(malloc(sizeof(core::async::FutureTable<int>))) as *mut core::async::FutureTable<int>
        t.poll = async_closure_countdown_poll
        t.drop = async_closure_countdown_drop
        return unsafe(core::async::future_handle_raw<int>(f as *mut void, t))
    }
}

@test
func test_async_closure_cancel_destroys_capture(env : &mut TestEnv) {
    unsafe {
        var before = async_closure_drops
        {
            var d = AsyncClosureDropCounter { value : 7 }
            var f = async |d|() => {
                var v = await async_closure_make_countdown(3, 1)
                return v + d.value
            }
            var h = f()
            var cx = core::async::Context { waker : core::async::Waker.empty() }
            var p = h.vtbl_ptr().poll(h.frame_ptr(), &raw mut cx)
            if(!(p is core::async::Poll.Pending)) {
                env.error("expected the async closure to suspend before completion")
            }
            // h is dropped here, cancelling the suspended closure future
        }
        if(async_closure_drops != before + 1) {
            env.error("cancelling an async closure must destroy its captured value exactly once")
        }
    }
}

@test
func test_async_closure_normal_completion_drops_capture(env : &mut TestEnv) {
    unsafe {
        var before = async_closure_drops
        {
            var d = AsyncClosureDropCounter { value : 40 }
            var f = async |d|() => {
                var z = await async_closure_add_one(1)
                return z + d.value
            }
            if(async::block_on<int>(f()) != 42) {
                env.error("the async closure should return 42")
            }
        }
        if(async_closure_drops != before + 1) {
            env.error("a completed async closure must destroy its capture exactly once")
        }
    }
}
