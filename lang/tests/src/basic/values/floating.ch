import "/test.ch"

func test_floating_expr() {
    test("comparing floats", () => {
        return 5.5f == 5.5f;
    });
    test("initialized comparing floats", () => {
        var a = 5f
        var b = 5f
        return a == b;
    })
    // TODO this is failing
    // test("integers can be divided with floats", () => {
    //     var a = 10
    //     var b = 2f
    //     return a / b == 5f
    // })
    test("integers can be divided with doubles", () => {
        var a = 10
        var b = 2.0
        return a / b == 5.0
    })
    test("comparing float expr with fp", () => {
        return (4.0f + 1.5f) == 5.5f;
    });
    test("comparing float with int expr with fp", () => {
        return (4 + 1.5f) == 5.5f;
    });
    test("comparing float with referenced fp", () => {
        var result = 4.0f + 1.5f;
        return result == 5.5f;
    });
    test("comparing float with referenced int expr", () => {
        var result = 4 + 1.5f;
        return result == 5.5f;
    });
    test("comparing doubles", () => {
        return 5.5 == 5.5;
    });
    test("comparing double expr with double", () => {
        return (4.0 + 1.5) == 5.5;
    });
    test("comparing double with int expr with double", () => {
        return (4 + 1.5) == 5.5;
    });
    test("comparing double with referenced double", () => {
        var result = 4.0 + 1.5;
        return result == 5.5;
    });
    test("comparing double with referenced int expr", () => {
        var result = 4 + 1.5;
        return result == 5.5;
    });
    // TODO test when float is passed to var arg function, it's converted to a double first
}