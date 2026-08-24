// Windows implementation of process management.
//
// execute / spawn / wait use CreateProcessA + pipes for I/O capture.
// kill() works via TerminateProcess.

public namespace process {

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

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

comptime const STARTF_USESTDHANDLES : DWORD = 0x00000100 as DWORD;
comptime const CREATE_NO_WINDOW : DWORD = 0x08000000 as DWORD;
comptime const WAIT_OBJECT_0 : DWORD = 0 as DWORD;
comptime const WAIT_TIMEOUT : DWORD = 258 as DWORD;
comptime const INFINITE : DWORD = 0xFFFFFFFF as DWORD;
comptime const HANDLE_FLAG_INHERIT : DWORD = 0x00000001 as DWORD;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Read all data from a handle until EOF.
func win_read_all(h : HANDLE, data : *mut vector<u8>) : bool {
    unsafe var buf : [4096]u8;
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

/// Build a command line string from args with "cmd /c" wrapper.
func win_build_cmdline(args : *vector<string>) : string {
    var cmd = string("cmd /c ")
    var i : size_t = 0
    var prev_was_flag = false
    while(i < args.size()) {
        if(i > 0) { cmd.append(' ' as char) }
        var arg = args.get_ptr(i).to_view()
        var needs_quote = prev_was_flag
        prev_was_flag = false
        if(arg.size() == 2 && arg.get(0) == '-' && arg.get(1) == 'c') {
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

/// Build a Windows environment block from a vector of "KEY=VALUE" strings.
/// The block is a sequence of null-terminated strings followed by an extra null.
func win_build_env_block(env : *vector<string>) : *mut void {
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

// ---------------------------------------------------------------------------
// execute — run a process and wait for completion
// ---------------------------------------------------------------------------

public func win_execute(cfg : *ProcessConfig, out : *mut ProcessResult) : bool {
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
                if(entry.get(j) == '=' as char) {
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
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE)
    }
    if(cfg.merge_stdout_stderr) {
        si.hStdError = stdout_write
    } else if(cfg.capture_stderr) {
        si.hStdError = stderr_write
    } else {
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE)
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
    var wait_ms : DWORD = INFINITE
    if(cfg.timeout_ms > 0) {
        wait_ms = cfg.timeout_ms as DWORD
    }
    var timed_out = false

    if(cfg.timeout_ms > 0) {
        // With timeout: wait first with timeout, then read output.
        // This avoids win_read_all blocking forever when the child hangs.
        var wait_rc = WaitForSingleObject(pi.hProcess, wait_ms)
        if(wait_rc == WAIT_TIMEOUT) {
            timed_out = true
            TerminateProcess(pi.hProcess, 1)
            WaitForSingleObject(pi.hProcess, INFINITE)
            // Do NOT read stdout/stderr — orphaned child processes (cmd→sh→sleep)
            // may still hold the write end open, causing win_read_all to block.
            // Close the read handles instead.
            if(cfg.capture_stdout || cfg.merge_stdout_stderr) {
                CloseHandle(stdout_read)
            }
            if(cfg.capture_stderr && !cfg.merge_stdout_stderr) {
                CloseHandle(stderr_read)
            }
        } else {
            // Process exited normally — read its output
            if(cfg.capture_stdout || cfg.merge_stdout_stderr) {
                win_read_all(stdout_read, &raw mut stdout_data)
                CloseHandle(stdout_read)
            }
            if(cfg.capture_stderr && !cfg.merge_stdout_stderr) {
                win_read_all(stderr_read, &raw mut stderr_data)
                CloseHandle(stderr_read)
            }
        }
    } else {
        // No timeout: read output, then wait
        if(cfg.capture_stdout || cfg.merge_stdout_stderr) {
            win_read_all(stdout_read, &raw mut stdout_data)
            CloseHandle(stdout_read)
        }
        if(cfg.capture_stderr && !cfg.merge_stdout_stderr) {
            win_read_all(stderr_read, &raw mut stderr_data)
            CloseHandle(stderr_read)
        }
        WaitForSingleObject(pi.hProcess, INFINITE)
    }

    // Get exit code
    var exit_code : DWORD = 0

    // Close process handles
    CloseHandle(pi.hProcess)
    CloseHandle(pi.hThread)

    out.output.stdout_data = stdout_data
    out.output.stderr_data = stderr_data
    out.status.code = exit_code as int
    out.status.signaled = false
    out.status.signal = 0
    out.success = (exit_code == 0 && !timed_out)
    return true
}

// ---------------------------------------------------------------------------
// spawn — start a process without waiting
// ---------------------------------------------------------------------------

public func win_spawn(cfg : *ProcessConfig, child : *mut ChildProcess) : bool {
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
    if(cfg.capture_stdout) {
        si.hStdOutput = stdout_write
    } else {
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE)
    }
    if(cfg.capture_stderr) {
        si.hStdError = stderr_write
    } else {
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE)
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

// ---------------------------------------------------------------------------
// wait — wait for a spawned process and collect output
// ---------------------------------------------------------------------------

public func win_wait(child : *mut ChildProcess, out : *mut ProcessResult) : bool {
    var stdout_data = vector<u8>()
    var stderr_data = vector<u8>()

    // Close stdin first — the child may be blocking on stdin (e.g. cat,
    // shell). Closing the write end sends EOF so the child can exit and
    // close its stdout/stderr, preventing a deadlock in win_read_all.
    if(child.win.h_stdin_write != null) {
        CloseHandle(child.win.h_stdin_write)
        child.win.h_stdin_write = null
    }

    // Read stdout
    if(child.win.h_stdout_read != null) {
        win_read_all(child.win.h_stdout_read, &raw mut stdout_data)
        CloseHandle(child.win.h_stdout_read)
        child.win.h_stdout_read = null
    }

    // Read stderr
    if(child.win.h_stderr_read != null) {
        win_read_all(child.win.h_stderr_read, &raw mut stderr_data)
        CloseHandle(child.win.h_stderr_read)
        child.win.h_stderr_read = null
    }

    // Wait for process
    WaitForSingleObject(child.win.h_process, INFINITE)

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
    out.status.code = exit_code as int
    out.status.signaled = false
    out.status.signal = 0
    out.success = (exit_code == 0)
    return true
}

} // end namespace process
