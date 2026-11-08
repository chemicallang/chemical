// process_extended_test.ch — Extended tests for the process library.

using std::string;
using std::string_view;
using std::vector;
using std::Result;
using std::Option;

// ---------------------------------------------------------------------------
// execute: exit code
// ---------------------------------------------------------------------------

@test
public func test_process_execute_exit_code_zero(env : &mut TestEnv) {
    var cfg = process::Command.new("true")
    var res = cfg.output()
    if(res is Result.Err) {
        env.error("execute(true) returned Err")
        return
    }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) { env.error("true should exit successfully"); return }
    if(r.exit_code() != 0) { env.error("true should exit with code 0"); return }
}

@test
public func test_process_execute_exit_code_nonzero(env : &mut TestEnv) {
    var cfg = process::Command.new("false")
    var res = cfg.output()
    if(res is Result.Err) {
        env.error("execute(false) returned Err")
        return
    }
    var Ok(r) = res else unsafe { unreachable }
    if(r.success()) { env.error("false should not succeed"); return }
    if(r.exit_code() != 1) { env.error("false should exit with code 1"); return }
}

// ---------------------------------------------------------------------------
// execute: stderr capture
// ---------------------------------------------------------------------------

@test
public func test_process_execute_stderr_capture(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("echo error_output >&2")
    var res = cfg.output()
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unsafe { unreachable }
    if(r.stderr_bytes().size() == 0) { env.error("stderr should not be empty"); return }
}

// ---------------------------------------------------------------------------
// execute: stdout_str / stderr_str convenience
// ---------------------------------------------------------------------------

@test
public func test_process_result_stdout_str(env : &mut TestEnv) {
        unsafe {
        var cfg = process::Command.new("echo")
        cfg.arg("convenience_test")
        var res = cfg.output()
        if(res is Result.Err) {
            env.error("execute returned Err")
            return
        }
        var Ok(r) = res else unsafe { unreachable }
        var svopt = r.stdout_str()
        if(svopt is Option.None) { env.error("stdout_str invalid UTF-8"); return }
        var Some(sv) = svopt else unsafe { unreachable }
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
    }

// ---------------------------------------------------------------------------
// execute: is_success convenience
// ---------------------------------------------------------------------------

@test
public func test_process_result_is_success(env : &mut TestEnv) {
    var cfg = process::Command.new("true")
    var res = cfg.output()
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.is_success()) { env.error("is_success should be true for true"); return }
}

// ---------------------------------------------------------------------------
// spawn + wait
// ---------------------------------------------------------------------------

@test
public func test_process_spawn_and_wait(env : &mut TestEnv) {
    var cfg = process::Command.new("echo")
    cfg.arg("spawn_test")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unsafe { unreachable }
    if(!process::is_running(&mut child)) { env.error("child should be running after spawn"); return }
    var pid = child.id() as int
    if(pid <= 0) { env.error("child_pid should be > 0"); return }
    var wait_res = child.wait_with_output()
    if(wait_res is Result.Err) {
        env.error("wait returned Err")
        return
    }
    var Ok(wr) = wait_res else unsafe { unreachable }
    if(!wr.success()) { env.error("spawn+wait should succeed"); return }
    if(process::is_running(&mut child)) { env.error("child should not be running after wait"); return }
}

// ---------------------------------------------------------------------------
// kill
// ---------------------------------------------------------------------------

@test
public func test_process_kill(env : &mut TestEnv) {
    var cfg = process::Command.new("sleep")
    cfg.arg("100")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unsafe { unreachable }
    var kill_res = child.kill()
    if(kill_res is Result.Err) {
        env.error("kill returned Err")
        return
    }
    if(process::is_running(&mut child)) { env.error("child should not be running after kill"); return }
}

// ---------------------------------------------------------------------------
// is_running (non-blocking)
// ---------------------------------------------------------------------------

@test
public func test_process_is_running_after_spawn(env : &mut TestEnv) {
    var cfg = process::Command.new("sleep")
    cfg.arg("100")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unsafe { unreachable }
    if(!process::is_running(&mut child)) { env.error("child should be running immediately after spawn"); return }
    // Kill it so we don't leave zombies
    child.kill()
}

// ---------------------------------------------------------------------------
// error: empty args
// ---------------------------------------------------------------------------

@test
public func test_process_execute_empty_args(env : &mut TestEnv) {
    var cfg = process::Command.new("")
    // cfg.args is empty by default
    var res = cfg.output()
    if(res is Result.Ok) { env.error("execute with empty args should return Err"); return }
}

@test
public func test_process_spawn_empty_args(env : &mut TestEnv) {
    var cfg = process::Command.new("")
    var res = cfg.spawn()
    if(res is Result.Ok) { env.error("spawn with empty args should return Err"); return }
}

// ---------------------------------------------------------------------------
// wait on non-running child
// ---------------------------------------------------------------------------

@test
public func test_process_wait_is_idempotent(env : &mut TestEnv) {
    var cfg = process::Command.new("true")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unsafe { unreachable }
    var w1 = child.wait()
    if(w1 is Result.Err) { env.error("first wait should succeed"); return }
    var w2 = child.wait()
    if(w2 is Result.Err) { env.error("second wait should return the cached status"); return }
    var tw = child.try_wait()
    if(tw is Result.Err) { env.error("try_wait on reaped child should succeed"); return }
    var Ok(tws) = tw else unsafe { unreachable }
    if(tws is std::Option.None) { env.error("try_wait should report the cached exit status"); return }
}

// ---------------------------------------------------------------------------
// kill on non-running child
// ---------------------------------------------------------------------------

@test
public func test_process_kill_not_running(env : &mut TestEnv) {
    var cfg = process::Command.new("true")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unsafe { unreachable }
    // Wait to reap
    child.wait()
    // Kill on non-running should fail
    var kill_res = child.kill()
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
    var cfg = process::Command.new("true")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unsafe { unreachable }
    var pid = child.id() as int
    if(pid <= 0) { env.error("child_pid should be > 0"); return }
    child.wait()
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
    var cfg = process::Command.new("true")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unsafe { unreachable }
    child.wait()
    // try_wait on a reaped child returns the cached status
    var tw = child.try_wait()
    if(tw is Result.Err) { env.error("try_wait on reaped child should succeed"); return }
    var Ok(tws) = tw else unsafe { unreachable }
    if(tws is std::Option.None) { env.error("try_wait should return the cached status"); return }
}

// ---------------------------------------------------------------------------
// ProcessConfig default field values
// ---------------------------------------------------------------------------

@test
public func test_process_command_builder(env : &mut TestEnv) {
    var cfg = process::Command.new("echo")
    cfg.arg("builder")
    var res = cfg.output()
    if(res is Result.Err) { env.error("Command builder should run"); return }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) { env.error("Command builder should succeed"); return }
}

// ---------------------------------------------------------------------------
// Execute with multiple args
// ---------------------------------------------------------------------------

@test
public func test_process_execute_multiple_args(env : &mut TestEnv) {
        unsafe {
        var cfg = process::Command.new("sh")
        cfg.arg("-c")
        cfg.arg("echo arg1 arg2 arg3")
        var res = cfg.output()
        if(res is Result.Err) {
            env.error("execute returned Err")
            return
        }
        var Ok(r) = res else unsafe { unreachable }
        if(!r.success()) { env.error("should succeed"); return }
        // stdout should contain "arg1 arg2 arg3"
        var svopt = r.stdout_str()
        if(svopt is Option.None) { env.error("stdout_str invalid UTF-8"); return }
        var Some(sv) = svopt else unsafe { unreachable }
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
    }

// ---------------------------------------------------------------------------
// Execute: stderr_str convenience
// ---------------------------------------------------------------------------

@test
public func test_process_result_stderr_str(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("echo err_output >&2")
    var res = cfg.output()
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unsafe { unreachable }
    var svopt = r.stderr_str()
    if(svopt is Option.None) { env.error("stderr_str invalid UTF-8"); return }
    var Some(sv) = svopt else unsafe { unreachable }
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
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("cat")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) {
        env.error("spawn returned Err")
        return
    }
    var Ok(child) = spawn_res else unsafe { unreachable }
    // Close stdin — cat should then exit
    var cs = child.close_stdin()
    if(cs is Result.Err) { env.error("close_stdin should succeed"); return }
    // Wait for cat to finish
    process::sleep_ms(50)
    var w = child.wait_with_output()
    if(w is Result.Err) { env.error("wait after close_stdin should succeed"); return }
}

// ---------------------------------------------------------------------------
// Execute: capture_stdout = false
// ---------------------------------------------------------------------------

@test
public func test_process_execute_no_stdout_capture(env : &mut TestEnv) {
    var cfg = process::Command.new("echo")
    cfg.arg("should_not_appear")
    cfg.stdout(process::Stdio.Null())
    var res = cfg.output()
    if(res is Result.Err) {
        env.error("execute returned Err")
        return
    }
    var Ok(r) = res else unsafe { unreachable }
    if(r.stdout_bytes().size() != 0) { env.error("stdout should be empty when capture is off"); return }
}
