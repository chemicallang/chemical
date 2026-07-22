// ═══════════════════════════════════════════════════════════════
// C codegen edge case: negative i8/i16/i32 literal minimum values
// The C codegen must not produce "--128" / "--32768" / "--2147483648"
// when writing e.g. -32768i16, because (int16_t)32768 overflows to -32768
// and combining with the '-' from VisitNegativeValue gives --32768
// ═══════════════════════════════════════════════════════════════

// ── i8: min value = -128 ──

@test
public func neg_i8_min_value(env : &mut TestEnv) {
    var x : i8 = -128i8
    if(x != -128i8) { env.error("i8 min value should be -128") }
}

@test
public func neg_i8_non_overflow_arith(env : &mut TestEnv) {
    var x : i8 = -128i8
    var y : i8 = 127i8
    var sum = x + y
    if(sum != -1i8) { env.error("(-128) + 127 should be -1") }
}

@test
public func neg_i8_comparison(env : &mut TestEnv) {
    if(!(-128i8 < -127i8)) { env.error("-128 should be less than -127") }
    if(!(-128i8 == -128i8)) { env.error("-128 should equal itself") }
    if(-128i8 == -127i8) { env.error("-128 should not equal -127") }
}

// ── i16: min value = -32768 (the specific bug trigger) ──

@test
public func neg_i16_min_value(env : &mut TestEnv) {
    var x : i16 = -32768i16
    if(x != -32768i16) { env.error("i16 min value should be -32768") }
}

@test
public func neg_i16_copy(env : &mut TestEnv) {
    var x : i16 = -32768i16
    var y : i16 = x
    if(y != -32768i16) { env.error("copy of -32768 should be -32768") }
}

@test
public func neg_i16_non_overflow_arith(env : &mut TestEnv) {
    var x : i16 = -32768i16
    var y : i16 = 1i16
    var sum = x + y
    if(sum != -32767i16) { env.error("(-32768) + 1 should be -32767") }
}

@test
public func neg_i16_comparison(env : &mut TestEnv) {
    if(!(-32768i16 < 0i16)) { env.error("-32768 should be < 0") }
    if(-32768i16 == 0i16) { env.error("-32768 != 0") }
}

@test
public func neg_i16_array_init(env : &mut TestEnv) {
    var arr : [3]i16 = [-1i16, -32768i16, 1i16]
    if(arr[1] != -32768i16) { env.error("arr[1] should be -32768") }
}

@test
public func neg_i16_in_expression(env : &mut TestEnv) {
    var result : i16 = (-32768i16 + 32767i16) + (-1i16)
    if(result != -2i16) { env.error("(-32768 + 32767) + (-1) should be -2") }
}

@test
public func neg_i16_through_var(env : &mut TestEnv) {
    var a : i16 = -32768i16
    var b : i16 = 100i16
    var c : i16 = 200i16
    var result = a + b - c
    if(result != -32668i16) { env.error("-32768 + 100 - 200 should be -32668") }
}

@test
public func neg_i16_multiple_negations(env : &mut TestEnv) {
    var x : i16 = -(-(-(-32768i16)))
    if(x != -32768i16) { env.error("-(-(-(-32768))) should be -32768") }
}

@test
public func neg_i16_conditional_reference(env : &mut TestEnv) {
    var check : bool
    if(-32768i16 < 30000i16) { check = true } else { check = false }
    if(!check) { env.error("-32768 should be < 30000") }
}

// ── i32: min value = -2147483648 ──

@test
public func neg_i32_min_value(env : &mut TestEnv) {
    var x : i32 = -2147483648i32
    if(x != -2147483648i32) { env.error("i32 min value should be -2147483648") }
}

@test
public func neg_i32_non_overflow_arith(env : &mut TestEnv) {
    var x : i32 = -2147483648i32
    var sum = x + 1i32
    if(sum != -2147483647i32) { env.error("(-2147483648) + 1 should be -2147483647") }
}

@test
public func neg_i32_comparison(env : &mut TestEnv) {
    if(!(-2147483648i32 < 0i32)) { env.error("min i32 < 0") }
    if(-2147483648i32 > 0i32) { env.error("min i32 > 0 should be false") }
}

@test
public func neg_i32_positive_range(env : &mut TestEnv) {
    var x : i32 = -(-2147483647i32)
    if(x != 2147483647i32) { env.error("-(-2147483647) = 2147483647") }
}

// ── Mixed types: casts between sizes ──

@test
public func neg_casts_between_types(env : &mut TestEnv) {
    var a : i8 = -128i8
    var b : i16 = a as i16
    if(b != -128i16) { env.error("i8 -128 cast to i16 should be -128") }

    var c : i16 = -32768i16
    var d : i32 = c as i32
    if(d != -32768i32) { env.error("i16 -32768 cast to i32 should be -32768") }
}

@test
public func neg_i16_used_with_unsigned(env : &mut TestEnv) {
    var x : i16 = -32768i16
    var y : i16 = 32767i16
    var diff = y - x
    // 32767 - (-32768) = 65535
    if(diff != -1i16) { env.error("32767 - (-32768) should overflow i16 to -1") }
}

// ── Loop with negative values ──

@test
public func neg_i16_in_loop(env : &mut TestEnv) {
    var sum : i16 = 0i16
    var i : i16 = -3i16
    while(i < 3i16) {
        sum = sum + i
        i = i + 1i16
    }
    if(sum != -3i16) { env.error("sum of -3..2 should be -3") }
}
