// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Common Test Infrastructure
// Works in both compiled (runtime) and interpreted modes.
// Uses comptime if to choose between intrinsics::println
// (interpretation) and printf (compiled mode).
// -------------------------------------------------------

const ANSI_COLOR_RESET = "\x1b[0m"
const ANSI_COLOR_RED = "\x1b[31m"
const ANSI_COLOR_GREEN = "\x1b[32m"

var total_tests = 0;
var tests_passed = 0;
var tests_failed = 0;

public func test(name : *char, assert : () => bool) {
    if(assert()) {
        tests_passed++;
        comptime if(intrinsics::is_interpretation()) {
            intrinsics::println("Test ", total_tests + 1, " [", name, "] succeeded");
        } else {
            printf("%sTest %d [%s] succeeded %s\n", ANSI_COLOR_GREEN, total_tests + 1, name, ANSI_COLOR_RESET);
        }
    } else {
        tests_failed++;
        comptime if(intrinsics::is_interpretation()) {
            intrinsics::println("Test ", total_tests + 1, " [", name, "] failed");
        } else {
            printf("%sTest %d [%s] failed %s\n", ANSI_COLOR_RED, total_tests + 1, name, ANSI_COLOR_RESET);
        }
    }
    total_tests++;
}

public func print_common_test_stats() {
    comptime if(intrinsics::is_interpretation()) {
        intrinsics::println("Common tests: ", total_tests, " total, ", tests_passed, " passed, ", tests_failed, " failed");
    } else {
        printf("Common tests: %d total, %d passed, %d failed\n", total_tests, tests_passed, tests_failed);
    }
}

public func assertEquals(actual : int, expected : int) : bool {
    if(actual != expected) {
        comptime if(intrinsics::is_interpretation()) {
            intrinsics::println("Expected ", expected, " Got ", actual);
        } else {
            printf("%sExpected %s Got %s%s\n", ANSI_COLOR_RED, expected, actual, ANSI_COLOR_RESET);
        }
        return false;
    } else {
        return true;
    }
}
