// Move Semantics Negative Tests
// Tests that the compiler correctly enforces move semantics.

@test
func neg_move_after_use(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    var c = a\n}\n"
    expect_compile_error(env, "move_after_use", ch, "cannot move")
}

@test
func neg_move_non_struct_to_struct(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = 42\n    var b : S = a\n}\n"
    expect_compile_error(env, "move_non_struct_to_struct", ch, "does not satisfy")
}

@test
func neg_move_twice(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    var c = a\n}\n"
    expect_compile_error(env, "move_twice", ch, "cannot move")
}

@test
func neg_zeroed_with_dtor(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var s = zeroed<S>()\n}\n"
    expect_compile_error(env, "zeroed_with_dtor", ch, "@allow_zeroed")
}

@test
func neg_move_into_fun_twice(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc consume(s : S) { }\nfunc main() {\n    var a = S { x : 1 }\n    consume(a)\n    consume(a)\n}\n"
    expect_compile_error(env, "move_into_fn_twice", ch, "cannot move")
}

@test
func neg_deref_non_copy_generic(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // De-referencing a `*T` where `T` is a generic parameter that is not `Copy`
    // is rejected (the value would be copied out of borrowed memory).
    var ch = "struct NonCopy {\n    @delete\n    func delete(&mut self) { }\n}\nfunc <T> get_val(p : *T) : T {\n    return *p\n}\nfunc main() {\n    var n = NonCopy {}\n    var p = &raw n\n    var v = get_val(p)\n}\n"
    expect_compile_error(env, "deref_non_copy_generic", ch, "Copy")
}

@test
func neg_index_non_copy_generic(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Indexing a `*T` where `T` is a generic parameter that is not `Copy` is rejected
    // (the element would be copied out of borrowed memory). Arrays of destructible
    // element types are supported, so this is the only restricted indexing form.
    var ch = "struct NonCopy {\n    @delete\n    func delete(&mut self) { }\n}\nfunc <T> get_at(p : *T) : T {\n    return p[0]\n}\nfunc main() {\n    var n = NonCopy {}\n    var p = &raw n\n    var v = get_at(p)\n}\n"
    expect_compile_error(env, "index_non_copy_generic", ch, "Copy")
}

@test
func array_of_destructible_elements_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // an array of a destructible element type is fine, each element is destroyed
    var ch = "struct DT {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var a = DT { x : 1 }\n    var b = DT { x : 2 }\n    var arr : [2]DT = [a, b]\n    return arr[0].x\n}\n"
    expect_compile_success(env, "array_of_destructible_elements_allowed", ch)
}

@test
func neg_move_unknown_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    b = a\n}\n"
    expect_compile_error(env, "move_unknown", ch, "cannot move")
}
