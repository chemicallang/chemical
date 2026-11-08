// Readiness futures over file descriptors (`AsyncFd`), built on the platform
// reactor in `posix/reactor.ch` / `win/reactor.ch`.
//
// `readable(fd)` / `writable(fd)` become `Ready(true)` when the fd is ready for
// the requested operation. Polling is non-blocking: a `Pending` poll registers
// the task's waker with the process-wide reactor, and `executor_wait` blocks on
// the reactor while any fd is registered (so an idle executor does not spin).
//
// Tier 1 wires this to `net::AsyncSocket`.
public namespace async {

// A registered file descriptor. `readable`/`writable` complete when the fd is
// ready for that operation; the fd is never closed by `AsyncFd`.
public struct AsyncFd {
    var fd : int

    public func readable(&self) : core::async::FutureHandle<'static, core::async::Unit> {
        unsafe {
            return raw::make_ready(self.fd, posix::POLLIN)
    
        }
    }

    public func writable(&self) : core::async::FutureHandle<'static, core::async::Unit> {
        unsafe {
            return raw::make_ready(self.fd, posix::POLLOUT)
    
        }
    }
}

public func readable(fd : int) : core::async::FutureHandle<'static, core::async::Unit> {
        unsafe {
        return raw::make_ready(fd, posix::POLLIN)

        }
    }

public func writable(fd : int) : core::async::FutureHandle<'static, core::async::Unit> {
        unsafe {
        return raw::make_ready(fd, posix::POLLOUT)

        }
    }

}
