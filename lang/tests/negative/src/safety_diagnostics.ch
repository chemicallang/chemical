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
func generic_negative_send_impl_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Wrapper<T> {\n    var value : T\n}\nimpl<T> !Send for Wrapper<T> {}\npublic func main() : int { return 0 }\n"
    expect_compile_output_not_contains(env, "generic_negative_send_impl", ch, "negative implementations", NEG_MOD_CORE)
}

@test
func neg_generic_negative_send_bound_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> !Send for Wrapper<T> {}\npublic func main() : int {\n    var h = Holder<Wrapper<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "generic_negative_send_bound", ch, "is declared !Send", NEG_MOD_CORE)
}

@test
func neg_generic_negative_sync_bound_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> !Sync for Wrapper<T> {}\npublic func main() : int {\n    var h = Holder<Wrapper<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "generic_negative_sync_bound", ch, "is declared !Sync", NEG_MOD_CORE)
}

@test
func generic_negative_with_bound_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using core::ops::Clone;\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T: Clone> !Send for Wrapper<T> {}\npublic func main() : int { return 0 }\n"
    expect_compile_output_not_contains(env, "generic_negative_bound", ch, "negative implementations", NEG_MOD_STD_CORE)
}

@test
func neg_conditional_generic_negative_applies(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // int is Clone, so the conditional `impl<T: Clone> !Send` applies.
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T: Clone> !Send for Wrapper<T> {}\npublic func main() : int {\n    var h = Holder<Wrapper<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "conditional_generic_negative", ch, "is declared !Send", NEG_MOD_STD_CORE)
}

@test
func conditional_generic_negative_does_not_apply(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Plain is Send structurally but does not implement Clone, so the
    // conditional negative impl must not apply.
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Plain {\n    var x : int\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T: Clone> !Send for Wrapper<T> {}\npublic func main() : int {\n    var h = Holder<Wrapper<Plain>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "conditional_generic_negative_skip", ch, "is declared !Send", NEG_MOD_STD_CORE)
}

@test
func negative_send_impl_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Safe {\n    var x : int\n}\nimpl !Send for Safe {}\npublic func main() : int { return 0 }\n"
    expect_compile_output_not_contains(env, "negative_send_impl", ch, "negative implementations", NEG_MOD_CORE)
}

@test
func neg_negative_send_bound_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Safe {\n    var x : int\n}\nimpl !Send for Safe {}\npublic func main() : int {\n    var h = Holder<Safe> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "negative_send_bound", ch, "is declared !Send", NEG_MOD_CORE)
}

@test
func neg_negative_sync_bound_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Safe {\n    var x : int\n}\nimpl !Sync for Safe {}\npublic func main() : int {\n    var h = Holder<Safe> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "negative_sync_bound", ch, "is declared !Sync", NEG_MOD_CORE)
}

@test
func neg_negative_impl_only_auto_traits(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Row {\n    var x : int\n}\nimpl !Show for Row {}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "negative_impl_non_auto", ch, "SAFE-AUTO-002")
}

@test
func neg_negative_impl_conflicts_with_positive(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Safe {\n    var x : int\n}\nunsafe impl Send for Safe {}\nimpl !Send for Safe {}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "negative_impl_conflict", ch, "SAFE-AUTO-002", NEG_MOD_CORE)
}

@test
func unsafe_send_impl_allows_raw_pointer(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Raw {\n    var p : *mut int\n}\nunsafe impl Send for Raw {}\npublic func main() : int {\n    var h = Holder<Raw> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "unsafe_send_impl", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func unsafe_sync_impl_allows_raw_pointer(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Raw {\n    var p : *mut int\n}\nunsafe impl Sync for Raw {}\npublic func main() : int {\n    var h = Holder<Raw> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "unsafe_sync_impl", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func neg_safe_send_impl_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Raw {\n    var p : *mut int\n}\nimpl Send for Raw {}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "safe_send_impl", ch, "SAFE-AUTO-001", NEG_MOD_CORE)
}

@test
func generic_function_inferred_bound_checked(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Row {\n    var x : int\n}\nfunc <T> f(x : T) : int where T: Copy {\n    return 0\n}\npublic func main() : int {\n    var r = Row { x : 1 }\n    return f(r)\n}\n"
    expect_compile_error_with_mod(env, "generic_fn_inferred_bound", ch, "SAFE-COPY-001", NEG_MOD_CORE)
}

@test
func generic_function_explicit_bound_checked(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Row {\n    var x : int\n}\nfunc <T> f(x : T) : int where T: Copy {\n    return 0\n}\npublic func main() : int {\n    var r = Row { x : 1 }\n    return f<Row>(r)\n}\n"
    expect_compile_error_with_mod(env, "generic_fn_explicit_bound", ch, "SAFE-COPY-001", NEG_MOD_CORE)
}

@test
func generic_function_bound_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Row {\n    var x : int\n}\nimpl Copy for Row {}\nfunc <T> f(x : T) : int where T: Copy {\n    return 0\n}\npublic func main() : int {\n    var r = Row { x : 1 }\n    return f(r) + f<Row>(r)\n}\n"
    expect_compile_output_not_contains(env, "generic_fn_bound_ok", ch, "SAFE-COPY", NEG_MOD_CORE)
}

@test
func send_bound_structural_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Safe {\n    var x : int\n    var r : &int\n}\npublic func main() : int {\n    var h = Holder<Safe> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "send_structural_ok", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func sync_bound_structural_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Safe {\n    var x : int\n    var r : &int\n}\npublic func main() : int {\n    var h = Holder<Safe> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "sync_structural_ok", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func neg_send_raw_pointer_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Unsafe {\n    var p : *mut int\n}\npublic func main() : int {\n    var h = Holder<Unsafe> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "send_raw_pointer", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_CORE)
}

@test
func neg_send_phantom_raw_pointer(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct PhantomBad {\n    var marker : std::PhantomData<*mut int>\n}\npublic func main() : int {\n    var h = Holder<PhantomBad> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "send_phantom_raw_pointer", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_STD_CORE)
}

@test
func clone_bound_copy_implies_clone(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Clone {\n        return 0\n    }\n}\nstruct Row {\n    var x : int\n}\nimpl Copy for Row {}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "clone_copy_implies", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func clone_bound_explicit_impl(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Clone {\n        return 0\n    }\n}\nstruct Row {\n    var x : int\n}\nimpl Clone for Row {\n    func clone(&self) : Row {\n        return Row { x : self.x }\n    }\n}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "clone_explicit_impl", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func neg_clone_bound_requires_impl(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Clone {\n        return 0\n    }\n}\nstruct Row {\n    var x : int\n}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "clone_bound_requires_impl", ch, "does not satisfy where clause constraint 'T : Clone'", NEG_MOD_CORE)
}

@test
func default_bound_uses_zero_arg_constructor(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using core::ops::Default;\nstruct D {\n    var x : int\n    @constructor\n    func make() : D {\n        return D { x : 0 }\n    }\n}\nfunc <T> f(x : T) : int where T: Default {\n    return 0\n}\npublic func main() : int {\n    var d = D()\n    return f<D>(d) + f<int>(1)\n}\n"
    expect_compile_output_not_contains(env, "default_bound_ok", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func generic_impl_where_clause_copy_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Wrapper<T> {\n    var value : T\n}\nimpl<T> Copy for Wrapper<T> where T: Copy {}\npublic func main() : int {\n    var w = Wrapper<int> { value : 3 }\n    return w.value - 3\n}\n"
    expect_compile_output_not_contains(env, "generic_impl_where_copy", ch, "SAFE-COPY", NEG_MOD_CORE)
}

@test
func neg_generic_impl_where_clause_canceled_by_missing_bound(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Wrapper<T> {\n    var value : T\n}\nimpl<T> Copy for Wrapper<T> {}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "generic_impl_where_missing", ch, "SAFE-COPY-003", NEG_MOD_CORE)
}

@test
func generic_impl_where_clause_bound_enforced(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> Show for Wrapper<T> where T: Show {\n    func show(&self) : int {\n        return 0\n    }\n}\npublic func main() : int {\n    var w = Wrapper<int> { value : 3 }\n    return w.show()\n}\n"
    expect_compile_error(env, "generic_impl_where_bound", ch, "does not satisfy bound 'Show' required by this generic implementation")
}

@test
func generic_impl_where_clause_bound_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Row {\n    var x : int\n}\nimpl Show for Row {\n    func show(&self) : int {\n        return self.x\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> Show for Wrapper<T> where T: Show {\n    func show(&self) : int {\n        return 0\n    }\n}\npublic func main() : int {\n    var w = Wrapper<Row> { value : Row { x : 3 } }\n    return w.show()\n}\n"
    expect_compile_success(env, "generic_impl_where_bound_ok", ch)
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
    // Report-only rule: only raw pointers need unsafe; references do not.
    var ch = "public func main() : int {\n    var x : int = 1\n    var p = &raw x\n    return *p\n}\n"
    expect_compile_error(env, "unsafe_deref_warning_code", ch, "SAFE-UNSAFE-001")
}

@test
func reference_deref_is_safe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 1\n    var r = &x\n    return *r\n}\n"
    expect_compile_success(env, "reference_deref_safe", ch)
}

@test
func raw_pointer_reborrow_warns(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 1\n    var p = &raw mut x\n    var r = &*p\n    return *r\n}\n"
    expect_compile_error(env, "prov_raw_reborrow", ch, "SAFE-PROV-001")
}

@test
func raw_pointer_reference_cast_warns(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 1\n    var p = &raw mut x\n    var r = p as &int\n    return *r\n}\n"
    expect_compile_error(env, "prov_raw_cast", ch, "SAFE-PROV-001")
}

@test
func raw_pointer_reborrow_in_unsafe_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 1\n    var p = &raw mut x\n    var r = unsafe(&*p)\n    return *r\n}\n"
    expect_compile_success(env, "prov_raw_reborrow_unsafe", ch)
}

@test
func neg_conditional_where_generic_negative_applies(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> !Sync for Wrapper<T> where T: Clone {}\npublic func main() : int {\n    var h = Holder<Wrapper<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "conditional_where_generic_negative", ch, "is declared !Sync", NEG_MOD_STD_CORE)
}

@test
func neg_unsafe_cell_is_not_sync(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var h = Holder<std::UnsafeCell<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "unsafe_cell_not_sync", ch, "is declared !Sync", NEG_MOD_STD_CORE)
}

@test
func unsafe_cell_is_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var h = Holder<std::UnsafeCell<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "unsafe_cell_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_vector_get_ref_is_immutable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.push(1)\n    var r = v.get_ref(0)\n    *r = 2\n    return v.get(0)\n}\n"
    expect_compile_error_with_mod(env, "vector_get_ref_immutable", ch, "cannot assign to a non mutable value", NEG_MOD_STD_CORE)
}

@test
func nested_unsafe_send_impl_is_honored(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // The raw-pointer field is not Send structurally, but the nested type's
    // explicit unsafe impl must make the containing aggregate Send too.
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Raw {\n    var p : *mut int\n}\nunsafe impl Send for Raw {}\nstruct Nested {\n    var raw : Raw\n}\npublic func main() : int {\n    var h = Holder<Nested> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "nested_unsafe_send", ch, "does not satisfy", NEG_MOD_CORE)
}

@test
func neg_nested_raw_without_send_impl(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Raw {\n    var p : *mut int\n}\nstruct Nested {\n    var raw : Raw\n}\npublic func main() : int {\n    var h = Holder<Nested> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "nested_raw_not_send", ch, "does not satisfy", NEG_MOD_CORE)
}

@test
func neg_non_null_is_not_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var h = Holder<std::NonNull<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "non_null_not_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_non_null_is_not_sync(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var h = Holder<std::NonNull<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "non_null_not_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_manually_drop_follows_inner_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var d = std::manually_drop(std::NonNull<int> { ptr = null })\n    var h = Holder<std::ManuallyDrop<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "manually_drop_inner_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_maybe_uninit_follows_inner_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var m = std::maybe_uninit<std::NonNull<int>>()\n    var h = Holder<std::MaybeUninit<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "maybe_uninit_inner_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_box_follows_inner_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var b = std::box(std::NonNull<int> { ptr = null })\n    var h = Holder<std::Box<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "box_inner_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_box_follows_inner_sync(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var b = std::box(std::NonNull<int> { ptr = null })\n    var h = Holder<std::Box<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "box_inner_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_rc_is_not_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var r = std::rc(1)\n    var h = Holder<std::Rc<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "rc_not_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_rc_is_not_sync(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var r = std::rc(1)\n    var h = Holder<std::Rc<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "rc_not_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_unsafe_function_call_requires_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "unsafe func danger() : int { return 1 }\npublic func main() : int {\n    var x = danger()\n    return x\n}\n"
    expect_compile_error(env, "unsafe_fn_call", ch, "SAFE-UNSAFE-002")
}

@test
func neg_unsafe_method_call_requires_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Reader {\n    var value : int\n    unsafe func read(&self) : int { return self.value }\n}\npublic func main() : int {\n    var r = Reader { value : 1 }\n    return r.read()\n}\n"
    expect_compile_error(env, "unsafe_method_call", ch, "SAFE-UNSAFE-002")
}

@test
func neg_ref_cell_is_not_sync(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var c = std::ref_cell(1)\n    var h = Holder<std::RefCell<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "ref_cell_not_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func ref_cell_is_send_when_inner_is_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var c = std::ref_cell(1)\n    var h = Holder<std::RefCell<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "ref_cell_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func mutex_is_sync_when_inner_is_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var m = std::new_mutex(1)\n    var h = Holder<std::Mutex<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "mutex_sync_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_mutex_not_sync_when_inner_is_not_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var m = std::new_mutex(std::NonNull<int> { ptr = null })\n    var h = Holder<std::Mutex<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "mutex_inner_not_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_deque_get_ref_is_immutable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var d = std::deque<int>()\n    d.push(1)\n    *d.get_ref(0) = 2\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "deque_get_ref_immutable", ch, "cannot assign to a non mutable", NEG_MOD_STD_CORE)
}

@test
func deque_get_mut_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var d = std::deque<int>()\n    d.push(1)\n    *d.get_mut(0) = 2\n    return *d.get_ref(0)\n}\n"
    expect_compile_success_with_mod(env, "deque_get_mut_allowed", ch, NEG_MOD_STD_CORE)
}

@test
func neg_unsafe_cell_get_ref_requires_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var c = std::unsafe_cell(1)\n    var r = c.get_ref()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "unsafe_cell_safe_get_ref", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_maybe_uninit_assume_init_requires_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var m = std::maybe_uninit<int>()\n    var x = m.assume_init()\n    return x\n}\n"
    expect_compile_error_with_mod(env, "maybe_uninit_safe_assume", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_manually_drop_take_requires_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var d = std::manually_drop(1)\n    var x = d.take()\n    return x\n}\n"
    expect_compile_error_with_mod(env, "manually_drop_safe_take", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_vector_set_len_requires_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.push(1)\n    v.set_len(2)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "vector_set_len_requires_unsafe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func vector_set_len_allowed_in_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.push(1)\n    unsafe { v.set_len(1) }\n    return v.size() as int\n}\n"
    expect_compile_success_with_mod(env, "vector_set_len_unsafe_ok", ch, NEG_MOD_STD_CORE)
}

@test
func neg_function_instance_make2_requires_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func dtor(obj : *mut void) { }\npublic func main() : int {\n    var f = std::default_function_instance.make2(null, null, dtor, 0, 0)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "function_instance_make2_requires_unsafe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_option_take_requires_mutable_receiver(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var o = std::Option.Some<int>(1)\n    const c = o\n    var x = c.take()\n    return x\n}\n"
    expect_compile_error_with_mod(env, "option_take_immutable", ch, "mutable implicit self", NEG_MOD_STD_CORE)
}

@test
func rwlock_is_send_and_sync_when_inner_qualifies(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send + Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var rw = std::new_rwlock(1)\n    var h = Holder<std::RwLock<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "rwlock_send_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func rwlock_is_send_when_inner_is_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var rw = std::new_rwlock(std::unsafe_cell(1))\n    var h = Holder<std::RwLock<std::UnsafeCell<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "rwlock_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_rwlock_not_sync_when_inner_is_not_sync(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var rw = std::new_rwlock(std::unsafe_cell(1))\n    var h = Holder<std::RwLock<std::UnsafeCell<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "rwlock_inner_not_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func arc_is_send_and_sync_when_inner_qualifies(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send + Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var a = std::arc(1)\n    var h = Holder<std::Arc<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "arc_send_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_arc_not_send_when_inner_is_not_send(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var a = std::arc(std::NonNull<int> { ptr = null })\n    var h = Holder<std::Arc<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "arc_inner_not_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_option_clone_requires_clone(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var o = std::Option.Some<std::NonNull<int>>(std::NonNull<int> { ptr = null })\n    var c = o.clone()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "option_clone_bound", ch, "Clone", NEG_MOD_STD_CORE)
}

@test
func neg_vector_clone_requires_clone(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var v = std::vector<std::NonNull<int>>()\n    var c = v.clone()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "vector_clone_bound", ch, "Clone", NEG_MOD_STD_CORE)
}

@test
func waker_is_send_and_sync(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func <T> needs_send(v : T) : int where T : Send { return 0 }\nfunc <T> needs_sync(v : T) : int where T : Sync { return 0 }\npublic func main() : int {\n    var w = core::async::Waker { data : null, vtbl : null }\n    var w2 = core::async::Waker { data : null, vtbl : null }\n    return needs_send(w) + needs_sync(w2)\n}\n"
    expect_compile_success_with_mod(env, "waker_send_sync", ch, NEG_MOD_CORE)
}

@test
func extern_call_warns_without_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "@extern func abs(v : int) : int\npublic func main() : int {\n    return abs(-1)\n}\n"
    expect_compile_warning_id(env, "ffi_extern_call", ch, "SAFE-FFI-001")
}

@test
func extern_call_in_unsafe_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "@extern func abs(v : int) : int\npublic func main() : int {\n    return unsafe(abs(-1))\n}\n"
    expect_compile_success(env, "ffi_extern_call_unsafe", ch)
}
