// process — cross-platform process management.
// Provides synchronous and async process spawning with output capture.

public namespace process {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ---------------------------------------------------------------------------
// Core types
// ---------------------------------------------------------------------------

public struct ProcessOutput {
    var stdout_data : vector<u8>;
    var stderr_data : vector<u8>;
}

public struct ExitStatus {
    var code : int; // -1 if signaled
    var signaled : bool;
    var signal : int; // 0 if not signaled
}

public struct ProcessResult {
    var output : ProcessOutput;
    var status : ExitStatus;
    var success : bool;
}

public struct ProcessConfig {
    var args : vector<string>;
    var env : vector<string>;      // "KEY=VALUE" pairs, empty = inherit
    var working_dir : string;
    var capture_stdout : bool;
    var capture_stderr : bool;
    var merge_stdout_stderr : bool;
    var stdin_data : vector<u8>;   // data to write to stdin

    func default() : ProcessConfig {
        var cfg : ProcessConfig;
        cfg.capture_stdout = true;
        cfg.capture_stderr = true;
        cfg.merge_stdout_stderr = false;
        return cfg;
    }
}

// ---------------------------------------------------------------------------
// Synchronous process execution
// ---------------------------------------------------------------------------

/// Launch a process and wait for it to complete.
/// Returns the captured output and exit status.
public func execute(cfg : ProcessConfig) : Result<ProcessResult, ProcessError> {
    // Validate
    if(cfg.args.size() == 0) { return Result.Err(ProcessError.InvalidArgs("no arguments provided")); }

    comptime if(def.windows) {
        return windows_execute(cfg);
    } else {
        return posix_execute(cfg);
    }
}

/// Convenience: run a command with args, capture output.
public func run(program : string_view, args : vector<string_view>) : Result<ProcessResult, ProcessError> {
    var cfg = ProcessConfig.default();
    cfg.args.push(string(program));
    var i : size_t = 0;
    while(i < args.size()) {
        cfg.args.push(string(args.get(i)));
        i += 1;
    }
    return execute(cfg);
}

/// Convenience: run a single command string through the shell.
public func run_shell(cmd : string_view) : Result<ProcessResult, ProcessError> {
    comptime if(def.windows) {
        var cfg = ProcessConfig.default();
        cfg.args.push(string("cmd.exe"));
        cfg.args.push(string("/C"));
        cfg.args.push(string(cmd));
        return execute(cfg);
    } else {
        var cfg = ProcessConfig.default();
        cfg.args.push(string("/bin/sh"));
        cfg.args.push(string("-c"));
        cfg.args.push(string(cmd));
        return execute(cfg);
    }
}

// ---------------------------------------------------------------------------
// Async process (future-based)
// ---------------------------------------------------------------------------

/// A handle to a running process.
public struct ChildProcess {
    comptime if(def.windows) {
        var h_process : *mut void;
        var h_thread : *mut void;
        var h_stdout_read : *mut void;
        var h_stderr_read : *mut void;
        var h_stdin_write : *mut void;
    } else {
        var pid : int;
        var stdout_fd : int;
        var stderr_fd : int;
        var stdin_fd : int;
    }
    var is_running : bool;
    var captured_stdout : vector<u8>;
    var captured_stderr : vector<u8>;
}

/// Spawn a process without waiting for it to complete.
/// Returns a ChildProcess handle that can be awaited later.
public func spawn(cfg : ProcessConfig) : Result<ChildProcess, ProcessError> {
    if(cfg.args.size() == 0) { return Result.Err(ProcessError.InvalidArgs("no arguments provided")); }

    comptime if(def.windows) {
        return windows_spawn(cfg);
    } else {
        return posix_spawn(cfg);
    }
}

/// Wait for a child process to complete and collect output.
public func wait(child : *mut ChildProcess) : Result<ProcessResult, ProcessError> {
    if(!child.is_running) { return Result.Err(ProcessError.NotRunning()); }

    comptime if(def.windows) {
        return windows_wait(child);
    } else {
        return posix_wait(child);
    }
}

/// Convenience: spawn + wait (async pattern for futures).
/// Users can call spawn(), do other work, then call wait().
/// This function does both in one call (synchronous convenience).
public func spawn_and_wait(cfg : ProcessConfig) : Result<ProcessResult, ProcessError> {
    var child_r = spawn(cfg);
    if(child_r is Result.Err) {
        var Err(e) = child_r else unreachable;
        return Result.Err(e);
    }
    var Ok(child) = child_r else unreachable;
    return wait(&raw mut child);
}

// ---------------------------------------------------------------------------
// Send signal / kill
// ---------------------------------------------------------------------------

/// Send a signal to a process (SIGTERM by default on POSIX, TerminateProcess on Windows).
public func kill(child : *mut ChildProcess, signal : int) : Result<UnitTy, ProcessError> {
    if(!child.is_running) { return Result.Err(ProcessError.NotRunning()); }

    comptime if(def.windows) {
        var r = TerminateProcess(child.h_process, 1u32);
        if(r == 0) { return Result.Err(ProcessError.OperationFailed("TerminateProcess failed")); }
        child.is_running = false;
        return Result.Ok(UnitTy{});
    } else {
        var r = kill_pid(child.pid, signal);
        if(r != 0) { return Result.Err(ProcessError.OperationFailed("kill failed")); }
        return Result.Ok(UnitTy{});
    }
}

// ---------------------------------------------------------------------------
// Platform-specific implementations (see platform files)
// ---------------------------------------------------------------------------

comptime if(def.windows) {
    func windows_execute(cfg : ProcessConfig) : Result<ProcessResult, ProcessError>
    func windows_spawn(cfg : ProcessConfig) : Result<ChildProcess, ProcessError>
    func windows_wait(child : *mut ChildProcess) : Result<ProcessResult, ProcessError>
}

comptime if(!def.windows) {
    func posix_execute(cfg : ProcessConfig) : Result<ProcessResult, ProcessError>
    func posix_spawn(cfg : ProcessConfig) : Result<ChildProcess, ProcessError>
    func posix_wait(child : *mut ChildProcess) : Result<ProcessResult, ProcessError>
}

} // end namespace process
