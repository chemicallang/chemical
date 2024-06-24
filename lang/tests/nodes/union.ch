import "../test.ch"

union IntFloatUnion {
    int a;
    float b;
}

func test_unions() {
    test("test that int float union works - 1", () => {
        var u = IntFloatUnion {}
        u.a = 5;
        return u.a == 5;
    })
    test("test that int float union works - 2", () => {
        var u = IntFloatUnion {}
        u.b = 1.2;
        return u.b == 1.2;
    })
}