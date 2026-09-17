// Copyright (c) Chemical Language Foundation 2026.

// The interpretation job finds and calls this main() function.
// It runs common tests via run_common_tests().

// Async at comptime is eager: the interpreter has no executor, symres does not
// wrap async return types here, and `await` just yields its inner expression.
// This locks that behavior (see lang/docs/async-library-integration.md §3.1).
async func interp_eager_add(a : int, b : int) : int {
    return a + b
}

async func interp_await_chain(a : int) : int {
    var first = await interp_eager_add(a, 1)
    var second = await interp_eager_add(first, 1)
    return second
}

func run_interpret_async_tests() {
    test("async/await is eager during interpretation", () => {
        return interp_await_chain(40) == 42
    })
}

public func main() {
    run_common_tests();
    run_native_common_tests();
    run_interpret_async_tests();
    print_test_stats();
}
