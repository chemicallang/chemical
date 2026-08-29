func create_test_env(fn : *mut TestFunction, config : &mut TestRunnerConfig) : TestEnvImpl {
    var pipeName = get_test_pipe_name(fn.id)
    // connect to named pipe of parent executable
    var hPipe : HANDLE;
    while(true) {
        hPipe = CreateFileA(
            pipeName.data(),
            GENERIC_WRITE | GENERIC_READ, // we want to send messages
            0,                            // no sharing
            null,                         // default security
            OPEN_EXISTING,                // must already exist
            0,
            null
        );
        if (hPipe != INVALID_HANDLE_VALUE) break;
        if (GetLastError() != ERROR_PIPE_BUSY) {
            print_last_error("CreateFileA");
            abort();
        }
        // If pipe is busy, wait a little and retry.
        WaitNamedPipeA(pipeName.data(), 2000);
    }
    return TestEnvImpl {
        fn : fn,
        pipeHandle : hPipe
    }
}