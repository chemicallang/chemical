// Move Semantics Negative Tests
// Tests that the compiler correctly enforces move semantics.

@test
func neg_move_after_use(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    var c = a\n}\n"
    expect_compile_error(env, "move_after_use", ch, "SAFE-MOVE-001")
}

@test
func neg_move_non_struct_to_struct(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = 42\n    var b : S = a\n}\n"
    expect_compile_error(env, "move_non_struct_to_struct", ch, "does not satisfy")
}

@test
func neg_move_twice(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    var c = a\n}\n"
    expect_compile_error(env, "move_twice", ch, "SAFE-MOVE-001")
}

@test
func neg_use_after_move_has_code(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    var c = a.x\n}\n"
    expect_compile_error(env, "use_after_move_code", ch, "SAFE-MOVE-001")
}

@test
func neg_move_twice_has_code(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    var c = a\n}\n"
    expect_compile_error(env, "move_twice_code", ch, "SAFE-MOVE-001")
}

@test
func neg_zeroed_with_dtor(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var s = zeroed<S>()\n}\n"
    expect_compile_error(env, "zeroed_with_dtor", ch, "@allow_zeroed")
}

@test
func neg_move_into_fun_twice(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc consume(s : S) { }\nfunc main() {\n    var a = S { x : 1 }\n    consume(a)\n    consume(a)\n}\n"
    expect_compile_error(env, "move_into_fn_twice", ch, "SAFE-MOVE-001")
}

@test
func neg_deref_non_copy_generic(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // De-referencing a `*T` where `T` is a generic parameter that is not `Copy`
    // is rejected (the value would be copied out of borrowed memory).
    var ch = "struct NonCopy {\n    @delete\n    func delete(&mut self) { }\n}\nfunc <T> get_val(p : *T) : T {\n    return *p\n}\nfunc main() {\n    var n = NonCopy {}\n    var p = &raw n\n    var v = get_val(p)\n}\n"
    expect_compile_error(env, "deref_non_copy_generic", ch, "Copy")
}

@test
func neg_index_non_copy_generic(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Indexing a `*T` where `T` is a generic parameter that is not `Copy` is rejected
    // (the element would be copied out of borrowed memory). Arrays of destructible
    // element types are supported, so this is the only restricted indexing form.
    var ch = "struct NonCopy {\n    @delete\n    func delete(&mut self) { }\n}\nfunc <T> get_at(p : *T) : T {\n    return p[0]\n}\nfunc main() {\n    var n = NonCopy {}\n    var p = &raw n\n    var v = get_at(p)\n}\n"
    expect_compile_error(env, "index_non_copy_generic", ch, "Copy")
}

@test
func array_of_destructible_elements_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // an array of a destructible element type is fine, each element is destroyed
    var ch = "struct DT {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var a = DT { x : 1 }\n    var b = DT { x : 2 }\n    var arr : [2]DT = [a, b]\n    return arr[0].x\n}\n"
    expect_compile_success(env, "array_of_destructible_elements_allowed", ch)
}

@test
func neg_move_out_of_array_dynamic_index(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct DT {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var arr : [2]DT = [DT { x : 1 }, DT { x : 2 }]\n    var idx = 0\n    var moved = arr[idx]\n    return moved.x\n}\n"
    expect_compile_error(env, "move_out_of_array_dynamic", ch, "SAFE-MOVE-001")
}

@test
func neg_move_out_of_array_constant_index(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct DT {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    var arr : [2]DT = [DT { x : 1 }, DT { x : 2 }]\n    var moved = arr[0]\n    return moved.x\n}\n"
    expect_compile_error(env, "move_out_of_array_constant", ch, "SAFE-MOVE-001")
}

@test
func array_index_copy_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var arr : [2]int = [1, 2]\n    var idx = 0\n    var copied = arr[idx]\n    return copied\n}\n"
    expect_compile_success(env, "array_index_copy_allowed", ch)
}

@test
func neg_move_unknown_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nfunc main() {\n    var a = S { x : 1 }\n    var b = a\n    b = a\n}\n"
    expect_compile_error(env, "move_unknown", ch, "SAFE-MOVE-001")
}

@test
func neg_unsafe_does_not_disable_init(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // unsafe(...) is a marker for inherently unchecked operations. It must not
    // disable definite-assignment/ownership checking, so the MIR dataflow still
    // rejects moving the uninitialized destructor-bearing value.
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    unsafe {\n        var a : S\n        var b = a\n        return b.x\n    }\n}\n"
    expect_compile_error(env, "unsafe_does_not_disable_init", ch, "SAFE-INIT-001")
}

@test
func neg_conditional_init_use_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    unsafe {\n        var cond = true\n        var s : S\n        if(cond) { s = S { x : 1 } }\n        return s.x\n    }\n}\n"
    expect_compile_error(env, "conditional_init_use", ch, "SAFE-MOVE-001")
}

@test
func conditional_init_without_use_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\npublic func main() : int {\n    unsafe {\n        var cond = true\n        var s : S\n        if(cond) { s = S { x : 1 } }\n        return 0\n    }\n}\n"
    expect_compile_success(env, "conditional_init_ok", ch)
}

@test
func neg_constant_array_index_out_of_bounds(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var a : [3]int\n    a[3] = 1\n    return 0\n}\n"
    expect_compile_error(env, "array_oob", ch, "SAFE-BOUNDS-001")
}

@test
func constant_array_index_in_bounds_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var a : [3]int\n    a[2] = 1\n    return a[2]\n}\n"
    expect_compile_success(env, "array_in_bounds", ch)
}

// ── partial initialization (S3.1c) ──────────────────────────────────────────

@test
func partial_owned_field_init_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A struct with generated drop glue can be built field by field: every
    // owned field carries its own drop flag, so only the initialized ones are
    // destroyed at scope exit.
    var ch = "using std::string\nstruct S {\n    var a : string\n    var b : string\n}\npublic func main() : int {\n    unsafe {\n        var s : S\n        s.a = string(\"x\")\n        s.b = string(\"y\")\n    }\n    return 0\n}\n"
    expect_compile_success_with_mod(env, "partial_owned_field_init_ok", ch, NEG_MOD_STD_CORE)
}

@test
func neg_partial_init_with_user_destructor(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A user-written destructor expects a whole value, so field-by-field
    // initialization stays rejected for such a struct.
    var ch = "using std::string\nstruct S {\n    var a : string\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    unsafe {\n        var s : S\n        s.a = string(\"x\")\n    }\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "partial_user_dtor_init", ch, "SAFE-INIT-001", NEG_MOD_STD_CORE)
}

@test
func partial_owned_field_init_after_whole_value_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using std::string\nstruct S {\n    var a : string\n    var b : string\n}\npublic func main() : int {\n    var s = S { a : string(\"x\"), b : string(\"y\") }\n    s.a = string(\"z\")\n    return s.a.size() as int\n}\n"
    expect_compile_success_with_mod(env, "partial_after_whole_ok", ch, NEG_MOD_STD_CORE)
}

@test
func partial_array_element_init_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A bare fixed array of destructible elements can be filled by constant
    // index; each element has its own drop flag.
    var ch = "struct DT {\n    var x : int\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    unsafe {\n        var arr : [2]DT\n        arr[0] = DT { x : 1 }\n    }\n    return 0\n}\n"
    expect_compile_success(env, "partial_array_element_init_ok", ch)
}

@test
func neg_partial_array_dynamic_index(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A dynamic index cannot update a per-element drop flag, so it may only
    // write into an already fully initialized array.
    var ch = "struct DT {\n    var x : int\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    unsafe {\n        var arr : [2]DT\n        var i = 0\n        arr[i] = DT { x : 1 }\n    }\n    return 0\n}\n"
    expect_compile_error(env, "partial_array_dynamic_index", ch, "SAFE-INIT-001")
}

@test
func neg_field_write_after_move(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "using std::string\nstruct S {\n    var a : string\n    var b : string\n}\npublic func main() : int {\n    var s = S { a : string(\"x\"), b : string(\"y\") }\n    var t = s\n    s.a = string(\"z\")\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "field_write_after_move", ch, "SAFE-MOVE-001", NEG_MOD_STD_CORE)
}

@test
func neg_read_uninitialized_nested_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A chained field read (`s.b.id`) goes through a derived field address;
    // the dataflow must still resolve it to the uninitialized `b` subobject.
    var ch = "struct T {\n    var id : int\n    @delete func delete(&mut self) { }\n}\nstruct S {\n    var a : T\n    var b : T\n}\npublic func main() : int {\n    unsafe {\n        var s : S\n        s.a = T { id : 1 }\n        return s.b.id\n    }\n}\n"
    expect_compile_error(env, "read_uninit_nested_field", ch, "SAFE-INIT-001")
}

@test
func neg_read_uninitialized_grandchild_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A nested write through a derived field address must not initialize the
    // sibling leaf; reading the sibling is still an uninitialized read.
    var ch = "struct T {\n    var id : int\n    @delete func delete(&mut self) { }\n}\nstruct Inner {\n    var a : T\n    var b : T\n}\nstruct Outer {\n    var inner : Inner\n}\npublic func main() : int {\n    unsafe {\n        var o : Outer\n        o.inner.a = T { id : 1 }\n        return o.inner.b.id\n    }\n}\n"
    expect_compile_error(env, "read_uninit_grandchild", ch, "SAFE-INIT-001")
}

@test
func nested_field_init_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct T {\n    var id : int\n    @delete func delete(&mut self) { }\n}\nstruct Inner {\n    var a : T\n    var b : T\n}\nstruct Outer {\n    var inner : Inner\n}\npublic func main() : int {\n    unsafe {\n        var o : Outer\n        o.inner.a = T { id : 1 }\n        o.inner.b = T { id : 2 }\n        return o.inner.b.id\n    }\n}\n"
    expect_compile_success(env, "nested_field_init_ok", ch)
}

@test
func neg_return_partially_initialized(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct T {\n    var id : int\n    @delete func delete(&mut self) { }\n}\nstruct S {\n    var a : T\n    var b : T\n}\nfunc make() : S {\n    unsafe {\n        var s : S\n        s.a = T { id : 1 }\n        return s\n    }\n}\npublic func main() : int { return 0 }\n"
    expect_compile_error(env, "return_partially_initialized", ch, "SAFE-INIT-001")
}

@test
func full_field_init_is_returnable(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct T {\n    var id : int\n    @delete func delete(&mut self) { }\n}\nstruct S {\n    var a : T\n    var b : T\n}\nfunc make() : S {\n    unsafe {\n        var s : S\n        s.a = T { id : 1 }\n        s.b = T { id : 2 }\n        return s\n    }\n}\npublic func main() : int { return make().b.id }\n"
    expect_compile_success(env, "full_field_init_returnable", ch)
}

@test
func scalar_field_init_of_owned_free_local_is_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A destructor-bearing type with no owned fields can still be initialized
    // by hand (the `var s : string; s.state = ...` pattern in std): ownership
    // has nothing to track, so the value is usable after a field write.
    var ch = "struct C {\n    var x : int\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    unsafe {\n        var c : C\n        c.x = 5\n        return c.x\n    }\n}\n"
    expect_compile_success(env, "scalar_field_init_ok", ch)
}
