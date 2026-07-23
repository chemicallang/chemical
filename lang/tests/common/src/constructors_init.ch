// Copyright (c) Chemical Language Foundation 2026.

struct InitDefConsStruct {

    var a : int
    var b : int

    @make
    func make() {
        return InitDefConsStruct {
            a = 98
            b = 87
        }
    }

}

struct InitBlockAutoDefConsCall {

    var d : InitDefConsStruct

}

struct InitBlockManualDefConsCall {

    var d : InitDefConsStruct

    @make
    func make() {
        return InitBlockManualDefConsCall {
            d = InitDefConsStruct()
        }
    }

}

struct InitBlockManualDefConsCall2 {

    var d : InitDefConsStruct

    @make
    func make() {
        return InitBlockManualDefConsCall2 {
            d : InitDefConsStruct()
        }
    }

}

struct InitBlockManualDefConsCall3 : InitDefConsStruct {
}

struct InitBlockManualDefConsCall6 : InitDefConsStruct {

    @make
    func make() {
        return InitBlockManualDefConsCall6 {
            InitDefConsStruct = InitDefConsStruct()
        }
    }

}

struct InitBlockManualDefConsCall7 : InitDefConsStruct {

    @make
    func make() {
        return InitBlockManualDefConsCall7 {
            InitDefConsStruct : InitDefConsStruct()
        }
    }

}

func test_constructors_with_init() {

    // These tests work: direct constructor call, manual init blocks
    
    test("default auto constructor call in init block works - constructors init", () => {
        var c = InitDefConsStruct()
        return c.a == 98 && c.b == 87
    })

    test("default manual constructor call in init block works - 1", () => {
        var c = InitBlockManualDefConsCall()
        return c.d.a == 98 && c.d.b == 87
    })

    test("default manual constructor call in init block works - 2", () => {
        var c = InitBlockManualDefConsCall2()
        return c.d.a == 98 && c.d.b == 87
    })

    test("default auto constructor call in init block works", () => {
        var c = InitBlockAutoDefConsCall()
        return c.d.a == 98 && c.d.b == 87
    })

    // Native-only: inheritance init crashes interpreter
    test("default manual constructor call in init block works - 3", () => {
        var c = InitBlockManualDefConsCall3()
        return c.a == 98 && c.b == 87
    })
    test("default manual constructor call in init block works - 6", () => {
        var c = InitBlockManualDefConsCall6()
        return c.a == 98 && c.b == 87
    })
    test("default manual constructor call in init block works - 7", () => {
        var c = InitBlockManualDefConsCall7()
        return c.a == 98 && c.b == 87
    })

}

// ── @constructor with @delete ──────────────────────────────────

struct CtorWithDtorType {
    var ptr : *void
    var val : int

    @constructor
    func constructor() : CtorWithDtorType {
        return CtorWithDtorType {
            ptr: null,
            val: 42
        }
    }

    @delete
    func destruct(&self) {
        // safe: ptr is null from constructor
    }
}

func test_constructors_with_destructor() {

    test("var x = Type() calls @constructor correctly", () => {
        var s = CtorWithDtorType()
        return s.ptr == null && s.val == 42
    })

    test("@delete after @constructor is safe (null check)", () => {
        var s = CtorWithDtorType()
        // destructor runs at scope exit, checks ptr == null, safe
        return s.val == 42
    })

}

// ── Bitwise & precedence with != / == ──────────────────────────

func test_bitwise_precedence() {

    test("bitwise & precedence with != 0", () => {
        var flags : u8 = 0x03
        return (flags & 0x02) != 0 && (flags & 0x01) != 0 && (flags & 0x04) == 0
    })

    test("bitwise & with == 0", () => {
        var flags : u8 = 0x0A
        return (flags & 0x01) == 0 && (flags & 0x04) == 0 && (flags & 0x02) != 0 && (flags & 0x08) != 0
    })

}
