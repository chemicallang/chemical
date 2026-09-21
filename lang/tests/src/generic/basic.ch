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

// A generic function used as a *type argument* (`genref_wrap<genref_wrap<int>>`) refers
// to the instantiation `genref_wrap<int>`, whose type is a function type. The argument
// must resolve to that concrete function type — resolving it to the declaration's master
// signature instead yields a function type whose own parameter is that very type (a self
// referencing type), which used to recurse until the compiler crashed.
func <T> genref_wrap(x : T) : T { return x }

func genref_inc(x : int) : int { return x + 1 }

func genref_wrap_fn(f : (x : int) => int) : (x : int) => int { return f }

@retained
func <T> genref_wrap_ref() : (x : T) => T { return genref_wrap<T> }

// NOTE : a *nested* generic function reference inside a generic body
// (`genref_wrap<genref_wrap<T>>`) is deliberately not tested here. The generic body is
// checked before instantiation, where `link_generic_func_reference` defers by taking the
// declaration's master signature, so the outer reference is typed as the flat
// `(x : T) => T` and the nested type doesn't type check yet.

func test_native_generic_nested_fn_type() {
    test("nested generic function reference type is instantiated", () => {
        var z : (f : (x : int) => int) => (x : int) => int = genref_wrap<genref_wrap<int>>
        var g = z(genref_inc)
        return g(50) == 51
    })
    test("nested generic function reference is callable", () => {
        var g = genref_wrap<genref_wrap<int>>(genref_inc)
        return g(51) == 52
    })
    test("function type returning a function type as a local", () => {
        var f : (p : (x : int) => int) => (x : int) => int = genref_wrap<genref_wrap<int>>
        var g = f(genref_inc)
        return g(53) == 54
    })
    test("triple nested generic function reference is instantiated", () => {
        // three levels of function return types : the declarator nests once per level
        var z = genref_wrap<genref_wrap<genref_wrap<int>>>
        var g = z(genref_wrap_fn)
        var h = g(genref_inc)
        return h(54) == 55
    })
    test("is against a function type is deterministically false", () => {
        var z = genref_inc
        return !(z is (x : int) => int)
    })
}

// ---------------------------------------------------------------------------
// B20: field access on a *concretely instantiated* generic type inside a generic
// body must use the substituted member type, not the master declaration's
// generic parameter. Previously `b.value` (b : B20Box<int>) stayed typed `T`,
// so `b.value == 42` failed; and a function-typed member of `B20Table<int>`
// stayed `(x : T) => int`. Partially applied types (still mentioning a generic
// parameter) must stay deferred.
// ---------------------------------------------------------------------------

struct B20Box<T> {
    var value : T
}

struct B20Table<T> {
    var poll : (x : T) => int
}

struct B20Wrapper<A, B> {
    var a : A
    var b : B
}

struct B20FutureTable<T> {
    var cb : (x : T) => int
}

func b20_unit(x : int) : int { return 0 }

func <T> b20_read_box(b : B20Box<int>) : int {
    if(b.value == 42) { return 0 } else { return 1 }
}

func <T> b20_make_table() : int {
    var t = malloc(sizeof(B20Table<int>)) as *mut B20Table<int>
    t.poll = b20_unit
    return t.poll(1)
}

func b20_wrapper_cb(w : B20Wrapper<int, int>) : int { return w.a + w.b }

func <T> b20_composite(ft : B20FutureTable<B20Wrapper<int, int>>) : int {
    var f = ft.cb
    return f(B20Wrapper<int, int> { a : 3, b : 4 })
}

func test_native_generic_composite_field() {
    test("field access on a concrete generic instantiation inside a generic body (B20)", () => {
        var b = B20Box<int> { value : 42 }
        return b20_read_box<int>(b) == 0
    })
    test("function-typed field of a concrete generic instantiation inside a generic body (B20)", () => {
        return b20_make_table<int>() == 0
    })
    test("composite generic argument field access inside a generic body (B20)", () => {
        var ft = B20FutureTable<B20Wrapper<int, int>> { cb : b20_wrapper_cb }
        return b20_composite<int>(ft) == 7
    })
}

