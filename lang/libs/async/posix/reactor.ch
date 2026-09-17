// POSIX fd readiness. Uses `select(2)` rather than `poll(2)` because the `test`
// library already declares a different `poll` prototype and the C translation
// would reject two conflicting `poll` declarations in one program.
//
// The libc names live in `async::posix` so they do not collide with the
// Chemical-level `async::select<T>` combinator.
//
// This is the Tier 0 reactor scaffold: a registered fd + waker table that
// `executor_wait` waits on. The full epoll/kqueue registration is Tier 1.
public namespace async {
    @extern public func pipe(fds : *mut int) : int
}
public namespace async {
public namespace posix {

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

    public func reactor_is_ready(fd : int, events : short) : bool {
        var rfds : FdSet
        var wfds : FdSet
        memset(&raw mut rfds, 0, sizeof(FdSet))
        memset(&raw mut wfds, 0, sizeof(FdSet))
        if((events & POLLIN) != 0) {
            fdset_set(&raw mut rfds, fd)
        }
        if((events & POLLOUT) != 0) {
            fdset_set(&raw mut wfds, fd)
        }
        var zero : TimeVal
        zero.tv_sec = 0
        zero.tv_usec = 0
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

    public func reactor_platform_poll(fds : *mut PollFd, n : ulong, timeout_ms : int) : int {
        var rfds : FdSet
        var wfds : FdSet
        memset(&raw mut rfds, 0, sizeof(FdSet))
        memset(&raw mut wfds, 0, sizeof(FdSet))
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
        var tv : TimeVal
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

}
}
