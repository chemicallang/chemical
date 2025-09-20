struct PrimSingleAssignmentTestStruct : core::ops::AddAssign<int>, core::ops::SubAssign<int>, core::ops::MulAssign<int>, core::ops::DivAssign<int>, core::ops::RemAssign<int> {

    var a : int
    var b : int

    @override
    func add_assign(&mut self, rhs: int) {
        a += rhs;
        b += rhs;
    }

    @override
    func sub_assign(&mut self, rhs: int) {
        a -= rhs;
        b -= rhs;
    }

    @override
    func mul_assign(&mut self, rhs: int) {
        a *= rhs;
        b *= rhs;
    }

    @override
    func div_assign(&mut self, rhs: int) {
        a /= rhs;
        b /= rhs;
    }

    @override
    func rem_assign(&mut self, rhs: int) {
        a %= rhs;
        b %= rhs;
    }

}

func test_primitive_single_assignment() {
    test("add single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 10, b : 15 }
        p += 5
        return p.a == 15 && p.b == 20
    })
    test("sub single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 10, b : 15 }
        p -= 5
        return p.a == 5 && p.b == 10
    })
    test("mul single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 10, b : 15 }
        p *= 5
        return p.a == 50 && p.b == 75
    })
    test("div single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 10, b : 15 }
        p /= 5
        return p.a == 2 && p.b == 3
    })
    test("rem single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 11, b : 15 }
        p %= 5
        return p.a == 1 && p.b == 0
    })
}