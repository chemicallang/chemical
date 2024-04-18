func test_floating_expr() {
    test("comparing floating points", []() => {
        return 5.5 == 5.5;
    });
    test("comparing floating point expr with fp", []() => {
        return (4.0 + 1.5) == 5.5;
    });
    test("comparing floating point with int expr with fp", []() => {
        return (4 + 1.5) == 5.5;
    });
    test("comparing floating point with referenced fp", []() => {
        var result = 4.0 + 1.5;
        return result == 5.5;
    });
    test("comparing floating point with referenced int expr", []() => {
        var result = 4 + 1.5;
        return result == 5.5;
    });
}