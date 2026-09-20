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
    expect_compile_error(env, "impl_missing_method_full", ch, "does not implement interface member")
}

@test
func neg_impl_wrong_method_sig_full(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Printable {\n    func print(&self) : int\n}\nstruct MyType {}\nimpl Printable for MyType {\n    func print(&self) : float { return 1.0 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_wrong_ret_type", ch, "does not satisfy")
}

@test
func neg_impl_missing_method_name(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface HasSize {\n    func size(&self) : int\n}\nstruct MyType {}\nimpl HasSize for MyType {}\nfunc main() {}\n"
    expect_compile_error(env, "impl_missing_method_name2", ch, "does not implement interface member")
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
func neg_inheritance_cycle_three(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // a cycle longer than two nodes must be caught too, not just `A : A` / `A : B, B : A`
    var ch = "struct A : B {}\nstruct B : C {}\nstruct C : A {}\nfunc main() {}\n"
    expect_compile_error(env, "cycle_three", ch, "recursion in inheritance")
}

@test
func neg_inheritance_interface_self(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // interfaces can inherit each other, so their cycle must be rejected as well
    var ch = "interface I : I {\n    func f(&self)\n}\nfunc main() {}\n"
    expect_compile_error(env, "interface_self_inheritance", ch, "recursion in inheritance")
}

@test
func neg_inherit_pointer_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // a pointer type has no linked declaration node; the inheritance check must
    // report it instead of dereferencing a null node
    var ch = "struct Derived : *char {}\nfunc main() {}\n"
    expect_compile_error(env, "inherit_pointer_type", ch, "must be a struct")
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
    // `@static` is an annotation, not a `static` keyword
    var ch = "@static\ninterface Unique {\n    func id(&self) : int\n}\nstruct A {}\nimpl Unique for A {\n    func id(&self) : int { return 1 }\n}\nstruct B {}\nimpl Unique for B {\n    func id(&self) : int { return 2 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "static_interface_multi_impl", ch, "static interface must have only")
}

@test
func neg_inherit_duplicate_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // both fields would still need their own initializer, so `B { x = 1 }` used to
    // emit a struct value with a missing member and then malformed C
    var ch = "struct A {\n    var x : int\n}\nstruct B : A {\n    var x : int\n}\nfunc main() {}\n"
    expect_compile_error(env, "inherit_duplicate_field", ch, "already exists in an inherited type")
}

@test
func neg_inherit_duplicate_field_deep_base(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // the collision is with a field of a base several levels up
    var ch = "struct A {\n    var x : int\n}\nstruct B : A {\n}\nstruct C : B {\n    var x : int\n}\nfunc main() {}\n"
    expect_compile_error(env, "inherit_duplicate_field_deep", ch, "already exists in an inherited type")
}

@test
func inherit_distinct_field_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // a derived struct may declare fields of its own, as long as the names are distinct
    var ch = "struct A {\n    var x : int\n}\nstruct B : A {\n    var y : int\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_success(env, "inherit_distinct_field_allowed", ch)
}

@test
func neg_impl_missing_for(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // an implementation must name the type it is implemented for. Without the 'for'
    // keyword the declaration has no target type at all, which used to be accepted
    // silently and crashed the compiler in a later pass
    var ch = "interface I {\n    func f(&self) : int\n}\nimpl I {\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_missing_for", ch, "expected 'for'")
}

@test
func neg_impl_missing_target_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // the 'for' keyword was written, so a type must follow it. Leaving the target type
    // absent made the declaration look like an implementation without a target type,
    // which the generic instantiation pass dereferenced until it crashed
    var ch = "interface I {\n    func f(&self) : int\n}\nimpl I for {\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_missing_target_type", ch, "expected a type after 'for'")
}

@test
func neg_impl_declares_generic_params(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // `impl <T> I for S<T>` never produced a usable implementation: the parameters of
    // the implemented type are the ones that are used, so declaring them is rejected
    var ch = "interface I {\n    func f(&self) : int\n}\nstruct S <T> {\n    var a : T\n}\nimpl <T> I for S<T> {\n    func f(&self) : int { return 1 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_declares_generic_params", ch, "don't declare generic parameters")
}

@test
func neg_impl_generic_params_nested(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // same rule inside a generic struct, where it used to be reported as a duplicate
    // symbol for the parameter the implementation declared
    var ch = "interface I {\n    func f(&self) : int\n}\nstruct S <T> {\n    var a : T\n    impl <T> I for S<T> {\n        func f(&self) : int { return 1 }\n    }\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_generic_params_nested", ch, "don't declare generic parameters")
}

@test
func neg_impl_self_as_target(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // 'Self' cannot be used as the type an implementation is for, because the type is
    // not known yet at that point. It used to leave the target type unlinked, which
    // crashed symbol resolution
    var ch = "interface I {\n    func f(&self) : int\n}\nimpl I for Self {\n    func f(&self) : int { return 1 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_self_as_target", ch, "cannot implement unsupported declaration")
}

@test
func neg_impl_missing_interface_name(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {}\nimpl for S {\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_missing_interface_name", ch, "expected keyword or identifier for interface type")
}

@test
func impl_inside_generic_struct_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // the supported form of an implementation for a generic type is a nested one, which
    // uses the parameters of the containing type and is instantiated along with it
    var ch = "interface I {\n    func f(&self) : int\n}\nstruct S <T> {\n    var a : T\n    impl I for S<T> {\n        func f(&self) : int { return 1 }\n    }\n}\npublic func main() : int {\n    var s = S<int>{a: 1}\n    return s.f() - 1\n}\n"
    expect_compile_success(env, "impl_inside_generic_struct", ch)
}

@test
func neg_impl_for_array_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // an array type has no members container, so no interface can be implemented for it
    var ch = "interface Empty {}\nimpl Empty for [3]int {\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_for_array_type", ch, "cannot implement unsupported type")
}
