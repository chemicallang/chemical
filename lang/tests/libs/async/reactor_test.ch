// Tier 0 fd reactor tests (`readable`/`writable`/`AsyncFd`). Run under
// `./scripts/test.sh --tcc --libs` and `--llvm --libs`. POSIX uses `poll(2)`;
// the Windows stub reports readiness immediately, which the tests tolerate.
using namespace std;

func reactor_write_later(wfd : int) : core::async::Unit {
    std::concurrent.sleep_ms(50u)
    var c = 'x'
    write(wfd, &raw mut c as *void, 1u)
    return core::async::Unit { }
}

async func reactor_readable_roundtrip() : int {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        return -1
    }
    var rfd = fds[0]
    var wfd = fds[1]
    var h = async::spawn_blocking<core::async::Unit>(|wfd|() => reactor_write_later(wfd))
    var r0 = await async::readable(rfd)
    var buf : [8]char
    var n = read(rfd, &raw mut buf[0] as *mut void, 8u) as int
    var u = await h
    close(rfd)
    close(wfd)
    return n
}

async func reactor_writable() : int {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        return -1
    }
    var rfd = fds[0]
    var wfd = fds[1]
    var r1 = await async::writable(wfd)
    close(rfd)
    close(wfd)
    return 1
}

async func reactor_asyncfd_readable() : bool {
    var fds : [2]int
    if(async::pipe(&raw mut fds[0]) != 0) {
        return false
    }
    var afd = async::AsyncFd { fd : fds[0] }
    var wfd = fds[1]
    var h = async::spawn_blocking<core::async::Unit>(|wfd|() => reactor_write_later(wfd))
    var r2 = await afd.readable()
    var u = await h
    close(fds[0])
    close(fds[1])
    return true
}

@test
func test_async_reactor_readable(env : &mut TestEnv) {
    var n = async::block_on<int>(reactor_readable_roundtrip())
    if(n != 1) {
        env.error("reactor readable roundtrip mismatch")
    }
}

@test
func test_async_reactor_writable(env : &mut TestEnv) {
    var v = async::block_on<int>(reactor_writable())
    if(v != 1) {
        env.error("reactor writable mismatch")
    }
}

@test
func test_async_reactor_asyncfd(env : &mut TestEnv) {
    var b = async::block_on<bool>(reactor_asyncfd_readable())
    if(!b) {
        env.error("AsyncFd readable mismatch")
    }
}
