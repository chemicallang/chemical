// Namespace & Access Negative Tests
// Tests that the compiler correctly rejects invalid namespace usage and access patterns.

@test
func neg_namespace_expected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "namespace Math {\n    func add(a : int, b : int) : int { return a + b }\n}\nfunc main() {\n    Math.add(1, 2)\n}\n"
    // This should work - valid namespace
    expect_compile_success(env, "namespace_ok", ch)
}

@test
func neg_namespace_not_found(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = UnknownNamespace::value\n}\n"
    expect_compile_error(env, "namespace_not_found", ch, "expected value to be a namespace")
}

@test
func neg_value_not_namespace(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "var x = 42\nfunc main() {\n    var y = x::something\n}\n"
    expect_compile_error(env, "value_not_namespace", ch, "expected value to be a namespace")
}

@test
func neg_access_specifier_namespace(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "private namespace Hidden {\n    func do_it() { }\n}\nfunc main() {\n    Hidden.do_it()\n}\n"
    // Private namespace in the same file should be accessible
    expect_compile_success(env, "private_namespace_same_file_ok", ch)
}

@test
func neg_symbol_outside_lambda(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var fn = || {\n        return outside_var\n    }\n}\n"
    expect_compile_error(env, "symbol_outside_lambda", ch, "outside of lambda scope")
}

@test
func neg_alias_incompatible_value(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "typealias MyInt = int\nvar x : MyInt = \"hello\"\nfunc main() {}\n"
    expect_compile_error(env, "alias_incompatible", ch, "does not satisfy")
}

@test
func neg_incompatible_alias(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "typealias Number = int\ntypealias Text = *char\nvar x : Number = Text\nfunc main() {}\n"
    expect_compile_error(env, "incompatible_alias2", ch, "incompatible")
}

@test
func neg_expect_interface_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {}\nimpl SomeNonExistentInterface for S {}\nfunc main() {}\n"
    expect_compile_error(env, "expect_interface_type", ch, "expected type to be an interface")
}

@test
func neg_impl_for_unsupported_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Empty {}\nimpl Empty for *char {\n    // unsupported\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_unsupported_type", ch, "cannot implement unsupported type")
}

@test
func neg_impl_unsupported_decl(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "impl int for *char {}\nfunc main() {}\n"
    expect_compile_error(env, "impl_unsupported_decl", ch, "cannot implement unsupported declaration")
}
