// Function Declaration Negative Tests
// Tests that the compiler correctly rejects invalid function declarations and calls.

@test
func neg_self_param_non_method(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func do_something(&self) { }\nfunc main() {}\n"
    expect_compile_error(env, "self_param_non_method", ch, "self")
}

@test
func neg_break_outside_loop(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    break\n}\n"
    expect_compile_error(env, "break_outside_loop", ch, "break")
}

@test
func neg_continue_outside_loop(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    continue\n}\n"
    expect_compile_error(env, "continue_outside_loop", ch, "continue")
}

@test
func neg_return_outside_func(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "return 42\nfunc main() {}\n"
    expect_compile_error(env, "return_outside_func", ch, "return")
}

@test
func neg_missing_return_value(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_int() : int {\n    // missing return\n}\nfunc main() {}\n"
    expect_compile_error(env, "missing_return_value", ch, "return")
}

@test
func neg_extern_with_body(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "@extern func foo() : int { return 42 }\nfunc main() {}\n"
    expect_compile_error(env, "extern_with_body", ch, "extern")
}

@test
func neg_nonretained_call(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    @delete func delete(&mut self) { }\n}\nfunc main() {\n    var s = S {}\n    s.delete()\n}\n"
    expect_compile_error(env, "nonretained_call", ch, "not retained")
}

@test
func neg_invalid_capture_list(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 5\n    var fn = [x] () => x\n}\n"
    expect_compile_error(env, "invalid_lambda_capture", ch, "capture")
}

@test
func neg_variadic_param_not_last(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func foo(x : any..., y : int) { }\nfunc main() {}\n"
    expect_compile_error(env, "variadic_not_last", ch, "variadic")
}

@test
func neg_func_redefined_param(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func foo(a : int, a : int) { }\nfunc main() {}\n"
    expect_compile_error(env, "redefined_param", ch, "already exists")
}
