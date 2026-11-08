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
    var cfg = process::Command.new("echo")
    cfg.arg("async hello")
    var res = async::block_on(process::output_async(cfg))
    if(res is Result.Err) {
        env.error("process::execute_async returned Err")
        return
    }
    var Ok(r) = res else unsafe { unreachable }
    if(!r.success()) {
        env.error("process::execute_async did not succeed")
        return
    }
    if(!stdout_contains(r.stdout_bytes(), "async hello")) {
        env.error("execute_async stdout did not contain expected text")
    }
}

@test
public func test_process_spawn_wait_async(env : &mut TestEnv) {
    var cfg = process::Command.new("sh")
    cfg.arg("-c")
    cfg.arg("echo waited")
    var sp = cfg.spawn()
    if(sp is Result.Err) {
        env.error("spawn failed")
        return
    }
    var Ok(child) = sp else unsafe { unreachable }
    var wr = async::block_on(process::wait_async(&mut child))
    if(wr is Result.Err) {
        env.error("wait_async returned Err")
        return
    }
    var Ok(r) = wr else unsafe { unreachable }
    if(!r.success()) {
        env.error("wait_async child did not succeed")
        return
    }
}

// `wait` (and therefore `wait_async`) must still reap a child that was killed:
// wait-after-kill is part of the synchronous contract and the async wrapper
// preserves it.
@test
public func test_process_wait_async_after_kill(env : &mut TestEnv) {
    var cfg = process::Command.new("sleep")
    cfg.arg("100")
    var sp = cfg.spawn()
    if(sp is Result.Err) {
        env.error("spawn failed")
        return
    }
    var Ok(child) = sp else unsafe { unreachable }
    var kr = child.kill()
    if(kr is Result.Err) {
        env.error("kill_process failed")
        return
    }
    var r = async::block_on(process::wait_async(&mut child))
    if(r is Result.Err) {
        env.error("wait_async after kill should still reap the child")
    }
}

// `sleep_ms_async` is backed by the runtime timer, so its first poll must
// return Pending (it genuinely yields to the executor) and it must not finish
// before the requested delay has elapsed.
@test
public func test_process_sleep_ms_async_suspends(env : &mut TestEnv) {
        unsafe {
        var h = process::sleep_ms_async(50)
        var cx = core::async::Context {
            waker : core::async::Waker.empty()
        }
        var p = h.vtbl_ptr().poll(h.frame_ptr(), &raw mut cx)
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
    }

@test
public func test_process_is_running_async(env : &mut TestEnv) {
    var cfg = process::Command.new("sleep")
    cfg.arg("100")
    var sp = cfg.spawn()
    if(sp is Result.Err) {
        env.error("spawn failed")
        return
    }
    var Ok(child) = sp else unsafe { unreachable }
    var running = async::block_on(process::is_running_async(&mut child))
    if(!running) {
        env.error("is_running_async should report true right after spawn")
    }
    child.kill()
}

// `spawn_async` and `wait_async` both offload to `spawn_blocking`; the config is
// captured by value into the task and the result (a nested `ProcessResult` with
// two vectors) is moved back out through the shared state.
@test
public func test_process_spawn_async_then_wait_async(env : &mut TestEnv) {
    var cfg = process::Command.new("echo")
    cfg.arg("spawned")
    var sp = async::block_on(process::spawn_async(cfg))
    if(sp is Result.Err) {
        env.error("process::spawn_async returned Err")
        return
    }
    var Ok(child) = sp else unsafe { unreachable }
    var wr = async::block_on(process::wait_async(&mut child))
    if(wr is Result.Err) {
        env.error("wait_async after spawn_async returned Err")
        return
    }
    var Ok(r) = wr else unsafe { unreachable }
    if(!r.success()) {
        env.error("spawn_async child did not succeed")
        return
    }
}
