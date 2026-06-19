// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Native-Common Tests
// These tests use pointer arithmetic and run in both
// compiled and interpretation mode (but NOT on JVM).
// -------------------------------------------------------

public func run_native_common_tests() {

    // Basic pointer arithmetic and access (safe for interpretation mode)
    test_pointer_math();
    test_casts();

    // Comptime pointer arithmetic
    test_pointers_in_comptime();

}
