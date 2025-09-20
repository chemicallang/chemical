struct PrimSingleAssignmentTestStruct : core::ops::AddAssign<int>,
    core::ops::SubAssign<int>,
    core::ops::MulAssign<int>,
    core::ops::DivAssign<int>,
    core::ops::RemAssign<int>,
    core::ops::BitAndAssign<int>,
    core::ops::BitOrAssign<int>,
    core::ops::BitXorAssign<int>,
    core::ops::ShlAssign<int>,
    core::ops::ShrAssign<int> {

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

    @override
    func bitand_assign(&mut self, rhs: int) {
        a &= rhs
        b &= rhs
    }

    @override
    func bitor_assign(&mut self, rhs: int) {
        a |= rhs
        b |= rhs
    }

    @override
    func bitxor_assign(&mut self, rhs: int) {
        a ^= rhs
        b ^= rhs
    }

    @override
    func shl_assign(&mut self, rhs: int) {
        a <<= rhs
        b <<= rhs
    }

    @override
    func shr_assign(&mut self, rhs: int) {
        a >>= rhs
        b >>= rhs
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
    test("bitand single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 6, b : 14 } // 6=0110, 14=1110
        p &= 3                                      // 3=0011
        return p.a == 2 && p.b == 2                 // 0110&0011=0010, 1110&0011=0010
    })
    test("bitor single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 6, b : 14 } // 6=0110, 14=1110
        p |= 1                                      // 1=0001
        return p.a == 7 && p.b == 15                // 0110|0001=0111, 1110|0001=1111
    })
    test("bitxor single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 6, b : 14 } // 6=0110, 14=1110
        p ^= 7                                      // 7=0111
        return p.a == 1 && p.b == 9                 // 0110^0111=0001, 1110^0111=1001
    })
    test("shl single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 3, b : 5 }
        p <<= 2
        return p.a == 12 && p.b == 20               // 3<<2=12, 5<<2=20
    })
    test("shr single primitive assignment operator overload works", () => {
        var p = PrimSingleAssignmentTestStruct { a : 20, b : 40 }
        p >>= 2
        return p.a == 5 && p.b == 10                // 20>>2=5, 40>>2=10
    })
}