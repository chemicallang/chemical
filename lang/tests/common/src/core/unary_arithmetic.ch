impl core::ops::Neg<int> for PrimUnaryArithTestStruct {
    func neg(&self) : int {
        return -a;
    }
}
impl core::ops::Not<int> for PrimUnaryArithTestStruct {
    func not(&self) : int {
        return -b;
    }
}
impl core::ops::Increment for PrimUnaryArithTestStruct {
    func inc_pre(&mut self) : &PrimUnaryArithTestStruct {
        a++
        b++
        return self
    }
    func inc_post(&mut self) : PrimUnaryArithTestStruct {
        var temp = PrimUnaryArithTestStruct {
            a : a,
            b : b
        }
        a++
        b++
        return temp;
    }
}
impl core::ops::Decrement for PrimUnaryArithTestStruct {
    func dec_pre(&mut self) : &PrimUnaryArithTestStruct {
        a--
        b--
        return self
    }
    func dec_post(&mut self) : PrimUnaryArithTestStruct {
        var temp = PrimUnaryArithTestStruct {
            a : a,
            b : b
        }
        a--
        b--
        return temp;
    }
}
struct PrimUnaryArithTestStruct {

    var a : int
    var b : int

}

impl core::ops::Neg<StructuralUnaryArithTestStruct> for StructuralUnaryArithTestStruct {
    func neg(self) : StructuralUnaryArithTestStruct {
        return StructuralUnaryArithTestStruct {
            a : -a,
            b : -b
        }
    }
}
impl core::ops::Not<StructuralUnaryArithTestStruct> for StructuralUnaryArithTestStruct {
    func not(self) : StructuralUnaryArithTestStruct {
        return StructuralUnaryArithTestStruct {
            a : a,
            b : -b
        }
    }
}

struct StructuralUnaryArithTestStruct {

    var a : int
    var b : int

}

func test_unary_arithmetic() {

    test("unary neg operator overload with primitive output type works", () => {
        var p = PrimUnaryArithTestStruct { a : 54, b : 32 }
        var x = -p
        return x == -54
    })
    test("unary neg operator overload with structural output type works", () => {
        var p = StructuralUnaryArithTestStruct { a : 23, b : 43 }
        var x = -p
        return x.a == -23 && x.b == -43
    })
    test("unary not operator overload with primitive output type works", () => {
        var p = PrimUnaryArithTestStruct { a : 54, b : 32 }
        var x = !p
        return x == -32
    })
    test("unary not operator overload with structural output type works", () => {
        var p = StructuralUnaryArithTestStruct { a : 23, b : 43 }
        var x = !p
        return x.a == 23 && x.b == -43
    })
    test("pre increment unary arithmetic operator overload works", () => {
        var p = PrimUnaryArithTestStruct { a : 25, b : 64 }
        var x = ++p
        return p.a == 26 && p.b == 65 && x.a == 26 && x.b == 65
    })
    test("pre decrement unary arithmetic operator overload works", () => {
        var p = PrimUnaryArithTestStruct { a : 25, b : 64 }
        var x = --p
        return p.a == 24 && p.b == 63 && x.a == 24 && x.b == 63
    })
    test("post increment unary arithmetic operator overload works", () => {
        var p = PrimUnaryArithTestStruct { a : 25, b : 64 }
        var x = p++
        return p.a == 26 && p.b == 65 && x.a == 25 && x.b == 64
    })
    test("post decrement unary arithmetic operator overload works", () => {
        var p = PrimUnaryArithTestStruct { a : 25, b : 64 }
        var x = p--
        return p.a == 24 && p.b == 63 && x.a == 25 && x.b == 64
    })
}