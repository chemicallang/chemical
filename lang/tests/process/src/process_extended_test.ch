// process_extended_test.ch — Extended tests for the process library.

using std::string;
using std::string_view;
using std::vector;
using std::Result;

// ---------------------------------------------------------------------------
// execute: exit code
// ---------------------------------------------------------------------------

@test
public func test_process_execute_exit_code_zero(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("true"))
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("execute(true) returned Err")
        return
    }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("true should exit successfully"); return }
    if(r.exit_code() != 0) { env.error("true should exit with code 0"); return }
}

@test
public func test_process_execute_exit_code_nonzero(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("false"))
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("execute(false) returned Err")
        return
    }
    var Ok(r) = res else unreachable
    if(r.success) { env.error("false should not succeed"); return }
    if(r.exit_code() != 1) { env.error("false should exit with code 1"); return }
}

// ---------------------------------------------------------------------------
// execute: stderr capture
// ---------------------------------------------------------------------------

@test
public func test_process_execute_stderr_capture(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo error_output >&2"))
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unreachable
    if(r.output.stderr_data.size() == 0) { env.error("stderr should not be empty"); return }
}

// ---------------------------------------------------------------------------
// execute: stdout_str / stderr_str convenience
// ---------------------------------------------------------------------------

@test
public func test_process_result_stdout_str(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("convenience_test"))
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unreachable
    var sv = r.stdout_str()
    if(sv.size() == 0) { env.error("stdout_str should not be empty"); return }
    // stdout should contain "convenience_test"
    var found = false
    var i : size_t = 0
    while(i + 16 <= sv.size()) {
        if(sv.data()[i] == 'c' as char &&
           sv.data()[i+1] == 'o' as char &&
           sv.data()[i+2] == 'n' as char &&
           sv.data()[i+3] == 'v' as char) {
            found = true
            break
        }
        i += 1
    }
    if(!found) { env.error("stdout_str should contain 'convenience_test'"); return }
}

// ---------------------------------------------------------------------------
// execute: is_success convenience
// ---------------------------------------------------------------------------

@test
public func test_process_result_is_success(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("true"))
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unreachable
    if(!r.is_success()) { env.error("is_success should be true for true"); return }
}

// ---------------------------------------------------------------------------
// spawn + wait
// ---------------------------------------------------------------------------

@test
public func test_process_spawn_and_wait(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("spawn_test"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unreachable
    if(!child.is_running) { env.error("child should be running after spawn"); return }
    var pid = process::child_pid(&raw mut child)
    if(pid <= 0) { env.error("child_pid should be > 0"); return }
    var wait_res = process::wait(&raw mut child)
    if(wait_res is Result.Err) {
        env.error("wait returned Err")
        return
    }
    var Ok(wr) = wait_res else unreachable
    if(!wr.success) { env.error("spawn+wait should succeed"); return }
    if(child.is_running) { env.error("child should not be running after wait"); return }
}

// ---------------------------------------------------------------------------
// kill
// ---------------------------------------------------------------------------

@test
public func test_process_kill(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sleep"))
    cfg.args.push(string("100"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unreachable
    var kill_res = process::kill(&raw mut child, 9)
    if(kill_res is Result.Err) {
        env.error("kill returned Err")
        return
    }
    if(child.is_running) { env.error("child should not be running after kill"); return }
}

// ---------------------------------------------------------------------------
// is_running (non-blocking)
// ---------------------------------------------------------------------------

@test
public func test_process_is_running_after_spawn(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sleep"))
    cfg.args.push(string("100"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unreachable
    if(!process::is_running(&raw mut child)) { env.error("child should be running immediately after spawn"); return }
    // Kill it so we don't leave zombies
    process::kill(&raw mut child, 9)
}

// ---------------------------------------------------------------------------
// error: empty args
// ---------------------------------------------------------------------------

@test
public func test_process_execute_empty_args(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    // cfg.args is empty by default
    var res = process::execute(cfg)
    if(res is Result.Ok) { env.error("execute with empty args should return Err"); return }
}

@test
public func test_process_spawn_empty_args(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    var res = process::spawn(cfg)
    if(res is Result.Ok) { env.error("spawn with empty args should return Err"); return }
}

// ---------------------------------------------------------------------------
// wait on non-running child
// ---------------------------------------------------------------------------

@test
public func test_process_wait_not_running(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("true"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unreachable
    // First wait should succeed
    var w1 = process::wait(&raw mut child)
    if(w1 is Result.Err) { env.error("first wait should succeed"); return }
    // Second wait should fail (not running)
    var w2 = process::wait(&raw mut child)
    if(w2 is Result.Ok) { env.error("second wait should return Err"); return }
}

// ---------------------------------------------------------------------------
// kill on non-running child
// ---------------------------------------------------------------------------

@test
public func test_process_kill_not_running(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("true"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unreachable
    // Wait to reap
    process::wait(&raw mut child)
    // Kill on non-running should fail
    var kill_res = process::kill(&raw mut child, 9)
    if(kill_res is Result.Ok) { env.error("kill on non-running child should return Err"); return }
}

// ---------------------------------------------------------------------------
// current_pid
// ---------------------------------------------------------------------------

@test
public func test_process_current_pid(env : &mut TestEnv) {
    var pid = process::current_pid()
    if(pid <= 0) { env.error("current_pid should be > 0"); return }
}

// ---------------------------------------------------------------------------
// child_pid
// ---------------------------------------------------------------------------

@test
public func test_process_child_pid(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("true"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unreachable
    var pid = process::child_pid(&raw mut child)
    if(pid <= 0) { env.error("child_pid should be > 0"); return }
    process::wait(&raw mut child)
}

// ---------------------------------------------------------------------------
// sleep_ms (just verify it doesn't crash)
// ---------------------------------------------------------------------------

@test
public func test_process_sleep_ms(env : &mut TestEnv) {
    process::sleep_ms(10)
    // If we get here, sleep_ms didn't crash
}

// ---------------------------------------------------------------------------
// exit_status via try_wait
// ---------------------------------------------------------------------------

@test
public func test_process_try_wait_not_running(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("true"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unreachable
    process::wait(&raw mut child)
    // try_wait on already-reaped child should fail
    var tw = process::try_wait(&raw mut child)
    if(tw is Result.Ok) { env.error("try_wait on non-running child should return Err"); return }
}

// ---------------------------------------------------------------------------
// ProcessConfig default field values
// ---------------------------------------------------------------------------

@test
public func test_process_config_default_fields(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    if(cfg.args.size() != 0) { env.error("default args should be empty"); return }
    if(cfg.env.size() != 0) { env.error("default env should be empty"); return }
    if(cfg.working_dir.size() != 0) { env.error("default working_dir should be empty"); return }
    if(!cfg.capture_stdout) { env.error("default capture_stdout should be true"); return }
    if(!cfg.capture_stderr) { env.error("default capture_stderr should be true"); return }
    if(cfg.merge_stdout_stderr) { env.error("default merge_stdout_stderr should be false"); return }
    if(cfg.stdin_data.size() != 0) { env.error("default stdin_data should be empty"); return }
}

// ---------------------------------------------------------------------------
// Execute with multiple args
// ---------------------------------------------------------------------------

@test
public func test_process_execute_multiple_args(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo arg1 arg2 arg3"))
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("should succeed"); return }
    // stdout should contain "arg1 arg2 arg3"
    var sv = r.stdout_str()
    var found = false
    var i : size_t = 0
    while(i + 12 <= sv.size()) {
        if(sv.data()[i] == 'a' as char &&
           sv.data()[i+1] == 'r' as char &&
           sv.data()[i+2] == 'g' as char &&
           sv.data()[i+3] == '1' as char) {
            found = true
            break
        }
        i += 1
    }
    if(!found) { env.error("stdout should contain 'arg1'"); return }
}

// ---------------------------------------------------------------------------
// Execute: stderr_str convenience
// ---------------------------------------------------------------------------

@test
public func test_process_result_stderr_str(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo err_output >&2"))
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unreachable
    var sv = r.stderr_str()
    if(sv.size() == 0) { env.error("stderr_str should not be empty"); return }
}

// ---------------------------------------------------------------------------
// ProcessError message
// ---------------------------------------------------------------------------

@test
public func test_process_error_messages(env : &mut TestEnv) {
    var e1 = process::ProcessError.InvalidArgs(string("bad args"))
    var m1 = e1.message()
    if(m1.size() == 0) { env.error("InvalidArgs message should not be empty"); return }

    var e2 = process::ProcessError.OperationFailed(string("failed"))
    var m2 = e2.message()
    if(m2.size() == 0) { env.error("OperationFailed message should not be empty"); return }

    var e3 = process::ProcessError.NotRunning()
    var m3 = e3.message()
    if(m3.size() == 0) { env.error("NotRunning message should not be empty"); return }

    var e4 = process::ProcessError.TimedOut()
    var m4 = e4.message()
    if(m4.size() == 0) { env.error("TimedOut message should not be empty"); return }

    var e5 = process::ProcessError.IoError(string("io fail"))
    var m5 = e5.message()
    if(m5.size() == 0) { env.error("IoError message should not be empty"); return }
}

// ---------------------------------------------------------------------------
// close_stdin
// ---------------------------------------------------------------------------

@test
public func test_process_close_stdin(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("cat"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unreachable
    // Close stdin — cat should then exit
    var cs = process::close_stdin(&raw mut child)
    if(cs is Result.Err) { env.error("close_stdin should succeed"); return }
    // Wait for cat to finish
    process::sleep_ms(50)
    var w = process::wait(&raw mut child)
    if(w is Result.Err) { env.error("wait after close_stdin should succeed"); return }
}

// ---------------------------------------------------------------------------
// Execute: capture_stdout = false
// ---------------------------------------------------------------------------

@test
public func test_process_execute_no_stdout_capture(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("should_not_appear"))
    cfg.capture_stdout = false
    var res = process::execute(cfg)
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unreachable
    if(r.output.stdout_data.size() != 0) { env.error("stdout should be empty when capture is off"); return }
}
