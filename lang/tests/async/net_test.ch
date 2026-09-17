// Tier 1 `net` async socket tests (loopback).
//
// These are not hermetic (they bind a local port), which is why they live in
// the dedicated `--async` suite rather than `--libs` (see the integration doc,
// §8). Run with `./scripts/test.sh --tcc --async` / `--llvm --async`.
using namespace std;

async func net_echo_once(listener_fd : int, expect : int) : int {
    var server_fd = await net::accept_async(listener_fd)
    if(server_fd <= 0) {
        return -1
    }
    var server = net::async_socket(server_fd)
    var buf : [64]char
    var got = await server.read(&raw mut buf[0] as *mut u8, expect as usize)
    if(got != expect) {
        server.close()
        return -2
    }
    var sent = await server.write(&raw mut buf[0] as *char, got)
    server.close()
    return sent
}

@test
func test_net_async_loopback(env : &mut TestEnv) {
    var port = 45933u
    var listener = net::async_listener("127.0.0.1", port)
    if(!listener.valid()) {
        env.error("listener creation failed")
        return
    }

    var server_f = async::spawn<int>(net_echo_once(listener.raw(), 9))
    var client_f = net::dial_async("127.0.0.1", port)

    var client_fd = async::block_on<int>(client_f)
    if(client_fd <= 0) {
        env.error("dial failed")
        listener.close()
        return
    }
    var client = net::async_socket(client_fd)

    var msg = "async-net"
    var sent = async::block_on<int>(client.write(msg, 9))
    if(sent != 9) {
        env.error("client send failed")
    }

    var buf : [16]char
    var got = async::block_on<int>(client.read(&raw mut buf[0] as *mut u8, 9u))
    if(got != 9) {
        env.error("client recv failed")
    } else {
        buf[9] = '\0'
        if(strcmp(&raw mut buf[0] as *char, "async-net") != 0) {
            env.error("echo payload mismatch")
        }
    }

    var server_sent = async::block_on<int>(server_f)
    if(server_sent != 9) {
        env.error("server echo failed")
    }

    client.close()
    listener.close()
}
