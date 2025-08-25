func create_test_env(fn : *mut TestFunction, config : &mut TestRunnerConfig) : TestEnvImpl {
    return TestEnvImpl {
        fn : fn,
        fd : config.comm_id
    }
}