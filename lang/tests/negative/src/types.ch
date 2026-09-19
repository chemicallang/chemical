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
    // string * int fails during C translation, producing a compiler error
    expect_compile_error(env, "string_multiply", ch, "couldn't compile")
}

@test
func int_to_pointer_cast_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // An explicit `as` cast from an integer to a pointer is supported.
    var ch = "func main() {\n    var x = 42\n    var p = x as *int\n}\n"
    expect_compile_success(env, "int_to_ptr_cast_ok", ch)
}

@test
func neg_array_index_on_non_array(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var y = x[0]\n}\n"
    expect_compile_error(env, "index_on_int", ch, "can't be of type void")
}

@test
func pointer_arithmetic_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Pointer arithmetic is a core language operation (used across the TLS
    // and archive libraries, e.g. `data + pos`).
    var ch = "func main() {\n    var x = 42\n    var p : *int = &raw x\n    var y = p + 1\n}\n"
    expect_compile_success(env, "ptr_arithmetic_ok", ch)
}

@test
func neg_missing_struct_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Point {\n    var x : int\n    var y : int\n}\nfunc main() {\n    var p = Point { x : 1 }\n}\n"
    expect_compile_error(env, "missing_struct_field", ch, "couldn't find value for member")
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
