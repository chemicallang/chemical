// Stable safety diagnostic codes (S0.1).
//
// These tests pin the error[SAFE-*]/warn[SAFE-*] rendering independently of the
// human-readable message. Every safety rule added later must add a code test.

@test
func neg_uninit_diagnostic_has_code(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Container {\n    @delete func delete(&mut self) { }\n}\nfunc use(c : Container) { }\npublic func main() : int {\n    var c : Container\n    use(c)\n    return 0\n}\n"
    expect_compile_error(env, "uninit_diagnostic_code", ch, "SAFE-INIT-001")
}

@test
func neg_safety_option_removed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // There is no module-level safety opt-out anymore. The option registry
    // rejects the old key instead of silently disabling checks.
    var mod = "module neg_test\nsource \".\"\noption safety = false\n"
    expect_compile_error_with_mod(env, "safety_option_removed", "func main() {}", "unknown option 'safety'", mod)
}

@test
func lifetime_reference_syntax_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // S1.1a: `&'a T` is parsed and carried on ReferenceType. Lifetime
    // resolution/checking lands in the next stage.
    var ch = "func <'a> read_ref(x : &'a int) : int {\n    return *x\n}\nstruct View<'a> {\n    var p : &'a int\n}\nstruct Holder<'a> {\n    var view : View<'a>\n}\npublic func main() : int {\n    var x = 5\n    return read_ref(&x) - 5\n}\n"
    expect_compile_success(env, "lifetime_reference_syntax", ch)
}

@test
func neg_unknown_lifetime_parameter(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct View<'a> {\n    var p : &'b int\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "unknown_lifetime_param", ch, "unknown lifetime parameter 'b'")
}

@test
func neg_duplicate_lifetime_parameter(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func <'a, 'a> bad(x : &'a int) : int { return *x }\nfunc main() {}\n"
    expect_compile_error(env, "duplicate_lifetime_param", ch, "duplicate lifetime parameter")
}

@test
func unsafe_deref_warning_has_code(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Report-only safety rule: the program still compiles, but the warning
    // carries a stable code so clients can act on it.
    var ch = "public func main() : int {\n    var x : int = 1\n    var p = &raw x\n    return *p\n}\n"
    expect_compile_warning_id(env, "unsafe_deref_warning_code", ch, "SAFE-UNSAFE-001")
}
