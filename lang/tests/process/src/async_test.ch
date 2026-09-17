// Async wrappers over the blocking process API (lang/libs/process/src/async.ch).
//
// Additive surface: `*_async` entry points return the same result types as their
// synchronous counterparts, wrapped in a compiler-lowered future. The I/O
// wrappers use a v1 blocking body (matching `fs`); `sleep_ms_async` is backed
// by the runtime timer and really suspends. Run with
// `./scripts/test.sh --tcc --process` and `--llvm --process`.

using std::string;
using std::string_view;
using std::vector;
using std::Result;

@test
public func test_process_execute_async(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("async hello"))
    var res = async::block_on(process::execute_async(cfg))
    if(res is Result.Err) {
        env.error("process::execute_async returned Err")
        return
    }
    var Ok(r) = res else unreachable
    if(!r.success) {
        env.error("process::execute_async did not succeed")
        return
    }
    if(!stdout_contains(&raw r.output.stdout_data, "async hello")) {
        env.error("execute_async stdout did not contain expected text")
    }
}

@test
public func test_process_spawn_wait_async(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sh"))
    cfg.args.push(string("-c"))
    cfg.args.push(string("echo waited"))
    var sp = process::spawn(cfg)
    if(sp is Result.Err) {
        env.error("spawn failed")
        return
    }
    var Ok(child) = sp else unreachable
    var wr = async::block_on(process::wait_async(&raw mut child))
    if(wr is Result.Err) {
        env.error("wait_async returned Err")
        return
    }
    var Ok(r) = wr else unreachable
    if(!r.success) {
        env.error("wait_async child did not succeed")
        return
    }
    if(!stdout_contains(&raw r.output.stdout_data, "waited")) {
        env.error("wait_async stdout did not contain expected text")
    }
}

// `wait` (and therefore `wait_async`) must still reap a child that was killed:
// wait-after-kill is part of the synchronous contract and the async wrapper
// preserves it.
@test
public func test_process_wait_async_after_kill(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sleep"))
    cfg.args.push(string("100"))
    var sp = process::spawn(cfg)
    if(sp is Result.Err) {
        env.error("spawn failed")
        return
    }
    var Ok(child) = sp else unreachable
    var kr = process::kill_process(&raw mut child, 9)
    if(kr is Result.Err) {
        env.error("kill_process failed")
        return
    }
    var r = async::block_on(process::wait_async(&raw mut child))
    if(r is Result.Err) {
        env.error("wait_async after kill should still reap the child")
    }
}

// `sleep_ms_async` is backed by the runtime timer, so its first poll must
// return Pending (it genuinely yields to the executor) and it must not finish
// before the requested delay has elapsed.
@test
public func test_process_sleep_ms_async_suspends(env : &mut TestEnv) {
    var h = process::sleep_ms_async(50)
    var cx = core::async::Context {
        waker : core::async::Waker { data : null, vtbl : null }
    }
    var p = h.vtbl.poll(h.frame, &raw mut cx)
    if(!(p is core::async::Poll.Pending)) {
        env.error("sleep_ms_async should yield on its first poll")
    }
    var start = std::now_milli()
    var unit = async::block_on(h)
    var elapsed = std::now_milli() - start
    if(elapsed < 5i64) {
        env.error("sleep_ms_async completed before the requested delay")
    }
}

@test
public func test_process_is_running_async(env : &mut TestEnv) {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("sleep"))
    cfg.args.push(string("100"))
    var sp = process::spawn(cfg)
    if(sp is Result.Err) {
        env.error("spawn failed")
        return
    }
    var Ok(child) = sp else unreachable
    var running = async::block_on(process::is_running_async(&raw mut child))
    if(!running) {
        env.error("is_running_async should report true right after spawn")
    }
    process::kill_process(&raw mut child, 9)
}
