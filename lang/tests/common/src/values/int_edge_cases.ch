// ═══════════════════════════════════════════════════════════════════════
// Integer literal codegen edge case tests
//
// Verifies that signed integer literals at boundary values (INT_MIN, INT_MAX)
// are parsed and codegen'd correctly. These tests run in both compiled AND
// interpretation mode.
//
// Bug history:
// - Test 326: `~5` wasn't codegen'd correctly (written as 4294967290 instead of -6)
// - `-32768i16`: (int16_t)32768 overflowed in C, then combining with '-'
//   gave '--32768' → maximal munch → syntax error
// - `-9223372036854775808i64`: strtoll rejected because 2^63 overflows signed long long
// ═══════════════════════════════════════════════════════════════════════

func test_int_edge_cases() {

    // ===================================================================
    // i8: range -128 to 127
    // ===================================================================

    test("int min i8 declaration", () => {
        var x : i8 = -128i8
        return x == -128i8
    })

    test("int max i8 declaration", () => {
        var x : i8 = 127i8
        return x == 127i8
    })

    test("i8 arithmetic range", () => {
        var min : i8 = -128i8
        var max : i8 = 127i8
        return min + max == -1i8
    })

    test("i8 negate positive", () => {
        var x : i8 = -127i8
        return x == -127i8 && (-x) == 127i8
    })

    test("i8 bitwise not", () => {
        var x : i8 = 5i8
        return (~x) == -6i8
    })

    test("i8 bitwise not zero", () => {
        var x : i8 = 0i8
        return (~x) == -1i8
    })

    test("i8 bitwise not max", () => {
        var x : i8 = 127i8
        return (~x) == -128i8
    })

    test("i8 comparison chain", () => {
        return (-128i8 < -64i8) && (-64i8 < 0i8) && (0i8 < 64i8) && (64i8 < 127i8) &&
            (-128i8 <= -128i8) && (127i8 >= 127i8)
    })

    // ===================================================================
    // i16: range -32768 to 32767
    // ===================================================================

    test("int min i16 declaration", () => {
        var x : i16 = -32768i16
        return x == -32768i16
    })

    test("int max i16 declaration", () => {
        var x : i16 = 32767i16
        return x == 32767i16
    })

    test("i16 copy min", () => {
        var x : i16 = -32768i16
        var y : i16 = x
        return y == -32768i16
    })

    test("i16 arithmetic range", () => {
        var x : i16 = -32768i16
        var y : i16 = 32767i16
        return x + y == -1i16
    })

    test("i16 negate positive", () => {
        var x : i16 = -32767i16
        return x == -32767i16 && (-x) == 32767i16
    })

    test("i16 single negation", () => {
        var x : i16 = -32768i16
        return x == -32768i16
    })

    test("i16 double negation", () => {
        var x : i16 = -(-32768i16)
        // -(-32768) = 32768 which overflows i16 to -32768
        return x == -32768i16
    })

    test("i16 triple negation", () => {
        var x : i16 = -(-(-32768i16))
        return x == -32768i16
    })

    test("i16 bitwise not", () => {
        var x : i16 = 5i16
        return (~x) == -6i16
    })

    test("i16 bitwise not zero", () => {
        var x : i16 = 0i16
        return (~x) == -1i16
    })

    test("i16 bitwise not max", () => {
        var x : i16 = 32767i16
        return (~x) == -32768i16
    })

    test("i16 bitwise not min", () => {
        var x : i16 = -32768i16
        return (~x) == 32767i16
    })

    test("i16 comparison min", () => {
        return (-32768i16 < -32767i16) && (-32768i16 != -32767i16) && (-32768i16 <= -32768i16)
    })

    test("i16 subtract from min", () => {
        var x : i16 = -32768i16
        return x - 1i16 == 32767i16
    })

    test("i16 add to max", () => {
        var x : i16 = 32767i16
        return x + 1i16 == -32768i16
    })

    // ===================================================================
    // i32: range -2147483648 to 2147483647
    // ===================================================================

    test("int min i32 declaration", () => {
        var x : i32 = -2147483648i32
        return x == -2147483648i32
    })

    test("int max i32 declaration", () => {
        var x : i32 = 2147483647i32
        return x == 2147483647i32
    })

    test("i32 arithmetic range", () => {
        var x : i32 = -2147483648i32
        var y : i32 = 2147483647i32
        return x + y == -1i32
    })

    test("i32 negate positive", () => {
        var x : i32 = -2147483647i32
        return (-x) == 2147483647i32
    })

    test("i32 double negation", () => {
        var x : i32 = -(-2147483648i32)
        // -(-2147483648) = 2147483648 which overflows i32 to -2147483648
        return x == -2147483648i32
    })

    test("i32 bitwise not", () => {
        var x : i32 = 5i32
        return (~x) == -6i32
    })

    test("i32 bitwise not zero", () => {
        var x : i32 = 0i32
        return (~x) == -1i32
    })

    test("i32 bitwise not max", () => {
        var x : i32 = 2147483647i32
        return (~x) == -2147483648i32
    })

    test("i32 comparison min", () => {
        return (-2147483648i32 < 0i32) && (-2147483648i32 <= -2147483648i32)
    })

    // ===================================================================
    // i64: range -9223372036854775808 to 9223372036854775807
    // ===================================================================

    test("i64 bitwise not", () => {
        var x : i64 = 5i64
        return (~x) == -6i64
    })

    test("i64 bitwise not zero", () => {
        var x : i64 = 0i64
        return (~x) == -1i64
    })

    // ===================================================================
    // Bitwise NOT (the original Test 326 bug)
    // ===================================================================

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

    test("bitwise not all bits", () => {
        return (~(-1i8) == 0i8) && (~(-1i16) == 0i16) && (~(-1i32) == 0i32) && (~(-1i64) == 0i64)
    })

    // ===================================================================
    // Mixed operations across types
    // ===================================================================

    test("cross type arithmetic", () => {
        var a : i8 = -128i8
        var b : i16 = a as i16
        var c : i16 = -32768i16
        var d : i32 = c as i32
        var e : i32 = -2147483648i32
        var f : i64 = e as i64
        return b == -128i16 && d == -32768i32 && f == -2147483648i64
    })

    test("negation chain in expression", () => {
        var a : i16 = -1i16
        var b : i16 = -(-(-(-a)))
        return b == 1i16
    })

    test("negate chain mixed", () => {
        var x : i16 = -(-(-1i16))
        var y : i16 = -(-(-(-1i16)))
        return x == -1i16 && y == 1i16
    })

    // ===================================================================
    // Loop with edge values
    // ===================================================================

    test("loop negative range", () => {
        var sum : i16 = 0i16
        var i : i16 = -5i16
        while(i < 5i16) {
            sum = sum + i
            i = i + 1i16
        }
        return sum == -5i16
    })

    test("loop near min", () => {
        var x : i16 = -32768i16
        var count : i16 = 0i16
        while(x < -32760i16) {
            count = count + 1i16
            x = x + 1i16
        }
        return count == 8i16
    })

}
