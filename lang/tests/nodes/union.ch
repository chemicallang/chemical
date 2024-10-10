import "../test.ch"

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

func test_unions() {
    test("test that int float union works - 1", () => {
        var u : IntFloatUnion
        u.a = 5;
        return u.a == 5;
    })
    test("test that int float union works - 2", () => {
        var u : IntFloatUnion
        u.b = 1.2f;
        return u.b == 1.2f;
    })
    test("test a complex union of two structs - 1", () => {
        var tu : TwoStructs
        var y = 55;
        tu.First.data = &y;
        tu.First.length = 345678;
        return *tu.First.data == 55 && tu.First.length == 345678;
    })
    test("test a complex union of two structs - 2", () => {
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
}