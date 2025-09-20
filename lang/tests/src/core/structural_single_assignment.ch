struct StructSingleAssignmentTestStruct : core::ops::AddAssign<&StructSingleAssignmentTestStruct>,
    core::ops::SubAssign<&StructSingleAssignmentTestStruct>,
    core::ops::MulAssign<&StructSingleAssignmentTestStruct>,
    core::ops::DivAssign<&StructSingleAssignmentTestStruct>,
    core::ops::RemAssign<&StructSingleAssignmentTestStruct> {

    var a : int
    var b : int

    @override
    func add_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a += rhs.a;
        b += rhs.b;
    }

    @override
    func sub_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a -= rhs.a;
        b -= rhs.b;
    }

    @override
    func mul_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a *= rhs.a;
        b *= rhs.b;
    }

    @override
    func div_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a /= rhs.a;
        b /= rhs.b;
    }

    @override
    func rem_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a %= rhs.a;
        b %= rhs.b;
    }

}

func test_structural_single_assignment() {
    test("add single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 10, b : 15 }
        var s = StructSingleAssignmentTestStruct { a : 10, b : 15 }
        p += s
        return p.a == 20 && p.b == 30
    })
    test("sub single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 10, b : 15 }
        var s = StructSingleAssignmentTestStruct { a : 5, b : 10 }
        p -= s
        return p.a == 5 && p.b == 5
    })
    test("mul single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 10, b : 15 }
        var s = StructSingleAssignmentTestStruct { a : 2, b : 3 }
        p *= s
        return p.a == 20 && p.b == 45
    })
    test("div single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 10, b : 15 }
        var s = StructSingleAssignmentTestStruct { a : 2, b : 3 }
        p /= s
        return p.a == 5 && p.b == 5
    })
    test("rem single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 11, b : 15 }
        var s = StructSingleAssignmentTestStruct { a : 2, b : 3 }
        p %= s
        return p.a == 1 && p.b == 0
    })
}