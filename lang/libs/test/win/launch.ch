func launch_test(exe_path : *char, id : int, state : &mut TestFunctionState) : int {

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
        PIPE_ACCESS_DUPLEX,     // both read and write
        PIPE_TYPE_MESSAGE |  PIPE_READMODE_MESSAGE | PIPE_WAIT, // message-based (not byte stream)
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
        cmd.data(),
        null,
        null,
        false, // do not inherit handles
        0,
        null,
        null, // inherits cwd
        &si,
        &pi
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

    var buffer : char[2048];
    var bytesRead : DWORD;
    while(true) {
        if (ReadFile(hPipe, &buffer[0], sizeof(buffer)-1, &bytesRead, null)) {
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
    WaitForSingleObject(pi.hProcess, INFINITE);

    var exitCode : DWORD;
    if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        // set the exit code in state
        state.exitCode = exitCode;
        if(exitCode != 0 && !state.fn.pass_on_crash) {
            state.has_failed = true;
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;

}