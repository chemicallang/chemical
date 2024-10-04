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

struct ImplicitFuncStruct {

    var i : int

    func take_implicit(&direct_int) : int {
        return direct_int
    }

    func take_implicit_self(&self, &direct_int) : int {
        return i + direct_int
    }

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
    test("implicit args work with functions in structs", () => {
        var i = ImplicitFuncStruct { i : 1 }
        provide 2 as direct_int {
            return i.take_implicit() == 2;
        }
    })
    test("implicit args work with self functions in structs", () => {
        var i = ImplicitFuncStruct { i : 1 }
        provide 2 as direct_int {
            return i.take_implicit_self() == 3;
        }
    })
    test("provide doesn't introduce a scope", () => {
        provide 3 as direct_int {
            var i = 0;
            i = check_direct_implicit_arg();
        }
        return i == 3;
    })
}