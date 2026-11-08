// Async wrappers for the process library.
//
// Additive: the synchronous `Command`/`Child` API is unchanged. The blocking
// entry points offload their work to the runtime thread pool via
// `async::spawn_blocking`, so the executor keeps running while the child
// process is created or reaped.
public namespace process {

using std::Result;
using std::vector;

public async func output_async(cmd : Command) : Result<Output, ProcessError> {
    var result = await unsafe(async::spawn_blocking<Result<Output, ProcessError>>(
        |cmd|() => {
            var owned = std::replace<Command>(&mut cmd, unsafe(std::mem.zeroed_unchecked<Command>()))
            return owned.output()
        }
    ))
    return result
}

// Compatibility alias for the old name.
public async func execute_async(cmd : Command) : Result<Output, ProcessError> {
    return await output_async(cmd)
}

public async func spawn_async(cmd : Command) : Result<Child, ProcessError> {
    var result = await unsafe(async::spawn_blocking<Result<Child, ProcessError>>(
        |cmd|() => {
            var owned = std::replace<Command>(&mut cmd, unsafe(std::mem.zeroed_unchecked<Command>()))
            return owned.spawn()
        }
    ))
    return result
}

public async func wait_async(child : &mut Child) : Result<ExitStatus, ProcessError> {
    var result = await unsafe(async::spawn_blocking<Result<ExitStatus, ProcessError>>(|child|() => child.wait()))
    return result
}

public async func try_wait_async(child : &mut Child) : Result<std::Option<ExitStatus>, ProcessError> {
    return child.try_wait()
}

public async func kill_process_async(child : &mut Child, signal : int) : UT_Result {
    return kill_process(child, signal)
}

public async func write_stdin_async(child : &mut Child, data : &[u8]) : UT_Result {
    return write_stdin_raw(&raw mut child.inner, data.data(), data.size())
}

public async func close_stdin_async(child : &mut Child) : UT_Result {
    return child.close_stdin()
}

public async func is_running_async(child : &mut Child) : bool {
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
