// C default argument promotions at a variadic @extern boundary.
//
// `float` must become `double`; `i8`/`u8`/`bool` must become `int` with the
// value preserved. The C fixture reads the promoted types through va_arg.
@extern func check_vararg_promotions(count : int, _ : any...) : int

func test_variadic_default_promotions() {
    test("variadic default promotions", () => unsafe(check_vararg_promotions(4, 2.5f, (-7) as i8, true, 200 as u8)) == 1)
}
