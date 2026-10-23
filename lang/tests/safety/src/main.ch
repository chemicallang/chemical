// Safety suite (S0.1 skeleton).
//
// This suite is the home for runtime-failure and parity tests. Compile-fail
// safety tests currently live in lang/tests/negative and are run by
// `--negative`; this module is wired as `--safety` so those tests can grow
// without slowing the main suite.

@test
func safety_suite_smoke(env : &mut TestEnv) {
    if(1 + 1 != 2) {
        env.error("safety suite sanity check failed")
    }
}

public func main(argc : int, argv : **char) {
    test_runner(argc, argv)
}
