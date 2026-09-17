// Tier 0 runtime tests: executor + `spawn`/`spawn_local`, `select`,
// `timeout_or` and `block_on_timeout`. Run under
// `./scripts/test.sh --tcc --libs` and `--llvm --libs`.
using namespace std;

async func exec_compute(x : int) : int {
    var u = await async::yield_now()
    return x * 2
}

async func exec_named_await(x : int) : int {
    var j = async::spawn<int>(exec_compute(x))
    var v = await j
    return v + 1
}

func exec_slow(v : int) : int {
    std::concurrent.sleep_ms(150u)
    return v
}

@test
func test_async_spawn_direct(env : &mut TestEnv) {
    var v = async::block_on<int>(async::spawn<int>(exec_compute(21)))
    if(v != 42) {
        env.error("spawn direct result mismatch")
    }
}

@test
func test_async_spawn_await_inside_async(env : &mut TestEnv) {
    var v = async::block_on<int>(exec_named_await(10))
    if(v != 21) {
        env.error("await of a stored spawn join handle mismatch")
    }
}

@test
func test_async_spawn_local(env : &mut TestEnv) {
    var v = async::block_on<int>(async::spawn_local<int>(exec_compute(4)))
    if(v != 8) {
        env.error("spawn_local result mismatch")
    }
}

@test
func test_async_spawn_many(env : &mut TestEnv) {
    var total = 0
    var i = 0
    while(i < 5) {
        total = total + async::block_on<int>(async::spawn<int>(exec_compute(i)))
        i = i + 1
    }
    if(total != 20) {
        env.error("spawn many sum mismatch")
    }
}

@test
func test_async_select(env : &mut TestEnv) {
    var v = async::block_on<int>(async::select<int>(async::spawn<int>(exec_compute(3)), async::spawn<int>(exec_compute(7))))
    if(v != 6 && v != 14) {
        env.error("select result mismatch")
    }
}

@test
func test_async_timeout_or_completes(env : &mut TestEnv) {
    var v = async::block_on<int>(async::timeout_or<int>(async::spawn<int>(exec_compute(5)), 10000u, -1))
    if(v != 10) {
        env.error("timeout_or should have completed with the child value")
    }
}

@test
func test_async_timeout_or_fires(env : &mut TestEnv) {
    var v = async::block_on<int>(async::timeout_or<int>(async::spawn_blocking<int>(() => exec_slow(5)), 20u, -1))
    if(v != -1) {
        env.error("timeout_or should have returned the fallback")
    }
}

@test
func test_async_block_on_timeout_completes(env : &mut TestEnv) {
    var d = async::block_on_timeout<int>(async::spawn<int>(exec_compute(4)), 10000u)
    if(d is std::Option.None) {
        env.error("block_on_timeout should have completed")
        return
    }
    var Some(v) = d else unreachable
    if(v != 8) {
        env.error("block_on_timeout value mismatch")
    }
}

@test
func test_async_block_on_timeout_fires(env : &mut TestEnv) {
    var d = async::block_on_timeout<int>(async::spawn_blocking<int>(() => exec_slow(9)), 20u)
    if(d is std::Option.Some) {
        env.error("block_on_timeout should have returned None")
    }
}

// Dropping a spawn join handle cancels the task; the executor reclaims it on a
// later pass and nothing is double-freed.
@test
func test_async_spawn_cancel(env : &mut TestEnv) {
    {
        var j = async::spawn<int>(async::spawn_blocking<int>(() => exec_slow(1)))
    }
    var unit = async::block_on<core::async::Unit>(async::yield_now())
}
