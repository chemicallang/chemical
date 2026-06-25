impl core::ops::Add<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func add(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a + rhs.a,
            b : b + rhs.b
        }
    }
}
impl core::ops::Sub<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func sub(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a - rhs.a,
            b : b - rhs.b
        }
    }
}
impl core::ops::Mul<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func mul(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a * rhs.a,
            b : b * rhs.b
        }
    }
}
impl core::ops::Div<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func div(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a / rhs.a,
            b : b / rhs.b
        }
    }
}
impl core::ops::Rem<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func rem(&self, rhs : StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a % rhs.a,
            b : b % rhs.b
        }
    }
}
impl core::ops::BitAnd<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func bitand(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a & rhs.a,
            b : b & rhs.b
        }
    }
}
impl core::ops::BitOr<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func bitor(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a | rhs.a,
            b : b | rhs.b
        }
    }
}
impl core::ops::BitXor<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func bitxor(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a ^ rhs.a,
            b : b ^ rhs.b
        }
    }
}
impl core::ops::Shl<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func shl(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a << rhs.a,
            b : b << rhs.b
        }
    }
}
impl core::ops::Shr<StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func shr(&self, rhs: StructuralArithBinOpStruct) : StructuralArithBinOpStruct {
        return StructuralArithBinOpStruct {
            a : a >> rhs.a,
            b : b >> rhs.b
        }
    }
}
impl core::ops::PartialEq<&StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func eq(&self, other : &StructuralArithBinOpStruct) : bool {
        return a == other.a && b == other.b
    }

    func ne(&self, other : &StructuralArithBinOpStruct) : bool {
        return a != other.a && b != other.b
    }
}

impl core::ops::PartialOrd<core::ops::Ordering, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func partial_cmp(&self, other : &StructuralArithBinOpStruct) : core::ops::Option<core::ops::Ordering> {
        return core::ops::Option.None<core::ops::Ordering>()
    }
}

impl core::ops::Ord for StructuralArithBinOpStruct {
    func cmp(&self, other : &StructuralArithBinOpStruct) : core::ops::Ordering {
        // no time to write the compare
        if(a < other.a && b < other.b) {
            return core::ops::Ordering.Less
        }
        if(a == other.a && b == other.b) {
            return core::ops::Ordering.Equal
        }
        return core::ops::Ordering.Greater
    }
    // TODO: removing this default implementation doesn't work
    func lt(&self, other : &StructuralArithBinOpStruct) : bool {
        return cmp(other) == core::ops::Ordering.Less
    }

    // TODO: removing this default implementation doesn't work
    func lte(&self, other : &StructuralArithBinOpStruct) : bool {
        return cmp(other) in core::ops::Ordering.Less, core::ops::Ordering.Equal
    }

    // TODO: removing this default implementation doesn't work
    func gt(&self, other : &StructuralArithBinOpStruct) : bool {
        return cmp(other) == core::ops::Ordering.Greater
    }

    // TODO: removing this default implementation doesn't work
    func gte(&self, other : &StructuralArithBinOpStruct) : bool {
        return cmp(other) in core::ops::Ordering.Greater, core::ops::Ordering.Equal
    }
}
impl core::ops::Index<&StructuralArithBinOpStruct, StructuralArithBinOpStruct> for StructuralArithBinOpStruct {
    func index(&self, idx : &StructuralArithBinOpStruct) : &StructuralArithBinOpStruct {
        return idx;
    }
}
struct StructuralArithBinOpStruct {

    var a : int
    var b : int

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
        return (a == &b) && (a == &c) == false && (b == &c) == false
    })
    test("ne operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 20, b : 40 }
        var b = StructuralArithBinOpStruct { a : 20, b : 40 }
        var c = StructuralArithBinOpStruct { a : 34, b : 30 }
        return (a != &b) == false && (a != &c) == true && (b != &c) == true
    })
    test("lt operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 10, b : 20 }
        var b = StructuralArithBinOpStruct { a : 15, b : 25 }
        var c = StructuralArithBinOpStruct { a : 5,  b : 10 }
        return (a < &b) && (c < &a) && !(b < &a)
    })
    test("le operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 10, b : 20 }
        var b = StructuralArithBinOpStruct { a : 10, b : 20 }
        var c = StructuralArithBinOpStruct { a : 15, b : 25 }
        return (a <= &b) && (a <= &c) && !(c <= &a)
    })
    test("gt operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 30, b : 40 }
        var b = StructuralArithBinOpStruct { a : 20, b : 25 }
        var c = StructuralArithBinOpStruct { a : 10, b : 15 }
        return (a > &b) && (b > &c) && !(c > &a)
    })
    test("ge operator with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 50, b : 60 }
        var b = StructuralArithBinOpStruct { a : 50, b : 60 }
        var c = StructuralArithBinOpStruct { a : 40, b : 50 }
        return (a >= &b) && (a >= &c) && !(c >= &a)
    })
    test("index operator overloaded with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 21, b : 87 }
        var b = StructuralArithBinOpStruct { a : 67, b : 98 }
        return a[b].a == 67 && a[b].b == 98
    })
    test("index operator overloaded with structure type works", () => {
        var a = StructuralArithBinOpStruct { a : 21, b : 87 }
        var b = StructuralArithBinOpStruct { a : 67, b : 98 }
        return a[a].a == 21 && a[a].b == 87
    })
}