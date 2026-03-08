func run_test_fn_ptr(env : &mut TestEnv, ptr : *void, ret_bool : bool) : bool {
    if(ret_bool) {
        type bool_fn_type = (env : &mut TestEnv) => bool
        const fn_ptr = ptr as bool_fn_type
        return fn_ptr(env);
    } else {
        type non_bool_fn_type = (env : &mut TestEnv) => void
        const fn_ptr = ptr as non_bool_fn_type
        fn_ptr(env);
        return true;
    }
}

func run_single_test(tfn : *mut TestFunction, config : &mut TestRunnerConfig) {

    var env = create_test_env(tfn, config);

    if(config.before_each) {
        config.before_each(env)
    }

    var return_success : bool
    if(config.benchmark) {
        // TODO: benchmarking code here
        return_success = run_test_fn_ptr(env, tfn.ptr as *void, tfn.returns_bool)
    } else {
        return_success = run_test_fn_ptr(env, tfn.ptr as *void, tfn.returns_bool)
    }
    if(return_success == false) {
        env.error("boolean test returned false");
    }

    if(config.after_each) {
        config.after_each(env)
    }

}

func append_integer(str : &mut std::string, dig : int) {
    var buffer : [160]char
    const buffStart = &mut buffer[0]
    snprintf(buffStart, sizeof(buffer), "%d", dig)
    str.append_char_ptr(buffStart);
}

func launch_test_with_retries(exe_path : *char, id : int, state : &mut TestFunctionState, tries : int, timeout_ms : uint) {
    // actual launch
    launch_test(exe_path, id, state, timeout_ms)
    // retry for number of times
    if(tries > 1) {
        tries--
        while(tries > 1 && state.has_failed) {
            state.logs.clear()
            launch_test(exe_path, id, state, timeout_ms)
            tries--
        }
    }
}

type TestFunctionPtr = (env : &mut TestEnv) => void

func run_tests(tests_view : &std::span<TestFunction>, exe_path : *char, config : &mut TestRunnerConfig) {

    if(config.single_test_id != -1) {

        // initialize before each and after each
        config.before_each = intrinsics::get_single_marked_decl_ptr("test.before_each") as TestFunctionPtr;
        config.after_each = intrinsics::get_single_marked_decl_ptr("test.after_each") as TestFunctionPtr;

        var test_start = tests_view.data() as *mut TestFunction
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

        if(tests_view.empty()) {
            return;
        }

        // create test runner state
        var state = TestRunnerState()
        state.tests.reserve(tests_view.size());

        // creating a thread pool for launching asynchronous tests on it
        var pool = std.concurrent.create_pool(std.concurrent.hardware_threads());
        var asyncJobs = std.vector<std.concurrent.Future<int>>();

        var test_start = tests_view.data() as *mut TestFunction
        const test_end = test_start + tests_view.size()

        while(test_start != test_end) {
            // creating state for the test function
            var ind = state.tests.size()
            state.tests.push(TestFunctionState(test_start));
            const fn_state = state.tests.get_ptr(ind)
            const test_id = test_start.id;
            const test_retry = test_start.retry as int

            // timeout in milliseconds
            // by default 10 seconds
            var timeout_ms = 10000;
            if(test_start.timeout > 0) {
                // assuming test_start.timeout is in milliseconds
                // if it's in seconds, we should multiply by 1000
                // but let's assume it's ms for flexibility
                timeout_ms = test_start.timeout as int
            }

            // actual launch on the thread pool
            var job = pool.submit<int>(|exe_path, test_id, fn_state, test_retry, timeout_ms|() => {
                launch_test_with_retries(exe_path, test_id, *fn_state, test_retry, timeout_ms as uint);
                return 0;
            })
            asyncJobs.push(job)

            test_start++;
        }

        // wait for the thread pool to finish
        var asyncJobsStart = asyncJobs.data() as *mut std.concurrent.Future<int>
        const asyncJobsEnd = asyncJobsStart + asyncJobs.size()
        while(asyncJobsStart != asyncJobsEnd) {
            const redundant = asyncJobsStart.get()
            asyncJobsStart++;
        }

        // print the test results
        print_test_results(config.display, state.tests.data(), state.tests.size())

    }

}
