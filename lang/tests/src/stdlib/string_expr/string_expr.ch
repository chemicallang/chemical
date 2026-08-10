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

// ---- implicit conversion (expressive string -> std::string) ----

// var-init with explicit type annotation
func test_implicit_string_expr_var_init() : bool {
    var world = "World"
    var greeting : std::string = `Hello ${world}!`
    var expected = std::string("Hello World!")
    return greeting.equals(&expected)
}

// const var-init with explicit type annotation
func test_implicit_string_expr_const_var_init() : bool {
    var world = "World"
    const greeting : std::string = `Hello ${world}!`
    var expected = std::string("Hello World!")
    return greeting.equals(&expected)
}

// assignment to an already-typed variable
func test_implicit_string_expr_assignment() : bool {
    var greeting : std::string
    var world = "World"
    greeting = `Hello ${world}!`
    var expected = std::string("Hello World!")
    return greeting.equals(&expected)
}

func takes_std_string(s : std::string) : bool {
    var expected = std::string("Hello World!")
    return s.equals(&expected)
}

// function argument
func test_implicit_string_expr_func_arg() : bool {
    var world = "World"
    return takes_std_string(`Hello ${world}!`)
}

// return value
func make_greeting_from_expr(world : *char) : std::string {
    return `Hello ${world}!`
}

func test_implicit_string_expr_return() : bool {
    var g = make_greeting_from_expr("World")
    var expected = std::string("Hello World!")
    return g.equals(&expected)
}

variant StringExprVariant {
    SomeValue(s : std::string)
    func len(&self) : size_t {
        switch(self) {
            SomeValue(s) => { return s.size() }
        }
    }
}

// variant call
func test_implicit_string_expr_variant() : bool {
    var world = "World"
    var v = StringExprVariant.SomeValue(`Hello ${world}!`)
    return v.len() == 12
}

func greet_default(s : std::string = `Hello ${"World"}!`) : bool {
    var expected = std::string("Hello World!")
    return s.equals(&expected)
}

// default parameter value
func test_implicit_string_expr_default_arg() : bool {
    if(!greet_default()) { return false }
    var world = "Universe"
    return greet_default(`Hello ${world}!`) == false
}

struct StringExprFieldContainer {
    var s : std::string
}

// struct field init
func test_implicit_string_expr_struct_field() : bool {
    var world = "World"
    var c = StringExprFieldContainer { s : `Hello ${world}!` }
    var expected = std::string("Hello World!")
    return c.s.equals(&expected)
}

// array element
func test_implicit_string_expr_array() : bool {
    var world = "World"
    var arr : [2]std::string = [ `Hello ${world}!`, `Bye ${world}!` ]
    var exp1 = std::string("Hello World!")
    var exp2 = std::string("Bye World!")
    return arr[0].equals(&exp1) && arr[1].equals(&exp2)
}

func test_string_expr() {
    test("string from expressive string (append_expr)", test_string_append_expr)
    test("string from expressive string (constructor)", test_string_make_from_expr)
    test("implicit string from expressive string (var init)", test_implicit_string_expr_var_init)
    test("implicit string from expressive string (const var init)", test_implicit_string_expr_const_var_init)
    test("implicit string from expressive string (assignment)", test_implicit_string_expr_assignment)
    test("implicit string from expressive string (function argument)", test_implicit_string_expr_func_arg)
    test("implicit string from expressive string (return value)", test_implicit_string_expr_return)
    test("implicit string from expressive string (variant call)", test_implicit_string_expr_variant)
    test("implicit string from expressive string (default argument)", test_implicit_string_expr_default_arg)
    test("implicit string from expressive string (struct field init)", test_implicit_string_expr_struct_field)
    test("implicit string from expressive string (array element)", test_implicit_string_expr_array)
}
