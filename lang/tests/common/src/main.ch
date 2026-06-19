// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Common Tests
// These tests run in both compiled and interpretation mode.
// -------------------------------------------------------

// Self-contained comptime test functions

comptime func common_comptime_test() : bool {
    var x = 5 + 3
    return x == 8
}

comptime func common_comptime_loop_test() : bool {
    var sum = 0
    for(var i = 0; i < 5; i++) {
        sum += i
    }
    return sum == 10
}

public func run_common_tests() {

    // Test 1: Basic comptime arithmetic
    test("comptime arithmetic works", () => {
        return common_comptime_test();
    })

    // Test 2: Comptime loop works
    test("comptime loop works", () => {
        return common_comptime_loop_test();
    })

    // Print test stats
    print_common_test_stats();
}
