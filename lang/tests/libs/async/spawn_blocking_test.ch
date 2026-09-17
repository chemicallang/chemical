// `async::spawn_blocking` — thread-pool bridge tests (design F4/F5).
//
// Additive runtime surface: `spawn_blocking(f)` submits `f` to a shared pool and
// returns a `FutureHandle<T>` that becomes `Ready` when the worker finishes.
// Run under `./scripts/test.sh --tcc --libs` and `--llvm --libs`.
using namespace std;

var sb_runs : int = 0;

@test
func test_async_spawn_blocking_eager(env : &mut TestEnv) {
    var v = async::block_on(async::spawn_blocking<int>(() => 42))
    if(v != 42) {
        env.error("spawn_blocking eager result mismatch")
    }
}

@test
func test_async_spawn_blocking_captures(env : &mut TestEnv) {
    var base = 40
    var v = async::block_on(async::spawn_blocking<int>(|base|() => base + 2))
    if(v != 42) {
        env.error("spawn_blocking capturing result mismatch")
    }
}

@test
func test_async_spawn_blocking_destructible_result(env : &mut TestEnv) {
    var s = async::block_on(async::spawn_blocking<std::string>(() => std::string("hello")))
    if(s.size() != 5u) {
        env.error("spawn_blocking string result size mismatch")
    }
}

// Await a blocking task from inside a compiler-lowered async function.
async func sb_await_in_async(x : int) : int {
    var v = await async::spawn_blocking<int>(|x|() => x * 3)
    return v + 1
}

@test
func test_async_spawn_blocking_awaited(env : &mut TestEnv) {
    var v = async::block_on<int>(sb_await_in_async(13))
    if(v != 40) {
        env.error("awaiting spawn_blocking inside an async func mismatch")
    }
}

@test
func test_async_spawn_blocking_many(env : &mut TestEnv) {
    // sum of i*i for i in [0, 8) = 140
    var total = 0
    var i = 0
    while(i < 8) {
        total = total + async::block_on(async::spawn_blocking<int>(|i|() => i * i))
        i = i + 1
    }
    if(total != 140) {
        env.error("spawn_blocking many-result sum mismatch")
    }
}

// Dropping a `spawn_blocking` handle before the worker completes must be safe:
// the worker still runs, observes `abandoned`, and frees the shared state itself.
@test
func test_async_spawn_blocking_cancel(env : &mut TestEnv) {
    var before = sb_runs
    {
        var h = async::spawn_blocking<int>(() => {
            std::concurrent.sleep_ms(50u)
            sb_runs = sb_runs + 1
            return 7
        })
        var cx = core::async::Context {
            waker : core::async::Waker { data : null, vtbl : null }
        }
        var p = h.vtbl.poll(h.frame, &raw mut cx)
        if(!(p is core::async::Poll.Pending)) {
            env.error("spawn_blocking should be pending while the worker runs")
        }
    }
    std::concurrent.sleep_ms(200u)
    if(sb_runs != before + 1) {
        env.error("the worker must run even if the future is dropped first")
    }
}
