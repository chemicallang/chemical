// process — cross-platform process management.

public namespace process {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// Internal build configuration. Public construction goes through `Command`.
public struct ProcessConfig {
    internal var args : vector<string>;
    internal var env : vector<string>;
    internal var working_dir : string;
    internal var capture_stdout : bool;
    internal var capture_stderr : bool;
    internal var merge_stdout_stderr : bool;
    internal var stdin_data : vector<u8>;
    internal var timeout_ms : int;

    func default() : ProcessConfig {
        return ProcessConfig {
            args = vector<string>(),
            env = vector<string>(),
            working_dir = string(),
            capture_stdout = true,
            capture_stderr = true,
            merge_stdout_stderr = false,
            stdin_data = vector<u8>(),
            timeout_ms = 0
        }
    }
}

// Internal platform result. The public surface is `Output`.
public struct ProcessResult {
    internal var output : ProcessOutput;
    internal var status : ExitStatus;
    internal var success : bool;

    func exit_code(&self) : int {
        return self.status._code
    }

    func is_success(&self) : bool {
        return self.success
    }
}

public struct ProcessOutput {
    internal var stdout_data : vector<u8>;
    internal var stderr_data : vector<u8>;
}

protected struct ChildProcess {
    if(def.windows) {
        struct {
            var h_process : *mut void;
            var h_thread : *mut void;
            var h_stdout_read : *mut void;
            var h_stderr_read : *mut void;
            var h_stdin_write : *mut void;
            var pid : int;
        } win;
    } else {
        struct {
            var pid : int;
            var stdout_fd : int;
            var stderr_fd : int;
            var stdin_fd : int;
        } _unix;
    }
    var is_running : bool;
    var captured_stdout : vector<u8>;
    var captured_stderr : vector<u8>;
}

// ---------------------------------------------------------------------------
// Command builder
// ---------------------------------------------------------------------------

/// Builder for a child process. The configuration is private; use the
/// `arg`/`args`/`env`/`current_dir`/`std*` methods.
public struct Command {
    internal var cfg : ProcessConfig
    internal var stdin_mode : Stdio
    internal var stdout_mode : Stdio
    internal var stderr_mode : Stdio

    @make
    func new(program : &str) : Command {
        var cfg = ProcessConfig.default()
        if(program.size() > 0) {
            cfg.args.push(string(program.to_view()))
        }
        return Command {
            cfg : cfg,
            stdin_mode : Stdio.Inherit(),
            stdout_mode : Stdio.Piped(),
            stderr_mode : Stdio.Piped()
        }
    }

    func arg(&mut self, arg : &str) : &mut Command {
        self.cfg.args.push(string(arg.to_view()))
        return self
    }

    /// Replace the argv vector (including argv[0]).
    func set_argv(&mut self, args : &vector<string>) : &mut Command {
        self.cfg.args.clear()
        var i : size_t = 0
        while(i < args.size()) {
            self.cfg.args.push(unsafe(args.get_ptr(i).copy()))
            i += 1
        }
        return self
    }

    func args(&mut self, values : &[&str]) : &mut Command {
        var i : size_t = 0
        while(i < values.size()) {
            var sv = unsafe(*values.at(i))
            self.cfg.args.push(string(sv.to_view()))
            i += 1
        }
        return self
    }

    func env(&mut self, key : &str, value : &str) : &mut Command {
        var entry = string(key.to_view())
        entry.append('=')
        entry.append_view(value.to_view())
        self.cfg.env.push(entry)
        return self
    }

    func current_dir(&mut self, dir : &path::Path) : &mut Command {
        self.cfg.working_dir = dir.as_string()
        return self
    }

    func stdin(&mut self, stdio : Stdio) : &mut Command {
        self.stdin_mode = stdio
        return self
    }

    func stdout(&mut self, stdio : Stdio) : &mut Command {
        self.stdout_mode = stdio
        return self
    }

    func stderr(&mut self, stdio : Stdio) : &mut Command {
        self.stderr_mode = stdio
        return self
    }

    /// Legacy convenience: append a byte to buffered stdin.
    func stdin_push(&mut self, byte : u8) : &mut Command {
        self.cfg.stdin_data.push(byte)
        return self
    }

    /// Legacy convenience: merge stderr into stdout.
    func merge_stdout_stderr(&mut self, value : bool) : &mut Command {
        self.cfg.merge_stdout_stderr = value
        return self
    }

    /// Legacy convenience: execution timeout in milliseconds (0 = none).
    func timeout_ms(&mut self, ms : int) : &mut Command {
        self.cfg.timeout_ms = ms
        return self
    }

    func fill_config(&self, out : *mut ProcessConfig) {
        var cfg = out
        unsafe {
            cfg.args.clear()
            cfg.env.clear()
            cfg.stdin_data.clear()
        }
        var i : size_t = 0
        while(i < self.cfg.args.size()) {
            unsafe { cfg.args.push(self.cfg.args.get_ptr(i).copy()) }
            i += 1
        }
        i = 0
        while(i < self.cfg.env.size()) {
            unsafe { cfg.env.push(self.cfg.env.get_ptr(i).copy()) }
            i += 1
        }
        unsafe {
            cfg.working_dir = self.cfg.working_dir.copy()
            cfg.capture_stdout = self.stdout_mode is Stdio.Piped
            cfg.capture_stderr = self.stderr_mode is Stdio.Piped
            cfg.merge_stdout_stderr = self.cfg.merge_stdout_stderr
        }
        i = 0
        while(i < self.cfg.stdin_data.size()) {
            unsafe { cfg.stdin_data.push(self.cfg.stdin_data.at(i)) }
            i += 1
        }
        unsafe { cfg.timeout_ms = self.cfg.timeout_ms }
    }

    /// Run to completion and capture stdout/stderr.
    func output(&mut self) : Result<Output, ProcessError> {
        var cfg = ProcessConfig.default()
        self.fill_config(&raw mut cfg)
        var r = execute(std::replace<ProcessConfig>(&mut cfg, unsafe(std::mem.zeroed_unchecked<ProcessConfig>())))
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(pr) = r else unsafe { unreachable }
        return Result.Ok(Output {
            _status : pr.status,
            stdout_data : std::replace<std::vector<u8>>(&mut pr.output.stdout_data, unsafe(std::mem.zeroed_unchecked<std::vector<u8>>())),
            stderr_data : std::replace<std::vector<u8>>(&mut pr.output.stderr_data, unsafe(std::mem.zeroed_unchecked<std::vector<u8>>()))
        })
    }

    /// Run to completion and report only the exit status.
    func status(&mut self) : Result<ExitStatus, ProcessError> {
        var cfg = ProcessConfig.default()
        self.fill_config(&raw mut cfg)
        cfg.capture_stdout = false
        cfg.capture_stderr = false
        cfg.merge_stdout_stderr = false
        var r = execute(std::replace<ProcessConfig>(&mut cfg, unsafe(std::mem.zeroed_unchecked<ProcessConfig>())))
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(pr) = r else unsafe { unreachable }
        return Result.Ok(pr.status)
    }

    /// Start the child process without waiting for it.
    func spawn(&mut self) : Result<Child, ProcessError> {
        var cfg = ProcessConfig.default()
        self.fill_config(&raw mut cfg)
        return process::spawn(std::replace<ProcessConfig>(&mut cfg, unsafe(std::mem.zeroed_unchecked<ProcessConfig>())))
    }
}

// Owning child-process handle.
//
// `@delete` closes the owned stdin/stdout/stderr pipes and reaps the child if it
// has already exited, but it does NOT implicitly kill a still-running process.
// Call `kill` (or `wait`) explicitly if termination/reaping must be guaranteed.
public struct Child {
    internal var inner : ChildProcess;
    internal var cached : bool;
    internal var cached_status : ExitStatus;

    @delete
    func delete(&mut self) {
        process_cleanup(&raw mut self.inner)
    }

    /// Block until the child exits. Idempotent: after the first successful wait
    /// the cached status is returned.
    func wait(&mut self) : Result<ExitStatus, ProcessError> {
        if(self.cached) {
            return Result.Ok(self.cached_status)
        }
        var r = wait_raw(&raw mut self.inner)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(pr) = r else unsafe { unreachable }
        self.cached = true
        self.cached_status = pr.status
        return Result.Ok(self.cached_status)
    }

    /// Block until the child exits and return captured stdout/stderr. A second
    /// call returns the cached status with empty capture buffers.
    func wait_with_output(&mut self) : Result<Output, ProcessError> {
        if(self.cached) {
            return Result.Ok(Output {
                _status : self.cached_status,
                stdout_data : vector<u8>(),
                stderr_data : vector<u8>()
            })
        }
        var r = wait_raw(&raw mut self.inner)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(pr) = r else unsafe { unreachable }
        self.cached = true
        self.cached_status = pr.status
        return Result.Ok(Output {
            _status : pr.status,
            stdout_data : std::replace<std::vector<u8>>(&mut pr.output.stdout_data, unsafe(std::mem.zeroed_unchecked<std::vector<u8>>())),
            stderr_data : std::replace<std::vector<u8>>(&mut pr.output.stderr_data, unsafe(std::mem.zeroed_unchecked<std::vector<u8>>()))
        })
    }

    /// Non-blocking check. Returns `Some(status)` once the child has exited and
    /// `None` while it is still running. The result is cached.
    func try_wait(&mut self) : Result<Option<ExitStatus>, ProcessError> {
        if(self.cached) {
            return Result.Ok(Option.Some<ExitStatus>(self.cached_status))
        }
        var status : ExitStatus = ExitStatus { _code : 0, _signaled : false, _signal : 0 }
        var r = try_wait_status_raw(&raw mut self.inner, &raw mut status)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(exited) = r else unsafe { unreachable }
        if(!exited) {
            return Result.Ok(Option.None<ExitStatus>())
        }
        self.cached = true
        self.cached_status = status
        return Result.Ok(Option.Some<ExitStatus>(self.cached_status))
    }

    /// Terminate the child with SIGKILL on POSIX / TerminateProcess on Windows.
    func kill(&mut self) : Result<UnitTy, ProcessError> {
        return kill_process_raw(&raw mut self.inner, 9)
    }

    /// Write bytes to the child's stdin pipe.
    func write_stdin(&mut self, data : &[u8]) : Result<UnitTy, ProcessError> {
        return write_stdin_raw(&raw mut self.inner, data.data(), data.size())
    }

    /// Close the child's stdin pipe (sends EOF).
    func close_stdin(&mut self) : Result<UnitTy, ProcessError> {
        return close_stdin_raw(&raw mut self.inner)
    }

    /// The child's process id. Zero after the child has been reaped.
    func id(&self) : u32 {
        return child_pid_raw(&raw self.inner) as u32
    }
}

public type PR_Result = Result<ProcessResult, ProcessError>
public type CP_Result = Result<Child, ProcessError>
public type UT_Result = Result<UnitTy, ProcessError>

func pr_ok(v : &mut ProcessResult, ret : &mut PR_Result) {
    std::replace<PR_Result>(ret, Result.Ok<ProcessResult, ProcessError>(std::replace<ProcessResult>(v, unsafe(std::mem.zeroed_unchecked<ProcessResult>()))))
}
func pr_err(e : ProcessError, ret : &mut PR_Result) {
    std::replace<PR_Result>(ret, Result.Err<ProcessResult, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
}

public func execute(cfg : ProcessConfig) : PR_Result {
    var ret : PR_Result = unsafe(std::mem.zeroed_unchecked<PR_Result>())
    if(cfg.args.size() == 0) {
        var e = ProcessError.InvalidArgs(string("no arguments provided"))
        pr_err(e, &mut ret)
        return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
    } else {}
    comptime if(def.windows) {
        var result : ProcessResult = unsafe(std::mem.zeroed_unchecked<ProcessResult>())
        if(win_execute(&raw mut cfg, &raw mut result)) {
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
        } else {
            var e = ProcessError.OperationFailed(string("exec failed"))
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
        }
    } else {
        var result : ProcessResult = unsafe(std::mem.zeroed_unchecked<ProcessResult>())
        if(posix_execute(&raw mut cfg, &raw mut result)) {
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
        } else {
            var e = ProcessError.OperationFailed(string("exec failed"))
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
        }
    }
}

public func spawn(cfg : ProcessConfig) : CP_Result {
    var ret : CP_Result = unsafe(std::mem.zeroed_unchecked<CP_Result>())
    if(cfg.args.size() == 0) {
        var e = ProcessError.InvalidArgs(string("no arguments provided"))
        std::replace<CP_Result>(&mut ret, Result.Err<Child, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
        return std::replace<CP_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<CP_Result>()))
    } else {}
    comptime if(def.windows) {
        var child : ChildProcess = unsafe(std::mem.zeroed_unchecked<ChildProcess>())
        if(win_spawn(&raw mut cfg, &raw mut child)) {
            var owned = Child {
                inner : std::replace<ChildProcess>(&mut child, unsafe(std::mem.zeroed_unchecked<ChildProcess>())),
                cached : false,
                cached_status : ExitStatus { _code : 0, _signaled : false, _signal : 0 }
            }
            std::replace(&mut ret, Result.Ok<Child, ProcessError>(owned))
            return std::replace<CP_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<CP_Result>()))
        } else {
            var e = ProcessError.OperationFailed(string("spawn failed"))
            std::replace<CP_Result>(&mut ret, Result.Err<Child, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<CP_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<CP_Result>()))
        }
    } else {
        var child : ChildProcess = unsafe(std::mem.zeroed_unchecked<ChildProcess>())
        if(posix_spawn(&raw mut cfg, &raw mut child)) {
            var owned = Child {
                inner : std::replace<ChildProcess>(&mut child, unsafe(std::mem.zeroed_unchecked<ChildProcess>())),
                cached : false,
                cached_status : ExitStatus { _code : 0, _signaled : false, _signal : 0 }
            }
            std::replace(&mut ret, Result.Ok<Child, ProcessError>(owned))
            return std::replace<CP_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<CP_Result>()))
        } else {
            var e = ProcessError.OperationFailed(string("spawn failed"))
            std::replace<CP_Result>(&mut ret, Result.Err<Child, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<CP_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<CP_Result>()))
        }
    }
}

func wait_raw(child : *mut ChildProcess) : PR_Result {
        unsafe {
        var ret : PR_Result = unsafe(std::mem.zeroed_unchecked<PR_Result>())
        // Allow wait() after kill() — process was killed but needs to be reaped.
        // Only reject if the process handle has already been cleaned up.
        comptime if(def.windows) {
            if(!child.is_running && child.win.h_process == null) {
                var e = ProcessError.NotRunning()
                pr_err(e, &mut ret)
                return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
            } else {}
        } else {
            if(child._unix.pid == 0) {
                var e = ProcessError.NotRunning()
                pr_err(e, &mut ret)
                return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
            } else {}
        }
        comptime if(def.windows) {
            var result : ProcessResult = unsafe(std::mem.zeroed_unchecked<ProcessResult>())
            if(win_wait(child, &raw mut result)) {
                pr_ok(&mut result, &mut ret)
                return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
            } else {
                var e = ProcessError.OperationFailed(string("wait failed"))
                pr_err(e, &mut ret)
                return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
            }
        } else {
            var result : ProcessResult = unsafe(std::mem.zeroed_unchecked<ProcessResult>())
            if(posix_wait(child, &raw mut result)) {
                pr_ok(&mut result, &mut ret)
                return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
            } else {
                var e = ProcessError.OperationFailed(string("wait failed"))
                pr_err(e, &mut ret)
                return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
            }
        }

        }
    }

/// Non-blocking reap. Returns true when a status was produced and written to
/// `out`, false while the child is still running.
func try_wait_status_raw(child : *mut ChildProcess, out : *mut ExitStatus) : Result<bool, ProcessError> {
        unsafe {
        if(!child.is_running) {
            comptime if(def.windows) {
                if(child.win.h_process == null) { return Result.Ok(false) }
            } else {
                if(child._unix.pid == 0) { return Result.Ok(false) }
            }
        }
        comptime if(def.windows) {
            var rc = WaitForSingleObject(child.win.h_process, 0u32)
            if(rc == WAIT_TIMEOUT) { return Result.Ok(false) }
            var exit_code : DWORD = 0
            GetExitCodeProcess(child.win.h_process, &raw mut exit_code)
            CloseHandle(child.win.h_process)
            CloseHandle(child.win.h_thread)
            child.win.h_process = null
            child.win.h_thread = null
            if(child.win.h_stdout_read != null) { CloseHandle(child.win.h_stdout_read); child.win.h_stdout_read = null }
            if(child.win.h_stderr_read != null) { CloseHandle(child.win.h_stderr_read); child.win.h_stderr_read = null }
            if(child.win.h_stdin_write != null) { CloseHandle(child.win.h_stdin_write); child.win.h_stdin_write = null }
            child.is_running = false
            out._code = exit_code as int
            out._signaled = false
            out._signal = 0
            return Result.Ok(true)
        } else {
            var status : int = 0
            var pid = waitpid(child._unix.pid, &raw mut status, 1) // WNOHANG
            if(pid == 0) { return Result.Ok(false) }
            if(pid < 0) {
                return Result.Err(ProcessError.OperationFailed(string("waitpid failed")))
            }
            var exit_code : int = 0
            var signaled : bool = false
            var signal_no : int = 0
            if(WIFEXITED(status)) { exit_code = WEXITSTATUS(status) }
            else if(WIFSIGNALED(status)) { signaled = true; signal_no = WTERMSIG(status); exit_code = -1 }
            child.is_running = false
            child._unix.pid = 0
            if(child._unix.stdout_fd >= 0) { close(child._unix.stdout_fd); child._unix.stdout_fd = -1 }
            if(child._unix.stderr_fd >= 0) { close(child._unix.stderr_fd); child._unix.stderr_fd = -1 }
            if(child._unix.stdin_fd >= 0) { close(child._unix.stdin_fd); child._unix.stdin_fd = -1 }
            out._code = exit_code
            out._signaled = signaled
            out._signal = signal_no
            return Result.Ok(true)
        }

        }
    }

func kill_process_raw(child : *mut ChildProcess, signal : int) : UT_Result {
        unsafe {
        var ret = unsafe(std::mem.zeroed_unchecked<UT_Result>())
        if(!child.is_running) {
            var e = ProcessError.NotRunning()
            std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
        } else {}
        comptime if(def.windows) {
            var r = TerminateProcess(child.win.h_process, 1u32);
            if(r == 0) {
                var e = ProcessError.OperationFailed(string("TerminateProcess failed"))
                std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
            } else {}
            if(child.win.h_stdin_write != null) {
                CloseHandle(child.win.h_stdin_write)
                child.win.h_stdin_write = null
            }
            if(child.win.h_stdout_read != null) {
                CloseHandle(child.win.h_stdout_read)
                child.win.h_stdout_read = null
            }
            if(child.win.h_stderr_read != null) {
                CloseHandle(child.win.h_stderr_read)
                child.win.h_stderr_read = null
            }
            child.is_running = false
            std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
            return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
        } else {
            var r = unsafe(kill(child._unix.pid, signal));
            if(r != 0) {
                var e = ProcessError.OperationFailed(string("kill failed"))
                std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
            } else {}
            child.is_running = false
            std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
            return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
        }

        }
    }

/// Non-blocking check whether a child process is still running.
func is_running_raw(child : *mut ChildProcess) : bool {
        unsafe {
        if(!child.is_running) {
            return false
        }
        comptime if(def.windows) {
            var rc = WaitForSingleObject(child.win.h_process, 0u32)
            if(rc == 0u32) {
                child.is_running = false
                return false
            }
            return true
        } else {
            var status : int = 0
            var ret = unsafe(waitpid(child._unix.pid, &raw mut status, 1)) // WNOHANG = 1
            if(ret == 0) {
                return true
            }
            child.is_running = false
            return false
        }

        }
    }

func write_stdin_raw(child : *mut ChildProcess, data : *u8, len : size_t) : UT_Result {
        unsafe {
        var ret = unsafe(std::mem.zeroed_unchecked<UT_Result>())
        comptime if(def.windows) {
            if(child.win.h_stdin_write == null) {
                var e = ProcessError.InvalidArgs(string("no stdin pipe"))
                std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
            }
            if(len > 0) {
                var written : DWORD = 0
                var ok = WriteFile(child.win.h_stdin_write, data as *void, len as DWORD, &raw mut written, null)
                if(ok == 0) {
                    var e = ProcessError.IoError(string("write to stdin failed"))
                    std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                    return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
                }
            }
            std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
            return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
        } else {
            if(child._unix.stdin_fd < 0) {
                var e = ProcessError.InvalidArgs(string("no stdin pipe"))
                std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
            }
            if(len > 0) {
                var written = unsafe(write(child._unix.stdin_fd, data as *void, len))
                if(written < 0) {
                    var e = ProcessError.IoError(string("write to stdin failed"))
                    std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                    return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
                }
            }
            std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
            return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
        }

        }
    }

/// Close the stdin pipe of a child process (sends EOF to the child).
func close_stdin_raw(child : *mut ChildProcess) : UT_Result {
        unsafe {
        var ret = unsafe(std::mem.zeroed_unchecked<UT_Result>())
        comptime if(def.windows) {
            if(child.win.h_stdin_write != null) {
                CloseHandle(child.win.h_stdin_write)
                child.win.h_stdin_write = null
            }
            std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
            return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
        } else {
            if(child._unix.stdin_fd >= 0) {
                unsafe { close(child._unix.stdin_fd) }
                child._unix.stdin_fd = -1
            }
            std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
            return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
        }

        }
    }

// ---------------------------------------------------------------------------
// Free compatibility aliases
// ---------------------------------------------------------------------------

/// Block until the child exits. Idempotent.
public func wait(child : &mut Child) : Result<ExitStatus, ProcessError> {
    return child.wait()
}

/// Non-blocking wait. `None` while the child is still running.
public func try_wait(child : &mut Child) : Result<Option<ExitStatus>, ProcessError> {
    return child.try_wait()
}

/// Signal the child. On Windows the signal is ignored and the process is killed.
public func kill_process(child : &mut Child, signal : int) : UT_Result {
    return kill_process_raw(&raw mut child.inner, signal)
}

/// Poll whether the child is still running.
public func is_running(child : &mut Child) : bool {
    return is_running_raw(&raw mut child.inner)
}

/// Write bytes to the child's stdin pipe.
public func write_stdin(child : &mut Child, data : &[u8]) : UT_Result {
    return child.write_stdin(data)
}

/// Close the child's stdin pipe (sends EOF).
public func close_stdin(child : &mut Child) : UT_Result {
    return child.close_stdin()
}

/// Return the child's process id.
public func child_pid(child : &mut Child) : int {
    return child_pid_raw(&raw child.inner)
}

// ---------------------------------------------------------------------------
// Process-wide utilities
// ---------------------------------------------------------------------------

/// Get the current process ID.
public func current_pid() : int {
    comptime if(def.windows) {
        return GetCurrentProcessId() as int
    } else {
        return unsafe(getpid())
    }
}

/// Sleep for the given number of milliseconds.
public func sleep_ms(ms : int) {
    comptime if(def.windows) {
        Sleep(ms as u32)
    } else {
        unsafe { usleep(ms * 1000) }
    }
}

/// Get the PID of a child process.
func child_pid_raw(child : *ChildProcess) : int {
        unsafe {
        comptime if(def.windows) {
            return child.win.pid
        } else {
            return child._unix.pid
        }

        }
    }

} // end namespace process
