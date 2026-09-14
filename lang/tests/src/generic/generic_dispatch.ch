// Regression tests for two generic/impl code generation bugs.
//
// B1: a `loop { break value }` expression whose result type is a generic
//     parameter was not specialized during generic instantiation, so the
//     generated C contained an unspecialized type and failed to compile.
//
// B2: a call to a method declared only inside an `impl Interface for T` block
//     was mangled using the interface's name instead of the concrete type's
//     (e.g. `Pingping` instead of `Ping_Counter_ping`) and failed to link.

// ---------------------------------------------------------------------------
// B1
// ---------------------------------------------------------------------------

func <T> generic_pick(flag : bool, a : T, b : T) : T {
    var out : T = loop {
        if(flag) {
            break a
        } else {
            break b
        }
    }
    return out
}

// `return loop { ... }` directly (no intermediate initializer) as well.
func <T> generic_pick_direct(flag : bool, a : T, b : T) : T {
    return loop {
        if(flag) {
            break a
        } else {
            break b
        }
    }
}

func test_generic_loop_expression_result() {
    test("generic loop expression result is specialized (true branch)", () => {
        return generic_pick<int>(true, 11, 22) == 11
    })
    test("generic loop expression result is specialized (false branch)", () => {
        return generic_pick<int>(false, 11, 22) == 22
    })
    test("generic loop expression result works for another type", () => {
        return generic_pick<u64>(true, 7u64, 9u64) == 7u64
    })
    test("generic loop expression direct return is specialized", () => {
        return generic_pick_direct<int>(false, 3, 4) == 4
    })
}

// ---------------------------------------------------------------------------
// B2
// ---------------------------------------------------------------------------

public interface ImplDispatchProbe {
    func probe(&mut self) : int
}

@direct_init
struct ImplDispatchCounter {
    var n : int

    impl ImplDispatchProbe for ImplDispatchCounter {
        func probe(&mut self) : int {
            return self.n
        }
    }
}

func call_impl_probe_concrete(c : *mut ImplDispatchCounter) : int {
    return c.probe()
}

func <P : ImplDispatchProbe> call_impl_probe_generic(c : *mut P) : int {
    return c.probe()
}

func test_impl_only_method_call() {
    test("concrete call to an impl-only method dispatches to the impl", () => {
        var c = ImplDispatchCounter { n : 7 }
        return call_impl_probe_concrete(&raw mut c) == 7
    })
    test("generic call to an impl-only method dispatches to the impl", () => {
        var c = ImplDispatchCounter { n : 9 }
        return call_impl_probe_generic<ImplDispatchCounter>(&raw mut c) == 9
    })
}

// ---------------------------------------------------------------------------
// B6: a generic struct with a destructor passed by value into a generic
//     function whose parameter is written with the generic parameter applied
//     (`H<T>`) reported "unknown value being moved". This is the exact
//     `block_on<T>(handle : FutureHandle<T>)` shape.
// ---------------------------------------------------------------------------

@direct_init
struct MoveProbe<T> {
    var value : T
    var drops : *mut int

    @delete
    func delete(&mut self) {
        if(self.drops != null) {
            unsafe { *self.drops = *self.drops + 1 }
        }
    }
}

func <T> consume_move_probe(h : MoveProbe<T>) : int {
    return 1
}

func test_generic_struct_by_value_param() {
    test("generic destructible struct moves into a generic fn (H<T> param)", () => {
        var drops = 0
        var h = MoveProbe<int> { value : 5, drops : &raw mut drops }
        var r = consume_move_probe<int>(h)
        return r == 1 && drops == 1
    })
    test("generic destructible struct moves for another instantiation", () => {
        var drops = 0
        var h = MoveProbe<u64> { value : 9u64, drops : &raw mut drops }
        var r = consume_move_probe<u64>(h)
        return r == 1 && drops == 1
    })
}

// ---------------------------------------------------------------------------
// B7: a `loop { ... break ... }` expression emitted the destructor of every
//     in-scope destructible local/parameter on each `break`/`continue` *and*
//     at scope exit (C backend only), destroying them more than once.
// ---------------------------------------------------------------------------

struct LoopDropProbe {
    var counter : *mut int

    @delete
    func delete(&mut self) {
        if(self.counter != null) {
            unsafe { *self.counter = *self.counter + 1 }
        }
    }
}

func loop_drop_count(counter : *mut int) : int {
    var p = LoopDropProbe { counter : counter }
    var out : int = loop {
        if(true) {
            break 5
        } else {
            continue
        }
    }
    return out
}

func test_loop_expression_cleanup_once() {
    test("loop expression destroys in-scope locals exactly once", () => {
        var counter = 0
        var r = loop_drop_count(&raw mut counter)
        return counter == 1 && r == 5
    })
}
