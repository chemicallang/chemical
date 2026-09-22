// Regression test for POSIX-EPOLL: the async fd reactor must work for
// descriptors whose number is >= FD_SETSIZE (1024).
//
// The old `select(2)` reactor built an fd_set with `fdset_set`, which indexed
// past the 16-word (1024-bit) set, and passed `maxfd + 1` (> FD_SETSIZE) to
// `select(2)`; a descriptor numbered 1100 could therefore never be reported
// ready (and the out-of-bounds write could corrupt memory). The epoll/kqueue
// reactors have no such cap.
//
// The test opens pipes until a read end lands above 1100. If the process fd
// limit is too low to reach that, it returns without failing.

func reactor_make_high_pipe(r : *mut int, w : *mut int) : bool {
    var i = 0
    while(i < 4000) {
        var fds : [2]int
        if(async::pipe(&raw mut fds[0]) != 0) {
            return false
        }
        if(fds[0] >= 1100) {
            *r = fds[0]
            *w = fds[1]
            return true
        }
        i = i + 1
    }
    return false
}

@test
public func test_posix_reactor_high_fd(env : &mut TestEnv) {
    var r = 0
    var w = 0
    if(!reactor_make_high_pipe(&raw mut r, &raw mut w)) {
        // fd limit too low to place a descriptor past FD_SETSIZE; skip.
        return
    }

    var ch : char = 'x'
    if(write(w, &raw mut ch as *void, 1u as ulong) != 1) {
        env.error("write to a high-fd pipe failed")
        return
    }

    // `ready_poll` (async::readable/writable) uses this readiness probe before
    // parking.
    if(!async::posix::reactor_is_ready(r, async::posix::POLLIN)) {
        env.error("reactor_is_ready missed a readable fd above FD_SETSIZE")
    }

    // The executor's blocking path registers the fd and waits in the reactor.
    var e = async::executor()
    var wk = async::exec_make_waker(e)
    async::reactor_register(r, async::posix::POLLIN, wk)
    if(async::reactor_poll(0) <= 0) {
        env.error("reactor_poll did not report a ready fd above FD_SETSIZE")
    }

    var got : char = 0
    if(read(r, &raw mut got as *mut void, 1u as ulong) != 1) {
        env.error("the high fd was not actually readable")
    }
}

// Registering the same fd for a second event set must *merge* the kernel
// registration (epoll MOD), not replace it. If the merge were lost, the read
// registration would disappear and this poll would report nothing.
@test
public func test_posix_reactor_mod_preserves_events(env : &mut TestEnv) {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        env.error("pipe failed")
        return
    }
    var r = fds[0]
    var w = fds[1]
    var e = async::executor()
    async::reactor_register(r, async::posix::POLLIN, async::exec_make_waker(e))
    // The node already exists, so this is a modification (POLLIN | POLLOUT).
    async::reactor_register(r, async::posix::POLLOUT, async::exec_make_waker(e))

    var ch : char = 'x'
    if(write(w, &raw mut ch as *void, 1u as ulong) != 1) {
        env.error("write failed")
        return
    }
    if(async::reactor_poll(0) <= 0) {
        env.error("reactor MOD must preserve the read registration")
    }
    read(r, &raw mut ch as *mut void, 1u as ulong)
    close(r)
    close(w)
}

// A closed peer must wake a parked reader (EPOLLHUP / EOF) instead of parking
// forever.
@test
public func test_posix_reactor_hup_wakes_reader(env : &mut TestEnv) {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        env.error("pipe failed")
        return
    }
    var r = fds[0]
    var w = fds[1]
    close(w)

    var e = async::executor()
    var wk = async::exec_make_waker(e)
    async::reactor_register(r, async::posix::POLLIN, wk)
    if(async::reactor_poll(0) <= 0) {
        env.error("a closed peer must wake the reader (HUP)")
    }
    close(r)
}

// End-to-end: `await readable(r)` must complete at EOF when the writer is gone.
async func reactor_readable_at_eof() : int {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        return -1
    }
    var r = fds[0]
    var w = fds[1]
    close(w)
    var u = await async::readable(r)
    var buf : [1]char
    var n = read(r, &raw mut buf[0] as *mut void, 1u) as int
    close(r)
    return n
}

@test
public func test_posix_reactor_readable_eof(env : &mut TestEnv) {
    var n = async::block_on<int>(reactor_readable_at_eof())
    if(n != 0) {
        env.error("readable on a closed pipe should complete and read 0 bytes")
    }
}

// Two independent fds registered at once: each readiness future must wake from
// its own descriptor.
async func reactor_two_pipes() : int {
    var a : [2]int
    var b : [2]int
    if(async::pipe(&raw mut a[0]) != 0) {
        return -1
    }
    if(async::pipe(&raw mut b[0]) != 0) {
        return -1
    }
    var wa = a[1]
    var wb = b[1]
    var h = async::spawn_blocking<core::async::Unit>(|wa, wb|() => {
        var c = 'x'
        write(wa, &raw mut c as *void, 1u as ulong)
        write(wb, &raw mut c as *void, 1u as ulong)
        return core::async::Unit { }
    })
    var ra = await async::readable(a[0])
    var rb = await async::readable(b[0])
    var buf : [1]char
    var na = read(a[0], &raw mut buf[0] as *mut void, 1u as ulong) as int
    var nb = read(b[0], &raw mut buf[0] as *mut void, 1u as ulong) as int
    var u = await h
    close(a[0])
    close(a[1])
    close(b[0])
    close(b[1])
    if(na == 1 && nb == 1) {
        return 1
    }
    return 0
}

@test
public func test_posix_reactor_two_pipes(env : &mut TestEnv) {
    if(async::block_on<int>(reactor_two_pipes()) != 1) {
        env.error("two registered fds must each wake their own reader")
    }
}
