struct PrimUnaryArithTestStruct : core::ops::Neg<int>, core::ops::Increment, core::ops::Decrement {

    var a : int
    var b : int

    @override
    func neg(&self) : int {
        return -a;
    }

    @override
    func inc_pre(&mut self) : &Self {
        a++
        b++
        return self
    }

    @override
    func inc_post(&mut self) : Self {
        var temp = PrimUnaryArithTestStruct {
            a : a,
            b : b
        }
        a++
        b++
        return temp;
    }

    @override
    func dec_pre(&mut self) : &Self {
        a--
        b--
        return self
    }

    @override
    func dec_post(&mut self) : Self {
        var temp = PrimUnaryArithTestStruct {
            a : a,
            b : b
        }
        a--
        b--
        return temp;
    }

}

struct StructuralUnaryArithTestStruct : core::ops::Neg<StructuralUnaryArithTestStruct> {

    var a : int
    var b : int

    @override
    func neg(self) : StructuralUnaryArithTestStruct {
        return StructuralUnaryArithTestStruct {
            a : -a,
            b : -b
        }
    }

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