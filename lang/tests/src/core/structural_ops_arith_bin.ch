struct StructuralArithBinOpStruct : core::ops::Add<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Sub<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Mul<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Div<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Rem<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::BitAnd<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::BitOr<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::BitXor<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Shl<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::Shr<StructuralArithBinOpStruct, StructuralArithBinOpStruct>,
    core::ops::PartialEq<&StructuralArithBinOpStruct> {

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

    @override
    func bitand(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a & rhs.a,
            b : b & rhs.b
        }
    }

    @override
    func bitor(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a | rhs.a,
            b : b | rhs.b
        }
    }

    @override
    func bitxor(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a ^ rhs.a,
            b : b ^ rhs.b
        }
    }

    @override
    func shl(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a << rhs.a,
            b : b << rhs.b
        }
    }

    @override
    func shr(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a >> rhs.a,
            b : b >> rhs.b
        }
    }

    @override
    func eq(&self, other : &StructuralArithBinOpStruct) : bool {
        return a == other.a && b == other.b
    }

    @override
    func ne(&self, other : &StructuralArithBinOpStruct) : bool {
        return a != other.a && b != other.b
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
    test("bitand operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 6, b : 14 }   // 0110, 1110
        var b = StructuralArithBinOpStruct { a : 3, b : 3 }   // 0011, 0011
        var c = a & b
        return c.a == (a.a & b.a) && c.b == (a.b & b.b)       // 2, 2
    })
    test("bitor operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 6, b : 14 }   // 0110, 1110
        var b = StructuralArithBinOpStruct { a : 1, b : 1 }   // 0001, 0001
        var c = a | b
        return c.a == (a.a | b.a) && c.b == (a.b | b.b)       // 7, 15
    })
    test("bitxor operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 6, b : 14 }   // 0110, 1110
        var b = StructuralArithBinOpStruct { a : 7, b : 7 }   // 0111, 0111
        var c = a ^ b
        return c.a == (a.a ^ b.a) && c.b == (a.b ^ b.b)       // 1, 9
    })
    test("shl operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 3, b : 5 }
        var b = StructuralArithBinOpStruct { a : 2, b : 2 }
        var c = a << b
        return c.a == (a.a << b.a) && c.b == (a.b << b.b)     // 12, 20
    })
    test("shr operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 20, b : 40 }
        var b = StructuralArithBinOpStruct { a : 2, b : 2 }
        var c = a >> b
        return c.a == (a.a >> b.a) && c.b == (a.b >> b.b)     // 5, 10
    })
    test("eq operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 20, b : 40 }
        var b = StructuralArithBinOpStruct { a : 20, b : 40 }
        var c = StructuralArithBinOpStruct { a : 33, b : 45 }
        return (a == b) && (a == c) == false && (b == c) == false
    })
    test("ne operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 20, b : 40 }
        var b = StructuralArithBinOpStruct { a : 20, b : 40 }
        var c = StructuralArithBinOpStruct { a : 34, b : 30 }
        return (a != b) == false && (a != c) == true && (b != c) == true
    })
}