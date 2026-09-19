// Miscellaneous Negative Tests
// Tests for import, annotation, interface, and other errors.

@test
func neg_import_non_existent(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "import \"non_existent_module\"\nfunc main() {}\n"
    expect_compile_error(env, "import_non_existent", ch, "import")
}

@test
func neg_interface_missing_method(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Printable {\n    func print(&self)\n}\nstruct MyType {\n    var x : int\n}\nimpl Printable for MyType {\n    // missing print method\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_missing_method", ch, "does not implement interface member")
}

@test
func neg_interface_wrong_method_sig(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Printable {\n    func print(&self)\n}\nstruct MyType {\n    var x : int\n}\nimpl Printable for MyType {\n    func print(&self, extra : int) { }\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_wrong_sig", ch, "implementation")
}

@test
func neg_invalid_annotation(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "@nonexistent_annotation\nfunc main() {}\n"
    expect_compile_error(env, "unknown_annotation", ch, "annotation")
}

@test
func neg_using_on_non_module(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using struct Foo { var x : int }\nfunc main() {}\n"
    expect_compile_error(env, "using_non_module", ch, "using")
}

@test
func neg_namespace_conflict(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo { var x : int }\nnamespace Foo { }\nfunc main() {}\n"
    expect_compile_error(env, "namespace_conflict", ch, "already exists")
}

@test
func impl_for_primitive_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // primitive types intentionally support interface implementations, and the
    // implementation's methods resolve on values of that primitive type
    var ch = "interface Printable {\n    func print(&self) : int\n}\nimpl Printable for int {\n    func print(&self) : int { return 1 }\n}\npublic func main() : int {\n    var x = 5\n    return x.print()\n}\n"
    expect_compile_success(env, "impl_for_primitive_ok", ch)
}

@test
func neg_unsafe_block_in_safe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    unsafe {\n        var x = 42\n    }\n    return 0\n}\n"
    // unsafe blocks should be fine in default mode
    expect_compile_success(env, "unsafe_block_ok", ch)
}

@test
func neg_infinite_loop_no_progress(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    while(true) { }\n    return 0\n}\n"
    // This is valid syntactically - just an infinite loop
    expect_compile_success(env, "infinite_loop_ok", ch)
}

@test
func neg_typealias_invalid(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // The alias must actually be used: an unused typealias is never resolved,
    // so referencing a non-existent type in one only errors once it is used.
    var ch = "typealias MyInt = NonExistentType\nvar g : MyInt = 0\nfunc main() {}\n"
    expect_compile_error(env, "typealias_invalid", ch, "unresolved")
}

@test
func neg_access_private_other_file(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "module test_module\nsource \".\"\nimport \"other\"\nfunc main() {\n    var x = other::hidden_value\n}\n"
    // This tests module-level access control - private symbols from other files
    expect_compile_error(env, "access_private", ch, "private")
}

@test
func neg_annotation_on_wrong_target(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "@delete\nstruct MyStruct {}\nfunc main() {}\n"
    expect_compile_error(env, "delete_on_struct", ch, "delete")
}

@test
func neg_invalid_number_conversion(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // a struct value cannot be reinterpreted as a primitive type. (A pointer to a string
    // literal can be cast to an integer, that is a legitimate pointer to integer cast.)
    var ch = "struct S {\n    var x : int\n}\nfunc main() {\n    var s = S { x : 1 }\n    var y = s as int\n}\n"
    expect_compile_error(env, "struct_to_int_cast", ch, "cannot be cast")
}

@test
func neg_cast_struct_to_pointer(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // a struct value is not a machine representation that a pointer can name. Take its
    // address first (`&raw s as *void`) if a pointer is wanted.
    var ch = "struct S {\n    var x : int\n}\nfunc main() {\n    var s = S { x : 1 }\n    var p = s as *void\n}\n"
    expect_compile_error(env, "struct_to_pointer_cast", ch, "cannot be cast")
}

@test
func neg_cast_struct_to_bool(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n}\nfunc main() {\n    var s = S { x : 1 }\n    var b = s as bool\n}\n"
    expect_compile_error(env, "struct_to_bool_cast", ch, "cannot be cast")
}

@test
func pointer_to_int_cast_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // an explicit pointer to integer cast is supported
    var ch = "public func main() : int {\n    var x = 42\n    var p : *int = &raw x\n    var addr = p as ulong\n    return (addr != 0) as int\n}\n"
    expect_compile_success(env, "pointer_to_int_cast_allowed", ch)
}
