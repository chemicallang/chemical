// ============================================================
// Aggregate field layout: every field access must follow MIR's
// recorded byte offsets, including the implicit padding introduced
// before a field whose type is more aligned than the running offset.
//
// Regression: the LLVM backend's `gep_path` evaluated the field's
// LLVM element index (`mir_field_elem(...)`) in the same call
// expression that publishes the field -> element plan
// (`mir_type(...)`). C++ leaves argument evaluation order
// unspecified, so on the *first* access to an aggregate the plan was
// not built yet and the raw MIR field index was used as the LLVM
// element index. Any aggregate whose LLVM element sequence differs
// from its MIR field list — a struct with padding, a union, or a
// variant — therefore addressed the wrong field on that first access.
//
// `AL_Outer` below has an element sequence `{ char, [3 x i8],
// AL_Inner, int }`: MIR field 2 (`y`) is LLVM element 3. A `@delete`
// is emitted before the functions that build instances and its
// `self` is a pointer (so no `alloca` of the aggregate forces
// `mir_type` first), which makes its `self.y` the very first access
// and reproduces the bug deterministically on every backend.
// ============================================================

struct AL_Inner {
    var a : int
    var b : int
}

var al_delete_seen : int = 0

// `x` is one byte; `inner` needs four-byte alignment, so MIR places it
// at offset 4 (three bytes of padding) and `y` at offset 12.
struct AL_Outer {
    var x : char
    var inner : AL_Inner
    var y : int

    @delete
    func delete(&self) {
        al_delete_seen = self.y
    }
}

// Reads field 2 (`y`) through a pointer: no aggregate `alloca`, so the
// first `gep_path` for `AL_Outer` is this field access.
func al_read_y(o : &AL_Outer) : int {
    return o.y
}

func test_aggregate_field_layout() {
    test("sizeof and offsetof include implicit padding", () => {
        return sizeof(AL_Inner) == 8 && sizeof(AL_Outer) == 16 &&
               offsetof(AL_Outer, x) == 0 && offsetof(AL_Outer, y) == 12
    })
    test("field after padding round-trips through a reference", () => {
        var o : AL_Outer
        o.y = 4242
        o.x = 'A'
        o.inner.a = 7
        o.inner.b = 9
        return al_read_y(&o) == 4242 && o.x == 'A' &&
               o.inner.a == 7 && o.inner.b == 9
    })
    test("destructor reads the field after padding", () => {
        al_delete_seen = 0
        if(al_delete_seen == 0) {
            var o : AL_Outer
            o.y = 4242
            o.x = 'A'
            o.inner.a = 7
            o.inner.b = 9
        }
        return al_delete_seen == 4242
    })
}
