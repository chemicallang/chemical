// Copyright (c) Chemical Language Foundation 2026.

// The interpretation job finds and calls this main() function.
// It runs common tests via run_common_tests().
//
// The interpretation job runs the *same MIR* as the compiled backends, so it
// no longer has an AST-interpreter-specific eager `async` mode. Async functions
// are coroutines on every path; a future must be driven by an executor, which
// the interpreter does not provide. The old `run_interpret_async_tests` (which
// locked the eager behaviour of the AST interpreter, see
// `lang/docs/async-library-integration.md` §3.1) is therefore gone with the AST
// interpreter. Comptime async remains eager and is locked by the compiler tests.

public func main() {
    run_common_tests();
    run_native_common_tests();
    print_test_stats();
}
