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

// A generic function used as a value must instantiate the named type arguments and
// yield a pointer to the concrete function. `ident<int>` is not a call, so the
// arguments have nowhere to live on a FunctionCall — they are stored on the
// identifier and resolved during symbol resolution.
func <T> generic_identity(x : T) : T { return x }

@retained
func <T> pick_generic_identity() : (x : T) => T {
    return generic_identity<T>
}

// global initializer: uses the generic instantiation pass rather than the body pass
var global_generic_identity : (x : int) => int = generic_identity<int>

func test_native_generic_fn_reference() {
    test("generic function reference is instantiated (B10)", () => {
        var g : (x : int) => int = generic_identity<int>
        return g(41) == 41
    })
    test("generic function reference inside a generic function is instantiated (B10)", () => {
        var f : (x : int) => int = pick_generic_identity<int>()
        return f(42) == 42
    })
    test("generic function reference in a global initializer is instantiated (B10)", () => {
        return global_generic_identity(43) == 43
    })
    test("namespaced generic function reference is instantiated (B10)", () => {
        var f : (x : int) => int = genref_ns::ident<int>
        return f(44) == 44
    })
    test("parenthesized generic function reference is instantiated (B10)", () => {
        var f : (x : int) => int = (generic_identity<int>)
        return f(45) == 45
    })
    test("generic function reference passed as an argument is instantiated (B10)", () => {
        return genref_call(genref_ns::ident<int>, 46) == 46
    })
    test("generic function reference with multiple parameters is instantiated (B10)", () => {
        var f : (x : int, y : int) => int = genref_two<int>
        return f(47, 0) == 47
    })
    test("generic function reference with mixed generic parameters is instantiated (B10)", () => {
        var f : (x : int, y : long) => int = genref_first<int, long>
        return f(48, 1l) == 48
    })
}

namespace genref_ns {

    func <T> ident(x : T) : T { return x }

}

func <T> genref_two(x : T, y : T) : T { return x }

func <T, U> genref_first(x : T, y : U) : T { return x }

func genref_call(f : (x : int) => int, v : int) : int { return f(v) }

