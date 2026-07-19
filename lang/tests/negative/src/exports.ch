// Export & Import Negative Tests
// Tests that the compiler correctly rejects invalid export and import statements.

@test
func neg_export_from_current_module(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {}\nexport main\n"
    expect_compile_error(env, "export_current_module", ch, "cannot export a symbol from the current module")
}

@test
func neg_export_unresolved_symbol(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "export nonexistent_symbol\nfunc main() {}\n"
    expect_compile_error(env, "export_unresolved", ch, "unresolved symbol")
}

@test
func neg_export_unsupported_decl(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "export 42\nfunc main() {}\n"
    expect_compile_error(env, "export_unsupported_decl", ch, "unsupported declaration")
}

@test
func neg_export_top_level_stmt(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    export main\n}\n"
    expect_compile_error(env, "export_non_top_level", ch, "top level statement")
}

@test
func neg_export_unresolved_parent(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S { var x : int }\nexport unknown_module::S\nfunc main() {}\n"
    expect_compile_error(env, "export_unresolved_parent", ch, "unresolved symbol")
}

@test
func neg_import_bad_module_path(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "module neg_test\nsource \".\"\nimport \"/nonexistent/path\"\nfunc main() {}\n"
    expect_compile_error(env, "import_bad_path", ch, "import")
}

@test
func neg_export_non_public(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func hidden() {}\nexport hidden\nfunc main() {}\n"
    // hidden is private by default, exporting it should work since it's in the same file
    expect_compile_error(env, "export_non_public", ch, "cannot export")
}
