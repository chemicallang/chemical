using namespace std;

@test
func test_safe_tcp_loopback(env : &mut TestEnv) {
    var listener_r = net::bind_tcp("127.0.0.1", 45934u16)
    if(listener_r is Result.Err) {
        env.error("bind_tcp failed")
        return
    }
    var Ok(listener) = listener_r else unsafe { unreachable }

    var client_r = net::connect_tcp("127.0.0.1", 45934u16)
    if(client_r is Result.Err) {
        env.error("connect_tcp failed")
        return
    }
    var Ok(client) = client_r else unsafe { unreachable }

    var server_r = listener.accept()
    if(server_r is Result.Err) {
        env.error("TcpListener.accept failed")
        return
    }
    var Ok(server) = server_r else unsafe { unreachable }

    var msg : [5]u8 = ['h', 'e', 'l', 'l', 'o']
    var msg_view : [u8] = msg
    var wr = client.write_all(msg_view)
    if(wr is Result.Err) {
        env.error("TcpStream.write_all failed")
        return
    }

    var buf : [16]u8 = [0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8]
    var buf_view : &mut [u8] = &mut buf
    var rr = server.read(buf_view)
    if(rr is Result.Err) {
        env.error("TcpStream.read failed")
        return
    }
    var Ok(n) = rr else unsafe { unreachable }
    if(n != 5) {
        env.error("TcpStream.read size mismatch")
    }
    listener.close()
    client.close()
    server.close()
}
