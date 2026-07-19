// Syntax & Parser Negative Tests
// Tests that the compiler correctly rejects syntactically invalid code.

@test
func neg_lifetime_dep_syntax(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyView 'a {\n    var data : *char\n}\nstruct MyObj {\n    func get_view(&self) : 'self MyView {\n        return MyView { data : null }\n    }\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var v = MyObj().get_view()\n}\n"
    expect_compile_error(env, "lifetime_dep_syntax", ch, "lifetime")
}

@test
func neg_undeclared_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x : NonExistentType = 5\n}\n"
    expect_compile_error(env, "undeclared_type", ch, "undeclared")
}

@test
func neg_unresolved_identifier(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = some_undefined_variable\n}\n"
    expect_compile_error(env, "unresolved_identifier", ch, "unresolved")
}

@test
func neg_void_variable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x : void\n}\n"
    expect_compile_error(env, "void_variable", ch, "void")
}

@test
func neg_duplicate_function(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func foo() : int { return 1 }\nfunc foo() : int { return 2 }\nfunc main() {}\n"
    expect_compile_error(env, "duplicate_function", ch, "already exists")
}

@test
func neg_assign_to_immutable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    const x = 5\n    x = 10\n}\n"
    expect_compile_error(env, "assign_to_immutable", ch, "cannot assign")
}

@test
func neg_wrong_return_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_int() : int {\n    return \"hello\"\n}\nfunc main() {}\n"
    expect_compile_error(env, "wrong_return_type", ch, "does not satisfy")
}

@test
func neg_cannot_take_addr_of_rvalue(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var p = &raw 42\n}\n"
    expect_compile_error(env, "addr_of_rvalue", ch, "r-value")
}

@test
func neg_type_mismatch_var_init(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x : int = \"not a number\"\n}\n"
    expect_compile_error(env, "type_mismatch_var_init", ch, "does not satisfy")
}

@test
func neg_undefined_field_in_struct(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Point {\n    var x : int\n    var y : int\n}\nfunc main() {\n    var p = Point { x : 1, y : 2, z : 3 }\n}\n"
    expect_compile_error(env, "extra_field_struct", ch, "unresolved child")
}

@test
func neg_unresolved_child_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Point {\n    var x : int\n}\nfunc main() {\n    var p = Point {}\n    var z = p.z\n}\n"
    expect_compile_error(env, "unresolved_child_field_access", ch, "unresolved child")
}

@test
func neg_cannot_assign_non_mutable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S { var x : int }\nfunc main() {\n    var s = S { x : 1 }\n    s.x = 10\n}\n"
    expect_compile_error(env, "assign_non_mutable_field", ch, "cannot assign")
}

@test
func neg_export_not_top_level(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    export func inner() {}\n}\n"
    expect_compile_error(env, "export_not_top_level", ch, "top level")
}
