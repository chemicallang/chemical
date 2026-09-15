// Real-suspension async/await tests (CHEMICAL_ASYNC_LAZY + CHEMICAL_ASYNC_SUSPEND).
//
// These drive a hand-authored future that returns `Pending` several times, so
// the compiler-generated state machine must actually suspend and resume while
// preserving parameters and locals across the suspension. Run with
// `./scripts/test.sh --tcc --async-suspend`.

var suspend_polls : int = 0;
var suspend_drops : int = 0;

// A destructor-bearing payload used to observe cancellation cleanup.
struct DropCounter {
    var value : int

    @delete
    func delete(&mut self) {
        suspend_drops = suspend_drops + 1
    }
}

// A future that is Pending `remaining` times before becoming Ready.
struct Countdown {
    var remaining : int
    var value : int
}

func countdown_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<int> {
    suspend_polls = suspend_polls + 1;
    var f = frame as *mut Countdown
    if(f.remaining > 0) {
        f.remaining = f.remaining - 1
        return core::async::Poll.Pending<int>()
    } else {
        return core::async::Poll.Ready<int>(f.value)
    }
}

func countdown_drop(frame : *mut void) { }

func make_countdown(remaining : int, value : int) : core::async::FutureHandle<int> {
    var f = malloc(sizeof(Countdown)) as *mut Countdown
    f.remaining = remaining
    f.value = value
    var table = malloc(sizeof(core::async::FutureTable<int>)) as *mut core::async::FutureTable<int>
    table.poll = countdown_poll
    table.drop = countdown_drop
    return core::async::FutureHandle<int> { frame : f as *mut void, vtbl : table }
}

// Two sequential suspensions; parameters and the first result must survive.
async func suspend_two(x : int) : int {
    var a = await make_countdown(3, 40)
    var b = await make_countdown(2, 1)
    return a + x + b
}

// Suspension inside a loop; the accumulator and index must survive.
async func suspend_loop(n : int) : int {
    var total = 0
    var i = 0
    while(i < n) {
        var step = await make_countdown(1, i + 1)
        total = total + step
        i += 1
    }
    return total
}

// Suspension inside a branch; the branch-local result must survive.
async func suspend_in_branch(flag : bool) : int {
    var base = 10
    if(flag) {
        var v = await make_countdown(2, 30)
        return base + v
    } else {
        var v = await make_countdown(1, 5)
        return base + v
    }
}

// A ready future must not suspend at all (single poll, no state change).
async func suspend_ready(x : int) : int {
    var v = await make_countdown(0, x)
    return v + 1
}

// Destructor-bearing locals live across suspensions; they must be frame-resident
// so their address is stable and their destructor runs exactly once.
async func suspend_strings(x : int) : int {
    var s = std::string("hello")
    var n = await make_countdown(2, 10)
    var t = std::string("world")
    var m = await make_countdown(1, x)
    return (s.size() as int) + (t.size() as int) + n + m
}

@test
func test_async_suspend_sequential(env : &mut TestEnv) {
    if(async::block_on<int>(suspend_two(1)) != 42) {
        env.error("two sequential suspensions should sum to 42")
    }
}

@test
func test_async_suspend_in_loop(env : &mut TestEnv) {
    if(async::block_on<int>(suspend_loop(4)) != 10) {
        env.error("suspension inside a loop should accumulate to 10")
    }
}

@test
func test_async_suspend_in_branch(env : &mut TestEnv) {
    if(async::block_on<int>(suspend_in_branch(true)) != 40) {
        env.error("suspension in the true branch should be 40")
    }
    if(async::block_on<int>(suspend_in_branch(false)) != 15) {
        env.error("suspension in the false branch should be 15")
    }
}

@test
func test_async_suspend_ready_no_suspend(env : &mut TestEnv) {
    var before = suspend_polls
    var r = async::block_on<int>(suspend_ready(41))
    if(r != 42) {
        env.error("ready future should yield 42")
    }
    if(suspend_polls != before + 1) {
        env.error("a ready future must be polled exactly once")
    }
}

@test
func test_async_suspend_destructible_locals(env : &mut TestEnv) {
    // "hello".size() + "world".size() + 10 + x  => 5 + 5 + 10 + 22 = 42
    if(async::block_on<int>(suspend_strings(22)) != 42) {
        env.error("destructor-bearing locals across suspensions should be 42")
    }
}

// A suspended future that holds a destructor-bearing local.
async func suspend_cancel() : int {
    var d = DropCounter { value : 7 }
    var v = await make_countdown(3, 1)
    return d.value + v
}

// A transparent `await e` (operand is not a future) must not become a frame
// site; it is emitted inline and must coexist with real suspension sites.
async func suspend_transparent(x : int) : int {
    var v = await x
    var n = await make_countdown(1, 10)
    return v + n
}

@test
func test_async_suspend_transparent_await(env : &mut TestEnv) {
    if(async::block_on<int>(suspend_transparent(32)) != 42) {
        env.error("a transparent await mixed with a real suspension should be 42")
    }
}

// Awaiting a destructor-bearing result: the value is moved out of the child's
// `Poll` into a frame-resident result slot, used, then destroyed at scope end.
async func make_label(x : int) : std::string {
    var n = await make_countdown(1, x)
    return std::string("hello-world")
}

async func use_label() : int {
    var s = await make_label(3)
    return s.size() as int
}

// Two awaits with different result types in one function (per-site Poll<T>).
async func two_labels() : int {
    var a = await make_label(1)
    var b = await make_label(2)
    return (a.size() + b.size()) as int
}

@test
func test_async_suspend_destructible_result(env : &mut TestEnv) {
    if(async::block_on<int>(use_label()) != 11) {
        env.error("awaiting a destructor-bearing result should yield 11")
    }
    if(async::block_on<int>(two_labels()) != 22) {
        env.error("two destructor-bearing await results should yield 22")
    }
}

@test
func test_async_suspend_destructible_result_block_on(env : &mut TestEnv) {
    var s = async::block_on<std::string>(make_label(5))
    if(s.size() != 11) {
        env.error("block_on of a destructor-bearing result should yield an 11-char string")
    }
}

// Arrays across a suspension: buffers are frame-resident and stay valid across
// the suspend/resume boundary.
async func suspend_array_zeroed(x : int) : int {
    var buf : [8]char = zeroed<[8]char>()
    buf[0] = 42 as char
    var n = await make_countdown(1, 5)
    buf[1] = (x + n) as char
    return (buf[0] as int) + (buf[1] as int)
}

async func suspend_array_literal() : int {
    var arr : [3]int = [1, 2, 3]
    var n = await make_countdown(1, 4)
    return arr[0] + arr[1] + arr[2] + n
}

@test
func test_async_suspend_arrays(env : &mut TestEnv) {
    if(async::block_on<int>(suspend_array_zeroed(37)) != 84) {
        env.error("a zeroed buffer mutated across a suspension should be 84")
    }
    if(async::block_on<int>(suspend_array_literal()) != 10) {
        env.error("a literal array across a suspension should be 10")
    }
}

@test
func test_async_suspend_cancel_destroys_locals(env : &mut TestEnv) {
    var before = suspend_drops
    {
        var h = suspend_cancel()
        var cx = core::async::Context {
            waker : core::async::Waker { data : null, vtbl : null }
        }
        var p = h.vtbl.poll(h.frame, &raw mut cx)
        if(p is core::async::Poll.Pending) {
            // `h` is dropped at the end of this block, cancelling the suspended
            // future; its frame-resident DropCounter must be destroyed once
        } else {
            env.error("expected the future to be pending on the first poll")
        }
    }
    if(suspend_drops != before + 1) {
        env.error("cancelling a suspended future must destroy its frame-resident local exactly once")
    }
}

@test
func test_async_suspend_poll_counts(env : &mut TestEnv) {
    // two countdowns of 3 and 2 pending steps => 6 polls; plus the one that
    // resolves each future => the poll that returns Ready is counted too
    var before = suspend_polls
    var r = async::block_on<int>(suspend_two(0))
    if(r != 41) {
        env.error("suspend_two(0) should be 41")
    }
    if(suspend_polls != before + 7) {
        env.error("suspend_two should take 7 polls of the child futures")
    }
}
