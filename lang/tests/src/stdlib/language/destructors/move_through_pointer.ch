using std::string;
using std::string_view;

// ============================================================
// Regression tests: a move whose source is a *field* (an address, not a whole
// place) and whose destination is only reachable through a raw pointer, or is a
// mutable global, used to leave the source's drop flag set. The source's
// destructor then freed the bytes the destination kept pointing at: the value
// read back was garbage shared with whatever reused the chunk, and the process
// aborted at scope exit with "free(): double free detected in tcache 2".
//
// The lowering marks the moved field when it lowers the assignment
// (AccessChain field-assignment path, and the `store_global` path). These tests
// read the destination back *after* allocating an equally sized string, which
// is what makes a freed chunk show up as wrong bytes instead of accidentally
// still-correct stale memory.

struct MvPair {
    var text : string
}

struct MvNested {
    var inner : MvPair
}

// A global of a destructor-bearing type must opt out of its drop glue; the
// point of these tests is the source's drop flag, not the global's lifetime.
@never_destructed
var MOVE_TEST_GLOBAL : string

const MV_SIZE = 600u

func mv_long_text() : string {
    var unit = string_view("0123456789")
    var s = string()
    var i = 0
    while(i < 60) {
        s.append_view(&unit)
        i += 1
    }
    return s
}

// An equally sized allocation: if the moved-from source was freed anyway, this
// reuses its chunk and overwrites the bytes the destination still points at.
func mv_churn() : string {
    var unit = string_view("ZZZZZZZZZZ")
    var s = string()
    var i = 0
    while(i < 60) {
        s.append_view(&unit)
        i += 1
    }
    return s
}

func mv_text_is_intact(view : string_view) : bool {
    if(view.size() != MV_SIZE) { return false }
    var text = unsafe(view.data())
    return unsafe(text[0] == '0' && text[9] == '9' && text[599] == '9')
}

func mv_ptr_field_from_local_field() : bool {
    var dst = MvPair { text : string() }
    var p = &raw mut dst
    {
        var local = MvPair { text : mv_long_text() }
        unsafe { p.text = local.text }
    }
    var noise = mv_churn()
    return noise.size() == MV_SIZE && mv_text_is_intact(dst.text.to_view())
}

func mv_ptr_field_from_param_field(taken : MvPair) : bool {
    var dst = MvPair { text : string() }
    var p = &raw mut dst
    unsafe { p.text = taken.text }
    var noise = mv_churn()
    return noise.size() == MV_SIZE && mv_text_is_intact(dst.text.to_view())
}

func mv_nested_ptr_field_from_local() : bool {
    var dst = MvNested { inner : MvPair { text : string() } }
    var p = &raw mut dst
    {
        var local = mv_long_text()
        unsafe { p.inner.text = local }
    }
    var noise = mv_churn()
    return noise.size() == MV_SIZE && mv_text_is_intact(dst.inner.text.to_view())
}

func mv_global_from_local() : bool {
    {
        var local = mv_long_text()
        unsafe { MOVE_TEST_GLOBAL = local }
    }
    var noise = mv_churn()
    var view = unsafe(MOVE_TEST_GLOBAL.to_view())
    return noise.size() == MV_SIZE && mv_text_is_intact(view)
}

func mv_global_from_local_field() : bool {
    {
        var local = MvPair { text : mv_long_text() }
        unsafe { MOVE_TEST_GLOBAL = local.text }
    }
    var noise = mv_churn()
    var view = unsafe(MOVE_TEST_GLOBAL.to_view())
    return noise.size() == MV_SIZE && mv_text_is_intact(view)
}

func test_move_through_pointer() {
    test("a local field moved into a raw-pointer field keeps its bytes", () => {
        return mv_ptr_field_from_local_field()
    })

    test("a parameter field moved into a raw-pointer field keeps its bytes", () => {
        return mv_ptr_field_from_param_field(MvPair { text : mv_long_text() })
    })

    test("a local moved into a nested raw-pointer field keeps its bytes", () => {
        return mv_nested_ptr_field_from_local()
    })

    test("a local moved into a global keeps its bytes", () => {
        return mv_global_from_local()
    })

    test("a local field moved into a global keeps its bytes", () => {
        return mv_global_from_local_field()
    })

    test("moves into a raw-pointer field survive allocator churn", () => {
        var idx = 0
        while(idx < 200) {
            if(!mv_ptr_field_from_local_field()) { return false }
            if(!mv_global_from_local()) { return false }
            idx += 1
        }
        return true
    })
}
