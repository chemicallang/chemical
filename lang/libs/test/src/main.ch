comptime func get_tests() : []TestFunction {
    return intrinsics::get_tests<TestFunction>() as []TestFunction
}

func parse_int_or_skip(str : *char, out : *mut int) : int {
    while(*str == ' ' || *str == '\t') { str++ }
    if(*str < '0' || *str > '9') { return -1 }
    return parse_int(str, out)
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
                    const res = parse_int(next, &raw mut config.single_test_id)
                    if(res != 0) {
                        printf("error: invalid function id %s", next);
                        return "invalid function id given for --test-id argument";
                    }
                } else {
                    printf("error: --test-id requires a single argument for the id")
                    return "--test-id requires a single argument for the id"
                }
            }
            comptime_fnv1_hash("--test-ids"), comptime_fnv1_hash("-test-ids") => {
                current++;
                if(current != end) {
                    const next = *current;
                    // Parse comma-separated integers manually (parse_int rejects trailing non-whitespace)
                    var buf = std::string()
                    var pos = next
                    while(*pos) {
                        if(*pos == ',') {
                            if(!buf.empty()) {
                                var val : int = 0
                                if(parse_int(buf.data(), &raw mut val) == 0) {
                                    config.test_ids.push(val)
                                }
                                buf.clear()
                            }
                        } else if(*pos >= '0' && *pos <= '9') {
                            buf.append(*pos)
                        }
                        pos++
                    }
                    if(!buf.empty()) {
                        var val : int = 0
                        if(parse_int(buf.data(), &raw mut val) == 0) {
                            config.test_ids.push(val)
                        }
                    }
                    config.has_test_ids = true
                    if(config.test_ids.empty()) {
                        printf("error: --test-ids requires comma-separated integer ids, got '%s'", next);
                        return "invalid test ids given for --test-ids argument";
                    }
                } else {
                    printf("error: --test-ids requires a comma-separated list of ids")
                    return "--test-ids requires a list of ids"
                }
            }
            comptime_fnv1_hash("--test-names"), comptime_fnv1_hash("-test-names") => {
                current++;
                if(current != end) {
                    const next = *current;
                    // parse comma-separated function names
                    var buf = std::string()
                    var pos = next
                    while(*pos) {
                        if(*pos == ',') {
                            if(!buf.empty()) {
                                var name_copy = strdup(buf.data())
                                config.test_names.push(name_copy)
                                buf.clear()
                            }
                        } else if(*pos != ' ') {
                            buf.append(*pos)
                        }
                        pos++
                    }
                    if(!buf.empty()) {
                        var name_copy = strdup(buf.data())
                        config.test_names.push(name_copy)
                    }
                    config.has_test_names = true
                    if(config.test_names.empty()) {
                        printf("error: --test-names requires comma-separated function names, got '%s'", next);
                        return "invalid test names given for --test-names argument";
                    }
                } else {
                    printf("error: --test-names requires a comma-separated list of function names")
                    return "--test-names requires a list of function names"
                }
            }
            comptime_fnv1_hash("--skip-sequential"), comptime_fnv1_hash("-skip-sequential") => {
                config.skip_sequential = true;
            }
            comptime_fnv1_hash("--include-remote"), comptime_fnv1_hash("-include-remote") => {
                config.include_remote = true;
            }
            comptime_fnv1_hash("--serial"), comptime_fnv1_hash("-serial") => {
                config.serial = true;
            }
            comptime_fnv1_hash("--jobs"), comptime_fnv1_hash("-jobs") => {
                current++;
                if(current != end) {
                    const next = *current;
                    var n : int = 0
                    if(parse_int(next, &raw mut n) != 0 || n < 1) {
                        printf("error: invalid job count '%s' for --jobs (expected a positive integer)", next)
                        return "--jobs requires a positive integer"
                    }
                    // 1 means serial; anything higher is handled by the same
                    // pool path with a narrower width.
                    config.serial = (n == 1)
                    if(n > 1) { config.job_limit = n }
                } else {
                    printf("error: --jobs requires a single positive integer argument")
                    return "--jobs requires a positive integer"
                }
            }
            comptime_fnv1_hash("--benchmark"), comptime_fnv1_hash("-benchmark") => {
                config.benchmark = true;
            }
            comptime_fnv1_hash("--comm-id") => {
                current++;
                if(current != end) {
                    const next = *current;
                    const res = parse_int(next, &raw mut config.comm_id)
                    if(res != 0) {
                        printf("error: invalid comm id %s", next);
                        return "invalid function id given for --comm-id argument";
                    }
                } else {
                    printf("error: --comm-id requires a single argument for the id");
                    return "--comm-id requires a single argument for the id"
                }
            }
            comptime_fnv1_hash("--successful-only") => {
                config.display.successful_only = true;
            }
            comptime_fnv1_hash("--failure-only") => {
                config.display.failure_only = true;
            }
            comptime_fnv1_hash("--no-logs") => {
                config.display.display_logs = false;
            }
            comptime_fnv1_hash("--process-limit") => {
                current++
                if(current != end) {
                    const next = *current;
                    const res = parse_int(next, &raw mut config.process_limit)
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
    return null;
}

/**
 * Runs the @test-discovered tests, streaming every finished test to `reporter`.
 *
 * Returns 1 when this process is a spawned test child (`--comm-id` present):
 * in that case tests run in-process for IPC and no reporting is performed.
 * Returns 0 otherwise.
 */
@retained
public func run_test_runner_reporting(tests_view : std::span<TestFunction>, reporter : &mut TestReporter, argc : int, argv : **char) : int {

    if(argc == 0) {
        // error out, the executable argument not given
        printf("error: expected the first command line argument to be the executable path");
        return 0;
    }

    // See run_tests logic - it relies on specific sentinel values
    // Since var config = TestRunnerConfig() zero-inits, we must reset key fields
    var config = TestRunnerConfig()
    config.single_test_id = -1
    config.comm_id = -1
    config.process_limit = 6
    config.display.display_logs = true
    config.has_test_ids = false
    config.has_test_names = false

    if(argc > 1) {
        // parse the command line
        parseCommand(&mut config, argv + 1, argv + argc)
    }

    // spawned child: run the single test in-process for IPC, no reporting
    if(config.comm_id != -1) {
        run_tests(&tests_view, *argv, &mut config, null);
        return 1;
    }

    var disp = config.display
    reporter.configure(&raw disp)
    reporter.begin_runner(std::string_view("test"))

    // run the tests (it knows which ones to run from configuration)
    run_tests(&tests_view, *argv, &mut config, &raw mut reporter)

    reporter.end_runner()
    return 0;
}

/** Single-runner entry point: streams to a default console reporter. */
@retained
public func run_test_runner(tests_view : std::span<TestFunction>, argc : int, argv : **char) : int {
    var console = new_console_reporter()
    const is_child = run_test_runner_reporting(tests_view, &mut console, argc, argv)
    if(is_child == 0) {
        finish_reporter(&mut console)
    }
    return 0;
}

type TestHandleRunFn = (h : *mut TestRunnerHandle, reporter : &mut TestReporter, argc : int, argv : **char) => int

@retained
public func test_handle_run(h : *mut TestRunnerHandle, reporter : &mut TestReporter, argc : int, argv : **char) : int {
    var ptr = h.tests_ptr as *mut TestFunction
    var span = std::span<TestFunction>(ptr, h.tests_count)
    return run_test_runner_reporting(span, reporter, argc, argv)
}

@retained
public func make_test_runner_handle(tests : std::span<TestFunction>) : TestRunnerHandle {
    return TestRunnerHandle {
        name : std::string_view("test"),
        tests_ptr : tests.data() as *void,
        tests_count : tests.size(),
        run_fn : test_handle_run as TestHandleRunFn
    }
}

/**
 * Comptime builder for a runnable @test runner handle. Evaluated at the call
 * site so `get_tests()` sees the caller's @test declarations, mirroring
 * `test_runner`.
 */
public comptime func test_runner_handle() : TestRunnerHandle {
    const t = get_tests()
    return %runtime_value(make_test_runner_handle(std::span<TestFunction>(t)))
}

public comptime func test_runner(argc : %maybe_runtime<int>, argv : %runtime<**char>) : int {
    const t = get_tests()
    return %runtime_value(run_test_runner(std::span<TestFunction>(t), argc, argv)) as int
}
