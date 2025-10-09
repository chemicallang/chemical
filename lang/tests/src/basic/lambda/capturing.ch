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

func take_cap_func(fun : std::function<() => int>) : int {
    return fun()
}

func take_cap_func_2(fun : std::function<() => int>) : int {
    var moved_func = fun
    return moved_func()
}

func pass_func_lambda(fun : std::function<() => int>) : int {
    return take_cap_func(fun);
}

func take_cap_func2(fun : std::function<(a : int) => int>) : int {
    return fun(8327)
}

func test_capturing_lambda() {
    test("capturing lambda works in var init", () => {
        var temp = 11;
        var fn : std::function<() => int> = |temp|() => {
            return temp;
        }
        return fn() == 11;
    })
    test("capturing lambda moved into var init can be called", () => {
        var temp = 938;
        var result = take_cap_func_2(|temp|() => { return temp; })
        return result == 938
    })
    test("capturing lambda works in function return", () => {
        var lambda = cap_func_ret();
        return lambda() == 22;
    })
    test("capturing lambda works in array value", () => {
        var temp1 = 87;
        var temp2 = 99;
        var arr : []std::function<() => int> = [
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
    test("capturing lambda works with assignment - 1", () => {
        var temp = 836;
        var fn : std::function<() => int> = |temp|() => {
            return temp + 11;
        }
        fn = |temp|() => {
            return temp;
        }
        return fn() == temp;
    })
    test("capturing lambda works with assignment - 2", () => {
        var temp = 836;
        var fn : std::function<() => int> = |temp|() => {
            return temp + 11;
        }
        var fn2 : std::function<() => int> = |temp|() => { return temp + 3; }
        fn = fn2
        return fn() == 839;
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
    test("can read captured reference", () => {
        var temp = 434
        return take_cap_func(|&temp|() => {
            return temp;
        }) == 434
    })
    test("can modify captured reference", () => {
        var temp = 0
        take_cap_func(|&mut temp|() => {
            temp = 121;
            return 0;
        })
        return temp == 121;
    })
    test("capturing lambdas can take parameters (no capture) - 1", () => {
        const arg = take_cap_func2(||(a : int) => {
            return a;
        })
        return arg == 8327
    })
    test("capturing lambdas can take parameters (with capture) - 2", () => {
        var x = 8237
        const arg = take_cap_func2(|x|(a : int) => {
            return a;
        })
        return arg == 8327
    })
    test("automatic dereference when captured is a reference for indexing - 1", () => {
        var index = 0
        const arg = take_cap_func2(|&mut index|(a : int) => {
            var arr = [ 8323, 23485 ]
            return arr[index]
        })
        return arg == 8323
    })
    test("automatic dereference when captured is a reference for indexing - 2", () => {
        var index = 1
        const arg = take_cap_func2(|&mut index|(a : int) => {
            var arr = [ 8323, 23485 ]
            return arr[index]
        })
        return arg == 23485
    })
    test_capturing_lambda_destruction()
}