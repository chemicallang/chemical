// POSIX implementation of process management.

public namespace process {

using std::string;
using std::string_view;
using std::vector;

// POSIX impl returns bool: true=success, false=error (details in errno)
public func posix_execute(cfg : *ProcessConfig, out : *mut ProcessResult) : bool {
    unsafe var stdout_pipe : [2]int;
    unsafe var stderr_pipe : [2]int;

    if(cfg.capture_stdout) {
        if(pipe(&raw mut stdout_pipe[0]) != 0) { return false } else {}
    } else {}
    if(cfg.capture_stderr || cfg.merge_stdout_stderr) {
        if(pipe(&raw mut stderr_pipe[0]) != 0) {
            if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
            return false
        } else {}
    } else {}

    var pid = fork();
    if(pid == -1) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
        if(cfg.capture_stderr || cfg.merge_stdout_stderr) { close(stderr_pipe[0]); close(stderr_pipe[1]); } else {}
        return false
    } else {}

    if(pid == 0) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); dup2(stdout_pipe[1], 1); close(stdout_pipe[1]); } else {}
        if(cfg.capture_stderr || cfg.merge_stdout_stderr) {
            close(stderr_pipe[0]);
            if(cfg.merge_stdout_stderr) { dup2(stdout_pipe[1], 2); } else { dup2(stderr_pipe[1], 2); }
            close(stderr_pipe[1]);
        } else {}
        var argv = build_argv(&raw mut cfg.args);
        execvp(argv.ptrs[0], &raw argv.ptrs[0]);
        _exit(1);
    } else {}

    if(cfg.capture_stdout) { close(stdout_pipe[1]); } else {}
    if(cfg.capture_stderr) { close(stderr_pipe[1]); } else {}
    if(cfg.merge_stdout_stderr) { close(stderr_pipe[1]); } else {}

    var stdout_data = vector<u8>();
    var stderr_data = vector<u8>();

    if(cfg.capture_stdout) {
        if(!read_all_fd(stdout_pipe[0], &raw mut stdout_data)) {
            close(stdout_pipe[0]);
            if(cfg.capture_stderr) { close(stderr_pipe[0]); } else {}
            return false
        } else {}
        close(stdout_pipe[0]);
    } else {}
    if(cfg.capture_stderr) {
        if(!read_all_fd(stderr_pipe[0], &raw mut stderr_data)) {
            close(stderr_pipe[0]);
            return false
        } else {}
        close(stderr_pipe[0]);
    } else {}

    var status : int = 0;
    waitpid(pid, &raw mut status, 0);

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
    out.success = (exit_code == 0 && !signaled);
    return true
}

public func posix_spawn(cfg : *ProcessConfig, child : *mut ChildProcess) : bool {
    unsafe var stdout_pipe : [2]int;
    unsafe var stderr_pipe : [2]int;
    unsafe var stdin_pipe : [2]int;

    if(cfg.capture_stdout) {
        if(pipe(&raw mut stdout_pipe[0]) != 0) { return false } else {}
    } else {}
    if(cfg.capture_stderr) {
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

    var pid = fork();
    if(pid == -1) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); close(stdout_pipe[1]); } else {}
        if(cfg.capture_stderr) { close(stderr_pipe[0]); close(stderr_pipe[1]); } else {}
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        return false
    } else {}

    if(pid == 0) {
        if(cfg.capture_stdout) { close(stdout_pipe[0]); dup2(stdout_pipe[1], 1); close(stdout_pipe[1]); } else {}
        if(cfg.capture_stderr) { close(stderr_pipe[0]); dup2(stderr_pipe[1], 2); close(stderr_pipe[1]); } else {}
        // Child reads from stdin_pipe[0]; parent writes to stdin_pipe[1].
        close(stdin_pipe[1]); dup2(stdin_pipe[0], 0); close(stdin_pipe[0]);
        var argv = build_argv(&raw mut cfg.args);
        execvp(argv.ptrs[0], &raw argv.ptrs[0]);
        _exit(1);
    } else {}

    if(cfg.capture_stdout) { close(stdout_pipe[1]); } else {}
    if(cfg.capture_stderr) { close(stderr_pipe[1]); } else {}
    close(stdin_pipe[0]); // parent writes to stdin_pipe[1]

    child._unix.pid = pid;
    child._unix.stdout_fd = if(cfg.capture_stdout) stdout_pipe[0] else -1;
    child._unix.stderr_fd = if(cfg.capture_stderr) stderr_pipe[0] else -1;
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
    unsafe var buf : ArgvBuffer;
    buf.count = args.size();
    var i : size_t = 0;
    while(i < args.size()) {
        buf.ptrs[i] = args.get_ptr(i).data()
        i += 1;
    }
    buf.ptrs[i] = null;
    return buf;
}

func read_all_fd(fd : int, data : *mut vector<u8>) : bool {
    unsafe var buf : [4096]u8;
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
@extern public func waitpid(pid : int, status : *mut int, options : int) : int
@extern public func _exit(status : int)
@extern public func close(fd : int) : int
@extern public func read(fd : int, buf : *mut void, count : size_t) : isize
@extern public func write(fd : int, buf : *void, count : size_t) : isize
@extern public func getpid() : int
@extern public func usleep(usec : int) : int

const _WIFEXITED_MASK = 0x7f;
func WIFEXITED(status : int) : bool { return (status & _WIFEXITED_MASK) == 0; }
func WEXITSTATUS(status : int) : int { return (status >> 8) & 0xff; }
func WIFSIGNALED(status : int) : bool { return (status & _WIFEXITED_MASK) != 0 && ((status & 0x7f) + 1) as int >> 1 > 0; }
func WTERMSIG(status : int) : int { return status & _WIFEXITED_MASK; }
func WIFSTOPPED(status : int) : bool { return (status & 0xff) == 0x7f; }
func WSTOPSIG(status : int) : int { return (status >> 8) & 0xff; }

} // end namespace process
