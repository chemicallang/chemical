// ============================================================
// Tests that `delete` and `destruct` on a nullptr do NOT call
// the destructor.
//
// The compiler's codegen for DestructStmt generates a null check
// before calling the destructor. These tests verify that check.
// ============================================================

struct NullCheckedDestructible {
    var counter : *mut int

    @delete
    func delete(&mut self) {
        *self.counter = *self.counter + 1
    }
}

func test_nullptr_destruct() {

    test("delete on null pointer does NOT call destructor", () => {
        var counter = 0
        var ptr : *mut NullCheckedDestructible = null
        delete ptr
        return counter == 0
    })

    test("destruct on null pointer does NOT call destructor", () => {
        var counter = 0
        var ptr : *mut NullCheckedDestructible = null
        destruct ptr
        return counter == 0
    })

    test("delete on valid pointer DOES call destructor", () => {
        var counter = 0
        var ptr = new NullCheckedDestructible { counter : &raw mut counter }
        delete ptr
        return counter == 1
    })

}
