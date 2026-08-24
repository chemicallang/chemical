// process_practical_test.ch — Practical integration tests for the process library.
// These tests exercise real command execution, file I/O, stdin piping,
// environment variables, and working directory handling.

using std::string;
using std::string_view;
using std::vector;
using std::Result;

// Helper: does a byte slice contain the given substring? needle does NOT
// include a trailing null — we search for exact byte content.
func bytes_contains(data : *vector<u8>, needle : *char, needle_len : size_t) : bool {
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

// ---------------------------------------------------------------------------
// Run a real command and verify stdout content
// ---------------------------------------------------------------------------

@test
public func test_practical_echo_hello(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("hello_from_chemical"))
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("echo should succeed"); return }
    if(!bytes_contains(&raw r.output.stdout_data, "hello_from_chemical" as *char, 19)) {
        env.error("stdout should contain 'hello_from_chemical'")
        return
    }
}

// ---------------------------------------------------------------------------
// Run a command that writes to a file, verify the file
// ---------------------------------------------------------------------------

@test
public func test_practical_write_file(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo chemical_test_content > /tmp/chemical_test_write.txt"))
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("write command should succeed"); return }

    // Now read the file back
    var cfg2 = process::ProcessConfig.default()
    cfg2.args.push(string("sh"))
    cfg2.args.push(string("-c"))
    cfg2.args.push(string("cat /tmp/chemical_test_write.txt"))
    var res2 = process::execute(cfg2)
    if(res2 is Result.Err) { env.error("cat failed"); return }
    var Ok(r2) = res2 else unreachable
    if(!r2.success) { env.error("cat should succeed"); return }
    if(!bytes_contains(&raw r2.output.stdout_data, "chemical_test_content" as *char, 21)) {
        env.error("file should contain 'chemical_test_content'")
        return
    }
    // Cleanup
    var cleanup_cfg = process::ProcessConfig.default()
    cleanup_cfg.args.push(string("sh"))
    cleanup_cfg.args.push(string("-c"))
    cleanup_cfg.args.push(string("rm -f /tmp/chemical_test_write.txt"))
    process::execute(cleanup_cfg)
}

// ---------------------------------------------------------------------------
// Run a command with non-zero exit code and verify it
// ---------------------------------------------------------------------------

@test
public func test_practical_exit_code_42(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("exit 42"))
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(r.success) { env.error("exit 42 should not succeed"); return }
    if(r.exit_code() != 42) { env.error("expected exit code 42"); return }
}

// ---------------------------------------------------------------------------
// Stdin piping: write to cat, read from stdout
// ---------------------------------------------------------------------------

@test
public func test_practical_stdin_to_cat(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("cat"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) { env.error("spawn failed"); return }
    var Ok(child) = spawn_res else unreachable

    // Write data to stdin
    var input = vector<u8>()
    var msg = "piped_from_chemical"
    var i : size_t = 0
    while(i < 19) {
        input.push(msg[i] as u8)
        i += 1
    }
    var wr = process::write_stdin(&raw mut child, &raw input)
    if(wr is Result.Err) { env.error("write_stdin failed"); return }

    // Close stdin so cat finishes
    process::close_stdin(&raw mut child)

    // Wait for result
    var wait_res = process::wait(&raw mut child)
    if(wait_res is Result.Err) { env.error("wait failed"); return }
    var Ok(wr_res) = wait_res else unreachable
    if(!wr_res.success) { env.error("cat should succeed"); return }
    if(!bytes_contains(&raw wr_res.output.stdout_data, "piped_from_chemical" as *char, 19)) {
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
    environment::set("CHEMICAL_PRACTICAL_TEST_VAR", "practical_value")

    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo $CHEMICAL_PRACTICAL_TEST_VAR"))
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("command should succeed"); return }
    if(!bytes_contains(&raw r.output.stdout_data, "practical_value" as *char, 15)) {
        env.error("child should see CHEMICAL_PRACTICAL_TEST_VAR")
        return
    }
    // Cleanup
    environment::unset("CHEMICAL_PRACTICAL_TEST_VAR")
}

// ---------------------------------------------------------------------------
// Working directory: run pwd in /tmp
// ---------------------------------------------------------------------------

@test
public func test_practical_working_directory(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("pwd"))
    cfg.working_dir = string("/tmp")
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("pwd should succeed"); return }
    // pwd outputs /tmp followed by a newline; search for just "/tmp"
    if(!bytes_contains(&raw r.output.stdout_data, "/tmp" as *char, 4)) {
        env.error("pwd should output /tmp")
        return
    }
}

// ---------------------------------------------------------------------------
// Spawn, check is_running, wait for completion, verify output
// ---------------------------------------------------------------------------

@test
public func test_practical_spawn_lifecycle(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo alive && sleep 0.1 && echo done"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) { env.error("spawn failed"); return }
    var Ok(child) = spawn_res else unreachable

    // Should be running right after spawn
    if(!process::is_running(&raw mut child)) { env.error("should be running"); return }

    // Wait for it to finish
    var wait_res = process::wait(&raw mut child)
    if(wait_res is Result.Err) { env.error("wait failed"); return }
    var Ok(wr) = wait_res else unreachable
    if(!wr.success) { env.error("should succeed"); return }

    // Should not be running now
    if(process::is_running(&raw mut child)) { env.error("should not be running after wait"); return }

    // Output should contain both lines
    if(!bytes_contains(&raw wr.output.stdout_data, "alive" as *char, 5)) {
        env.error("stdout should contain 'alive'")
        return
    }
    if(!bytes_contains(&raw wr.output.stdout_data, "done" as *char, 4)) {
        env.error("stdout should contain 'done'")
        return
    }
}

// ---------------------------------------------------------------------------
// Kill a long-running process
// ---------------------------------------------------------------------------

@test
public func test_practical_kill_long_process(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sleep"))
    cfg.args.push(string("300"))
    var spawn_res = process::spawn(cfg)
    if(spawn_res is Result.Err) { env.error("spawn failed"); return }
    var Ok(child) = spawn_res else unreachable

    // Verify it's running
    if(!process::is_running(&raw mut child)) { env.error("sleep should be running"); return }

    // Kill it
    var kr = process::kill_process(&raw mut child, 9)
    if(kr is Result.Err) { env.error("kill should succeed"); return }

    // Should not be running
    if(process::is_running(&raw mut child)) { env.error("should not be running after kill"); return }
}

// ---------------------------------------------------------------------------
// Multiple sequential processes
// ---------------------------------------------------------------------------

@test
public func test_practical_sequential_processes(env : &mut TestEnv) {
    var i = 0
    while(i < 5) {
        var cfg = process::ProcessConfig.default()
        cfg.args.push(string("echo"))
        cfg.args.push(string("seq_test"))
        var res = process::execute(cfg)
        if(res is Result.Err) { env.error("execute failed in loop"); return }
        var Ok(r) = res else unreachable
        if(!r.success) { env.error("should succeed in loop"); return }
        if(!bytes_contains(&raw r.output.stdout_data, "seq_test" as *char, 8)) {
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
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo out && echo err >&2"))
    cfg.merge_stdout_stderr = true
    cfg.capture_stderr = false
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("should succeed"); return }
    // Both stdout and stderr should be merged into stdout_data
    if(!bytes_contains(&raw r.output.stdout_data, "out" as *char, 3)) {
        env.error("merged stdout should contain 'out'")
        return
    }
    if(!bytes_contains(&raw r.output.stdout_data, "err" as *char, 3)) {
        env.error("merged stdout should contain 'err'")
        return
    }
}

// ---------------------------------------------------------------------------
// Execute a command pipeline (echo | wc -w)
// ---------------------------------------------------------------------------

@test
public func test_practical_command_pipeline(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo one two three | wc -w"))
    var res = process::execute(cfg)
    if(res is Result.Err) { env.error("execute failed"); return }
    var Ok(r) = res else unreachable
    if(!r.success) { env.error("pipeline should succeed"); return }
    // wc -w outputs "3\n"
    if(!bytes_contains(&raw r.output.stdout_data, "3" as *char, 1)) {
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
