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

func main() {
    test("4 + 2 / 2 == 5", []() => {
        return (4 + 2 / 2) == 5;
    });
    test("4 / 2 + 2 == 4", []() => {
        return (4 / 2 + 2) == 4;
    });
    test("3 * (4 + 2) - 8 / 2 == 5", []() => {
        return (4 + 2 / 2) == 5;
    });
    test("3 * (4 + 2) - 8 / 2 == 14", []() => {
        return (3 * (4 + 2) - 8 / 2) == 14;
    });
    test("8 / (2 + 2) * 3 - 1 == 5", []() => {
        return (8 / (2 + 2) * 3 - 1) == 5;
    });
    test("(5 + 3) * 2 - 4 / (1 + 1) == 14", []() => {
        return ((5 + 3) * 2 - 4 / (1 + 1)) == 14;
    });
    return;
}