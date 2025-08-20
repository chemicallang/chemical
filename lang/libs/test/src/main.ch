// TODO: take it out
const INFINITE = 0xffffff

enum LogType {
    Information,
    Warning,
    Success,
    Error,
    Panic,
    UnknownFailure,
    ConfigFailure,
    IOFailure,
    NetworkFailure,
    MemoryFailure,
    RuntimeFailure,
    OutOfBoundFailure,
    ResourceFailure,
    TodoFailure,
    SecurityFailure,
    WTFFailure
}

/**
 * this interface is exposed to client testing environment
 * tests use this interface to send messages to the parent executable
 */
@static
public interface TestEnv {

    /**
     * get the current test id
     * less than zero when not known due to a failure
     */
    func get_test_id(&self) : int

    /**
     * get the group name this test belongs to
     * empty when belongs to no group
     */
    func get_group_name(&self) : std::string_view;

    /**
     * this function logs it
     */
    func logIt(&self, type : LogType, msg : *char, lineNum : uint, charNum : uint)

    /**
     * quits the current group of tests
     */
    func quit_current_group(&self, reason : *char);

    /**
     * quit all the tests running
     */
    func quit_all_tests(&self, reason : *char)

}

public func (env : &mut TestEnv) info(msg : *char) {
    env.logIt(LogType.Information, msg, 0, 0)
}

public func (env : &mut TestEnv) warning(msg : *char) {
    env.logIt(LogType.Warning, msg, 0, 0)
}

public func (env : &mut TestEnv) warn(msg : *char) {
    env.logIt(LogType.Warning, msg, 0, 0)
}

public func (env : &mut TestEnv) success(msg : *char) {
    env.logIt(LogType.Success, msg, 0, 0)
}

public func (env : &mut TestEnv) error(msg : *char) {
    env.logIt(LogType.Error, msg, 0, 0)
}

public func (env : &mut TestEnv) panic(msg : *char) {
    env.logIt(LogType.Panic, msg, 0, 0)
    // TODO destruct everything
}

struct TestFunction {
    var id : int
    var name : std::string_view
    var group : std::string_view
    var ptr : (env : &mut TestEnv) => void
    var file : std::string_view
    var timeout : uint
    var retry : uint
    var pass_on_crash : bool
    var benchmark : bool
    var lineNum : uint
    var charNum : uint
}

struct TestEnvImpl : TestEnv {

    var fn : *mut TestFunction

    var pipeHandle : HANDLE

    @override
    func get_test_id(&self) : int {
        return fn.id;
    }

    @override
    func get_group_name(&self) : std::string_view {
        return fn.group
    }

    func send_message(&self, msg : *char, len : DWORD) {
        var written : DWORD;
        if (!WriteFile(pipeHandle, msg, len, &written, null)) {
            print_last_error("WriteFile");
        }
    }

    @override
    func logIt(&self, type : LogType, msgData : *char, lineNum : uint, charNum : uint) {
        var msg = std::string();
        msg.append_char_ptr("$log,")
        var buff : [2048]char
        snprintf(&mut buff[0], sizeof(buff), "%d,%d,%d,%s", type as int, lineNum, charNum, msgData);
        msg.append_char_ptr(&buff[0])
        send_message(msg.data(), msg.size() as DWORD)
    }

    @override
    func quit_current_group(&self, reason : *char) {
        var msg = std::string();
        msg.append_char_ptr("$quit_group:");
        msg.append_char_ptr(reason)
        send_message(msg.data(), msg.size() as DWORD)
    }

    @override
    func quit_all_tests(&self, reason : *char) {
        var msg = std::string();
        msg.append_char_ptr("$quit_all:");
        msg.append_char_ptr(reason)
        send_message(msg.data(), msg.size() as DWORD)
    }

}

comptime func get_tests() : []TestFunction {
    return intrinsics::get_tests<TestFunction>() as []TestFunction
}

struct TestRunnerConfig {
    /**
     * should benchmark all the tests
     */
    var benchmark : bool = false
    /**
     * when -1, means user didn't give --test-id command line argument
     * this contains the parsed id
     */
    var single_test_id : int = -1
    /**
     * limit total processes to it
     */
    var process_limit : int = 6;
    /**
     * when user asks to launch only specific groups
     */
    var groups_to_launch : std::vector<*char>
    /**
     * the testing function that should be executed before each test
     */
    var before_each : (env : &mut TestEnv) => void = null
    /**
     * the testing function that should be executed after each test
     */
    var after_each : (env : &mut TestEnv) => void = null
}

func run_single_test(tfn : *mut TestFunction, config : &mut TestRunnerConfig) {

    var env = create_test_env(tfn);

    if(config.before_each) {
        config.before_each(env)
    }

    if(config.benchmark) {
        // TODO: benchmarking code here
        tfn.ptr(env);
    } else {
        tfn.ptr(env);
    }

    if(config.after_each) {
        config.after_each(env)
    }

}

struct TestLog {

    var type : LogType = LogType.Success

    var message : std::string

    var line : ubigint = 0

    var character : ubigint = 0

}

struct TestFunctionState {

    var fn : *mut TestFunction

    var exitCode : DWORD = 0

    var has_error_log : bool = false

    var has_failed : bool = false

    var logs : std::vector<TestLog>

    @make
    func make(t_fn : *mut TestFunction) {
        fn = t_fn
    }

}

enum MessageCommandType {
    None,
    Log,
    QuitGroup,
    QuitAll
}

func to_msg_cmd_type(str : *char) : MessageCommandType {
    if (!str) return MessageCommandType.None; // guard nullptr
    switch (fnv1_hash(str)) {
        comptime_fnv1_hash("log") => { return MessageCommandType.Log; }
        comptime_fnv1_hash("quit_group") => { return MessageCommandType.QuitGroup; }
        comptime_fnv1_hash("quit_all") => { return MessageCommandType.QuitAll; }
        default => { return MessageCommandType.None; }
    }
}

func read_msg_type(msg_ptr : **char) : MessageCommandType {
    if (!msg_ptr) return MessageCommandType.None;      // guard **null
    var msg = *msg_ptr;
    if (!msg)      return MessageCommandType.None;     // guard *null

    const CMD_MAX = 120;
    var command_buffer : char[CMD_MAX];
    var out = &command_buffer[0];
    var written : int = 0;

    while (true) {
        var c = *msg;
        if (c == '\0' || c == ',') {
            if (written < CMD_MAX) {
                *out = '\0';
            } else {
                // ensure termination even if we filled the buffer completely
                command_buffer[CMD_MAX-1] = '\0';
            }
            *msg_ptr = msg; // leave at delimiter or '\0'
            break;
        } else {
            if (written < CMD_MAX - 1) { // keep room for '\0'
                *out = c;
                out++;
                written++;
            }
            // Always advance source, even if we stopped writing, to not overflow.
            msg++;
        }
    }

    return to_msg_cmd_type(&command_buffer[0]);
}

/*
return codes:
  0 = success (out filled)
 -1 = bad arguments (null pointers)
  1 = no digits found (nothing converted)
  2 = trailing non-space characters after number
  3 = out of range for int (overflow/underflow)
*/
func parse_int_w_end(s : *char, out : *mut int, endPtrPtr : **mut char) : int {
    if (!s || !out || !endPtrPtr) return -1;  // also guard endPtrPtr
    set_errno(0);

    // Call strtol with a local endptr, then publish it back to *endPtrPtr
    var local_end : *mut char = null;
    var val : long = strtol(s, &mut local_end, 10);

    if (local_end == s) { // no conversion
        *endPtrPtr = s;   // keep caller’s pointer unchanged logically
        return 1;
    }

    // Allow trailing whitespace only
    var scan = local_end;
    while (*scan != '\0' && isspace(*scan as int)) {
        scan++;
    }
    if (*scan != '\0') {
        *endPtrPtr = local_end;
        return 2;
    }

    // Range check
    if (get_errno() == ERANGE || val > INT_MAX || val < INT_MIN) {
        *endPtrPtr = local_end;
        return 3;
    }

    *out = val as int;
    *endPtrPtr = local_end;
    return 0;
}

func parse_int_from_str(pstr : **mut char) : int {
    if (!pstr || !*pstr) {
        fprintf(get_stderr(), "parse_int_from_str: null pointer passed\n");
        return 0;
    }

    const s = *pstr;
    const endptr = null;

    set_errno(0);
    const val = strtol(s, &endptr, 10);

    if (endptr == s) {
        // No digits were found
        fprintf(get_stderr(), "parse_int_from_str: no digits at '%s'\n", s);
        *pstr = endptr; // leave pointer unchanged
        return 0;
    }

    if (get_errno() == ERANGE || val > INT_MAX || val < INT_MIN) {
        fprintf(get_stderr(), "parse_int_from_str: value out of range at '%s'\n", s);
        *pstr = endptr;
        return 0;
    }

    // Move caller's pointer to endptr (either \0 or first non-digit)
    *pstr = endptr;

    return val as int;
}

func read_char(msg_ptr : **char, c : char) : bool {
    if (!msg_ptr) return false;
    var msg = *msg_ptr;
    if (!msg) return false;

    if (*msg == c) {
        msg++;
        *msg_ptr = msg;
        return true;
    } else {
        return false;
    }
}

func read_str(msg_ptr : **mut char, into : &mut std::string) {
    if (!msg_ptr) return;
    var msg = *msg_ptr;
    if (!msg) return;

    // Append until NUL; caller is responsible for delimitation (e.g., already consumed the comma).
    while (*msg != '\0') {
        into.append(*msg); // use a single-char append; adjust to your std::string API
        msg++;
    }
    *msg_ptr = msg;
}

func parseLog(msg_ptr : **char, log : &mut TestLog) {
    if (!msg_ptr || !log) return;

    if (!read_char(msg_ptr, ',')) { return; }
    log.type = parse_int_from_str(msg_ptr) as LogType;

    if (!read_char(msg_ptr, ',')) { return; }
    log.line = parse_int_from_str(msg_ptr) as ubigint;

    if (!read_char(msg_ptr, ',')) { return; }
    log.character = parse_int_from_str(msg_ptr) as ubigint;

    if (!read_char(msg_ptr, ',')) { return; }

    read_str(msg_ptr, log.message);
}

/*
Message format examples:
$log,0,10,20,this is a normal log message
$log,1,10,20,this is an error log message
$log,2,10,20,this is a warning log message
$log,3,10,20,this is a info log message
$log,4,10,20,this is a success log message
$quit_group
$quit_all
*/
func process_message(state : &mut TestFunctionState, msg : *char) {
    if (!state) return;
    if (!msg)   return;

    // Must start with '$' to be one of ours
    if (!read_char(&mut msg, '$')) {
        return;
    }

    var msgType = read_msg_type(&mut msg);

    switch (msgType) {
        MessageCommandType.None, default => {
            return;
        }
        MessageCommandType.Log => {
            var log = TestLog();
            parseLog(&mut msg, log);
            if(log.type !in LogType.Information, LogType.Warning, LogType.Success) {
                state.has_failed = true;
                state.has_error_log = true;
            }
            state.logs.push(log);
        }
        MessageCommandType.QuitGroup => {
            // TODO: quit current group
        }
        MessageCommandType.QuitAll => {
            // TODO: quit all
        }
    }
}

struct TestRunnerState {

    var tests : std::vector<TestFunctionState>

}

func append_integer(str : &mut std::string, dig : int) {
    var buffer : char[160]
    const buffStart = &mut buffer[0]
    sprintf(buffStart, "%d", dig)
    str.append_char_ptr(buffStart);
}

func get_test_pipe_name(id : int) : std::string {
    var pipeName = std::string()
    pipeName.append_char_ptr("\\\\.\\pipe\\")
    pipeName.append_char_ptr("chem_test_pipe-");
    append_integer(pipeName, id);
    return pipeName;
}

func create_test_env(fn : *mut TestFunction) : TestEnvImpl {
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

func print_last_error(what : *char) {
    const err = GetLastError();
    var msg : LPVOID;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        null, err, 0, (&mut msg) as LPSTR, 0, null
    );
    fprintf(get_stderr(), "%s failed with error %lu: %s\n", what, err, msg as *char);
    LocalFree(msg);
}

func launch_test(exe_path : *char, id : int, state : &mut TestFunctionState) : int {

    if(def.windows) {

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

}

type TestFunctionPtr = (env : &mut TestEnv) => void

func run_tests(exe_path : *char, config : &mut TestRunnerConfig) {

    var tests = get_tests();
    var tests_view = std::span<TestFunction>(tests)

    if(config.single_test_id != -1) {

        // initialize before each and after each
        config.before_each = intrinsics::get_single_marked_decl_ptr("test.before_each") as TestFunctionPtr;
        config.after_each = intrinsics::get_single_marked_decl_ptr("test.after_each") as TestFunctionPtr;

        var test_start = &tests[0]
        const test_end = test_start + tests_view.size()

        while(test_start != test_end) {
            if(test_start.id == config.single_test_id) {
                // executing the single test
                run_single_test(test_start, config)
                // returning
                return;
            }
            test_start++;
        }

        // TODO: test not found error, should be reported to parent

    } else if(!config.groups_to_launch.empty()) {

        // TODO: launch only these group of tests

    } else {

        // create test runner state
        var state = TestRunnerState()
        state.tests.reserve(tests_view.size());

        var test_start = &mut tests[0]
        const test_end = test_start + tests_view.size()

        while(test_start != test_end) {
            // creating state for the test function
            var ind = state.tests.size()
            state.tests.push(TestFunctionState(test_start));
            const fn_state = state.tests.get_ptr(ind)

            launch_test(exe_path, test_start.id, *fn_state)

            var tries : int = test_start.retry as int
            if(tries > 1) {
                tries--
                while(tries > 1 && fn_state.has_failed) {
                    fn_state.logs.clear()
                    launch_test(exe_path, test_start.id, *fn_state)
                    tries--
                }
            }

            test_start++;
        }

        // print the test results
        print_test_results(state.tests.data(), state.tests.size())

    }

}

/* return codes:
 *   0 = success (out filled)
 *  -1 = bad arguments (null pointers)
 *   1 = no digits found (nothing converted)
 *   2 = trailing non-space characters after number
 *   3 = out of range for int (overflow/underflow)
 */
func parse_int(s : *char, out : *mut int) : int {
    var end_ptr : *char
    return parse_int_w_end(s, out, &mut end_ptr)
}

func parseCommand(config : &mut TestRunnerConfig, args : **char, end : **char) : *char {

    var current = args;
    while(current != end) {
        var command = *current;
        switch(fnv1_hash(command)) {
            comptime_fnv1_hash("--test-id"), comptime_fnv1_hash("-test-id") => {
                current++;
                if(current != end) {
                    const next = *current;
                    const res = parse_int(next, &config.single_test_id)
                    if(res != 0) {
                        printf("error: invalid function id %s", next);
                        return "invalid function id given for --test-id argument";
                    }
                } else {
                    printf("error: --test-id requires a single argument for the id")
                    return "--test-id requires a single argument for the id"
                }
            }
            comptime_fnv1_hash("--benchmark"), comptime_fnv1_hash("-benchmark") => {
                config.benchmark = true;
            }
            comptime_fnv1_hash("--process-limit") => {
                current++
                if(current != end) {
                    const next = *current;
                    const res = parse_int(next, &config.process_limit)
                    if(res != 0) {
                        printf("error: invalid process limit given", next);
                        return "invalid process limit given";
                    }
                } else {
                    printf("error: --process-limit requires a single integer argument")
                    return "--process-limit requires a single integer argument"
                }
            }
        }
        current++;
    }

}

func tester(argc : int, argv : **char) : int {

    if(argc == 0) {
        // error out, the executable argument not given
        printf("error: expected the first command line argument to be the executable path");
        return 1;
    }

    // super fast case
    if(argc == 1) {
        var config = TestRunnerConfig()
        run_tests(*argv, config);
        return 0;
    }

    // parse the command line
    var state = TestRunnerConfig()
    parseCommand(state, argv, argv + argc)

    // run the tests (it knows which ones to run from configuration)
    run_tests(*argv, state)

    return 0;

}