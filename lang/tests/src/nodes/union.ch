union IntFloatUnion {
    var a : int;
    var b : float;
}

union TwoStructs {
    struct {
        var data : *int
        var length : bigint;
    } First;
    struct {
        var data : int;
        var length : int;
    } Second;
};

union ShortFloatUnion {

    var a : short
    var b : float

    func give_short(&self) : short {
        return a;
    }

    func give_float(&self) : float {
        return b;
    }

}

func (u : &mut ShortFloatUnion) sfu_short_plus() : short {
    return u.a + 2;
}

func (u : &mut ShortFloatUnion) sfu_float_plus() : float {
    return u.b + 3.0f;
}

func test_unions() {
    test("int float union works - 1", () => {
        var u : IntFloatUnion
        u.a = 5;
        return u.a == 5;
    })
    test("int float union works - 2", () => {
        var u : IntFloatUnion
        u.b = 1.2f;
        return u.b == 1.2f;
    })
    test("a complex union of two structs - 1", () => {
        var tu : TwoStructs
        var y = 55;
        tu.First.data = &y;
        tu.First.length = 345678;
        return *tu.First.data == 55 && tu.First.length == 345678;
    })
    test("a complex union of two structs - 2", () => {
        var tu : TwoStructs
        tu.Second.data = 123;
        tu.Second.length = 456;
        return tu.Second.data == 123 && tu.Second.length == 456;
    })
    test("union can be initialized just like structs", () => {
        var u = IntFloatUnion { a : 20 }
        return u.a == 20;
    })
    test("union can be initialized just like structs", () => {
        var u = IntFloatUnion { b : 3.4f }
        return u.b == 3.4f;
    })
    test("unions work with functions - 1", () => {
        var u = ShortFloatUnion { a : 33 }
        return u.give_short() == 33
    })
    test("unions work with functions - 2", () => {
        var u = ShortFloatUnion { b : 32.0f }
        return u.give_float() == 32.0f
    })
    test("extension methods work on unions - 1", () => {
        var u = ShortFloatUnion { a : 33 }
        return u.sfu_short_plus() == 35
    })
    test("extension methods work on unions - 2", () => {
        var u = ShortFloatUnion { b : 32.0f }
        return u.sfu_float_plus() == 35.0f
    })
}