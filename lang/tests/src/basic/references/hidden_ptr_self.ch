// Tests for hidden-pointer self reference codegen
// The bug: &mut self / &self as an expression produced double-pointer (&self)
// instead of just self in C when self is already a hidden pointer param.

struct HiddenPtrSelf {
    var data: int
    func set_data(&mut self, v: int) {
        data = v
    }
    func get_data(&self) : int {
        return data
    }
    func mutate_via_ptr(&mut self) {
        var ptr = &mut self as *void
        var s = ptr as &mut HiddenPtrSelf
        s.data = 99
    }
    func read_via_ptr(&self) : int {
        var ptr = &self as *void
        var s = ptr as &HiddenPtrSelf
        return s.data
    }
}

func test_hidden_ptr_self_in_methods() {
    test("call method on &mut self works", () => {
        var s = HiddenPtrSelf { data: 0 }
        s.set_data(77)
        return s.data == 77
    })
    test("call method on &self works", () => {
        var s = HiddenPtrSelf { data: 42 }
        return s.get_data() == 42
    })
    test("cast &mut self to *void then back and mutate works", () => {
        var s = HiddenPtrSelf { data: 0 }
        s.mutate_via_ptr()
        return s.data == 99
    })
    test("cast &self to *void then back and read works", () => {
        var s = HiddenPtrSelf { data: 55 }
        return s.read_via_ptr() == 55
    })
}

// Tests that &*r (take ref of deref) correctly emits & in C
// even when the deref involves a hidden-pointer parameter node.
// This covers the DereferenceValue special case in VisitReferenceOfValue.

struct RefDerefStruct {
    var val: int
}

func read_rd(s: &RefDerefStruct) : int {
    return s.val
}

func pass_ref_of_deref(s: &RefDerefStruct) : int {
    return read_rd(&*s)
}

func test_ref_of_deref_hidden_ptr() {
    test("&*s where s is &T passes correct value", () => {
        var x = RefDerefStruct { val: 77 }
        return pass_ref_of_deref(&x) == 77
    })
}
