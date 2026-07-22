// ═══════════════════════════════════════════════════════════════════════
// Integer literal codegen edge case tests
//
// Verifies that signed integer literals at boundary values (INT_MIN, INT_MAX)
// are parsed and codegen'd correctly. These tests run in both compiled AND
// interpretation mode.
//
// Bug history:
// - `-32768i16`: (int16_t)32768 overflowed in C, then combining with '-'
//   gave '--32768' → maximal munch → syntax error
// - `-9223372036854775808i64`: strtoll rejected because 2^63 overflows signed long long
// ═══════════════════════════════════════════════════════════════════════

func test_int_edge_cases() {

    // ═══════════════════════════════════════════════════════
    // i8: min value = -128
    // ═══════════════════════════════════════════════════════

    test("neg i8 min value", () => {
        var x : i8 = -128i8
        return x == -128i8
    })

    test("neg i8 non overflow arith", () => {
        var x : i8 = -128i8
        var y : i8 = 127i8
        var sum = x + y
        return sum == -1i8
    })

    test("neg i8 comparison", () => {
        return (-128i8 < -127i8) && (-128i8 == -128i8) && (-128i8 != -127i8)
    })

    test("i8 bitwise not", () => {
        var x : i8 = 5i8
        return (~x) == -6i8
    })

    test("i8 bitwise not zero", () => {
        var x : i8 = 0i8
        return (~x) == -1i8
    })

    // ═══════════════════════════════════════════════════════
    // i16: min value = -32768
    // ═══════════════════════════════════════════════════════

    test("neg i16 min value", () => {
        var x : i16 = -32768i16
        return x == -32768i16
    })

    test("neg i16 copy", () => {
        var x : i16 = -32768i16
        var y : i16 = x
        return y == -32768i16
    })

    test("neg i16 non overflow arith", () => {
        var x : i16 = -32768i16
        var y : i16 = 1i16
        var sum = x + y
        return sum == -32767i16
    })

    test("neg i16 comparison", () => {
        return (-32768i16 < 0i16) && (-32768i16 != 0i16)
    })

    test("neg i16 array init", () => {
        var arr : [3]i16 = [-1i16, -32768i16, 1i16]
        return arr[1] == -32768i16
    })

    test("neg i16 in expression", () => {
        var result : i16 = (-32768i16 + 32767i16) + (-1i16)
        return result == -2i16
    })

    test("neg i16 through var", () => {
        var a : i16 = -32768i16
        var b : i16 = 100i16
        var c : i16 = 200i16
        var result = a + b - c
        // -32768 + 100 - 200 = -32868, wrapped to i16 = 32668
        return result == 32668i16
    })

    test("neg i16 multiple negations", () => {
        var x : i16 = -(-(-(-32768i16)))
        return x == -32768i16
    })

    test("neg i16 conditional reference", () => {
        var check : bool
        if(-32768i16 < 30000i16) { check = true } else { check = false }
        return check
    })

    test("i16 bitwise not", () => {
        var x : i16 = 5i16
        return (~x) == -6i16
    })

    test("i16 bitwise not zero", () => {
        var x : i16 = 0i16
        return (~x) == -1i16
    })

    // ═══════════════════════════════════════════════════════
    // i32: min value = -2147483648
    // ═══════════════════════════════════════════════════════

    test("neg i32 min value", () => {
        var x : i32 = -2147483648i32
        return x == -2147483648i32
    })

    test("neg i32 non overflow arith", () => {
        var x : i32 = -2147483648i32
        var sum = x + 1i32
        return sum == -2147483647i32
    })

    test("neg i32 comparison", () => {
        return (-2147483648i32 < 0i32) && (-2147483648i32 > 0i32 == false)
    })

    test("neg i32 positive range", () => {
        var x : i32 = -(-2147483647i32)
        return x == 2147483647i32
    })

    test("i32 bitwise not", () => {
        var x : i32 = 5i32
        return (~x) == -6i32
    })

    test("i32 bitwise not zero", () => {
        var x : i32 = 0i32
        return (~x) == -1i32
    })

    // ═══════════════════════════════════════════════════════
    // Cross-type: casts between sizes
    // ═══════════════════════════════════════════════════════

    test("neg casts between types", () => {
        var a : i8 = -128i8
        var b : i16 = a as i16
        var c : i16 = -32768i16
        var d : i32 = c as i32
        return b == -128i16 && d == -32768i32
    })

    test("neg i16 used with unsigned", () => {
        var x : i16 = -32768i16
        var y : i16 = 32767i16
        var diff = y - x
        // 32767 - (-32768) = 65535, overflows i16 to -1
        return diff == -1i16
    })

    // ═══════════════════════════════════════════════════════
    // Loop with negative values
    // ═══════════════════════════════════════════════════════

    test("neg i16 in loop", () => {
        var sum : i16 = 0i16
        var i : i16 = -3i16
        while(i < 3i16) {
            sum = sum + i
            i = i + 1i16
        }
        return sum == -3i16
    })

    // ═══════════════════════════════════════════════════════
    // Bitwise NOT on broader types (i32, i64 - no truncation issues)
    // ═══════════════════════════════════════════════════════

    test("i64 bitwise not", () => {
        var x : i64 = 5i64
        return (~x) == -6i64
    })

    test("i64 bitwise not zero", () => {
        var x : i64 = 0i64
        return (~x) == -1i64
    })

    test("bitwise not in comparison", () => {
        var x : i32 = ~5i32
        return x == -6i32 && (x == ~5i32)
    })

    test("bitwise not chain", () => {
        var a : i32 = 5i32
        var b : i32 = ~a
        var c : i32 = ~b
        return c == 5i32
    })
}
