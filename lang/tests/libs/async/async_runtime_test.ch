// Async/await runtime tests.
//
// These run under `./scripts/test.sh --tcc --libs` (and `--llvm --libs`). They
// are deterministic and reactor-free: they exercise the bootstrap executor
// (`async::block_on`), the deterministic test executor (`async::test::block_on`),
// `yield_now`, and the poll-driven `sleep`, plus their composition with
// compiler-lowered `async func` bodies.
using namespace std;

async func lib_async_add_after_sleep(a : int, b : int) : int {
    var unit = await async::sleep(1u)
    return a + b
}

async func lib_async_double(x : int) : int {
    var unit = await async::yield_now()
    return x * 2
}

@test
func test_async_runtime_yield_now(env : &mut TestEnv) {
    var unit = async::block_on(async::yield_now())
    // `unit` is zero-sized; reaching here without hanging/crashing is the test.
}

@test
func test_async_runtime_test_block_on(env : &mut TestEnv) {
    var unit = async::test::block_on(async::yield_now())
}

@test
func test_async_runtime_sleep(env : &mut TestEnv) {
    var start = std::now_milli()
    var unit = async::block_on(async::sleep(10u))
    var elapsed = std::now_milli() - start
    if(elapsed < 5i64) {
        env.error("sleep returned too early")
    }
}

@test
func test_async_runtime_async_fn_await(env : &mut TestEnv) {
    var v = async::block_on(lib_async_add_after_sleep(20, 22))
    if(v != 42) {
        env.error("async func await result mismatch")
    }
}

@test
func test_async_runtime_async_fn_yield(env : &mut TestEnv) {
    var v = async::block_on(lib_async_double(21))
    if(v != 42) {
        env.error("async func yield result mismatch")
    }
}

@test
func test_async_runtime_repeated_block_on(env : &mut TestEnv) {
    var i = 0
    var sum = 0
    while(i < 3) {
        sum = sum + async::block_on(lib_async_double(i))
        i = i + 1
    }
    if(sum != 6) {
        env.error("repeated block_on sum mismatch")
    }
}
