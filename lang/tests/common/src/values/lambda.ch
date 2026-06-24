// Copyright (c) Chemical Language Foundation 2026.
//
// Lambda tests — moved from lang/tests/src/basic/lambda/lambda.ch
// Only basic lambda patterns that work in the interpreter.
// Storing lambdas in struct fields and arrays is not yet supported
// by the interpreter — those tests remain native-only.

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

struct PointSome {
    var a : int
    var b : int
}

func lamb_ret_struct() : (a : int, b : int) => PointSome {
    return (a, b) => {
        return PointSome {
            a : a,
            b : b
        }
    }
}

struct ProvideStructLamb {
    var lamb : (a : int, b : int) => PointSome
}

func take_lambda_with_param(lambda : (param : int) => int) : int {
    return lambda(4)
}

func fn_rets_1() : int {
    return 1;
}

func fn_rets_2() : int {
    return 2;
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
    // Storing lambdas in struct fields, arrays, or returning from functions
    // is not yet supported in the interpreter — kept native-only
}
