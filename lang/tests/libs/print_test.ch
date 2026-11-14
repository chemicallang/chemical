// Console printing and expressive-string formatting.
//
// `print`/`println` render through `StringStream`, so these assertions cover
// exactly the bytes the console functions emit. Everything is checked as a
// string, so a failure names the rendering that changed.

using namespace std;

struct PrintFixture {
    var x : int
    var label : str

    func stream(&self, s : &mut StringStream) {
        unsafe { s.writeStr(label.data(), label.size()) }
        s.writeChar('=')
        s.writeSigned(x as bigint)
    }
}

func expect_render(env : &mut TestEnv, actual : &string, expected : &string_view) {
    if(!actual.to_view().equals(expected)) {
        env.error("unexpected rendering (see the got/want lines above)")
        println(`  got  [${actual}]`)
        println(`  want [${expected}]`)
    }
}

@test
func print_scalars(env : &mut TestEnv) {
    var b = true
    var f = false
    var i : int = -42
    var u : u64 = 18446744073709551615u64 as u64
    var c : char = 'Q'

    var s = string()
    s.append_expr(`${b} ${f} ${i} ${u} ${c}`)
    const expected = string_view("true false -42 18446744073709551615 Q")
    expect_render(env, &s, &expected)
}

@test
func print_integers_all_widths(env : &mut TestEnv) {
    var i8v : i8 = -128i8 as i8
    var i16v : i16 = -32768i16 as i16
    var i32v : i32 = -2147483648i32 as i32
    var i64v : i64 = -9223372036854775807i64 as i64
    var u8v : u8 = 255u8 as u8
    var u16v : u16 = 65535u16 as u16
    var u32v : u32 = 4294967295u32 as u32

    var s = string()
    s.append_expr(`${i8v} ${i16v} ${i32v} ${i64v} ${u8v} ${u16v} ${u32v}`)
    const expected = string_view("-128 -32768 -2147483648 -9223372036854775807 255 65535 4294967295")
    expect_render(env, &s, &expected)
}

@test
func print_strings_and_views(env : &mut TestEnv) {
    var owned = string("owned")
    var view = string_view("a view")
    var safe = str("utf8 ok")

    var s = string()
    s.append_expr(`[${owned}] [${view}] [${safe}]`)
    const expected = string_view("[owned] [a view] [utf8 ok]")
    expect_render(env, &s, &expected)
}

@test
func print_literal_has_known_length(env : &mut TestEnv) {
    var s = string()
    s.append_expr(`lit=${"abc"} empty=${""}`)
    const expected = string_view("lit=abc empty=")
    expect_render(env, &s, &expected)
}

@test
func print_double_shortest(env : &mut TestEnv) {
    var half = 0.5
    var third = 1.0 / 3.0
    var big = 1000000000000000000000000000000.0
    var tiny = 0.00000000000000000001
    var one = 1.0
    var zero = 0.0
    var nan = 0.0 / 0.0
    var inf = 1.0 / 0.0
    var ninf = -1.0 / 0.0

    var s = string()
    s.append_expr(`${one} ${zero} ${half} ${third} ${big} ${tiny} ${nan} ${inf} ${ninf}`)
    // 15 significant digits, trailing zeros trimmed, scientific outside
    // [1e-4, 1e15); nan/inf spell themselves.
    const expected = string_view("1 0 0.5 0.333333333333333 1e30 1e-20 nan inf -inf")
    expect_render(env, &s, &expected)
}

@test
func print_double_negative_zero_keeps_sign(env : &mut TestEnv) {
    var negz = -0.0
    var s = string()
    s.append_expr(`${negz}`)
    const expected = string_view("-0")
    expect_render(env, &s, &expected)
}

@test
func print_double_explicit_precision(env : &mut TestEnv) {
    var s = string()
    s.append_double(1.0, 2)
    s.append(' ')
    s.append_double(0.5, 2)
    s.append(' ')
    s.append_double(-2.5, 2)
    s.append(' ')
    s.append_double(0.0, 2)
    const expected = string_view("1.00 0.50 -2.50 0.00")
    expect_render(env, &s, &expected)
}

@test
func print_double_large_and_small_are_finite_text(env : &mut TestEnv) {
    // Regression: these used to render as garbage (`9223372036854775.807`)
    // because the integer part overflowed while scaling.
    var s = string()
    s.append_double(123456789012345678.0, -1)
    s.append(' ')
    s.append_double(123456789.123456, -1)
    s.append(' ')
    s.append_double(100000000000000.0, -1)
    const expected = string_view("1.23456789012346e17 123456789.123456 100000000000000")
    expect_render(env, &s, &expected)
}

@test
func print_user_type_hook(env : &mut TestEnv) {
    var p = PrintFixture { x : 7, label : str("nodes") }
    var s = string()
    s.append_expr(`count ${p}`)
    const expected = string_view("count nodes=7")
    expect_render(env, &s, &expected)
}

@test
func print_multiple_appends_accumulate(env : &mut TestEnv) {
    var s = string()
    s.append_expr(`a=${1}`)
    s.append_expr(` b=${2}`)
    const expected = string_view("a=1 b=2")
    expect_render(env, &s, &expected)
}

@test
func print_console_functions_emit(env : &mut TestEnv) {
    // These must not silently do nothing: they render and write in one call.
    // The lines below are expected in the suite output.
    println(`print-smoke ${1} + ${2} = ${3}`)
    eprintln(`eprint-smoke ${4}`)
}
