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
        return unsafe(string_view(self.output.stdout_data.data() as *char, self.output.stdout_data.size()))
    }

    /// Get stderr as a string view (assumes UTF-8).
    public func stderr_str(&self) : string_view {
        return unsafe(string_view(self.output.stderr_data.data() as *char, self.output.stderr_data.size()))
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
    var timeout_ms : int;

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

// Owning child-process handle. The internal platform handle is private; all
// public operations take `&mut Child` and return `Result` errors.
public struct Child {
    internal var inner : ChildProcess;

    @delete
    func delete(&mut self) {
        process_cleanup(&raw mut self.inner)
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
            var owned = Child { inner : std::replace<ChildProcess>(&mut child, unsafe(std::mem.zeroed_unchecked<ChildProcess>())) }
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
            var owned = Child { inner : std::replace<ChildProcess>(&mut child, unsafe(std::mem.zeroed_unchecked<ChildProcess>())) }
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
            // Allow wait() after kill(): the process was signalled but still needs
            // to be reaped. Only reject once it has already been waited on (pid 0).
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

func kill_process_raw(child : *mut ChildProcess, signal : int) : UT_Result {
        unsafe {
        var ret = unsafe(std::mem.zeroed_unchecked<UT_Result>())
        if(!child.is_running) {
            var e = ProcessError.NotRunning()
            std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace(&mut e, ProcessError.NotRunning())))
            return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
        } else {}
        comptime if(def.windows) {
            var r = TerminateProcess(child.win.h_process, 1u32);
            if(r == 0) {
                var e = ProcessError.OperationFailed(string("TerminateProcess failed"))
                std::replace(&mut ret, Result.Err<UnitTy, ProcessError>(std::replace<ProcessError>(&mut e, ProcessError.NotRunning())))
                return std::replace<UT_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<UT_Result>()))
            } else {}
            // Close pipe handles so win_read_all doesn't block on orphaned children.
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

// ---------------------------------------------------------------------------
// Process status & I/O
// ---------------------------------------------------------------------------

/// Non-blocking check whether a child process is still running.
func is_running_raw(child : *mut ChildProcess) : bool {
        unsafe {
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
            var ret = unsafe(waitpid(child._unix.pid, &raw mut status, 1)) // WNOHANG = 1
            if(ret == 0) {
                return true // still running
            }
            child.is_running = false
            return false
        }

        }
    }

/// Write data to the child process's stdin pipe.
/// Returns Err if the child has no stdin pipe or the write fails.
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

/// Reap the child process and return its exit status without blocking.
/// If the process is still running, returns NotRunning error.
func try_wait_raw(child : *mut ChildProcess) : PR_Result {
        unsafe {
        var ret : PR_Result = unsafe(std::mem.zeroed_unchecked<PR_Result>())
        if(!child.is_running) {
            var e = ProcessError.NotRunning()
            pr_err(e, &mut ret)
            return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
        }
        comptime if(def.windows) {
            // Non-blocking: check with 0 timeout
            var rc = WaitForSingleObject(child.win.h_process, 0u32)
            if(rc == WAIT_TIMEOUT) {
                var e = ProcessError.NotRunning()
                pr_err(e, &mut ret)
                return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
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
            var result : ProcessResult = unsafe(std::mem.zeroed_unchecked<ProcessResult>())
            result.output.stdout_data = stdout_data
            result.output.stderr_data = stderr_data
            result.status.code = exit_code as int
            result.status.signaled = false
            result.status.signal = 0
            result.success = (exit_code == 0)
            pr_ok(&mut result, &mut ret)
            return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
        } else {
            var status : int = 0
            var pid = unsafe(waitpid(child._unix.pid, &raw mut status, 1)) // WNOHANG
            if(pid == 0) {
                var e = ProcessError.NotRunning()
                pr_err(e, &mut ret)
                return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
            }
            var result : ProcessResult = unsafe(std::mem.zeroed_unchecked<ProcessResult>())
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
            return std::replace<PR_Result>(&mut ret, unsafe(std::mem.zeroed_unchecked<PR_Result>()))
        }

        }
    }

// ---------------------------------------------------------------------------
// Safe public operations
// ---------------------------------------------------------------------------

/// Block until the child exits, collecting captured stdout/stderr.
public func wait(child : &mut Child) : PR_Result {
    return wait_raw(&raw mut child.inner)
}

/// Non-blocking wait; returns `NotRunning` while the child is still alive.
public func try_wait(child : &mut Child) : PR_Result {
    return try_wait_raw(&raw mut child.inner)
}

/// Signal the child. On Windows `signal` is ignored and the process is terminated.
public func kill_process(child : &mut Child, signal : int) : UT_Result {
    return kill_process_raw(&raw mut child.inner, signal)
}

/// Poll whether the child is still running.
public func is_running(child : &mut Child) : bool {
    return is_running_raw(&raw mut child.inner)
}

/// Write bytes to the child's stdin pipe.
public func write_stdin(child : &mut Child, data : [u8]) : UT_Result {
    return write_stdin_raw(&raw mut child.inner, data.data(), data.size())
}

/// Close the child's stdin pipe (sends EOF).
public func close_stdin(child : &mut Child) : UT_Result {
    return close_stdin_raw(&raw mut child.inner)
}

/// Return the child's process id.
public func child_pid(child : &mut Child) : int {
    return child_pid_raw(&raw mut child.inner)
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
func child_pid_raw(child : *mut ChildProcess) : int {
        unsafe {
        comptime if(def.windows) {
            return child.win.pid
        } else {
            return child._unix.pid
        }

        }
    }

} // end namespace process
