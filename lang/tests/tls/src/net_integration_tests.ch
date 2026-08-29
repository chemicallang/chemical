// ============================================================================
// Net-layer integration tests — raw TCP behavior beneath http/tls
// ============================================================================
// These exercise net:: primitives directly (send_all loops, partial recv,
// hostname resolution) since every HTTP/TLS exchange above depends on them.
//
// Ports used: 20450-20452.
// ============================================================================

using namespace std;
using namespace net;

const NET_TOTAL : usize = 1048576u  // 1 MB

func ntx_fill_pattern(buf : *mut u8, len : usize) {
    var i : usize = 0
    while(i < len) {
        buf[i] = (i % 256) as u8
        i += 1
    }
}

// ─── 1. send_all delivers a full megabyte intact across loopback TCP ─────────
// send_all must loop over partial sends; the receiver accumulates until EOF
// and verifies every byte.
@test
@test.timeout(60000)
public func NET_send_all_megabyte_integrity(env : &mut TestEnv) {
    const PORT : uint = 20450u

    var ls = listen_addr("127.0.0.1", PORT)
    if(ls == 0 as Socket) { env.error("listen failed"); return }

    var sender = std.concurrent.spawn(||(arg : *void) => {
        var sock = dial("127.0.0.1", 20450u)
        if(sock == 0 as Socket) { return null }
        var buf = malloc(1048576u) as *mut u8
        if(buf == null) { net::close_socket(sock); return null }
        ntx_fill_pattern(buf, 1048576u)
        var off : usize = 0
        while(off < 1048576u) {
            var n = send_all(sock, (buf + off) as *char, (1048576u - off) as int)
            if(n <= 0) { break }
            off += (n as usize)
        }
        unsafe { dealloc buf }
        close_socket(sock)
        return null
    }, null)

    std::concurrent::sleep_ms(200u)
    var cs = accept_socket(ls)
    if(cs == 0 as Socket) {
        env.error("accept failed")
        close_socket(ls)
        sender.join()
        return
    }

    set_recv_timeout(cs, 10, 0)
    var acc = malloc(1048576u) as *mut u8
    if(acc == null) { env.error("alloc failed"); close_socket(cs); close_socket(ls); sender.join(); return }

    var total : usize = 0
    var bad = false
    while(total < 1048576u) {
        var n = recv_all(cs, acc + total, 1048576u - total)
        if(n <= 0) { break }
        total += (n as usize)
    }
    if(total != 1048576u) {
        bad = true
        var m = string("received ")
        m.append_uinteger(total as ubigint)
        env.error(m.data())
    } else {
        var i : usize = 0
        while(i < total) {
            if(acc[i] != (i % 256) as u8) { bad = true; env.error("pattern mismatch"); break }
            i += 1
        }
    }

    unsafe { dealloc acc }
    close_socket(cs)
    close_socket(ls)
    sender.join()
    if(bad) { env.error("megabyte transfer incomplete or corrupted") }
}

// ─── 2. recv returns only what arrived when the peer closes early ────────────
// Sender writes 1000 bytes then closes; a single recv_all call may legitimately
// return fewer bytes. The data returned must be a correct prefix.
@test
@test.timeout(60000)
public func NET_recv_partial_on_early_peer_close(env : &mut TestEnv) {
    const PORT : uint = 20451u
    const SEND_LEN : usize = 1000u

    var ls = listen_addr("127.0.0.1", PORT)
    if(ls == 0 as Socket) { env.error("listen failed"); return }

    var sender = std.concurrent.spawn(||(arg : *void) => {
        var sock = dial("127.0.0.1", 20451u)
        if(sock == 0 as Socket) { return null }
        var buf = malloc(1000u) as *mut u8
        if(buf == null) { net::close_socket(sock); return null }
        ntx_fill_pattern(buf, 1000u)
        send_all(sock, buf as *char, 1000)
        unsafe { dealloc buf }
        close_socket(sock)
        return null
    }, null)

    std::concurrent::sleep_ms(200u)
    var cs = accept_socket(ls)
    if(cs == 0 as Socket) {
        env.error("accept failed")
        close_socket(ls)
        sender.join()
        return
    }

    set_recv_timeout(cs, 3, 0)
    var buf : [4096]u8
    var n = recv_all(cs, &raw mut buf[0], 4096)

    if(n <= 0) {
        env.error("no data received before close")
    } else if((n as usize) > SEND_LEN) {
        env.error("received more than the peer ever sent")
    } else {
        var i : usize = 0
        while(i < (n as usize)) {
            if(buf[i] != (i % 256) as u8) { env.error("prefix data corrupted"); break }
            i += 1
        }
    }

    close_socket(cs)
    close_socket(ls)
    sender.join()
}

// ─── 3. dial() resolves "localhost" through getaddrinfo ──────────────────────
// HTTP clients pass hostnames straight to dial; prove the resolution path
// works for names (not just literal IPs).
@test
@test.timeout(60000)
public func NET_dial_resolves_localhost_hostname(env : &mut TestEnv) {
    const PORT : uint = 20452u

    var ls = listen_addr("127.0.0.1", PORT)
    if(ls == 0 as Socket) { env.error("listen failed"); return }

    var connector = std.concurrent.spawn(||(arg : *void) => {
        var sock = dial("localhost", 20452u)
        if(sock == 0 as Socket) { return null }
        var msg = "ping-hostname\0"
        send_all(sock, msg as *char, 13)
        close_socket(sock)
        return null
    }, null)

    std::concurrent::sleep_ms(200u)
    var cs = accept_socket(ls)
    if(cs == 0 as Socket) {
        env.error("hostname dial never connected")
        close_socket(ls)
        connector.join()
        return
    }

    set_recv_timeout(cs, 3, 0)
    var buf : [32]u8
    var n = recv_all(cs, &raw mut buf[0], 31)

    var ok = true
    if(n != 13) { ok = false } else {
        var expect = "ping-hostname"
        var i : usize = 0
        while(i < 13u) {
            if(buf[i] != (expect[i] as u8)) { ok = false; break }
            i += 1
        }
    }

    close_socket(cs)
    close_socket(ls)
    connector.join()

    if(!ok) { env.error("dial(localhost) echo mismatch") }
}
