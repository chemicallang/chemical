// Async/await tests that run in both compiled and interpretation mode.

async func ct_async_inc(x : int) : int {
    var v = await x
    return v + 1
}

async func ct_async_sum(a : int, b : int) : int {
    return (await ct_async_inc(a)) + (await ct_async_inc(b))
}

public func test_async_common() {
    test("async await of a local value increments", () => {
        return ct_async_inc(41) == 42
    })

    test("async awaiting two async calls sums", () => {
        // (1+1) + (2+1) = 5
        return ct_async_sum(1, 2) == 5
    })
}
