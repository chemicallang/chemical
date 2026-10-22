// Generic implementations (`impl<T> Trait for Type<T>`) — core language tests.
//
// These run on the compiled (TCC/LLVM) and interpreted paths. They cover the
// lazy/dormant instantiation shape: an implementation is only instantiated when
// a member/operator is actually demanded for a concrete target.
//
// Naming is prefixed (`GImpl`) to avoid collisions with the other common test
// files, which share one symbol table.

// ---------------------------------------------------------------------------
// Basic method dispatch
// ---------------------------------------------------------------------------

interface GImplShow {
    func gimpl_show(&self) : int
}

struct GImplBox<T> {
    var value : T
}

impl<T> GImplShow for GImplBox<T> {
    func gimpl_show(&self) : int {
        return 11
    }
}

struct GImplPair<A, B> {
    var a : A
    var b : B
}

impl<A, B> GImplShow for GImplPair<A, B> {
    func gimpl_show(&self) : int {
        return 22
    }
}

// nested target pattern: the target argument is itself generic
struct GImplWrap<T> {
    var inner : T
}

impl<T> GImplShow for GImplWrap<GImplBox<T>> {
    func gimpl_show(&self) : int {
        return 33
    }
}

// concrete pattern argument
struct GImplTag {}

interface GImplTagged {
    func gimpl_tagged(&self) : int
}

impl<V> GImplTagged for GImplPair<GImplTag, V> {
    func gimpl_tagged(&self) : int {
        return 44
    }
}

// ---------------------------------------------------------------------------
// Bounds + static dispatch through a generic function
// ---------------------------------------------------------------------------

interface GImplDescribe {
    func gimpl_describe(&self) : int
}

impl GImplDescribe for int {
    func gimpl_describe(&self) : int {
        return *self
    }
}

impl<T : GImplDescribe> GImplDescribe for GImplBox<T> {
    func gimpl_describe(&self) : int {
        return 100
    }
}

func <T : GImplShow> gimpl_call_show(v : &T) : int {
    return v.gimpl_show()
}

// Unused generic impl with a bound the concrete argument does NOT satisfy. It
// must never be instantiated unless demanded (I3: cost only on use).
interface GImplExtra {
    func gimpl_extra(&self) : int
}

struct GImplNoDescribe {}

impl<T : GImplDescribe> GImplExtra for GImplBox<T> {
    func gimpl_extra(&self) : int {
        return 1
    }
}

// ---------------------------------------------------------------------------
// Default interface method supplied by adoption
// ---------------------------------------------------------------------------

interface GImplDefault {
    func gimpl_default(&self) : int

    func gimpl_default_extra(&self) : int {
        return 55
    }
}

impl<T> GImplDefault for GImplBox<T> {
    func gimpl_default(&self) : int {
        return 1
    }
}

// ---------------------------------------------------------------------------
// Operators (demand through ImplementationsIndex, not a method name)
// ---------------------------------------------------------------------------

struct GImplNum<T> {
    var value : T
}

impl<T> core::ops::PartialEq<GImplNum<T>> for GImplNum<T> {
    func eq(&self, other : GImplNum<T>) : bool {
        return true
    }
    func ne(&self, other : GImplNum<T>) : bool {
        return false
    }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

func test_generic_impls() {

    test("generic impl: method dispatch for one instantiation", () => {
        var b = GImplBox<int> { value : 1 }
        return b.gimpl_show() == 11
    })

    test("generic impl: same impl for another instantiation", () => {
        var b = GImplBox<long> { value : 2 }
        return b.gimpl_show() == 11
    })

    test("generic impl: multiple type parameters", () => {
        var p = GImplPair<int, int> { a : 1, b : 2 }
        return p.gimpl_show() == 22
    })

    test("generic impl: nested target pattern", () => {
        var w = GImplWrap<GImplBox<int>> { inner : GImplBox<int> { value : 3 } }
        return w.gimpl_show() == 33
    })

    test("generic impl: concrete pattern argument", () => {
        var p = GImplPair<GImplTag, int> { a : GImplTag {}, b : 4 }
        return p.gimpl_tagged() == 44
    })

    test("generic impl: satisfied bound at use", () => {
        var b = GImplBox<int> { value : 5 }
        return b.gimpl_describe() == 100
    })

    test("generic impl: dispatch through a bounded generic function", () => {
        var b = GImplBox<int> { value : 6 }
        return gimpl_call_show<GImplBox<int>>(&b) == 11
    })

    test("generic impl: unused impl never fires its unsatisfied bound", () => {
        // GImplExtra demands T : GImplDescribe; GImplNoDescribe has no impl.
        // Merely instantiating GImplBox<GImplNoDescribe> must stay legal.
        var b = GImplBox<GImplNoDescribe> { value : GImplNoDescribe {} }
        // only GImplShow is demanded here; GImplExtra stays dormant
        return b.gimpl_show() == 11
    })

    test("generic impl: interface default method is adopted", () => {
        var b = GImplBox<int> { value : 7 }
        return b.gimpl_default() == 1 && b.gimpl_default_extra() == 55
    })

    test("generic impl: operator dispatch", () => {
        var a = GImplNum<int> { value : 1 }
        var b = GImplNum<int> { value : 2 }
        return (a == b) && !(a != b)
    })

    test("generic impl: two instantiations coexist", () => {
        var a = GImplBox<int> { value : 1 }
        var b = GImplBox<long> { value : 2 }
        return a.gimpl_show() == 11 && b.gimpl_show() == 11
    })
}
