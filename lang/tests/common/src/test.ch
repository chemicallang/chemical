// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Common Test Infrastructure
// Works in both compiled (runtime) and interpreted modes without requiring std lib
// -------------------------------------------------------

@extern
public func printf(format : *char, _ : any...) : int

const ANSI_COLOR_RESET = "\x1b[0m"
const ANSI_COLOR_RED = "\x1b[31m"
const ANSI_COLOR_GREEN = "\x1b[32m"

var total_tests = 0;
var tests_passed = 0;
var tests_failed = 0;

public func test(name : *char, assert : () => bool) {
    if(assert()) {
        tests_passed++;
        comptime if(intrinsics::is_comptime()) {
            intrinsics::expr_println(`${ANSI_COLOR_GREEN}Test ${total_tests + 1} [${name}] succeeded${ANSI_COLOR_RESET}`);
        } else {
            printf("%sTest %d [%s] succeeded %s\n", ANSI_COLOR_GREEN, total_tests + 1, name, ANSI_COLOR_RESET);
        }
    } else {
        tests_failed++;
        comptime if(intrinsics::is_comptime()) {
            intrinsics::expr_println(`${ANSI_COLOR_RED}Test ${total_tests + 1} [${name}] failed${ANSI_COLOR_RESET}`);
        } else {
            printf("%sTest %d [%s] failed %s\n", ANSI_COLOR_RED, total_tests + 1, name, ANSI_COLOR_RESET);
        }
    }
    total_tests++;
}

public func print_test_stats() {
    var failed_color = if(tests_failed == 0) ANSI_COLOR_GREEN else ANSI_COLOR_RED
    comptime if(intrinsics::is_comptime()) {
        intrinsics::expr_println(`Total ${total_tests} ${ANSI_COLOR_GREEN}Passed ${tests_passed}${ANSI_COLOR_RESET} ${failed_color}Failed ${tests_failed}${ANSI_COLOR_RESET}`);
    } else {
        printf("Total %d %sPassed %d%s %sFailed %d%s", total_tests, ANSI_COLOR_GREEN, tests_passed, ANSI_COLOR_RESET, failed_color, tests_failed, ANSI_COLOR_RESET);
    }
}


public func assertEquals(actual : int, expected : int) : bool {
    if(actual != expected) {
        comptime if(intrinsics::is_comptime()) {
            intrinsics::expr_println(`${ANSI_COLOR_RED}Expected ${expected} Got ${actual}${ANSI_COLOR_RESET}`);
        } else {
            printf("%sExpected %s Got %s%s\n", ANSI_COLOR_RED, expected, actual, ANSI_COLOR_RESET);
        }
        return false;
    } else {
        return true;
    }
}
