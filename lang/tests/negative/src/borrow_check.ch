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
    var ch = "struct Holder<'a> {\n    var r : &'a int\n}\nfunc <'a> bad(x : &'a int) : Holder<'a> {\n    var a = 1\n    var h = Holder<'a> { r : &a }\n    return h\n}\npublic func main() : int {\n    var a = 1\n    var h = bad(&a)\n    return *h.r\n}\n"
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

@test
func aggregate_parameter_field_uses_declared_lifetime(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S4.2b: the aggregate's second lifetime argument ('b) is the region of
    // `y`, so returning it as 'b is accepted.
    var ch = "struct Holder<'a> {\n    var r : &'a int\n}\nfunc <'a> get_ref(h : Holder<'a>) : &'a int {\n    return h.r\n}\nfunc <'a> check(x : &'a int) : int {\n    var h = Holder<'a> { r : x }\n    return *get_ref(h)\n}\npublic func main() : int {\n    var a = 1\n    return check(&a)\n}\n"
    expect_compile_success(env, "borrow_aggregate_param_field_region", ch)
}

@test
func neg_aggregate_parameter_field_wrong_lifetime(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S4.2b: `x` is tied to 'a, not the declared return region 'b.
    var ch = "struct Holder<'a> {\n    var r : &'a int\n}\nfunc <'a, 'b> bad(h : Holder<'a>) : &'b int {\n    return h.r\n}\nfunc <'a, 'b> check(x : &'a int, y : &'b int) : int {\n    var h = Holder<'a> { r : x }\n    return *bad(h)\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    return check(&a, &b)\n}\n"
    expect_compile_error(env, "borrow_aggregate_param_field_wrong", ch, "SAFE-LIFE-003")
}

@test
func aggregate_parameter_field_with_outlives_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S4.2b: a `where 'a: 'b` edge makes the 'a field a valid 'b result.
    var ch = "struct Holder<'a> {\n    var r : &'a int\n}\nfunc <'a, 'b> good(h : Holder<'a>) : &'b int where 'a : 'b {\n    return h.r\n}\nfunc <'a, 'b> check(x : &'a int, y : &'b int) : int where 'a : 'b {\n    var h = Holder<'a> { r : x }\n    return *good(h)\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    return check(&a, &b)\n}\n"
    expect_compile_success(env, "borrow_aggregate_param_field_outlives", ch)
}

@test
func disjoint_constant_array_indices_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S4.5: constant indices into the same array are disjoint even when the
    // element type is Copy (no move-path tree).
    var ch = "public func main() : int {\n    var a : [3]int = [1, 2, 3]\n    var x = &mut a[0]\n    var y = &mut a[1]\n    *x = 10\n    *y = 20\n    return a[0] + a[1]\n}\n"
    expect_compile_success(env, "borrow_disjoint_const_indices", ch)
}

@test
func neg_dynamic_array_indices_overlap(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S4.5: dynamic indices are not proven distinct, so the borrows overlap.
    var ch = "public func main() : int {\n    var a : [3]int = [1, 2, 3]\n    var i = 0\n    var j = 1\n    var x = &mut a[i]\n    var y = &mut a[j]\n    *x = 10\n    *y = 20\n    return a[0] + a[1]\n}\n"
    expect_compile_error(env, "borrow_dynamic_indices_overlap", ch, "SAFE-BORROW-001")
}

@test
func mutable_reborrow_through_reference_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S4.5: `&mut *r` is a reborrow of `r`'s loan, not a second alias.
    var ch = "public func main() : int {\n    var x = 1\n    var r = &mut x\n    var q = &mut *r\n    *q = 5\n    return x\n}\n"
    expect_compile_success(env, "borrow_mutable_reborrow", ch)
}

@test
func method_reference_field_matches_return_lifetime(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S4.2b: `&self` is a reference to an aggregate, so the container's
    // lifetime arguments still select the field's region.
    var ch = "struct Holder<'a> {\n    var r : &'a int\n    func get(&self) : &'a int {\n        return self.r\n    }\n}\nfunc <'a> check(x : &'a int) : int {\n    var h = Holder<'a> { r : x }\n    return *h.get()\n}\npublic func main() : int {\n    var a = 1\n    return check(&a)\n}\n"
    expect_compile_success(env, "borrow_method_field_region", ch)
}

@test
func neg_method_reference_field_wrong_lifetime(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // S4.2b: `self.x` is tied to 'a, not the declared return region 'b.
    var ch = "struct Holder<'a> {\n    var r : &'a int\n    func get(&self) : &'static int {\n        return self.r\n    }\n}\nfunc <'a> check(x : &'a int) : int {\n    var h = Holder<'a> { r : x }\n    return *h.get()\n}\npublic func main() : int {\n    var a = 1\n    return check(&a)\n}\n"
    expect_compile_error(env, "borrow_method_field_wrong_region", ch, "SAFE-LIFE-004")
}

@test
func neg_string_view_outlives_local(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A view is a raw pointer + length, so the ordinary reference-field check
    // cannot see it. The S4.2b view tracking attaches the source loan.
    var ch = "public func bad() : std::string_view {\n    var s = std::string(\"hello\")\n    return s.to_view()\n}\npublic func main() : int {\n    var v = bad()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "view_outlives_local", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
}

@test
func neg_span_outlives_local(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public func bad() : std::span<int> {\n    var a : [3]int = [1, 2, 3]\n    return unsafe(std::span<int>(&raw mut a[0], 3))\n}\npublic func main() : int {\n    var v = bad()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "span_outlives_local", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
}

@test
func view_from_parameter_or_same_scope_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func from_param(s : &std::string) : std::string_view {\n    return s.to_view()\n}\nfunc same_scope() : size_t {\n    var s = std::string(\"hello\")\n    var v = s.to_view()\n    return v.size()\n}\npublic func main() : int {\n    var s = std::string(\"x\")\n    var v = from_param(&s)\n    return (v.size() + same_scope()) as int\n}\n"
    expect_compile_success_with_mod(env, "view_ok", ch, NEG_MOD_STD_CORE)
}

@test
func neg_future_handle_cannot_be_forged(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The frame/vtable pair is the async runtime's unchecked contract; safe
    // code must go through spawn/spawn_blocking.
    var ch = "public func main() : int {\n    var x : int = 7\n    var h = core::async::FutureHandle<int> { frame : &raw mut x as *mut void, vtbl : null }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "future_handle_forged", ch, "SAFE-VIS-001", NEG_MOD_CORE)
}

@test
func neg_async_move_while_borrowed(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A mutable borrow crosses `.await`; moving the referent in the resume
    // function while the frame reference is still used is a use-after-move.
    var ch = "async func bad() : std::string {\n    var x = std::string(\"hello\")\n    var p = &mut x\n    var _ = await async::yield_now()\n    var y = x\n    p.append('!')\n    return y\n}\npublic func main() : int {\n    var r = async::block_on<std::string>(bad())\n    return r.size() as int\n}\n"
    expect_compile_error_with_mod(env, "async_move_while_borrowed", ch, "SAFE-BORROW-002", NEG_MOD_ASYNC_STD)
}

@test
func async_borrow_across_await_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The borrow ends before the local is returned, so the move is safe.
    var ch = "async func ok() : std::string {\n    var x = std::string(\"hello\")\n    var p = &mut x\n    var _ = await async::yield_now()\n    p.append('!')\n    return x\n}\npublic func main() : int {\n    var r = async::block_on<std::string>(ok())\n    return r.size() as int\n}\n"
    expect_compile_success_with_mod(env, "async_borrow_across_await", ch, NEG_MOD_ASYNC_STD)
}

@test
func neg_view_from_data_pointer_escapes(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A view built from `s.data()` must inherit the receiver's loan even though
    // the pointer is produced by an intermediate call.
    var ch = "func bad() : std::string_view {\n    var s = std::string(\"hello\")\n    return unsafe(std::string_view(s.data(), s.size()))\n}\npublic func main() : int {\n    var v = bad()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "view_from_data_escapes", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
}

@test
func neg_temporary_not_extended_through_call_argument(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Temporary lifetime extension applies to a borrow in the let/var
    // initializer, not to a temporary passed as a call argument. The callee
    // returns the reference, so it is still live when the argument temporary
    // dies at the end of the statement.
    var ch = "struct D {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc make() : D { return D { x : 7 } }\nfunc id(p : &D) : &D { return p }\npublic func main() : int {\n    var r = id(&make())\n    return r.x\n}\n"
    expect_compile_error(env, "temporary_call_arg_not_extended", ch, "SAFE-LIFE-002")
}

@test
func neg_temporary_not_extended_through_assignment(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Only a variable's initializer extends a borrowed temporary. Assigning an
    // aggregate literal later is still a statement-scoped temporary.
    var ch = "struct D {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nstruct H {\n    var r : &D\n}\nfunc make() : D { return D { x : 7 } }\npublic func main() : int {\n    var d = make()\n    var h = H { r : &d }\n    h = H { r : &make() }\n    return h.r.x\n}\n"
    expect_compile_error(env, "temporary_assignment_not_extended", ch, "SAFE-LIFE-002")
}

@test
func hrtb_elided_source_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A function with an elided reference lifetime is generic over that
    // lifetime and satisfies `for<'a>`.
    var ch = "func read_int(x : &int) : int { return *x }\nfunc apply(f : for<'a> (v : &'a int) => int, x : &int) : int {\n    return f(x)\n}\npublic func main() : int {\n    var x = 41\n    if(apply(read_int, &x) != 41) { return 1 }\n    return 0\n}\n"
    expect_compile_success(env, "hrtb_elided_source", ch)
}

@test
func neg_hrtb_static_source_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `&'static int` is more restrictive than `for<'a> &'a int`; accepting it
    // would let the callee demand static storage from a short-lived argument.
    var ch = "func read_static(x : &'static int) : int { return *x }\nfunc apply(f : for<'a> (v : &'a int) => int, x : &int) : int {\n    return f(x)\n}\npublic func main() : int {\n    var x = 41\n    return apply(read_static, &x)\n}\n"
    expect_compile_error(env, "hrtb_static_source", ch, "SAFE-BORROW-099")
}

@test
func hrtb_mutable_parameter_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A single mutable borrow has no second caller-owned place to store it
    // into, so the call contract can bind its loan to the call.
    var ch = "func touch(v : &mut int) { *v = *v + 1 }\nfunc apply(f : for<'a> (v : &'a mut int) => void, x : &mut int) {\n    f(x)\n}\npublic func main() : int {\n    var x = 1\n    apply(touch, &mut x)\n    return x\n}\n"
    expect_compile_success(env, "hrtb_mutable_param", ch)
}

@test
func hrtb_borrowed_return_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The call-site contract maps the result to the one borrowed argument.
    var ch = "func first_int(x : &int) : &int { return x }\nfunc apply(f : for<'a> (v : &'a int) => &'a int, x : &int) : &int {\n    return f(x)\n}\npublic func main() : int {\n    var x = 41\n    return *apply(first_int, &x)\n}\n"
    expect_compile_success(env, "hrtb_borrowed_return", ch)
}

@test
func neg_hrtb_borrowed_return_keeps_loan(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The result is borrowed from x; taking a mutable borrow of x while the
    // result is live must conflict.
    var ch = "func first_int(x : &int) : &int { return x }\nfunc apply(f : for<'a> (v : &'a int) => &'a int, x : &int) : &int {\n    return f(x)\n}\npublic func main() : int {\n    var x = 41\n    var r = apply(first_int, &x)\n    var q = &mut x\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "hrtb_borrowed_return_alias", ch, "SAFE-BORROW-001")
}

@test
func neg_hrtb_nested_borrowed_return_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // An aggregate carrying the binder would need field-wise region mapping.
    var ch = "using std::Option;\nfunc apply(f : for<'a> (v : &'a int) => Option<&'a int>) : int { return 0 }\npublic func main() : int { return apply(null) }\n"
    expect_compile_error_with_mod(env, "hrtb_nested_return", ch, "SAFE-BORROW-099", NEG_MOD_STD_CORE)
}

@test
func neg_hrtb_mismatched_return_binder_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `'b` is not tied to the `'a` input, so the result has no caller region.
    var ch = "func apply(f : for<'a, 'b> (v : &'a int) => &'b int) : int { return 0 }\npublic func main() : int { return apply(null) }\n"
    expect_compile_error(env, "hrtb_mismatched_binder", ch, "SAFE-BORROW-099")
}

@test
func neg_hrtb_multiple_references_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Two higher-ranked borrows could be stored into one another; full region
    // constraints are required before accepting them.
    var ch = "func sum2(a : &int, b : &int) : int { return *a + *b }\nfunc apply(f : for<'a> (x : &'a int, y : &'a int) => int, a : &int, b : &int) : int {\n    return f(a, b)\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    return apply(sum2, &a, &b)\n}\n"
    expect_compile_error(env, "hrtb_multiple_refs", ch, "SAFE-BORROW-099")
}

@test
func neg_hrtb_duplicate_binder_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func main() : int {\n    var f : for<'a, 'a> (v : &'a int) => int = null\n    return 0\n}\n"
    expect_compile_error(env, "hrtb_duplicate_binder", ch, "duplicate higher-ranked lifetime parameter")
}

@test
func neg_static_reference_assignment_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A stack reference cannot be stored in a `'static` binding.
    var ch = "public func main() : int {\n    var x = 1\n    var r : &'static int = &x\n    return *r\n}\n"
    expect_compile_error(env, "static_reference_assignment", ch, "SAFE-LIFE-004")
}

@test
func neg_static_struct_literal_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A struct argument `'static` determines the field reference lifetime too;
    // the literal cannot borrow a stack local.
    var ch = "struct P<'a> {\n    var r : &'a int\n}\npublic func main() : int {\n    var x = 1\n    var p = P<'static> { r : &x }\n    return *p.r\n}\n"
    expect_compile_error(env, "static_struct_literal", ch, "SAFE-LIFE-004")
}

@test
func neg_static_lifetime_argument_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A container typed with `G<'static>` cannot accept a `G<'a>` built from a
    // stack borrow. The guard would otherwise be dropped after its referent.
    var ch = "using std::vector;\nstruct G<'a> {\n    var r : &'a int\n    @delete\n    func delete(&mut self) { unsafe { printf(\"%d\\n\", *self.r) } }\n}\nfunc <'a> mk(x : &'a int) : G<'a> { return G<'a> { r : x } }\npublic func main() : int {\n    var v = vector<G<'static>>()\n    {\n        var x = 41\n        v.push(mk(&x))\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "static_lifetime_argument", ch, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
}

@test
func neg_hrtb_binder_does_not_leak(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `'a` is bound only by the function type; it must not be usable in the
    // enclosing function body.
    var ch = "func apply(f : for<'a> (v : &'a int) => int) : int {\n    var r : &'a int = null\n    return 0\n}\npublic func main() : int {\n    return apply(null)\n}\n"
    expect_compile_error(env, "hrtb_binder_leak", ch, "unknown lifetime parameter 'a'")
}
