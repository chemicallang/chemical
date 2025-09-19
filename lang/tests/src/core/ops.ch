struct AddOpTestStruct : core::ops::Add<int, int>, core::ops::Sub<int, int>, core::ops::Mul<int, int>, core::ops::Div<int, int>, core::ops::Rem<int, int> {
    var a : int
    var b : int

    @override
    func add(&self, rhs : int) : int {
        return a + rhs
    }

    @override
    func sub(&self, rhs : int) : int {
        return b - rhs
    }

    @override
    func mul(&self, rhs : int) : int {
        return a * rhs
    }

    @override
    func div(&self, rhs : int) : int {
        return b / rhs
    }

    @override
    func rem(&self, rhs : int) : int {
        return a % rhs;
    }

}

func test_core_ops() {
    test("add operator with primitive type works", () => {
        var s = AddOpTestStruct { a : 13, b : 6 }
        var d = s + 3
        return d == 16
    })
    test("sub operator with primitive type works", () => {
        var s = AddOpTestStruct { a : 13, b : 8 }
        var d = s - 4
        return d == 4
    })
    test("mul operator with primitive type works", () => {
        var s = AddOpTestStruct { a : 7, b : 3 }
        var d = s * 7
        return d == 49
    })
    test("mul operator with primitive type works", () => {
        var s = AddOpTestStruct { a : 7, b : 36 }
        var d = s / 6
        return d == 6
    })
    test("mul operator with primitive type works", () => {
        var s = AddOpTestStruct { a : 50, b : 9 }
        var d = s % 7
        return d == 1
    })
}