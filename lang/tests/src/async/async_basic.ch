// Phase 1 async/await tests.
//
// At this stage `async` / `await` are implemented as an eager, transparent
// bootstrap: an `async func f() : T` is callable and returns `T`, and
// `await e` yields the value of `e`. These tests pin down the surface
// (parsing, symbol resolution, type checking, codegen) so the lazy coroutine
// lowering can replace the bootstrap without changing user-visible behaviour.
//
// Only scalar result types are used deliberately, so the tests are not
// affected by the known LLVM destructor-by-value-return bug (B9).

// ---------------------------------------------------------------------------
// Simple async functions
// ---------------------------------------------------------------------------

async func async_add(a : int, b : int) : int {
    var sum = a + b
    return sum
}

async func async_await_local(x : int) : int {
    var v = x * 2
    var awaited = await v
    return awaited
}

async func async_chain(x : int) : int {
    var v = await async_add(x, 1)
    return await async_add(v, 1)
}

async func async_await_in_expr(a : int, b : int) : int {
    return (await async_add(a, 1)) + (await async_add(b, 2))
}

// `await` on a boolean condition
async func async_await_condition(flag : bool) : int {
    var cond = await flag
    if(cond) {
        return 1
    } else {
        return 0
    }
}

// `await` inside a loop, accumulating results
async func async_await_loop(n : int) : int {
    var total = 0
    var i = 0
    while(i < n) {
        var step = await async_add(i, 1)
        total = total + step
        i += 1
    }
    return total
}

// async function calling another async function directly
async func async_double(x : int) : int {
    return x * 2
}

async func async_nested(x : int) : int {
    var doubled = await async_double(x)
    return await async_double(doubled)
}

// ---------------------------------------------------------------------------
// Normalization: awaits nested inside larger expressions / conditions.
// These exercise AwaitNormalizePass (hoisting into VarInitStatements).
// ---------------------------------------------------------------------------

// `return await ...` (hoisted before the return)
async func async_return_await(x : int) : int {
    return await async_double(x)
}

// await in a binary expression
async func async_await_binop(a : int, b : int) : int {
    return (await async_double(a)) + (await async_double(b))
}

// await as a call argument
async func async_await_arg(x : int) : int {
    return async_double(await async_double(x))
}

// await in an if condition
async func async_await_if(flag : bool) : int {
    if(await flag) {
        return 10
    } else {
        return 20
    }
}

// await in a while condition, expecting immediate termination
async func async_await_while(limit : int) : int {
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

// left-to-right evaluation order with multiple awaits in one expression
async func async_eval_order(a : int, b : int, c : int) : int {
    return (await async_double(a)) * 100 + (await async_double(b)) * 10 + (await async_double(c))
}

// destructor-bearing locals live across awaits must be tracked and cleaned up
async func async_strings_across_await(x : int) : int {
    var s = std::string("hello")
    var n = await async_double(x)
    var t = std::string("world")
    var m = await async_double(n)
    return (s.size() as int) + (t.size() as int) + m
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

func test_async_basic() {
    test("async function returns its value", () => {
        return async_add(2, 3) == 5
    })

    test("await on a local value", () => {
        return async_await_local(21) == 42
    })

    test("await chained async calls", () => {
        return async_chain(40) == 42
    })

    test("await in an expression", () => {
        return async_await_in_expr(10, 20) == 33
    })

    test("await on a boolean", () => {
        return async_await_condition(true) == 1
    })

    test("await inside a while loop", () => {
        // sum of (i+1) for i in [0, n) = n*(n+1)/2; n=4 -> 10
        return async_await_loop(4) == 10
    })

    test("nested async calls", () => {
        return async_nested(5) == 20
    })

    test("return await is normalized", () => {
        return async_return_await(21) == 42
    })

    test("await in a binary expression", () => {
        return async_await_binop(3, 4) == 14
    })

    test("await as a call argument", () => {
        return async_await_arg(5) == 20
    })

    test("await in an if condition (true)", () => {
        return async_await_if(true) == 10
    })

    test("await in an if condition (false)", () => {
        return async_await_if(false) == 20
    })

    test("await in a while condition", () => {
        return async_await_while(3) == 3
    })

    test("multiple awaits preserve left-to-right order", () => {
        // doubled: 2*100 + 4*10 + 6 = 246
        return async_eval_order(1, 2, 3) == 246
    })

    test("destructible locals live across awaits are cleaned up", () => {
        // "hello".size() + "world".size() + double(1) doubled = 5 + 5 + 4 = 14
        return async_strings_across_await(1) == 14
    })
}
