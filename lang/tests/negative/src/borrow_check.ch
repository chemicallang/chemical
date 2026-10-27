// Borrow-checker (S4.4-S4.6) negative/positive tests. SAFE-BORROW-* and
// SAFE-LIFE-* are enforced (compilation fails); positive cases assert success.

@test
func neg_borrow_mutable_alias_warns(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var a = &mut x\n    var b = &mut x\n    *a = 2\n    *b = 3\n    return x\n}\n"
    expect_compile_error(env, "borrow_mutable_alias", ch, "SAFE-BORROW-001")
}

@test
func neg_borrow_shared_mutable_alias_warns(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var a = &x\n    var b = &mut x\n    *b = 2\n    return *a\n}\n"
    expect_compile_error(env, "borrow_shared_mutable_alias", ch, "SAFE-BORROW-001")
}

@test
func sequential_mutable_borrows_do_not_conflict(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The first borrow dies at its last use; the second starts after it.
    var ch = "public func main() : int {\n    var x = 1\n    var a = &mut x\n    *a = 2\n    var b = &mut x\n    *b = 3\n    return x\n}\n"
    expect_compile_success(env, "borrow_sequential_ok", ch)
}

@test
func shared_borrows_do_not_conflict(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func main() : int {\n    var x = 1\n    var a = &x\n    var b = &x\n    return *a + *b\n}\n"
    expect_compile_success(env, "borrow_shared_ok", ch)
}

@test
func neg_move_while_borrowed_warns(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct S {\n    var x : int\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    var s = S { x : 1 }\n    var p = &s\n    var t = s\n    return p.x\n}\n"
    expect_compile_error(env, "borrow_move_while_live", ch, "SAFE-BORROW-002")
}

@test
func disjoint_branch_borrows_do_not_conflict(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // CFG liveness: the two loans are in different arms and die before the
    // merge, so they are never live together.
    var ch = "public func main() : int {\n    var x = 1\n    var c = true\n    if(c) { var a = &mut x\n *a = 2 } else { var b = &mut x\n *b = 3 }\n    return x\n}\n"
    expect_compile_output_not_contains(env, "borrow_branch_disjoint", ch, "SAFE-BORROW-001", NEG_MOD)
}

@test
func neg_reference_returning_call_extends_loan(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The call result aliases the argument, so the loan stays live until the
    // result's last use and conflicts with a later mutable borrow.
    var ch = "func id(p : &int) : &int {\n    return p\n}\npublic func main() : int {\n    var x = 1\n    var a = id(&x)\n    var b = &mut x\n    *b = 2\n    return *a\n}\n"
    expect_compile_error(env, "borrow_ref_return_call", ch, "SAFE-BORROW-001")
}

@test
func value_returning_call_ends_loan(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A callee that returns a value cannot retain the reference; the loan
    // ends at the call, so the later mutable borrow is fine.
    var ch = "func read_int(p : &int) : int {\n    return *p\n}\npublic func main() : int {\n    var x = 1\n    var a = read_int(&x)\n    var b = &mut x\n    *b = 2\n    return a\n}\n"
    expect_compile_output_not_contains(env, "borrow_value_return_call", ch, "SAFE-BORROW-001", NEG_MOD)
}

@test
func neg_return_local_reference_errors(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // SAFE-LIFE-001 is enforced (not report-only).
    var ch = "func bad() : &int {\n    var x = 1\n    return &x\n}\npublic func main() : int {\n    var r = bad()\n    return *r\n}\n"
    expect_compile_error(env, "borrow_return_local_ref", ch, "SAFE-LIFE-001")
}

@test
func returned_parameter_reference_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func id(p : &int) : &int {\n    return p\n}\npublic func main() : int {\n    var x = 5\n    var r = id(&x)\n    return *r\n}\n"
    expect_compile_output_not_contains(env, "borrow_return_param_ref", ch, "SAFE-LIFE-001", NEG_MOD)
}

@test
func neg_reference_in_field_outlives_local(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The struct's reference field outlives the local it points at.
    // SAFE-LIFE-002 is enforced (not report-only).
    var ch = "struct H {\n    var r : &int\n}\npublic func main() : int {\n    var h : H\n    {\n        var x = 1\n        h = H { r : &x }\n    }\n    return *h.r\n}\n"
    expect_compile_error(env, "borrow_field_outlives_local", ch, "SAFE-LIFE-002")
}

@test
func same_scope_field_reference_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct H {\n    var r : &int\n}\npublic func main() : int {\n    var x = 1\n    var h = H { r : &x }\n    return *h.r\n}\n"
    expect_compile_output_not_contains(env, "borrow_field_same_scope", ch, "SAFE-LIFE-002", NEG_MOD)
}

@test
func neg_return_region_does_not_outlive(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The returned reference comes from 'a but the signature declares 'b.
    var ch = "func <'a, 'b> bad(x : &'a int) : &'b int {\n    return x\n}\npublic func main() : int {\n    var a = 1\n    var r = bad(&a)\n    return *r\n}\n"
    expect_compile_error(env, "borrow_return_region", ch, "SAFE-LIFE-003")
}

@test
func return_region_with_outlives_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a, 'b> good(x : &'a int) : &'b int where 'a : 'b {\n    return x\n}\npublic func main() : int {\n    var a = 1\n    var r = good(&a)\n    return *r\n}\n"
    expect_compile_success(env, "borrow_return_region_ok", ch)
}

@test
func neg_aggregate_return_region_does_not_outlive(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The reference is carried in a returned struct field, not returned
    // directly; SAFE-LIFE-003 still applies.
    var ch = "struct Holder<'a> {\n    var r : &'a int\n}\nfunc <'a, 'b> bad(x : &'a int) : Holder<'b> {\n    return Holder<'b> { r : x }\n}\npublic func main() : int {\n    var a = 1\n    var h = bad(&a)\n    return *h.r\n}\n"
    expect_compile_error(env, "borrow_aggregate_return_region", ch, "SAFE-LIFE-003")
}

@test
func aggregate_return_region_with_outlives_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Holder<'a> {\n    var r : &'a int\n}\nfunc <'a, 'b> good(x : &'a int) : Holder<'b> where 'a : 'b {\n    return Holder<'b> { r : x }\n}\npublic func main() : int {\n    var a = 1\n    var h = good(&a)\n    return *h.r\n}\n"
    expect_compile_success(env, "borrow_aggregate_return_region_ok", ch)
}

@test
func neg_aggregate_return_local_field(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A struct field referencing a local escapes with the returned struct.
    var ch = "struct Holder<'a> {\n    var r : &'a int\n}\nfunc bad() : Holder<'static> {\n    var a = 1\n    var h = Holder<'static> { r : &a }\n    return h\n}\npublic func main() : int {\n    var h = bad()\n    return *h.r\n}\n"
    expect_compile_error(env, "borrow_aggregate_return_local", ch, "SAFE-LIFE-001")
}

@test
func neg_direct_reference_field_return_region(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Returning the reference field itself goes through field_params.
    var ch = "struct Holder<'a> {\n    var r : &'a int\n}\nfunc <'a, 'b> bad(x : &'a int) : &'b int {\n    var h = Holder<'a> { r : x }\n    return h.r\n}\npublic func main() : int {\n    var a = 1\n    var r = bad(&a)\n    return *r\n}\n"
    expect_compile_error(env, "borrow_direct_field_return_region", ch, "SAFE-LIFE-003")
}

@test
func neg_multi_reference_call_maps_return_loan(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `pick` returns its first reference; the second argument must not hide
    // the result loan behind the conservative multi-reference escape.
    var ch = "func <'a, 'b> pick(x : &'a mut int, y : &'b mut int) : &'a mut int {\n    return x\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    var r = pick(&mut a, &mut b)\n    var q = &mut a\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "borrow_multi_ref_call_map", ch, "SAFE-BORROW-001")
}

@test
func multi_reference_call_unrelated_argument_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The result aliases `a`, so a subsequent mutable borrow of `b` is fine.
    var ch = "func <'a, 'b> pick(x : &'a mut int, y : &'b mut int) : &'a mut int {\n    return x\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    var r = pick(&mut a, &mut b)\n    var q = &mut b\n    *q = 9\n    return *r\n}\n"
    expect_compile_output_not_contains(env, "borrow_multi_ref_call_other", ch, "SAFE-BORROW-001", NEG_MOD)
}

@test
func neg_ref_guard_outlives_cell(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A RefCell guard keeps its borrow until it is dropped. Storing the guard
    // outside the cell's block makes it outlive the cell (dropck via LIFE-002).
    var ch = "using std::Option;\npublic func main() : int {\n    var guard : Option<std::Ref<int>>\n    {\n        var cell = std::ref_cell(1)\n        guard = cell.try_borrow()\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "borrow_ref_guard_outlives", ch, "SAFE-LIFE-002", NEG_MOD_STD_CORE)
}

@test
func ref_guard_same_scope_drops_first_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using std::Option;\npublic func main() : int {\n    var cell = std::ref_cell(1)\n    var guard = cell.try_borrow()\n    return 0\n}\n"
    expect_compile_success_with_mod(env, "borrow_ref_guard_same_scope", ch, NEG_MOD_STD_CORE)
}

@test
func neg_ambiguous_reference_result_aliases_all_candidates(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Both arguments have the return region, so the result may alias either.
    var ch = "func <'a> pick(x : &'a mut int, y : &'a mut int) : &'a mut int {\n    return x\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    var r = pick(&mut a, &mut b)\n    var q = &mut b\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "borrow_ambiguous_result_alias", ch, "SAFE-BORROW-001")
}

@test
func ambiguous_reference_result_unrelated_place_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a> pick(x : &'a mut int, y : &'a mut int) : &'a mut int {\n    return x\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    var c = 3\n    var r = pick(&mut a, &mut b)\n    var q = &mut c\n    *q = 9\n    return *r\n}\n"
    expect_compile_success(env, "borrow_ambiguous_result_other", ch)
}
