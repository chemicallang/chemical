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
