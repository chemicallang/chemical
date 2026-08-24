// additional_test.ch — Additional tests for uncovered APIs in process,
// environment, and window libraries.

using std::string;
using std::string_view;
using std::vector;
using std::Result;
using std::Option;

// ===========================================================================
// Environment: convenience getters
// ===========================================================================

@test
public func test_environment_home_dir(env : &mut TestEnv) {
    var home = environment::home_dir()
    if(home is Option.None) { env.error("home_dir() should return Some"); return }
    var Some(h) = home else unreachable
    if(h.size() == 0) { env.error("home_dir() path should not be empty"); return }
}

@test
public func test_environment_user_name(env : &mut TestEnv) {
    var name = environment::user_name()
    if(name is Option.None) { env.error("user_name() should return Some"); return }
    var Some(n) = name else unreachable
    if(n.size() == 0) { env.error("user_name() should not be empty"); return }
}

@test
public func test_environment_temp_dir(env : &mut TestEnv) {
    var tmp = environment::temp_dir()
    if(tmp is Option.None) { env.error("temp_dir() should return Some"); return }
    var Some(t) = tmp else unreachable
    if(t.size() == 0) { env.error("temp_dir() should not be empty"); return }
}

@test
public func test_environment_shell(env : &mut TestEnv) {
    var sh = environment::shell()
    if(sh is Option.None) { env.error("shell() should return Some"); return }
    var Some(s) = sh else unreachable
    if(s.size() == 0) { env.error("shell() should not be empty"); return }
}

// ===========================================================================
// Environment: set + unset + get round-trip
// ===========================================================================

@test
public func test_environment_set_get_unset_roundtrip(env : &mut TestEnv) {
    var set_res = environment::set("CHEMICAL_ROUNDTRIP_XYZ", "value_abc")
    if(set_res is Result.Err) { env.error("set failed"); return }
    var got = environment::get("CHEMICAL_ROUNDTRIP_XYZ")
    if(got is Option.None) { env.error("get should return Some after set"); return }
    var Some(v) = got else unreachable
    if(!string_eq(&raw v, string_view("value_abc"))) { env.error("get returned wrong value"); return }
    var unset_res = environment::unset("CHEMICAL_ROUNDTRIP_XYZ")
    if(unset_res is Result.Err) { env.error("unset failed"); return }
    var after = environment::get("CHEMICAL_ROUNDTRIP_XYZ")
    if(after is Option.Some) { env.error("get should return None after unset"); return }
}

@test
public func test_environment_get_missing(env : &mut TestEnv) {
    var got = environment::get("CHEMICAL_DEFINITELY_MISSING_VAR_98765")
    if(got is Option.Some) { env.error("get on missing var should return None"); return }
}

@test
public func test_environment_get_or_existing(env : &mut TestEnv) {
    environment::set("CHEMICAL_GET_OR_TEST", "real_val")
    var v = environment::get_or("CHEMICAL_GET_OR_TEST", "default")
    if(!string_eq(&raw v, string_view("real_val"))) { env.error("get_or should return real value"); return }
    environment::unset("CHEMICAL_GET_OR_TEST")
}

// ===========================================================================
// Process: spawn + stdin_data in execute
// ===========================================================================

@test
public func test_process_stdin_data_echo(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("cat"))
    var msg = "hello_stdin"
    var i : size_t = 0
    while(i < 11) {
        cfg.stdin_data.push(msg[i] as u8)
        i += 1
    }
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("should succeed"); return }
    if(!bc(&raw r.output.stdout_data, "hello_stdin" as *char, 11)) {
        env.error("stdout should contain stdin data")
        return
    }
}

@test
public func test_process_empty_stdin_data(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("no_stdin"))
    // stdin_data is empty by default — should work fine
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("should succeed"); return }
}

// ===========================================================================
// Process: working directory change verification
// ===========================================================================

@test
public func test_process_working_directory(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("pwd"))
    cfg.working_dir = string("/tmp")
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("pwd should succeed"); return }
    if(!bc(&raw r.output.stdout_data, "/tmp" as *char, 4)) {
        env.error("pwd should output /tmp")
        return
    }
}

// ===========================================================================
// Process: spawn + kill + wait lifecycle
// ===========================================================================

@test
public func test_process_spawn_kill_wait_lifecycle(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("ping"))
    cfg.args.push(string("-n"))
    cfg.args.push(string("60"))
    cfg.args.push(string("127.0.0.1"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) { env.error("spawn failed"); return }
    var Ok(child) = spawn_res else unreachable
    if(!child.is_running) { env.error("should be running after spawn"); return }
    var kr = process::kill_process(&raw mut child, 9)
    if(kr is Result.Err) { env.error("kill should succeed"); return }
    if(child.is_running) { env.error("should not be running after kill"); return }
    var w = process::wait(&raw mut child)
    if(w is Result.Err) { env.error("wait should succeed"); return }
    var Ok(wr) = w else unreachable
    if(wr.success) { env.error("killed process should not succeed"); return }
}

// ===========================================================================
// Process: capture_stderr = false
// ===========================================================================

@test
public func test_process_no_stderr_capture(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo err >&2"))
    cfg.capture_stderr = false
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(r.output.stderr_data.size() != 0) { env.error("stderr should be empty when capture is off"); return }
}

// ===========================================================================
// Window: maximize/minimize/restore (before creation — stored state only)
// ===========================================================================

@test
public func test_window_maximize_minimize_restore(env : &mut TestEnv) {
    var w = window::Window.make()
    // These should not crash when called before creation
    window::window_maximize(&raw mut w)
    window::window_minimize(&raw mut w)
    window::window_restore(&raw mut w)
}

@test
public func test_window_show_hide_before_create(env : &mut TestEnv) {
    var w = window::Window.make()
    window::window_show(&raw mut w)
    if(!w.visible) { env.error("visible should be true after show"); return }
    window::window_hide(&raw mut w)
    if(w.visible) { env.error("visible should be false after hide"); return }
}

// ===========================================================================
// Window: error variant messages
// ===========================================================================

@test
public func test_window_error_messages(env : &mut TestEnv) {
    var e1 = window::WindowError.PlatformNotSupported()
    var m1 = e1.message()
    if(m1.size() == 0) { env.error("PlatformNotSupported message should not be empty"); return }
    var e2 = window::WindowError.CreateFailed(string("test"))
    var m2 = e2.message()
    if(m2.size() == 0) { env.error("CreateFailed message should not be empty"); return }
    var e3 = window::WindowError.InvalidState(string("test"))
    var m3 = e3.message()
    if(m3.size() == 0) { env.error("InvalidState message should not be empty"); return }
}

// ===========================================================================
// Window: user data roundtrip (before creation)
// ===========================================================================

@test
public func test_window_user_data_before_create(env : &mut TestEnv) {
    var w = window::Window.make()
    var marker : int = 99
    window::window_set_user_data(&raw mut w, &raw mut marker as *mut void)
    // Should not crash — no handle to verify from public API
}

// ===========================================================================
// Window: null pointer safety
// ===========================================================================

// ===========================================================================
// Window: quit_by_destroy
// ===========================================================================

@test
public func test_window_quit_by_destroy_default(env : &mut TestEnv) {
    // Without creating/destroying any window, should return 0
    var qbd = window::window_quit_by_destroy()
    if(qbd != 0) { env.error("quit_by_destroy should be 0 by default"); return }
}

// ===========================================================================
// Window: post_empty_event
// ===========================================================================

@test
public func test_window_post_empty_event(env : &mut TestEnv) {
    // Should not crash
    window::window_post_empty_event()
}

// ===========================================================================
// Process: merge_stderr_stdout in spawn
// ===========================================================================

@test
public func test_process_merge_stderr_stdout_spawn(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo out && echo err >&2"))
    cfg.merge_stdout_stderr = true
    cfg.capture_stderr = false
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) { env.error("spawn failed"); return }
    var Ok(child) = spawn_res else unreachable
    var w = process::wait(&raw mut child)
    if(w is Result.Err) { env.error("wait failed"); return }
    var Ok(wr) = w else unreachable
    if(!wr.success) { env.error("should succeed"); return }
    // Both should be in stdout_data since they're merged
    if(!bc(&raw wr.output.stdout_data, "out" as *char, 3)) {
        env.error("merged output should contain 'out'")
        return
    }
    if(!bc(&raw wr.output.stdout_data, "err" as *char, 3)) {
        env.error("merged output should contain 'err'")
        return
    }
}

// ===========================================================================
// Helpers
// ===========================================================================

func bc(data : *vector<u8>, needle : *char, needle_len : size_t) : bool {
    if(data.size() < needle_len) { return false }
    var d = data.data()
    var i : size_t = 0
    while(i + needle_len <= data.size()) {
        var match = true
        var j : size_t = 0
        while(j < needle_len) {
            if(d[i + j] != needle[j]) { match = false; break }
            j += 1
        }
        if(match) { return true }
        i += 1
    }
    return false
}
