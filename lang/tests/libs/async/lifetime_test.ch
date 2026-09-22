// Lifetime / destructor / cancellation coverage for the async runtime.
//
// These tests use a global destructor counter so leaks and double-free bugs fail
// deterministically instead of silently corrupting the heap. Run under
// ./scripts/test.sh --libs (TCC and LLVM).
using namespace std;

var life_drops : int = 0;

struct LifeTracked {
    var v : int

    @delete
    func delete(&mut self) {
        life_drops = life_drops + 1;
    }
}

// ---- channel queue ownership ----------------------------------------------

// Values still queued when a channel is dropped must have their destructors run.
@test
func test_channel_drop_destroys_queued_values(env : &mut TestEnv) {
    life_drops = 0;
    {
        var ch = async::channel<LifeTracked>()
        ch.sender.send(LifeTracked { v : 1 })
        ch.sender.send(LifeTracked { v : 2 })
        ch.sender.close()
        // drop ch (sender + receiver) without draining
    }
    if(life_drops != 2) {
        env.error("dropping a channel with queued values must destroy each exactly once")
    }
}

// Control for the above: draining the queue destroys each value exactly once.
@test
func test_channel_drain_destroys_values_once(env : &mut TestEnv) {
    life_drops = 0;
    var ch = async::channel<LifeTracked>()
    ch.sender.send(LifeTracked { v : 1 })
    ch.sender.send(LifeTracked { v : 2 })
    ch.sender.close()
    {
        var a = ch.receiver.try_recv()
        var b = ch.receiver.try_recv()
        if(a is std::Option.None || b is std::Option.None) {
            env.error("try_recv should drain the queued values")
            return
        }
    }
    if(life_drops != 2) {
        env.error("drained values must be destroyed exactly once")
    }
}

// `send` after `close` fails and leaves already-queued values intact.
@test
func test_channel_send_after_close(env : &mut TestEnv) {
    var ch = async::channel<int>()
    ch.sender.send(1)
    ch.sender.close()
    if(ch.sender.send(2)) {
        env.error("send after close must return false")
    }
    var a = ch.receiver.try_recv()
    if(a is std::Option.None) {
        env.error("a value queued before close must still be received")
        return
    }
    var Some(v) = a else unreachable
    if(v != 1) {
        env.error("queued value mismatch after close")
    }
    if(!(ch.receiver.try_recv() is std::Option.None)) {
        env.error("no value should remain after draining a closed channel")
    }
    if(!ch.receiver.is_closed()) {
        env.error("receiver should observe the closed channel")
    }
}

// A cloned sender keeps the channel open; dropping the clone must not close it
// while the original sender is still alive.
@test
func test_channel_sender_clone_refcount(env : &mut TestEnv) {
    var ch = async::channel<int>()
    {
        var s2 = ch.sender.clone_sender()
        if(ch.receiver.is_closed()) {
            env.error("cloning a sender must not close the channel")
        }
    }
    if(ch.receiver.is_closed()) {
        env.error("dropping a sender clone must not close while the original is alive")
    }
}

// ---- combinator / spawn cancellation ---------------------------------------

// A future that owns a destructor-bearing local and then suspends.
async func life_sleeper(ms : u64, v : int) : int {
    var t = LifeTracked { v : v }
    var u = await async::sleep(ms)
    return t.v
}

// `timeout_or` must cancel (drop) the inner future when it times out.
@test
func test_timeout_or_cancels_child_destroys_locals(env : &mut TestEnv) {
    life_drops = 0;
    var v = async::block_on<int>(async::timeout_or<int>(life_sleeper(100000u, 5), 10u, -1))
    if(v != -1) {
        env.error("timeout_or should have returned the fallback")
        return
    }
    if(life_drops != 1) {
        env.error("timing out must destroy the child's suspended local exactly once")
    }
}

// `block_on_timeout` must cancel the handle when it gives up.
@test
func test_block_on_timeout_cancels_child(env : &mut TestEnv) {
    life_drops = 0;
    var d = async::block_on_timeout<int>(life_sleeper(100000u, 5), 10u)
    if(d is std::Option.Some) {
        env.error("block_on_timeout should have returned None")
        return
    }
    if(life_drops != 1) {
        env.error("block_on_timeout must destroy the suspended child's local exactly once")
    }
}

async func life_quick(v : int) : int { return v }

// `select` must cancel (drop) the losing branch, destroying its locals.
@test
func test_select_cancels_loser_destroys_locals(env : &mut TestEnv) {
    life_drops = 0;
    var v = async::block_on<int>(async::select<int>(life_sleeper(100000u, 5), life_quick(7)))
    if(v != 7) {
        env.error("select should have taken the ready branch")
        return
    }
    if(life_drops != 1) {
        env.error("select must destroy the cancelled branch's local exactly once")
    }
}

// Dropping a spawn join handle before the task completes cancels it, and the
// task frame's suspended locals must be destroyed exactly once.
@test
func test_spawn_cancel_destroys_task_locals(env : &mut TestEnv) {
    life_drops = 0;
    {
        var h = async::spawn<int>(life_sleeper(100000u, 3))
        // Drive the executor so the task runs up to its suspension.
        var u = async::block_on<core::async::Unit>(async::yield_now())
        // `h` is dropped here, cancelling the task.
    }
    // Drive the executor once more so it reclaims the cancelled task.
    var u2 = async::block_on<core::async::Unit>(async::yield_now())
    if(life_drops != 1) {
        env.error("cancelling a spawned task must destroy its local exactly once")
    }
}

// ---- destructible payloads through generic runtime futures -----------------

async func life_make_str(n : int) : std::string {
    var u = await async::yield_now()
    return std::string("abcdefghij")
}

@test
func test_spawn_destructible_payload(env : &mut TestEnv) {
    var s = async::block_on<std::string>(async::spawn<std::string>(life_make_str(1)))
    if(s.size() != 10u) {
        env.error("spawned destructible payload size mismatch")
    }
}

@test
func test_select_destructible_payload(env : &mut TestEnv) {
    var s = async::block_on<std::string>(async::select<std::string>(life_make_str(1), life_make_str(2)))
    if(s.size() != 10u) {
        env.error("select destructible payload size mismatch")
    }
}

// `recv_or` can carry a destructible fallback, returned once the channel closes.
@test
func test_channel_recv_or_destructible_fallback(env : &mut TestEnv) {
    var ch = async::channel<std::string>()
    ch.sender.close()
    var s = async::block_on<std::string>(ch.receiver.recv_or(std::string("fallback")))
    if(s.size() != 8u) {
        env.error("recv_or should return the destructible fallback when closed")
    }
}
