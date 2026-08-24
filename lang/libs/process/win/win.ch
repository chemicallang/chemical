// Windows implementation of process management.
//
// execute / spawn / wait use CreateProcessA + pipes for I/O capture.
// kill() works via TerminateProcess.

public namespace process {

using std::string;
using std::string_view;
using std::vector;

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

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

comptime const STARTF_USESTDHANDLES : DWORD = 0x00000100 as DWORD;
comptime const CREATE_NO_WINDOW : DWORD = 0x08000000 as DWORD;
comptime const WAIT_OBJECT_0 : DWORD = 0 as DWORD;
comptime const WAIT_TIMEOUT : DWORD = 258 as DWORD;
comptime const INFINITE : DWORD = 0xFFFFFFFF as DWORD;

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

/// Build a command line string from args: "cmd /c arg1 arg2 arg3"
/// The result is a null-terminated mutable buffer for CreateProcessA.
func win_build_cmdline(args : *vector<string>) : string {
    // Always use cmd /c to ensure cmd.exe builtins and PATH resolution work.
    // Quote args after -c flags so sh receives the full command string.
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

// ---------------------------------------------------------------------------
// execute — run a process and wait for completion
// ---------------------------------------------------------------------------

public func win_execute(cfg : *ProcessConfig, out : *mut ProcessResult) : bool {
    if(cfg.args.size() == 0) { return false }

    var stdout_read : HANDLE = null
    var stdout_write : HANDLE = null
    var stderr_read : HANDLE = null
    var stderr_write : HANDLE = null

    var sa : SECURITY_ATTRIBUTES = zeroed<SECURITY_ATTRIBUTES>()
    sa.nLength = sizeof(SECURITY_ATTRIBUTES) as DWORD
    sa.bInheritHandle = 1
    sa.lpSecurityDescriptor = null

    // Create stdout pipe
    if(cfg.capture_stdout) {
        if(CreatePipe(&raw mut stdout_read, &raw mut stdout_write, &raw mut sa, 0) == 0) {
            return false
        }
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
    }

    // Build command line
    var cmd = win_build_cmdline(&raw mut cfg.args)

    // Set up startup info
    var si : STARTUPINFOA = zeroed<STARTUPINFOA>()
    si.cb = sizeof(STARTUPINFOA) as DWORD
    si.dwFlags = STARTF_USESTDHANDLES
    if(cfg.capture_stdout || cfg.merge_stdout_stderr) {
        si.hStdOutput = stdout_write
    } else {
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE)
    }
    // When merging, redirect stderr to the same stdout pipe
    if(cfg.merge_stdout_stderr) {
        si.hStdError = stdout_write
    } else if(cfg.capture_stderr) {
        si.hStdError = stderr_write
    } else {
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE)
    }
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE)

    var pi : PROCESS_INFORMATION = zeroed<PROCESS_INFORMATION>()

    // Set working directory
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
        null,
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

    if(ok == 0) {
        // Process creation failed — close remaining handles
        if(cfg.capture_stdout || cfg.merge_stdout_stderr) { CloseHandle(stdout_read) }
        if(cfg.capture_stderr && !cfg.merge_stdout_stderr) { CloseHandle(stderr_read) }
        return false
    }

    // Read output
    var stdout_data = vector<u8>()
    var stderr_data = vector<u8>()

    if(cfg.capture_stdout || cfg.merge_stdout_stderr) {
        win_read_all(stdout_read, &raw mut stdout_data)
        CloseHandle(stdout_read)
    }
    if(cfg.capture_stderr && !cfg.merge_stdout_stderr) {
        win_read_all(stderr_read, &raw mut stderr_data)
        CloseHandle(stderr_read)
    }

    // Wait for process to finish
    WaitForSingleObject(pi.hProcess, INFINITE)

    // Get exit code
    var exit_code : DWORD = 0
    GetExitCodeProcess(pi.hProcess, &raw mut exit_code)

    // Close process handles
    CloseHandle(pi.hProcess)
    CloseHandle(pi.hThread)

    out.output.stdout_data = stdout_data
    out.output.stderr_data = stderr_data
    out.status.code = exit_code as int
    out.status.signaled = false
    out.status.signal = 0
    out.success = (exit_code == 0)
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
    }

    // Create stdin pipe
    if(CreatePipe(&raw mut stdin_read, &raw mut stdin_write, &raw mut sa, 0) == 0) {
        if(cfg.capture_stdout) { CloseHandle(stdout_read); CloseHandle(stdout_write) }
        if(cfg.capture_stderr) { CloseHandle(stderr_read); CloseHandle(stderr_write) }
        return false
    }

    // Build command line
    var cmd = win_build_cmdline(&raw mut cfg.args)

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
        null,
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

    // Close stdin
    if(child.win.h_stdin_write != null) {
        CloseHandle(child.win.h_stdin_write)
        child.win.h_stdin_write = null
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
