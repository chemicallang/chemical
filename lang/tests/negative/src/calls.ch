// Function Call Negative Tests
// Tests that the compiler correctly rejects invalid function calls.

@test
func neg_call_not_retained_delete(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct HasDtor {\n    @delete func cleanup(&mut self) { }\n}\nfunc main() {\n    var obj = HasDtor {}\n    obj.cleanup()\n}\n"
    expect_compile_error(env, "call_not_retained_dtor", ch, "cannot")
}

@test
func neg_call_mutable_self_immutable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    func set(&mut self, val : int) {\n        x = val\n    }\n}\nfunc main() {\n    const s = S { x : 1 }\n    s.set(5)\n}\n"
    expect_compile_error(env, "call_mut_on_const", ch, "requires a mutable implicit self")
}

@test
func neg_call_no_implicit_self(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    func get(&self) : int { return x }\n}\nfunc main() {\n    var v = S.get()\n}\n"
    expect_compile_error(env, "call_no_implicit_self", ch, "cannot call function without an implicit self arg")
}

@test
func neg_call_comptime_with_runtime(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "comptime func add(a : int, b : int) : int { return a + b }\nfunc main() {\n    var x = 5\n    add(x, 10)\n}\n"
    expect_compile_error(env, "call_comptime_runtime", ch, "comptime function expects argument")
}

@test
func neg_call_function_sig_unresolved(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func forward_decl() : int\nfunc main() {\n    var x = forward_decl()\n}\nfunc forward_decl() : int { return 42 }\n"
    // forward declarations should work - this tests a different pattern
    expect_compile_success(env, "forward_decl_ok", ch)
}

@test
func neg_call_unresolved_signature(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func forward_decl() : NonExistentType\nfunc main() {}\n"
    expect_compile_error(env, "call_unresolved_sig", ch, "unresolved type")
}

@test
func neg_call_wrong_args_count_str(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func foo(a : int, b : int) { }\nfunc main() {\n    foo(1, 2, 3, 4)\n}\n"
    expect_compile_error(env, "wrong_args_count_str", ch, "too many arguments given")
}

@test
func neg_call_not_enough_args_str(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func foo(a : int, b : int, c : int) { }\nfunc main() {\n    foo(1, 2)\n}\n"
    expect_compile_error(env, "not_enough_args_str", ch, "doesn't have a default value")
}

@test
func neg_call_comptime_with_runtime_complex(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "comptime func compute(x : int) : int { return x * 2 }\nfunc helper() : int { return 42 }\nfunc main() {\n    compute(helper())\n}\n"
    expect_compile_error(env, "call_comptime_runtime_complex", ch, "comptime function expects argument")
}

@test
func neg_call_temporary_with_lifetime(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct View 'a {\n    var data : *char\n}\nstruct Obj {\n    func get_view(&self) : 'self View {\n        return View { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var v = Obj().get_view()\n}\n"
    expect_compile_error(env, "call_temporary_lifetime", ch, "temporary that is destroyed")
}
