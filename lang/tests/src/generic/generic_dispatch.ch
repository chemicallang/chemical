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
