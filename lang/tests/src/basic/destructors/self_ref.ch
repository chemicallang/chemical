// ============================================================
// Tests for self-referencing assignment use-after-free bug.
//
// Compiler bug: in `x = f(x.get_ptr())`, the C codegen placed
// the LHS destructor BEFORE evaluating the RHS. When the RHS
// calls x.get_ptr(), the returned pointer pointed to data that
// was already invalidated by the destructor.
//
// These tests use only pure language features (structs, dtor,
// raw pointers) — no stdlib dependency.
// ============================================================

struct SelfRef {
    var value : int
    var count : *mut int

    func get_ptr(&self) : *mut int {
        return &raw mut self.value
    }

    @delete
    func delete(&mut self) {
        self.value = -999
        if(self.count != null) {
            *self.count = *self.count + 1
        }
    }
}

func make_self_ref(initial : int, count : *mut int) : SelfRef {
    return SelfRef { value: initial, count: count }
}

func read_val_then_add(p : *mut int, extra : int) : SelfRef {
    var r : SelfRef
    r.value = *p + extra
    return r
}

func test_self_ref_destruct() {

    test("self-ref: x = f(x.get_ptr(), N) reads correct value after assignment", () => {
        var count = 0
        var x = make_self_ref(42, &raw mut count)
        x = read_val_then_add(x.get_ptr(), 10)
        // If bug: dtor runs first → x.value = -999 → get_ptr returns &-999
        //          → read_val_then_add reads -999 → r.value = -989 → x.value = -989
        // If fix: get_ptr returns &42 → read_val_then_add reads 42 → r.value = 52
        //          → dtor runs (x.value = -999) → struct copy → x.value = 52
        return x.value == 52 && count == 1
    })

    test("self-ref: x = f(x.get_ptr(), N) with different values preserves order", () => {
        var count = 0
        var x = make_self_ref(100, &raw mut count)
        x = read_val_then_add(x.get_ptr(), 7)
        return x.value == 107 && count == 1
    })

}
