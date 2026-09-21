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
    // a comptime block is evaluated by the interpreter, which evaluates regular
    // function bodies as well, so calling a non-comptime function whose result is
    // computable at compile time is allowed
    var ch = "comptime func get_val() : int { return 42 }\npublic func main() {\n    comptime {\n        var x = get_val()\n    }\n}\n"
    expect_compile_success(env, "comptime_const_call", ch)
}

@test
func comptime_block_outside_func_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // a `comptime { }` block at the top level of a module is legal: its contents are
    // evaluated while the module is being declared
    var ch = "comptime {\n    var x = 5\n}\npublic func main() {}\n"
    expect_compile_success(env, "comptime_block_toplevel", ch)
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
    expect_compile_error(env, "const_runtime_init", ch, "cannot call a runtime function")
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

@test
func neg_comptime_infinite_recursion(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // the interpreter recurses on the compiler's own stack, so a comptime function
    // which calls itself forever used to overflow it and crash the compiler
    var ch = "comptime func f() : int {\n    return f()\n}\npublic func main() : int {\n    return f()\n}\n"
    expect_compile_error(env, "comptime_infinite_recursion", ch, "ran out of stack space")
}

@test
func neg_comptime_unbounded_recursion_depth(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // recursion that is deep enough to exhaust the stack must be reported as well,
    // there is no way to know that it would never return
    var ch = "comptime func f(n : int) : int {\n    if(n <= 0) {\n        return 0\n    } else {\n        return f(n - 1)\n    }\n}\npublic func main() : int {\n    return f(1000000)\n}\n"
    expect_compile_error(env, "comptime_recursion_depth", ch, "ran out of stack space")
}

@test
func neg_comptime_error_during_codegen_fails_build(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // an error raised while a comptime function is evaluated during code generation
    // used to be printed and then dropped, letting the build continue with a value
    // that no longer matched the source
    var ch = "comptime func f() : int {\n    intrinsics::error(\"comptime boom\")\n    return 7\n}\npublic func main() : int {\n    return f()\n}\n"
    expect_compile_error(env, "comptime_error_codegen", ch, "comptime boom")
}
