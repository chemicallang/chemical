import "../test.ch"

union IntDoubleUnion {
    var a : int;
    var b : double;
}

union TwoStructs {
    struct {
        var data : int*
        var length : bigint;
    } First;
    struct {
        var data : int;
        var length : int;
    } Second;
};

func test_unions() {
    test("test that int float union works - 1", () => {
        var u : IntDoubleUnion
        u.a = 5;
        return u.a == 5;
    })
    test("test that int float union works - 2", () => {
        var u : IntDoubleUnion
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
}