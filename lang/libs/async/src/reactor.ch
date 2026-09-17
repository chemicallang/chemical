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

public struct ReactorFd {
    var fd : int
    var events : short
    var read_waker : core::async::Waker
    var write_waker : core::async::Waker
    var next : *mut ReactorFd
}

public struct Reactor {
    var m : std::mutex
    var head : *mut ReactorFd
}

@never_destructed
var g_reactor : Reactor
var g_reactor_ready : bool = false

public func reactor() : *mut Reactor {
    if(!g_reactor_ready) {
        g_reactor = Reactor {
            m : std::mutex(),
            head : null
        }
        g_reactor_ready = true
    }
    return &raw mut g_reactor
}

public func reactor_register(fd : int, events : short, waker : core::async::Waker) {
    var e = reactor()
    e.m.lock()
    var fe = e.head
    while(fe != null) {
        if(fe.fd == fd) {
            if((events & posix::POLLIN) != 0) {
                fe.read_waker = waker.clone()
            }
            if((events & posix::POLLOUT) != 0) {
                fe.write_waker = waker.clone()
            }
            fe.events = fe.events | events
            e.m.unlock()
            return
        }
        fe = fe.next
    }
    var rw = core::async::Waker { data : null, vtbl : null }
    var ww = core::async::Waker { data : null, vtbl : null }
    if((events & posix::POLLIN) != 0) {
        rw = waker.clone()
    }
    if((events & posix::POLLOUT) != 0) {
        ww = waker.clone()
    }
    var node = malloc(sizeof(ReactorFd)) as *mut ReactorFd
    new(node) ReactorFd {
        fd : fd,
        events : events,
        read_waker : rw,
        write_waker : ww,
        next : e.head
    }
    e.head = node
    e.m.unlock()
}

public func reactor_has_fds() : bool {
    var e = reactor()
    e.m.lock()
    var h = e.head
    e.m.unlock()
    return h != null
}

// Poll the registered fds once, waking any task whose fd became ready. Blocks
// up to `timeout_ms` (0 = non-blocking). Returns the number of ready fds.
public func reactor_poll(timeout_ms : int) : int {
    var e = reactor()
    e.m.lock()
    var n = 0u
    var it = e.head
    while(it != null) {
        n = n + 1u
        it = it.next
    }
    if(n == 0u) {
        e.m.unlock()
        return 0
    }
    var pfd = malloc(n * sizeof(posix::PollFd)) as *mut posix::PollFd
    var idx = 0u
    it = e.head
    while(it != null) {
        pfd[idx].fd = it.fd
        pfd[idx].events = it.events
        pfd[idx].revents = 0
        it = it.next
        idx = idx + 1u
    }
    e.m.unlock()
    var rc = posix::reactor_platform_poll(pfd, n as ulong, timeout_ms)
    if(rc > 0) {
        e.m.lock()
        idx = 0u
        it = e.head
        while(it != null) {
            var rev = pfd[idx].revents
            if((rev & (posix::POLLIN | posix::POLLERR | posix::POLLHUP)) != 0 && it.read_waker.vtbl != null) {
                var w = it.read_waker.clone()
                w.wake()
            }
            if((rev & posix::POLLOUT) != 0 && it.write_waker.vtbl != null) {
                var w2 = it.write_waker.clone()
                w2.wake()
            }
            it = it.next
            idx = idx + 1u
        }
        e.m.unlock()
    }
    unsafe {
        dealloc pfd
    }
    return rc
}

// An awaitable readiness future for a raw fd.
public struct ReadyState {
    var fd : int
    var events : short
    var parked : bool
    var vtbl : *mut core::async::FutureTable<core::async::Unit>
}

@retained
public func ready_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<core::async::Unit> {
    var st = frame as *mut ReadyState
    // Register and yield once before reporting readiness. Awaiting a future that
    // is `Ready` on the very first poll currently mis-lowers on LLVM (a coroutine
    // that never suspends is destroyed through an `unreachable` cleanup block —
    // B22). One `Pending` pass also matches how `yield_now` behaves.
    if(st.parked && posix::reactor_is_ready(st.fd, st.events)) {
        return core::async::Poll.Ready<core::async::Unit>(core::async::Unit { })
    }
    st.parked = true
    if(cx.waker.vtbl != null) {
        reactor_register(st.fd, st.events, cx.waker.clone())
    }
    return core::async::Poll.Pending<core::async::Unit>()
}

@retained
public func ready_drop(frame : *mut void) {
    var st = frame as *mut ReadyState
    var vtbl = st.vtbl
    unsafe {
        delete st
    }
    if(vtbl != null) {
        unsafe {
            dealloc vtbl
        }
    }
}

func make_ready(fd : int, events : short) : core::async::FutureHandle<core::async::Unit> {
    var st = malloc(sizeof(ReadyState)) as *mut ReadyState
    var vtbl = malloc(sizeof(core::async::FutureTable<core::async::Unit>)) as *mut core::async::FutureTable<core::async::Unit>
    vtbl.poll = ready_poll
    vtbl.drop = ready_drop
    new(st) ReadyState {
        fd : fd,
        events : events,
        parked : false,
        vtbl : vtbl
    }
    return core::async::FutureHandle<core::async::Unit> { frame : st as *mut void, vtbl : vtbl }
}

// A registered file descriptor. `readable`/`writable` complete when the fd is
// ready for that operation; the fd is never closed by `AsyncFd`.
public struct AsyncFd {
    var fd : int

    public func readable(&self) : core::async::FutureHandle<core::async::Unit> {
        return make_ready(self.fd, posix::POLLIN)
    }

    public func writable(&self) : core::async::FutureHandle<core::async::Unit> {
        return make_ready(self.fd, posix::POLLOUT)
    }
}

public func readable(fd : int) : core::async::FutureHandle<core::async::Unit> {
    return make_ready(fd, posix::POLLIN)
}

public func writable(fd : int) : core::async::FutureHandle<core::async::Unit> {
    return make_ready(fd, posix::POLLOUT)
}

}
