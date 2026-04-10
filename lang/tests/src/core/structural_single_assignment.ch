
impl core::ops::AddAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func add_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a += rhs.a;
        b += rhs.b;
    }
}
impl core::ops::SubAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func sub_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a -= rhs.a;
        b -= rhs.b;
    }
}
impl core::ops::MulAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func mul_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a *= rhs.a;
        b *= rhs.b;
    }
}
impl core::ops::DivAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func div_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a /= rhs.a;
        b /= rhs.b;
    }
}
impl core::ops::RemAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func rem_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a %= rhs.a;
        b %= rhs.b;
    }
}
impl core::ops::BitAndAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func bitand_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a &= rhs.a
        b &= rhs.b
    }
}
impl core::ops::BitOrAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func bitor_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a |= rhs.a
        b |= rhs.b
    }
}
impl core::ops::BitXorAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func bitxor_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a ^= rhs.a
        b ^= rhs.b
    }
}
impl core::ops::ShlAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func shl_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a <<= rhs.a
        b <<= rhs.b
    }
}
impl core::ops::ShrAssign<&StructSingleAssignmentTestStruct> for StructSingleAssignmentTestStruct {
    func shr_assign(&mut self, rhs: &StructSingleAssignmentTestStruct) {
        a >>= rhs.a
        b >>= rhs.b
    }
}

struct StructSingleAssignmentTestStruct {

    var a : int
    var b : int

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
    test("bitand single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 6, b : 14 }  // 0110, 1110
        var s = StructSingleAssignmentTestStruct { a : 3, b : 3 }   // 0011
        p &= s
        return p.a == 2 && p.b == 2             // 0110&0011=0010, 1110&0011=0010
    })
    test("bitor single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 6, b : 14 }  // 0110, 1110
        var s = StructSingleAssignmentTestStruct { a : 1, b : 1 }   // 0001
        p |= s
        return p.a == 7 && p.b == 15            // 0110|0001=0111, 1110|0001=1111
    })
    test("bitxor single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 6, b : 14 }  // 0110, 1110
        var s = StructSingleAssignmentTestStruct { a : 7, b : 7 }   // 0111
        p ^= s
        return p.a == 1 && p.b == 9             // 0110^0111=0001, 1110^0111=1001
    })
    test("shl single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 3, b : 5 }
        var s = StructSingleAssignmentTestStruct { a : 2, b : 2 }
        p <<= s
        return p.a == 12 && p.b == 20           // 3<<2=12, 5<<2=20
    })
    test("shr single structural assignment operator overload works", () => {
        var p = StructSingleAssignmentTestStruct { a : 20, b : 40 }
        var s = StructSingleAssignmentTestStruct { a : 2, b : 2 }
        p >>= s
        return p.a == 5 && p.b == 10            // 20>>2=5, 40>>2=10
    })
}