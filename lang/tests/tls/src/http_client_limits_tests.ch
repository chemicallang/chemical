// ============================================================================
// HTTP Client limits and timeouts — full stack against live python servers
// ============================================================================
// Exercises the Client's own knobs (max_body_len, max_response_header_bytes,
// per-request timeout) plus robustness against servers that lie about
// Content-Length. Everything flows through http -> tls -> net.
//
// Ports used: 20420-20424.
// ============================================================================

using namespace tls
using namespace http
using std::Result
using std::string
using std::string_view

func lim_url(port : uint, path : string_view) : string {
    var s = string("https://127.0.0.1:")
    s.append_uinteger(port as ubigint)
    s.append_view(&path)
    return s
}

// ─── 1. max_body_len caps how much body the client will deliver ─────────────
// A 64 KB download with a 32 KB cap must stop delivering (read error) while a
// default client reads the same resource fully.
@test
@test.timeout(60000)
public func LIMIT_max_body_len_enforced(env : &mut TestEnv) {
    const PORT : uint = 20420u
    int_start_httpsrv(env, PORT, "/tmp/tls_lim20_cert.pem", "/tmp/tls_lim20_key.pem")

    var u = lim_url(PORT, "/size/65536")

    // Capped client: 64 KB requested, cap at 1 KB.
    var capped = Client()
    capped.insecure_skip_verify()
    capped.max_body_len = 1024u

    var r1 = capped.get(u.to_view())
    if(r1 is Result.Err) { env.error("capped request should still receive headers"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.Some) {
        env.error("body read must fail once max_body_len is exceeded")
    }

    // Control: a default client downloads the full resource.
    var full = Client()
    full.insecure_skip_verify()
    var r2 = full.get(u.to_view())
    if(r2 is Result.Err) { env.error("control download failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("control body read failed"); test_kill_port(PORT as int); return }
    var Some(v2) = b2 else unreachable;
    if(v2.size() != 65536u) {
        env.error("control download length mismatch")
        var msg = string("got ")
        msg.append_uinteger(v2.size() as ubigint)
        env.error(msg.data())
    }

    test_kill_port(PORT as int)
}

// ─── 2. Oversized response headers are rejected ──────────────────────────────
// Default cap is 64 KB of header bytes; an ~80 KB header block must produce a
// request error, while a normal response on another server succeeds.
@test
@test.timeout(90000)
public func LIMIT_max_response_header_bytes_enforced(env : &mut TestEnv) {
    const BIG_PORT : uint = 20421u
    const OK_PORT : uint = 20422u

    write_http_extra_py()

    // Big-header server: 1200 headers x ~56 bytes ≈ 67 KB of header block.
    xpy_kill_and_wait(BIG_PORT)
    xpy_gen_cert("lim21")
    var cmd_big = xpy_server_cmd("hdrsrv", "lim21", BIG_PORT)
    cmd_big.append_view(" 1 1200 48")
    xpy_bg(cmd_big.to_view())

    // Normal server for the control.
    xpy_kill_and_wait(OK_PORT)
    xpy_gen_cert("lim22")
    var cmd_ok = xpy_server_cmd("hdrsrv", "lim22", OK_PORT)
    cmd_ok.append_view(" 2 8 8")
    xpy_bg(cmd_ok.to_view())
    test_server_wait()
    test_server_wait()
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    var ub = lim_url(BIG_PORT, "/big")
    var r1 = client.get(ub.to_view())
    if(r1 is Result.Ok) {
        env.error("oversized response header block must be rejected")
    }

    var uo = lim_url(OK_PORT, "/ok")
    var r2 = client.get(uo.to_view())
    if(r2 is Result.Err) {
        // Server startup can lag under load; give it one more chance.
        std::concurrent::sleep_ms(1500u)
        r2 = client.get(uo.to_view())
    }
    if(r2 is Result.Err) { env.error("normal-size response failed"); test_kill_port(OK_PORT as int); test_kill_port(BIG_PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    if(x2.status != 200u) { env.error("control expected 200") }
    var h = x2.headers.get("X-H-3")
    if(h is std.Option.None) { env.error("control header missing") } else {
        var Some(hv) = h else unreachable;
        if(!hv.equals_view("vvvvvvvv")) { env.error("control header value mismatch") }
    }

    test_kill_port(BIG_PORT as int)
    test_kill_port(OK_PORT as int)
}

// ─── 3. Per-request timeout plumbing: short fails fast, long succeeds ────────
// The python slowsrv sleeps 3 seconds before answering. timeout(1) must give
// up during the header wait; timeout(8) must complete successfully.
@test
@test.timeout(60000)
public func LIMIT_request_timeout_short_fails_long_succeeds(env : &mut TestEnv) {
    const PORT : uint = 20423u
    write_http_extra_py()
    xpy_kill_and_wait(PORT)
    xpy_gen_cert("lim23")
    var cmd = xpy_server_cmd("slowsrv", "lim23", PORT)
    cmd.append_view(" 3 2")
    xpy_bg(cmd.to_view())
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    var base = string("https://127.0.0.1:20423/slow")

    // Impatient request — must time out.
    var u_opt1 = URL::parse(base.to_view())
    if(u_opt1 is std::Option.None) { env.error("url parse failed"); test_kill_port(PORT as int); return }
    var Some(u1) = u_opt1 else unreachable;
    var rb1 = RequestBuilder("GET", std::replace(&mut u1, URL()))
    rb1.timeout(1)
    var r1 = client.request(&rb1)
    if(r1 is Result.Ok) {
        env.error("1s timeout against a 3s server must fail")
    }

    // Patient request — must succeed.
    var u_opt2 = URL::parse(base.to_view())
    if(u_opt2 is std::Option.None) { env.error("url parse failed"); test_kill_port(PORT as int); return }
    var Some(u2) = u_opt2 else unreachable;
    var rb2 = RequestBuilder("GET", std::replace(&mut u2, URL()))
    rb2.timeout(10)
    var r2 = client.request(&rb2)
    if(r2 is Result.Err) { env.error("10s timeout should tolerate the 3s delay"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    if(x2.status != 200u) { env.error("slow request expected 200") }
    var b = x2.body.read_to_string()
    if(b is std.Option.None) { env.error("slow request body read failed"); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(!v.equals_view("late-ok")) { env.error("slow request body mismatch") }

    test_kill_port(PORT as int)
}

// ─── 4. Server declares Content-Length but closes early → read error ─────────
// The response parses fine (headers arrive), but read_to_string must surface
// the truncation instead of silently returning short data.
@test
@test.timeout(60000)
public func LIMIT_truncated_body_detected(env : &mut TestEnv) {
    const PORT : uint = 20424u
    write_http_extra_py()
    xpy_kill_and_wait(PORT)
    xpy_gen_cert("lim24")
    var cmd = xpy_server_cmd("liessrv", "lim24", PORT)
    cmd.append_view(" 1")
    xpy_bg(cmd.to_view())
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    var u = lim_url(PORT, "/truncated")
    var res = client.post(u.to_view(), string_view("0123456789ABCDEF"), "text/plain")
    if(res is Result.Err) { env.error("truncated request should parse headers"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("expected 200 status despite later truncation") }

    var b = r.body.read_to_string()
    if(b is std.Option.Some) {
        var Some(v) = b else unreachable;
        if(v.size() >= 1000u) {
            env.error("server only sent 400 bytes; full 1000 cannot have arrived")
        } else {
            env.error("truncation must be reported as a read failure, not short data")
        }
    }

    test_kill_port(PORT as int)
}
