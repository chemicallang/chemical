// Windows implementation of process management.
//
// execute / spawn / wait use CreateProcessA + pipes for I/O capture.
// kill() works via TerminateProcess.

public namespace process {

// Close handles owned by a Child. Does not terminate the process.
func process_cleanup(child : *mut ChildProcess) {
    unsafe {
        if(child.win.h_stdin_write != null) { CloseHandle(child.win.h_stdin_write); child.win.h_stdin_write = null; }
        if(child.win.h_stdout_read != null) { CloseHandle(child.win.h_stdout_read); child.win.h_stdout_read = null; }
        if(child.win.h_stderr_read != null) { CloseHandle(child.win.h_stderr_read); child.win.h_stderr_read = null; }
        if(child.win.h_process != null) { CloseHandle(child.win.h_process); child.win.h_process = null; }
        if(child.win.h_thread != null) { CloseHandle(child.win.h_thread); child.win.h_thread = null; }
        child.is_running = false;
    }
}



using std::string;
using std::string_view;
using std::vector;
using std::Option;
using environment;

// ---------------------------------------------------------------------------
// Win32 API declarations
// ---------------------------------------------------------------------------

@dllimport @extern @stdcall
public func TerminateProcess(hProcess : HANDLE, uExitCode : UINT) : BOOL;

@dllimport @extern @stdcall
public func WaitForSingleObject(hHandle : HANDLE, dwMilliseconds : DWORD) : DWORD;

@dllimport @extern @stdcall
public func GetCurrentProcessId() : DWORD;

@dllimport @extern @stdcall
public func Sleep(dwMilliseconds : DWORD) : void;

@dllimport @extern @stdcall
public func CreatePipe(
    hReadPipe : *mut HANDLE,
    hWritePipe : *mut HANDLE,
    lpPipeAttributes : *mut SECURITY_ATTRIBUTES,
    nSize : DWORD
) : BOOL;

@dllimport @extern @stdcall
public func SetHandleInformation(hObject : HANDLE, dwMask : DWORD, dwFlags : DWORD) : BOOL;

@extern
func SetEnvironmentVariableA(name : *char, value : *char) : int;

@dllimport @extern @stdcall
func CreateFileA(
    lpFileName : LPCSTR,
    dwDesiredAccess : DWORD,
    dwShareMode : DWORD,
    lpSecurityAttributes : *mut SECURITY_ATTRIBUTES,
    dwCreationDisposition : DWORD,
    dwFlagsAndAttributes : DWORD,
    hTemplateFile : HANDLE
) : HANDLE;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

comptime const STARTF_USESTDHANDLES : DWORD = 0x00000100 as DWORD;
comptime const CREATE_NO_WINDOW : DWORD = 0x08000000 as DWORD;
comptime const WAIT_OBJECT_0 : DWORD = 0 as DWORD;
comptime const WAIT_TIMEOUT : DWORD = 258 as DWORD;
comptime const INFINITE : DWORD = 0xFFFFFFFF as DWORD;
comptime const HANDLE_FLAG_INHERIT : DWORD = 0x00000001 as DWORD;
comptime const FILE_WRITE_DATA : DWORD = 0x00000002 as DWORD;
comptime const OPEN_EXISTING : DWORD = 3 as DWORD;
comptime const INVALID_HANDLE_VALUE : HANDLE = (0xFFFFFFFF as usize) as HANDLE;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Open NUL: (Windows /dev/null equivalent) so un-captured output doesn't
/// leak to the parent's console.
func win_open_nul() : HANDLE {
    unsafe {
    var h = CreateFileA("NUL", FILE_WRITE_DATA, 0, null, OPEN_EXISTING, 0, null)
    if(h == INVALID_HANDLE_VALUE) { return GetStdHandle(STD_OUTPUT_HANDLE) }
    return h
    }
}

/// Read all data from a handle until EOF.
func win_read_all(h : HANDLE, data : *mut vector<u8>) : bool {
    unsafe {
    var buf : [4096]u8;
    while(true) {
        var bytes_read : DWORD = 0
        var ok = ReadFile(h, &raw mut buf[0], 4096, &raw mut bytes_read, null)
        if(ok == 0) { break }
        if(bytes_read == 0) { break }
        var i : DWORD = 0
        while(i < bytes_read) {
            data.push(buf[i as size_t] as u8)
            i += 1
        }
    }
    return true
    }
}

/// Append whatever is currently buffered on a pipe without blocking, appending
/// it to `data`. Sets `open` to false once the write end is gone (the child
/// exited), which is how the wait loop learns the stream reached EOF.
func win_drain_available(h : HANDLE, data : *mut vector<u8>, open : *mut bool) {
    unsafe {
    var buf : [4096]u8
    while(true) {
        var avail : DWORD = 0
        if(PeekNamedPipe(h, null, 0, null, &raw mut avail, null) == 0) {
            // ERROR_BROKEN_PIPE (109): the child closed the write end.
            open[0] = false
            return
        }
        if(avail == 0) { return }
        var to_read : DWORD = avail
        if(to_read > 4096 as DWORD) { to_read = 4096 as DWORD }
        var bytes_read : DWORD = 0
        if(ReadFile(h, &raw mut buf[0], to_read, &raw mut bytes_read, null) == 0) {
            open[0] = false
            return
        }
        if(bytes_read == 0) { return }
        var i : DWORD = 0
        while(i < bytes_read) {
            data.push(buf[i as size_t] as u8)
            i += 1
        }
    }
    }
}

/// Build a command line string from args with "cmd /c" wrapper.
func win_build_cmdline(args : *vector<string>) : string {
    unsafe {
    var cmd = string("cmd /c ")
    var i : size_t = 0
    var prev_was_flag = false
    while(i < args.size()) {
        if(i > 0) { cmd.append(' ' as char) }
        var arg = args.get_ptr(i).to_view()
        var needs_quote = prev_was_flag
        prev_was_flag = false
        if(arg.size() == 2 && arg.at(0) == '-' && arg.at(1) == 'c') {
            prev_was_flag = true
        }
        if(needs_quote) {
            cmd.append(34 as char)
            cmd.append_view(&arg)
            cmd.append(34 as char)
        } else {
            cmd.append_view(&arg)
        }
        i += 1
    }
    return cmd
    }
}

/// Build a Windows environment block from a vector of "KEY=VALUE" strings.
/// The block is a sequence of null-terminated strings followed by an extra null.
func win_build_env_block(env : *vector<string>) : *mut void {
    unsafe {
    if(env.size() == 0) { return null }
    // Calculate total size
    var total : size_t = 0
    var i : size_t = 0
    while(i < env.size()) {
        total += env.get_ptr(i).size() + 1 // +1 for null terminator
        i += 1
    }
    total += 1 // final double-null
    // Allocate and fill
    var block_raw = malloc(total)
    var block = block_raw as *mut u8
    var pos : size_t = 0
    i = 0
    while(i < env.size()) {
        var entry = env.get_ptr(i).data()
        var entry_len = env.get_ptr(i).size()
        var j : size_t = 0
        while(j < entry_len) {
            block[pos + j] = entry[j] as u8
            j += 1
        }
        pos += entry_len
        block[pos] = 0 // null terminator
        pos += 1
        i += 1
    }
    block[pos] = 0 // final double-null
    return block_raw
    }
}

// ---------------------------------------------------------------------------
// execute — run a process and wait for completion
// ---------------------------------------------------------------------------

func win_execute(cfg : *ProcessConfig, out : *mut ProcessResult) : bool {
    unsafe {
    if(cfg.args.size() == 0) { return false }

    var stdout_read : HANDLE = null
    var stdout_write : HANDLE = null
    var stderr_read : HANDLE = null
    var stderr_write : HANDLE = null
    var stdin_read : HANDLE = null
    var stdin_write : HANDLE = null

    var sa : SECURITY_ATTRIBUTES = zeroed<SECURITY_ATTRIBUTES>()
    sa.nLength = sizeof(SECURITY_ATTRIBUTES) as DWORD
    sa.bInheritHandle = 1
    sa.lpSecurityDescriptor = null

    // Create stdout pipe
    if(cfg.capture_stdout) {
        if(CreatePipe(&raw mut stdout_read, &raw mut stdout_write, &raw mut sa, 0) == 0) {
            return false
        }
        SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0)
    }

    // Create stderr pipe (not needed when merging into stdout)
    if(cfg.capture_stderr && !cfg.merge_stdout_stderr) {
        if(CreatePipe(&raw mut stderr_read, &raw mut stderr_write, &raw mut sa, 0) == 0) {
            if(cfg.capture_stdout) {
                CloseHandle(stdout_read)
                CloseHandle(stdout_write)
            }
            return false
        }
        SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0)
    }

    // Create stdin pipe if stdin_data is provided
    var has_stdin_data = cfg.stdin_data.size() > 0
    if(has_stdin_data) {
        if(CreatePipe(&raw mut stdin_read, &raw mut stdin_write, &raw mut sa, 0) == 0) {
            if(cfg.capture_stdout || cfg.merge_stdout_stderr) { CloseHandle(stdout_read); CloseHandle(stdout_write) }
            if(cfg.capture_stderr && !cfg.merge_stdout_stderr) { CloseHandle(stderr_read); CloseHandle(stderr_write) }
            return false
        }
        SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0)
    }

    // Build command line
    var cmd = win_build_cmdline(&raw mut cfg.args)

    // Set env vars via SetEnvironmentVariableA so they're inherited
    // (lpEnvironment replaces the ENTIRE env, which would lose PATH/SystemRoot).
    var env_count = cfg.env.size()
    if(env_count > 0) {
        // Split each KEY=VALUE and call SetEnvironmentVariableA
        var i : size_t = 0
        while(i < env_count) {
            var entry = cfg.env.get_ptr(i).to_view()
            var eq_pos : size_t = 0
            var found_eq = false
            var j : size_t = 0
            while(j < entry.size()) {
                if(entry.at(j) == '=' as char) {
                    eq_pos = j
                    found_eq = true
                    break
                }
                j += 1
            }
            if(found_eq) {
                // Create null-terminated key and value from the entry
                var key_str = string(entry.subview(0, eq_pos))
                var val_str = string(entry.subview(eq_pos + 1, entry.size()))
                SetEnvironmentVariableA(key_str.data(), val_str.data())
            }
            i += 1
        }
    }

    // Set up startup info
    var si : STARTUPINFOA = zeroed<STARTUPINFOA>()
    si.cb = sizeof(STARTUPINFOA) as DWORD
    si.dwFlags = STARTF_USESTDHANDLES
    if(cfg.capture_stdout || cfg.merge_stdout_stderr) {
        si.hStdOutput = stdout_write
    } else {
        si.hStdOutput = win_open_nul()
    }
    if(cfg.merge_stdout_stderr) {
        si.hStdError = stdout_write
    } else if(cfg.capture_stderr) {
        si.hStdError = stderr_write
    } else {
        si.hStdError = win_open_nul()
    }
    if(has_stdin_data) {
        si.hStdInput = stdin_read
    } else {
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE)
    }

    var pi : PROCESS_INFORMATION = zeroed<PROCESS_INFORMATION>()

    var work_dir : LPCSTR = null
    if(cfg.working_dir.size() > 0) {
        work_dir = cfg.working_dir.data()
    }

    var ok = CreateProcessA(
        null,
        cmd.data() as LPSTR,
        null,
        null,
        1, // bInheritHandles = TRUE
        CREATE_NO_WINDOW,
        null, // inherit parent env (with our overrides)
        work_dir,
        &raw mut si,
        &raw mut pi
    )

    // Close write ends in parent
    if(cfg.capture_stdout || cfg.merge_stdout_stderr) {
        CloseHandle(stdout_write)
    }
    if(cfg.capture_stderr && !cfg.merge_stdout_stderr) {
        CloseHandle(stderr_write)
    }
    if(has_stdin_data) {
        CloseHandle(stdin_read)
    }

    if(ok == 0) {
        if(cfg.capture_stdout || cfg.merge_stdout_stderr) { CloseHandle(stdout_read) }
        if(cfg.capture_stderr && !cfg.merge_stdout_stderr) { CloseHandle(stderr_read) }
        if(has_stdin_data) { CloseHandle(stdin_write) }
        return false
    }

    // Write stdin_data and close
    if(has_stdin_data) {
        if(cfg.stdin_data.size() > 0) {
            var written : DWORD = 0
            WriteFile(stdin_write, cfg.stdin_data.data() as *void, cfg.stdin_data.size() as DWORD, &raw mut written, null)
        }
        CloseHandle(stdin_write)
    }

    var stdout_data = vector<u8>()
    var stderr_data = vector<u8>()
    var timed_out = false

    // Drain both pipes *while* the child runs. Waiting for the process first
    // deadlocks as soon as it writes more than one pipe buffer — the compiler
    // emits tens of kilobytes of warnings, fills the pipe, blocks on write and
    // never exits, so a compile invoked through `Command::output()` would sit
    // until the timeout fired and report the terminated exit code.
    var stdout_open = cfg.capture_stdout || cfg.merge_stdout_stderr
    var stderr_open = cfg.capture_stderr && !cfg.merge_stdout_stderr
    var start = std::chrono::Instant::now()
    while(true) {
        var exited = WaitForSingleObject(pi.hProcess, 0) == 0 as DWORD // WAIT_OBJECT_0
        if(stdout_open) {
            win_drain_available(stdout_read, &raw mut stdout_data, &raw mut stdout_open)
        }
        if(stderr_open) {
            win_drain_available(stderr_read, &raw mut stderr_data, &raw mut stderr_open)
        }
        // Both streams are drained to EOF and the child is gone: done. A child
        // that exits without closing its pipes (an orphan grandchild holding the
        // write end) is bounded by the timeout below instead.
        if(exited && !stdout_open && !stderr_open) {
            break
        }
        if(cfg.timeout_ms > 0) {
            var elapsed_ms = start.elapsed().as_millis()
            if(elapsed_ms >= cfg.timeout_ms as i64) {
                timed_out = true
                TerminateProcess(pi.hProcess, 1)
                WaitForSingleObject(pi.hProcess, INFINITE)
                break
            }
        } else if(exited) {
            // no timeout: the streams are closed when the child (and every
            // process that inherited the write end) exited
            if(!stdout_open && !stderr_open) { break }
        }
        Sleep(1)
    }
    if(stdout_open) { CloseHandle(stdout_read); stdout_open = false }
    if(stderr_open) { CloseHandle(stderr_read); stderr_open = false }

    // Get exit code
    var exit_code : DWORD = 0
    GetExitCodeProcess(pi.hProcess, &raw mut exit_code)

    // Close process handles
    CloseHandle(pi.hProcess)
    CloseHandle(pi.hThread)

    out.output.stdout_data = stdout_data
    out.output.stderr_data = stderr_data
    out.status._code = exit_code as int
    out.status._signaled = false
    out.status._signal = 0
    out.success = (exit_code == 0 && !timed_out)
    return true
    }
}

// ---------------------------------------------------------------------------
// spawn — start a process without waiting
// ---------------------------------------------------------------------------

func win_spawn(cfg : *ProcessConfig, child : *mut ChildProcess) : bool {
    unsafe {
    if(cfg.args.size() == 0) { return false }

    var stdout_read : HANDLE = null
    var stdout_write : HANDLE = null
    var stderr_read : HANDLE = null
    var stderr_write : HANDLE = null
    var stdin_read : HANDLE = null
    var stdin_write : HANDLE = null

    var sa : SECURITY_ATTRIBUTES = zeroed<SECURITY_ATTRIBUTES>()
    sa.nLength = sizeof(SECURITY_ATTRIBUTES) as DWORD
    sa.bInheritHandle = 1
    sa.lpSecurityDescriptor = null

    // Create stdout pipe
    if(cfg.capture_stdout) {
        if(CreatePipe(&raw mut stdout_read, &raw mut stdout_write, &raw mut sa, 0) == 0) {
            return false
        }
        // Prevent the parent-side read handle from being inherited by the child.
        // Without this, CreateProcessA duplicates ALL inheritable handles, so the
        // parent's copy of stdout_read would keep the pipe alive even after the
        // real read end is closed — blocking win_read_all forever.
        SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0)
    }

    // Create stderr pipe
    if(cfg.capture_stderr) {
        if(CreatePipe(&raw mut stderr_read, &raw mut stderr_write, &raw mut sa, 0) == 0) {
            if(cfg.capture_stdout) {
                CloseHandle(stdout_read)
                CloseHandle(stdout_write)
            }
            return false
        }
        SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0)
    }

    // Create stdin pipe
    if(CreatePipe(&raw mut stdin_read, &raw mut stdin_write, &raw mut sa, 0) == 0) {
        if(cfg.capture_stdout) { CloseHandle(stdout_read); CloseHandle(stdout_write) }
        if(cfg.capture_stderr) { CloseHandle(stderr_read); CloseHandle(stderr_write) }
        return false
    }
    // Prevent the parent-side write handle from being inherited by the child.
    // Without this, closing stdin_write in the parent doesn't send EOF because
    // the child still has its own inherited copy of the write handle.
    SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0)

    // Build command line
    var cmd = win_build_cmdline(&raw mut cfg.args)

    // Build environment block if env vars provided
    var env_block = win_build_env_block(&raw mut cfg.env)

    // Set up startup info
    var si : STARTUPINFOA = zeroed<STARTUPINFOA>()
    si.cb = sizeof(STARTUPINFOA) as DWORD
    si.dwFlags = STARTF_USESTDHANDLES
    if(cfg.capture_stdout || cfg.merge_stdout_stderr) {
        si.hStdOutput = stdout_write
    } else {
        si.hStdOutput = win_open_nul()
    }
    if(cfg.merge_stdout_stderr) {
        si.hStdError = stdout_write
    } else if(cfg.capture_stderr) {
        si.hStdError = stderr_write
    } else {
        si.hStdError = win_open_nul()
    }
    si.hStdInput = stdin_read

    var pi : PROCESS_INFORMATION = zeroed<PROCESS_INFORMATION>()

    var work_dir : LPCSTR = null
    if(cfg.working_dir.size() > 0) {
        work_dir = cfg.working_dir.data()
    }

    var ok = CreateProcessA(
        null,
        cmd.data() as LPSTR,
        null,
        null,
        1,
        CREATE_NO_WINDOW,
        env_block,
        work_dir,
        &raw mut si,
        &raw mut pi
    )

    // Close parent-side pipe ends
    if(cfg.capture_stdout) { CloseHandle(stdout_write) }
    if(cfg.capture_stderr) { CloseHandle(stderr_write) }
    CloseHandle(stdin_read)

    if(ok == 0) {
        if(cfg.capture_stdout) { CloseHandle(stdout_read) }
        if(cfg.capture_stderr) { CloseHandle(stderr_read) }
        CloseHandle(stdin_write)
        if(env_block != null) { unsafe { dealloc env_block } }
        return false
    }

    child.win.h_process = pi.hProcess
    child.win.h_thread = pi.hThread
    child.win.h_stdout_read = if(cfg.capture_stdout) stdout_read else null
    child.win.h_stderr_read = if(cfg.capture_stderr) stderr_read else null
    child.win.h_stdin_write = stdin_write
    child.win.pid = pi.dwProcessId as int
    child.is_running = true
    return true
    }
}

// ---------------------------------------------------------------------------
// wait — wait for a spawned process and collect output
// ---------------------------------------------------------------------------

func win_wait(child : *mut ChildProcess, out : *mut ProcessResult) : bool {
    unsafe {
    var stdout_data = vector<u8>()
    var stderr_data = vector<u8>()

    // Close stdin first — the child may be blocking on stdin (e.g. cat,
    // shell). Closing the write end sends EOF so the child can exit and
    // close its stdout/stderr, preventing a deadlock in win_read_all.
    if(child.win.h_stdin_write != null) {
        CloseHandle(child.win.h_stdin_write)
        child.win.h_stdin_write = null
    }

    // Check if the process has already exited (e.g. after kill()).
    // If so, close pipe handles to avoid blocking on orphaned children.
    var already_exited = (WaitForSingleObject(child.win.h_process, 0) == WAIT_OBJECT_0)

    if(already_exited) {
        // Process is dead — close pipe read handles so reads return immediately
        if(child.win.h_stdout_read != null) {
            CloseHandle(child.win.h_stdout_read)
            child.win.h_stdout_read = null
        }
        if(child.win.h_stderr_read != null) {
            CloseHandle(child.win.h_stderr_read)
            child.win.h_stderr_read = null
        }
    } else {
        // Normal case: read stdout, then stderr, then wait
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
        WaitForSingleObject(child.win.h_process, INFINITE)
    }

    // Get exit code
    var exit_code : DWORD = 0
    GetExitCodeProcess(child.win.h_process, &raw mut exit_code)

    CloseHandle(child.win.h_process)
    CloseHandle(child.win.h_thread)
    child.win.h_process = null
    child.win.h_thread = null
    child.is_running = false

    out.output.stdout_data = stdout_data
    out.output.stderr_data = stderr_data
    out.status._code = exit_code as int
    out.status._signaled = false
    out.status._signal = 0
    out.success = (exit_code == 0)
    return true
    }
}

} // end namespace process
