func cap_func_ret() : std::function<() => int> {
    var temp = 22;
    return |temp|() => {
        return temp;
    }
}

struct CapLambTestCont {
    var lamb1 : std::function<() => int>
}

variant OptCapLamb {
    Some(lamb : std::function<() => int>)
    None()
}

func take_cap_func(func : std::function<() => int>) : int {
    return func()
}

func pass_func_lambda(func : std::function<() => int>) : int {
    return take_cap_func(func);
}

func test_capturing_lambda() {
    test("capturing lambda works in var init", () => {
        var temp = 11;
        var fn : std::function<() => int> = |temp|() => {
            return temp;
        }
        return fn() == 11;
    })
    test("capturing lambda works in function return", () => {
        var lambda = cap_func_ret();
        return lambda() == 22;
    })
    test("capturing lambda works in array value", () => {
        var temp1 = 87;
        var temp2 = 99;
        var arr : std::function<() => int>[] = [
            |temp1|() => {
                return temp1;
            },
            |temp2|() => {
                return temp2;
            }
        ]
        return arr[0]() == temp1 && arr[1]() == temp2
    })
    test("capturing lambda works in struct value", () => {
        var temp1 = 57;
        var con = CapLambTestCont {
            lamb1 : |temp1|() => {
                return temp1;
            }
        }
        return con.lamb1() == temp1
    })
    test("capturing lambda works when passing to functions", () => {
        var temp = 74
        return take_cap_func(|temp|() => {
            return temp;
        }) == temp
    })
    test("capturing lambda can be stored and called from variant", () => {
        var temp = 873
        var s = OptCapLamb.Some(|temp|() => { return temp; })
        var Some(lamb) = s else unreachable
        return lamb() == temp;
    })
    test("capturing lambda works with assignment", () => {
        var temp = 836;
        var fn : std::function<() => int> = |temp|() => {
            return temp + 11;
        }
        fn = |temp|() => {
            return temp;
        }
        return fn() == temp;
    })
    test("capturing functions can be passed around", () => {
        var temp = 384
        return pass_func_lambda(|temp|() => { return temp; }) == 384
    })
    test("non capturing functions can be passed to capturing function types - 1", () => {
        return pass_func_lambda(() => { return 832; }) == 832
    })
    test("non capturing functions can be passed to capturing function types - 2", () => {
        return pass_func_lambda(||() => { return 234; }) == 234
    })
}