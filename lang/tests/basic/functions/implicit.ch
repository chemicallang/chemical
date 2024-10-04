import "../../test.ch"

typealias direct_int = int

func check_direct_implicit_arg(&direct_int) : int {
    return direct_int
}

func test_implicit_functions() {
    test("provide works with direct implicit arguments", () => {
        provide 3 as direct_int {
            return check_direct_implicit_arg() == 3;
        }
    })
}