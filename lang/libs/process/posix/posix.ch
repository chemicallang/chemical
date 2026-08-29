// POSIX implementation of process management.

public namespace process {

using std::string;
using std::string_view;
using std::vector;

// Redirect a file descriptor to /dev/null so un-captured output doesn't
// leak to the parent's terminal.
func redirect_to_devnull(fd : int) {
    var dn = open("/dev/null", O_WRONLY, 0);
    if(dn >= 0) {
        dup2(dn, fd);
        close(dn);
    } else {}
}

// POSIX impl returns bool: true=success, false=error (details in errno)
//
// NOTE: argv/envp and the program path are built in the PARENT, before
// fork(). The forked child must only run async-signal-safe ops
// (close/dup2/chdir/execve) — never getenv()/malloc()/execvp — otherwise
// process::execute() deadlocks when called from a worker thread while
// another thread holds a lock (e.g. WebKitGTK/GLib).
public func posix_execute(cfg : *ProcessConfig, out : *mut ProcessResult) : bool {
    var stdout_pipe : [2]int;
    var stderr_pipe : [2]int;
    var stdin_pipe : [2]int;
    var has_stdin_pipe = false;

    if(cfg.capture_stdout) {
        if(pipe(&raw mut stdout_pipe[0]) != 0) { return false } else {}
    } else {}
    if(cfg.capture_stderr || cfg.merge_stdout_stderr) {
        if(pipe(&raw mut stderr_pipe[0]) != 0) {
            if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
            return false
        } else {}
    } else {}
    if(cfg.stdin_data.size() > 0) {
        if(pipe(&raw mut stdin_pipe[0]) != 0) {
            if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
            if(cfg.capture_stderr || cfg.merge_stdout_stderr) { close(stderr_pipe[0]); close(stderr_pipe[1]); } else {}
            return false
        } else {}
        has_stdin_pipe = true;
    } else {}

    // ---- Parent-side setup: build argv/envp and resolve the program path ----
    // This runs BEFORE fork() so the child never has to call getenv()/malloc().
    var argv = build_argv(&raw mut cfg.args);
    var resolved : [4096]char;
    var prog = lookup_program(argv.ptrs[0], &raw mut resolved[0], 4096);
    var use_envp = false;
    var envp : ArgvBuffer;
    if(cfg.env.size() > 0) {
        envp = build_envp(&raw mut cfg.env);
        use_envp = true;
    } else {}

    var pid = fork();
    if(pid == -1) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
        if(cfg.capture_stderr || cfg.merge_stdout_stderr) { close(stderr_pipe[0]); close(stderr_pipe[1]); } else {}
        if(has_stdin_pipe) { close(stdin_pipe[0]); close(stdin_pipe[1]); } else {}
        return false
    } else {}

    if(pid == 0) {
        // ---- Child: async-signal-safe operations only ----
        if(cfg.working_dir.size() > 0) {
            chdir(cfg.working_dir.data())
        } else {}
        if(cfg.capture_stdout) {
            close(stdout_pipe[0])
            dup2(stdout_pipe[1], 1)
            if(cfg.merge_stdout_stderr) {
                dup2(stdout_pipe[1], 2)
            }
            close(stdout_pipe[1])
        } else if(cfg.merge_stdout_stderr) {
            close(stderr_pipe[0])
            dup2(stderr_pipe[1], 1)
            dup2(stderr_pipe[1], 2)
            close(stderr_pipe[1])
        } else {
            redirect_to_devnull(1)
        }
        if(cfg.merge_stdout_stderr) {
            // fd2 already handled above
        } else if(cfg.capture_stderr) {
            close(stderr_pipe[0])
            dup2(stderr_pipe[1], 2)
            close(stderr_pipe[1])
        } else {
            redirect_to_devnull(2)
        }
        if(has_stdin_pipe) {
            close(stdin_pipe[1])
            dup2(stdin_pipe[0], 0)
            close(stdin_pipe[0])
        } else {}
        // execve takes pre-built argv/envp and the parent-resolved path, so the
        // child performs no PATH lookup (no getenv) itself.
        if(use_envp) {
            execve(prog, &raw argv.ptrs[0], &raw envp.ptrs[0]);
        } else {
            execve(prog, &raw argv.ptrs[0], get_environ());
        }
        _exit(1);
    } else {}

    // ---- Parent: feed stdin, then read the child's output ----
    if(has_stdin_pipe) {
        close(stdin_pipe[0]);
        write(stdin_pipe[1], cfg.stdin_data.data() as *void, cfg.stdin_data.size());
        close(stdin_pipe[1]);
    } else {}

    if(cfg.capture_stdout) { close(stdout_pipe[1]); } else {}
    if(cfg.capture_stderr && !cfg.merge_stdout_stderr) { close(stderr_pipe[1]); } else {}
    if(cfg.merge_stdout_stderr && !cfg.capture_stdout) { close(stderr_pipe[1]); } else {}

    var stdout_data = vector<u8>();
    var stderr_data = vector<u8>();

    // Read from the pipes in a non-blocking fashion so we can also poll the
    // child and enforce the timeout (otherwise a long-running child would
    // block the read forever and the timeout could never fire).
    if(cfg.capture_stdout) { set_nonblock(stdout_pipe[0]); } else {}
    if(cfg.capture_stderr && !cfg.merge_stdout_stderr) { set_nonblock(stderr_pipe[0]); } else {}
    if(cfg.merge_stdout_stderr && !cfg.capture_stdout) { set_nonblock(stderr_pipe[0]); } else {}

    var status : int = 0;
    var timed_out = false
    var elapsed : int = 0;
    var done = false;
    while(!done) {
        if(cfg.capture_stdout) { read_available(stdout_pipe[0], &raw mut stdout_data); } else {}
        if(cfg.capture_stderr && !cfg.merge_stdout_stderr) { read_available(stderr_pipe[0], &raw mut stderr_data); } else {}
        if(cfg.merge_stdout_stderr && !cfg.capture_stdout) { read_available(stderr_pipe[0], &raw mut stdout_data); } else {}

        var ret = waitpid(pid, &raw mut status, 1) // WNOHANG
        if(ret != 0) {
            done = true
        } else if(cfg.timeout_ms > 0) {
            usleep(10000) // 10ms
            elapsed += 10
            if(elapsed >= cfg.timeout_ms) {
                timed_out = true
                kill(pid, 9) // SIGKILL
                waitpid(pid, &raw mut status, 0)
                done = true
            }
        } else {
            usleep(5000)
        }
    }

    // Final drain now that the child is dead (pipe yields remaining data then EOF).
    if(cfg.capture_stdout) { read_available(stdout_pipe[0], &raw mut stdout_data); close(stdout_pipe[0]); } else {}
    if(cfg.capture_stderr && !cfg.merge_stdout_stderr) { read_available(stderr_pipe[0], &raw mut stderr_data); close(stderr_pipe[0]); } else {}
    if(cfg.merge_stdout_stderr && !cfg.capture_stdout) { read_available(stderr_pipe[0], &raw mut stdout_data); close(stderr_pipe[0]); } else {}

    var exit_code : int = 0;
    var signaled : bool = false;
    var signal_no : int = 0;

    if(WIFEXITED(status)) { exit_code = WEXITSTATUS(status); }
    else if(WIFSIGNALED(status)) { signaled = true; signal_no = WTERMSIG(status); exit_code = -1; } else {}

    out.output.stdout_data = stdout_data;
    out.output.stderr_data = stderr_data;
    out.status.code = exit_code;
    out.status.signaled = signaled;
    out.status.signal = signal_no;
    out.success = (exit_code == 0 && !signaled && !timed_out);
    return true
}

// Make a file descriptor non-blocking so reads don't stall the timeout loop.
func set_nonblock(fd : int) {
    var flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Read whatever is currently available from a non-blocking fd, stopping on
// EOF or when no more data is immediately readable (EAGAIN).
func read_available(fd : int, data : *mut vector<u8>) : bool {
    var buf : [4096]u8;
    while(true) {
        var n = read(fd, &raw mut buf[0], 4096);
        if(n > 0) {
            var i : size_t = 0;
            while(i < n as size_t) {
                data.push(buf[i]);
                i += 1;
            }
        } else if(n == 0) {
            return true
        } else {
            if(*__errno_location() == EAGAIN) { return true }
            return false
        }
    }
    return false
}

// Same parent-side setup rule as posix_execute() above (no getenv()/malloc()
// in the forked child) to avoid the multi-threaded deadlock.
public func posix_spawn(cfg : *ProcessConfig, child : *mut ChildProcess) : bool {
    var stdout_pipe : [2]int;
    var stderr_pipe : [2]int;
    var stdin_pipe : [2]int;

    if(cfg.capture_stdout) {
        if(pipe(&raw mut stdout_pipe[0]) != 0) { return false } else {}
    } else {}
    if(cfg.capture_stderr || cfg.merge_stdout_stderr) {
        if(pipe(&raw mut stderr_pipe[0]) != 0) {
            if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
            return false
        } else {}
    } else {}
    // Always create a stdin pipe so write_stdin/close_stdin work.
    if(pipe(&raw mut stdin_pipe[0]) != 0) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
        if(cfg.capture_stderr) { close(stderr_pipe[0]); close(stderr_pipe[1]); } else {}
        return false
    } else {}

    // ---- Parent-side setup (see posix_execute for the rationale) ----
    // Build argv/envp and resolve the program path BEFORE fork() so the child
    // never calls getenv()/malloc() (which would deadlock under held locks).
    var argv = build_argv(&raw mut cfg.args);
    var resolved : [4096]char;
    var prog = lookup_program(argv.ptrs[0], &raw mut resolved[0], 4096);
    var use_envp = false;
    var envp : ArgvBuffer;
    if(cfg.env.size() > 0) {
        envp = build_envp(&raw mut cfg.env);
        use_envp = true;
    } else {}

    var pid = fork();
    if(pid == -1) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
        if(cfg.capture_stderr) { close(stderr_pipe[0]); close(stderr_pipe[1]); } else {}
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        return false
    } else {}

    if(pid == 0) {
        // ---- Child: async-signal-safe operations only ----
        if(cfg.working_dir.size() > 0) {
            chdir(cfg.working_dir.data())
        } else {}
        if(cfg.capture_stdout) {
            close(stdout_pipe[0])
            if(cfg.merge_stdout_stderr) {
                dup2(stdout_pipe[1], 1)
                dup2(stdout_pipe[1], 2)
                close(stdout_pipe[1])
            } else {
                dup2(stdout_pipe[1], 1)
                close(stdout_pipe[1])
            }
        } else {
            redirect_to_devnull(1)
        }
        if(cfg.capture_stderr && !cfg.merge_stdout_stderr) {
            close(stderr_pipe[0])
            dup2(stderr_pipe[1], 2)
            close(stderr_pipe[1])
        } else if(cfg.merge_stdout_stderr && !cfg.capture_stdout) {
            close(stderr_pipe[0])
            dup2(stderr_pipe[1], 1)
            dup2(stderr_pipe[1], 2)
            close(stderr_pipe[1])
        } else if(!cfg.merge_stdout_stderr && !cfg.capture_stderr) {
            redirect_to_devnull(2)
        } else {}
        // Child reads from stdin_pipe[0]; parent writes to stdin_pipe[1].
        close(stdin_pipe[1]); dup2(stdin_pipe[0], 0); close(stdin_pipe[0]);
        if(use_envp) {
            execve(prog, &raw argv.ptrs[0], &raw envp.ptrs[0]);
        } else {
            execve(prog, &raw argv.ptrs[0], get_environ());
        }
        _exit(1);
    } else {}

    if(cfg.capture_stdout) { close(stdout_pipe[1]); } else {}
    if(cfg.capture_stderr || cfg.merge_stdout_stderr) { close(stderr_pipe[1]); } else {}
    close(stdin_pipe[0]); // parent keeps stdin_pipe[1] for writes

    child._unix.pid = pid;
    child._unix.stdout_fd = if(cfg.capture_stdout) stdout_pipe[0] else -1;
    child._unix.stderr_fd = if(cfg.capture_stderr) stderr_pipe[0] else if(cfg.merge_stdout_stderr && !cfg.capture_stdout) stderr_pipe[0] else -1;
    child._unix.stdin_fd = stdin_pipe[1];
    child.is_running = true;
    return true
}

public func posix_wait(child : *mut ChildProcess, out : *mut ProcessResult) : bool {
    var stdout_data = vector<u8>();
    var stderr_data = vector<u8>();

    if(child._unix.stdout_fd >= 0) {
        if(!read_all_fd(child._unix.stdout_fd, &raw mut stdout_data)) {
            close(child._unix.stdout_fd);
            return false
        } else {}
        close(child._unix.stdout_fd);
        child._unix.stdout_fd = -1;
    } else {}
    if(child._unix.stderr_fd >= 0) {
        if(!read_all_fd(child._unix.stderr_fd, &raw mut stderr_data)) {
            close(child._unix.stderr_fd);
            return false
        } else {}
        close(child._unix.stderr_fd);
        child._unix.stderr_fd = -1;
    } else {}
    // Close stdin pipe if still open.
    if(child._unix.stdin_fd >= 0) {
        close(child._unix.stdin_fd);
        child._unix.stdin_fd = -1;
    } else {}

    var status : int = 0;
    waitpid(child._unix.pid, &raw mut status, 0);

    var exit_code : int = 0;
    var signaled : bool = false;
    var signal_no : int = 0;

    if(WIFEXITED(status)) { exit_code = WEXITSTATUS(status); }
    else if(WIFSIGNALED(status)) { signaled = true; signal_no = WTERMSIG(status); exit_code = -1; } else {}

    child.is_running = false;
    child._unix.pid = 0;
    out.output.stdout_data = stdout_data;
    out.output.stderr_data = stderr_data;
    out.status.code = exit_code;
    out.status.signaled = signaled;
    out.status.signal = signal_no;
    out.success = (exit_code == 0 && !signaled);
    return true
}

struct ArgvBuffer {
    var ptrs : [256]*char;
    var count : size_t;
}

func build_argv(args : *vector<string>) : ArgvBuffer {
    var buf : ArgvBuffer;
    buf.count = args.size();
    var i : size_t = 0;
    while(i < args.size()) {
        buf.ptrs[i] = args.get_ptr(i).data()
        i += 1;
    }
    buf.ptrs[i] = null;
    return buf;
}

func build_envp(env : *vector<string>) : ArgvBuffer {
    var buf : ArgvBuffer;
    buf.count = env.size();
    var i : size_t = 0;
    while(i < env.size()) {
        buf.ptrs[i] = env.get_ptr(i).data()
        i += 1;
    }
    buf.ptrs[i] = null;
    return buf;
}

func read_all_fd(fd : int, data : *mut vector<u8>) : bool {
    var buf : [4096]u8;
    while(true) {
        var n = read(fd, &raw mut buf[0], 4096);
        if(n < 0) {
            if(n == -1) { continue; } else {}
            return false
        } else {}
        if(n == 0) { break; } else {}
        var i : size_t = 0;
        while(i < n as size_t) {
            data.push(buf[i]);
            i += 1;
        }
    }
    return true
}

@extern public func pipe(pipefd : *mut int) : int
@extern public func fork() : int
@extern public func dup2(oldfd : int, newfd : int) : int
@extern public func execvp(file : *char, argv : **char) : int
@extern public func execve(file : *char, argv : **char, envp : **char) : int
@extern public func waitpid(pid : int, status : *mut int, options : int) : int
@extern public func _exit(status : int)
@extern public func close(fd : int) : int
@extern public func read(fd : int, buf : *mut void, count : size_t) : isize
@extern public func write(fd : int, buf : *void, count : size_t) : isize
@extern public func getpid() : int
@extern public func usleep(usec : int) : int
@extern public func chdir(path : *char) : int
@extern public func kill(pid : int, sig : int) : int
@extern public func fcntl(fd : int, cmd : int, _ : any...) : int
@extern public func __errno_location() : *mut int
@extern public func access(path : *char, mode : int) : int

const F_GETFL = 3
const F_SETFL = 4
const O_WRONLY = 1
const O_NONBLOCK = 2048
const EAGAIN = 11
const X_OK = 1

// Resolve an executable name to a path usable by execve, searching PATH when
// the name doesn't already contain a '/'. `out` is filled with the result and
// its pointer is returned (must remain valid until exec).
func lookup_program(prog : *char, out : *mut char, out_size : size_t) : *mut char {
    var i : size_t = 0;
    while(prog[i] != 0) {
        if(prog[i] == '/') {
            var k : size_t = 0;
            while(prog[k] != 0 && k < out_size) { out[k] = prog[k]; k += 1 }
            if(k < out_size) { out[k] = 0 }
            return out
        }
        i += 1;
    }
    var path = getenv("PATH");
    if(path == null) {
        var k : size_t = 0;
        while(prog[k] != 0 && k < out_size) { out[k] = prog[k]; k += 1 }
        if(k < out_size) { out[k] = 0 }
        return out
    }
    var seg_start : size_t = 0;
    while(true) {
        var seg_end : size_t = seg_start;
        while(path[seg_end] != 0 && path[seg_end] != ':') { seg_end += 1 }
        var len : size_t = 0;
        var k = seg_start;
        while(k < seg_end && len + 1 < out_size) { out[len] = path[k]; len += 1; k += 1 }
        if(len < out_size) { out[len] = '/'; len += 1 }
        var pi : size_t = 0;
        while(prog[pi] != 0 && len < out_size) { out[len] = prog[pi]; len += 1; pi += 1 }
        if(len < out_size) { out[len] = 0 }
        if(access(out, X_OK) == 0) { return out }
        if(path[seg_end] == 0) { break }
        seg_start = seg_end + 1;
    }
    var k : size_t = 0;
    while(prog[k] != 0 && k < out_size) { out[k] = prog[k]; k += 1 }
    if(k < out_size) { out[k] = 0 }
    return out
}

const _WIFEXITED_MASK = 0x7f;
func WIFEXITED(status : int) : bool { return (status & _WIFEXITED_MASK) == 0; }
func WEXITSTATUS(status : int) : int { return (status >> 8) & 0xff; }
func WIFSIGNALED(status : int) : bool { return (status & _WIFEXITED_MASK) != 0 && ((status & 0x7f) + 1) as int >> 1 > 0; }
func WTERMSIG(status : int) : int { return status & _WIFEXITED_MASK; }
func WIFSTOPPED(status : int) : bool { return (status & 0xff) == 0x7f; }
func WSTOPSIG(status : int) : int { return (status >> 8) & 0xff; }

} // end namespace process
