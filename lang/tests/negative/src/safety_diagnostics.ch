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
func lifetime_elision_method_and_single_input(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Box {\n    var value : int\n    func get(&self) : &int {\n        return &self.value\n    }\n}\nfunc identity(x : &int) : &int {\n    return x\n}\npublic func main() : int {\n    var b = Box { value : 7 }\n    return *b.get() - 7\n}\n"
    expect_compile_success(env, "lifetime_elision_ok", ch)
}

@test
func neg_lifetime_elision_ambiguous(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func <'a, 'b> bad(x : &'a int, y : &'b int) : &int {\n    return x\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "lifetime_elision_ambiguous", ch, "missing lifetime specifier")
}

@test
func lifetime_where_outlives_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func <'a, 'b> f(x : &'a int) : int where 'a: 'b {\n    return *x\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "lifetime_where_ok", ch)
}

@test
func neg_lifetime_where_unknown(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func <'a> f(x : &'a int) : int where 'a: 'c {\n    return *x\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "lifetime_where_unknown", ch, "unknown lifetime parameter 'c' in where clause")
}

@test
func lifetime_generic_impl_and_literal_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct View<'a> {\n    var p : &'a int\n}\nimpl<'a> Show for View<'a> {\n    func show(&self) : int {\n        return *self.p\n    }\n}\nfunc <'a> make(x : &'a int) : View<'a> {\n    return View<'a> { p : x }\n}\npublic func main() : int {\n    var x = 9\n    var v = make(&x)\n    return v.show() - 9\n}\n"
    expect_compile_success(env, "lifetime_generic_impl_ok", ch)
}

@test
func lifetime_generic_impl_with_type_param_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct View<'a, T> {\n    var p : &'a T\n    func get(&self) : &'a T {\n        return self.p\n    }\n}\nimpl<'a, T> Show for View<'a, T> {\n    func show(&self) : int {\n        return 1\n    }\n}\nfunc <'a, T> make(x : &'a T) : View<'a, T> {\n    return View<'a, T> { p : x }\n}\npublic func main() : int {\n    var x = 9\n    var v = make(&x)\n    return *v.get() + v.show() - 10\n}\n"
    expect_compile_success(env, "lifetime_generic_impl_type_param_ok", ch)
}

@test
func lifetime_static_and_type_bounds_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func <'a, T> keep_static(x : T) : int where T: 'static {\n    return 0\n}\nfunc <'a, T> keep_a(x : T) : int where T: 'static + 'a {\n    return 0\n}\nfunc static_ref(x : &'static int) : int {\n    return *x\n}\npublic func main() : int {\n    var one = 1\n    return keep_static(1) + keep_a(2) + static_ref(&one) - 1\n}\n"
    expect_compile_success(env, "lifetime_static_bounds_ok", ch)
}

@test
func neg_lifetime_type_bound_unknown(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func <'a, T> bad(x : T) : int where T: 'b {\n    return 0\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "lifetime_type_bound_unknown", ch, "unknown lifetime parameter 'b' in where clause")
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
func destructor_valid_signatures_compile(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct A {\n    var x : int\n    @delete\n    func delete(&self) { }\n}\nstruct B {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "destructor_valid_signatures", ch)
}

@test
func neg_destructor_value_self_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Bad {\n    var x : int\n    @delete\n    func delete(self) { }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "destructor_value_self", ch, "SAFE-DROP-001")
}

@test
func neg_destructor_return_value_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Bad {\n    var x : int\n    @delete\n    func delete(&self) : int { return 1 }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "destructor_return_value", ch, "SAFE-DROP-001")
}

@test
func neg_destructor_non_self_param_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Bad {\n    var x : int\n    @delete\n    func delete(x : int) { }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "destructor_non_self_param", ch, "SAFE-DROP-001")
}

@test
func unsafe_deref_warning_has_code(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Report-only safety rule: the program still compiles, but the warning
    // carries a stable code so clients can act on it.
    var ch = "public func main() : int {\n    var x : int = 1\n    var p = &raw x\n    return *p\n}\n"
    expect_compile_warning_id(env, "unsafe_deref_warning_code", ch, "SAFE-UNSAFE-001")
}
