// ═══════════════════════════════════════════════════════════════════════
// C codegen edge case tests for signed integer literals
//
// These tests verify that the C codegen correctly handles edge cases
// like min/max signed integer values, chained negations, bitwise NOT,
// and mixed operations — ensuring no invalid C is produced and the
// runtime values are correct.
//
// Bug history:
// - Test 326: `~5` in comparison produced wrong value because -6 was
//   written as 4294967290 (unsigned reinterpret) instead of -6.
// - `-32768i16`: (int16_t)32768 overflows to -32768 in C, combining
//   with '-' from VisitNegativeValue gave '--32768' → maximal munch
//   parses as decrement operator → "lvalue expected" error.
// ═══════════════════════════════════════════════════════════════════════

// ===================================================================
// i8: range -128 to 127
// ===================================================================

@test
public func int_min_i8_declaration(env : &mut TestEnv) {
    var x : i8 = -128i8
    if(x != -128i8) { env.error("min i8 should be -128") }
}

@test
public func int_max_i8_declaration(env : &mut TestEnv) {
    var x : i8 = 127i8
    if(x != 127i8) { env.error("max i8 should be 127") }
}

@test
public func int_i8_arithmetic_range(env : &mut TestEnv) {
    var min : i8 = -128i8
    var max : i8 = 127i8
    var sum = min + max
    if(sum != -1i8) { env.error("-128 + 127 should be -1") }
}

@test
public func int_i8_negate_positive(env : &mut TestEnv) {
    var x : i8 = -127i8
    if(x != -127i8) { env.error("-127 should be -127") }
    var neg = -x
    // -(-127) = 127
    if(neg != 127i8) { env.error("-(-127) should be 127") }
}

@test
public func int_i8_bitwise_not(env : &mut TestEnv) {
    var x : i8 = 5i8
    var nx = ~x
    if(nx != -6i8) { env.error("~5 on i8 should be -6") }
}

@test
public func int_i8_bitwise_not_zero(env : &mut TestEnv) {
    var x : i8 = 0i8
    if(~x != -1i8) { env.error("~0 on i8 should be -1") }
}

@test
public func int_i8_bitwise_not_max(env : &mut TestEnv) {
    var x : i8 = 127i8
    if(~x != -128i8) { env.error("~127 on i8 should be -128") }
}

@test
public func int_i8_comparison_chain(env : &mut TestEnv) {
    if(!(-128i8 < -64i8)) { env.error("-128 < -64") }
    if(!(-64i8 < 0i8)) { env.error("-64 < 0") }
    if(!(0i8 < 64i8)) { env.error("0 < 64") }
    if(!(64i8 < 127i8)) { env.error("64 < 127") }
    if(!(-128i8 <= -128i8)) { env.error("-128 <= -128") }
    if(!(127i8 >= 127i8)) { env.error("127 >= 127") }
}

// ===================================================================
// i16: range -32768 to 32767
// ===================================================================

@test
public func int_min_i16_declaration(env : &mut TestEnv) {
    var x : i16 = -32768i16
    if(x != -32768i16) { env.error("min i16 should be -32768") }
}

@test
public func int_max_i16_declaration(env : &mut TestEnv) {
    var x : i16 = 32767i16
    if(x != 32767i16) { env.error("max i16 should be 32767") }
}

@test
public func int_i16_copy_min(env : &mut TestEnv) {
    var x : i16 = -32768i16
    var y : i16 = x
    if(y != -32768i16) { env.error("copy of min i16") }
}

@test
public func int_i16_arithmetic_range(env : &mut TestEnv) {
    var x : i16 = -32768i16
    var y : i16 = 32767i16
    var sum = x + y
    if(sum != -1i16) { env.error("-32768 + 32767 should be -1") }
}

@test
public func int_i16_negate_positive(env : &mut TestEnv) {
    var x : i16 = -32767i16
    if(x != -32767i16) { env.error("-32767 should be -32767") }
    var neg = -x
    if(neg != 32767i16) { env.error("-(-32767) should be 32767") }
}

@test
public func int_i16_single_negation(env : &mut TestEnv) {
    var x : i16 = -32768i16
    if(x != -32768i16) { env.error("single negation -32768") }
}

@test
public func int_i16_double_negation(env : &mut TestEnv) {
    var x : i16 = -(-32768i16)
    // -(-32768) = 32768 which overflows i16 to -32768
    if(x != -32768i16) { env.error("-(-32768) should overflow i16 to -32768") }
}

@test
public func int_i16_triple_negation(env : &mut TestEnv) {
    var x : i16 = -(-(-32768i16))
    if(x != -32768i16) { env.error("3 negations of -32768 should overflow to -32768") }
}

@test
public func int_i16_quadruple_negation(env : &mut TestEnv) {
    var x : i16 = -(-(-(-32768i16)))
    if(x != -32768i16) { env.error("4 negations of -32768 should overflow to -32768") }
}

@test
public func int_i16_bitwise_not(env : &mut TestEnv) {
    var x : i16 = 5i16
    if(~x != -6i16) { env.error("~5 on i16 should be -6") }
}

@test
public func int_i16_bitwise_not_zero(env : &mut TestEnv) {
    var x : i16 = 0i16
    if(~x != -1i16) { env.error("~0 on i16 should be -1") }
}

@test
public func int_i16_bitwise_not_max(env : &mut TestEnv) {
    var x : i16 = 32767i16
    if(~x != -32768i16) { env.error("~32767 on i16 should be -32768") }
}

@test
public func int_i16_bitwise_not_min(env : &mut TestEnv) {
    var x : i16 = -32768i16
    if(~x != 32767i16) { env.error("~(-32768) on i16 should be 32767") }
}

@test
public func int_i16_comparison_min(env : &mut TestEnv) {
    if(!(-32768i16 < -32767i16)) { env.error("-32768 < -32767") }
    if(-32768i16 == -32767i16) { env.error("-32768 should not = -32767") }
    if(!(-32768i16 <= -32768i16)) { env.error("-32768 <= -32768") }
}

@test
public func int_i16_array_init(env : &mut TestEnv) {
    var arr : [5]i16 = [-32768i16, -16384i16, 0i16, 16384i16, 32767i16]
    if(arr[0] != -32768i16) { env.error("arr[0]") }
    if(arr[2] != 0i16) { env.error("arr[2]") }
    if(arr[4] != 32767i16) { env.error("arr[4]") }
}

func get_min_i16_helper() : i16 {
    return -32768i16
}

func check_i16_is_min(val : i16) : bool {
    return val == -32768i16
}

@test
public func int_i16_function_return_min(env : &mut TestEnv) {
    if(get_min_i16_helper() != -32768i16) { env.error("return min i16") }
}

@test
public func int_i16_function_arg_min(env : &mut TestEnv) {
    if(!check_i16_is_min(-32768i16)) { env.error("pass min i16 to function") }
}

@test
public func int_i16_subtract_from_min(env : &mut TestEnv) {
    var x : i16 = -32768i16
    var diff = x - 1i16
    if(diff != 32767i16) { env.error("-32768 - 1 should overflow to 32767") }
}

@test
public func int_i16_add_to_max(env : &mut TestEnv) {
    var x : i16 = 32767i16
    var sum = x + 1i16
    if(sum != -32768i16) { env.error("32767 + 1 should overflow to -32768") }
}

// ===================================================================
// i32: range -2147483648 to 2147483647
// ===================================================================

@test
public func int_min_i32_declaration(env : &mut TestEnv) {
    var x : i32 = -2147483648i32
    if(x != -2147483648i32) { env.error("min i32 should be -2147483648") }
}

@test
public func int_max_i32_declaration(env : &mut TestEnv) {
    var x : i32 = 2147483647i32
    if(x != 2147483647i32) { env.error("max i32 should be 2147483647") }
}

@test
public func int_i32_arithmetic_range(env : &mut TestEnv) {
    var x : i32 = -2147483648i32
    var y : i32 = 2147483647i32
    var sum = x + y
    if(sum != -1i32) { env.error("min i32 + max i32 should be -1") }
}

@test
public func int_i32_negate_positive(env : &mut TestEnv) {
    var x : i32 = -2147483647i32
    var neg = -x
    if(neg != 2147483647i32) { env.error("-(-2147483647) should be 2147483647") }
}

@test
public func int_i32_double_negation(env : &mut TestEnv) {
    var x : i32 = -(-2147483648i32)
    // -(-2147483648) = 2147483648 which overflows i32 to -2147483648
    if(x != -2147483648i32) { env.error("-(-2147483648) should overflow to itself") }
}

@test
public func int_i32_bitwise_not(env : &mut TestEnv) {
    var x : i32 = 5i32
    if(~x != -6i32) { env.error("~5 on i32 should be -6") }
}

@test
public func int_i32_bitwise_not_zero(env : &mut TestEnv) {
    var x : i32 = 0i32
    if(~x != -1i32) { env.error("~0 on i32 should be -1") }
}

@test
public func int_i32_bitwise_not_max(env : &mut TestEnv) {
    var x : i32 = 2147483647i32
    if(~x != -2147483648i32) { env.error("~max i32 should be min i32") }
}

@test
public func int_i32_comparison_min(env : &mut TestEnv) {
    if(!(-2147483648i32 < 0i32)) { env.error("min i32 < 0") }
    if(-2147483648i32 > 0i32) { env.error("min i32 > 0 should be false") }
    if(!(-2147483648i32 <= -2147483648i32)) { env.error("min i32 <= min i32") }
}

// ===================================================================
// i64: range -9223372036854775808 to 9223372036854775807
// ===================================================================

@test
public func int_i64_negate(env : &mut TestEnv) {
    var x : i64 = -9223372036854775807i64 - 1i64
    if(x != -9223372036854775808i64) { env.error("min i64 via expression") }
}

@test
public func int_i64_bitwise_not(env : &mut TestEnv) {
    var x : i64 = 5i64
    if(~x != -6i64) { env.error("~5 on i64 should be -6") }
}

@test
public func int_i64_bitwise_not_zero(env : &mut TestEnv) {
    var x : i64 = 0i64
    if(~x != -1i64) { env.error("~0 on i64 should be -1") }
}

// ===================================================================
// Bitwise NOT (the original Test 326 bug)
// ===================================================================

@test
public func int_bitwise_not_in_comparison(env : &mut TestEnv) {
    // This was Test 326: comptime_bitwise_not(5) == ~5
    // The bug: -6 was written as 4294967290, then 4294967290 != ~5 → FAIL
    var x : i32 = ~5i32
    if(x != -6i32) { env.error("~5 should be -6") }
    // Verify the comparison itself works
    var result = (x == ~5i32)
    if(!result) { env.error("comparison with ~5 should work") }
}

@test
public func int_bitwise_not_chain(env : &mut TestEnv) {
    var a : i32 = 5i32
    var b : i32 = ~a
    var c : i32 = ~b
    if(c != 5i32) { env.error("~~5 should be 5") }
}

@test
public func int_bitwise_not_all_bits(env : &mut TestEnv) {
    // ~(-1) = 0 and ~0 = -1 for all signed types
    if(~(-1i8) != 0i8) { env.error("~(-1) on i8 should be 0") }
    if(~(-1i16) != 0i16) { env.error("~(-1) on i16 should be 0") }
    if(~(-1i32) != 0i32) { env.error("~(-1) on i32 should be 0") }
    if(~(-1i64) != 0i64) { env.error("~(-1) on i64 should be 0") }
}

// ===================================================================
// Mixed operations across types
// ===================================================================

@test
public func int_cross_type_arithmetic(env : &mut TestEnv) {
    var a : i8 = -128i8
    var b : i16 = a as i16
    if(b != -128i16) { env.error("i8 -128 cast to i16") }

    var c : i16 = -32768i16
    var d : i32 = c as i32
    if(d != -32768i32) { env.error("i16 -32768 cast to i32") }

    var e : i32 = -2147483648i32
    var f : i64 = e as i64
    if(f != -2147483648i64) { env.error("i32 min cast to i64") }
}

@test
public func int_negation_in_expression(env : &mut TestEnv) {
    var a : i16 = -1i16
    var b : i16 = -(-(-(-a)))
    if(b != 1i16) { env.error("-(-(-(-(-1)))) should be 1") }
}

@test
public func int_negate_chain_mixed(env : &mut TestEnv) {
    var x : i16 = -(-(-1i16))
    // -(-(-1)) = -(1) = -1
    if(x != -1i16) { env.error("-(-(-1)) should be -1") }

    var y : i16 = -(-(-(-1i16)))
    // -(-(-(-1))) = -(-(1)) = -(-1) = 1
    if(y != 1i16) { env.error("-(-(-(-1))) should be 1") }
}

// ===================================================================
// Loop with edge values
// ===================================================================

@test
public func int_loop_negative_range(env : &mut TestEnv) {
    var sum : i16 = 0i16
    var i : i16 = -5i16
    while(i < 5i16) {
        sum = sum + i
        i = i + 1i16
    }
    if(sum != -5i16) { env.error("sum of -5..4 should be -5") }
}

@test
public func int_loop_near_min(env : &mut TestEnv) {
    var x : i16 = -32768i16
    var count : i16 = 0i16
    while(x < -32760i16) {
        count = count + 1i16
        x = x + 1i16
    }
    if(count != 8i16) { env.error("should iterate 8 times from -32768") }
}
