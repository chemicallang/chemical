// Type System Negative Tests
// Tests that the compiler correctly rejects type-level errors.

@test
func neg_string_to_int_assign(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 5\n    x = \"hello\"\n}\n"
    expect_compile_error(env, "string_to_int_assign", ch, "does not satisfy")
}

@test
func neg_int_to_float_explicit(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x : float = 42\n}\n"
    expect_compile_error(env, "int_to_float_implicit", ch, "does not satisfy")
}

@test
func neg_bool_arithmetic(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = true + 1\n}\n"
    expect_compile_error(env, "bool_plus_int", ch, "operator")
}

@test
func neg_string_multiply(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = \"hello\" * 2\n}\n"
    expect_compile_error(env, "string_multiply", ch, "operator")
}

@test
func neg_pointer_to_non_pointer_cast(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var p = x as *int\n}\n"
    // Casting non-pointer to pointer should error in safe mode or need explicit
    expect_compile_error(env, "non_ptr_to_ptr_cast", ch, "operator")
}

@test
func neg_array_index_on_non_array(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var y = x[0]\n}\n"
    expect_compile_error(env, "index_on_int", ch, "operator")
}

@test
func neg_implicit_pointer_arithmetic(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var p : *int = &raw x\n    var y = p + 1\n}\n"
    expect_compile_error(env, "ptr_arithmetic", ch, "operator")
}

@test
func neg_missing_struct_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Point {\n    var x : int\n    var y : int\n}\nfunc main() {\n    var p = Point { x : 1 }\n}\n"
    // Missing required field may or may not be an error - depends on compiler
    expect_compile_error(env, "missing_struct_field", ch, "child")
}

@test
func neg_return_with_value_in_void(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func do_nothing() : void {\n    return 42\n}\nfunc main() {}\n"
    expect_compile_error(env, "return_value_in_void", ch, "does not satisfy")
}

@test
func neg_wrong_arg_type_call(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func greet(name : *char) { }\nfunc main() {\n    greet(42)\n}\n"
    expect_compile_error(env, "wrong_arg_type", ch, "does not satisfy")
}

@test
func neg_too_many_args(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func add(a : int, b : int) : int { return a + b }\nfunc main() {\n    add(1, 2, 3)\n}\n"
    expect_compile_error(env, "too_many_args", ch, "argument")
}

@test
func neg_too_few_args(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func add(a : int, b : int) : int { return a + b }\nfunc main() {\n    add(1)\n}\n"
    expect_compile_error(env, "too_few_args", ch, "argument")
}

@test
func neg_void_var_init(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_void() : void {}\nfunc main() {\n    var x = get_void()\n}\n"
    expect_compile_error(env, "cannot_init_var_with_void", ch, "void")
}

@test
func neg_different_pointer_types(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var y : *char = &raw x\n}\n"
    expect_compile_error(env, "ptr_type_mismatch", ch, "does not satisfy")
}
