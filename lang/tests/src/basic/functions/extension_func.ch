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
}