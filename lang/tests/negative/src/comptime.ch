// Comptime Negative Tests
// Tests that the compiler correctly rejects invalid compile-time code.

@test
func neg_comptime_var_mutation(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    comptime var x = 5\n    comptime x = 10\n}\n"
    expect_compile_error(env, "comptime_var_mutation", ch, "comptime")
}

@test
func neg_comptime_non_const_expr(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_val() : int { return 42 }\nfunc main() {\n    comptime {\n        var x = get_val()\n    }\n}\n"
    expect_compile_error(env, "comptime_non_const_call", ch, "comptime")
}

@test
func neg_comptime_block_outside_func(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "comptime {\n    var x = 5\n}\nfunc main() {}\n"
    expect_compile_error(env, "comptime_block_toplevel", ch, "comptime")
}

@test
func neg_comptime_if_not_const(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var runtime_var = 42\n    comptime if(runtime_var > 10) { }\n}\n"
    expect_compile_error(env, "comptime_if_runtime_cond", ch, "comptime")
}

@test
func neg_comptime_unreachable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    comptime {\n        intrinsics::unreachable()\n    }\n}\n"
    expect_compile_error(env, "comptime_unreachable", ch, "comptime")
}

@test
func neg_const_with_runtime_init(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_val() : int { return 42 }\nconst GLOBAL = get_val()\nfunc main() {}\n"
    expect_compile_error(env, "const_runtime_init", ch, "comptime")
}

@test
func neg_comptime_pointer_deref(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    comptime {\n        var p = &raw x\n        *p\n    }\n}\n"
    expect_compile_error(env, "comptime_ptr_deref", ch, "comptime")
}

@test
func neg_comptime_for_not_const_iter(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var runtime_arr : [3]int = [1, 2, 3]\n    comptime for(var x in runtime_arr) { }\n}\n"
    expect_compile_error(env, "comptime_for_runtime", ch, "comptime")
}

@test
func neg_comptime_fn_runtime_arg(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "comptime func compute(x : int) : int { return x * 2 }\nfunc main() {\n    var v = 42\n    compute(v)\n}\n"
    expect_compile_error(env, "comptime_fn_runtime_arg", ch, "comptime")
}
