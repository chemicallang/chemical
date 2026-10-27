// Unpin / Pin (S4.1) negative and positive tests.

@test
func pin_unpin_type_ok(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var x = 41\n    var p = core::pin(&mut x)\n    var r = p.get_mut()\n    *r = 42\n    return *p.get_ref()\n}\n"
    expect_compile_success_with_mod(env, "pin_unpin_ok", ch, NEG_MOD_CORE)
}

@test
func pin_phantom_pinned_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Pinned {\n    var marker : core::PhantomPinned\n}\npublic func main() : int {\n    var x = Pinned { marker : core::PhantomPinned {} }\n    var p = core::pin(&mut x)\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "pin_phantom_pinned", ch, "Unpin", NEG_MOD_CORE)
}

@test
func pin_get_mut_phantom_pinned_rejected(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Pinned {\n    var marker : core::PhantomPinned\n}\npublic func main() : int {\n    var x = Pinned { marker : core::PhantomPinned {} }\n    var p = unsafe(core::pin_unchecked(&raw mut x))\n    var r = p.get_mut()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "pin_get_mut_pinned", ch, "Unpin", NEG_MOD_CORE)
}

@test
func pin_unchecked_mut_requires_unsafe(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Pinned {\n    var marker : core::PhantomPinned\n}\npublic func main() : int {\n    var x = Pinned { marker : core::PhantomPinned {} }\n    var p = unsafe(core::pin_unchecked(&raw mut x))\n    var r = p.get_unchecked_mut()\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "pin_unchecked_mut", ch, "SAFE-UNSAFE-002", NEG_MOD_CORE)
}

@test
func pin_get_ref_does_not_require_unpin(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Pinned {\n    var marker : core::PhantomPinned\n}\npublic func main() : int {\n    var x = Pinned { marker : core::PhantomPinned {} }\n    var p = unsafe(core::pin_unchecked(&raw mut x))\n    var r = p.get_ref()\n    return 0\n}\n"
    expect_compile_success_with_mod(env, "pin_get_ref_ok", ch, NEG_MOD_CORE)
}
