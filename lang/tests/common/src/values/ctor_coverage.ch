// Copyright (c) Chemical Language Foundation 2026.
//
// A `T(args)` result temporary is guaranteed to be fully defined because the
// lowerer emits a `zero_init` for it, *unless* it can prove the construction
// writes the whole value. That proof (MIRLowerer::constructor_writes_whole_value
// / default_init_writes_all_fields, action C1 of
// lang/docs/mir-migration-evaluation.md) is allowed to skip the zero when:
//
//   * the constructor's body is `{ return T { ... } }` — the returned literal is
//     lowered to its own temporary and copied into the result with a whole-value
//     assignment, so every byte is written; or
//   * the constructor is compiler-generated (or recursive default
//     initialization) and provably covers every field.
//
// These tests pin the *values* those constructions produce. If the proof is
// loosened incorrectly the result reads uninitialized bytes here; if it is
// tightened, the tests still pass (they assert values, not code shape).
// The conservative cases (a constructor whose body is not a single return
// literal, a member with a real default) are covered too.

// ---------------------------------------------------------------- return T { }

@direct_init
struct CtorPair {
    var a : int
    var b : int

    @constructor
    func make(a_ : int, b_ : int) {
        return CtorPair { a : a_, b : b_ }
    }
}

func ctor_return_literal_fields_hold() : bool {
    var p = CtorPair(3, 4)
    if (p.a != 3) return false
    if (p.b != 4) return false
    // and the literal form agrees with the constructor form
    var q = CtorPair { a : 5, b : 6 }
    return q.a == 5 && q.b == 6
}

// ------------------------------------------- nested member default-constructed

struct CtorInner {
    var x : int
    var y : int

    @constructor
    func make() {
        return CtorInner { x : 11, y : 22 }
    }
}

@direct_init
struct CtorOuter {
    var inner : CtorInner
    var tag : int = 5
}

func ctor_nested_member_default_construction() : bool {
    // `inner` is omitted: it takes the member's own default constructor, whose
    // temporary is a `return T { ... }` construction
    var o = CtorOuter { tag : 9 }
    if (o.inner.x != 11) return false
    if (o.inner.y != 22) return false
    return o.tag == 9
}

// ------------------------------------------------- inherited base default-init

struct CtorBase {
    var base_a : int
    var base_b : int

    @constructor
    func make() {
        return CtorBase { base_a : 31, base_b : 32 }
    }
}

@direct_init
struct CtorDerived : CtorBase {
    var derived : int = 7
}

func ctor_inherited_base_default_construction() : bool {
    // the base field is not set in the literal, so it is default-constructed
    var d = CtorDerived { derived : 3 }
    if (d.base_a != 31) return false
    if (d.base_b != 32) return false
    return d.derived == 3
}

// -------------------------------------------- generated constructor / defaults

struct CtorAllDefaults {
    var a : int = 40
    var b : long = 2
}

func ctor_generated_default_constructor() : bool {
    var v = CtorAllDefaults()
    return v.a == 40 && v.b == 2
}

// ------------------------------------------ conservative: multi-statement body

@direct_init
struct CtorMultiStmt {
    var a : int
    var b : int

    @constructor
    func make(v : int) {
        // not provable (a local is built first, then returned), so the result
        // temporary keeps its zero -- the values must still be correct
        var m = CtorMultiStmt { a : v, b : v + 1 }
        return m
    }
}

func ctor_multi_statement_body_fields_hold() : bool {
    var m = CtorMultiStmt(4)
    return m.a == 4 && m.b == 5
}

func test_constructor_field_coverage() {
    test("constructor returning a struct literal sets every field", () => {
        return ctor_return_literal_fields_hold();
    })
    test("nested member default constructor sets every field", () => {
        return ctor_nested_member_default_construction();
    })
    test("inherited base default constructor sets every field", () => {
        return ctor_inherited_base_default_construction();
    })
    test("generated default constructor applies every field default", () => {
        return ctor_generated_default_constructor();
    })
    test("multi statement constructor body still sets every field", () => {
        return ctor_multi_statement_body_fields_hold();
    })
}
