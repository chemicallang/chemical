// Tier 0 channel tests. Run under `./scripts/test.sh --tcc --libs` and
// `--llvm --libs`.
using namespace std;

async func chan_pair_sum() : int {
    var ch = async::channel<int>()
    ch.sender.send(40)
    ch.sender.send(2)
    ch.sender.close()
    var a = await ch.receiver.recv_or(-1)
    var b = await ch.receiver.recv_or(-1)
    var c = await ch.receiver.recv_or(-1)
    return a + b + c
}

async func chan_worker_sum() : int {
    var ch = async::channel<int>()
    var s2 = ch.sender.clone_sender()
    var s3 = ch.sender.clone_sender()
    var s4 = ch.sender.clone_sender()
    var h1 = async::spawn_blocking<bool>(|s2|() => s2.send(1))
    var h2 = async::spawn_blocking<bool>(|s3|() => s3.send(2))
    var h3 = async::spawn_blocking<bool>(|s4|() => s4.send(3))
    var total = 0
    var i = 0
    while(i < 3) {
        total = total + await ch.receiver.recv_or(0)
        i = i + 1
    }
    var u1 = await h1
    var u2 = await h2
    var u3 = await h3
    return total
}

async func chan_string_roundtrip() : size_t {
    var ch = async::channel<std::string>()
    ch.sender.send(std::string("hello"))
    ch.sender.send(std::string("world"))
    ch.sender.close()
    var a = await ch.receiver.recv_or(std::string())
    var b = await ch.receiver.recv_or(std::string())
    return a.size() + b.size()
}

@test
func test_async_channel_basic(env : &mut TestEnv) {
    var v = async::block_on<int>(chan_pair_sum())
    if(v != 41) {
        env.error("channel basic sum mismatch")
    }
}

@test
func test_async_channel_try_recv(env : &mut TestEnv) {
    var ch = async::channel<int>()
    ch.sender.send(9)
    var a = ch.receiver.try_recv()
    if(a is std::Option.None) {
        env.error("try_recv should have returned a value")
        return
    }
    var Some(v) = a else unreachable
    if(v != 9) {
        env.error("try_recv value mismatch")
    }
    if(!(ch.receiver.try_recv() is std::Option.None)) {
        env.error("try_recv on an empty channel should be None")
    }
}

@test
func test_async_channel_closed_fallback(env : &mut TestEnv) {
    var ch = async::channel<int>()
    ch.sender.close()
    var v = async::block_on<int>(ch.receiver.recv_or(-7))
    if(v != -7) {
        env.error("recv_or should return the fallback once closed")
    }
}

@test
func test_async_channel_worker_send(env : &mut TestEnv) {
    var v = async::block_on<int>(chan_worker_sum())
    if(v != 6) {
        env.error("channel worker sum mismatch")
    }
}

@test
func test_async_channel_destructible_payload(env : &mut TestEnv) {
    var v = async::block_on<size_t>(chan_string_roundtrip())
    if(v != 10u) {
        env.error("channel string payload size mismatch")
    }
}
