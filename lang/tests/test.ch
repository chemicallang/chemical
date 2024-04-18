import "../std/std.ch"

const ANSI_COLOR_RESET = "\x1b[0m"
const ANSI_COLOR_RED = "\x1b[31m"
const ANSI_COLOR_GREEN = "\x1b[32m"
const ANSI_COLOR_YELLOW = "\x1b[33m"
const ANSI_COLOR_BLUE = "\x1b[34m"
const ANSI_COLOR_MAGENTA = "\x1b[35m"
const ANSI_COLOR_CYAN = "\x1b[36m"
const ANSI_COLOR_WHITE = "\x1b[37m"

func success_msg(message : string*) {
    printf("%s%s%s\n", ANSI_COLOR_GREEN, message, ANSI_COLOR_RESET);
}

func error_msg(message : string*) {
    printf("%s%s%s\n", ANSI_COLOR_RED, message, ANSI_COLOR_RESET);
}

func test(name : string*, assert : () => bool) {
    if(assert()) {
        printf("%s Test [%s] succeeded %s\n", ANSI_COLOR_GREEN, name, ANSI_COLOR_RESET);
    } else {
        printf("%s Test [%s] failed %s\n", ANSI_COLOR_RED, name, ANSI_COLOR_RESET);
    }
}

func assertEquals(actual : int, expected : int) : bool {
    if(actual != expected) {
        printf("%sExpected %s Got %s%s\n", ANSI_COLOR_RED, expected, actual, ANSI_COLOR_RESET);
        return false;
    }
    return true;
}