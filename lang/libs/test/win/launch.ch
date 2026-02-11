@dllimport
@extern
@stdcall
public func TerminateProcess(hProcess : HANDLE, uExitCode : UINT) : BOOL;

func launch_test(exe_path : *char, id : int, state : &mut TestFunctionState, timeout_ms : uint) : int {

    var si : STARTUPINFOA
    var pi : PROCESS_INFORMATION;
    ZeroMemory(&mut si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&mut pi, sizeof(pi));

    var cmd = std::string()
    cmd.append_char_ptr(exe_path)
    cmd.append(' ');
    cmd.append_char_ptr("--test-id ");
    append_integer(cmd, id);

    // get pipe name
    var pipeName = get_test_pipe_name(id);

    // creating a pipe for communication with test process
    const hPipe = CreateNamedPipeA(
        pipeName.data(),
        PIPE_ACCESS_DUPLEX as DWORD,     // both read and write
        (PIPE_TYPE_MESSAGE |  PIPE_READMODE_MESSAGE | PIPE_WAIT) as DWORD, // message-based (not byte stream)
        1,
        1024, // output buffer size
        1024, // input buffer size
        0, // timeout
        null // security
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        print_last_error("CreateNamedPipeA")
        return 1;
    }

    var ok = CreateProcessA(
        null,
        cmd.mutable_data(),
        null,
        null,
        false, // do not inherit handles
        0,
        null,
        null, // inherits cwd
        &mut si,
        &mut pi
    )

    if(!ok) {
         var e = GetLastError();
         fprintf(get_stderr(), "CreateProcess failed: %lu\n", e as ulong);
         CloseHandle(hPipe);
         return e as int;
    }

    // Wait for the child to connect.
    if (!ConnectNamedPipe(hPipe, null)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) {
            fprintf(get_stderr(), "error: during connect named pipe");
            CloseHandle(hPipe);
            return 1;
        }
    }

    var buffer : [2048]char;
    var bytesRead : DWORD;
    while(true) {
        if (ReadFile(hPipe, &mut buffer[0], sizeof(buffer)-1, &mut bytesRead, null)) {
            if(bytesRead > 0) {
                buffer[bytesRead] = '\0'; // null terminate
                process_message(state, &mut buffer[0]);
            } else {
                // 0 bytes means pipe closed gracefully
                break;
            }
        } else {
            var err = GetLastError();
            if (err == ERROR_BROKEN_PIPE) {
                // closed by the client
                break;
            } else if(err == ERROR_MORE_DATA) {
                fprintf(get_stderr(), "buffer too small for testing data received: %lu\n", err);
                // Our buffer was too small for the message
                buffer[sizeof(buffer) - 1] = '\0';
                process_message(state, &mut buffer[0]);
                break;
            } else {
                fprintf(get_stderr(), "ReadFile failed: %lu\n", err);
                break;
            }
        }
    }

    // Wait for process to finish
    // WAIT_TIMEOUT is 258
    var waitRes = WaitForSingleObject(pi.hProcess, timeout_ms as DWORD);

    if(waitRes == 258 as DWORD) {
        TerminateProcess(pi.hProcess, 1)
        var l = TestLog()
        l.type = LogType.Error
        l.message.append_view("Test timed out after 10s")
        state.logs.push(l)
        state.has_failed = true
        state.exitCode = 1 // dummy exit code for timeout
    } else {
        var exitCode : DWORD;
        if (GetExitCodeProcess(pi.hProcess, &mut exitCode)) {
            // set the exit code in state
            state.exitCode = exitCode;
            if(exitCode != 0 && !state.fn.pass_on_crash) {
                state.has_failed = true;
            }
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;

}
