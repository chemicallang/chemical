impl core::ops::AddAssign<int> for PrimSingleAssignmentTestStruct {
    func add_assign(&mut self, rhs: int) {
        a += rhs;
        b += rhs;
    }
}

impl core::ops::SubAssign<int> for PrimSingleAssignmentTestStruct {
    func sub_assign(&mut self, rhs: int) {
        a -= rhs;
        b -= rhs;
    }
}
impl core::ops::MulAssign<int> for PrimSingleAssignmentTestStruct {
    func mul_assign(&mut self, rhs: int) {
        a *= rhs;
        b *= rhs;
    }
}
impl core::ops::DivAssign<int> for PrimSingleAssignmentTestStruct {
    func div_assign(&mut self, rhs: int) {
        a /= rhs;
        b /= rhs;
    }
}
impl core::ops::RemAssign<int> for PrimSingleAssignmentTestStruct {
    func rem_assign(&mut self, rhs: int) {
        a %= rhs;
        b %= rhs;
    }
}
impl core::ops::BitAndAssign<int> for PrimSingleAssignmentTestStruct {
    func bitand_assign(&mut self, rhs: int) {
        a &= rhs
        b &= rhs
    }
}
impl core::ops::BitOrAssign<int> for PrimSingleAssignmentTestStruct {
    func bitor_assign(&mut self, rhs: int) {
        a |= rhs
        b |= rhs
    }
}
impl core::ops::BitXorAssign<int> for PrimSingleAssignmentTestStruct {
    func bitxor_assign(&mut self, rhs: int) {
        a ^= rhs
        b ^= rhs
    }
}
impl core::ops::ShlAssign<int> for PrimSingleAssignmentTestStruct {
    func shl_assign(&mut self, rhs: int) {
        a <<= rhs
        b <<= rhs
    }
}
impl core::ops::ShrAssign<int> for PrimSingleAssignmentTestStruct {
   func shr_assign(&mut self, rhs: int) {
        a >>= rhs
        b >>= rhs
    }
}
struct PrimSingleAssignmentTestStruct {

    var a : int
    var b : int

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