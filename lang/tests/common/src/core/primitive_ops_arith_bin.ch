impl core::ops::Add<int, int> for PrimArithBinOpStruct {
    func add(&self, rhs : int) : int {
        return a + rhs
    }
}

impl core::ops::Sub<int, int> for PrimArithBinOpStruct {
    func sub(&self, rhs : int) : int {
        return b - rhs
    }
}

impl core::ops::Mul<int, int> for PrimArithBinOpStruct {
    func mul(&self, rhs : int) : int {
        return a * rhs
    }
}

impl core::ops::Div<int, int> for PrimArithBinOpStruct {
    func div(&self, rhs : int) : int {
        return b / rhs
    }
}

impl core::ops::Rem<int, int> for PrimArithBinOpStruct {
    func rem(&self, rhs : int) : int {
        return a % rhs;
    }
}

impl core::ops::BitAnd<int, int> for PrimArithBinOpStruct {
    func bitand(&self, rhs: int) : int {
        return b & rhs;
    }
}

impl core::ops::BitOr<int, int> for PrimArithBinOpStruct {
    func bitor(&self, rhs: int) : int {
        return a | rhs;
    }
}

impl core::ops::BitXor<int, int> for PrimArithBinOpStruct {
    func bitxor(&self, rhs: int) : int {
        return b ^ rhs;
    }
}

impl core::ops::Shl<int, int> for PrimArithBinOpStruct {
    func shl(&self, rhs: int) : int {
        return a << rhs;
    }
}

impl core::ops::Shr<int, int> for PrimArithBinOpStruct {
    func shr(&self, rhs: int) : int {
        return b >> rhs;
    }
}

impl core::ops::PartialEq<int> for PrimArithBinOpStruct {
    func eq(&self, other : int) : bool {
        return a == other;
    }
    func ne(&self, other : int) : bool {
        return b != other;
    }
}

impl core::ops::Index<int, int> for PrimArithBinOpStruct {
    func index(&self, idx : int) : &int {
        if(idx == 0) {
            return take_ref(&a)
        } else {
            return take_ref(&b)
        }
    }
}

struct PrimArithBinOpStruct {

    var a : int
    var b : int

    // TODO: can't take reference of the struct member directly (bug)
    func take_ref(ptr : &int) : &int {
        return ptr
    }

}

func assign_mut_ref_op_overloaded(ref : &mut int) {
    *ref = 834
}

func test_arithmetic_bin_op_with_primitive() {
    test("add operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 13, b : 6 }
        var d = s + 3
        return d == 16
    })
    test("sub operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 13, b : 8 }
        var d = s - 4
        return d == 4
    })
    test("mul operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 7, b : 3 }
        var d = s * 7
        return d == 49
    })
    test("div operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 7, b : 36 }
        var d = s / 6
        return d == 6
    })
    test("mod operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 50, b : 9 }
        var d = s % 7
        return d == 1
    })
    test("bitand operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 13, b : 14 }  // b = 1110
        var d = s & 3                                    // 0011
        return d == 2                                    // 1110 & 0011 = 0010
    })
    test("bitor operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 6, b : 14 }   // a = 0110
        var d = s | 9                                    // 1001
        return d == 15                                   // 0110 | 1001 = 1111
    })
    test("bitxor operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 6, b : 14 }   // b = 1110
        var d = s ^ 5                                    // 0101
        return d == 11                                   // 1110 ^ 0101 = 1011
    })
    test("shl operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 3, b : 9 }
        var d = s << 2
        return d == 12                                   // 3 << 2 = 12
    })
    test("shr operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 100, b : 40 }
        var d = s >> 3
        return d == 5                                    // 40 >> 3 = 5
    })
    test("eq operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 50, b : 9 }
        return s == 50 && (s == 9) == false
    })
    test("eq operator with primitive type works", () => {
        var s = PrimArithBinOpStruct { a : 50, b : 9 }
        return (s != 9) == false && (s != 10) == true
    })
    test("index operator overload with primitive type works - 1", () => {
        var s = PrimArithBinOpStruct { a : 50, b : 9 }
        return s[0] == 50
    })
    test("index operator overload with primitive type works - 2", () => {
        var s = PrimArithBinOpStruct { a : 50, b : 9 }
        return s[1] == 9
    })
    test("index operator overload in assignment with primitive type works - 1", () => {
        var s = PrimArithBinOpStruct { a : 50, b : 9 }
        s[0] = 76
        return s.a == 76
    })
    test("index operator overload in assignment with primitive type works - 2", () => {
        var s = PrimArithBinOpStruct { a : 50, b : 9 }
        s[1] = 97
        return s.b == 97
    })
    test("index operator overload automatically sends a reference to reference types", () => {
        var s = PrimArithBinOpStruct { a : 50, b : 9 }
        assign_mut_ref_op_overloaded(&mut s[0])
        return s.a == 834
    })
}