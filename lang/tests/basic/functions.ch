import "../test.ch"

struct TestStruct1 {
    var x : int
    var y : int
}

struct TestStruct2 {
    var test : TestStruct1
}

func test_struct() : TestStruct1 {
    return TestStruct1 {
        x : 100,
        y : 100
    }
}

struct MemLamb {

    var lamb : () => TestStruct1

    func mem_func_test() : TestStruct1 {
        return TestStruct1 {
            x : 200,
            y : 200
        }
    }

}

func (mem : MemLamb*) ext_func_test() : TestStruct1 {
    return TestStruct1 {
        x : 300,
        y : 300
    }
}

var is_expr_test_func_called = false;

func expr_test_func_call() : bool {
    is_expr_test_func_called = true;
    return true;
}

func test_functions() {
    test("test struct member access in chain, tld function return", () => {
        return test_struct().x == 100;
    })
    test("test struct member access in chain, lambda function return", () => {
        var lamb : () => TestStruct1 = test_struct;
        return lamb().x == 100;
    })
    test("test struct member access in chain, member lambda return", () => {
        var mem = MemLamb {
            lamb : test_struct
        }
        return mem.lamb().x == 100;
    })
    test("test struct member access in chain, member function return", () => {
        var mem = MemLamb {}
        return mem.mem_func_test().x == 200;
    })
    test("test struct member access in chain, extension function return", () => {
        var mem = MemLamb {}
        return mem.ext_func_test().x == 300;
    })
    test("supports function calls that return structs inside struct values", () => {
        var t = TestStruct2 {
            test : test_struct()
        }
        return t.test.x == 100 && t.test.y == 100
    })
    test("supports function calls that return structs inside array values", () => {
        var arr = {test_struct()}TestStruct2(1)
        return arr[0].x == 100 && arr[0].y == 100
    })
    test("expression test function is not called in 'or' ||", () => {
        if(true || expr_test_func_call()) {

        }
        return is_expr_test_func_called == false;
    })
    test("expression test function is called in 'or' ||", () => {
        if(false || expr_test_func_call()) {

        }
        return is_expr_test_func_called == true;
    })
    test("expression test function is not called in 'and' ||", () => {
        if(false && expr_test_func_call()) {

        }
        return is_expr_test_func_called == false;
    })
    test("expression test function is called in 'and' ||", () => {
        if(true && expr_test_func_call()) {

        }
        return is_expr_test_func_called == true;
    })
}