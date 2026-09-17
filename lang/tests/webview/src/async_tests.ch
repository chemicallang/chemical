using std::string;

// ===========================================================================
// Tier 5 — UI-thread async bridge
//
// `async::spawn_local` submits a future to the process-wide executor. On its
// own that executor only runs inside `block_on`; a GUI app is instead parked in
// the native event loop. `window_pump` and `window_run_async` drive the
// executor from that loop, so a UI-spawned task is polled on the UI thread and
// the continuation after an `await` returns to the UI thread.
//
// These tests run with a display (the `--webview` suite), like the rest of the
// bridge tests.
// ===========================================================================

var g_ui_await_ran : int = 0
var g_ui_await_value : int = 0

// Suspends on background (thread-pool) work, then continues on whatever thread
// polls it — the UI thread, when driven by `window_pump`.
async func async_ui_await_blocking() : int {
    g_ui_await_ran = 1
    var v = await async::spawn_blocking<int>(() => 20)
    g_ui_await_value = v + 1
    return g_ui_await_value
}

// Drive the UI pump until `done` is true or the iteration budget runs out.
func ui_pump_until(done : () => bool, max_iters : int) : bool {
    var n = 0
    while(n < max_iters) {
        window::window_pump()
        if(done()) { return true }
        std::concurrent.sleep_ms(1u)
        n = n + 1
    }
    return done()
}

// ---------------------------------------------------------------------------
// spawn_local + window_pump
// ---------------------------------------------------------------------------

@test
public func test_spawn_local_await_blocking(env : &mut TestEnv) {
    g_ui_await_ran = 0
    g_ui_await_value = 0
    var h = async::spawn_local<int>(async_ui_await_blocking())
    var ok = ui_pump_until(() => g_ui_await_value == 21, 2000)
    if(!ok) {
        env.error("spawn_local continuation after await did not run")
        return
    }
}

@test
public func test_spawn_local_value_is_joinable(env : &mut TestEnv) {
    // Drive the coroutine to completion through the pump, then read its join
    // result (already complete, so block_on returns without parking the UI
    // thread).
    g_ui_await_value = 0
    var h = async::spawn_local<int>(async_ui_await_blocking())
    var done = ui_pump_until(() => g_ui_await_value == 21, 2000)
    if(!done) {
        env.error("spawn_local future did not complete")
        return
    }
    var v = async::block_on<int>(h)
    if(v != 21) {
        env.error("spawn_local join result mismatch")
        return
    }
}

@test
public func test_spawn_local_cancel_no_crash(env : &mut TestEnv) {
    // Spawn several tasks and drop their join handles immediately. The executor
    // must observe cancellation and reclaim them without crashing.
    var i = 0
    while(i < 8) {
        var h = async::spawn_local<int>(async_ui_await_blocking())
        i = i + 1
    }
    window::window_pump()
}

// ---------------------------------------------------------------------------
// window_run_async: the event loop itself drives the executor
// ---------------------------------------------------------------------------

var g_run_async_quit : int = 0

func run_async_quit_cb(data : *mut void) {
    g_run_async_quit = 1
    window::window_quit()
}

@test
public func test_window_run_async_drives_executor(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("window async test")
    w.width = 200
    w.height = 120
    var cr = window::window_create(&raw mut w)
    if(cr is std::Result.Err) {
        env.error("window_run_async test: could not create window (display?)")
        return
    }
    window::window_show(&raw mut w)

    g_ui_await_ran = 0
    g_ui_await_value = 0
    g_run_async_quit = 0
    var h = async::spawn_local<int>(async_ui_await_blocking())
    // Quit the loop shortly after it starts; the executor tick runs in between.
    window::window_set_timer(250, run_async_quit_cb, null)
    window::window_run_async()

    if(g_ui_await_value != 21) {
        env.error("window_run_async did not drive the executor to completion")
        window::window_destroy(&raw mut w)
        return
    }
    window::window_destroy(&raw mut w)
}
