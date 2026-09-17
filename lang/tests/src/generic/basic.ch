// Native-only generic tests that can't run in interpret mode:
func test_native_generic_specifics() {
    test("monomorphization of a struct present in a module that is not directly inherited works", () => {
        var s = ExposedGenSecond<int> { value : 9473 }
        return s.give() == 9474
    })
}

// A generic struct with a function typed field, initialized with a lambda, must be
// specialized like any other member. The lambda's return type is linked against the
// master member's function type, so it references the container's own generic
// parameters; during instantiation those must be replaced by the concrete arguments.
struct GenericFnField<T> {
    var produce : () => T
}

@retained
func <T> make_generic_fn_field() : GenericFnField<T> {
    return GenericFnField<T> { produce : () => zeroed<T>() }
}

func test_native_generic_fn_field() {
    test("generic struct with a function typed field is specialized (B12)", () => {
        var holder = make_generic_fn_field<int>()
        return holder.produce() == 0
    })
}

