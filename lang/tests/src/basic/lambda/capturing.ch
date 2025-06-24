func cap_func_ret() : std::function<() => int> {
    var temp = 22;
    return |temp|() => {
        return temp;
    }
}

struct CapLambTestCont {
    var lamb1 : std::function<() => int>
}

func take_cap_func(func : std::function<() => int>) : int {
    return func()
}

func test_capturing_lambda() {
    test("capturing lambda works in var init", () => {
        var temp = 11;
        var fn : std::function<() => int> = |temp|() => {
            return temp;
        }
        return fn() == 11;
    })
    /**
    test("capturing lambda works in function return", () => {
        var lambda = cap_func_ret();
        return lambda() == 22;
    })
    **/
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
}