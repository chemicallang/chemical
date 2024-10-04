import "../../test.ch"

typealias direct_int = int

func check_direct_implicit_arg(&direct_int) : int {
    return direct_int
}

func in_between(&direct_int) : int {
    return check_direct_implicit_arg();
}

func implicit_with_explicit_arg(&direct_int, other : int) : int {
    return check_direct_implicit_arg() + other;
}

func test_implicit_functions() {
    test("provide works with direct implicit arguments", () => {
        provide 3 as direct_int {
            return check_direct_implicit_arg() == 3;
        }
    })
    test("provide works with direct implicit arguments passed between functions", () => {
        provide 4 as direct_int {
            return in_between() == 4;
        }
    })
    test("function with implicit argument can take explicit argument", () => {
        provide 5 as direct_int {
            return implicit_with_explicit_arg(3) == 8;
        }
    })
}