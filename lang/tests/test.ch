import "../std/std.ch"

const ANSI_COLOR_RESET = "\x1b[0m"
const ANSI_COLOR_RED = "\x1b[31m"
const ANSI_COLOR_GREEN = "\x1b[32m"
const ANSI_COLOR_YELLOW = "\x1b[33m"
const ANSI_COLOR_BLUE = "\x1b[34m"
const ANSI_COLOR_MAGENTA = "\x1b[35m"
const ANSI_COLOR_CYAN = "\x1b[36m"
const ANSI_COLOR_WHITE = "\x1b[37m"

func success_msg(message : *char) {
    printf("%s%s%s\n", ANSI_COLOR_GREEN, message, ANSI_COLOR_RESET);
}

func error_msg(message : *char) {
    printf("%s%s%s\n", ANSI_COLOR_RED, message, ANSI_COLOR_RESET);
}

var total_tests = 0;
var tests_passed = 0;
var tests_failed = 0;

func test(name : *char, assert : () => bool) {
    if(assert()) {
        printf("%sTest %d [%s] succeeded %s\n", ANSI_COLOR_GREEN, total_tests + 1, name, ANSI_COLOR_RESET);
        tests_passed++;
    } else {
        printf("%sTest %d [%s] failed %s\n", ANSI_COLOR_RED, total_tests + 1, name, ANSI_COLOR_RESET);
        tests_failed++;
    }
    total_tests++;
}

func print_test_stats() {
    printf("Total %d", total_tests);
    printf(" %sPassed %d%s", ANSI_COLOR_GREEN, tests_passed, ANSI_COLOR_RESET);

    if(tests_failed == 0) {
        printf(" %s", ANSI_COLOR_GREEN);
    } else {
        printf(" %s", ANSI_COLOR_RED);
    }

    printf("Failed %d", tests_failed);

    printf("%s", ANSI_COLOR_RESET);
}

func assertEquals(actual : int, expected : int) : bool {
    if(actual != expected) {
        printf("%sExpected %s Got %s%s\n", ANSI_COLOR_RED, expected, actual, ANSI_COLOR_RESET);
        return false;
    } else {
        return true;
    }
}