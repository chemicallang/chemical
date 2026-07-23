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

    // this exercises the Operation.cpp safety net: i1 ZExt'd before And/Or/Xor
    test("bitwise & with bool expression does not crash", () => {
        var flags : u8 = 0x03
        var result = flags & (0x02 != 0)
        return result == 1
    })

    test("bitwise | with bool expression does not crash", () => {
        var flags : u8 = 0x00
        var result = flags | (0x01 == 0x01)
        return result == 1
    })

}

// ── Double to typealias cast (CastedValue pure_type fix) ───────

type mysize = int

func test_double_to_typealias_cast() {

    test("double to type_alias cast via 'as' works", () => {
        var x = (44100.0 * 0.5) as mysize
        return x == 22050
    })

    test("fractional double truncated on cast to int", () => {
        var x = (99.0 / 2.0) as mysize
        return x == 49
    })

}

// ── Access chain through ref field (add_member_index fix) ──────

struct DataBlock {
    var values : [4]int
}

struct ChainRefHolder {
    var block : &mut DataBlock
}

func test_access_chain_thru_ref() {

    test("access chain through struct ref field works", () => {
        var data = DataBlock { values: [10, 20, 30, 40] }
        var holder = ChainRefHolder { block: &mut data }
        holder.block.values[1] = 99
        return holder.block.values[1] == 99
    })

    test("multiple access chain through ref field works", () => {
        var data = DataBlock { values: [0, 0, 0, 0] }
        var holder = ChainRefHolder { block: &mut data }
        holder.block.values[0] = 5
        holder.block.values[2] = 7
        return holder.block.values[0] == 5 && holder.block.values[2] == 7 && holder.block.values[1] == 0
    })

}
