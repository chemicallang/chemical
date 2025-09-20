struct StructuralArithBinOpStruct : core::ops::Add<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Sub<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Mul<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Div<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Rem<StructuralArithBinOpStruct, StructuralArithBinOpStruct> {

    var a : int
    var b : int

    @override
    func add(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a + rhs.a,
            b : b + rhs.b
        }
    }

    @override
    func sub(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a - rhs.a,
            b : b - rhs.b
        }
    }

    @override
    func mul(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a * rhs.a,
            b : b * rhs.b
        }
    }

    @override
    func div(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a / rhs.a,
            b : b / rhs.b
        }
    }

    @override
    func rem(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a % rhs.a,
            b : b % rhs.b
        }
    }

}

func test_arithmetic_bin_op_with_structure() {
    test("add operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 12, b : 65 }
        var b = StructuralArithBinOpStruct { a : 32, b : 65 }
        var c = a + b
        return c.a == (a.a + b.a) && c.b == (a.b + b.b)
    })
    test("sub operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 12, b : 65 }
        var b = StructuralArithBinOpStruct { a : 32, b : 65 }
        var c = a - b
        return c.a == (a.a - b.a) && c.b == (a.b - b.b)
    })
    test("mul operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 12, b : 65 }
        var b = StructuralArithBinOpStruct { a : 32, b : 65 }
        var c = a * b
        return c.a == (a.a * b.a) && c.b == (a.b * b.b)
    })
    test("div operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 12, b : 65 }
        var b = StructuralArithBinOpStruct { a : 32, b : 65 }
        var c = a / b
        return c.a == (a.a / b.a) && c.b == (a.b / b.b)
    })
    test("mod operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 12, b : 65 }
        var b = StructuralArithBinOpStruct { a : 32, b : 65 }
        var c = a % b
        return c.a == (a.a % b.a) && c.b == (a.b % b.b)
    })
}