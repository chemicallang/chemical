func test_string_append_expr() : bool {
    var world = "World"
    var greeting = std::string()
    greeting.append_expr(`Hello ${world}!`)
    var expected = std::string("Hello World!")
    return greeting.equals(&expected)
}

func test_string_make_from_expr() : bool {
    var world = "World"
    var greeting = std::string(`Hello ${world}!`)
    var expected = std::string("Hello World!")
    return greeting.equals(&expected)
}

func test_string_expr() {
    test("string from expressive string (append_expr)", test_string_append_expr)
    test("string from expressive string (constructor)", test_string_make_from_expr)
}
