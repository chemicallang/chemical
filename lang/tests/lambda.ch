import "test.ch"

struct LambdaProvider {
    var provide : () => int
}

struct Nested {
    var provider : LambdaProvider
}

func capturing(lambda : []() => bool) : bool {
    return lambda();
}

func dontCapture(lambda : () => bool) : bool {
    return lambda();
}

func passed(pass : () => int) : int {
    return pass();
}

func create_lamb(first : bool) : () => int {
    if(first) {
        return () => 5;
    } else {
        return () => 10;
    }
}

func test_lambda() {
    test("testing non capturing lambda works", () => {
        return true;
    })
    test("testing lambda without braces works", () => true)
    test("testing non capturing lambda works", () => {
        return dontCapture(() => {
            return true;
        });
    });
    test("testing non capturing lambda works without body", () => {
        return dontCapture(() => true);
    });
    test("testing non capturing lambda can be passed to capturing lambda", () => {
        return capturing(() => {
            return true;
        });
    });
    test("testing capturing lambda works", () => {
        return capturing([]() => {
            return true;
        });
    });
    test("can pass function pointer as lambda 1", () => {
        return passed(fn_rets_1) == 1;
    })
    test("can pass function pointer as lambda 2", () => {
        return passed(fn_rets_2) == 2;
    })
    test("can call lambda from a variable", () => {
        var x : () => int = () => {
            return 252;
        }
        return x() == 252;
    })
    test("can call lambda without knowing type", () => {
        var x = () => {
            return 253;
        }
        return x() == 253;
    })
    test("can call lambda stored in struct", () => {
        var p = LambdaProvider {
            provide : () => {
                return 252;
            }
        }
        return p.provide() == 252;
    })
    test("can call lambda stored nested in a struct", () => {
        var n = Nested {
            provider : LambdaProvider {
               provide : () => {
                   return 252;
               }
           }
        }
        return n.provider.provide() == 252;
    })
    test("can call lambda stored in an array", () => {
        var arr = {}()=>int(2);
        arr[0] = () => 5;
        arr[1] = () => 10;
        return arr[0]() == 5 && arr[1]() == 10;
    })
    test("a function can return lambdas", () => {
        return create_lamb(true)() == 5 && create_lamb(false)() == 10;
    })
}

func fn_rets_1() : int {
    return 1;
}

func fn_rets_2() : int {
    return 2;
}