@dllimport
@extern
@stdcall
public func TerminateProcess(hProcess : HANDLE, uExitCode : UINT) : BOOL;

// Read whatever is still buffered on a pipe whose client has gone away.
// A message-mode pipe keeps delivering already-written messages after the
// client closes its handle; ReadFile only reports ERROR_BROKEN_PIPE once the
// buffer is empty. Returns the number of messages processed.
func drain_pipe_messages(state : &mut TestFunctionState, hPipe : HANDLE) : int {
    var drained : int = 0
    while(true) {
        var avail : DWORD = 0
        var left : DWORD = 0
        // Peek may legitimately fail now that the client is gone; if it does we
        // simply have nothing buffered and stop.
        if(!PeekNamedPipe(hPipe, null, 0, null, &raw mut avail, &raw mut left)) { return drained }
        if(avail == 0) { return drained }
        var buf : [2048]char
        var n : DWORD = 0
        if(!ReadFile(hPipe, &raw mut buf[0], sizeof(buf)-1, &raw mut n, null)) { return drained }
        if(n == 0) { return drained }
        buf[n] = '\0'
        process_message(state, &raw mut buf[0])
        drained += 1
    }
    return drained
}

func launch_test(exe_path : *char, id : int, state : &mut TestFunctionState, timeout_ms : uint) : int {

    var si : STARTUPINFOA
    var pi : PROCESS_INFORMATION;
    ZeroMemory(&raw mut si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&raw mut pi, sizeof(pi));

    var cmd = std::string()
    cmd.append_char_ptr(exe_path)
    cmd.append(' ')
    cmd.append_char_ptr("--test-id ");
    append_integer(&mut cmd, id);

    // On Windows, comm_id is only used as a sentinel to tell the child it is a child
    // (is_child = comm_id != -1). The actual pipe connection is made by test id.
    cmd.append(' ')
    cmd.append_char_ptr("--comm-id ");
    append_integer(&mut cmd, id);

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
        fprintf(get_stderr(), "CreateNamedPipeA failed for test id %d, pipe '%s'\n", id, pipeName.data());
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
        &raw mut si,
        &raw mut pi
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
    // Poll the pipe for incoming messages with PeekNamedPipe and enforce the
    // overall test timeout at the top of every iteration (mirrors the posix
    // poll() read loop). WaitForSingleObject on a message-mode pipe can report
    // signaled right after the client connects even when no message is pending,
    // which made ReadFile block forever when the child stalled (e.g. a compiler
    // invocation that hangs), so the timeout was never reached and the whole
    // test run hung. PeekNamedPipe never blocks, so the timeout always fires.
    var start_time = std::chrono::Instant::now()
    var timed_out = false
    while(true) {
        // enforce the timeout even when the child is alive but silent
        var now = std::chrono::Instant::now()
        var elapsed_ms = now.duration_since(&start_time).as_millis()
        if(elapsed_ms >= timeout_ms as i64) {
            TerminateProcess(pi.hProcess, 1)
            var l = TestLog()
            l.type = LogType.Error
            l.message.append_view("Test timed out after 10s")
            state.logs.push(l)
            state.has_failed = true
            state.exitCode = 1 // dummy exit code for timeout
            timed_out = true
            break
        }

        // check how many bytes are queued without blocking
        var total_avail : DWORD = 0
        var left_this_msg : DWORD = 0
        var peek_ok = PeekNamedPipe(hPipe, null, 0, null, &raw mut total_avail, &raw mut left_this_msg)
        if(!peek_ok) {
            var perr = GetLastError();
            if (perr == ERROR_BROKEN_PIPE) {
                // closed by the client
                break;
            }
            // transient error: sleep briefly and retry
            Sleep(50)
            continue
        }
        if(total_avail == 0) {
            Sleep(50)
            continue
        }

        if (ReadFile(hPipe, &raw mut buffer[0], sizeof(buffer)-1, &raw mut bytesRead, null)) {
            if(bytesRead > 0) {
                buffer[bytesRead] = '\0'; // null terminate
                process_message(state, &raw mut buffer[0]);
            } else {
                // 0 bytes means pipe closed gracefully
                drain_pipe_messages(state, hPipe)
                break;
            }
        } else {
            var err = GetLastError();
            if (err == ERROR_BROKEN_PIPE) {
                // Closed by the client. A child that reports a failure and exits
                // immediately can leave that final message sitting in the pipe
                // buffer while the handle already reads as broken, and breaking
                // here would discard it — the test then shows as "[N] FAIL"
                // with no explanation. Drain whatever is still buffered first.
                drain_pipe_messages(state, hPipe)
                break;
            } else if(err == ERROR_MORE_DATA) {
                fprintf(get_stderr(), "buffer too small for testing data received: %lu\n", err);
                // Our buffer was too small for the message
                buffer[sizeof(buffer) - 1] = '\0';
                process_message(state, &raw mut buffer[0]);
                break;
            } else {
                fprintf(get_stderr(), "ReadFile failed: %lu\n", err);
                break;
            }
        }
    }

    // Wait for process to finish (skipped when the read loop already timed out and killed it)
    if(!timed_out) {
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
            if (GetExitCodeProcess(pi.hProcess, &raw mut exitCode)) {
                // set the exit code in state
                state.exitCode = exitCode;
                if(exitCode != 0 && !state.fn.pass_on_crash) {
                    state.has_failed = true;
                }
            }
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hPipe);

    return 0;

}
