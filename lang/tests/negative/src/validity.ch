// Value validity rules (M5.1).

@test
func zeroed_reference_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // A zeroed reference is null: invalid regardless of the struct's
    // constructor/destructor status.
    var ch = "struct S {\n    var r : &int\n}\npublic func main() : int {\n    var s : S = zeroed<S>()\n    return 0\n}\n"
    expect_compile_error(env, "zeroed_reference_field", ch, "SAFE-VALID-001")
}

@test
func zeroed_plain_struct_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct S {\n    var x : int\n}\npublic func main() : int {\n    var s : S = zeroed<S>()\n    return s.x\n}\n"
    expect_compile_success(env, "zeroed_plain_struct", ch)
}

@test
func zeroed_phantom_data_reference_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // PhantomData names a reference but stores nothing; zeroing it is valid.
    var ch = "struct P<'a> {\n    var p : std::PhantomData<&'a int>\n}\npublic func main() : int {\n    var x : P = zeroed<P>()\n    return 0\n}\n"
    expect_compile_success_with_mod(env, "zeroed_phantom_data", ch, NEG_MOD_STD_CORE)
}

@test
func zeroed_generic_reference_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The concrete generic argument is a reference; the specialized field is
    // still storage, so zeroing it is invalid.
    var ch = "struct W<T> {\n    var value : T\n}\npublic func main() : int {\n    var w : W<&int> = zeroed<W<&int>>()\n    return 0\n}\n"
    expect_compile_error(env, "zeroed_generic_reference", ch, "SAFE-VALID-001")
}

@test
func zeroable_impl_does_not_bypass_reference_check(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // An unsafe Zeroable impl cannot make a null reference valid either; the
    // use-site reference check still rejects it.
    var ch = "struct S {\n    var r : &int\n}\nunsafe impl Zeroable for S { }\npublic func main() : int {\n    var s : S = zeroed<S>()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "zeroable_impl_reference", ch, "SAFE-VALID-001", NEG_MOD_STD_CORE)
}

@test
func zeroed_unsafe_escape_allowed(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    // The explicit unsafe form is the escape hatch.
    var ch = "struct S {\n    var r : &int\n}\npublic func main() : int {\n    var s : S = zeroed:unsafe<S>()\n    return 0\n}\n"
    expect_compile_success(env, "zeroed_unsafe_reference", ch)
}

@test
func zeroed_generic_missing_bound_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> make() : T {\n    return zeroed<T>()\n}\npublic func main() : int {\n    return make<int>()\n}\n"
    expect_compile_error_with_mod(env, "zeroed_generic_missing_bound", ch, "SAFE-ZERO-001", NEG_MOD_STD_CORE)
}

@test
func zeroed_generic_bound_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "func <T> make() : T where T: Zeroable {\n    return zeroed<T>()\n}\npublic func main() : int {\n    return make<int>()\n}\n"
    expect_compile_success_with_mod(env, "zeroed_generic_bound", ch, NEG_MOD_STD_CORE)
}

@test
func zeroed_explicit_impl_with_dtor_ok(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "struct S {\n    var x : int\n    @delete\n    func delete(&mut self) { }\n}\nunsafe impl Zeroable for S { }\npublic func main() : int {\n    var s = zeroed<S>()\n    return s.x\n}\n"
    expect_compile_success_with_mod(env, "zeroed_explicit_impl", ch, NEG_MOD_STD_CORE)
}

@test
func zeroed_struct_with_nonzero_enum_field_rejected(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "enum Color : int {\n    Red = 1,\n    Green = 2\n}\nstruct S {\n    var c : Color\n}\npublic func main() : int {\n    var s = zeroed<S>()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "zeroed_nonzero_enum_field", ch, "SAFE-ZERO-001", NEG_MOD_STD_CORE)
}
