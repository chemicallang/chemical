func 加(a : int, b : int) : int {
    return a + b;
}

func multiply_unicode_params(数1 : int, 数2 : int) : int {
    return 数1 * 数2;
}

func foo_δ(x : int) : int {
    return x * 2;
}

func test_unicode() {
    test("unicode function name works", () => {
        var x = 7
        var y = 8
        return 加(x, y) == 15
    })
    test("unicode variable name works", () => {
        var α = 5
        var β = 6
        return α * β == 30
    })
    test("unicode parameter names work", () => {
        return multiply_unicode_params(2, 14) == 28
    })
    test("mixed ascii + unicode", () => {
        return foo_δ(9) == 18
    })
}