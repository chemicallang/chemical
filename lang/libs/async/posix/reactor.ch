// POSIX fd readiness for the async executor.
//
// Linux uses `epoll(7)` and macOS/BSD use `kqueue(2)`: descriptors are
// registered once (from `reactor_register`, under the reactor lock) and the
// executor then blocks in `epoll_wait` / `kevent`. This removes the
// `FD_SETSIZE` (~1024) cap and the O(n) fd-set rebuild of a `select(2)` reactor.
// `select(2)` is kept as a portable fallback for other POSIX platforms.
//
// `reactor_platform_poll(fds, n, timeout_ms)` keeps its original signature: the
// generic layer still builds a snapshot of the registered fds, while epoll /
// kqueue track the registration internally. Ready `revents` are written back
// into that snapshot.
//
// The libc names live in `async::posix` so they do not collide with the
// Chemical-level `async::select<T>` combinator.
public namespace async {
    @extern public func pipe(fds : *mut int) : int
}
public namespace async {
public namespace posix {

    // ---- select(2) fallback ----
    @extern public func select(nfds : int, readfds : *mut FdSet, writefds : *mut FdSet, exceptfds : *mut FdSet, timeout : *mut TimeVal) : int

    public struct TimeVal {
        var tv_sec : long
        var tv_usec : long
    }

    // 1024 bits = 128 bytes, matching fd_set on Linux and macOS.
    public struct FdSet {
        var bits : [16]ulong
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

    // ---- epoll(7) (Linux) ----
    @extern public func epoll_create1(flags : int) : int
    @extern public func epoll_ctl(epfd : int, op : int, fd : int, event : *mut void) : int
    @extern public func epoll_wait(epfd : int, events : *mut void, maxevents : int, timeout : int) : int

    comptime const EPOLL_CTL_ADD : int = 1
    comptime const EPOLL_CTL_DEL : int = 2
    comptime const EPOLL_CTL_MOD : int = 3
    comptime const EPOLLIN : u32 = 0x001u
    comptime const EPOLLOUT : u32 = 0x004u
    comptime const EPOLLERR : u32 = 0x008u
    comptime const EPOLLHUP : u32 = 0x010u
    comptime const EPOLLRDHUP : u32 = 0x2000u

    // ---- kqueue(2) (macOS / FreeBSD) ----
    @extern public func kqueue() : int
    @extern public func kevent(kq : int, changelist : *mut void, nchanges : int, eventlist : *mut void, nevents : int, timeout : *mut KTimeSpec) : int

    comptime const EVFILT_READ : i16 = -1
    comptime const EVFILT_WRITE : i16 = -2
    comptime const EV_ADD : u16 = 0x0001u
    comptime const EV_DELETE : u16 = 0x0002u
    comptime const EV_EOF : u16 = 0x8000u
    comptime const EV_ERROR : u16 = 0x4000u

    public struct KEvent {
        var ident : ulong
        var filter : i16
        var flags : u16
        var fflags : u32
        var data : i64
        var udata : *mut void
    }

    public struct KTimeSpec {
        var tv_sec : long
        var tv_nsec : long
    }

    func fdset_set(set : *mut FdSet, fd : int) {
        var slot = fd / 64
        var bit = fd % 64
        set.bits[slot] = set.bits[slot] | ((1 as ulong) << (bit as ulong))
    }

    func fdset_isset(set : *mut FdSet, fd : int) : bool {
        var slot = fd / 64
        var bit = fd % 64
        return (set.bits[slot] & ((1 as ulong) << (bit as ulong))) != 0u
    }

    // ---- event bit translation ----
    func events_to_epoll(events : short) : u32 {
        var ep : u32 = 0u
        if((events & POLLIN) != 0) {
            ep = ep | EPOLLIN
        }
        if((events & POLLOUT) != 0) {
            ep = ep | EPOLLOUT
        }
        return ep
    }

    // Error/hangup must wake both readers and writers, so they are reported as
    // POLLIN too (a read will then fail/EOF) rather than leaving a writer parked.
    func epoll_to_revents(ep : u32) : short {
        var rev : short = 0
        if((ep & EPOLLIN) != 0u) {
            rev = rev | POLLIN
        }
        if((ep & EPOLLOUT) != 0u) {
            rev = rev | POLLOUT
        }
        if((ep & EPOLLERR) != 0u) {
            rev = rev | POLLERR | POLLIN | POLLOUT
        }
        if((ep & EPOLLHUP) != 0u) {
            rev = rev | POLLHUP | POLLIN | POLLOUT
        }
        if((ep & EPOLLRDHUP) != 0u) {
            rev = rev | POLLIN
        }
        return rev
    }

    // `struct epoll_event` is `__attribute__((packed))` on x86 (x86_64 / i386):
    // events at offset 0, the 8-byte data union at offset 4, size 12. Elsewhere
    // the union is naturally aligned at offset 8, size 16. We address the buffer
    // by byte offset so no struct-layout assumptions leak into codegen.
    func epoll_data_off() : size_t {
        comptime if(def.x86_64 || def.i386) {
            return 4u
        } else {
            return 8u
        }
    }

    func epoll_event_size() : size_t {
        comptime if(def.x86_64 || def.i386) {
            return 12u
        } else {
            return 16u
        }
    }

    func put_u32(base : *mut u8, off : size_t, v : u32) {
        var p = (base + off) as *mut u32
        *p = v
    }

    func get_u32(base : *mut u8, off : size_t) : u32 {
        var p = (base + off) as *mut u32
        return *p
    }

    func write_epoll_event(base : *mut u8, ev : u32, fd : int) {
        memset(base as *mut void, 0, epoll_event_size())
        put_u32(base, 0u, ev)
        put_u32(base, epoll_data_off(), fd as u32)
    }

    // ---- persistent kernel registration ----
    @never_destructed
    var g_epoll_fd : int = -1

    @never_destructed
    var g_kqueue_fd : int = -1

    // Created lazily under the reactor lock (only `reactor_platform_register`
    // calls this before any `reactor_platform_poll` sees a registered fd).
    func main_epoll_fd() : int {
        comptime if(def.linux) {
            if(g_epoll_fd < 0) {
                g_epoll_fd = epoll_create1(0)
            }
            return g_epoll_fd
        } else {
            return -1
        }
    }

    func main_kqueue_fd() : int {
        comptime if(def.macos || def.freebsd) {
            if(g_kqueue_fd < 0) {
                g_kqueue_fd = kqueue()
            }
            return g_kqueue_fd
        } else {
            return -1
        }
    }

    // Adds (or refreshes) a single filter on the persistent kqueue.
    func kqueue_add_one(kq : int, fd : int, filter : i16) {
        comptime if(def.macos || def.freebsd) {
            var chg = zeroed<KEvent>()
            chg.ident = fd as ulong
            chg.filter = filter
            chg.flags = EV_ADD
            var zero = zeroed<KTimeSpec>()
            kevent(kq, (&raw mut chg) as *mut void, 1, null, 0, &raw mut zero)
        }
    }

    func kqueue_apply(fd : int, events : short) {
        comptime if(def.macos || def.freebsd) {
            var kq = main_kqueue_fd()
            if(kq < 0) {
                return
            }
            if((events & POLLIN) != 0) {
                kqueue_add_one(kq, fd, EVFILT_READ)
            }
            if((events & POLLOUT) != 0) {
                kqueue_add_one(kq, fd, EVFILT_WRITE)
            }
        }
    }

    // Called from `reactor_register` while the reactor lock is held, so the
    // lazy fd creation and the kernel update are serialized.
    public func reactor_platform_register(fd : int, events : short, is_new : bool) {
        comptime if(def.linux) {
            var epfd = main_epoll_fd()
            if(epfd >= 0) {
                var buf = malloc(16) as *mut u8
                write_epoll_event(buf, events_to_epoll(events), fd)
                var op = EPOLL_CTL_MOD
                if(is_new) {
                    op = EPOLL_CTL_ADD
                }
                epoll_ctl(epfd, op, fd, buf as *mut void)
                unsafe {
                    dealloc buf
                }
            }
        } else {
            comptime if(def.macos || def.freebsd) {
                kqueue_apply(fd, events)
            }
        }
    }

    // ---- readiness probe (used by `ready_poll` before parking) ----
    func select_is_ready(fd : int, events : short) : bool {
        if(fd < 0) {
            return false
        }
        var rfds = zeroed<FdSet>()
        var wfds = zeroed<FdSet>()
        if((events & POLLIN) != 0) {
            fdset_set(&raw mut rfds, fd)
        }
        if((events & POLLOUT) != 0) {
            fdset_set(&raw mut wfds, fd)
        }
        var zero = zeroed<TimeVal>()
        var rc = select(fd + 1, &raw mut rfds, &raw mut wfds, null, &raw mut zero)
        if(rc <= 0) {
            return false
        }
        if((events & POLLIN) != 0 && fdset_isset(&raw mut rfds, fd)) {
            return true
        }
        if((events & POLLOUT) != 0 && fdset_isset(&raw mut wfds, fd)) {
            return true
        }
        return false
    }

    func epoll_is_ready(fd : int, events : short) : bool {
        comptime if(def.linux) {
            if(fd < 0) {
                return false
            }
            var epfd = epoll_create1(0)
            if(epfd < 0) {
                return select_is_ready(fd, events)
            }
            var buf = malloc(16) as *mut u8
            write_epoll_event(buf, events_to_epoll(events), fd)
            if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, buf as *mut void) != 0) {
                unsafe {
                    dealloc buf
                }
                close(epfd)
                return select_is_ready(fd, events)
            }
            var out = malloc(16) as *mut u8
            var rc = epoll_wait(epfd, out as *mut void, 1, 0)
            close(epfd)
            var ready = false
            if(rc > 0) {
                ready = epoll_to_revents(get_u32(out, 0u)) != 0
            }
            unsafe {
                dealloc buf
            }
            unsafe {
                dealloc out
            }
            return ready
        } else {
            return false
        }
    }

    // Adds a filter to a throwaway kqueue and reports whether it is ready now.
    func kqueue_probe_one(kq : int, fd : int, filter : i16) : bool {
        comptime if(def.macos || def.freebsd) {
            var chg = zeroed<KEvent>()
            chg.ident = fd as ulong
            chg.filter = filter
            chg.flags = EV_ADD
            var out = zeroed<KEvent>()
            var zero = zeroed<KTimeSpec>()
            var rc = kevent(kq, (&raw mut chg) as *mut void, 1, &raw mut out, 1, &raw mut zero)
            if(rc <= 0) {
                return false
            }
            if((out.flags & EV_ERROR) != 0u) {
                return false
            }
            return true
        } else {
            return false
        }
    }

    func kqueue_is_ready(fd : int, events : short) : bool {
        comptime if(def.macos || def.freebsd) {
            if(fd < 0 || events == 0) {
                return false
            }
            var kq = kqueue()
            if(kq < 0) {
                return select_is_ready(fd, events)
            }
            var ready = false
            if((events & POLLIN) != 0 && kqueue_probe_one(kq, fd, EVFILT_READ)) {
                ready = true
            }
            if(!ready && (events & POLLOUT) != 0 && kqueue_probe_one(kq, fd, EVFILT_WRITE)) {
                ready = true
            }
            close(kq)
            return ready
        } else {
            return false
        }
    }

    public func reactor_is_ready(fd : int, events : short) : bool {
        comptime if(def.linux) {
            return epoll_is_ready(fd, events)
        } else {
            comptime if(def.macos || def.freebsd) {
                return kqueue_is_ready(fd, events)
            } else {
                return select_is_ready(fd, events)
            }
        }
    }

    // ---- blocking wait ----
    func select_reactor_poll(fds : *mut PollFd, n : ulong, timeout_ms : int) : int {
        var rfds = zeroed<FdSet>()
        var wfds = zeroed<FdSet>()
        var maxfd = -1
        var i = 0u
        while(i < n) {
            var fd = fds[i].fd
            fds[i].revents = 0
            if((fds[i].events & POLLIN) != 0) {
                fdset_set(&raw mut rfds, fd)
            }
            if((fds[i].events & POLLOUT) != 0) {
                fdset_set(&raw mut wfds, fd)
            }
            if(fd > maxfd) {
                maxfd = fd
            }
            i = i + 1u
        }
        var tv = zeroed<TimeVal>()
        tv.tv_sec = (timeout_ms / 1000) as long
        tv.tv_usec = ((timeout_ms % 1000) * 1000) as long
        var rc = select(maxfd + 1, &raw mut rfds, &raw mut wfds, null, &raw mut tv)
        if(rc > 0) {
            i = 0u
            while(i < n) {
                var rev : short = 0
                var fd2 = fds[i].fd
                if(fdset_isset(&raw mut rfds, fd2)) {
                    rev = rev | POLLIN
                }
                if(fdset_isset(&raw mut wfds, fd2)) {
                    rev = rev | POLLOUT
                }
                fds[i].revents = rev
                i = i + 1u
            }
        }
        return rc
    }

    func epoll_reactor_poll(fds : *mut PollFd, n : ulong, timeout_ms : int) : int {
        comptime if(def.linux) {
            var epfd = main_epoll_fd()
            if(epfd < 0) {
                return select_reactor_poll(fds, n, timeout_ms)
            }
            if(n == 0u) {
                return 0
            }
            var esz = epoll_event_size()
            var buf = malloc(n * esz) as *mut u8
            var rc = epoll_wait(epfd, buf as *mut void, n as int, timeout_ms)
            if(rc > 0) {
                var i = 0u
                while(i < rc as ulong) {
                    var fd = get_u32(buf, i * esz + epoll_data_off()) as int
                    var rev = epoll_to_revents(get_u32(buf, i * esz))
                    var j = 0u
                    while(j < n) {
                        if(fds[j].fd == fd) {
                            fds[j].revents = rev
                            break
                        }
                        j = j + 1u
                    }
                    i = i + 1u
                }
            }
            unsafe {
                dealloc buf
            }
            return rc
        } else {
            return -1
        }
    }

    func kqueue_reactor_poll(fds : *mut PollFd, n : ulong, timeout_ms : int) : int {
        comptime if(def.macos || def.freebsd) {
            var kq = main_kqueue_fd()
            if(kq < 0) {
                return select_reactor_poll(fds, n, timeout_ms)
            }
            if(n == 0u) {
                return 0
            }
            var events = malloc(n * sizeof(KEvent)) as *mut KEvent
            var ts = zeroed<KTimeSpec>()
            ts.tv_sec = (timeout_ms / 1000) as long
            ts.tv_nsec = ((timeout_ms % 1000) * 1000000) as long
            var rc = kevent(kq, null, 0, events as *mut void, n as int, &raw mut ts)
            if(rc > 0) {
                var i = 0u
                while(i < rc as ulong) {
                    var fd = events[i].ident as int
                    var rev : short = 0
                    if(events[i].filter == EVFILT_READ) {
                        rev = rev | POLLIN
                    }
                    if(events[i].filter == EVFILT_WRITE) {
                        rev = rev | POLLOUT
                    }
                    if((events[i].flags & EV_EOF) != 0u) {
                        rev = rev | POLLHUP | POLLIN
                    }
                    if((events[i].flags & EV_ERROR) != 0u) {
                        rev = rev | POLLERR
                    }
                    var j = 0u
                    while(j < n) {
                        if(fds[j].fd == fd) {
                            fds[j].revents = rev
                            break
                        }
                        j = j + 1u
                    }
                    i = i + 1u
                }
            }
            unsafe {
                dealloc events
            }
            return rc
        } else {
            return -1
        }
    }

    public func reactor_platform_poll(fds : *mut PollFd, n : ulong, timeout_ms : int) : int {
        comptime if(def.linux) {
            return epoll_reactor_poll(fds, n, timeout_ms)
        } else {
            comptime if(def.macos || def.freebsd) {
                return kqueue_reactor_poll(fds, n, timeout_ms)
            } else {
                return select_reactor_poll(fds, n, timeout_ms)
            }
        }
    }

}
}
