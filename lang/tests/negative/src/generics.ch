// Generic & Instantiation Negative Tests
// Tests that the compiler correctly rejects invalid generic type usage.

@test
func neg_generic_wrong_arg_count(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Box 'T {\n    var value : T\n}\nfunc main() {\n    var b = Box<int, float> { value : 42 }\n}\n"
    expect_compile_error(env, "generic_wrong_arg_count", ch, "argument")
}

@test
func neg_generic_missing_type_arg(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Box 'T {\n    var value : T\n}\nfunc main() {\n    var b = Box { value : 42 }\n}\n"
    expect_compile_error(env, "generic_missing_arg", ch, "infer")
}

@test
func neg_generic_non_type_arg(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Box 'T {\n    var value : T\n}\nfunc main() {\n    var b = Box<42> { value : 42 }\n}\n"
    expect_compile_error(env, "generic_non_type_arg", ch, "expected")
}

@test
func neg_generic_where_clause_fail(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface HasSize {\n    func get_size() : int\n}\nstruct Box 'T {\n    var value : T\n}\nimpl 'T HasSize for Box 'T {\n    func get_size() : int { return 1 }\n}\nfunc main() {\n    var b = Box<int> { value : 42 }\n}\n"
    // This might not actually error - depends on where clause support
    expect_compile_error(env, "generic_where_fail", ch, "generic")
}

@test
func neg_generic_recursive(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Node 'T {\n    var next : Node<Node<'T>>\n}\nfunc main() {}\n"
    expect_compile_error(env, "generic_recursive", ch, "infinite")
}

@test
func neg_generic_const_mismatch_int(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Array 'T 'N : int {\n    var data : [N]T\n}\nfunc main() {\n    var arr : Array<int, \"hello\">\n}\n"
    expect_compile_error(env, "generic_const_mismatch", ch, "expected")
}

@test
func neg_generic_missing_constraint(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Printable {\n    func print(&self)\n}\nfunc print_it 'T(value : T) where T : Printable {\n    value.print()\n}\nfunc main() {\n    print_it(42)\n}\n"
    expect_compile_error(env, "generic_no_constraint", ch, "constraint")
}

@test
func neg_duplicate_generic_param_name(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo 'T 'T {\n    var a : T\n    var b : T\n}\nfunc main() {}\n"
    expect_compile_error(env, "dup_generic_param", ch, "already exists")
}

@test
func neg_generic_struct_no_impl(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Addable {\n    func add(&self, other : &self) : self\n}\nstruct Pair 'T {\n    var first : T\n    var second : T\n}\nfunc add_pairs 'T(a : Pair<'T>, b : Pair<'T>) : Pair<'T> where T : Addable {\n    return Pair<'T> { first : a.first.add(&b.first), second : a.second.add(&b.second) }\n}\nfunc main() {\n    var p = Pair<int> { first : 1, second : 2 }\n    add_pairs(p, p)\n}\n"
    expect_compile_error(env, "generic_no_impl", ch, "no implementation")
}

@test
func neg_generic_struct_with_non_type_param(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Wrapper 'T {\n    var val : T\n}\nfunc main() {\n    var w : Wrapper<NonExistent>\n}\n"
    expect_compile_error(env, "generic_non_existent_type", ch, "unresolved")
}
