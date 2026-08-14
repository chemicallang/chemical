// Switch/Case & Control Flow Negative Tests
// Tests that the compiler correctly rejects invalid switch statements and control flow.

@test
func neg_switch_no_default_last(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 1\n    switch(x) {\n        1 => { return 0 }\n    }\n}\n"
    // switch without a default case as the last statement of the function
    expect_compile_error(env, "switch_no_default_last", ch, "missing default case")
}

@test
func neg_switch_not_integer_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = \"hello\"\n    switch(x) {\n        \"hi\" => { }\n        default => { }\n    }\n}\n"
    expect_compile_error(env, "switch_not_integer", ch, "switch expression should have integer like type")
}

@test
func neg_switch_not_all_cases(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "enum Color : int {\n    Red\n    Green\n    Blue\n}\npublic func main() : int {\n    var c = Color.Red\n    switch(c) {\n        Color.Red => { }\n        default => { }\n    }\n    return 0\n}\n"
    expect_compile_success(env, "switch_enum_with_default_ok", ch)
}

@test
func neg_switch_last_stmt_no_default(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_val(x : int) : int {\n    switch(x) {\n        1 => { return 10 }\n        default => { return 20 }\n    }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "switch_with_default_ok", ch)
}

@test
func neg_switch_enum_missing_cases(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "enum Color : int {\n    Red\n    Green\n    Blue\n}\nfunc get_name(c : Color) : *char {\n    switch(c) {\n        Color.Red => { return \"red\" }\n        Color.Green => { return \"green\" }\n        // missing Blue case and no default\n    }\n}\nfunc main() {}\n"
    // switch without all cases and no default is rejected as a missing default case
    expect_compile_error(env, "switch_enum_missing_cases", ch, "missing default case")
}

@test
func neg_switch_variant_missing_cases(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "variant Result {\n    Ok(v : int)\n    Err(e : *char)\n}\nfunc handle(r : Result) {\n    switch(r) {\n        Ok(x) => { }\n        // missing Err case and no default\n    }\n}\nfunc main() {}\n"
    expect_compile_error(env, "switch_variant_missing_cases", ch, "expected all cases")
}

@test
func neg_switch_default_missing_last_stmt(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func get_val(x : int) : int {\n    switch(x) {\n        1 => { return 10 }\n    }\n    // fallthrough - no return after switch\n}\nfunc main() {}\n"
    expect_compile_error(env, "switch_default_missing_last_stmt", ch, "missing default case")
}

@test
func neg_switch_non_exhaustive_cases(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "enum Color : int {\n    Red\n    Green\n}\nfunc is_red(c : Color) : bool {\n    switch(c) {\n        Color.Red => { return true }\n    }\n}\nfunc main() {}\n"
    // switch without all cases and no default is rejected as a missing default case
    expect_compile_error(env, "switch_non_exhaustive", ch, "missing default case")
}

@test
func neg_switch_bool_not_int(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var b = true\n    switch(b) {\n        true => { }\n        default => { }\n    }\n}\n"
    expect_compile_error(env, "switch_bool_not_int", ch, "switch expression should have integer like type")
}
