// Non-suspending async/await tests.
//
// An `async func f() : T` returns a `FutureHandle<T>` and `await e` drives the
// future to completion and yields `T`. These tests are compiled and run by
// scripts/test.sh --async on both the C and LLVM backends.
//
// The compiler currently lowers async functions to an "eager-ready" future: the
// body runs in the ramp and the generated `poll` is immediately `Ready`. The
// suspension state machine (Phase 4.2) will replace the body placement without
// changing any of these observable semantics.

using namespace std;

async func al_add(a : int, b : int) : int {
    var sum = a + b
    return sum
}

async func al_double(x : int) : int {
    return x * 2
}

async func al_await_local(x : int) : int {
    var v = x * 2
    var awaited = await v
    return awaited
}

async func al_chain(x : int) : int {
    var v = await al_add(x, 1)
    return await al_add(v, 1)
}

async func al_await_in_expr(a : int, b : int) : int {
    return (await al_add(a, 1)) + (await al_add(b, 2))
}

async func al_await_loop(n : int) : int {
    var total = 0
    var i = 0
    while(i < n) {
        var step = await al_add(i, 1)
        total = total + step
        i += 1
    }
    return total
}

async func al_await_if(flag : bool) : int {
    var cond = await flag
    if(cond) {
        return 1
    } else {
        return 0
    }
}

// destructor-bearing locals live across awaits must still be cleaned up
async func al_strings_across_await(x : int) : int {
    var s = std::string("hello")
    var n = await al_double(x)
    var t = std::string("world")
    var m = await al_double(n)
    return (s.size() as int) + (t.size() as int) + m
}

// left-to-right evaluation order with multiple awaits in one expression
async func al_eval_order(a : int, b : int, c : int) : int {
    return (await al_double(a)) * 100 + (await al_double(b)) * 10 + (await al_double(c))
}

async func al_await_while(limit : int) : int {
    var i = 0
    var cond = true
    while(await cond) {
        i += 1
        if(i >= limit) {
            cond = false
        }
    }
    return i
}

// extra cases migrated from the old main-suite async tests
async func al_return_await(x : int) : int {
    return await al_double(x)
}

async func al_await_binop(a : int, b : int) : int {
    return (await al_double(a)) + (await al_double(b))
}

async func al_await_arg(x : int) : int {
    return await al_double(await al_double(x))
}

async func al_await_condition(flag : bool) : int {
    var cond = await flag
    if(cond) {
        return 1
    } else {
        return 0
    }
}

@test
func test_async_lazy_return_await(env : &mut TestEnv) {
    if(async::block_on<int>(al_return_await(21)) != 42) {
        env.error("return await should be 42")
    }
}

@test
func test_async_lazy_await_binop(env : &mut TestEnv) {
    if(async::block_on<int>(al_await_binop(3, 4)) != 14) {
        env.error("await in a binary expression should be 14")
    }
}

@test
func test_async_lazy_await_arg(env : &mut TestEnv) {
    if(async::block_on<int>(al_await_arg(5)) != 20) {
        env.error("await as a call argument should be 20")
    }
}

@test
func test_async_lazy_await_condition_bool(env : &mut TestEnv) {
    if(async::block_on<int>(al_await_condition(true)) != 1) {
        env.error("await on a true condition should be 1")
    }
    if(async::block_on<int>(al_await_condition(false)) != 0) {
        env.error("await on a false condition should be 0")
    }
}

@test
func test_async_lazy_returns_handle_result(env : &mut TestEnv) {
    if(async::block_on<int>(al_add(2, 3)) != 5) {
        env.error("block_on(async add) should be 5")
    }
}

@test
func test_async_lazy_await_local(env : &mut TestEnv) {
    if(async::block_on<int>(al_await_local(21)) != 42) {
        env.error("await on a local value should be 42")
    }
}

@test
func test_async_lazy_await_chained_calls(env : &mut TestEnv) {
    if(async::block_on<int>(al_chain(40)) != 42) {
        env.error("await chained async calls should be 42")
    }
}

@test
func test_async_lazy_await_in_expression(env : &mut TestEnv) {
    if(async::block_on<int>(al_await_in_expr(10, 20)) != 33) {
        env.error("await in expression should be 33")
    }
}

@test
func test_async_lazy_await_in_loop(env : &mut TestEnv) {
    // sum of (i+1) for i in [0, 4) = 10
    if(async::block_on<int>(al_await_loop(4)) != 10) {
        env.error("await in while loop should be 10")
    }
}

@test
func test_async_lazy_await_condition(env : &mut TestEnv) {
    if(async::block_on<int>(al_await_if(true)) != 1) {
        env.error("await on true condition should be 1")
    }
    if(async::block_on<int>(al_await_if(false)) != 0) {
        env.error("await on false condition should be 0")
    }
}

@test
func test_async_lazy_destructible_locals_across_await(env : &mut TestEnv) {
    // "hello".size() + "world".size() + doubled(1) * 2 = 5 + 5 + 4 = 14
    if(async::block_on<int>(al_strings_across_await(1)) != 14) {
        env.error("destructible locals across await should be 14")
    }
}

@test
func test_async_lazy_await_evaluation_order(env : &mut TestEnv) {
    // 2*100 + 4*10 + 6 = 246
    if(async::block_on<int>(al_eval_order(1, 2, 3)) != 246) {
        env.error("multiple awaits should preserve left-to-right order (246)")
    }
}

@test
func test_async_lazy_await_while_condition(env : &mut TestEnv) {
    if(async::block_on<int>(al_await_while(3)) != 3) {
        env.error("await in while condition should terminate at 3")
    }
}

async func al_nested(x : int) : int {
    var doubled = await al_double(x)
    return await al_double(doubled)
}

@test
func test_async_lazy_nested(env : &mut TestEnv) {
    if(async::block_on<int>(al_nested(5)) != 20) {
        env.error("nested async calls should be 20")
    }
}
