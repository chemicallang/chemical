// Inheritance Negative Tests
// Tests that the compiler correctly rejects invalid inheritance.

@test
func neg_inherit_non_struct(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Base { var x : int }\nstruct Derived : int {}\nfunc main() {}\n"
    expect_compile_error(env, "inherit_non_struct", ch, "must be a struct")
}

@test
func neg_inherit_non_struct_second(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Base1 { var x : int }\nstruct Base2 { var y : int }\nstruct Derived : Base1, Base2 {}\nfunc main() {}\n"
    expect_compile_error(env, "inherit_non_empty_second", ch, "not empty")
}

@test
func neg_interface_inherit_non_interface(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Base {}\ninterface Derived : Base {}\nfunc main() {}\n"
    expect_compile_error(env, "interface_inherit_non_interface", ch, "interfaces can only inherit interfaces")
}

@test
func neg_interface_missing_method_full(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Printable {\n    func print(&self)\n}\nstruct MyType {}\nimpl Printable for MyType {}\nfunc main() {}\n"
    expect_compile_error(env, "impl_missing_method_full", ch, "no implementation of interface")
}

@test
func neg_impl_wrong_method_sig_full(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Printable {\n    func print(&self) : int\n}\nstruct MyType {}\nimpl Printable for MyType {\n    func print(&self) : float { return 1.0 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_wrong_ret_type", ch, "does not implement interface member")
}

@test
func neg_impl_missing_method_name(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface HasSize {\n    func size(&self) : int\n}\nstruct MyType {}\nimpl HasSize for MyType {}\nfunc main() {}\n"
    expect_compile_error(env, "impl_missing_method_name2", ch, "no implementation of interface")
}

@test
func neg_inheritance_recursive(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct A : B {}\nstruct B : A {}\nfunc main() {}\n"
    expect_compile_error(env, "recursive_inheritance", ch, "recursion in inheritance")
}

@test
func neg_inheritance_self(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct A : A {}\nfunc main() {}\n"
    expect_compile_error(env, "self_inheritance", ch, "recursion in inheritance")
}

@test
func neg_interface_duplicate_method(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Foo {\n    func do_it(&self)\n    func do_it(&self) : int\n}\nfunc main() {}\n"
    expect_compile_error(env, "interface_dup_method", ch, "already exists")
}

@test
func neg_static_interface_multi_impl(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "static interface Unique {\n    func id(&self) : int\n}\nstruct A {}\nimpl Unique for A {\n    func id(&self) : int { return 1 }\n}\nstruct B {}\nimpl Unique for B {\n    func id(&self) : int { return 2 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "static_interface_multi_impl", ch, "static interface must have only")
}

@test
func neg_impl_for_primitive_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Foo {\n    func bar(&self)\n}\nimpl Foo for int {\n    func bar(&self) { }\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_for_primitive_type", ch, "implementation for type is not allowed")
}
