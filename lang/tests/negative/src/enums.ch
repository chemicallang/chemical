// Enum Negative Tests
// Enum member values are emitted verbatim into the generated C, so a member whose
// value (transitively) refers back to itself used to send code generation into
// infinite recursion (stack overflow) as soon as the member was referenced.

@test
func neg_enum_member_self_reference(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "enum E : int {\n    A = A\n}\npublic func main() : int {\n    return E.A as int\n}\n"
    expect_compile_error(env, "enum_member_self_reference", ch, "refers back to itself")
}

@test
func neg_enum_member_self_reference_unused(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // the cycle must be reported even when the enum is never used anywhere
    var ch = "enum E : int {\n    A = A\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error(env, "enum_member_self_reference_unused", ch, "refers back to itself")
}

@test
func neg_enum_member_mutual_cycle(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "enum E : int {\n    A = B\n    B = A\n}\npublic func main() : int {\n    return E.A as int\n}\n"
    expect_compile_error(env, "enum_member_mutual_cycle", ch, "refers back to itself")
}

@test
func neg_enum_member_three_way_cycle(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "enum E : int {\n    A = B\n    B = C\n    C = A\n}\npublic func main() : int {\n    return E.A as int\n}\n"
    expect_compile_error(env, "enum_member_three_way_cycle", ch, "refers back to itself")
}

@test
func neg_enum_member_cross_enum_cycle(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // a cycle that goes through another enum declaration must be caught as well
    var ch = "enum E : int {\n    A = F.B\n}\nenum F : int {\n    B = E.A\n}\npublic func main() : int {\n    return E.A as int\n}\n"
    expect_compile_error(env, "enum_member_cross_enum_cycle", ch, "refers back to itself")
}

@test
func enum_member_previous_reference_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // referencing an already defined member is a normal constant expression, only
    // cycles are rejected
    var ch = "enum E : int {\n    A = 1\n    B = A + 2\n}\npublic func main() : int {\n    return E.B as int\n}\n"
    expect_compile_success(env, "enum_member_previous_reference_allowed", ch)
}

@test
func enum_empty_member_values_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // members without an explicit value keep using their positional index
    var ch = "enum E : int {\n    A\n    B\n}\npublic func main() : int {\n    return E.B as int\n}\n"
    expect_compile_success(env, "enum_empty_member_values_allowed", ch)
}
