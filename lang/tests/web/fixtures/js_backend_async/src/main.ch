// JS backend — promise-based async/await.
//
// In a JS build an `async func` is a real JS `async function`: the body runs
// until an `await`, the promise it returns carries the result, and `await`
// suspends the caller's body instead of driving a poll loop. Native async uses
// the coroutine protocol instead, so this fixture compares against
// `expected.txt` rather than against a native build (the client harness is the
// native side of the comparison).
//
// `async func main` is the program: the bundle's `$run` awaits it.

using namespace std;

async func inc(x : int) : int {
    return x + 1
}

async func twice(x : int) : int {
    var a = await inc(x)
    return await inc(a)
}

async func sum_loop(n : int) : int {
    var total = 0
    var i = 0
    while(i < n) {
        var step = await inc(i)
        total = total + step
        i += 1
    }
    return total
}

async func order(a : int, b : int) : int {
    var x = await inc(a)
    var y = await inc(b)
    return x * 100 + y * 10
}

async func main() : int {
    unsafe {
        printf("%s=%d\n", "inc", await inc(20))
        printf("%s=%d\n", "twice", await twice(20))
        printf("%s=%d\n", "loop", await sum_loop(4))
        printf("%s=%d\n", "order", await order(1, 3))
    }
    return 0
}
