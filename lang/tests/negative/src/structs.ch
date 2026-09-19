// Struct & Variant Negative Tests
// Tests that the compiler correctly rejects invalid struct and variant definitions.

@test
func neg_struct_missing_field_type(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo {\n    var x\n}\nfunc main() {}\n"
    expect_compile_error(env, "struct_field_no_type", ch, "expected")
}

@test
func neg_struct_duplicate_field(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo {\n    var x : int\n    var x : float\n}\nfunc main() {}\n"
    expect_compile_error(env, "struct_dup_field", ch, "already exists")
}

@test
func neg_variant_no_members(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "variant Empty {}\nfunc main() {}\n"
    expect_compile_error(env, "empty_variant", ch, "variant")
}

@test
func neg_self_referential_struct(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Node {\n    var next : Node\n}\nfunc main() {}\n"
    expect_compile_error(env, "self_ref_struct", ch, "recursion")
}

@test
func neg_struct_field_void(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo {\n    var data : void\n}\nfunc main() {}\n"
    expect_compile_error(env, "struct_field_void", ch, "void")
}

@test
func neg_invalid_enum_value(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // 256 does not fit the u8 underlying type of the enum
    var ch = "enum Color : u8 {\n    Red = 256\n    Green\n    Blue\n}\nfunc main() {}\n"
    expect_compile_error(env, "invalid_enum_value", ch, "does not fit")
}

@test
func neg_invalid_enum_value_signed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // 200 does not fit a signed i8 underlying type
    var ch = "enum Small : i8 {\n    A = 200\n}\nfunc main() {}\n"
    expect_compile_error(env, "invalid_enum_value_signed", ch, "does not fit")
}

@test
func neg_enum_value_valid_fits(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // the largest value that does fit the underlying type is accepted
    var ch = "enum Fits : u8 {\n    Max = 255\n}\npublic func main() : int {\n    return Fits.Max as int\n}\n"
    expect_compile_success(env, "enum_value_valid_fits", ch)
}

@test
func neg_struct_with_underscore_only_fields(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo {\n    var _ : int\n}\nfunc main() {}\n"
    expect_compile_error(env, "underscore_field", ch, "expected")
}

@test
func neg_variant_duplicate_constructor(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "variant Result {\n    Ok : int\n    Ok : float\n}\nfunc main() {}\n"
    expect_compile_error(env, "variant_dup_ctor", ch, "already exists")
}

@test
func neg_struct_missing_brace(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo {\n    var x : int\n}\nstruct Bar {\n    var y : float\nfunc main() {}\n"
    expect_compile_error(env, "missing_struct_brace", ch, "expected")
}

@test
func neg_struct_lifetime_too_many(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct View 'a 'b {\n    var data : *char\n}\nfunc main() {}\n"
    expect_compile_error(env, "too_many_lifetime_params", ch, "lifetime")
}
