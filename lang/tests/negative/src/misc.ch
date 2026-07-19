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
    expect_compile_error(env, "impl_missing_method", ch, "implementation")
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
func neg_impl_for_non_struct(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Printable {\n    func print(&self)\n}\nimpl Printable for int {\n    func print(&self) { }\n}\nfunc main() {}\n"
    expect_compile_error(env, "impl_for_primitive", ch, "cannot")
}

@test
func neg_unsafe_block_in_safe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    unsafe {\n        var x = 42\n    }\n}\n"
    // unsafe blocks should be fine in default mode
    expect_compile_success(env, "unsafe_block_ok", ch)
}

@test
func neg_infinite_loop_no_progress(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    while(true) { }\n}\n"
    // This is valid syntactically - just an infinite loop
    expect_compile_success(env, "infinite_loop_ok", ch)
}

@test
func neg_typealias_invalid(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "typealias MyInt = NonExistentType\nfunc main() {}\n"
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
    var ch = "func main() {\n    var x = \"123\" as int\n}\n"
    expect_compile_error(env, "string_to_int_cast", ch, "cast")
}
