// Borrow-checker (S4.4-S4.6) negative/positive tests. SAFE-BORROW-* and
// SAFE-LIFE-* are enforced (compilation fails); positive cases assert success.

@test
func neg_explicit_slice_lifetimes(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var wrong_region = "func <'a, 'b> bad(values : &'a [int]) : &'b [int] {\n    return values\n}\npublic func main() : int {\n    var arr : [2]int = [1, 2]\n    var out = bad(arr)\n    return out[0]\n}\n"
    expect_compile_error_with_mod(env, "slice_wrong_return_lifetime", wrong_region, "SAFE-LIFE-003", NEG_MOD_STD_CORE)
    var static_slice = "func <'a> bad(values : &'a [int]) : &'static [int] {\n    return values\n}\npublic func main() : int {\n    var arr : [2]int = [1, 2]\n    var out = bad(arr)\n    return out[0]\n}\n"
    expect_compile_error_with_mod(env, "slice_static_lifetime", static_slice, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
    var static_str = "func <'a> bad(value : &'a str) : &'static str {\n    return value\n}\npublic func main() : int {\n    var value : &str = \"hi\"\n    var out = bad(value)\n    return out.size() as int\n}\n"
    expect_compile_error_with_mod(env, "str_static_lifetime", static_str, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
    var elided_slice = "func bad(values : &[int]) : &'static [int] {\n    return values\n}\npublic func main() : int {\n    var arr : [2]int = [1, 2]\n    var out = bad(arr)\n    return out[0]\n}\n"
    expect_compile_error_with_mod(env, "elided_slice_static_lifetime", elided_slice, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
    var elided_str = "func bad(value : &str) : &'static str {\n    return value\n}\npublic func main() : int {\n    var value : &str = \"hi\"\n    var out = bad(value)\n    return out.size() as int\n}\n"
    expect_compile_error_with_mod(env, "elided_str_static_lifetime", elided_str, "str<static>", NEG_MOD_STD_CORE)
}

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
func neg_vector_span_escape(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A vector-derived span is a borrow view. Returning it from a function
    // whose owner is local must be rejected, even though the span stores the
    // pointer/length pair rather than a reference field.
    var ch = "func bad() : [int] {\n    var v = std::vector<int>()\n    v.push(1)\n    return std::span<int>(&v)\n}\npublic func main() : int {\n    var s = bad()\n    return unsafe(*s.get(0)) - 1\n}\n"
    expect_compile_error_with_mod(env, "borrow_vector_span_escape", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
}

@test
func neg_static_span_lifetime(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func bad() : std::span<'static, int> {\n    var v = std::vector<int>()\n    v.push(1)\n    return std::span<int>(&v)\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "borrow_static_span", ch, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
}

@test
func neg_vector_as_mut_span_static_escape(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func bad() : std::mut_span<'static, int> {\n    var v = std::vector<int>()\n    v.push(1)\n    return v.as_mut_span()\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error_with_mod(env, "borrow_vector_as_mut_span_static", ch, "SAFE-LIFE-004", NEG_MOD_STD_CORE)
}

@test
func neg_vector_as_span_escape(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func bad() : [int] {\n    var v = std::vector<int>()\n    v.push(1)\n    return v.as_span()\n}\npublic func main() : int {\n    var s = bad()\n    return unsafe(*s.get(0)) - 1\n}\n"
    expect_compile_error_with_mod(env, "borrow_vector_as_span_escape", ch, "SAFE-LIFE-001", NEG_MOD_STD_CORE)
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
func neg_hrtb_aggregate_alias_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The returned field borrow must remain live after the call.
    var ch = "struct View<'a> {\n    var p : &'a int\n}\nfunc <'a> get_view(v : View<'a>) : &'a int {\n    return v.p\n}\nfunc <'a> apply_view(f : for<'b> (v : View<'b>) => &'b int, x : &'a int) : &'a int {\n    return f(View<'a> { p : x })\n}\npublic func main() : int {\n    var x = 41\n    var r = apply_view(get_view, &x)\n    var q = &mut x\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "hrtb_aggregate_alias", ch, "SAFE-BORROW-001")
}

@test
func hrtb_deep_aggregate_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A nested aggregate of direct reference fields is a valid higher-ranked
    // parameter when the callback only reads through it.
    var ch = "struct View<'a> {\n    var p : &'a int\n}\nstruct Outer<'a> {\n    var view : View<'a>\n}\nfunc <'a> read_outer(v : Outer<'a>) : int {\n    return *v.view.p\n}\nfunc <'a> apply(f : for<'b> (v : Outer<'b>) => int, x : &'a int) : int {\n    return f(Outer<'a> { view : View<'a> { p : x } })\n}\npublic func main() : int {\n    var x = 41\n    return apply(read_outer, &x) - 41\n}\n"
    expect_compile_success(env, "hrtb_deep_aggregate", ch)
}

@test
func hrtb_deep_aggregate_borrowed_result_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The nested field loan is followed through the aggregate source chain and
    // remains live while the returned reference is used.
    var ch = "struct View<'a> {\n    var p : &'a int\n}\nstruct Outer<'a> {\n    var view : View<'a>\n}\nfunc <'a> read_outer_ref(v : Outer<'a>) : &'a int {\n    return v.view.p\n}\nfunc <'a> apply(f : for<'b> (v : Outer<'b>) => &'b int, x : &'a int) : &'a int {\n    return f(Outer<'a> { view : View<'a> { p : x } })\n}\npublic func main() : int {\n    var x = 41\n    return *apply(read_outer_ref, &x) - 41\n}\n"
    expect_compile_success(env, "hrtb_deep_aggregate_borrowed_result", ch)
}

@test
func neg_hrtb_deep_aggregate_borrowed_result_alias(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The returned nested-field reference must keep the source loan alive.
    var ch = "struct View<'a> {\n    var p : &'a int\n}\nstruct Outer<'a> {\n    var view : View<'a>\n}\nfunc <'a> read_outer_ref(v : Outer<'a>) : &'a int {\n    return v.view.p\n}\nfunc <'a> apply(f : for<'b> (v : Outer<'b>) => &'b int, x : &'a int) : &'a int {\n    return f(Outer<'a> { view : View<'a> { p : x } })\n}\npublic func main() : int {\n    var x = 41\n    var r = apply(read_outer_ref, &x)\n    var q = &mut x\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "hrtb_deep_aggregate_borrowed_result_alias", ch, "SAFE-BORROW-001")
}

@test
func hrtb_pair_lifetime_precision_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The result borrows left only; right remains independently mutable.
    var ch = "struct Pair<'a, 'b> {\n    var left : &'a int\n    var right : &'b int\n}\nfunc <'a, 'b> read_left(p : Pair<'a, 'b>) : &'a int {\n    return p.left\n}\nfunc <'a, 'b> apply(f : for<'x, 'y> (p : Pair<'x, 'y>) => &'x int, x : &'a int, y : &'b int) : &'a int {\n    return f(Pair<'a, 'b> { left : x, right : y })\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_left, &x, &y)\n    var q = &mut y\n    *q = 9\n    return *r - 41\n}\n"
    expect_compile_success(env, "hrtb_pair_lifetime_precision", ch)
}

@test
func hrtb_nested_variant_array_params_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Variant payloads and array elements of a nested aggregate are valid
    // higher-ranked inputs, including borrowed results. The generic variant
    // payload is substituted from the concrete type argument.
    var ch = "struct View<'a> {\n    var p : &'a int\n}\nfunc apply_opt(f : for<'b> (m : std::Option<View<'b>>) => int) : int { return 0 }\nfunc apply_opt_ref(f : for<'b> (m : std::Option<View<'b>>) => &'b int) : int { return 0 }\nfunc apply_arr(f : for<'b> (m : [2]View<'b>) => int) : int { return 0 }\npublic func main() : int { return 0 }\n"
    expect_compile_success_with_mod(env, "hrtb_nested_variant_array_params", ch, NEG_MOD_STD_CORE)
}

@test
func hrtb_variant_payload_borrowed_result_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A lifetime-parameterised variant payload participates in the same
    // field-source chain as a struct field.
    var ch = "struct View<'a> {\n    var p : &'a int\n}\nvariant Maybe<'a> {\n    Some(value : View<'a>)\n    None()\n}\nfunc <'a> read_maybe(m : Maybe<'a>) : &'a int {\n    var Some(value) = m else unsafe { unreachable }\n    return value.p\n}\nfunc <'a> apply(f : for<'b> (m : Maybe<'b>) => &'b int, x : &'a int) : &'a int {\n    return f(Maybe.Some(View<'a> { p : x }))\n}\npublic func main() : int {\n    var x = 41\n    var r = apply(read_maybe, &x)\n    return *r - 41\n}\n"
    expect_compile_success(env, "hrtb_variant_payload_borrowed_result", ch)
}

@test
func hrtb_array_element_borrowed_result_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A constant array-element projection is followed to its reference loan.
    var ch = "struct Refs<'a> {\n    var items : [2]&'a int\n}\nfunc <'a> read_refs(r : Refs<'a>) : &'a int {\n    var c = r\n    return c.items[0]\n}\nfunc <'a> apply(f : for<'b> (r : Refs<'b>) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    return f(Refs<'a> { items : [x, y] })\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_refs, &x, &y)\n    return *r - 41\n}\n"
    expect_compile_success(env, "hrtb_array_element_borrowed_result", ch)
}

@test
func hrtb_large_array_element_borrowed_result_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A top-level large array of direct references uses one wildcard element
    // loan, so the returned element keeps the caller's source live.
    var ch = "func <'a> read_refs(items : [65]&'a int) : &'a int {\n    return items[0]\n}\nfunc <'a> apply(f : for<'b> (items : [65]&'b int) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    var items : [65]&'a int = [x, y, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x]\n    return f(items)\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_refs, &x, &y)\n    return *r - 41\n}\n"
    expect_compile_success(env, "hrtb_large_array_element_borrowed_result", ch)
}

@test
func neg_hrtb_large_array_element_result_aliases(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Elements of the same large array share one region, so a later mutable
    // borrow of any source element must still conflict with the result.
    var ch = "func <'a> read_refs(items : [65]&'a int) : &'a int {\n    return items[0]\n}\nfunc <'a> apply(f : for<'b> (items : [65]&'b int) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    var items : [65]&'a int = [x, y, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x]\n    return f(items)\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_refs, &x, &y)\n    var q = &mut y\n    *q = 9\n    return *r - 41\n}\n"
    expect_compile_error(env, "hrtb_large_array_result_alias", ch, "SAFE-BORROW-001")
}

@test
func hrtb_struct_large_array_element_result_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A large direct-reference array in a struct field uses one wildcard
    // element loan keyed by (root, field), so the borrowed result keeps the
    // caller source live.
    var ch = "struct Refs<'a> {\n    var items : [65]&'a int\n}\nfunc <'a> read_refs(r : Refs<'a>) : &'a int {\n    return r.items[0]\n}\nfunc <'a> apply(f : for<'b> (r : Refs<'b>) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    var items : [65]&'a int = [x, y, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x]\n    return f(Refs<'a> { items : items })\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_refs, &x, &y)\n    return *r - 41\n}\n"
    expect_compile_success(env, "hrtb_struct_large_array_element_result", ch)
}

@test
func neg_hrtb_struct_large_array_element_result_aliases(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Refs<'a> {\n    var items : [65]&'a int\n}\nfunc <'a> read_refs(r : Refs<'a>) : &'a int {\n    return r.items[0]\n}\nfunc <'a> apply(f : for<'b> (r : Refs<'b>) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    var items : [65]&'a int = [x, y, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x]\n    return f(Refs<'a> { items : items })\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_refs, &x, &y)\n    var q = &mut y\n    *q = 9\n    return *r - 41\n}\n"
    expect_compile_error(env, "hrtb_struct_large_array_result_alias", ch, "SAFE-BORROW-001")
}

@test
func hrtb_large_array_nested_struct_compiles(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A large reference array nested through a struct field and through an
    // array-of-structs element keeps the same wildcard leaf contract.
    var ch = "struct Inner<'a> {\n    var items : [65]&'a int\n}\nstruct Outer<'a> {\n    var inner : Inner<'a>\n}\nfunc apply_nested(f : for<'b> (o : Outer<'b>) => int) : int { return 0 }\nfunc apply_array(f : for<'b> (o : [2]Inner<'b>) => int) : int { return 0 }\npublic func main() : int { return 0 }\n"
    expect_compile_success(env, "hrtb_large_array_nested_struct", ch)
}

@test
func hrtb_large_array_nested_borrowed_result_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The wildcard leaf keeps both the nested field path and the caller's
    // source loan, so a borrowed element result is path-mapped.
    var ch = "struct Inner<'a> {\n    var items : [65]&'a int\n}\nstruct Outer<'a> {\n    var inner : Inner<'a>\n}\nfunc <'a> read_outer(o : Outer<'a>) : &'a int { return o.inner.items[1] }\nfunc <'a> apply(f : for<'b> (o : Outer<'b>) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    return f(Outer<'a> { inner : Inner<'a> { items : [x, y, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x] } })\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_outer, &x, &y)\n    return *r - 7\n}\n"
    expect_compile_success(env, "hrtb_large_array_nested_result", ch)
}

@test
func hrtb_large_array_array_of_structs_borrowed_result_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The MIR path is element -> field -> wildcard; the wildcard leaf remains
    // keyed by the owning root and leaf field.
    var ch = "struct Inner<'a> {\n    var items : [65]&'a int\n}\nfunc <'a> read_array(ms : [2]Inner<'a>) : &'a int { return ms[1].items[1] }\nfunc <'a> apply(f : for<'b> (ms : [2]Inner<'b>) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    var ms : [2]Inner<'a>\n    ms[0] = Inner<'a> { items : [x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x] }\n    ms[1] = Inner<'a> { items : [x, y, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x] }\n    return f(ms)\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_array, &x, &y)\n    return *r - 7\n}\n"
    expect_compile_success(env, "hrtb_large_array_array_of_structs_result", ch)
}

@test
func neg_hrtb_large_array_nested_result_aliases(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The returned nested array element is y, so mutating y while the result
    // is live must conflict.
    var ch = "struct Inner<'a> {\n    var items : [65]&'a int\n}\nstruct Outer<'a> {\n    var inner : Inner<'a>\n}\nfunc <'a> read_outer(o : Outer<'a>) : &'a int { return o.inner.items[1] }\nfunc <'a> apply(f : for<'b> (o : Outer<'b>) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    return f(Outer<'a> { inner : Inner<'a> { items : [x, y, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x] } })\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_outer, &x, &y)\n    var q = &mut y\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "hrtb_large_array_nested_alias", ch, "SAFE-BORROW-001")
}

@test
func neg_hrtb_large_array_array_of_structs_result_aliases(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct Inner<'a> {\n    var items : [65]&'a int\n}\nfunc <'a> read_array(ms : [2]Inner<'a>) : &'a int { return ms[1].items[1] }\nfunc <'a> apply(f : for<'b> (ms : [2]Inner<'b>) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    var ms : [2]Inner<'a>\n    ms[0] = Inner<'a> { items : [x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x] }\n    ms[1] = Inner<'a> { items : [x, y, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x] }\n    return f(ms)\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_array, &x, &y)\n    var q = &mut y\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "hrtb_large_array_array_of_structs_alias", ch, "SAFE-BORROW-001")
}

@test
func hrtb_generic_struct_borrowed_result_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A type-parameterised wrapper is substituted from the concrete type
    // argument before its nested reference leaf is path-mapped.
    var ch = "struct View<'a> {\n    var p : &'a int\n}\nstruct Wrap<T> {\n    var value : T\n}\nfunc <'a> read_wrap(w : Wrap<View<'a>>) : &'a int {\n    return w.value.p\n}\nfunc <'a> apply(f : for<'b> (w : Wrap<View<'b>>) => &'b int, x : &'a int) : &'a int {\n    return f(Wrap<View<'a>> { value : View<'a> { p : x } })\n}\npublic func main() : int {\n    var x = 41\n    var r = apply(read_wrap, &x)\n    return *r - 41\n}\n"
    expect_compile_success(env, "hrtb_generic_struct_borrowed_result", ch)
}

@test
func neg_hrtb_variant_array_result_aliases(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Payload/element result loans must stay live. Same-region array elements
    // remain conservatively aliased because the region type does not carry an
    // index.
    var variant = "struct View<'a> {\n    var p : &'a int\n}\nvariant Maybe<'a> {\n    Some(value : View<'a>)\n    None()\n}\nfunc <'a> read_maybe(m : Maybe<'a>) : &'a int {\n    var Some(value) = m else unsafe { unreachable }\n    return value.p\n}\nfunc <'a> apply(f : for<'b> (m : Maybe<'b>) => &'b int, x : &'a int) : &'a int {\n    return f(Maybe.Some(View<'a> { p : x }))\n}\npublic func main() : int {\n    var x = 41\n    var r = apply(read_maybe, &x)\n    var q = &mut x\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "hrtb_variant_result_alias", variant, "SAFE-BORROW-001")
    var array = "struct Refs<'a> {\n    var items : [2]&'a int\n}\nfunc <'a> read_refs(r : Refs<'a>) : &'a int {\n    return r.items[0]\n}\nfunc <'a> apply(f : for<'b> (r : Refs<'b>) => &'b int, x : &'a int, y : &'a int) : &'a int {\n    return f(Refs<'a> { items : [x, y] })\n}\npublic func main() : int {\n    var x = 41\n    var y = 7\n    var r = apply(read_refs, &x, &y)\n    var q = &mut y\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "hrtb_array_result_alias", array, "SAFE-BORROW-001")
    var wrapper = "struct View<'a> {\n    var p : &'a int\n}\nstruct Wrap<T> {\n    var value : T\n}\nfunc <'a> read_wrap(w : Wrap<View<'a>>) : &'a int {\n    return w.value.p\n}\nfunc <'a> apply(f : for<'b> (w : Wrap<View<'b>>) => &'b int, x : &'a int) : &'a int {\n    return f(Wrap<View<'a>> { value : View<'a> { p : x } })\n}\npublic func main() : int {\n    var x = 41\n    var r = apply(read_wrap, &x)\n    var q = &mut x\n    *q = 9\n    return *r\n}\n"
    expect_compile_error(env, "hrtb_generic_struct_result_alias", wrapper, "SAFE-BORROW-001")
}

@test
func hrtb_multiple_references_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Equal-region parameter referents are accepted; they cannot be stored
    // into a longer-lived caller region by this contract.
    var ch = "func sum2(a : &int, b : &int) : int { return *a + *b }\nfunc apply(f : for<'a> (x : &'a int, y : &'a int) => int, a : &int, b : &int) : int {\n    return f(a, b)\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    return apply(sum2, &a, &b)\n}\n"
    expect_compile_success(env, "hrtb_multiple_refs", ch)
}

@test
func hrtb_aggregate_multiple_references_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Two aggregate fields from equal-region parameters are safe to pass as a
    // multi-borrow HRTB contract.
    var ch = "struct Pair<'a> {\n    var left : &'a int\n    var right : &'a int\n}\nfunc <'a> sum_pair(p : Pair<'a>) : int { return *p.left + *p.right }\nfunc <'a> apply_pair(f : for<'b> (p : Pair<'b>) => int, a : &'a int, b : &'a int) : int {\n    return f(Pair<'a> { left : a, right : b })\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    return apply_pair(sum_pair, &a, &b)\n}\n"
    expect_compile_success(env, "hrtb_aggregate_multiple_refs", ch)
}

@test
func hrtb_global_multiple_references_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Global referents are static; a shared-region multi-borrow contract can
    // safely retain them.
    var ch = "var g1 = 1\nvar g2 = 2\nfunc sum2(a : &int, b : &int) : int { return *a + *b }\nfunc apply(f : for<'a> (x : &'a int, y : &'a int) => int) : int {\n    return unsafe(f(&g1, &g2))\n}\npublic func main() : int { return apply(sum2) }\n"
    expect_compile_success(env, "hrtb_global_multiple_refs", ch)
}

@test
func hrtb_mixed_local_parameter_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The callback has no reference-capable storage and returns an int, so the
    // higher-ranked contract cannot retain the local borrow.
    var ch = "func sum2(a : &int, b : &int) : int { return *a + *b }\nfunc apply(f : for<'a> (x : &'a int, y : &'a int) => int, a : &int) : int {\n    var b = 2\n    return f(a, &b)\n}\npublic func main() : int {\n    var a = 1\n    return apply(sum2, &a)\n}\n"
    expect_compile_success(env, "hrtb_mixed_local_param", ch)
}

@test
func hrtb_distinct_parameter_regions_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Read-only shared references carry no retaining storage, so distinct
    // caller regions are safe for this callback.
    var ch = "func sum2(a : &int, b : &int) : int { return *a + *b }\nfunc apply(f : for<'a, 'b> (x : &'a int, y : &'b int) => int, a : &int, b : &int) : int {\n    return f(a, b)\n}\npublic func main() : int {\n    var a = 1\n    var b = 2\n    return apply(sum2, &a, &b)\n}\n"
    expect_compile_success(env, "hrtb_distinct_regions", ch)
}

@test
func hrtb_shared_reference_to_reference_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A shared reference to a reference cannot assign the inner reference, so
    // it exposes no retaining storage even when the caller regions differ.
    var ch = "func <'a> read_shared(x : &'a &'a int, y : &'a int) : int { return **x + *y }\nfunc apply(f : for<'a> (x : &'a &'a int, y : &'a int) => int, x : & &int, y : &int) : int {\n    return f(x, y)\n}\npublic func main() : int {\n    var a = 1\n    var r = &a\n    return apply(read_shared, &r, &a) - 2\n}\n"
    expect_compile_success(env, "hrtb_shared_reference_to_reference", ch)
}

@test
func hrtb_shared_reference_to_aggregate_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A shared reference to a struct with reference fields cannot mutate the
    // fields, so it is not retaining storage for the callback.
    var ch = "@direct_init\nstruct SharedRef<'a> {\n    var r : &'a int\n}\nfunc <'a> read_shared(h : &'a SharedRef<'a>, y : &'a int) : int { return *h.r + *y }\nfunc apply(f : for<'a> (h : &'a SharedRef<'a>, y : &'a int) => int, h : &SharedRef, y : &int) : int {\n    return f(h, y)\n}\npublic func main() : int {\n    var a = 1\n    var y = 2\n    var h = SharedRef<'a> { r : &a }\n    return apply(read_shared, &h, &y) - 3\n}\n"
    expect_compile_success(env, "hrtb_shared_reference_to_aggregate", ch)
}

@test
func neg_hrtb_shared_interior_mutability_retains(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `RefCell` mutates through a shared reference, so it remains retaining
    // storage and still needs a source-to-stored region proof.
    var ch = "using std::RefCell;\nusing std::ref_cell;\nfunc <'a> read_cell(c : &'a RefCell<&'a int>, y : &'a int) : int { return 0 }\nfunc apply(f : for<'a> (c : &'a RefCell<&'a int>, y : &'a int) => int, c : &RefCell<&int>, y : &int) : int {\n    return f(c, y)\n}\npublic func main() : int {\n    var a = 1\n    var r = &a\n    var c = ref_cell(r)\n    return apply(read_cell, &c, &a)\n}\n"
    expect_compile_error_with_mod(env, "hrtb_shared_interior_mutability", ch, "SAFE-BORROW-099", NEG_MOD_STD_CORE)
}

@test
func hrtb_retaining_local_slot_mixed_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The retaining slot is a local, so a parameter referent can be stored in
    // it even though the caller regions differ; the slot dies with the call.
    var ch = "func <'a> stash(slot : &mut &'a int, x : &'a int) : int {\n    *slot = x\n    return 0\n}\nfunc <'a> apply(f : for<'b> (slot : &mut &'b int, x : &'b int) => int, x : &'a int) : int {\n    var y = 7\n    var slot : &int = &y\n    f(&mut slot, x)\n    return *slot\n}\npublic func main() : int {\n    var x = 41\n    return apply(stash, &x) - 41\n}\n"
    expect_compile_success(env, "hrtb_retaining_local_slot", ch)
}

@test
func hrtb_retaining_aggregate_local_slot_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A by-value aggregate carrying a same-binder retaining reference can
    // store a source parameter when the referenced slot is local.
    var ch = "struct Stash<'a> {\n    var slot : &'a mut &'a int\n}\nfunc <'a> stash(st : Stash<'a>, x : &'a int) : int {\n    *st.slot = x\n    return 0\n}\nfunc <'a> apply(f : for<'b> (st : Stash<'b>, x : &'b int) => int, x : &'a int) : int {\n    var slot : &int = x\n    f(Stash<'a> { slot : &mut slot }, x)\n    return *slot\n}\npublic func main() : int {\n    var x = 41\n    return apply(stash, &x) - 41\n}\n"
    expect_compile_success(env, "hrtb_retaining_aggregate_local_slot", ch)
}

@test
func hrtb_retaining_nonlocal_slot_outlives_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The slot's outer borrow outlives the stored region, so a caller-owned
    // retaining slot is safe even though its outer region is distinct.
    var ch = "func <'a> stash(slot : &mut &'a int, x : &'a int) : int {\n    *slot = x\n    return 0\n}\nfunc <'o, 'i> apply(f : for<'c> (slot : &mut &'c int, x : &'c int) => int, slot : &'o mut &'i int, x : &'i int) : int where 'o : 'i {\n    return f(slot, x)\n}\npublic func main() : int {\n    var a = 1\n    var r : &int = &a\n    return apply(stash, &mut r, &a)\n}\n"
    expect_compile_success(env, "hrtb_retaining_nonlocal_slot_outlives", ch)
}

@test
func hrtb_retaining_nonlocal_slot_type_proves_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The slot's pointee and the source already share `'i`; storing the
    // source cannot outlive its own declared region even though the slot's
    // outer borrow region `'o` is unrelated. Only the source-to-stored
    // region edge is required.
    var ch = "func <'a> stash(slot : &mut &'a int, x : &'a int) : int {\n    *slot = x\n    return 0\n}\nfunc <'o, 'i> apply(f : for<'c> (slot : &mut &'c int, x : &'c int) => int, slot : &'o mut &'i int, x : &'i int) : int {\n    return f(slot, x)\n}\npublic func main() : int {\n    var a = 1\n    var r : &int = &a\n    return apply(stash, &mut r, &a) - 0\n}\n"
    expect_compile_success(env, "hrtb_retaining_nonlocal_slot_type_proves", ch)
}

@test
func neg_hrtb_nonlocal_retaining_slot_distinct(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The retaining slot is a caller-owned reference parameter with distinct
    // regions, so the callback could store a shorter borrow into it.
    var ch = "func <'a> stash(slot : &mut &'a int, x : &'a int) : int {\n    *slot = x\n    return 0\n}\nfunc <'a, 'b, 'd> apply(f : for<'c> (slot : &mut &'c int, x : &'c int) => int, slot : &'a mut &'b int, x : &'d int) : int {\n    return f(slot, x)\n}\npublic func main() : int {\n    var a = 1\n    var r : &int = &a\n    var x = 2\n    return apply(stash, &mut r, &x)\n}\n"
    expect_compile_error(env, "hrtb_nonlocal_retaining_slot_distinct", ch, "SAFE-BORROW-099")
}

@test
func hrtb_pointee_static_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A static pointee can only receive static references; no source edge is
    // required when the caller's slot is initialized with a static value.
    var ch = "func read_static(slot : &mut &'static int) : int { return 0 }\nfunc apply(f : for<'a> (slot : &'a mut &'static int) => int) : int {\n    var r : &'static int = unsafe(null as &'static int)\n    return f(&mut r)\n}\npublic func main() : int {\n    return apply(read_static)\n}\n"
    expect_compile_success(env, "hrtb_pointee_static", ch)
}

@test
func hrtb_pointee_free_lifetime_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A free pointee lifetime is accepted when the same lifetime is a direct
    // reference parameter, so the call contract can map the source loan to
    // the retained region.
    var ch = "func <'a, 'o> stash(slot : &'a mut &'o int, x : &'o int) : int {\n    *slot = x\n    return 0\n}\nfunc <'o> apply(f : for<'a> (slot : &'a mut &'o int, x : &'o int) => int, slot : &'o mut &'o int, x : &'o int) : int {\n    return f(slot, x)\n}\npublic func main() : int {\n    var a = 1\n    var r : &int = &a\n    return apply(stash, &mut r, &a) - 0\n}\n"
    expect_compile_success(env, "hrtb_pointee_free_lifetime", ch)
}

@test
func neg_hrtb_opaque_retaining_storage_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // `T` can retain `y` but exposes no named stored region, so the equal
    // call-region shortcut cannot prove what it stores. A caller-owned slot
    // with a caller-owned source must fail closed.
    var ch = "func store_ref(x : &mut &int, y : &int) : int {\n    *x = y\n    return 0\n}\nfunc <T, 'i> apply(f : for<'a> (x : &'a mut T, y : &'a int) => int, x : &'i mut T, y : &'i int) : int {\n    return f(x, y)\n}\npublic func main() : int {\n    var a = 1\n    var r : &int = &a\n    return apply<&int>(store_ref, &mut r, &a)\n}\n"
    expect_compile_error(env, "hrtb_opaque_retaining_storage", ch, "SAFE-BORROW-099")
}

@test
func neg_hrtb_pointee_free_lifetime_without_source(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A free pointee lifetime with no direct reference source cannot be mapped
    // by the call contract; keep the explicit rejection.
    var ch = "func read_slot(slot : &mut &int) : int { return 0 }\nfunc <'o> apply(f : for<'a> (slot : &'a mut &'o int) => int) : int {\n    var x = 1\n    var r : &int = &x\n    return f(&mut r)\n}\npublic func main() : int {\n    return apply(read_slot)\n}\n"
    expect_compile_error(env, "hrtb_pointee_free_lifetime_without_source", ch, "SAFE-BORROW-099")
}

@test
func hrtb_pointee_mismatched_binders_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The outer slot binder and the stored reference binder are distinct.
    // The callback body is checked for both binders and the call-site rule
    // proves every source outlives the stored region.
    var ch = "func <'a, 'b> read_slot(slot : &'a mut &'b int) : int {\n    return **slot\n}\nfunc apply(f : for<'a, 'b> (slot : &'a mut &'b int) => int) : int {\n    var x = 1\n    var r : &int = &x\n    return f(&mut r)\n}\npublic func main() : int {\n    return apply(read_slot) - 1\n}\n"
    expect_compile_success(env, "hrtb_pointee_mismatched_binders", ch)
}

@test
func hrtb_pointee_mismatched_binders_caller_slot_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <'a, 'b> stash(slot : &'a mut &'b int, x : &'b int) : int {\n    *slot = x\n    return 0\n}\nfunc <'o, 'i> apply(f : for<'a, 'b> (slot : &'a mut &'b int, x : &'b int) => int, slot : &'o mut &'i int, x : &'i int) : int {\n    return f(slot, x)\n}\npublic func main() : int {\n    var a = 1\n    var r : &int = &a\n    return apply(stash, &mut r, &a) - 0\n}\n"
    expect_compile_success(env, "hrtb_pointee_mismatched_binders_caller_slot", ch)
}

@test
func neg_hrtb_pointee_mismatched_binders_distinct_source(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // Distinct source/stored regions without an outlives edge stay rejected
    // even when the pointee binders differ from the slot's outer binder.
    var ch = "func <'a, 'b> stash(slot : &'a mut &'b int, x : &'b int) : int {\n    *slot = x\n    return 0\n}\nfunc <'o, 'i, 'd> apply(f : for<'a, 'b> (slot : &'a mut &'b int, x : &'b int) => int, slot : &'o mut &'i int, x : &'d int) : int {\n    return f(slot, x)\n}\npublic func main() : int {\n    var a = 1\n    var r : &int = &a\n    var x = 2\n    return apply(stash, &mut r, &x)\n}\n"
    expect_compile_error(env, "hrtb_pointee_mismatched_binders_distinct_source", ch, "SAFE-BORROW-099")
}

@test
func neg_hrtb_retaining_mixed_local_parameter_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A generic parameter can be a reference slot, so the callback could store
    // the local borrow into caller storage; keep the conservative rejection.
    var ch = "func store_ref(x : &mut &int, y : &int) : int {\n    *x = y\n    return **x\n}\nfunc <T> apply(f : for<'a> (x : &'a mut T, y : &'a int) => int, a : &mut T) : int {\n    var b = 2\n    return f(a, &b)\n}\npublic func main() : int {\n    var a = 1\n    var r : &int = &a\n    return apply<&int>(store_ref, &mut r)\n}\n"
    expect_compile_error(env, "hrtb_retaining_mixed_local_param", ch, "SAFE-BORROW-099")
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
