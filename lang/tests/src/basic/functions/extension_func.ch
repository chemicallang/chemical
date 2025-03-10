struct ExtFuncTestPoint {
    var a : int
    var b : int
}

func (point : &ExtFuncTestPoint) sum_ext_func_test_point() : int {
    return point.a + point.b;
}

struct ExtFuncTestVertex : ExtFuncTestPoint {
    var c : int
}

interface GenericExtFuncTestInterface {

    func sum_it(&self) : int

}

func <T : GenericExtFuncTestInterface> (inter : &mut T) sum_gen_ext_func_inter_twice() : int {
    return inter.sum_it() * 2;
}

struct SumItExtFuncTestOne : GenericExtFuncTestInterface {

    @override
    func sum_it(&self) : int {
        return 2;
    }

}

struct SumItExtFuncTestTwo : GenericExtFuncTestInterface {

    var a : int
    var b : int

    @override
    func sum_it(&self) : int {
        return a + b;
    }

}

func test_extension_functions() {
    test("extension functions of base structs can be called on extended structs", () => {
        var v = ExtFuncTestVertex {
            ExtFuncTestPoint : ExtFuncTestPoint {
                a : 23,
                b : 8
            },
            c : 2
        }
        return v.sum_ext_func_test_point() == 31
    })
    test("generic extension functions on interfaces work - 1", () => {
        var p = SumItExtFuncTestOne {}
        // TODO cannot deduce generic type here
        return p.sum_gen_ext_func_inter_twice<SumItExtFuncTestOne>() == 4
    })
    test("generic extension functions on interfaces work - 2", () => {
        var p = SumItExtFuncTestTwo { a : 2, b : 3 }
        // TODO cannot deduce generic type here
        return p.sum_gen_ext_func_inter_twice<SumItExtFuncTestTwo>() == 10
    })
}