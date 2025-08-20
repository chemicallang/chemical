func create_test_env(fn : *mut TestFunction) : TestEnvImpl {
    return TestEnvImpl {
        fn : fn,
        fd : FD_CONSTANT
    }
}