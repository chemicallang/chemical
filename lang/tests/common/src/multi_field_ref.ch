// Copyright (c) Chemical Language Foundation 2026.
//
// Multi-field reference tests — moved from lang/tests/src/basic/references/basic.ch
// No variant matching, no raw pointers, no external deps.

struct MultiField {
    var a : int
    var b : int
    var c : int
}

func read_multi(r : &MultiField) : int {
    return r.a + r.b + r.c
}

func pass_multi(r : &MultiField) : int {
    return read_multi(r)
}

func test_multi_field_ref() {
    test("multi-field struct passed as ref between functions retains all fields", () => {
        var s = MultiField { a : 10, b : 20, c : 30 }
        return pass_multi(&s) == 60
    })
    test("multi-field struct ref read directly retains all fields", () => {
        var s = MultiField { a : 100, b : 200, c : 300 }
        return read_multi(&s) == 600
    })
}
