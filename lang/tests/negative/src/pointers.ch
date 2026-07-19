// Pointer & Reference Negative Tests
// Tests that the compiler correctly rejects invalid pointer and reference operations.

@test
func neg_deref_void_ptr(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var p : *void = null\n    var x = *p\n}\n"
    expect_compile_error(env, "deref_void_ptr", ch, "void")
}

@test
func neg_addr_of_temporary(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_int() : int { return 42 }\nfunc main() {\n    var p = &raw get_int()\n}\n"
    expect_compile_error(env, "addr_of_temporary", ch, "r-value")
}

@test
func neg_double_deref_non_ptr(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var y = **x\n}\n"
    expect_compile_error(env, "double_deref_non_ptr", ch, "cannot dereference")
}

@test
func neg_pointer_to_mut_mismatch(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var p : *mut int = &raw x\n}\n"
    expect_compile_error(env, "ptr_mut_mismatch", ch, "type")
}

@test
func neg_null_not_in_pointer_context(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = null\n}\n"
    expect_compile_error(env, "null_alone", ch, "type")
}

@test
func neg_assign_to_const_ptr(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    const p : *int = &raw x\n    *p = 100\n}\n"
    expect_compile_error(env, "assign_via_const_ptr", ch, "cannot assign")
}

@test
func neg_subtract_pointers_diff_types(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var y : *char = null\n    var p : *int = &raw x\n    var d = p - y\n}\n"
    expect_compile_error(env, "ptr_subtract_diff_types", ch, "type")
}

@test
func neg_invalid_ptr_compare(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var p : *int = &raw x\n    var b = p < \"hello\"\n}\n"
    expect_compile_error(env, "ptr_compare_str", ch, "operator")
}

@test
func neg_index_on_pointer(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var arr : [3]int = [1, 2, 3]\n    var p = &raw arr[0]\n    var v = p[1]\n}\n"
    // Indexing a pointer may or may not be supported
    expect_compile_error(env, "index_on_ptr", ch, "operator")
}

@test
func neg_assign_different_ptr_types(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var p : *int = &raw x\n    var s : *char = p\n}\n"
    expect_compile_error(env, "assign_diff_ptr_types", ch, "does not satisfy")
}
