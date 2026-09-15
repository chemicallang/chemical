// Real-suspension async/await tests (CHEMICAL_ASYNC_LAZY + CHEMICAL_ASYNC_SUSPEND).
//
// These drive a hand-authored future that returns `Pending` several times, so
// the compiler-generated state machine must actually suspend and resume while
// preserving parameters and locals across the suspension. Run with
// `./scripts/test.sh --tcc --async-suspend`.

var suspend_polls : int = 0;

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
