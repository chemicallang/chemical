// Lambda & Closure Negative Tests
// Tests that the compiler correctly rejects invalid lambda expressions.

@test
func neg_lambda_wrong_param_count(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func takes_fn(fn : (int) => int) : int { return fn(1) }\nfunc main() {\n    takes_fn((a, b) => a + b)\n}\n"
    expect_compile_error(env, "lambda_wrong_param_count", ch, "Lambda function type expects")
}

@test
func neg_lambda_param_type_mismatch(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func takes_fn(fn : (*char) => int) : int { return fn(\"hello\") }\nfunc main() {\n    takes_fn((x : int) => x)\n}\n"
    expect_compile_error(env, "lambda_param_type", ch, "Lambda function parameter type mismatch")
}

@test
func neg_lambda_return_type_mismatch(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func takes_fn(fn : (int) => int) : int { return fn(1) }\nfunc main() {\n    takes_fn((x) => \"hello\")\n}\n"
    expect_compile_error(env, "lambda_return_type", ch, "expected return type")
}

@test
func neg_lambda_captures_outside_scope(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func maker() : () => int {\n    var x = 42\n    return () => x\n}\nfunc main() {}\n"
    expect_compile_error(env, "lambda_capture_outside_scope", ch, "outside of lambda scope")
}

@test
func neg_lambda_calling_non_function(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 42\n    x()\n}\n"
    expect_compile_error(env, "call_non_function", ch, "cannot call a non function type")
}

@test
func neg_lambda_no_capture_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func takes_fn(fn : () => int) : int { return fn() }\nfunc main() {\n    var x = 42\n    takes_fn(|| x)\n}\n"
    // Non-capturing fn type called with capturing lambda
    expect_compile_error(env, "lambda_non_capturing_mismatch", ch, "lambda function type is not capturing")
}

@test
func neg_fn_expects_lambda_wrong_param(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func call_it(fn : () => int) : int { return fn() }\nfunc main() {\n    call_it(42)\n}\n"
    expect_compile_error(env, "fn_lambda_arg_non_fn", ch, "function doesn't expect a lambda function argument")
}

@test
func neg_function_not_takes_self(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func greet(name : *char) { }\nfunc main() {\n    var str = \"hello\"\n    str.greet()\n}\n"
    expect_compile_error(env, "method_call_no_self", ch, "doesn't take a self argument")
}
