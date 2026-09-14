// Async/await tests that run in both compiled and interpretation mode.

async func ct_async_inc(x : int) : int {
    var v = await x
    return v + 1
}

async func ct_async_sum(a : int, b : int) : int {
    return (await ct_async_inc(a)) + (await ct_async_inc(b))
}

async func ct_async_return_await(x : int) : int {
    return await ct_async_inc(x)
}

async func ct_async_if(flag : bool) : int {
    if(await flag) {
        return 1
    } else {
        return 0
    }
}

public func test_async_common() {
    test("async await of a local value increments", () => {
        return ct_async_inc(41) == 42
    })

    test("async awaiting two async calls sums", () => {
        // (1+1) + (2+1) = 5
        return ct_async_sum(1, 2) == 5
    })

    test("async return await", () => {
        return ct_async_return_await(41) == 42
    })

    test("async await in an if condition", () => {
        return ct_async_if(true) == 1
    })
}
