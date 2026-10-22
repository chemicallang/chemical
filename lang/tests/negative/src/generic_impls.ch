// Generic implementation (`impl<T> Trait for Type<T>`) diagnostics and the
// zero-cost-when-unused invariant.

@test
func generic_impl_top_level_allowed(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // The feature itself: a top-level generic impl is accepted and usable.
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Box<T> {\n    var value : T\n}\nimpl<T> Show for Box<T> {\n    func show(&self) : int { return 1 }\n}\npublic func main() : int {\n    var b = Box<int> { value : 2 }\n    return b.show() - 1\n}\n"
    expect_compile_success(env, "generic_impl_top_level", ch)
}

@test
func neg_generic_impl_unused_parameter(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Box<T> {\n    var value : T\n}\nimpl<T, U> Show for Box<T> {\n    func show(&self) : int { return 1 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "generic_impl_unused_param", ch, "is not used by the implemented type")
}

@test
func neg_generic_impl_non_generic_target(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Plain {\n    var value : int\n}\nimpl<T> Show for Plain {\n    func show(&self) : int { return 1 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "generic_impl_non_generic_target", ch, "require a generic target type")
}

@test
func neg_generic_impl_duplicate_parameter(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Pair<A, B> {\n    var a : A\n    var b : B\n}\nimpl<T> Show for Pair<T, T> {\n    func show(&self) : int { return 1 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "generic_impl_duplicate_param", ch, "is used more than once")
}

@test
func neg_generic_impl_argument_count(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Pair<A, B> {\n    var a : A\n    var b : B\n}\nimpl<T> Show for Pair<T> {\n    func show(&self) : int { return 1 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "generic_impl_arg_count", ch, "expects 2 type arguments")
}

@test
func neg_generic_impl_duplicate(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Box<T> {\n    var value : T\n}\nimpl<T> Show for Box<T> {\n    func show(&self) : int { return 1 }\n}\nimpl<T> Show for Box<T> {\n    func show(&self) : int { return 2 }\n}\nfunc main() {}\n"
    expect_compile_error(env, "generic_impl_duplicate", ch, "duplicate implementation")
}

@test
func neg_generic_impl_unsatisfied_bound(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Describe {\n    func describe(&self) : int\n}\ninterface Extra {\n    func extra(&self) : int\n}\nstruct Box<T> {\n    var value : T\n}\nstruct Nope {}\nimpl<T : Describe> Extra for Box<T> {\n    func extra(&self) : int { return 1 }\n}\npublic func main() : int {\n    var b = Box<Nope> { value : Nope {} }\n    return b.extra()\n}\n"
    expect_compile_error(env, "generic_impl_unsatisfied_bound", ch, "does not satisfy bound")
}

@test
func generic_impl_unused_emits_no_code(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    // Box<int> is instantiated (the struct is emitted), but Show is never
    // demanded, so the generic impl must not be instantiated or emitted.
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Box<T> {\n    var value : T\n}\nimpl<T> Show for Box<T> {\n    func show(&self) : int { return 1 }\n}\npublic func main() : int {\n    var b = Box<int> { value : 2 }\n    return b.value - 2\n}\n"
    expect_emitted_symbol(env, "generic_impl_unused_no_code", ch, "_Show_Box__cgs__0_show", false)
}

@test
func generic_impl_used_emits_code(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "interface Show {\n    func show(&self) : int\n}\nstruct Box<T> {\n    var value : T\n}\nimpl<T> Show for Box<T> {\n    func show(&self) : int { return 1 }\n}\npublic func main() : int {\n    var b = Box<int> { value : 2 }\n    return b.show() - 1\n}\n"
    expect_emitted_symbol(env, "generic_impl_used_emits_code", ch, "_Show_Box__cgs__0_show", true)
}
