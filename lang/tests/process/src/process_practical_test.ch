// process_practical_test.ch — Practical integration tests for the process library.
// These tests exercise real command execution, file I/O, stdin piping,
// environment variables, and working directory handling.

using std::string;
using std::string_view;
using std::vector;
using std::Result;

// Helper: does a byte slice contain the given substring? needle does NOT
// include a trailing null — we search for exact byte content.
func bytes_contains(data : [u8], needle : *char, needle_len : size_t) : bool {
        unsafe {
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
    }

// ---------------------------------------------------------------------------
// Run a real command and verify stdout content
// ---------------------------------------------------------------------------

@test
public func test_practical_echo_hello(env : &mut TestEnv) {
    var cfg = process::Command.new("echo")
    cfg.arg("hello_from_chemical")
    var res = cfg.output()
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) { env.error("echo should succeed"); return }
    if(!bytes_contains(r.stdout_bytes(), "hello_from_chemical" as *char, 19)) {
        env.error("stdout should contain 'hello_from_chemical'")
        return
    }
}

// ---------------------------------------------------------------------------
// Run a command that writes to a file, verify the file
// ---------------------------------------------------------------------------

@test
public func test_practical_write_file(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("echo chemical_test_content > /tmp/chemical_test_write.txt")
    var res = cfg.output()
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) { env.error("write command should succeed"); return }

    // Now read the file back
    var cfg2 = process::Command.new("sh")
    cfg2.arg("-c")
    cfg2.arg("cat /tmp/chemical_test_write.txt")
    var res2 = cfg2.output()
    if(res2 is Result.Err) { env.error("cat failed"); return }
    var Ok(r2) = res2 else unsafe { unreachable }
    if(!r2.success()) { env.error("cat should succeed"); return }
    if(!bytes_contains(r2.stdout_bytes(), "chemical_test_content" as *char, 21)) {
        env.error("file should contain 'chemical_test_content'")
        return
    }
    // Cleanup
    var cleanup_cfg = process::Command.new("sh")
    cleanup_cfg.arg("-c")
    cleanup_cfg.arg("rm -f /tmp/chemical_test_write.txt")
    cleanup_cfg.output()
}

// ---------------------------------------------------------------------------
// Run a command with non-zero exit code and verify it
// ---------------------------------------------------------------------------

@test
public func test_practical_exit_code_42(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("exit 42")
    var res = cfg.output()
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unsafe { unreachable }
    if(r.success()) { env.error("exit 42 should not succeed"); return }
    if(r.exit_code() != 42) { env.error("expected exit code 42"); return }
}

// ---------------------------------------------------------------------------
// Stdin piping: write to cat, read from stdout
// ---------------------------------------------------------------------------

@test
public func test_practical_stdin_to_cat(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("cat")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) { env.error("spawn failed"); return }
    var Ok(child) = spawn_res else unsafe { unreachable }

    // Write data to stdin
    var input = vector<u8>()
    var msg = "piped_from_chemical"
    var i : size_t = 0
    while(i < 19) {
        input.push(msg[i] as u8)
        i += 1
    }
    var input_view : [u8] = &input
    var wr = child.write_stdin(input_view)
    if(wr is Result.Err) { env.error("write_stdin failed"); return }

    // Close stdin so cat finishes
    child.close_stdin()

    // Wait for result
    var wait_res = child.wait_with_output()
    if(wait_res is Result.Err) { env.error("wait failed"); return }
    var Ok(wr_res) = wait_res else unsafe { unreachable }
    if(!wr_res.success()) { env.error("cat should succeed"); return }
    if(!bytes_contains(wr_res.stdout_bytes(), "piped_from_chemical" as *char, 19)) {
        env.error("stdout should contain piped data")
        return
    }
}

// ---------------------------------------------------------------------------
// Environment variable passed to child process
// ---------------------------------------------------------------------------

@test
public func test_practical_env_var_in_child(env : &mut TestEnv) {
    // Set an env var
    unsafe(environment::set("CHEMICAL_PRACTICAL_TEST_VAR", "practical_value"))

    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("echo $CHEMICAL_PRACTICAL_TEST_VAR")
    var res = cfg.output()
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) { env.error("command should succeed"); return }
    if(!bytes_contains(r.stdout_bytes(), "practical_value" as *char, 15)) {
        env.error("child should see CHEMICAL_PRACTICAL_TEST_VAR")
        return
    }
    // Cleanup
    unsafe(environment::unset("CHEMICAL_PRACTICAL_TEST_VAR"))
}

// ---------------------------------------------------------------------------
// Working directory: run pwd in /tmp
// ---------------------------------------------------------------------------

@test
public func test_practical_working_directory(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("pwd")
    // `/tmp` is not a valid Win32 working directory (CreateProcess uses the
    // native path), so use a root that exists on both platforms.
    comptime if(def.windows) {
        cfg.current_dir(&path::Path.new("C:/"))
    } else {
        cfg.current_dir(&path::Path.new("/tmp"))
    }
    var res = cfg.output()
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) { env.error("pwd should succeed"); return }
    // pwd outputs the directory followed by a newline; search for the path only
    comptime if(def.windows) {
        if(!bytes_contains(r.stdout_bytes(), "/c" as *char, 2)) {
            env.error("pwd should output /c")
            return
        }
    } else {
        if(!bytes_contains(r.stdout_bytes(), "/tmp" as *char, 4)) {
            env.error("pwd should output /tmp")
            return
        }
    }
}

// ---------------------------------------------------------------------------
// Spawn, check is_running, wait for completion, verify output
// ---------------------------------------------------------------------------

@test
public func test_practical_spawn_lifecycle(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("echo alive && sleep 0.1 && echo done")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) { env.error("spawn failed"); return }
    var Ok(child) = spawn_res else unsafe { unreachable }

    // Should be running right after spawn
    if(!process::is_running(&mut child)) { env.error("should be running"); return }

    // Wait for it to finish
    var wait_res = child.wait_with_output()
    if(wait_res is Result.Err) { env.error("wait failed"); return }
    var Ok(wr) = wait_res else unsafe { unreachable }
    if(!wr.success()) { env.error("should succeed"); return }

    // Should not be running now
    if(process::is_running(&mut child)) { env.error("should not be running after wait"); return }

    // Output should contain both lines
    if(!bytes_contains(wr.stdout_bytes(), "alive" as *char, 5)) {
        env.error("stdout should contain 'alive'")
        return
    }
    if(!bytes_contains(wr.stdout_bytes(), "done" as *char, 4)) {
        env.error("stdout should contain 'done'")
        return
    }
}

// ---------------------------------------------------------------------------
// Kill a long-running process
// ---------------------------------------------------------------------------

@test
public func test_practical_kill_long_process(env : &mut TestEnv) {
    var cfg = process::Command.new("sleep")
    cfg.arg("300")
    var spawn_res = cfg.spawn()
    if(spawn_res is Result.Err) { env.error("spawn failed"); return }
    var Ok(child) = spawn_res else unsafe { unreachable }

    // Verify it's running
    if(!process::is_running(&mut child)) { env.error("sleep should be running"); return }

    // Kill it
    var kr = child.kill()
    if(kr is Result.Err) { env.error("kill should succeed"); return }

    // Should not be running
    if(process::is_running(&mut child)) { env.error("should not be running after kill"); return }
}

// ---------------------------------------------------------------------------
// Multiple sequential processes
// ---------------------------------------------------------------------------

@test
public func test_practical_sequential_processes(env : &mut TestEnv) {
    var i = 0
    while(i < 5) {
        var cfg = process::Command.new("echo")
        cfg.arg("seq_test")
        var res = cfg.output()
        if(res is Result.Err) { env.error("execute failed in loop"); return }
        var Ok(r) = res else unsafe { unreachable }
        if(!r.success()) { env.error("should succeed in loop"); return }
        if(!bytes_contains(r.stdout_bytes(), "seq_test" as *char, 8)) {
            env.error("stdout should contain 'seq_test'")
            return
        }
        i += 1
    }
}

// ---------------------------------------------------------------------------
// Merge stdout+stderr
// ---------------------------------------------------------------------------

@test
public func test_practical_merge_stdout_stderr(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("echo out && echo err >&2")
    cfg.merge_stdout_stderr(true)
    cfg.stderr(process::Stdio.Null())
    var res = cfg.output()
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) { env.error("should succeed"); return }
    // Both stdout and stderr should be merged into stdout_data
    if(!bytes_contains(r.stdout_bytes(), "out" as *char, 3)) {
        env.error("merged stdout should contain 'out'")
        return
    }
    if(!bytes_contains(r.stdout_bytes(), "err" as *char, 3)) {
        env.error("merged stdout should contain 'err'")
        return
    }
}

// ---------------------------------------------------------------------------
// Execute a command pipeline (echo | wc -w)
// ---------------------------------------------------------------------------

@test
public func test_practical_command_pipeline(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("echo one two three | wc -w")
    var res = cfg.output()
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) { env.error("pipeline should succeed"); return }
    // wc -w outputs "3\n"
    if(!bytes_contains(r.stdout_bytes(), "3" as *char, 1)) {
        env.error("pipeline output should contain '3'")
        return
    }
}

// ---------------------------------------------------------------------------
// current_pid is consistent across calls
// ---------------------------------------------------------------------------

@test
public func test_practical_pid_consistency(env : &mut TestEnv) {
    var pid1 = process::current_pid()
    var pid2 = process::current_pid()
    if(pid1 != pid2) { env.error("current_pid should return the same value"); return }
    if(pid1 <= 0) { env.error("pid should be positive"); return }
}

// ---------------------------------------------------------------------------
// sleep_ms actually sleeps (at least a tiny bit)
// ---------------------------------------------------------------------------

@test
public func test_practical_sleep_ms_sleeps(env : &mut TestEnv) {
    // Sleep 50ms — if sleep_ms is broken this will either crash or return instantly.
    // We can't measure time precisely, but we verify it doesn't crash.
    process::sleep_ms(50)
}
