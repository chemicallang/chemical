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

    func send_message(&self, msg : *char) {
        var written : DWORD;
        if (!WriteFile(pipeHandle, msg, strlen(msg) as DWORD, &written, NULL)) {
            print_last_error("WriteFile");
        }
    }

    @override
    func logIt(&self, type : LogType, msgData : *char, lineNum : uint, charNum : uint) {
        var msg = std::string();
        msg.append_char_ptr("$log,")
        var buff : [2048]char
        snprintf(&mut buff[0], 100, "%d,%d,%d,%s", type as int, lineNum, charNum, msgData);
        msg.append_char_ptr(&buff[0])
        send_message(msg.data())
    }

    @override
    func quit_current_group(&self, reason : *char) {
        var msg = std::string();
        msg.append_char_ptr("$quit_group:");
        msg.append_char_ptr(reason)
        send_message(msg.data())
    }

    @override
    func quit_all_tests(&self, reason : *char) {
        var msg = std::string();
        msg.append_char_ptr("$quit_all:");
        msg.append_char_ptr(reason)
        send_message(msg.data())
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
    var before_each : *mut TestFunction = null
    /**
     * the testing function that should be executed after each test
     */
    var after_each : *mut TestFunction = null
}

func run_single_test(tfn : *mut TestFunction, config : &mut TestRunnerConfig) {

    var env = create_test_env(tfn);

    if(config.before_each) {
        config.before_each.ptr(env)
    }

    if(config.benchmark) {
        // TODO: benchmarking code here
        tfn.ptr(env);
    } else {
        tfn.ptr(env);
    }

    if(config.after_each) {
        config.after_each.ptr(env)
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
    switch(fnv1_hash(str)) {
        comptime_fnv1_hash("log") => {
            return MessageCommandType.Log;
        }
        comptime_fnv1_hash("quit_group") => {
            return MessageCommandType.QuitGroup;
        }
        comptime_fnv1_hash("quit_all") => {
            return MessageCommandType.QuitAll;
        }
        default => {
            return MessageCommandType.None;
        }
    }
}

func read_msg_type(msg_ptr : **char) : MessageCommandType {
    var msg = *msg_ptr;
    var command_buffer : char[120]
    var out = &command_buffer[0]
    while(true) {
        var c = *msg;
        if(c == '\0' || c == ',') {
            *out = '\0'
            *msg_ptr = msg;
            break;
        } else {
            *out = c;
            out++;
            msg++;
        }
    }
    return to_msg_cmd_type(&command_buffer[0]);
}

/* return codes:
 *   0 = success (out filled)
 *  -1 = bad arguments (null pointers)
 *   1 = no digits found (nothing converted)
 *   2 = trailing non-space characters after number
 *   3 = out of range for int (overflow/underflow)
 */
func parse_int_w_end(s : *char, out : *mut int, endPtrPtr : **mut char) : int {
    if (!s || !out) return -1;

    set_errno(0);
    var endptr = *endPtrPtr
    var val : long = strtol(s, endPtrPtr, 10);

    /* no conversion performed (e.g. empty string or only whitespace) */
    if (endptr == s) return 1;

    /* allow trailing whitespace but no other characters */
    while (*endptr != '\0' && isspace(*endptr as int)) {
        endptr++
    };
    if (*endptr != '\0') return 2;

    /* check range and errno for overflow/underflow */
    if (get_errno() == ERANGE || val > INT_MAX || val < INT_MIN) return 3;

    *out = val as int;
    return 0;
}

func read_int(msg_ptr : **char) : int {
    var end_ptr : *mut char
    var out : int
    var err = parse_int_w_end(*msg_ptr, &mut out, &mut end_ptr)
    // TODO: ignoring error here
    *msg_ptr = end_ptr
    return out;
}

func read_char(msg_ptr : **char, c : char) : bool {
    var msg = *msg_ptr
    if(*msg == '\0') return false;
    if(*msg == c) {
        msg++;
        *msg_ptr = msg;
        return true;
    } else {
        return false;
    }
}

func read_str(msg_ptr : **mut char, into : &mut std::string) {
    var msg = *msg_ptr
    while(msg != '\0') {
        into.append(*msg)
        msg++;
    }
    *msg_ptr = msg;
}

func parseLog(msg_ptr : **char, log : &mut TestLog) {
    if(!read_char(msg_ptr, ',')) {
        return;
    }
    log.type = read_int(msg_ptr) as LogType
    if(!read_char(msg_ptr, ',')) {
        return;
    }
    log.line = read_int(msg_ptr) as ubigint
    if(!read_char(msg_ptr, ',')) {
        return;
    }
    log.character = read_int(msg_ptr) as ubigint
    if(!read_char(msg_ptr, ',')) {
        return;
    }
    read_str(msg_ptr, log.message)
}

/**
 * to give you an idea of how messages are formatted
 *
 * the format is
 * $command, multiple parameters
 *
 *
 * $log,0,10,20,this is a normal log message
 * $log,1,10,20,this is an error log message
 * $log,2,10,20,this is a warning log message
 * $log,3,10,20,this is a info log message
 * $log,4,10,20,this is a success log message
 * $quit_group: this is a message to quit current group
 * $quit_all: this is a message to quit everything
 */
func process_message(state : &mut TestFunctionState, msg : *char) {

    if(!read_char(&mut msg, '$')) {
        // probably not our message
    }

    var msgType = read_msg_type(&mut msg)
    switch(msgType) {
        MessageCommandType.None, default => {
            return;
        }
        MessageCommandType.Log => {
            var log = TestLog {}
            parseLog(&mut msg, log);
            state.logs.push(log)
        }
        MessageCommandType.QuitGroup => {
            // TODO quit group
        }
        MessageCommandType.QuitAll => {
            // TODO quit all
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
            if (exitCode == 0) {
                printf("Process exited normally (success)\n");
            } else if (exitCode < 0xC0000000) {
                printf("Process exited with failure code %lu\n", exitCode);
            } else {
                printf("Process crashed with exception code 0x%08lX\n", exitCode);
            }
        }

        // set the exit code in state
        state.exitCode = exitCode;

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return 0;

    }

}

func run_tests(exe_path : *char, config : &mut TestRunnerConfig) {

    var tests = get_tests();
    var tests_view = std::span<TestFunction>(tests)

    if(config.single_test_id != -1) {

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
            printf("launching test : %d\n", test_start.id);

            // creating state for the test function
            var ind = state.tests.size()
            state.tests.push(TestFunctionState(test_start));
            const fn_state = state.tests.get_ptr(ind)

            launch_test(exe_path, test_start.id, *fn_state)
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

@test
func test_me(env : &mut TestEnv) {

}

public func main(argc : int, argv : **char) : int {
    return tester(argc, argv);
}