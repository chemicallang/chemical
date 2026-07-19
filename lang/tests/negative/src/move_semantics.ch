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
    expect_compile_error(env, "move_non_struct_to_struct", ch, "cannot move a struct to a non struct")
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
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var s = zeroed<S>\n}\n"
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
    var ch = "struct Box 'T {\n    var val : T\n    func get(&self) : T { return val }\n}\nstruct NonCopy {\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var b = Box<NonCopy> { val : NonCopy {} }\n}\n"
    // Accessing a value type that's not Copy through a reference
    expect_compile_error(env, "deref_non_copy_generic", ch, "Copy")
}

@test
func neg_index_non_copy_generic(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct NonCopy {\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var arr : [3]NonCopy = []\n}\n"
    // Array of non-copy types may be restricted
    expect_compile_error(env, "arr_non_copy", ch, "Copy")
}

@test
func neg_move_unknown_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    b = a\n}\n"
    expect_compile_error(env, "move_unknown", ch, "cannot move")
}
