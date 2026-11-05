// Panic effect (`[no_panic]`) checks. The v1 slice currently parses and
// enforces the panic effect; other v1 effects are not implemented yet.

@test
func no_panic_simple_function_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func add(x : int) : int [no_panic] { return x + 1 }\npublic func main() : int { return add(41) - 42 }\n"
    expect_compile_success(env, "no_panic_simple", ch)
}

@test
func no_panic_annotated_callee_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper() : int [no_panic] { return 1 }\nfunc f() : int [no_panic] { return helper() }\npublic func main() : int { return f() - 1 }\n"
    expect_compile_success(env, "no_panic_callee", ch)
}

@test
func neg_no_panic_rejects_unannotated_callee(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper() : int { return 1 }\nfunc f() : int [no_panic] { return helper() }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "no_panic_unannotated_callee", ch, "SAFE-PANIC-001")
}

@test
func neg_no_panic_rejects_explicit_panic(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using namespace std;\nfunc f() [no_panic] { panic(\"boom\") }\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "no_panic_explicit_panic", ch, "SAFE-PANIC-001", NEG_MOD_STD_CORE)
}

@test
func neg_no_panic_rejects_bounds_check(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func at(x : int) : int [no_panic] {\n    var values : [2]int = [1, 2]\n    return values[x]\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "no_panic_bounds_check", ch, "SAFE-PANIC-001")
}

@test
func neg_no_panic_rejects_function_pointer_call(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f(cb : () => int) : int [no_panic] { return cb() }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "no_panic_function_pointer", ch, "SAFE-PANIC-001")
}

@test
func neg_no_panic_rejects_may_panic_callee(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `[panic]` is an explicit may-panic declaration; a `[no_panic]` caller
    // cannot trust it even when the current body happens to return normally.
    var ch = "func may_panic() : int [panic] { return 1 }\nfunc f() : int [no_panic] { return may_panic() }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "no_panic_may_panic_callee", ch, "SAFE-PANIC-001")
}

@test
func neg_unsupported_effect_name_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The v1 slice implements only panic/no_panic; other proposal-0005 names
    // are parsed and rejected explicitly instead of being mistaken for an
    // array suffix.
    var ch = "func f() : int [no_alloc] { return 1 }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "unsupported_effect_name", ch, "unsupported function effect")
}

@test
func no_panic_allows_unsafe_trap(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `unreachable` is an unsafe trap, not a panic; `[no_panic]` alone does not
    // reject it when the explicit unsafe boundary is present.
    var ch = "func f(x : bool) : int [no_panic] {\n    if(x) { return 1 } else unsafe { unreachable }\n}\npublic func main() : int { return f(true) - 1 }\n"
    expect_compile_success(env, "no_panic_unsafe_trap", ch)
}
