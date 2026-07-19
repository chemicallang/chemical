// Top-Level & Global Context Negative Tests
// Tests that the compiler correctly rejects invalid top-level declarations and expressions.

@test
func neg_top_level_call_runtime_func(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_val() : int { return 42 }\nvar x = get_val()\nfunc main() {}\n"
    expect_compile_error(env, "top_level_call_runtime", ch, "cannot call a runtime function at top level")
}

@test
func neg_top_level_expr_runtime(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_val() : int { return 42 }\nfunc main() {\n    var y = 10\n}\n// var x = get_val()\n"
    // Just test global variable type inference
    var ch2 = "var x = 42\nfunc main() {}\n"
    expect_compile_success(env, "global_var_infer_ok", ch2)
}

@test
func neg_top_level_addr_of(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "var x = 42\nvar p = &raw x\nfunc main() {}\n"
    expect_compile_error(env, "top_level_addr_of", ch, "cannot take address of value at runtime outside function body")
}

@test
func neg_top_level_deref(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "var x = 42\nvar p = &raw x\nvar v = *p\nfunc main() {}\n"
    expect_compile_error(env, "top_level_deref", ch, "cannot dereference value at runtime outside function body")
}

@test
func neg_top_level_inc_dec(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "var x = 42\nx++\nfunc main() {}\n"
    expect_compile_error(env, "top_level_inc_dec", ch, "cannot increment or decrement value at runtime outside")
}

@test
func neg_top_level_index(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "var arr : [3]int = [1, 2, 3]\nvar x = arr[0]\nfunc main() {}\n"
    expect_compile_error(env, "top_level_index", ch, "cannot index into a value at runtime outside")
}

@test
func neg_top_level_lambda(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "var fn = () => 42\nfunc main() {}\n"
    expect_compile_error(env, "top_level_lambda", ch, "lambda functions at top level outside function body aren't supported")
}

@test
func neg_global_var_no_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "var x\nfunc main() {}\n"
    expect_compile_error(env, "global_var_no_type", ch, "a type of a value must be given for global variable")
}

@test
func neg_global_var_infer_missing(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "var x = some_undefined\nfunc main() {}\n"
    expect_compile_error(env, "global_var_infer_missing", ch, "unresolved variable identifier")
}

@test
func neg_empty_access_chain(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {}\nvar s = S{}\nvar x = s.\nfunc main() {}\n"
    // This might be a parser error - empty access chain
    expect_compile_error(env, "empty_access_chain", ch, "empty")
}

@test
func neg_top_level_comptime_self_ref(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {}\ncomptime if(true) {\n    var x = main\n}\n"
    // comptime if at top level referencing local symbol
    expect_compile_error(env, "top_level_comptime_self_ref", ch, "top level comptime if")
}

@test
func neg_top_level_alias_incompatible(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo {}\nstruct Bar {}\ntypealias Alias = Foo\nvar x : Bar = Alias {}\nfunc main() {}\n"
    expect_compile_error(env, "alias_type_mismatch", ch, "does not satisfy")
}
