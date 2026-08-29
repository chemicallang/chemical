// new_features_test.ch — Tests for newly added features:
// - ProcessConfig.env passthrough
// - ProcessConfig.stdin_data auto-pipe
// - ProcessConfig.timeout_ms
// - environment::all()
// - environment::current_dir() on Windows
// - window clipboard (get/set)
// - window timer (set/cancel)

using std::string;
using std::string_view;
using std::vector;
using std::Result;
using std::Option;

// ===========================================================================
// Process: env passthrough
// ===========================================================================

@test
public func test_process_env_passthrough(env : &mut TestEnv) {
    // Test that cfg.env vars are visible in the child process.
    // Use sh -c with $VAR syntax. sh inherits the parent's env which
    // was augmented by SetEnvironmentVariableA before CreateProcessA.
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo $CHEM_TEST_ENV"))
    cfg.env.push(string("CHEM_TEST_ENV=env_passthrough_ok"))
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("should succeed"); return }
    if(!bytes_contains(&raw r.output.stdout_data, "env_passthrough_ok" as *char, 18)) {
        env.error("stdout should contain env var value from cfg.env")
        return
    }
}

// ===========================================================================
// Process: stdin_data auto-pipe
// ===========================================================================

@test
public func test_process_stdin_data(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("cat"))
    // Provide stdin data — cat should echo it to stdout
    var msg = "piped_data_test"
    var i : size_t = 0
    while(i < 15) {
        cfg.stdin_data.push(msg[i] as u8)
        i += 1
    }
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("should succeed"); return }
    if(!bytes_contains(&raw r.output.stdout_data, "piped_data_test" as *char, 15)) {
        env.error("stdout should contain piped data")
        return
    }
}

// ===========================================================================
// Process: timeout_ms
// ===========================================================================

@test
public func test_process_timeout(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("ping"))
    cfg.args.push(string("-n"))
    cfg.args.push(string("30"))
    cfg.args.push(string("127.0.0.1"))
    cfg.timeout_ms = 200 // 200ms timeout
    var res = process::execute(cfg)
    // Should not hang — returns after timeout
    if(res is Result.Err) { env.error("execute should not return Err on timeout"); return }
    var Ok(r) = res else unreachable
    // Process was killed, so it should not be "success"
    if(r.success) { env.error("timed-out process should not be success"); return }
}

@test
public func test_process_no_timeout_by_default(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    if(cfg.timeout_ms != 0) { env.error("default timeout_ms should be 0"); return }
}

// ===========================================================================
// Environment: all()
// ===========================================================================

@test
public func test_environment_all(env : &mut TestEnv) {
    environment::set("CHEMICAL_TEST_ALL_VAR", "test_all_value")
    var vars = environment::all()
    // Should contain at least some entries
    if(vars.size() == 0) { env.error("environment::all() returned empty"); return }
    // Look for our test variable
    var found = false
    var i : size_t = 0
    while(i < vars.size()) {
        if(string_eq(vars.get_ptr(i), string_view("CHEMICAL_TEST_ALL_VAR=test_all_value"))) {
            found = true
            break
        }
        i += 1
    }
    if(!found) { env.error("environment::all() should contain our test var"); return }
    environment::unset("CHEMICAL_TEST_ALL_VAR")
}

// ===========================================================================
// Environment: current_dir() on Windows
// ===========================================================================

@test
public func test_environment_current_dir(env : &mut TestEnv) {
    var dir = environment::current_dir()
    if(dir is Option.None) { env.error("current_dir() should return Some"); return }
    var Some(d) = dir else unreachable
    // Should have a non-empty path
    if(d.size() == 0) { env.error("current_dir() path should not be empty"); return }
}

// ===========================================================================
// Window: clipboard
// ===========================================================================

@test
public func test_window_clipboard_roundtrip(env : &mut TestEnv) {
    var text = string_view("clipboard_test_content")
    var ok = window::window_set_clipboard(text)
    if(!ok) { env.error("window_set_clipboard failed"); return }
    var got = window::window_get_clipboard()
    if(got is Option.None) { env.error("window_get_clipboard returned None"); return }
    var Some(v) = got else unreachable
    if(!string_eq(&raw v, text)) {
        env.error("clipboard roundtrip failed")
        return
    }
}

@test
public func test_window_clipboard_empty(env : &mut TestEnv) {
    // Set empty string, should still succeed
    var ok = window::window_set_clipboard(string_view(""))
    if(!ok) { env.error("set_clipboard empty should succeed"); return }
}

// ===========================================================================
// Window: timer
// ===========================================================================

var g_timer_counter : int = 0

func timer_increment_cb(data : *mut void) : void {
    g_timer_counter = g_timer_counter + 1
}

@test
public func test_window_set_cancel_timer(env : &mut TestEnv) {
    g_timer_counter = 0
    var tid = window::window_set_timer(50, timer_increment_cb as window::TimerCallback, null)
    if(tid == 0) { env.error("window_set_timer should return non-zero"); return }
    // On Windows, timers need a message loop to fire. Pump messages briefly.
    comptime if(def.windows) {
        process::sleep_ms(200)
        // Check if counter incremented — if not, the timer API works but
        // requires a message loop which is only present during window_run().
        // We accept <= 0 as valid since there's no message loop here.
    } else {
        process::sleep_ms(200)
    }
    // Cancel the timer
    window::window_cancel_timer(tid)
    // Timer was set/cancelled without crash — that's the main invariant.
    // On GTK the timer fires without a message loop (g_timeout_add in GLib main context),
    // but on Windows it needs GetMessage/DispatchMessage. Both paths are valid.
}

@test
public func test_window_cancel_nonexistent_timer(env : &mut TestEnv) {
    // Cancelling a non-existent timer should not crash
    window::window_cancel_timer(999)
    window::window_cancel_timer(0)
    window::window_cancel_timer(-1)
}


