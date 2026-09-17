// Async wrappers for the process library (design §7 Tier 4).
//
// Additive: every synchronous signature in `process.ch` is unchanged. Each entry
// point is a compiler-lowered `async func`, so the compiler emits its
// `FutureHandle<...>` vtable - no hand-authored generic future is needed, and
// callers drive it with `await` / `async::block_on`.
//
// v1 note: these entry points perform the blocking syscall directly in the ramp
// (the same pattern `fs` used before it moved to `spawn_blocking`). Offloading
// them requires moving a destructible by-value `ProcessConfig` (or a nested
// `ProcessResult`) into a thread-pool task, which currently hits backend
// codegen issues tracked as B19 in lang/docs/async-library-integration.md.
// Signatures and result types are unaffected, so the bodies can switch to
// `spawn_blocking` once B19 is fixed.
//
// Note: the polling (non-reaping) `try_wait` does not capture stdout/stderr on
// POSIX, so `wait_async` delegates to `wait`, which shares the same syscall
// helpers and preserves the synchronous result exactly.
public namespace process {

using std::Result;
using std::vector;

public async func execute_async(cfg : ProcessConfig) : PR_Result {
    return execute(cfg)
}

public async func spawn_async(cfg : ProcessConfig) : CP_Result {
    return spawn(cfg)
}

public async func wait_async(child : *mut ChildProcess) : PR_Result {
    return wait(child)
}

public async func try_wait_async(child : *mut ChildProcess) : PR_Result {
    return try_wait(child)
}

public async func kill_process_async(child : *mut ChildProcess, signal : int) : UT_Result {
    return kill_process(child, signal)
}

public async func write_stdin_async(child : *mut ChildProcess, data : *vector<u8>) : UT_Result {
    return write_stdin(child, data)
}

public async func close_stdin_async(child : *mut ChildProcess) : UT_Result {
    return close_stdin(child)
}

public async func is_running_async(child : *mut ChildProcess) : bool {
    return is_running(child)
}

// Awaitable equivalent of `sleep_ms`, backed by the runtime timer instead of
// blocking the thread: it reports `Pending` until the deadline passes, yielding
// the executor. This is the process surface's real suspend/resume path today.
public async func sleep_ms_async(ms : int) : UnitTy {
    var unit = await async::sleep(ms as u64)
    return UnitTy { }
}

}
