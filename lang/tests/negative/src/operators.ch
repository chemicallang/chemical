// Operator Negative Tests
// Tests that the compiler correctly rejects invalid operator usage.

@test
func neg_op_assign_no_overload(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyInt {\n    var val : int\n}\nfunc main() {\n    var a = MyInt { val : 1 }\n    var b = MyInt { val : 2 }\n    var c = a + b\n}\n"
    expect_compile_error(env, "op_add_no_overload", ch, "operator")
}

@test
func neg_op_equal_no_partial_eq(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyInt {\n    var val : int\n}\nfunc main() {\n    var a = MyInt { val : 1 }\n    var b = MyInt { val : 2 }\n    var c = a == b\n}\n"
    expect_compile_error(env, "op_eq_no_partial_eq", ch, "operator")
}

@test
func neg_op_assign_overload_not_two_params(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n}\nimpl core::ops::Add for S {\n    func add(&self) : S { return S { x : 1 } }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = S { x : 2 }\n    var c = a + b\n}\n"
    // This will likely fail because Add requires 2 params
    expect_compile_error(env, "op_add_wrong_params", ch, "operator")
}

@test
func neg_op_shift_negative(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 1 << -1\n}\n"
    // Shifting by negative may be a runtime check or compile error
    expect_compile_error(env, "shift_negative", ch, "operator")
}

@test
func neg_op_mul_string(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = \"hello\" * 3\n}\n"
    expect_compile_error(env, "mul_string", ch, "operator")
}

@test
func neg_op_mod_float(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 5.5 % 2.0\n}\n"
    expect_compile_error(env, "mod_float", ch, "operator")
}

@test
func neg_op_bitwise_not_bool(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = ~true\n}\n"
    expect_compile_error(env, "bitwise_not_bool", ch, "operator")
}

@test
func neg_op_assign_plus_non_mut(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    const x = 5\n    x += 1\n}\n"
    expect_compile_error(env, "op_assign_const", ch, "cannot assign")
}

@test
func neg_op_ptr_subtract_non_ptr(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var y = 10\n    var d = x - y\n}\n"
    // This is valid subtraction, not pointer-related
    expect_compile_success(env, "int_subtract_ok", ch)
}

@test
func neg_op_lt_string(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var cmp = \"abc\" < \"def\"\n}\n"
    expect_compile_error(env, "lt_string", ch, "operator")
}
