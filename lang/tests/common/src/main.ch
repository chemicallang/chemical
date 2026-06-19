// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Common Tests
// These tests run in both compiled and interpretation mode.
// -------------------------------------------------------

// Self-contained comptime test functions

func very_basic_arithemetic() : bool {
    return 5 + 3 == 8
}

func traditional_for_loop_with_sum() : bool {
    var sum = 0
    for(var i = 0; i < 5; i++) {
        sum += i
    }
    return sum == 10
}

public func run_common_tests() {

    // Test 1: Basic comptime arithmetic
    test("very basic arithmetic works", () => {
        return very_basic_arithemetic();
    })

    // Test 2: Comptime loop works
    test("C loop with sum works", () => {
        return traditional_for_loop_with_sum();
    })

    // Basic language tests (no pointers — compatible with all backends)
    test_numbers();
    test_floating_expr();
    test_struct_values();
    test_in_value();
    test_for_loop();
    test_inc_dec();

}
