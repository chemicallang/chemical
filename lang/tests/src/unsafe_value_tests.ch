// Verification of `unsafe(...)` value semantics and the new
// uninitialized-variable rules. `unsafe(expr)` is a compile-time-only safety
// marker: it must not change runtime behaviour (including destruction).
//
// Note: the test-framework lambdas `() => { ... }` cannot capture locals from
// the enclosing function, so any value that must be observed inside an
// assertion lambda is copied into a module-level global first.

internal var g_unsafe_dtor_count : i32 = 0
internal var g_unsafe_last_tag : i32 = 0
internal var g_unsafe_last_ptr : *mut UnsafeTracked = null

struct UnsafeTracked {
    var tag : i32
    @delete func delete(&mut self) {
        g_unsafe_dtor_count = g_unsafe_dtor_count + 1
    }
}

func make_tracked() : UnsafeTracked {
    return UnsafeTracked { tag : 7 }
}

// returning a local wrapped in `unsafe(...)` is a MOVE: the moved-from local
// must not be destroyed, while the caller's value is destroyed exactly once.
func return_unsafe_tracked() : UnsafeTracked {
    var t = make_tracked()
    return unsafe(t)
}

func test_unsafe_value_semantics() {
    // 1) A value obtained through `unsafe(...)` is still destroyed normally.
    //    `unsafe` is a no-op marker and must never suppress destruction.
    g_unsafe_dtor_count = 0
    {
        var t = unsafe(make_tracked())
        g_unsafe_last_tag = t.tag
        test("unsafe wrap does not suppress destruction of a normal local", () => {
            return g_unsafe_last_tag == 7
        })
    }
    test("unsafe wrap keeps the single destructor of a normal local", () => {
        return g_unsafe_dtor_count == 1
    })

    // 2) `unsafe(...)` used on a return is a move, not a copy.
    g_unsafe_dtor_count = 0
    {
        var t = return_unsafe_tracked()
        g_unsafe_last_tag = t.tag
        test("unsafe return preserves the moved value", () => {
            return g_unsafe_last_tag == 7
        })
    }
    test("unsafe return behaves as a move (exactly one destructor)", () => {
        return g_unsafe_dtor_count == 1
    })

    // 3) New uninitialized-variable rules: declare without `unsafe`, initialize
    //    via a full assignment (first init), and take the address only inside
    //    `unsafe(...)`. The value is still destroyed exactly once.
    g_unsafe_dtor_count = 0
    {
        var t : UnsafeTracked
        t = make_tracked()
        var p = unsafe(&raw mut t)
        g_unsafe_last_ptr = p
        test("address of uninitialized var allowed inside unsafe(...)", () => {
            return g_unsafe_last_ptr != null
        })
    }
    test("value declared uninitialized and first-initialized destructs once", () => {
        return g_unsafe_dtor_count == 1
    })
}
