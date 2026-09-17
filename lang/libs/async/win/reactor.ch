// Windows stub for the fd reactor. A real IOCP-backed reactor is Tier 1
// (`net::iocp` already exposes the completion port); until then readiness
// futures complete immediately and callers retry non-blocking I/O.
public namespace async {
    public func pipe(fds : *mut int) : int {
        return -1
    }
}
public namespace async {
public namespace posix {

    public struct TimeVal {
        var tv_sec : long
        var tv_usec : long
    }

    public struct PollFd {
        var fd : int
        var events : short
        var revents : short
    }

    comptime const POLLIN : short = 1
    comptime const POLLOUT : short = 4
    comptime const POLLERR : short = 8
    comptime const POLLHUP : short = 16

    public func reactor_is_ready(fd : int, events : short) : bool {
        return true
    }

    public func reactor_platform_poll(fds : *mut PollFd, n : ulong, timeout_ms : int) : int {
        return 0
    }

}
}
