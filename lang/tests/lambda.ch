import "test.ch"

func capturing(lambda : []() => bool) : bool {
    return lambda();
}

func dontCapture(lambda : () => bool) : bool {
    return lambda();
}

func passed(pass : () => int) : int {
    return pass();
}

func test_lambda() {
    test("testing non capturing lambda works", []() => {
        return true;
    })
    test("testing lambda without braces works", []() => true)
    test("testing non capturing lambda works", []() => {
        return dontCapture(() => {
            return true;
        });
    });
    test("testing non capturing lambda works without body", []() => {
        return dontCapture(() => true);
    });
    test("testing capturing & nested lambda works", []() => {
        return capturing([]() => {
            return true;
        });
    });
    test("can pass function pointer as lambda 1", []() => {
        return passed(fn_rets_1) == 1;
    })
    test("can pass function pointer as lambda 2", []() => {
        return passed(fn_rets_2) == 2;
    })
    test("can call lambda from a variable", []() => {
        var x : () => int = []() => {
            return 252;
        }
        return x() == 252;
    })
}

func fn_rets_1() {
    return 1;
}

func fn_rets_2() {
    return 2;
}