// Negative tests for the definite-assignment analysis (uninitialized variables).
//
// These verify the compiler *rejects* unsafe patterns and *accepts* the new,
// safe patterns described in the language semantics:
//   * `var x : T` is allowed without `unsafe`.
//   * Reading / taking the address of an uninitialized variable whose type has a
//     destructor is an error.
//   * A full assignment `x = value` initializes an uninitialized variable (and the
//     code generators skip destroying the previous garbage value).
//   * Member / index writes on an uninitialized *non-destructor* type (e.g. arrays)
//     are allowed; on a destructor type they are still an error.
//   * Taking a pointer/reference to an uninitialized variable requires `unsafe(...)`.
//   * `unsafe var` / `unsafe const` declarations are no longer supported.

@test
func neg_uninit_member_write_on_destructible_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // `x.inner = 5` accesses a field of an uninitialized destructor-bearing struct.
    var ch = "struct Container {\n    var inner : i32\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    var x : Container\n    x.inner = 5\n    return 0\n}\n"
    expect_compile_error(env, "uninit_member_write_on_destructible", ch, "uninitialized variable")
}

@test
func neg_uninit_read_on_destructible_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Container {\n    @delete func delete(&mut self) { }\n}\nfunc use(c : Container) { }\npublic func main() : int {\n    var c : Container\n    use(c)\n    return 0\n}\n"
    expect_compile_error(env, "uninit_read_on_destructible", ch, "uninitialized variable")
}

@test
func neg_uninit_full_assignment_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // First assignment to an uninitialized variable is its initialization.
    var ch = "struct Container {\n    var field : i32\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    var c : Container\n    c = Container { field : 0 }\n    return 0\n}\n"
    expect_compile_success(env, "uninit_full_assignment_ok", ch)
}

@test
func neg_uninit_addr_of_without_unsafe_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Taking a pointer to an uninitialized variable needs `unsafe(...)`.
    var ch = "func take(p : *mut i32) { }\npublic func main() : int {\n    var x : i32\n    take(&raw mut x)\n    return 0\n}\n"
    expect_compile_error(env, "uninit_addr_of_without_unsafe", ch, "uninitialized variable")
}

@test
func neg_uninit_addr_of_with_unsafe_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func take(p : *mut i32) { }\npublic func main() : int {\n    var x : i32\n    take(unsafe(&raw mut x))\n    return 0\n}\n"
    expect_compile_success(env, "uninit_addr_of_with_unsafe_ok", ch)
}

@test
func neg_uninit_var_decl_accepted(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // An uninitialized `var` is now accepted directly (no `unsafe` keyword needed).
    // Assigning a value later fully initializes it.
    var ch = "public func main() : int {\n    var x : i32\n    x = 5\n    return x\n}\n"
    expect_compile_success(env, "uninit_var_decl_accepted", ch)
}

@test
func neg_unsafe_var_decl_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // `unsafe var` is no longer supported by the parser.
    var ch = "public func main() : int {\n    unsafe var x : i32\n    x = 5\n    return x\n}\n"
    expect_compile_error(env, "unsafe_var_decl_rejected", ch, "no longer supported")
}

@test
func neg_unsafe_const_decl_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // `unsafe const` is no longer supported by the parser.
    var ch = "public func main() : int {\n    unsafe const x : i32 = 5\n    return x\n}\n"
    expect_compile_error(env, "unsafe_const_decl_rejected", ch, "no longer supported")
}

@test
func neg_uninit_array_index_write_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Arrays have no destructor, so writing through them while uninitialized is fine.
    var ch = "public func main() : int {\n    var s : [32]i32\n    s[0] = 5\n    return s[0]\n}\n"
    expect_compile_success(env, "uninit_array_index_write_ok", ch)
}

@test
func neg_uninit_branch_maybe_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // After `if (true) { c = ... }` the variable is only *maybe* initialized.
    var ch = "struct Container {\n    @delete func delete(&mut self) { }\n}\nfunc use(c : Container) { }\npublic func main() : int {\n    var c : Container\n    if(true) { c = Container {} }\n    use(c)\n    return 0\n}\n"
    expect_compile_error(env, "uninit_branch_maybe", ch, "uninitialized variable")
}

@test
func neg_uninit_branch_both_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Assigned on both branches -> definitely initialized.
    var ch = "struct Container {\n    @delete func delete(&mut self) { }\n}\nfunc use(c : Container) { }\npublic func main() : int {\n    var c : Container\n    if(true) { c = Container {} } else { c = Container {} }\n    use(c)\n    return 0\n}\n"
    expect_compile_success(env, "uninit_branch_both_ok", ch)
}

@test
func neg_uninit_nondestructor_read_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Reading an uninitialized non-destructor variable is allowed (no destruction).
    var ch = "public func main() : int {\n    var x : i32\n    var y = x + 1\n    return y\n}\n"
    expect_compile_success(env, "uninit_nondestructor_read_ok", ch)
}

// ============================================================================
// A variable declared without an initializer can be initialized in many ways.
// Every one of these is a *first initialization* (full assignment) and must be
// accepted by the compiler — it must NOT try to destroy the previous (garbage)
// value, and after the assignment the variable is definitely initialized.
// ============================================================================

@test
func neg_uninit_init_primitive_literal(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x : i32\n    x = 5\n    return x\n}\n"
    expect_compile_success(env, "uninit_init_primitive_literal", ch)
}

@test
func neg_uninit_init_from_function_call(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func f() : i32 { return 7 }\npublic func main() : int {\n    var x : i32\n    x = f()\n    return x\n}\n"
    expect_compile_success(env, "uninit_init_from_function_call", ch)
}

@test
func neg_uninit_init_from_initialized_var(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Copying from an already-initialized variable is a valid first init.
    var ch = "public func main() : int {\n    var a : i32 = 3\n    var b : i32\n    b = a\n    return b\n}\n"
    expect_compile_success(env, "uninit_init_from_initialized_var", ch)
}

@test
func neg_uninit_init_nested_block(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Assignment inside a nested block initializes the outer variable.
    var ch = "public func main() : int {\n    var x : i32\n    { x = 9 }\n    return x\n}\n"
    expect_compile_success(env, "uninit_init_nested_block", ch)
}

@test
func neg_uninit_init_reassign(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // A second assignment is a normal assignment once already initialized.
    var ch = "public func main() : int {\n    var x : i32\n    x = 1\n    x = 2\n    return x\n}\n"
    expect_compile_success(env, "uninit_init_reassign", ch)
}

@test
func neg_uninit_init_destructible_literal(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Container {\n    var field : i32\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    var c : Container\n    c = Container { field : 0 }\n    return c.field\n}\n"
    expect_compile_success(env, "uninit_init_destructible_literal", ch)
}

@test
func neg_uninit_init_destructible_via_func(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Container {\n    var field : i32\n    @delete func delete(&mut self) { }\n}\nfunc mk() : Container { return Container { field : 0 } }\npublic func main() : int {\n    var c : Container\n    c = mk()\n    return c.field\n}\n"
    expect_compile_success(env, "uninit_init_destructible_via_func", ch)
}

@test
func neg_uninit_init_destructible_in_unsafe_block(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Container {\n    var field : i32\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    var c : Container\n    unsafe { c = Container { field : 0 } }\n    return c.field\n}\n"
    expect_compile_success(env, "uninit_init_destructible_in_unsafe_block", ch)
}

@test
func neg_uninit_init_destructible_nested_block(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Container {\n    var field : i32\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    var c : Container\n    { c = Container { field : 0 } }\n    return c.field\n}\n"
    expect_compile_success(env, "uninit_init_destructible_nested_block", ch)
}

@test
func neg_uninit_init_array_in_loop(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Arrays have no destructor, so writing through them while uninitialized is
    // allowed; after the loop the array is treated as initialized.
    var ch = "public func main() : int {\n    var s : [32]i32\n    for(var i : int = 0; i < 32; i++) { s[i] = i }\n    return s[0]\n}\n"
    expect_compile_success(env, "uninit_init_array_in_loop", ch)
}

@test
func neg_uninit_init_read_after_first_init(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Reading the variable after a full assignment is fine (it is initialized).
    var ch = "struct Container {\n    var field : i32\n    @delete func delete(&mut self) { }\n}\npublic func main() : int {\n    var c : Container\n    c = Container { field : 4 }\n    var y = c.field\n    return y\n}\n"
    expect_compile_success(env, "uninit_init_read_after_first_init", ch)
}
