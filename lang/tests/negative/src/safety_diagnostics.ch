// Stable safety diagnostic codes (S0.1).
//
// These tests pin the error[SAFE-*]/warn[SAFE-*] rendering independently of the
// human-readable message. Every safety rule added later must add a code test.

@test
func neg_uninit_diagnostic_has_code(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Container {\n    @delete func delete(&mut self) { }\n}\nfunc use(c : Container) { }\npublic func main() : int {\n    var c : Container\n    use(c)\n    return 0\n}\n"
    expect_compile_error(env, "uninit_diagnostic_code", ch, "SAFE-INIT-001")
}

@test
func neg_safety_option_removed(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // There is no module-level safety opt-out anymore. The option registry
    // rejects the old key instead of silently disabling checks.
    var mod = "module neg_test\nsource \".\"\noption safety = false\n"
    expect_compile_error_with_mod(env, "safety_option_removed", "func main() {}", "unknown option 'safety'", mod)
}

@test
func lifetime_reference_syntax_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S1.1a: `&'a T` is parsed and carried on ReferenceType. Lifetime
    // resolution/checking lands in the next stage.
    var ch = "func <'a> read_ref(x : &'a int) : int {\n    return *x\n}\nstruct View<'a> {\n    var p : &'a int\n}\nstruct Holder<'a> {\n    var view : View<'a>\n}\npublic func main() : int {\n    var x = 5\n    return read_ref(&x) - 5\n}\n"
    expect_compile_success(env, "lifetime_reference_syntax", ch)
}

@test
func lifetime_elision_method_and_single_input(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Box {\n    var value : int\n    func get(&self) : &int {\n        return &self.value\n    }\n}\nfunc identity(x : &int) : &int {\n    return x\n}\npublic func main() : int {\n    var b = Box { value : 7 }\n    return *b.get() - 7\n}\n"
    expect_compile_success(env, "lifetime_elision_ok", ch)
}

@test
func neg_lifetime_elision_ambiguous(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a, 'b> bad(x : &'a int, y : &'b int) : &int {\n    return x\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "lifetime_elision_ambiguous", ch, "missing lifetime specifier")
}

@test
func lifetime_where_outlives_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a, 'b> f(x : &'a int) : int where 'a: 'b {\n    return *x\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "lifetime_where_ok", ch)
}

@test
func neg_lifetime_where_unknown(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a> f(x : &'a int) : int where 'a: 'c {\n    return *x\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "lifetime_where_unknown", ch, "unknown lifetime parameter 'c' in where clause")
}

@test
func lifetime_generic_impl_and_literal_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct View<'a> {\n    var p : &'a int\n}\nimpl<'a> Show for View<'a> {\n    func show(&self) : int {\n        return *self.p\n    }\n}\nfunc <'a> make(x : &'a int) : View<'a> {\n    return View<'a> { p : x }\n}\npublic func main() : int {\n    var x = 9\n    var v = make(&x)\n    return v.show() - 9\n}\n"
    expect_compile_success(env, "lifetime_generic_impl_ok", ch)
}

@test
func lifetime_generic_impl_with_type_param_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct View<'a, T> {\n    var p : &'a T\n    func get(&self) : &'a T {\n        return self.p\n    }\n}\nimpl<'a, T> Show for View<'a, T> {\n    func show(&self) : int {\n        return 1\n    }\n}\nfunc <'a, T> make(x : &'a T) : View<'a, T> {\n    return View<'a, T> { p : x }\n}\npublic func main() : int {\n    var x = 9\n    var v = make(&x)\n    return *v.get() + v.show() - 10\n}\n"
    expect_compile_success(env, "lifetime_generic_impl_type_param_ok", ch)
}

@test
func lifetime_static_and_type_bounds_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a, T> keep_static(x : T) : int where T: 'static {\n    return 0\n}\nfunc <'a, T> keep_a(x : T) : int where T: 'static + 'a {\n    return 0\n}\npublic func main() : int {\n    return keep_static(1) + keep_a(2) - 2\n}\n"
    expect_compile_success(env, "lifetime_static_bounds_ok", ch)
}

@test
func neg_lifetime_type_bound_unknown(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a, T> bad(x : T) : int where T: 'b {\n    return 0\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "lifetime_type_bound_unknown", ch, "unknown lifetime parameter 'b' in where clause")
}

@test
func neg_unknown_lifetime_parameter(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct View<'a> {\n    var p : &'b int\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "unknown_lifetime_param", ch, "unknown lifetime parameter 'b'")
}

@test
func neg_duplicate_lifetime_parameter(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a, 'a> bad(x : &'a int) : int { return *x }\nfunc main() {}\n"
    expect_compile_error(env, "duplicate_lifetime_param", ch, "duplicate lifetime parameter")
}

@test
func generic_negative_send_impl_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Wrapper<T> {\n    var value : T\n}\nimpl<T> !Send for Wrapper<T> {}\npublic func main() : int { return 0 }\n"
    expect_compile_output_not_contains(env, "generic_negative_send_impl", ch, "negative implementations", NEG_MOD_CORE)
}

@test
func neg_generic_negative_send_bound_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> !Send for Wrapper<T> {}\npublic func main() : int {\n    var h = Holder<Wrapper<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "generic_negative_send_bound", ch, "is declared !Send", NEG_MOD_CORE)
}

@test
func neg_generic_negative_sync_bound_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> !Sync for Wrapper<T> {}\npublic func main() : int {\n    var h = Holder<Wrapper<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "generic_negative_sync_bound", ch, "is declared !Sync", NEG_MOD_CORE)
}

@test
func generic_negative_with_bound_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using core::ops::Clone;\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T: Clone> !Send for Wrapper<T> {}\npublic func main() : int { return 0 }\n"
    expect_compile_output_not_contains(env, "generic_negative_bound", ch, "negative implementations", NEG_MOD_STD_CORE)
}

@test
func neg_conditional_generic_negative_applies(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // int is Clone, so the conditional `impl<T: Clone> !Send` applies.
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T: Clone> !Send for Wrapper<T> {}\npublic func main() : int {\n    var h = Holder<Wrapper<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "conditional_generic_negative", ch, "is declared !Send", NEG_MOD_STD_CORE)
}

@test
func conditional_generic_negative_does_not_apply(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Plain is Send structurally but does not implement Clone, so the
    // conditional negative impl must not apply.
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Plain {\n    var x : int\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T: Clone> !Send for Wrapper<T> {}\npublic func main() : int {\n    var h = Holder<Wrapper<Plain>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "conditional_generic_negative_skip", ch, "is declared !Send", NEG_MOD_STD_CORE)
}

@test
func negative_send_impl_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Safe {\n    var x : int\n}\nimpl !Send for Safe {}\npublic func main() : int { return 0 }\n"
    expect_compile_output_not_contains(env, "negative_send_impl", ch, "negative implementations", NEG_MOD_CORE)
}

@test
func neg_negative_send_bound_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Safe {\n    var x : int\n}\nimpl !Send for Safe {}\npublic func main() : int {\n    var h = Holder<Safe> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "negative_send_bound", ch, "is declared !Send", NEG_MOD_CORE)
}

@test
func neg_negative_sync_bound_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Safe {\n    var x : int\n}\nimpl !Sync for Safe {}\npublic func main() : int {\n    var h = Holder<Safe> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "negative_sync_bound", ch, "is declared !Sync", NEG_MOD_CORE)
}

@test
func neg_negative_impl_only_auto_traits(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Row {\n    var x : int\n}\nimpl !Show for Row {}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "negative_impl_non_auto", ch, "SAFE-AUTO-002")
}

@test
func neg_negative_impl_conflicts_with_positive(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Safe {\n    var x : int\n}\nunsafe impl Send for Safe {}\nimpl !Send for Safe {}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "negative_impl_conflict", ch, "SAFE-AUTO-002", NEG_MOD_CORE)
}

@test
func unsafe_send_impl_allows_raw_pointer(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Raw {\n    var p : *mut int\n}\nunsafe impl Send for Raw {}\npublic func main() : int {\n    var h = Holder<Raw> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "unsafe_send_impl", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func unsafe_sync_impl_allows_raw_pointer(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Raw {\n    var p : *mut int\n}\nunsafe impl Sync for Raw {}\npublic func main() : int {\n    var h = Holder<Raw> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "unsafe_sync_impl", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func neg_erased_std_function_is_not_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using namespace core;\nfunc <T> needs_send(v : T) : int where T : Send { return 0 }\nfunc erased(f : std::function<() => void>) : int {\n    return needs_send(f)\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "std_function_not_send", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_STD_CORE)
}

@test
func typed_capture_owned_is_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using namespace core;\nfunc <T> needs_send(v : T) : int where T : Send { return 0 }\npublic func main() : int {\n    var x = 1\n    var f : std::function<() => int> = |x|() => { return x }\n    return needs_send(f)\n}\n"
    expect_compile_success_with_mod(env, "typed_capture_send", ch, NEG_MOD_STD_CORE)
}

@test
func neg_spawn_closure_rc_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var r = std::rc(5)\n    var h = std::thread.spawn_closure(|r|() => { return *r.get() })\n    return h.join()\n}\n"
    expect_compile_error_with_mod(env, "spawn_closure_rc", ch, "does not satisfy where clause constraint 'C : Send'", NEG_MOD_STD_CORE)
}

@test
func neg_tainted_callback_field_into_erased_api(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder {\n    var f : std::function<() => int>\n}\nfunc take(f : std::function<() => int>) : int { return f() }\npublic func main() : int {\n    var x = 1\n    var h : Holder\n    unsafe { h.f = |&x|() => { return *x } }\n    return take(h.f)\n}\n"
    expect_compile_error_with_mod(env, "tainted_callback_field_erased_api", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_tainted_callback_wrapped_in_generic(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var f : std::function<() => int>\n    unsafe { f = |&x|() => { return *x } }\n    var o : std::Option<std::function<() => int>> = std::Option.Some(f)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "tainted_callback_generic", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_tainted_callback_in_struct_into_erased_api(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder {\n    var f : std::function<() => int>\n}\nfunc take(h : Holder) : int { return h.f() }\npublic func main() : int {\n    var x = 1\n    var h : Holder\n    unsafe { h.f = |&x|() => { return *x } }\n    return take(h)\n}\n"
    expect_compile_error_with_mod(env, "tainted_callback_struct_erased_api", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_tainted_callback_into_erased_api(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func take(f : std::function<() => int>) : int { return f() }\npublic func main() : int {\n    var x = 1\n    var f : std::function<() => int>\n    unsafe { f = |&x|() => { return *x } }\n    return take(f)\n}\n"
    expect_compile_error_with_mod(env, "tainted_callback_erased_api", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_scoped_spawn_closure_rc_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var r = std::rc(5)\n    std::thread.scoped_spawn_closure(|r|() => { var x = *r.get() })\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "scoped_closure_rc", ch, "does not satisfy where clause constraint 'C : Send'", NEG_MOD_STD_CORE)
}

@test
func neg_raw_future_handle_not_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "async func work() : int {\n    var u = await async::yield_now()\n    return 1\n}\nfunc consume(h : core::async::FutureHandle<int>) : int {\n    return async::block_on<int>(h)\n}\npublic func main() : int {\n    var h = work()\n    var t = std::thread.spawn(consume, h)\n    return t.join()\n}\n"
    expect_compile_error_with_mod(env, "raw_future_not_send", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_ASYNC_STD)
}

@test
func neg_async_spawn_send_rc_frame_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "async func work() : int {\n    var r = std::rc(5)\n    var u = await async::yield_now()\n    return *r.get()\n}\npublic func main() : int {\n    var h = async::spawn_send(work())\n    return async::block_on<int>(h)\n}\n"
    expect_compile_error_with_mod(env, "async_spawn_send_rc", ch, "SAFE-SEND-001", NEG_MOD_ASYNC_STD)
}

@test
func neg_async_spawn_send_borrowed_frame_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "async func work() : int {\n    var n = 5\n    var r = &n\n    var u = await async::yield_now()\n    return *r\n}\npublic func main() : int {\n    var h = async::spawn_send(work())\n    return async::block_on<int>(h)\n}\n"
    expect_compile_error_with_mod(env, "async_spawn_send_borrow", ch, "SAFE-SEND-001", NEG_MOD_ASYNC_STD)
}

@test
func neg_async_spawn_send_stored_future_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "async func work() : int {\n    var u = await async::yield_now()\n    return 1\n}\npublic func main() : int {\n    var h = work()\n    var h2 = async::spawn_send(h)\n    return async::block_on<int>(h2)\n}\n"
    expect_compile_error_with_mod(env, "async_spawn_send_stored", ch, "SAFE-SEND-001", NEG_MOD_ASYNC_STD)
}

@test
func neg_spawn_closure_borrowed_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 5\n    unsafe {\n        var h = std::thread.spawn_closure(|&x|() => { return *x })\n        return h.join()\n    }\n}\n"
    expect_compile_error_with_mod(env, "spawn_closure_borrow", ch, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
}

@test
func neg_scoped_scope_cannot_be_forged(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var s = std::thread.ScopedScope<int> {\n        value : null,\n        threads : zeroed<[32]*mut std::concurrent.Thread>(),\n        count : 0\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "scoped_scope_forge", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_scoped_closure_scope_rc_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    std::thread.scoped_run_closure(||(scope : &mut std::thread.ScopedClosureScope) => {\n        var r = std::rc(5)\n        std::thread.scoped_spawn_closure_in(scope, |r|() => { var x = *r.get() })\n    })\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "scoped_closure_scope_rc", ch, "does not satisfy where clause constraint 'C : Send'", NEG_MOD_STD_CORE)
}

@test
func neg_scoped_closure_scope_cannot_be_forged(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var s = std::thread.ScopedClosureScope {\n        threads : zeroed<[32]*mut std::concurrent.Thread>(),\n        count : 0\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "scoped_closure_scope_forge", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_send_function_rc_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func take(f : std::send_function<() => int>) : int { return f() }\npublic func main() : int {\n    var r = std::rc(5)\n    return take(|r|() => { return *r.get() })\n}\n"
    expect_compile_error_with_mod(env, "send_function_rc", ch, "SAFE-SEND-002", NEG_MOD_STD_CORE)
}

@test
func neg_send_function_erased_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func take(f : std::send_function<() => int>) : int { return f() }\nfunc pass(f : std::function<() => int>) : int { return take(f) }\npublic func main() : int { return pass(||() => { return 1 }) }\n"
    expect_compile_error_with_mod(env, "send_function_erased", ch, "SAFE-SEND-002", NEG_MOD_STD_CORE)
}

@test
func neg_send_function_borrowed_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func take(f : std::send_function<() => int>) : int { return f() }\npublic func main() : int {\n    var x = 1\n    return take(|&x|() => { return *x })\n}\n"
    expect_compile_error_with_mod(env, "send_function_borrow", ch, "SAFE-SEND-002", NEG_MOD_STD_CORE)
}

@test
func neg_send_function_is_not_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> needs_sync(v : T) : int where T : Sync { return 0 }\nfunc take(f : std::send_function<() => int>) : int { return needs_sync(f) }\npublic func main() : int { return take(||() => { return 1 }) }\n"
    expect_compile_error_with_mod(env, "send_function_not_sync", ch, "does not satisfy where clause constraint 'T : Sync'", NEG_MOD_STD_CORE)
}

@test
func neg_scoped_send_function_needs_contract(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func run(f : std::scoped_send_function<() => void>) {\n    std::thread.scoped_spawn_closure_erased(f)\n}\npublic func main() : int {\n    var n = 1\n    run(|&mut n|() => { *n = *n + 1 })\n    return n\n}\n"
    expect_compile_error_with_mod(env, "scoped_send_no_contract", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_scoped_send_function_is_not_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> needs_send(v : T) : int where T : Send { return 0 }\nfunc take(f : std::scoped_send_function<() => void>) : int { return needs_send(f) }\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "scoped_send_not_send", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_STD_CORE)
}

@test
func neg_safe_send_impl_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Raw {\n    var p : *mut int\n}\nimpl Send for Raw {}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "safe_send_impl", ch, "SAFE-AUTO-001", NEG_MOD_CORE)
}

@test
func generic_function_inferred_bound_checked(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Row {\n    var x : int\n}\nfunc <T> f(x : T) : int where T: Copy {\n    return 0\n}\npublic func main() : int {\n    var r = Row { x : 1 }\n    return f(r)\n}\n"
    expect_compile_error_with_mod(env, "generic_fn_inferred_bound", ch, "SAFE-COPY-001", NEG_MOD_CORE)
}

@test
func generic_function_explicit_bound_checked(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Row {\n    var x : int\n}\nfunc <T> f(x : T) : int where T: Copy {\n    return 0\n}\npublic func main() : int {\n    var r = Row { x : 1 }\n    return f<Row>(r)\n}\n"
    expect_compile_error_with_mod(env, "generic_fn_explicit_bound", ch, "SAFE-COPY-001", NEG_MOD_CORE)
}

@test
func generic_function_bound_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Row {\n    var x : int\n}\nimpl Copy for Row {}\nfunc <T> f(x : T) : int where T: Copy {\n    return 0\n}\npublic func main() : int {\n    var r = Row { x : 1 }\n    return f(r) + f<Row>(r)\n}\n"
    expect_compile_output_not_contains(env, "generic_fn_bound_ok", ch, "SAFE-COPY", NEG_MOD_CORE)
}

@test
func send_bound_structural_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Safe {\n    var x : int\n    var r : &int\n}\npublic func main() : int {\n    var h = Holder<Safe> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "send_structural_ok", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func sync_bound_structural_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Safe {\n    var x : int\n    var r : &int\n}\npublic func main() : int {\n    var h = Holder<Safe> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "sync_structural_ok", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func neg_send_raw_pointer_field(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Unsafe {\n    var p : *mut int\n}\npublic func main() : int {\n    var h = Holder<Unsafe> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "send_raw_pointer", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_CORE)
}

@test
func neg_send_phantom_raw_pointer(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct PhantomBad {\n    var marker : std::PhantomData<*mut int>\n}\npublic func main() : int {\n    var h = Holder<PhantomBad> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "send_phantom_raw_pointer", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_STD_CORE)
}

@test
func clone_bound_copy_implies_clone(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Clone {\n        return 0\n    }\n}\nstruct Row {\n    var x : int\n}\nimpl Copy for Row {}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "clone_copy_implies", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func clone_bound_explicit_impl(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Clone {\n        return 0\n    }\n}\nstruct Row {\n    var x : int\n}\nimpl Clone for Row {\n    func clone(&self) : Row {\n        return Row { x : self.x }\n    }\n}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "clone_explicit_impl", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func neg_clone_bound_requires_impl(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Clone {\n        return 0\n    }\n}\nstruct Row {\n    var x : int\n}\npublic func main() : int {\n    var h = Holder<Row> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "clone_bound_requires_impl", ch, "does not satisfy where clause constraint 'T : Clone'", NEG_MOD_CORE)
}

@test
func default_bound_uses_zero_arg_constructor(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using core::ops::Default;\nstruct D {\n    var x : int\n    @constructor\n    func make() : D {\n        return D { x : 0 }\n    }\n}\nfunc <T> f(x : T) : int where T: Default {\n    return 0\n}\npublic func main() : int {\n    var d = D()\n    return f<D>(d) + f<int>(1)\n}\n"
    expect_compile_output_not_contains(env, "default_bound_ok", ch, "does not satisfy where clause constraint", NEG_MOD_CORE)
}

@test
func generic_impl_where_clause_copy_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Wrapper<T> {\n    var value : T\n}\nimpl<T> Copy for Wrapper<T> where T: Copy {}\npublic func main() : int {\n    var w = Wrapper<int> { value : 3 }\n    return w.value - 3\n}\n"
    expect_compile_output_not_contains(env, "generic_impl_where_copy", ch, "SAFE-COPY", NEG_MOD_CORE)
}

@test
func neg_generic_impl_where_clause_canceled_by_missing_bound(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Wrapper<T> {\n    var value : T\n}\nimpl<T> Copy for Wrapper<T> {}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "generic_impl_where_missing", ch, "SAFE-COPY-003", NEG_MOD_CORE)
}

@test
func generic_impl_where_clause_bound_enforced(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> Show for Wrapper<T> where T: Show {\n    func show(&self) : int {\n        return 0\n    }\n}\npublic func main() : int {\n    var w = Wrapper<int> { value : 3 }\n    return w.show()\n}\n"
    expect_compile_error(env, "generic_impl_where_bound", ch, "does not satisfy bound 'Show' required by this generic implementation")
}

@test
func generic_impl_where_clause_bound_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Row {\n    var x : int\n}\nimpl Show for Row {\n    func show(&self) : int {\n        return self.x\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> Show for Wrapper<T> where T: Show {\n    func show(&self) : int {\n        return 0\n    }\n}\npublic func main() : int {\n    var w = Wrapper<Row> { value : Row { x : 3 } }\n    return w.show()\n}\n"
    expect_compile_success(env, "generic_impl_where_bound_ok", ch)
}

@test
func destructor_valid_signatures_compile(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct A {\n    var x : int\n    @delete\n    func delete(&self) { }\n}\nstruct B {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "destructor_valid_signatures", ch)
}

@test
func neg_destructor_value_self_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Bad {\n    var x : int\n    @delete\n    func delete(self) { }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "destructor_value_self", ch, "SAFE-DROP-001")
}

@test
func neg_destructor_return_value_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Bad {\n    var x : int\n    @delete\n    func delete(&self) : int { return 1 }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "destructor_return_value", ch, "SAFE-DROP-001")
}

@test
func neg_destructor_non_self_param_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Bad {\n    var x : int\n    @delete\n    func delete(x : int) { }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "destructor_non_self_param", ch, "SAFE-DROP-001")
}

@test
func unsafe_deref_warning_has_code(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Report-only rule: only raw pointers need unsafe; references do not.
    var ch = "public func main() : int {\n    var x : int = 1\n    var p = &raw x\n    return *p\n}\n"
    expect_compile_error(env, "unsafe_deref_warning_code", ch, "SAFE-UNSAFE-001")
}

@test
func reference_deref_is_safe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var r = &x\n    return *r\n}\n"
    expect_compile_success(env, "reference_deref_safe", ch)
}

@test
func raw_pointer_reborrow_warns(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var p = &raw mut x\n    var r = &*p\n    return *r\n}\n"
    expect_compile_error(env, "prov_raw_reborrow", ch, "SAFE-PROV-001")
}

@test
func raw_pointer_reference_cast_warns(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var p = &raw mut x\n    var r = p as &int\n    return *r\n}\n"
    expect_compile_error(env, "prov_raw_cast", ch, "SAFE-PROV-001")
}

@test
func raw_pointer_reborrow_in_unsafe_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var p = &raw mut x\n    var r = unsafe(&*p)\n    return *r\n}\n"
    expect_compile_success(env, "prov_raw_reborrow_unsafe", ch)
}

@test
func neg_raw_pointer_field_projection(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct S { var x : int }\npublic func main() : int {\n    var p : *S = null\n    return p.x\n}\n"
    expect_compile_error(env, "raw_pointer_field_projection", ch, "SAFE-UNSAFE-001")
}

@test
func neg_raw_pointer_index(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *int = null\n    return p[0]\n}\n"
    expect_compile_error(env, "raw_pointer_index", ch, "SAFE-UNSAFE-001")
}

@test
func neg_raw_pointer_arithmetic(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *int = null\n    var q = p + 1\n    return 0\n}\n"
    expect_compile_error(env, "raw_pointer_arithmetic", ch, "SAFE-UNSAFE-001")
}

@test
func neg_raw_pointer_compound_arithmetic(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *int = null\n    p += 1\n    return 0\n}\n"
    expect_compile_error(env, "raw_pointer_compound_arithmetic", ch, "SAFE-UNSAFE-001")
}

@test
func neg_raw_pointer_increment(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *int = null\n    p++\n    return 0\n}\n"
    expect_compile_error(env, "raw_pointer_increment", ch, "SAFE-UNSAFE-001")
}

@test
func neg_raw_pointer_delete(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *int = null\n    delete p\n    return 0\n}\n"
    expect_compile_error(env, "raw_pointer_delete", ch, "SAFE-UNSAFE-001")
}

@test
func neg_raw_pointer_dealloc(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *int = null\n    dealloc p\n    return 0\n}\n"
    expect_compile_error(env, "raw_pointer_dealloc", ch, "SAFE-UNSAFE-001")
}

@test
func neg_vector_data_index_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using namespace std;\npublic func main() : int {\n    var v = vector<int>()\n    v.push(1)\n    return v.data()[0]\n}\n"
    expect_compile_error_with_mod(env, "vector_data_index", ch, "SAFE-UNSAFE-001", NEG_MOD_STD_CORE)
}

@test
func raw_pointer_projection_in_unsafe_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct S { var x : int }\npublic func main() : int {\n    var s = S { x : 4 }\n    var p = &raw mut s\n    return unsafe(p.x)\n}\n"
    expect_compile_success(env, "raw_pointer_projection_unsafe", ch)
}

@test
func neg_placement_new_raw_pointer(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct S { var x : int }\npublic func main() : int {\n    var p : *S = null\n    new (p) S { x : 1 }\n    return 0\n}\n"
    expect_compile_error(env, "placement_new_raw_pointer", ch, "SAFE-UNSAFE-001")
}

@test
func neg_unsafe_func_pointer_call(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "unsafe func danger() : int { return 7 }\npublic func main() : int {\n    var f = danger\n    return f()\n}\n"
    expect_compile_error(env, "unsafe_func_pointer_call", ch, "SAFE-UNSAFE-002")
}

@test
func unsafe_func_pointer_call_in_unsafe_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "unsafe func danger() : int { return 7 }\npublic func main() : int {\n    var f = danger\n    return unsafe(f())\n}\n"
    expect_compile_success(env, "unsafe_func_pointer_call_unsafe", ch)
}

@test
func unsafe_func_pointer_type_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "unsafe func danger() : int { return 7 }\npublic func main() : int {\n    var f : unsafe () => int = danger\n    return unsafe(f())\n}\n"
    expect_compile_success(env, "unsafe_func_pointer_type", ch)
}

@test
func neg_extern_func_pointer_call(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@extern func getpid() : int\npublic func main() : int {\n    var f = getpid\n    return f()\n}\n"
    expect_compile_error(env, "extern_func_pointer_call", ch, "SAFE-UNSAFE-002")
}

@test
func neg_unsafe_func_pointer_coercion(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "unsafe func danger() : int { return 7 }\npublic func main() : int {\n    var f : () => int = danger\n    return 0\n}\n"
    expect_compile_error(env, "unsafe_func_pointer_coercion", ch, "SAFE-UNSAFE-002")
}

@test
func neg_unsafe_func_pointer_field_call(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "unsafe func danger() : int { return 7 }\nstruct Holder {\n    var f : unsafe () => int\n}\npublic func main() : int {\n    var h = Holder { f : danger }\n    return h.f()\n}\n"
    expect_compile_error(env, "unsafe_func_pointer_field_call", ch, "SAFE-UNSAFE-002")
}

@test
func neg_inline_asm_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    asm(\"nop\")\n    return 0\n}\n"
    expect_compile_error(env, "inline_asm_requires_unsafe", ch, "SAFE-UNSAFE-003")
}

@test
func inline_asm_in_unsafe_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    unsafe { asm(\"nop\") }\n    return 0\n}\n"
    expect_compile_success(env, "inline_asm_unsafe", ch)
}

@test
func neg_mutable_global_access(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "var g : int = 3\npublic func main() : int {\n    return g\n}\n"
    expect_compile_error(env, "mutable_global_access", ch, "SAFE-GLOBAL-001")
}

@test
func mutable_global_access_in_unsafe_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "var g : int = 3\npublic func main() : int {\n    return unsafe(g)\n}\n"
    expect_compile_success(env, "mutable_global_access_unsafe", ch)
}

@test
func neg_immutable_global_non_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct S { var p : *int }\nconst g = S { p : null }\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error(env, "immutable_global_non_sync", ch, "SAFE-GLOBAL-002")
}

@test
func immutable_global_sync_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "const g : int = 3\npublic func main() : int {\n    return g\n}\n"
    expect_compile_success(env, "immutable_global_sync", ch)
}

@test
func neg_extern_global_access(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@extern var errno : int\npublic func main() : int {\n    return errno\n}\n"
    expect_compile_error(env, "extern_global_access", ch, "SAFE-GLOBAL-001")
}

@test
func neg_named_unsafe_flag_removed(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    unsafe \"no_lifetime_check\" { return 0 }\n}\n"
    expect_compile_error(env, "named_unsafe_flag_removed", ch, "named unsafe flags are not supported")
}

@test
func neg_conditional_where_generic_negative_applies(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using core::ops::Clone;\nstruct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\nstruct Wrapper<T> {\n    var value : T\n}\nimpl<T> !Sync for Wrapper<T> where T: Clone {}\npublic func main() : int {\n    var h = Holder<Wrapper<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "conditional_where_generic_negative", ch, "is declared !Sync", NEG_MOD_STD_CORE)
}

@test
func neg_unsafe_cell_is_not_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var h = Holder<std::UnsafeCell<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "unsafe_cell_not_sync", ch, "is declared !Sync", NEG_MOD_STD_CORE)
}

@test
func unsafe_cell_is_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var h = Holder<std::UnsafeCell<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "unsafe_cell_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_vector_get_ref_is_immutable(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.push(1)\n    var r = v.get_ref(0)\n    *r = 2\n    return v.get(0)\n}\n"
    expect_compile_error_with_mod(env, "vector_get_ref_immutable", ch, "cannot assign to a non mutable value", NEG_MOD_STD_CORE)
}

@test
func nested_unsafe_send_impl_is_honored(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The raw-pointer field is not Send structurally, but the nested type's
    // explicit unsafe impl must make the containing aggregate Send too.
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Raw {\n    var p : *mut int\n}\nunsafe impl Send for Raw {}\nstruct Nested {\n    var raw : Raw\n}\npublic func main() : int {\n    var h = Holder<Nested> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "nested_unsafe_send", ch, "does not satisfy", NEG_MOD_CORE)
}

@test
func neg_nested_raw_without_send_impl(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\nstruct Raw {\n    var p : *mut int\n}\nstruct Nested {\n    var raw : Raw\n}\npublic func main() : int {\n    var h = Holder<Nested> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "nested_raw_not_send", ch, "does not satisfy", NEG_MOD_CORE)
}

@test
func neg_non_null_is_not_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var h = Holder<std::NonNull<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "non_null_not_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_non_null_is_not_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var h = Holder<std::NonNull<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "non_null_not_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_manually_drop_follows_inner_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var d = std::manually_drop(unsafe(std::non_null_unchecked<int>(null)))\n    var h = Holder<std::ManuallyDrop<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "manually_drop_inner_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_maybe_uninit_follows_inner_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var m = std::maybe_uninit<std::NonNull<int>>()\n    var h = Holder<std::MaybeUninit<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "maybe_uninit_inner_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_box_follows_inner_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var b = std::box(unsafe(std::non_null_unchecked<int>(null)))\n    var h = Holder<std::Box<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "box_inner_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_box_follows_inner_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var b = std::box(unsafe(std::non_null_unchecked<int>(null)))\n    var h = Holder<std::Box<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "box_inner_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_rc_is_not_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var r = std::rc(1)\n    var h = Holder<std::Rc<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "rc_not_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_rc_is_not_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var r = std::rc(1)\n    var h = Holder<std::Rc<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "rc_not_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_unsafe_function_call_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "unsafe func danger() : int { return 1 }\npublic func main() : int {\n    var x = danger()\n    return x\n}\n"
    expect_compile_error(env, "unsafe_fn_call", ch, "SAFE-UNSAFE-002")
}

@test
func neg_unsafe_method_call_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Reader {\n    var value : int\n    unsafe func read(&self) : int { return self.value }\n}\npublic func main() : int {\n    var r = Reader { value : 1 }\n    return r.read()\n}\n"
    expect_compile_error(env, "unsafe_method_call", ch, "SAFE-UNSAFE-002")
}

@test
func neg_ref_cell_is_not_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var c = std::ref_cell(1)\n    var h = Holder<std::RefCell<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "ref_cell_not_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func ref_cell_is_send_when_inner_is_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var c = std::ref_cell(1)\n    var h = Holder<std::RefCell<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "ref_cell_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func mutex_is_sync_when_inner_is_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var m = std::new_mutex(1)\n    var h = Holder<std::Mutex<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "mutex_sync_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_mutex_not_sync_when_inner_is_not_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var m = std::new_mutex(unsafe(std::non_null_unchecked<int>(null)))\n    var h = Holder<std::Mutex<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "mutex_inner_not_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_deque_get_ref_is_immutable(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var d = std::deque<int>()\n    d.push(1)\n    *d.get_ref(0) = 2\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "deque_get_ref_immutable", ch, "cannot assign to a non mutable", NEG_MOD_STD_CORE)
}

@test
func deque_get_mut_is_allowed(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var d = std::deque<int>()\n    d.push(1)\n    *d.get_mut(0) = 2\n    return *d.get_ref(0)\n}\n"
    expect_compile_success_with_mod(env, "deque_get_mut_allowed", ch, NEG_MOD_STD_CORE)
}

@test
func neg_unsafe_cell_get_ref_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var c = std::unsafe_cell(1)\n    var r = c.get_ref()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "unsafe_cell_safe_get_ref", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_maybe_uninit_assume_init_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var m = std::maybe_uninit<int>()\n    var x = m.assume_init()\n    return x\n}\n"
    expect_compile_error_with_mod(env, "maybe_uninit_safe_assume", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_manually_drop_take_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var d = std::manually_drop(1)\n    var x = d.take()\n    return x\n}\n"
    expect_compile_error_with_mod(env, "manually_drop_safe_take", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_vector_set_len_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.push(1)\n    v.set_len(2)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "vector_set_len_requires_unsafe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func vector_set_len_allowed_in_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.push(1)\n    unsafe { v.set_len(1) }\n    return v.size() as int\n}\n"
    expect_compile_success_with_mod(env, "vector_set_len_unsafe_ok", ch, NEG_MOD_STD_CORE)
}

@test
func neg_function_instance_make2_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func dtor(obj : *mut void) { }\npublic func main() : int {\n    var f = std::default_function_instance.make2(null, null, dtor, 0, 0)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "function_instance_make2_requires_unsafe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_by_reference_callback_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var f : std::function<() => int> = |&x|() => { return *x }\n    return f()\n}\n"
    expect_compile_error_with_mod(env, "by_ref_callback_safe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_by_reference_callback_argument_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func take(f : std::function<() => int>) : int { return 0 }\npublic func main() : int {\n    var x = 1\n    return take(|&x|() => { return *x })\n}\n"
    expect_compile_error_with_mod(env, "by_ref_callback_arg_safe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_by_value_reference_capture_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var r = &x\n    var f : std::function<() => int> = |r|() => { return *r }\n    return f()\n}\n"
    expect_compile_error_with_mod(env, "by_value_reference_capture", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func by_reference_callback_in_unsafe_is_allowed(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    unsafe {\n        var f : std::function<() => int> = |&x|() => { return *x }\n        return f()\n    }\n}\n"
    expect_compile_output_not_contains(env, "by_ref_callback_unsafe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_returning_by_reference_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func make() : std::function<() => int> {\n    var x = 1\n    return |&x|() => { return *x }\n}\npublic func main() : int { return make()() }\n"
    expect_compile_error_with_mod(env, "capture_escape_direct", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
}

@test
func neg_returning_tainted_capture_variable_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func make() : std::function<() => int> {\n    var x = 1\n    var f : std::function<() => int> = |&x|() => { return *x }\n    return f\n}\npublic func main() : int { return make()() }\n"
    expect_compile_error_with_mod(env, "capture_escape_variable", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
}

@test
func neg_returning_struct_with_by_reference_capture_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder {\n    var f : std::function<() => int>\n}\nfunc make() : Holder {\n    var x = 1\n    return Holder { f : |&x|() => { return *x } }\n}\npublic func main() : int { return make().f() }\n"
    expect_compile_error_with_mod(env, "capture_escape_struct", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
}

@test
func returning_by_value_capture_is_allowed(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func make() : std::function<() => int> {\n    var x = 7\n    return |x|() => { return x }\n}\npublic func main() : int { return make()() }\n"
    expect_compile_output_not_contains(env, "capture_by_value_return", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
}

@test
func neg_function_instance_cannot_be_forged(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func dtor(obj : *mut void) { }\npublic func main() : int {\n    var f = std::default_function_instance {\n        fn_pointer : null,\n        fn_data_ptr : null,\n        buffer : zeroed<[32]char>(),\n        dtor : dtor,\n        is_heap : false\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "function_instance_forge", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_option_take_requires_mutable_receiver(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var o = std::Option.Some<int>(1)\n    const c = o\n    var x = c.take()\n    return x\n}\n"
    expect_compile_error_with_mod(env, "option_take_immutable", ch, "mutable implicit self", NEG_MOD_STD_CORE)
}

@test
func rwlock_is_send_and_sync_when_inner_qualifies(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send + Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var rw = std::new_rwlock(1)\n    var h = Holder<std::RwLock<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "rwlock_send_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func rwlock_is_send_when_inner_is_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var rw = std::new_rwlock(std::unsafe_cell(1))\n    var h = Holder<std::RwLock<std::UnsafeCell<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "rwlock_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_rwlock_not_sync_when_inner_is_not_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var rw = std::new_rwlock(std::unsafe_cell(1))\n    var h = Holder<std::RwLock<std::UnsafeCell<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "rwlock_inner_not_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func arc_is_send_and_sync_when_inner_qualifies(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send + Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var a = std::arc(1)\n    var h = Holder<std::Arc<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_output_not_contains(env, "arc_send_sync", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_arc_not_send_when_inner_is_not_send(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Send {\n        return 0\n    }\n}\npublic func main() : int {\n    var a = std::arc(unsafe(std::non_null_unchecked<int>(null)))\n    var h = Holder<std::Arc<std::NonNull<int>>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "arc_inner_not_send", ch, "does not satisfy", NEG_MOD_STD_CORE)
}

@test
func neg_option_clone_requires_clone(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var o = std::Option.Some<std::NonNull<int>>(unsafe(std::non_null_unchecked<int>(null)))\n    var c = o.clone()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "option_clone_bound", ch, "Clone", NEG_MOD_STD_CORE)
}

@test
func neg_vector_clone_requires_clone(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var v = std::vector<std::NonNull<int>>()\n    var c = v.clone()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "vector_clone_bound", ch, "Clone", NEG_MOD_STD_CORE)
}

@test
func waker_is_send_and_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> needs_send(v : T) : int where T : Send { return 0 }\nfunc <T> needs_sync(v : T) : int where T : Sync { return 0 }\npublic func main() : int {\n    var w = core::async::Waker.empty()\n    var w2 = core::async::Waker.empty()\n    return needs_send(w) + needs_sync(w2)\n}\n"
    expect_compile_success_with_mod(env, "waker_send_sync", ch, NEG_MOD_CORE)
}

@test
func waker_private_fields_cannot_be_initialized_directly(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var w = core::async::Waker { data : null, vtbl : null }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "waker_private_init", ch, "SAFE-VIS-001", NEG_MOD_CORE)
}

@test
func waker_from_raw_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var w = core::async::Waker.from_raw(null, null)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "waker_from_raw_safe", ch, "SAFE-UNSAFE-002", NEG_MOD_CORE)
}

@test
func waker_from_raw_unsafe_and_empty_are_usable(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var w = unsafe(core::async::Waker.from_raw(null, null))\n    var e = core::async::Waker.empty()\n    e.wake()\n    var e2 = e.clone()\n    e2.wake()\n    return 0\n}\n"
    expect_compile_success_with_mod(env, "waker_constructors", ch, NEG_MOD_CORE)
}

@test
func extern_call_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@extern func abs(v : int) : int\npublic func main() : int {\n    return abs(-1)\n}\n"
    expect_compile_error(env, "ffi_extern_call", ch, "SAFE-FFI-001")
}

@test
func extern_call_in_unsafe_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@extern func abs(v : int) : int\npublic func main() : int {\n    return unsafe(abs(-1))\n}\n"
    expect_compile_success(env, "ffi_extern_call_unsafe", ch)
}

@test
func neg_generic_unsafe_function_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Generic calls link to the GenericFuncDecl wrapper, not the concrete
    // FunctionDeclaration copy. The parser must put `unsafe` on its master.
    var ch = "unsafe func <T> danger(x : T) : T { return x }\npublic func main() : int {\n    var x = danger<int>(1)\n    return x\n}\n"
    expect_compile_error(env, "generic_unsafe_fn_call", ch, "SAFE-UNSAFE-002")
}

@test
func neg_thread_spawn_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `spawn` moves a raw entry/argument pair to another thread; the compiler
    // cannot prove the pointee outlives the thread or that captures are Send.
    var ch = "func entry(arg : *void) : *void { return null }\npublic func main() : int {\n    var t = std::concurrent.spawn(entry, null)\n    t.join()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "thread_spawn_requires_unsafe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_async_spawn_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The future frame's captures are erased, so `spawn` cannot check
    // Send/'static; submitting it is an explicit unsafe boundary.
    var ch = "async func one() : int { return 1 }\npublic func main() : int {\n    var h = async::spawn<int>(one())\n    return async::block_on<int>(h) - 1\n}\n"
    expect_compile_error_with_mod(env, "async_spawn_requires_unsafe", ch, "SAFE-UNSAFE-002", NEG_MOD_ASYNC_STD)
}

@test
func neg_thread_from_raw_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var t = std::concurrent.Thread.from_raw(0 as usize)\n    t.join()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "thread_from_raw_requires_unsafe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_thread_handle_cannot_be_forged(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var t = std::concurrent.Thread { handle : 0 as usize }\n    t.join()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "thread_handle_forged", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_extern_variant_by_value_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "variant Payload {\n    Value(n : int)\n}\n@extern func take(p : Payload) : int\npublic func main() : int {\n    var p = Payload.Value(1)\n    return unsafe(take(p))\n}\n"
    expect_compile_error(env, "extern_variant_by_value", ch, "SAFE-FFI-002")
}

@test
func neg_packed_field_shared_reference_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@packed struct Packed {\n    var tag : u8\n    var value : u32\n}\npublic func main() : int {\n    var p = Packed { tag : 1, value : 2 }\n    var r = &p.value\n    return 0\n}\n"
    expect_compile_error(env, "packed_field_shared_reference", ch, "SAFE-VALID-008")
}

@test
func neg_packed_field_mut_reference_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@repr(\"packed\") struct Packed {\n    var tag : u8\n    var value : u32\n}\npublic func main() : int {\n    var p = Packed { tag : 1, value : 2 }\n    var r = &mut p.value\n    return 0\n}\n"
    expect_compile_error(env, "packed_field_mut_reference", ch, "SAFE-VALID-008")
}

@test
func neg_nested_packed_field_reference_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@packed struct Inner { var x : int }\n@packed struct Outer { var inner : Inner }\npublic func main() : int {\n    var o = Outer { inner : Inner { x : 1 } }\n    var r = &o.inner.x\n    return 0\n}\n"
    expect_compile_error(env, "nested_packed_field_reference", ch, "SAFE-VALID-008")
}

@test
func neg_packed_array_element_reference_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@packed struct P { var xs : [4]int }\npublic func main() : int {\n    var p = P { xs : [1, 2, 3, 4] }\n    var r = &p.xs[1]\n    return 0\n}\n"
    expect_compile_error(env, "packed_array_element_reference", ch, "SAFE-VALID-008")
}

@test
func neg_transparent_requires_one_field(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@repr(\"transparent\")\nstruct Bad {\n    var a : int\n    var b : int\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "transparent_two_fields", ch, "SAFE-REPR-001")
}

@test
func neg_transparent_rejects_union(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@repr(\"transparent\")\nunion Bad {\n    var a : int\n    var b : uint\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "transparent_union", ch, "can only be applied to structs")
}

@test
func neg_transparent_rejects_packed(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "@repr(\"transparent\")\n@packed\nstruct Bad { var a : int }\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "transparent_packed", ch, "cannot be combined")
}

@test
func neg_callback_cannot_move_noncopy_capture(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var s = std::string(\"a long captured value\")\n    var f : std::function<() => std::string> = |s|() => {\n        return s\n    }\n    return f().size() as int\n}\n"
    expect_compile_error_with_mod(env, "callback_move_noncopy_capture", ch, "SAFE-MOVE-002", NEG_MOD_STD_CORE)
}

@test
func neg_int_to_bool_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var b = 2 as bool\n    return b as int\n}\n"
    expect_compile_error(env, "int_to_bool_rejected", ch, "SAFE-VALID-002")
}

@test
func neg_invalid_enum_literal_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "enum Color : int {\n    Red,\n    Green\n}\npublic func main() : int {\n    var c = 99 as Color\n    return c as int\n}\n"
    expect_compile_error(env, "invalid_enum_literal", ch, "SAFE-VALID-003")
}

@test
func neg_zeroed_enum_without_zero_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "enum Color : int {\n    Red = 1,\n    Green = 2\n}\npublic func main() : int {\n    var c = zeroed<Color>()\n    return 0\n}\n"
    expect_compile_error(env, "zeroed_enum_without_zero", ch, "SAFE-VALID-004")
}

@test
func neg_comptime_heap_out_of_bounds_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    comptime {\n        var p = unsafe(malloc(4)) as *mut int\n        unsafe { p[1] = 1 }\n        unsafe(free(p))\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "comptime_heap_oob", ch, "out-of-bounds write", NEG_MOD_CSTD)
}

@test
func neg_comptime_heap_use_after_free_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    comptime {\n        var p = unsafe(malloc(4)) as *mut int\n        unsafe(free(p))\n        var x = unsafe(p[0])\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "comptime_heap_uaf", ch, "out-of-bounds read", NEG_MOD_CSTD)
}

@test
func neg_non_null_private_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var nn = std::NonNull<int> { ptr = null }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "non_null_private_field", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_non_null_unchecked_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var nn = std::non_null_unchecked<int>(null)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "non_null_unchecked_safe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func runtime_enum_cast_is_checked(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "enum E : int {\n    A = 1,\n    B = 2\n}\nfunc bad(n : int) : int {\n    var e = n as E\n    return 0\n}\npublic func main() : int {\n    return bad(1)\n}\n"
    expect_compile_success(env, "runtime_enum_cast_safe", ch)
}

@test
func neg_box_private_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var b = std::Box<int> { ptr = null }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "box_private_field", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_box_unchecked_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var b = std::box_unchecked<int>(null)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "box_unchecked_safe", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_rc_private_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var r = std::Rc<int> { ptr = null }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "rc_private_field", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_weak_private_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var w = std::Weak<int> { ptr = null }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "weak_private_field", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_arc_private_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var a = std::Arc<int> { ptr = null }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "arc_private_field", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_arc_weak_private_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var w = std::ArcWeak<int> { ptr = null }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "arc_weak_private_field", ch, "SAFE-VIS-001", NEG_MOD_STD_CORE)
}

@test
func neg_vector_internal_storage_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var v = std::vector<int>()\n    v.data_size = 5\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "vector_internal_size", ch, "cannot access internal member", NEG_MOD_STD_CORE)
}

@test
func neg_string_private_state_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var s = std::string()\n    s.state = '1'\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "string_private_state", ch, "cannot access private member", NEG_MOD_STD_CORE)
}

@test
func neg_refcell_internal_state_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var c = std::ref_cell(1)\n    c.state = 0\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "refcell_internal_state", ch, "cannot access internal member", NEG_MOD_STD_CORE)
}

@test
func neg_span_internal_size_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var s = std::span<int>()\n    s._size = 10\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "span_internal_size", ch, "cannot access internal member", NEG_MOD_STD_CORE)
}

@test
func neg_map_internal_size_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var m = std::unordered_map<int, int>()\n    m._size = 10\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "map_internal_size", ch, "cannot access internal member", NEG_MOD_STD_CORE)
}

@test
func neg_string_private_storage_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var s = std::string()\n    s.storage.sso.length = 0\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "string_private_storage", ch, "cannot access private member", NEG_MOD_STD_CORE)
}

@test
func neg_raw_mutex_internal_storage_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var m = std::mutex()\n    m.storage = []\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "raw_mutex_internal_storage", ch, "cannot access internal member", NEG_MOD_STD_CORE)
}

@test
func neg_pin_from_raw_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p = core::Pin.from_raw<int>(null)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "pin_from_raw_safe", ch, "SAFE-UNSAFE-002", NEG_MOD_CORE)
}

@test
func neg_pin_internal_pointer_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var p = core::Pin<int> { ptr = &raw mut x }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "pin_internal_ptr", ch, "SAFE-VIS-001", NEG_MOD_CORE)
}

@test
func neg_chunk_private_fields_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var c = core::iterable::Chunk<int> { ptr = null, len = 1 }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "chunk_private_fields", ch, "SAFE-VIS-001", NEG_MOD_CORE)
}

@test
func neg_linear_safe_impl_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct BadLinear {\n    var data : [3]int\n}\nimpl core::iterable::Linear<int> for BadLinear {\n    func data(&self) : *int { return null }\n    func size(&self) : u64 { return 3 }\n}\npublic func main() : int {\n    var b = BadLinear { data : [1, 2, 3] }\n    var total = 0\n    for(var x in b) { total += x }\n    return total\n}\n"
    expect_compile_error_with_mod(env, "linear_safe_impl", ch, "SAFE-TRAIT-001", NEG_MOD_CORE)
}

@test
func neg_global_allocator_debug_private(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var a = std::alloc::GlobalAllocator.make()\n    a._debug = null\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "allocator_debug_private", ch, "cannot access private member", NEG_MOD_STD_CORE)
}

@test
func neg_generic_safe_impl_of_unsafe_interface(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "unsafe interface BadIface {\n    func value(&self) : int\n}\nstruct G<T> {\n    var x : T\n}\nimpl <T> BadIface for G<T> {\n    func value(&self) : int { return 0 }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "generic_safe_unsafe_iface", ch, "SAFE-TRAIT-001", NEG_MOD_CORE)
}

@test
func neg_string_view_raw_constructor_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *char = null\n    var v = std::string_view(p, 1)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "string_view_raw_ctor", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_span_raw_constructor_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *int = null\n    var v = std::span<int>(p, 1)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "span_raw_ctor", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_string_raw_constructor_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *char = null\n    var v = std::string(p, 1)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "string_raw_ctor", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_u16string_raw_constructor_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var p : *u16 = null\n    var v = std::u16string(p, 1)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "u16string_raw_ctor", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_volatile_access_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    @volatile\n    var flag : u32 = 0\n    flag = 1\n    return flag as int\n}\n"
    expect_compile_error(env, "volatile_access_safe", ch, "SAFE-VALID-006")
}

@test
func neg_union_field_access_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "union U {\n    var a : int\n    var b : float\n}\npublic func main() : int {\n    var u = U { a : 1 }\n    return u.a\n}\n"
    expect_compile_error(env, "union_field_safe", ch, "SAFE-VALID-007")
}

@test
func neg_command_line_stream_raw_write_requires_unsafe(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using namespace std;\npublic func main() : int {\n    var s = CommandLineStream {}\n    s.writeStr(null, 0)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "command_line_stream_raw_write", ch, "SAFE-UNSAFE-002", NEG_MOD_STD_CORE)
}

@test
func neg_static_mut_access(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "static mut g : int = 7\npublic func main() : int {\n    return g\n}\n"
    expect_compile_error(env, "static_mut_access", ch, "SAFE-GLOBAL-001")
}

@test
func static_immutable_access_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "static g : int = 7\npublic func main() : int {\n    return g\n}\n"
    expect_compile_success(env, "static_immutable_access", ch)
}

@test
func neg_static_non_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "static g : *int = null\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error(env, "static_non_sync", ch, "SAFE-GLOBAL-002")
}

@test
func neg_comptime_foreign_pointer_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    comptime {\n        var p = 0x1234 as *int\n        var x = unsafe(p[0])\n    }\n    return 0\n}\n"
    expect_compile_error(env, "comptime_foreign_pointer", ch, "out-of-bounds read")
}

@test
func neg_comptime_cross_frame_pointer_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "comptime func leak() : *int {\n    var x : int = 7\n    return unsafe(&raw mut x)\n}\npublic func main() : int {\n    comptime {\n        var p = leak()\n        var y = unsafe(p[0])\n    }\n    return 0\n}\n"
    expect_compile_error(env, "comptime_cross_frame_pointer", ch, "out-of-bounds read")
}

@test
func neg_comptime_printf_foreign_string_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    comptime {\n        printf(\"%s\", 0x1234 as *char)\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "comptime_printf_foreign_string", ch, "foreign", NEG_MOD_CSTD)
}


@test
func neg_thread_spawn_non_send_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func worker(x : std::Rc<int>) : int {\n    return x.strong_count() as int\n}\npublic func main() : int {\n    var h = std::thread.spawn<std::Rc<int>, int>(worker, std::rc(1))\n    return h.join() - 1\n}\n"
    expect_compile_error_with_mod(env, "thread_spawn_non_send", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_STD_CORE)
}

@test
func neg_thread_spawn_non_static_reference_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func worker(x : &int) : int {\n    return *x\n}\npublic func main() : int {\n    var x = 1\n    var h = std::thread.spawn<&int, int>(worker, &x)\n    return h.join() - 1\n}\n"
    expect_compile_error_with_mod(env, "thread_spawn_non_static", ch, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
}

@test
func neg_thread_spawn_void_non_static_reference_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func worker(x : &int) {\n}\npublic func main() : int {\n    var x = 1\n    var t = std::thread.spawn_void(worker, &x)\n    t.join()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "thread_spawn_void_non_static", ch, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
}

@test
func neg_thread_spawn_struct_with_non_static_reference_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder {\n    var r : &int\n}\nfunc worker(h : Holder) : int {\n    return *h.r\n}\npublic func main() : int {\n    var x = 1\n    var h = Holder { r : &x }\n    var t = std::thread.spawn<Holder, int>(worker, h)\n    return t.join() - 1\n}\n"
    expect_compile_error_with_mod(env, "thread_spawn_struct_non_static", ch, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
}

@test
func neg_thread_spawn_vector_of_non_send_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func worker(v : std::vector<std::Rc<int> >) : int {\n    return v.size() as int\n}\npublic func main() : int {\n    var v = std::vector<std::Rc<int> >()\n    v.push(std::rc(1))\n    var t = std::thread.spawn<std::vector<std::Rc<int> >, int>(worker, v)\n    return t.join() - 1\n}\n"
    expect_compile_error_with_mod(env, "thread_spawn_vector_non_send", ch, "does not satisfy where clause constraint 'T : Send'", NEG_MOD_STD_CORE)
}

@test
func neg_thread_spawn_mutex_guard_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func worker(g : std::MutexGuard<int>) : int {\n    return 0\n}\npublic func main() : int {\n    var m = std::new_mutex(1)\n    var g = m.lock()\n    var t = std::thread.spawn<std::MutexGuard<int>, int>(worker, g)\n    return t.join()\n}\n"
    expect_compile_error_with_mod(env, "thread_spawn_mutex_guard", ch, "is declared !Send", NEG_MOD_STD_CORE)
}

@test
func neg_mutex_guard_is_not_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var m = std::new_mutex(1)\n    var g = m.lock()\n    var h = Holder<std::MutexGuard<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "mutex_guard_not_sync", ch, "is declared !Sync", NEG_MOD_STD_CORE)
}

@test
func neg_rwlock_read_guard_is_not_sync(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<T> {\n    var data : *mut T\n    func check(&self) : int where T: Sync {\n        return 0\n    }\n}\npublic func main() : int {\n    var m = std::new_rwlock(1)\n    var g = m.read()\n    var h = Holder<std::RwLockReadGuard<int>> { data = null }\n    return h.check()\n}\n"
    expect_compile_error_with_mod(env, "rwlock_read_guard_not_sync", ch, "is declared !Sync", NEG_MOD_STD_CORE)
}

@test
func neg_thread_spawn_rwlock_guard_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func worker(g : std::RwLockWriteGuard<int>) : int {\n    return 0\n}\npublic func main() : int {\n    var m = std::new_rwlock(1)\n    var g = m.write()\n    var t = std::thread.spawn<std::RwLockWriteGuard<int>, int>(worker, g)\n    return t.join()\n}\n"
    expect_compile_error_with_mod(env, "thread_spawn_rwlock_guard", ch, "is declared !Send", NEG_MOD_STD_CORE)
}
