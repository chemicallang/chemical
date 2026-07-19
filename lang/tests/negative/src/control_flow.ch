// Control Flow Negative Tests
// Tests that the compiler correctly rejects invalid control flow usage.

@test
func neg_else_without_if(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    else { }\n}\n"
    expect_compile_error(env, "else_without_if", ch, "expected")
}

@test
func neg_if_condition_not_bool(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    if(42) { }\n}\n"
    expect_compile_error(env, "if_cond_not_bool", ch, "bool")
}

@test
func neg_while_condition_not_bool(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    while(\"hello\") { }\n}\n"
    expect_compile_error(env, "while_cond_not_bool", ch, "bool")
}

@test
func neg_for_loop_bad_range(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    for(var i = 0; \"hello\"; i++) { }\n}\n"
    expect_compile_error(env, "for_cond_not_bool", ch, "bool")
}

@test
func neg_for_in_non_iterable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    for(var x in 42) { }\n}\n"
    expect_compile_error(env, "for_in_non_iterable", ch, "iterable")
}

@test
func neg_switch_no_expr(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    switch {\n        case 1 => { }\n    }\n}\n"
    expect_compile_error(env, "switch_no_expr", ch, "expected")
}

@test
func neg_switch_duplicate_case(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = 1\n    switch(x) {\n        case 1 => { }\n        case 1 => { }\n    }\n}\n"
    expect_compile_error(env, "switch_dup_case", ch, "duplicate")
}

@test
func neg_while_loop_no_body(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    while(true)\n}\n"
    expect_compile_error(env, "while_no_body", ch, "expected")
}

@test
func neg_for_loop_no_body(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    for(var i = 0; i < 10; i++)\n}\n"
    expect_compile_error(env, "for_no_body", ch, "expected")
}

@test
func neg_if_else_chain_type_mismatch(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var x = if(true) 42 else \"hello\"\n}\n"
    expect_compile_error(env, "if_else_type_mismatch", ch, "type")
}
