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

func can_take_literal_type(my_int : literal::int) : int {
    return my_int + 3;
}

func sno_func() : int {
    return 10;
}

func sno_func(a : int) : int {
    return a;
}

func sno_func(a : int, b : int) : int {
    return a + b;
}

func test_name_overriding() {
    test("correct function is called when same names overriding - 1", () => {
        return sno_func() == 10
    })
    test("correct function is called when same names overriding - 2", () => {
        return sno_func(20) == 20
    })
    test("correct function is called when same names overriding - 3", () => {
        return sno_func(20, 20) == 40
    })
}

struct SnoPair {

    var a : int
    var b : int

    func plus(&self) : int {
        return a + b;
    }

    func plus(&self, c : int) : int {
        return a + b + c;
    }

    func plus(&self, c : int, d : int) : int{
        return a + b + c + d;
    }

}

func test_name_overriding_in_struct() {
    test("correct function is called when same names overriding in struct - 1", () => {
        var p = SnoPair { a : 10, b : 10 }
        return p.plus() == 20;
    })
    test("correct function is called when same names overriding in struct - 1", () => {
        var p = SnoPair { a : 10, b : 10 }
        return p.plus(10) == 30;
    })
    test("correct function is called when same names overriding in struct - 1", () => {
        var p = SnoPair { a : 10, b : 10 }
        return p.plus(10, 10) == 40;
    })
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
        var arr = {test_struct()}TestStruct1(1)
        return arr[0].x == 100 && arr[0].y == 100
    })
    test("expression test function is not called in 'or' ||", () => {
        is_expr_test_func_called = false
        if(true || expr_test_func_call()) {

        }
        return is_expr_test_func_called == false;
    })
    test("expression test function is called in 'or' ||", () => {
        is_expr_test_func_called = false
        if(false || expr_test_func_call()) {

        }
        return is_expr_test_func_called == true;
    })
    test("expression test function is not called in 'and' &&", () => {
        is_expr_test_func_called = false
        if(false && expr_test_func_call()) {

        }
        return is_expr_test_func_called == false;
    })
    test("expression test function is called in 'and' &&", () => {
        is_expr_test_func_called = false
        if(true && expr_test_func_call()) {

        }
        return is_expr_test_func_called == true;
    })
    test("function can take literal type", () => {
        return can_take_literal_type(2) == 5
    })
    test_name_overriding();
    test_name_overriding_in_struct();
}