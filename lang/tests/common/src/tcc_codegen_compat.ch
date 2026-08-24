// Copyright (c) Chemical Language Foundation 2026.
//
// Tests for C codegen patterns that may fail with TinyCC.
// TinyCC has limitations with certain C99/C11 features that our
// C codegen may produce. These tests exercise those patterns.

struct TCCBox {
    var value : int

    @make
    func make(val : int) : TCCBox {
        return TCCBox { value : val }
    }

    @delete
    func delete(&mut self) {
    }
}

struct TCCWrapper {
    var box : TCCBox
    var label : int

    @make
    func make(v : int, l : int) : TCCWrapper {
        return TCCWrapper { box : TCCBox.make(v), label : l }
    }

    @delete
    func delete(&mut self) {
    }
}

func test_tcc_codegen_compat() {

    // Test 1: zeroed on array of destructible structs
    // This generates ((struct TCCBox[3]){0}) — a compound literal
    // that TinyCC doesn't support
    test("zeroed array of destructible structs works", () => {
        var boxes : [3]TCCBox = zeroed:unsafe<[3]TCCBox>()
        boxes[0] = TCCBox.make(10)
        boxes[1] = TCCBox.make(20)
        boxes[2] = TCCBox.make(30)
        return boxes[0].value == 10 && boxes[1].value == 20 && boxes[2].value == 30
    })

    // Test 2: zeroed on array of non-destructible structs
    // Same compound literal pattern but without destructor
    test("zeroed array of non-destructible structs works", () => {
        var values : [3]int = zeroed<[3]int>()
        values[0] = 10
        values[1] = 20
        values[2] = 30
        return values[0] == 10 && values[1] == 20 && values[2] == 30
    })

    // Test 3: Nested zeroed — wrapper containing destructible field
    test("zeroed nested struct with destructible field works", () => {
        var w : TCCWrapper = zeroed:unsafe<TCCWrapper>()
        w.box = TCCBox.make(42)
        w.label = 1
        return w.box.value == 42 && w.label == 1
    })

    // Test 4: Empty destructor bodies
    // The C codegen emits __chx__dstctr_clnup_blk__:{ } which TinyCC may reject
    test("empty destructor body compiles and runs", () => {
        var b = TCCBox.make(99)
        return b.value == 99
    })

    // Test 5: Multiple destructible structs in scope (triggers multiple destructor calls)
    test("multiple destructible structs in scope work", () => {
        var b1 = TCCBox.make(1)
        var b2 = TCCBox.make(2)
        var b3 = TCCBox.make(3)
        return b1.value + b2.value + b3.value == 6
    })

    // Test 6: Destructible struct passed to function
    test("destructible struct passed to function works", () => {
        var b = TCCBox.make(42)
        return accept_box(b) == 42
    })

    // Test 7: Destructible struct returned from function
    test("destructible struct returned from function works", () => {
        var b = create_box(77)
        return b.value == 77
    })
}

func accept_box(b : TCCBox) : int {
    return b.value
}

func create_box(val : int) : TCCBox {
    return TCCBox.make(val)
}
