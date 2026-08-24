// process — cross-platform process management.

public namespace process {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

public struct ProcessOutput {
    var stdout_data : vector<u8>;
    var stderr_data : vector<u8>;
}

public struct ExitStatus {
    var code : int;
    var signaled : bool;
    var signal : int;
}

public struct ProcessResult {
    var output : ProcessOutput;
    var status : ExitStatus;
    var success : bool;

    /// Get stdout as a string view (assumes UTF-8).
    public func stdout_str(&self) : string_view {
        return string_view(self.output.stdout_data.data() as *char, self.output.stdout_data.size())
    }

    /// Get stderr as a string view (assumes UTF-8).
    public func stderr_str(&self) : string_view {
        return string_view(self.output.stderr_data.data() as *char, self.output.stderr_data.size())
    }

    /// Get the exit code.
    public func exit_code(&self) : int {
        return self.status.code
    }

    /// Check whether the process exited successfully.
    public func is_success(&self) : bool {
        return self.success
    }
}

public struct ProcessConfig {
    var args : vector<string>;
    var env : vector<string>;
    var working_dir : string;
    var capture_stdout : bool;
    var capture_stderr : bool;
    var merge_stdout_stderr : bool;
    var stdin_data : vector<u8>;

    func default() : ProcessConfig {
        return ProcessConfig {
            args = vector<string>(),
            env = vector<string>(),
            working_dir = string(),
            capture_stdout = true,
            capture_stderr = true,
            merge_stdout_stderr = false,
            stdin_data = vector<u8>()
        }
    }
}

public struct ChildProcess {
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

public type PR_Result = Result<ProcessResult, ProcessError>
public type CP_Result = Result<ChildProcess, ProcessError>
public type UT_Result = Result<UnitTy, ProcessError>



func pr_ok(v : &mut ProcessResult, ret : &mut PR_Result) {
    std::replace<PR_Result>(ret, Result.Ok<ProcessResult, ProcessError>(std::replace<ProcessResult>(v, zeroed:unsafe<ProcessResult>())))
}
func pr_err(e : ProcessError, ret : &mut PR_Result) {
    std::replace<PR_Result>(ret, Result.Err<ProcessResult, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
}

public func execute(cfg : ProcessConfig) : PR_Result {
    var ret = zeroed:unsafe<PR_Result>()
    if(cfg.args.size() == 0) {
        var e = ProcessError.InvalidArgs(string("no arguments provided"))
        pr_err(e, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    } else {}
    comptime if(def.windows) {
        var result = zeroed:unsafe<ProcessResult>()
        if(win_execute(&raw mut cfg, &raw mut result)) {
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("exec failed"))
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        }
    } else {
        var result = zeroed:unsafe<ProcessResult>()
        if(posix_execute(&raw mut cfg, &raw mut result)) {
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("exec failed"))
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        }
    }
}

public func spawn(cfg : ProcessConfig) : CP_Result {
    var ret = zeroed:unsafe<CP_Result>()
    if(cfg.args.size() == 0) {
        var e = ProcessError.InvalidArgs(string("no arguments provided"))
        std::replace<CP_Result>(&mut ret, Result.Err<ChildProcess, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
        return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
    } else {}
    comptime if(def.windows) {
        var child = zeroed:unsafe<ChildProcess>()
        if(win_spawn(&raw mut cfg, &raw mut child)) {
            std::replace(&mut ret, Result.Ok<ChildProcess, ProcessError>(std::replace(&mut child, zeroed:unsafe<ChildProcess>())))
            return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("spawn failed"))
            std::replace<CP_Result>(&mut ret, Result.Err<ChildProcess, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
        }
    } else {
        var child = zeroed:unsafe<ChildProcess>()
        if(posix_spawn(&raw mut cfg, &raw mut child)) {
            std::replace(&mut ret, Result.Ok<ChildProcess, ProcessError>(std::replace(&mut child, zeroed:unsafe<ChildProcess>())))
            return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("spawn failed"))
            std::replace<CP_Result>(&mut ret, Result.Err<ChildProcess, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<CP_Result>(&mut ret, zeroed:unsafe<CP_Result>())
        }
    }
}

public func wait(child : *mut ChildProcess) : PR_Result {
    var ret = zeroed:unsafe<PR_Result>()
    if(!child.is_running) {
        var e = ProcessError.NotRunning()
        pr_err(e, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    } else {}
    comptime if(def.windows) {
        var result = zeroed:unsafe<ProcessResult>()
        if(win_wait(child, &raw mut result)) {
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("wait failed"))
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        }
    } else {
        var result = zeroed:unsafe<ProcessResult>()
        if(posix_wait(child, &raw mut result)) {
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        } else {
            var e = ProcessError.OperationFailed(string("wait failed"))
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        }
    }
}

public func kill(child : *mut ChildProcess, signal : int) : UT_Result {
    var ret = zeroed:unsafe<UT_Result>()
    if(!child.is_running) {
        var e = ProcessError.NotRunning()
        std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace(&mut e, ProcessError.NotRunning())))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    } else {}
    comptime if(def.windows) {
        var r = TerminateProcess(child.win.h_process, 1u32);
        if(r == 0) {
            var e = ProcessError.OperationFailed(string("TerminateProcess failed"))
            std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
        } else {}
        child.is_running = false;
        std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    } else {
        child.is_running = false;
        std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    }
}

// ---------------------------------------------------------------------------
// Process status & I/O
// ---------------------------------------------------------------------------

/// Non-blocking check whether a child process is still running.
public func is_running(child : *mut ChildProcess) : bool {
    if(!child.is_running) {
        return false
    }
    comptime if(def.windows) {
        // WaitForSingleObject with 0 timeout = non-blocking poll.
        var rc = WaitForSingleObject(child.win.h_process, 0u32)
        if(rc == 0u32) {
            // Process has exited.
            child.is_running = false
            return false
        }
        return true
    } else {
        var status : int = 0
        var ret = waitpid(child._unix.pid, &raw mut status, 1) // WNOHANG = 1
        if(ret == 0) {
            return true // still running
        }
        child.is_running = false
        return false
    }
}

/// Write data to the child process's stdin pipe.
/// Returns Err if the child has no stdin pipe or the write fails.
public func write_stdin(child : *mut ChildProcess, data : *vector<u8>) : UT_Result {
    var ret = zeroed:unsafe<UT_Result>()
    comptime if(def.windows) {
        if(child.win.h_stdin_write == null) {
            var e = ProcessError.InvalidArgs(string("no stdin pipe"))
            std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
        }
        if(data.size() > 0) {
            var written : DWORD = 0
            var ok = WriteFile(child.win.h_stdin_write, data.data() as *void, data.size() as DWORD, &raw mut written, null)
            if(ok == 0) {
                var e = ProcessError.IoError(string("write to stdin failed"))
                std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
            }
        }
        std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    } else {
        if(child._unix.stdin_fd < 0) {
            var e = ProcessError.InvalidArgs(string("no stdin pipe"))
            std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
            return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
        }
        if(data.size() > 0) {
            var written = write(child._unix.stdin_fd, data.data() as *void, data.size())
            if(written < 0) {
                var e = ProcessError.IoError(string("write to stdin failed"))
                std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
            }
        }
        std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    }
}

/// Close the stdin pipe of a child process (sends EOF to the child).
public func close_stdin(child : *mut ChildProcess) : UT_Result {
    var ret = zeroed:unsafe<UT_Result>()
    comptime if(def.windows) {
        if(child.win.h_stdin_write != null) {
            CloseHandle(child.win.h_stdin_write)
            child.win.h_stdin_write = null
        }
        std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    } else {
        if(child._unix.stdin_fd >= 0) {
            close(child._unix.stdin_fd)
            child._unix.stdin_fd = -1
        }
        std::replace(&mut ret, Result.Ok<UnitTy, ProcessError>(UnitTy{}))
        return std::replace<UT_Result>(&mut ret, zeroed:unsafe<UT_Result>())
    }
}

/// Reap the child process and return its exit status without blocking.
/// If the process is still running, returns NotRunning error.
public func try_wait(child : *mut ChildProcess) : PR_Result {
    var ret = zeroed:unsafe<PR_Result>()
    if(!child.is_running) {
        var e = ProcessError.NotRunning()
        pr_err(e, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    }
    comptime if(def.windows) {
        // Non-blocking: check with 0 timeout
        var rc = WaitForSingleObject(child.win.h_process, 0u32)
        if(rc == WAIT_TIMEOUT) {
            var e = ProcessError.NotRunning()
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        }
        // Process has exited
        var exit_code : DWORD = 0
        GetExitCodeProcess(child.win.h_process, &raw mut exit_code)
        // Read any remaining output
        var stdout_data = vector<u8>()
        var stderr_data = vector<u8>()
        if(child.win.h_stdout_read != null) {
            win_read_all(child.win.h_stdout_read, &raw mut stdout_data)
            CloseHandle(child.win.h_stdout_read)
            child.win.h_stdout_read = null
        }
        if(child.win.h_stderr_read != null) {
            win_read_all(child.win.h_stderr_read, &raw mut stderr_data)
            CloseHandle(child.win.h_stderr_read)
            child.win.h_stderr_read = null
        }
        if(child.win.h_stdin_write != null) {
            CloseHandle(child.win.h_stdin_write)
            child.win.h_stdin_write = null
        }
        CloseHandle(child.win.h_process)
        CloseHandle(child.win.h_thread)
        child.win.h_process = null
        child.win.h_thread = null
        child.is_running = false
        var result = zeroed:unsafe<ProcessResult>()
        result.output.stdout_data = stdout_data
        result.output.stderr_data = stderr_data
        result.status.code = exit_code as int
        result.status.signaled = false
        result.status.signal = 0
        result.success = (exit_code == 0)
        pr_ok(&mut result, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    } else {
        var status : int = 0
        var pid = waitpid(child._unix.pid, &raw mut status, 1) // WNOHANG
        if(pid == 0) {
            var e = ProcessError.NotRunning()
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
        }
        var result = zeroed:unsafe<ProcessResult>()
        var exit_code : int = 0
        var signaled : bool = false
        var signal_no : int = 0
        if(WIFEXITED(status)) { exit_code = WEXITSTATUS(status) }
        else if(WIFSIGNALED(status)) { signaled = true; signal_no = WTERMSIG(status); exit_code = -1 }
        child.is_running = false
        result.status.code = exit_code
        result.status.signaled = signaled
        result.status.signal = signal_no
        result.success = (exit_code == 0 && !signaled)
        pr_ok(&mut result, &mut ret)
        return std::replace<PR_Result>(&mut ret, zeroed:unsafe<PR_Result>())
    }
}

// ---------------------------------------------------------------------------
// Process-wide utilities
// ---------------------------------------------------------------------------

/// Get the current process ID.
public func current_pid() : int {
    comptime if(def.windows) {
        return GetCurrentProcessId() as int
    } else {
        return getpid()
    }
}

/// Sleep for the given number of milliseconds.
public func sleep_ms(ms : int) {
    comptime if(def.windows) {
        Sleep(ms as u32)
    } else {
        usleep(ms * 1000)
    }
}

/// Get the PID of a child process.
public func child_pid(child : *mut ChildProcess) : int {
    comptime if(def.windows) {
        return child.win.pid
    } else {
        return child._unix.pid
    }
}

} // end namespace process
