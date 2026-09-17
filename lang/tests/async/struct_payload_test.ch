// Regression tests for B23: an `async func` whose payload is a plain
// (non-variant) struct must return its field values, not pointer halves.
//
// The coroutine result store used to push the `StructValue` alloca pointer into
// the frame's struct result slot (`return Pair { ... }`), so the caller read the
// pointer's low/high halves as the fields on LLVM. Returning a *variable* was
// already correct. Both the eager (no-await) and suspending paths are covered,
// plus a destructor-bearing struct to make sure the byte-copy does not
// double-free.

using namespace std;

public struct B23Pair {
    var a : int
    var b : int
}

public struct B23Named {
    var s : std::string
    var n : int
}

async func b23_pair_literal() : B23Pair {
    return B23Pair { a: 1, b: 2 }
}

async func b23_pair_suspend() : B23Pair {
    var u = await async::yield_now()
    return B23Pair { a: 3, b: 4 }
}

async func b23_named_literal() : B23Named {
    return B23Named { s: std::string("hello"), n: 7 }
}

async func b23_named_suspend() : B23Named {
    var u = await async::yield_now()
    return B23Named { s: std::string("world"), n: 9 }
}

@test
func test_async_struct_payload_literal(env : &mut TestEnv) {
    var p = async::block_on<B23Pair>(b23_pair_literal())
    if(p.a != 1 || p.b != 2) {
        env.error("plain-struct literal payload should be (1, 2)")
    }
}

@test
func test_async_struct_payload_suspend(env : &mut TestEnv) {
    var p = async::block_on<B23Pair>(b23_pair_suspend())
    if(p.a != 3 || p.b != 4) {
        env.error("plain-struct payload after suspension should be (3, 4)")
    }
}

@test
func test_async_struct_payload_destructible(env : &mut TestEnv) {
    var e = async::block_on<B23Named>(b23_named_literal())
    if(e.n != 7 || e.s.size() != 5u) {
        env.error("destructible eager payload should be n=7 size=5")
    }
    var s = async::block_on<B23Named>(b23_named_suspend())
    if(s.n != 9 || s.s.size() != 5u) {
        env.error("destructible payload after suspension should be n=9 size=5")
    }
}
