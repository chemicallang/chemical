// Borrow-checker (S4.4a) negative/positive tests. The checker is report-only
// today: SAFE-BORROW-* are warnings and compilation still succeeds.

@test
func neg_borrow_mutable_alias_warns(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 1\n    var a = &mut x\n    var b = &mut x\n    *a = 2\n    *b = 3\n    return x\n}\n"
    expect_compile_warning_id(env, "borrow_mutable_alias", ch, "SAFE-BORROW-001")
}

@test
func neg_borrow_shared_mutable_alias_warns(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 1\n    var a = &x\n    var b = &mut x\n    *b = 2\n    return *a\n}\n"
    expect_compile_warning_id(env, "borrow_shared_mutable_alias", ch, "SAFE-BORROW-001")
}

@test
func sequential_mutable_borrows_do_not_conflict(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // The first borrow dies at its last use; the second starts after it.
    var ch = "public func main() : int {\n    var x = 1\n    var a = &mut x\n    *a = 2\n    var b = &mut x\n    *b = 3\n    return x\n}\n"
    expect_compile_success(env, "borrow_sequential_ok", ch)
}

@test
func shared_borrows_do_not_conflict(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 1\n    var a = &x\n    var b = &x\n    return *a + *b\n}\n"
    expect_compile_success(env, "borrow_shared_ok", ch)
}

@test
func neg_move_while_borrowed_warns(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    var s = S { x : 1 }\n    var p = &s\n    var t = s\n    return p.x\n}\n"
    expect_compile_warning_id(env, "borrow_move_while_live", ch, "SAFE-BORROW-002")
}

@test
func disjoint_branch_borrows_do_not_conflict(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // CFG liveness: the two loans are in different arms and die before the
    // merge, so they are never live together.
    var ch = "public func main() : int {\n    var x = 1\n    var c = true\n    if(c) { var a = &mut x\n *a = 2 } else { var b = &mut x\n *b = 3 }\n    return x\n}\n"
    expect_compile_output_not_contains(env, "borrow_branch_disjoint", ch, "SAFE-BORROW-001", NEG_MOD)
}

@test
func neg_reference_returning_call_extends_loan(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // The call result aliases the argument, so the loan stays live until the
    // result's last use and conflicts with a later mutable borrow.
    var ch = "func id(p : &int) : &int {\n    return p\n}\npublic func main() : int {\n    var x = 1\n    var a = id(&x)\n    var b = &mut x\n    *b = 2\n    return *a\n}\n"
    expect_compile_warning_id(env, "borrow_ref_return_call", ch, "SAFE-BORROW-001")
}

@test
func value_returning_call_ends_loan(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A callee that returns a value cannot retain the reference; the loan
    // ends at the call, so the later mutable borrow is fine.
    var ch = "func read_int(p : &int) : int {\n    return *p\n}\npublic func main() : int {\n    var x = 1\n    var a = read_int(&x)\n    var b = &mut x\n    *b = 2\n    return a\n}\n"
    expect_compile_output_not_contains(env, "borrow_value_return_call", ch, "SAFE-BORROW-001", NEG_MOD)
}

@test
func neg_return_local_reference_warns(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func bad() : &int {\n    var x = 1\n    return &x\n}\npublic func main() : int {\n    var r = bad()\n    return *r\n}\n"
    expect_compile_warning_id(env, "borrow_return_local_ref", ch, "SAFE-LIFE-001")
}

@test
func returned_parameter_reference_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func id(p : &int) : &int {\n    return p\n}\npublic func main() : int {\n    var x = 5\n    var r = id(&x)\n    return *r\n}\n"
    expect_compile_output_not_contains(env, "borrow_return_param_ref", ch, "SAFE-LIFE-001", NEG_MOD)
}
