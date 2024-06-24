import "../test.ch"

union IntFloatUnion {
    var a : int;
    var b : float;
}

func test_unions() {
    test("test that int float union works - 1", () => {
        var u : IntFloatUnion
        u.a = 5;
        return u.a == 5;
    })
    test("test that int float union works - 2", () => {
        var u : IntFloatUnion
        u.b = 1.2;
        return u.b == 1.2;
    })
}