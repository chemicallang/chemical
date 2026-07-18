// Copyright (c) Chemical Language Foundation 2026.
//
// Tests for parallel generic instantiation scenarios.
// These exercise the concurrent instantiation system described in
// lang/docs/parallel-generic-instantiation.md:
//   - Circular dependencies (A ↔ B, A → B → C → A)
//   - Generic structs containing other generic structs
//   - Methods referencing cross-generic types
//   - Deep nesting (Wrapper<Wrapper<T>>)
//   - Multiple independent generics instantiated in parallel

// ============================================================
// Scenario 1: Circular generic dependency (A ↔ B)
// This is the classic deadlock scenario from the document.
// When instantiating GenericNodeA<int>, signature finalization
// encounters GenericNodeB<int>, which in turn encounters
// GenericNodeA<int> back. With recursive_mutex this resolves.
// ============================================================

struct GenericNodeA<T> {
    var value : T
    func get_peer(&self, other : GenericNodeB<T>) : T {
        return other.value
    }
    func get_value(&self) : T {
        return value
    }
}

struct GenericNodeB<T> {
    var value : T
    func get_peer(&self, other : GenericNodeA<T>) : T {
        return other.value
    }
    func get_value(&self) : T {
        return value
    }
}

// ============================================================
// Scenario 2: Three-way circular dependency (A → B → C → A)
// Deeper recursion: A's signature encounters B, B encounters C,
// C encounters A (already registered → returns pointer).
// ============================================================

struct GenericChainA<T> {
    var value : T
    func chain(&self, b : GenericChainB<T>) : T {
        return b.value
    }
}

struct GenericChainB<T> {
    var value : T
    func chain(&self, c : GenericChainC<T>) : T {
        return c.value
    }
}

struct GenericChainC<T> {
    var value : T
    func chain(&self, a : GenericChainA<T>) : T {
        return a.value
    }
}

// ============================================================
// Scenario 3: Generic struct containing another generic struct
// When GenericWrapper<int> is instantiated, it needs
// GenericBox<int> to exist. The box's methods should work
// through the wrapper.
// ============================================================

struct GenericBox<T> {
    var item : T
    func get(&self) : T {
        return item
    }
}

struct GenericWrapper<T> {
    var box : GenericBox<T>
    var extra : T
    func unwrap(&self) : T {
        return box.item
    }
    func get_box_value(&self) : T {
        return box.get()
    }
}

// ============================================================
// Scenario 4: Generic struct with generic method that takes
// another generic type as parameter. Tests that signature
// finalization correctly links cross-generic references.
// ============================================================

struct GenericSender<T> {
    var data : T
    func send_to(&self, receiver : GenericReceiver<T>) : T {
        return receiver.buffer
    }
}

struct GenericReceiver<T> {
    var buffer : T
    func receive(&self) : T {
        return buffer
    }
}

// ============================================================
// Scenario 5: Deep nesting — Wrapper<Wrapper<T>>
// Instantiating GenericDeepWrapper<int> requires
// GenericDeepWrapper<GenericDeepWrapper<int>> which is a
// recursive instantiation at the type level.
// ============================================================

struct GenericDeepWrapper<T> {
    var inner : T
    func peek(&self) : T {
        return inner
    }
}

// ============================================================
// Scenario 6: Generic variant containing a generic struct.
// The variant GenericResult<T> has Ok(GenericBox<T>).
// Tests that variant + struct generic instantiation works
// together under parallel compilation.
// ============================================================

variant GenericResult<T> {
    Ok(value : GenericBox<T>)
    Err(code : int)
}

// ============================================================
// Scenario 7: Multiple different generic types used in the same
// function signature. Forces multiple instantiations to resolve
// concurrently during signature finalization.
// ============================================================

struct GenericPair<A, B> {
    var first : A
    var second : B
    func get_first(&self) : A {
        return first
    }
    func get_second(&self) : B {
        return second
    }
}

struct GenericTriple<A, B, C> {
    var a : A
    var b : B
    var c : C
    func get_a(&self) : A {
        return a
    }
    func get_b(&self) : B {
        return b
    }
    func get_c(&self) : C {
        return c
    }
}

// ============================================================
// Scenario 8: Generic function that creates and returns
// multiple different generic types simultaneously.
// Tests that body finalization handles multiple generics.
// ============================================================

func <T> make_generic_pair(a : T, b : T) : GenericPair<T, T> {
    return GenericPair<T, T> { first : a, second : b }
}

func <T> make_generic_box(val : T) : GenericBox<T> {
    return GenericBox<T> { item : val }
}

func <T> chain_three(a_val : T, b_val : T, c_val : T) : int {
    var a = GenericChainA<T> { value : a_val }
    var b = GenericChainB<T> { value : b_val }
    var c = GenericChainC<T> { value : c_val }
    var ab = a.chain(b)
    var bc = b.chain(c)
    var ca = c.chain(a)
    return (ab as int) + (bc as int) + (ca as int)
}

// ============================================================
// Scenario 9: Instantiating many different concrete types of
// the same generic to exercise parallel instantiation paths.
// Each different type (int, long, short, char) is a separate
// instantiation that may run on different threads.
// ============================================================

struct GenericSizeBox<T> {
    var value : T
}

// ============================================================
// Scenario 10: A generic struct with an inner generic from
// a different scenario (cross-dependency between scenarios).
// GenericBridge<int> uses GenericBox<int> from scenario 3
// and GenericNodeA<int> from scenario 1.
// ============================================================

struct GenericBridge<T> {
    var box : GenericBox<T>
    var node : GenericNodeA<T>
    func get_from_box(&self) : T {
        return box.get()
    }
    func get_from_node(&self) : T {
        return node.get_value()
    }
}

func test_generic_instantiation() {

    // ---- Scenario 1: Circular A ↔ B ----
    test("circular generic dependency A ↔ B resolves - int", () => {
        var a = GenericNodeA<int> { value : 10 }
        var b = GenericNodeB<int> { value : 20 }
        return a.get_value() == 10 && b.get_value() == 20
    })
    test("circular generic dependency A ↔ B resolves - long", () => {
        var a = GenericNodeA<long> { value : 100 }
        var b = GenericNodeB<long> { value : 200 }
        return a.get_value() == 100 && b.get_value() == 200
    })
    test("circular generic dependency A ↔ B peer method works", () => {
        var a = GenericNodeA<int> { value : 10 }
        var b = GenericNodeB<int> { value : 20 }
        return a.get_peer(b) == 20 && b.get_peer(a) == 10
    })

    // ---- Scenario 2: Three-way cycle A → B → C → A ----
    test("three-way circular dependency A → B → C → A resolves - int", () => {
        var a = GenericChainA<int> { value : 1 }
        var b = GenericChainB<int> { value : 2 }
        var c = GenericChainC<int> { value : 3 }
        return a.chain(b) == 2 && b.chain(c) == 3 && c.chain(a) == 1
    })
    test("three-way circular dependency A → B → C → A resolves - long", () => {
        var a = GenericChainA<long> { value : 10 }
        var b = GenericChainB<long> { value : 20 }
        var c = GenericChainC<long> { value : 30 }
        return a.chain(b) == 20 && b.chain(c) == 30 && c.chain(a) == 10
    })
    test("three-way circular chain function works", () => {
        return chain_three(1, 2, 3) == 6
    })

    // ---- Scenario 3: Generic wrapping another generic ----
    test("generic struct containing another generic struct works - int", () => {
        var box = GenericBox<int> { item : 42 }
        var wrapper = GenericWrapper<int> { box : box, extra : 100 }
        return wrapper.unwrap() == 42 && wrapper.get_box_value() == 42
    })
    test("generic struct containing another generic struct works - long", () => {
        var box = GenericBox<long> { item : 99999 }
        var wrapper = GenericWrapper<long> { box : box, extra : 200000 }
        return wrapper.unwrap() == 99999 && wrapper.get_box_value() == 99999
    })
    test("generic struct containing another generic struct works - short", () => {
        var box = GenericBox<short> { item : 7 }
        var wrapper = GenericWrapper<short> { box : box, extra : 8 }
        return wrapper.unwrap() == 7 && wrapper.get_box_value() == 7
    })

    // ---- Scenario 4: Sender → Receiver cross-generic ----
    test("generic sender/receiver cross-reference works - int", () => {
        var recv = GenericReceiver<int> { buffer : 55 }
        var send = GenericSender<int> { data : 42 }
        return send.send_to(recv) == 55
    })
    test("generic sender/receiver cross-reference works - long", () => {
        var recv = GenericReceiver<long> { buffer : 777 }
        var send = GenericSender<long> { data : 888 }
        return send.send_to(recv) == 777
    })

    // ---- Scenario 5: Deep nesting Wrapper<Wrapper<T>> ----
    test("deep generic nesting Wrapper<Wrapper<T>> works - int", () => {
        var inner = GenericDeepWrapper<int> { inner : 42 }
        var outer = GenericDeepWrapper<GenericDeepWrapper<int>> { inner : inner }
        return outer.peek().peek() == 42
    })
    test("deep generic nesting Wrapper<Wrapper<T>> works - long", () => {
        var inner = GenericDeepWrapper<long> { inner : 12345 }
        var outer = GenericDeepWrapper<GenericDeepWrapper<long>> { inner : inner }
        return outer.peek().peek() == 12345
    })

    // ---- Scenario 6: Generic variant with generic struct ----
    test("generic variant containing generic struct Ok works", () => {
        var result = GenericResult.Ok<int>(GenericBox<int> { item : 42 })
        if(result is GenericResult.Ok) {
            var Ok(val) = result else unreachable
            return val.item == 42
        }
        return false
    })
    test("generic variant containing generic struct Err works", () => {
        var result = GenericResult.Err<int>(404)
        if(result is GenericResult.Err) {
            var Err(code) = result else unreachable
            return code == 404
        }
        return false
    })

    // ---- Scenario 7: Multiple generics in same type ----
    test("generic pair with two different types works - int,long", () => {
        var p = GenericPair<int, long> { first : 10, second : 20 }
        return p.get_first() == 10 && p.get_second() == 20
    })
    test("generic pair with two different types works - short,char", () => {
        var p = GenericPair<short, char> { first : 3, second : 'a' }
        return p.get_first() == 3 && p.get_second() == 'a'
    })
    test("generic triple with three types works", () => {
        var t = GenericTriple<int, long, short> { a : 1, b : 2, c : 3 }
        return t.get_a() == 1 && t.get_b() == 2 && t.get_c() == 3
    })

    // ---- Scenario 8: Functions creating multiple generic types ----
    test("generic function creating pair works", () => {
        var p = make_generic_pair(10, 20)
        return p.get_first() == 10 && p.get_second() == 20
    })
    test("generic function creating box works", () => {
        var b = make_generic_box(42)
        return b.item == 42
    })

    // ---- Scenario 9: Many different concrete instantiations ----
    test("generic size box works for int", () => {
        var b = GenericSizeBox<int> { value : 42 }
        return sizeof(int) == 4
    })
    test("generic size box works for long", () => {
        var b = GenericSizeBox<long> { value : 42 }
        return sizeof(long) == 8
    })
    test("generic size box works for short", () => {
        var b = GenericSizeBox<short> { value : 42 }
        return sizeof(short) == 2
    })
    test("generic size box works for char", () => {
        var b = GenericSizeBox<char> { value : 'x' }
        return sizeof(char) == 1
    })
    test("generic size box works for double", () => {
        var b = GenericSizeBox<double> { value : 3.14 }
        return sizeof(double) == 8
    })

    // ---- Scenario 10: Cross-scenario generic dependencies ----
    test("cross-scenario generic bridge uses Box and Node - int", () => {
        var box = GenericBox<int> { item : 42 }
        var node = GenericNodeA<int> { value : 99 }
        var bridge = GenericBridge<int> { box : box, node : node }
        return bridge.get_from_box() == 42 && bridge.get_from_node() == 99
    })
    test("cross-scenario generic bridge uses Box and Node - long", () => {
        var box = GenericBox<long> { item : 500 }
        var node = GenericNodeA<long> { value : 600 }
        var bridge = GenericBridge<long> { box : box, node : node }
        return bridge.get_from_box() == 500 && bridge.get_from_node() == 600
    })
}
