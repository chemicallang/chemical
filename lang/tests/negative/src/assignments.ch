// Assignment & Mutation Negative Tests
// Tests that the compiler correctly rejects invalid assignment and mutation operations.

@test
func neg_mutate_struct_immutable_self(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Point {\n    var x : int\n    func set_x(&self, val : int) {\n        x = val\n    }\n}\nfunc main() {}\n"
    expect_compile_error(env, "mutate_immutable_self", ch, "mutating a struct member requires a mutable self")
}

@test
func neg_assign_to_reference(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    var r = &mut x\n    r = 10\n}\n"
    expect_compile_error(env, "assign_to_reference", ch, "assignment to reference is forbidden")
}

@test
func neg_expression_not_assignable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    (1 + 2) = 5\n}\n"
    expect_compile_error(env, "expr_not_assignable", ch, "Expression is not assignable")
}

@test
func neg_assign_to_const_ptr_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Point { var x : int }\nfunc main() {\n    var p = Point { x : 1 }\n    p.x = 5\n}\n"
    expect_compile_error(env, "assign_non_mutable_field2", ch, "cannot assign to a non mutable value")
}

@test
func neg_top_level_destructible(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct HasDtor {\n    @delete func delete(&mut self) { }\n}\nvar global = HasDtor {}\nfunc main() {}\n"
    expect_compile_error(env, "top_level_destructible", ch, "top level variables or constants must be non-destructible")
}

@test
func neg_destruct_on_non_pointer(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    destruct x\n}\n"
    expect_compile_error(env, "destruct_non_ptr", ch, "destruct cannot be called on a value that isn't a pointer")
}

@test
func neg_destruct_on_non_ptr_expr(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    destruct 42\n}\n"
    expect_compile_error(env, "destruct_literal", ch, "destruct cannot be called")
}

@test
func neg_assign_void_value(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_void() : void {}\nfunc main() {\n    var x = 5\n    x = get_void()\n}\n"
    expect_compile_error(env, "assign_void_value", ch, "does not satisfy")
}

@test
func neg_assign_to_function_result(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_int() : int { return 42 }\nfunc main() {\n    get_int() = 10\n}\n"
    expect_compile_error(env, "assign_to_function_result", ch, "Expression is not assignable")
}

@test
func neg_mutate_const_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n}\nfunc main() {\n    const s = S { x : 1 }\n    s.x = 2\n}\n"
    expect_compile_error(env, "mutate_const_field", ch, "cannot assign to a non mutable value")
}

@test
func neg_missing_return_in_else(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_val(b : bool) : int {\n    if(b) {\n        return 1\n    } else {\n        // missing return\n    }\n}\nfunc main() {}\n"
    expect_compile_error(env, "missing_return_else", ch, "missing return in else body")
}

@test
func neg_return_type_mismatch2(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_val() : int {\n    return 3.14\n}\nfunc main() {}\n"
    expect_compile_error(env, "return_type_mismatch2", ch, "does not satisfy")
}
