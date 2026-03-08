comptime func get_tests() : []TestFunction {
    return intrinsics::get_tests<TestFunction>() as []TestFunction
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
                    const res = parse_int(next, &mut config.single_test_id)
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
            comptime_fnv1_hash("--comm-id") => {
                current++;
                if(current != end) {
                    const next = *current;
                    const res = parse_int(next, &mut config.comm_id)
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
                    const res = parse_int(next, &mut config.process_limit)
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

public func run_test_runner(tests_view : std::span<TestFunction>, argc : int, argv : **char) : int {

    if(argc == 0) {
        // error out, the executable argument not given
        printf("error: expected the first command line argument to be the executable path");
        return 1;
    }

    // super fast case
    if(argc == 1) {
        var config = TestRunnerConfig()
        run_tests(tests_view, *argv, config);
        return 0;
    }

    // parse the command line
    var config = TestRunnerConfig()
    parseCommand(config, argv + 1, argv + argc)

    // run the tests (it knows which ones to run from configuration)
    run_tests(tests_view, *argv, config)

    return 0;

}

public comptime func test_runner(argc : %maybe_runtime<int>, argv : %runtime<**char>) : int {
    const t = get_tests()
    return intrinsics::wrap(run_test_runner(std::span<TestFunction>(t), argc, argv)) as int
}