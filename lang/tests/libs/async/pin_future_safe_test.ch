// P2.6 safe Pin / FutureHandle / spawn_send coverage.
//
// `core::Pin<T>` only exposes `get_mut` for `T: Unpin`; a `!Unpin` value can
// still be pinned and read. `SendFutureHandle<T>` is the checked cross-thread
// future form produced by `async::spawn_send`, and dropping a join handle
// cancels the task so its suspended locals are destroyed exactly once.
using namespace std;

var pf_drops : int = 0;

struct PfTracked {
    var value : int

    @delete
    func delete(&mut self) {
        unsafe {
            pf_drops = pf_drops + 1
        }
    }
}

// ---- Pin -------------------------------------------------------------------

func <T> pf_needs_unpin(value : T) : int where T : core::Unpin {
    return 0
}

@test
func pin_unpin_get_mut_roundtrip(env : &mut TestEnv) {
    var value = 41
    var pinned = core::pin(&mut value)
    if(*pinned.get_ref() != 41) {
        env.error("Pin.get_ref did not expose the pinned value")
    }
    *pinned.get_mut() = 42
    if(value != 42) {
        env.error("Pin.get_mut did not write through to the value")
    }
}

@test
func pin_unchecked_pinned_get_ref(env : &mut TestEnv) {
    var value = PfTracked { value : 7 }
    var pinned = unsafe(core::pin_unchecked(&raw mut value))
    if(pinned.get_ref().value != 7) {
        env.error("Pin.get_ref on a !Unpin value returned the wrong payload")
    }
}

@test
func pin_into_inner_returns_pointer(env : &mut TestEnv) {
    var value = 5
    var pinned = core::pin(&mut value)
    var raw = pinned.into_inner()
    if(unsafe(*raw) != 5) {
        env.error("Pin.into_inner did not return the underlying pointer")
    }
}

@test
func pin_unpin_bound_accepts_plain_value(env : &mut TestEnv) {
    if(pf_needs_unpin(1) != 0) {
        env.error("a plain int should satisfy the Unpin bound")
    }
}

// ---- FutureHandle / spawn_send --------------------------------------------

async func pf_send_owned(value : int) : int {
    var tracked = PfTracked { value : value }
    var u = await async::yield_now()
    return tracked.value
}

func pf_consume_send(handle : core::async::SendFutureHandle<int>) : int {
    return async::block_on_send<int>(handle)
}

@test
func spawn_send_owned_frame_completes(env : &mut TestEnv) {
    var handle = async::spawn_send(pf_send_owned(21))
    if(async::block_on_send<int>(handle) != 21) {
        env.error("spawn_send lost its owned frame result")
    }
}

@test
func spawn_send_handle_moves_to_thread(env : &mut TestEnv) {
    var handle = async::spawn_send(pf_send_owned(22))
    var thread = std::thread.spawn<core::async::SendFutureHandle<int>, int>(
        pf_consume_send, handle)
    if(thread.join() != 22) {
        env.error("SendFutureHandle did not complete after moving to a worker")
    }
}

// Dropping a `spawn_send` join handle cancels the task; the frame's suspended
// local must be destroyed exactly once when the executor reclaims it.
@test
func spawn_send_drop_cancels_frame_once(env : &mut TestEnv) {
    unsafe {
        pf_drops = 0
    }
    {
        var handle = async::spawn_send(pf_send_owned(1))
        // Run the task up to its first suspension.
        var u = async::block_on<core::async::Unit>(async::yield_now())
        // `handle` is dropped here, cancelling the task.
    }
    // Drive the executor once more so it observes the cancellation and drops
    // the task frame.
    var u2 = async::block_on<core::async::Unit>(async::yield_now())
    if(unsafe(pf_drops) != 1) {
        env.error("cancelling spawn_send must destroy its suspended local once")
    }
}

// The safe executor facade is callable without naming `Executor` or touching
// its raw queue/state; only the unsafe raw entry points expose those.
@test
func executor_drive_safe_wrapper(env : &mut TestEnv) {
    var completed = async::executor_drive(4096u)
    if(completed > 4096u) {
        env.error("executor_drive returned an impossible completion count")
    }
}
