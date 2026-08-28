// Copyright (c) Chemical Language Foundation 2026.
//
// Tests for generic instantiation in global variable values.
// The generic instantiation pass now runs after linking all signatures,
// so generic types used in global variable initializers don't crash the compiler.

// Generic struct used for testing instantiation in global variable values
struct GlobalGenBox<T : Copy> {
    var value : T
    func get(&self) : T {
        return value
    }
}

// Global variables whose values are generic instantiations
// These test that the generic instantiation pass correctly handles
// generic types used in global variable initializers (same module).
var g_int_box = GlobalGenBox<int> { value : 42 }
var g_long_box = GlobalGenBox<long> { value : 12345 }
var g_pair = PairGen<int, int, int> { a : 10, b : 20 }

func test_generic_in_global_var() {
    test("generic struct in global variable value works - int", () => {
        return g_int_box.get() == 42
    })
    test("generic struct in global variable value works - long", () => {
        return g_long_box.get() == 12345
    })
    test("generic struct in global variable value works - PairGen", () => {
        return g_pair.add() == 30
    })
}
