// ============================================================================
// HTTP matrix + stress tests (part 2) â€” redirects, methods, concurrency
// ============================================================================
// Ports used: 20435-20439.
// ============================================================================

using namespace tls
using namespace http
using std::Result
using std::string
using std::string_view

// Shared counter for concurrent matrix tests.
struct MtxCounter {
    var ok_plain : uint;
    var ok_tls : uint;
    var lock : std.mutex
}

func mtx2_url(port : uint, path : string_view) : string {
    var s = string("https://127.0.0.1:")
    s.append_uinteger(port as ubigint)
    s.append_view(&path)
    return s
}

// â”€â”€â”€ 6. Redirect flow: 301 Location header drives a second hop â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
@test
@test.timeout(60000)
public func MATRIX_redirect_location_header_flow(env : &mut TestEnv) {
    const PORT : uint = 20435u
    write_http_extra_py()
    xpy_kill_and_wait(PORT)
    xpy_gen_cert("mtx35")
    var cmd = xpy_server_cmd("redirsrv", "mtx35", PORT)
    cmd.append_view(" 2")
    xpy_bg(cmd.to_view())
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    // Hop 1: must observe 301 + Location without following anything.
    var u1 = mtx2_url(PORT, "/r1")
    var r1 = client.get(u1.to_view())
    if(r1 is Result.Err) { env.error("redirect source request failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    if(x1.status != 301u) { env.error("expected status 301") }
    var loc = x1.headers.get("Location")
    if(loc is std.Option.None) { env.error("Location header missing on 301"); test_kill_port(PORT as int); return }
    var Some(lv) = loc else unreachable;
    if(!lv.equals_view("/r2")) { env.error("Location value mismatch") }
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.None) { env.error("301 body read failed") } else {
        var Some(v1) = b1 else unreachable;
        if(v1.size() != 0u) { env.error("301 body should be empty (CL:0)") }
    }

    // Hop 2: manually follow to the target.
    var u2 = mtx2_url(PORT, "/r2")
    var r2 = client.get(u2.to_view())
    if(r2 is Result.Err) { env.error("redirect target request failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    if(x2.status != 200u) { env.error("target expected 200") }
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("target body read failed"); test_kill_port(PORT as int); return }
    var Some(v2) = b2 else unreachable;
    if(!v2.equals_view("final-target")) { env.error("target body mismatch") }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 7. OPTIONS method + URL fragment stripping, live â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// The builder supports arbitrary methods; fragments must never reach the wire.
@test
@test.timeout(60000)
public func MATRIX_options_method_and_fragment_stripping(env : &mut TestEnv) {
    const PORT : uint = 20436u
    write_http_extra_py()
    xpy_kill_and_wait(PORT)
    xpy_gen_cert("mtx36")
    var cmd = xpy_server_cmd("pathsrv", "mtx36", PORT)
    cmd.append_view(" 2")
    xpy_bg(cmd.to_view())
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    // OPTIONS via the generic builder path.
    var base1 = string("https://127.0.0.1:20436/opts")
    var uo = URL::parse(base1.to_view())
    if(uo is std::Option.None) { env.error("options url parse failed"); test_kill_port(PORT as int); return }
    var Some(u1) = uo else unreachable;
    var rb = RequestBuilder("OPTIONS", std::replace(&mut u1, URL()))
    var r1 = client.request(&rb)
    if(r1 is Result.Err) { env.error("OPTIONS request failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.None) { env.error("OPTIONS body read failed"); test_kill_port(PORT as int); return }
    var Some(v1) = b1 else unreachable;
    if(!v1.equals_view("OPTIONS /opts")) { env.error("OPTIONS echo mismatch") }
    var xm = x1.headers.get("X-Echo-Method")
    if(xm is std.Option.None) { env.error("X-Echo-Method missing") } else {
        var Some(mv) = xm else unreachable;
        if(!mv.equals_view("OPTIONS")) { env.error("X-Echo-Method value mismatch") }
    }

    // Fragment in the URL is stripped before the request line.
    var frag_url = string("https://127.0.0.1:20436/p#section-anchor")
    var r2 = client.get(frag_url.to_view())
    if(r2 is Result.Err) { env.error("fragment request failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("fragment body read failed"); test_kill_port(PORT as int); return }
    var Some(v2) = b2 else unreachable;
    if(!v2.equals_view("GET /p")) { env.error("fragment reached the server or path corrupted") }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 8. Concurrent mixed plain + TLS clients â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Two threads hammer a plain python http.server while two others hammer an
// HTTPS server; one Client instance per thread proves scheme dispatch stays
// stable under parallel load.
const MTX8_PLAIN_PORT : uint = 20437u
const MTX8_TLS_PORT : uint = 20438u

@test
@test.timeout(90000)
public func MATRIX_concurrent_plain_and_tls_clients(env : &mut TestEnv) {
    write_tls_python_utils()
    write_http_extra_py()
    test_ensure_tmp_dir()

    // Plain python file server (fully redirected so it never holds our stdio).
    xpy_force_kill_port(MTX8_PLAIN_PORT)
    test_server_wait()
    var plain_cmd = test_py_interp()
    plain_cmd.append_view("-m http.server 20437 --bind 127.0.0.1")
    var plain_redir = xpy_redir_all()
    plain_cmd.append_view(plain_redir.to_view())
    test_run_bg(plain_cmd.data())

    // Routed HTTPS server (tls_utils httpsrv, fully redirected).
    xpy_kill_and_wait(MTX8_TLS_PORT)
    xpy_gen_cert("mtx38")
    var srv_cert = xpy_cert_path("mtx38")
    var srv_key = xpy_key_path("mtx38")
    var cmd = test_py_interp()
    cmd.append_view("/tmp/tls_utils.py httpsrv ")
    cmd.append_view(srv_cert.to_view())
    cmd.append_view(" ")
    cmd.append_view(srv_key.to_view())
    cmd.append_view(" 20438 16")
    var tls_redir = xpy_redir_all()
    cmd.append_view(tls_redir.to_view())
    test_run_bg(cmd.data())
    test_server_wait()
    test_server_wait()
    test_server_wait()

    // Serial preflights with retries: prove both servers are answering before
    // the concurrent phase, so a readiness race can't masquerade as a bug.
    var attempt : uint = 0
    var plain_ok = false
    var tls_ok = false
    while(attempt < 5u && (!plain_ok || !tls_ok)) {
        if(!plain_ok) {
            var pre_plain = http::Client()
            var pu8 = string("http://127.0.0.1:20437/")
            var pp = pre_plain.get(pu8.to_view())
            if(pp is Result.Ok) { plain_ok = true }
        }
        if(!tls_ok) {
            var pre_tls = http::Client()
            pre_tls.insecure_skip_verify()
            var tu8 = string("https://127.0.0.1:20438/hello")
            var pt = pre_tls.get(tu8.to_view())
            if(pt is Result.Ok) { tls_ok = true }
        }
        if(!plain_ok || !tls_ok) { std::concurrent::sleep_ms(1000u) }
        attempt += 1u
    }

    if(!plain_ok || !tls_ok) {
        if(!plain_ok) { env.error("preflight: plain http.server not reachable after retries") }
        if(!tls_ok) { env.error("preflight: https server not reachable after retries") }
        xpy_force_kill_port(MTX8_PLAIN_PORT)
        xpy_force_kill_port(MTX8_TLS_PORT)
        return
    }

    var cap : MtxCounter = {
        ok_plain = 0u
        ok_tls = 0u
        lock = std.mutex()
    }

    // Plain worker template â€” two instances.
    var t1 = std.concurrent.spawn(||(arg : *void) => {
        var c = arg as *mut MtxCounter
        var client = http::Client()
        var u = string("http://127.0.0.1:20437/")
        for(var i=0u; i<5u; i++) {
            var res = client.get(u.to_view())
            if(res is Result.Ok) {
                var Ok(rr) = res else unreachable;
                if(rr.status == 200u) {
                    c.lock.lock()
                    c.ok_plain = c.ok_plain + 1u
                    c.lock.unlock()
                }
            }
        }
        return null
    }, &raw mut cap)

    var t2 = std.concurrent.spawn(||(arg : *void) => {
        var c = arg as *mut MtxCounter
        var client = http::Client()
        var u = string("http://127.0.0.1:20437/")
        for(var i=0u; i<5u; i++) {
            var res = client.get(u.to_view())
            if(res is Result.Ok) {
                var Ok(rr) = res else unreachable;
                if(rr.status == 200u) {
                    c.lock.lock()
                    c.ok_plain = c.ok_plain + 1u
                    c.lock.unlock()
                }
            }
        }
        return null
    }, &raw mut cap)

    // TLS workers â€” two instances.
    var t3 = std.concurrent.spawn(||(arg : *void) => {
        var c = arg as *mut MtxCounter
        var client = http::Client()
        client.insecure_skip_verify()
        var u = string("https://127.0.0.1:20438/hello")
        for(var i=0u; i<5u; i++) {
            var res = client.get(u.to_view())
            if(res is Result.Ok) {
                var Ok(rr) = res else unreachable;
                if(rr.status == 200u) {
                    c.lock.lock()
                    c.ok_tls = c.ok_tls + 1u
                    c.lock.unlock()
                }
            }
        }
        return null
    }, &raw mut cap)

    var t4 = std.concurrent.spawn(||(arg : *void) => {
        var c = arg as *mut MtxCounter
        var client = http::Client()
        client.insecure_skip_verify()
        var u = string("https://127.0.0.1:20438/hello")
        for(var i=0u; i<5u; i++) {
            var res = client.get(u.to_view())
            if(res is Result.Ok) {
                var Ok(rr) = res else unreachable;
                if(rr.status == 200u) {
                    c.lock.lock()
                    c.ok_tls = c.ok_tls + 1u
                    c.lock.unlock()
                }
            }
        }
        return null
    }, &raw mut cap)

    t1.join()
    t2.join()
    t3.join()
    t4.join()

    if(cap.ok_plain != 10u || cap.ok_tls != 10u) {
        env.error("concurrent mixed traffic: expected 10 plain + 10 TLS successes")
        var msg = string("plain=")
        msg.append_uinteger(cap.ok_plain as ubigint)
        msg.append_view(" tls=")
        msg.append_uinteger(cap.ok_tls as ubigint)
        env.error(msg.data())
    }

    xpy_force_kill_port(MTX8_PLAIN_PORT)
    xpy_force_kill_port(MTX8_TLS_PORT)
}

// â”€â”€â”€ 9. Parallel upload + download streams through the same server â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct MtxUpDown {
    var fails : uint;
    var lock : std.mutex
}

@test
@test.timeout(90000)
public func MATRIX_parallel_upload_download_streams(env : &mut TestEnv) {
    const PORT : uint = 20439u
    xpy_force_kill_port(PORT)
    int_start_httpsrv(env, PORT, "/tmp/tls_mtx39_cert.pem", "/tmp/tls_mtx39_key.pem")

    var st : MtxUpDown = {
        fails = 0u
        lock = std.mutex()
    }

    // Uploader: three 32 KB POST /echo roundtrips verified byte-for-byte.
    var up = std.concurrent.spawn(||(arg : *void) => {
        var s = arg as *mut MtxUpDown
        var client = http::Client()
        client.insecure_skip_verify()
        var phase : usize = 9u
        for(var k=0u; k<3u; k++) {
            var payload = full_pattern(32768u, phase)
            phase += 11u
            var u = string("https://127.0.0.1:20439/echo")
            var res = client.post(u.to_view(), payload.to_view(), "application/octet-stream")
            if(res is Result.Err) {
                s.lock.lock(); s.fails = s.fails + 1u; s.lock.unlock()
                continue
            }
            var Ok(r) = res else unreachable;
            var b = r.body.read_to_string()
            if(b is std.Option.None) {
                s.lock.lock(); s.fails = s.fails + 1u; s.lock.unlock()
                continue
            }
            var Some(v) = b else unreachable;
            if(!v.equals_view(payload.to_view())) {
                s.lock.lock(); s.fails = s.fails + 1u; s.lock.unlock()
            }
        }
        return null
    }, &raw mut st)

    // Downloader: three 64 KB GETs verified by length + pattern spot checks.
    var down = std.concurrent.spawn(||(arg : *void) => {
        var s = arg as *mut MtxUpDown
        var client = http::Client()
        client.insecure_skip_verify()
        for(var k=0u; k<3u; k++) {
            var u = string("https://127.0.0.1:20439/size/65536")
            var res = client.get(u.to_view())
            if(res is Result.Err) {
                s.lock.lock(); s.fails = s.fails + 1u; s.lock.unlock()
                continue
            }
            var Ok(r) = res else unreachable;
            var b = r.body.read_to_string()
            if(b is std.Option.None) {
                s.lock.lock(); s.fails = s.fails + 1u; s.lock.unlock()
                continue
            }
            var Some(v) = b else unreachable;
            if(v.size() != 65536u) {
                s.lock.lock(); s.fails = s.fails + 1u; s.lock.unlock()
                continue
            }
            if((v.get(65535u) as u8) != ((65535u % 251) as u8)) {
                s.lock.lock(); s.fails = s.fails + 1u; s.lock.unlock()
            }
        }
        return null
    }, &raw mut st)

    up.join()
    down.join()

    if(st.fails != 0u) {
        env.error("parallel up/down stream failures occurred")
        var msg = string("fails=")
        msg.append_uinteger(st.fails as ubigint)
        env.error(msg.data())
    }

    test_kill_port(PORT as int)
}
