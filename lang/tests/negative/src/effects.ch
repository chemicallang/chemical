// Proposal 0005 effect checks. `[no_panic]` is enforced on every lowered
// body; Phase 3 adds `[no_alloc]`, `[no_unsafe]`, `[no_blocking]` and the
// `[pure]` conjunction. Restrictions are transitive: a call is accepted only
// when the callee carries the same explicit restriction.

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
func no_panic_allows_unsafe_trap(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `unreachable` is an unsafe trap, not a panic; `[no_panic]` alone does not
    // reject it when the explicit unsafe boundary is present.
    var ch = "func f(x : bool) : int [no_panic] {\n    if(x) { return 1 } else unsafe { unreachable }\n}\npublic func main() : int { return f(true) - 1 }\n"
    expect_compile_success(env, "no_panic_unsafe_trap", ch)
}

// ── [no_alloc] ─────────────────────────────────────────────────────────────

@test
func no_alloc_simple_function_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func add(x : int, y : int) : int [no_alloc] { return x + y }\npublic func main() : int { return add(1, 1) - 2 }\n"
    expect_compile_success(env, "no_alloc_simple", ch)
}

@test
func no_alloc_annotated_callee_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper(v : int) : int [no_alloc] { return v + 1 }\nfunc f(v : int) : int [no_alloc] { return helper(v) }\npublic func main() : int { return f(0) - 1 }\n"
    expect_compile_success(env, "no_alloc_callee", ch)
}

@test
func neg_no_alloc_rejects_unannotated_callee(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper(v : int) : int { return v + 1 }\nfunc f(v : int) : int [no_alloc] { return helper(v) }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "no_alloc_unannotated_callee", ch, "SAFE-ALLOC-001")
}

@test
func neg_no_alloc_rejects_heap_allocation(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `new` lowers to a call to the runtime allocator; an unannotated allocator
    // is not provably alloc-free, so the restriction fails closed.
    var ch = "func f(v : int) : int [no_alloc] {\n    var p = unsafe(malloc(8))\n    unsafe { dealloc p }\n    return v\n}\npublic func main() : int { return f(1) - 1 }\n"
    expect_compile_error_with_mod(env, "no_alloc_new", ch, "SAFE-ALLOC-001", NEG_MOD_CSTD)
}

// ── [no_unsafe] ────────────────────────────────────────────────────────────

@test
func no_unsafe_simple_function_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func add(x : int, y : int) : int [no_unsafe] { return x + y }\npublic func main() : int { return add(1, 1) - 2 }\n"
    expect_compile_success(env, "no_unsafe_simple", ch)
}

@test
func no_unsafe_annotated_callee_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper(v : int) : int [no_unsafe] { return v + 1 }\nfunc f(v : int) : int [no_unsafe] { return helper(v) }\npublic func main() : int { return f(0) - 1 }\n"
    expect_compile_success(env, "no_unsafe_callee", ch)
}

@test
func neg_no_unsafe_rejects_unsafe_block(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f(v : int) : int [no_unsafe] {\n    var x = v\n    unsafe { x = x + 1 }\n    return x\n}\npublic func main() : int { return f(0) - 1 }\n"
    expect_compile_error(env, "no_unsafe_block", ch, "SAFE-UNSAFE-004")
}

@test
func neg_no_unsafe_rejects_unsafe_expression(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f(v : int) : int [no_unsafe] {\n    return unsafe(v) + 1\n}\npublic func main() : int { return f(0) - 1 }\n"
    expect_compile_error(env, "no_unsafe_expression", ch, "SAFE-UNSAFE-004")
}

@test
func neg_no_unsafe_rejects_trap(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f(x : bool) : int [no_unsafe] {\n    if(x) { return 1 } else unsafe { unreachable }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "no_unsafe_trap", ch, "SAFE-UNSAFE-004")
}

@test
func neg_no_unsafe_rejects_unannotated_callee(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper(v : int) : int { return v + 1 }\nfunc f(v : int) : int [no_unsafe] { return helper(v) }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "no_unsafe_unannotated_callee", ch, "SAFE-UNSAFE-004")
}

// ── [no_blocking] ──────────────────────────────────────────────────────────

@test
func no_blocking_annotated_callee_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper(v : int) : int [no_blocking] { return v + 1 }\nfunc f(v : int) : int [no_blocking] { return helper(v) }\npublic func main() : int { return f(0) - 1 }\n"
    expect_compile_success(env, "no_blocking_callee", ch)
}

@test
func neg_no_blocking_rejects_unannotated_callee(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper(v : int) : int { return v + 1 }\nfunc f(v : int) : int [no_blocking] { return helper(v) }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "no_blocking_unannotated_callee", ch, "SAFE-BLOCKING-001")
}

// ── [pure] and combined restrictions ───────────────────────────────────────

@test
func pure_arithmetic_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func add(x : int, y : int) : int [pure] { return x + y }\npublic func main() : int { return add(20, 22) - 42 }\n"
    expect_compile_success(env, "pure_arithmetic", ch)
}

@test
func neg_pure_rejects_unannotated_callee(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper(v : int) : int { return v + 1 }\nfunc f(v : int) : int [pure] { return helper(v) }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "pure_unannotated_callee", ch, "SAFE-")
}

@test
func neg_pure_rejects_unsafe_block(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f(v : int) : int [pure] {\n    var x = v\n    unsafe { x = x + 1 }\n    return x\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "pure_unsafe_block", ch, "SAFE-UNSAFE-004")
}

@test
func combined_restrictions_compile(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func helper(v : int) : int [no_panic, no_alloc, no_unsafe, no_blocking] { return v + 1 }\nfunc f(v : int) : int [no_panic, no_alloc, no_unsafe, no_blocking] { return helper(v) }\npublic func main() : int { return f(0) - 1 }\n"
    expect_compile_success(env, "combined_restrictions", ch)
}

// ── positive declarations, conflicts, unknown names ────────────────────────

@test
func positive_effect_declarations_compile(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func a() : int [panic] { return 1 }\nfunc b(v : int) : int [alloc] { return v }\nfunc c(v : int) : int [unsafe] { return v }\nfunc d(v : int) : int [blocking] { return v }\nfunc e() [noreturn] { panic(\"always\") }\npublic func main() : int { return a() + b(0) + c(0) + d(0) - 1 }\n"
    expect_compile_success_with_mod(env, "positive_effects", ch, NEG_MOD_STD_CORE)
}

// ── [noreturn] enforcement ─────────────────────────────────────────────────

@test
func noreturn_panic_body_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A tail `panic(...)` lowers to a call to a declaration marked `@no_return`,
    // so the implicit fallthrough return is unreachable.
    var ch = "func die() [noreturn] { panic(\"always\") }\npublic func main() : int { return 0 }\n"
    expect_compile_success_with_mod(env, "noreturn_panic_body", ch, NEG_MOD_STD_CORE)
}

@test
func noreturn_both_branches_panic_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func die(x : bool) [noreturn] {\n    if(x) { panic(\"yes\") } else { panic(\"no\") }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success_with_mod(env, "noreturn_both_branches", ch, NEG_MOD_STD_CORE)
}

@test
func noreturn_constant_true_loop_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The constant-true condition folds to the body edge, so the loop exit (and
    // the implicit fallthrough return after it) is unreachable.
    var ch = "func spin() [noreturn] { while(true) { } }\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "noreturn_while_true", ch)
}

@test
func neg_noreturn_rejects_return(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f() [noreturn] { return }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "noreturn_return", ch, "SAFE-NORETURN-001")
}

@test
func neg_noreturn_rejects_fallthrough(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f() [noreturn] { }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "noreturn_fallthrough", ch, "SAFE-NORETURN-001")
}

@test
func neg_noreturn_rejects_conditional_fallthrough(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f(x : bool) [noreturn] { if(x) { } else { } }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "noreturn_conditional_fallthrough", ch, "SAFE-NORETURN-001")
}

@test
func neg_no_alloc_conflict(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f() : int [alloc, no_alloc] { return 1 }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "effect_conflict_alloc", ch, "cannot declare both")
}

@test
func neg_no_unsafe_conflict(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f() : int [unsafe, no_unsafe] { return 1 }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "effect_conflict_unsafe", ch, "cannot declare both")
}

@test
func neg_no_blocking_conflict(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f() : int [blocking, no_blocking] { return 1 }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "effect_conflict_blocking", ch, "cannot declare both")
}

@test
func neg_no_panic_conflict(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f() : int [panic, no_panic] { return 1 }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "effect_conflict_panic", ch, "cannot declare both")
}

@test
func neg_pure_conflicts_with_positive(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func f() : int [pure, alloc] { return 1 }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "effect_conflict_pure", ch, "[pure] cannot be combined")
}

@test
func neg_unsupported_effect_name_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Unknown names are rejected explicitly instead of being mistaken for an
    // array suffix.
    var ch = "func f() [frobnicate] { }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "unsupported_effect_name", ch, "unsupported function effect")
}

// ── generic propagation ────────────────────────────────────────────────────

@test
func generic_no_alloc_propagates(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> ident(v : T) : T [no_alloc] { return v }\nfunc caller(v : int) : int [no_alloc] { return ident<int>(v) }\npublic func main() : int { return caller(42) - 42 }\n"
    expect_compile_success(env, "generic_no_alloc", ch)
}

@test
func generic_no_panic_propagates(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> ident(v : T) : T [no_panic] { return v }\nfunc caller(v : int) : int [no_panic] { return ident<int>(v) }\npublic func main() : int { return caller(42) - 42 }\n"
    expect_compile_success(env, "generic_no_panic", ch)
}

@test
func neg_generic_no_alloc_body_is_checked(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> make(v : T) : int [no_alloc] {\n    var p = unsafe(malloc(8))\n    unsafe { dealloc p }\n    return 1\n}\npublic func main() : int { return make<int>(1) - 1 }\n"
    expect_compile_error_with_mod(env, "generic_no_alloc_body", ch, "SAFE-ALLOC-001", NEG_MOD_CSTD)
}

@test
func generic_method_effect_propagates(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Boxed<T> {\n    var value : T\n    func get(&self) : T [no_alloc] { return self.value }\n}\nfunc caller(b : &Boxed<int>) : int [no_alloc] { return b.get() }\npublic func main() : int {\n    var b = Boxed<int>{ value: 42 }\n    return caller(&b) - 42\n}\n"
    expect_compile_success(env, "generic_method_effect", ch)
}

// ── module policy defaults (proposal 0005) ─────────────────────────────────

@test
func module_no_panic_default_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var mod = "module neg_test\nsource \".\"\nno_panic = true\n"
    var ch = "func helper(v : int) : int { return v + 1 }\npublic func main() : int { return helper(41) - 42 }\n"
    expect_compile_success_with_mod(env, "module_no_panic_default", ch, mod)
}

@test
func neg_module_no_panic_default_is_enforced(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var mod = "module neg_test\nsource \".\"\nimport std\nimport core\noption no_panic = true\n"
    var ch = "using namespace std;\nfunc helper() : int { panic(\"boom\"); return 0 }\npublic func main() : int { return helper() }\n"
    expect_compile_error_with_mod(env, "module_no_panic_enforced", ch, "SAFE-PANIC-001", mod)
}

@test
func module_no_alloc_default_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var mod = "module neg_test\nsource \".\"\noption no_alloc = true\n"
    var ch = "func helper(v : int) : int { return v + 1 }\npublic func main() : int { return helper(41) - 42 }\n"
    expect_compile_success_with_mod(env, "module_no_alloc_default", ch, mod)
}

@test
func module_default_positive_override(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var mod = "module neg_test\nsource \".\"\nimport std\nimport core\nno_panic = true\n"
    // `[panic]` opts this function out of the module default; main does the same
    // so the call is allowed.
    var ch = "using namespace std;\nfunc may_panic() [panic] { panic(\"boom\") }\npublic func main() : int [panic] { may_panic(); return 0 }\n"
    expect_compile_success_with_mod(env, "module_default_override", ch, mod)
}

// `[no_panic]` on the entry point used to be ignored and drop the `main`
// symbol (unresolved reference at link). It must link and run.
@test
func no_panic_main_entry_point_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int [no_panic] { return 7 }\n"
    var mod = "application neg_test\nsource \".\"\n"
    expect_compile_and_exit(env, "no_panic_main_entry", ch, mod, 7)
}
