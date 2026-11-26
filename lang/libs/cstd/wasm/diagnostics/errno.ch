/**
 * errno for the freestanding wasm profile.
 *
 * There is no OS error channel: the runtime never sets errno, and the console
 * write path only fails on OOM/abort. The value exists so the shared std
 * taxonomy (`error_from_errno`) and `get_errno`/`set_errno` compile and behave
 * predictably. The POSIX codes are included because std names them.
 */


public func get_errno() : int {
    return 0
}

public func set_errno(value : int) {
    // no OS error channel in the freestanding wasm profile; keep the API and
    // drop the value rather than expose a mutable global.
}

public comptime const EPERM = 1
public comptime const ENOENT = 2
public comptime const EINTR = 4
public comptime const EIO = 5
public comptime const EBADF = 9
public comptime const EAGAIN = 11
public comptime const ENOMEM = 12
public comptime const EACCES = 13
public comptime const EEXIST = 17
public comptime const EINVAL = 22
public comptime const ENAMETOOLONG = 36
public comptime const EPIPE = 32
public comptime const ENOSPC = 28
public comptime const ECONNRESET = 104
public comptime const ECONNREFUSED = 111
public comptime const ETIMEDOUT = 110
public comptime const EWOULDBLOCK = EAGAIN
