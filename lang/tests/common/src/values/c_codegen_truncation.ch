// ═══════════════════════════════════════════════════════════════════════
// C codegen truncation cast regression tests
//
// These tests verify that the C codegen adds explicit truncation casts
// (intN_t) around expressions with narrow integer types (i8, i16)
// when the result is used in a comparison or as a return value.
//
// C integer promotion rules promote i8/i16 to int before any arithmetic.
// Without explicit truncation casts, overflow/wrapping behavior is lost.
// ═══════════════════════════════════════════════════════════════════════

func test_c_codegen_truncation() {

    // ── i8 inline truncation ──

    test("i8 inline add overflow", () => {
        var x : i8 = 100i8
        var y : i8 = 100i8
        // 100 + 100 = 200, wraps i8 to -56
        // Without (int8_t) cast: (int)200 != (int)(-56) → FAIL
        return (x + y) == -56i8
    })

    test("i8 inline sub wrap", () => {
        var x : i8 = -128i8
        var y : i8 = 1i8
        // -128 - 1 = -129, wraps i8 to 127
        return (x - y) == 127i8
    })

    // ── i16 inline truncation (the original bug) ──

    test("i16 inline sub from min", () => {
        var x : i16 = -32768i16
        // x - 1 = -32769, wraps i16 to 32767
        return (x - 1i16) == 32767i16
    })

    test("i16 inline add to max", () => {
        var x : i16 = 32767i16
        // x + 1 = 32768, wraps i16 to -32768
        return (x + 1i16) == -32768i16
    })

    test("i16 inline expression through var", () => {
        var a : i16 = -32768i16
        var b : i16 = 100i16
        var c : i16 = 200i16
        // -32768 + 100 - 200 = -32868, wraps i16 to 32668
        return (a + b - c) == 32668i16
    })

    // ── Multi-operation inline chains ──

    test("i16 inline mul overflow", () => {
        var x : i16 = 200i16
        var y : i16 = 200i16
        // 200 * 200 = 40000, wraps i16 to -25536
        return (x * y) == -25536i16
    })

    test("i16 inline chain mul overflow", () => {
        var x : i16 = 200i16
        var y : i16 = 200i16
        // (200 + 200) = 400, 400 * 200 = 80000, wraps i16 to 14464
        return ((x + y) * 200i16) == 14464i16
    })

    test("i16 inline double negation overflow", () => {
        // -(-32768) = 32768 which wraps i16 to -32768
        return (-(-32768i16)) == -32768i16
    })

    // ── Mixed type inline overflow chain ──

    test("i8 i16 inline overflow chain", () => {
        var a : i8 = 100i8
        var b : i8 = 100i8
        var c : i16 = 200i16
        // (100 + 100) overflows i8 to -56
        // Then -56 as i16 + 200 = 144
        // Without truncation: (int)(100+100) = 200, 200 + 200 = 400, != 144
        return ((a + b) as i16 + c) == 144i16
    })

}
