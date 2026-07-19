// POSIX implementation of process management.

comptime if(!def.windows) {

public namespace process {

using std::Result;
using std::string;
using std::string_view;
using std::vector;
using std::Option;

// ---------------------------------------------------------------------------
// POSIX execute (sync)
// ---------------------------------------------------------------------------

func posix_execute(cfg : ProcessConfig) : Result<ProcessResult, ProcessError> {
    // Create pipes
    var stdout_pipe : [2]int;
    var stderr_pipe : [2]int;
    var stdin_pipe : [2]int;

    if(cfg.capture_stdout) {
        var r = pipe(&raw mut stdout_pipe[0]);
        if(r != 0) { return Result.Err(ProcessError.OperationFailed("pipe failed")); }
    }
    if(cfg.capture_stderr || cfg.merge_stdout_stderr) {
        var r = pipe(&raw mut stderr_pipe[0]);
        if(r != 0) {
            if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); }
            return Result.Err(ProcessError.OperationFailed("pipe failed")); }
    }

    var pid = fork();
    if(pid == -1) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); }
        if(cfg.capture_stderr || cfg.merge_stdout_stderr) { close(stderr_pipe[0]); close(stderr_pipe[1]); }
        return Result.Err(ProcessError.OperationFailed("fork failed"));
    }

    if(pid == 0) {
        // Child process
        // Redirect stdout
        if(cfg.capture_stdout) {
            close(stdout_pipe[0]); // Close read end
            dup2(stdout_pipe[1], 1);
            close(stdout_pipe[1]);
        }
        // Redirect stderr
        if(cfg.capture_stderr || cfg.merge_stdout_stderr) {
            close(stderr_pipe[0]);
            if(cfg.merge_stdout_stderr) {
                dup2(stdout_pipe[1], 2);
            } else {
                dup2(stderr_pipe[1], 2);
            }
            close(stderr_pipe[1]);
        }
        // Redirect stdin if data provided
        if(cfg.stdin_data.size() > 0) {
            // We'd write to pipe and redirect, simplified: just close stdin
        }

        // Build argv
        var argv = build_argv(cfg.args);
        // Build envp - for now, inherit
        var r = execvp(argv.data_ptr(0), argv.data_ptr(0));
        // If we get here, execvp failed
        _exit(1);
        return Result.Err(ProcessError.OperationFailed("execvp failed"));
    }

    // Parent process
    if(cfg.capture_stdout) { close(stdout_pipe[1]); } // Close write end
    if(cfg.capture_stderr) { close(stderr_pipe[1]); }
    if(cfg.merge_stdout_stderr) { close(stderr_pipe[1]); }

    // Read output
    var stdout_data = vector<u8>();
    var stderr_data = vector<u8>();

    if(cfg.capture_stdout) {
        var r = read_all_fd(stdout_pipe[0], &raw mut stdout_data);
        if(r is Result.Err) {
            var Err(e) = r else unreachable;
            close(stdout_pipe[0]);
            if(cfg.capture_stderr) { close(stderr_pipe[0]); }
            return Result.Err(e);
        }
        close(stdout_pipe[0]);
    }
    if(cfg.capture_stderr) {
        var r = read_all_fd(stderr_pipe[0], &raw mut stderr_data);
        if(r is Result.Err) {
            var Err(e) = r else unreachable;
            close(stderr_pipe[0]);
            return Result.Err(e);
        }
        close(stderr_pipe[0]);
    }

    // Wait for child
    var status : int = 0;
    var wpid = waitpid(pid, &raw mut status, 0);

    var exit_code : int = 0;
    var signaled : bool = false;
    var signal_no : int = 0;

    if(WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if(WIFSIGNALED(status)) {
        signaled = true;
        signal_no = WTERMSIG(status);
        exit_code = -1;
    }

    var result : ProcessResult;
    result.output.stdout_data = stdout_data;
    result.output.stderr_data = stderr_data;
    result.status.code = exit_code;
    result.status.signaled = signaled;
    result.status.signal = signal_no;
    result.success = (exit_code == 0 && !signaled);

    return Result.Ok(result);
}

// ---------------------------------------------------------------------------
// POSIX spawn (async)
// ---------------------------------------------------------------------------

func posix_spawn(cfg : ProcessConfig) : Result<ChildProcess, ProcessError> {
    var stdout_pipe : [2]int;
    var stderr_pipe : [2]int;

    if(cfg.capture_stdout) {
        var r = pipe(&raw mut stdout_pipe[0]);
        if(r != 0) { return Result.Err(ProcessError.OperationFailed("pipe failed")); }
    }
    if(cfg.capture_stderr) {
        var r = pipe(&raw mut stderr_pipe[0]);
        if(r != 0) {
            if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); }
            return Result.Err(ProcessError.OperationFailed("pipe failed"));
        }
    }

    var pid = fork();
    if(pid == -1) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); }
        if(cfg.capture_stderr) { close(stderr_pipe[0]); close(stderr_pipe[1]); }
        return Result.Err(ProcessError.OperationFailed("fork failed"));
    }

    if(pid == 0) {
        // Child
        if(cfg.capture_stdout) {
            close(stdout_pipe[0]);
            dup2(stdout_pipe[1], 1);
            close(stdout_pipe[1]);
        }
        if(cfg.capture_stderr) {
            close(stderr_pipe[0]);
            dup2(stderr_pipe[1], 2);
            close(stderr_pipe[1]);
        }
        var argv = build_argv(cfg.args);
        execvp(argv.data_ptr(0), argv.data_ptr(0));
        _exit(1);
    }

    // Parent
    if(cfg.capture_stdout) { close(stdout_pipe[1]); }
    if(cfg.capture_stderr) { close(stderr_pipe[1]); }

    var child : ChildProcess;
    child.pid = pid;
    child.stdout_fd = if(cfg.capture_stdout) stdout_pipe[0] else -1;
    child.stderr_fd = if(cfg.capture_stderr) stderr_pipe[0] else -1;
    child.stdin_fd = -1;
    child.is_running = true;

    return Result.Ok(child);
}

// ---------------------------------------------------------------------------
// POSIX wait
// ---------------------------------------------------------------------------

func posix_wait(child : *mut ChildProcess) : Result<ProcessResult, ProcessError> {
    // Read any remaining output
    var stdout_data = vector<u8>();
    var stderr_data = vector<u8>();

    if(child.stdout_fd >= 0) {
        var r = read_all_fd(child.stdout_fd, &raw mut stdout_data);
        if(r is Result.Err) {
            var Err(e) = r else unreachable;
            close(child.stdout_fd);
            return Result.Err(e);
        }
        close(child.stdout_fd);
        child.stdout_fd = -1;
    }
    if(child.stderr_fd >= 0) {
        var r = read_all_fd(child.stderr_fd, &raw mut stderr_data);
        if(r is Result.Err) {
            var Err(e) = r else unreachable;
            close(child.stderr_fd);
            return Result.Err(e);
        }
        close(child.stderr_fd);
        child.stderr_fd = -1;
    }

    // Wait
    var status : int = 0;
    var wpid = waitpid(child.pid, &raw mut status, 0);

    var exit_code : int = 0;
    var signaled : bool = false;
    var signal_no : int = 0;

    if(WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if(WIFSIGNALED(status)) {
        signaled = true;
        signal_no = WTERMSIG(status);
        exit_code = -1;
    }

    child.is_running = false;

    var result : ProcessResult;
    result.output.stdout_data = stdout_data;
    result.output.stderr_data = stderr_data;
    result.status.code = exit_code;
    result.status.signaled = signaled;
    result.status.signal = signal_no;
    result.success = (exit_code == 0 && !signaled);

    return Result.Ok(result);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

struct ArgvBuffer {
    var ptrs : [256]*char;
    var count : size_t;
}

func build_argv(args : vector<string>) : ArgvBuffer {
    var buf : ArgvBuffer;
    buf.count = args.size();
    var i : size_t = 0;
    while(i < args.size()) {
        buf.ptrs[i] = args.data_ptr(i).data();
        i += 1;
    }
    buf.ptrs[i] = null;
    return buf;
}

func read_all_fd(fd : int, data : *mut vector<u8>) : Result<UnitTy, ProcessError> {
    var buf : [4096]u8;
    while(true) {
        var n = read(fd, &raw mut buf[0], 4096);
        if(n < 0) {
            if(n == -1) {
                // EINTR? retry
                continue;
            }
            return Result.Err(ProcessError.OperationFailed("read failed"));
        }
        if(n == 0) { break; } // EOF
        var i : size_t = 0;
        while(i < n as size_t) {
            data.push(buf[i]);
            i += 1;
        }
    }
    return Result.Ok(UnitTy{});
}

// ---------------------------------------------------------------------------
// POSIX externs
// ---------------------------------------------------------------------------

@extern("pipe")
func pipe(pipefd : *mut int) : int

@extern("fork")
func fork() : int

@extern("dup2")
func dup2(oldfd : int, newfd : int) : int

@extern("execvp")
func execvp(file : *char, argv : **char) : int

@extern("waitpid")
func waitpid(pid : int, status : *mut int, options : int) : int

@extern("_exit")
func _exit(status : int)

@extern("close")
func close(fd : int) : int

@extern("read")
func read(fd : int, buf : *mut void, count : size_t) : isize

@extern("write")
func write(fd : int, buf : *void, count : size_t) : isize

// Wait status macros (simplified)
const _WIFEXITED_MASK = 0x7f;
func WIFEXITED(status : int) : bool { return (status & _WIFEXITED_MASK) == 0; }
func WEXITSTATUS(status : int) : int { return (status >> 8) & 0xff; }
func WIFSIGNALED(status : int) : bool { return (status & _WIFEXITED_MASK) != 0 && ((status & 0x7f) + 1) as int >> 1 > 0; }
func WTERMSIG(status : int) : int { return status & _WIFEXITED_MASK; }
func WIFSTOPPED(status : int) : bool { return (status & 0xff) == 0x7f; }
func WSTOPSIG(status : int) : int { return (status >> 8) & 0xff; }

} // end namespace process

} // end comptime if(!def.windows)
