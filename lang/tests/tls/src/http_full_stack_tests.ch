// ============================================================================
// Full-Stack HTTP Integration: every test begins with an HTTP request
// ============================================================================
// Each test drives complete HTTP request/response cycles through the ENTIRE
// stack at once â€” URL parsing, RequestBuilder serialization, TLS handshake +
// record layer, net socket I/O, and response parsing â€” against a live Python
// HTTPS server (or the reverse direction: a real Python http.client against a
// Chemical TLS server). One test exercises http + tls + net together.
//
// Client-direction tests use Client.insecure_skip_verify() with self-signed
// fixtures; chain tests use Client.set_ca_chain() for full verification.
// Ports used: 20300-20315, 20320.
// ============================================================================

using namespace tls
using namespace http
using std::Result
using std::string
using std::string_view

// Start the routed python HTTPS server with an explicit connection budget.
func full_start_httpsrv(env : &mut TestEnv, port : uint, cert_path : string_view,
                        key_path : string_view, nconn : uint) {
    write_tls_python_utils()
    test_kill_port(port as int)
    test_server_wait()
    var gen = string()
    gen.append_view("cert ")
    gen.append_view(&cert_path)
    gen.append_view(" ")
    gen.append_view(&key_path)
    gen.append_view(" localhost ec")
    test_py_run_foreground(gen.to_view())
    var cmd = string()
    cmd.append_view("httpsrv ")
    cmd.append_view(&cert_path)
    cmd.append_view(" ")
    cmd.append_view(&key_path)
    cmd.append_view(" ")
    cmd.append_uinteger(port as ubigint)
    cmd.append_view(" ")
    cmd.append_uinteger(nconn as ubigint)
    test_py_run_background(cmd.to_view())
    test_server_wait()
}

// Deterministic i%251 pattern payload.
func full_pattern(len : usize, phase : usize) : string {
    var s = string()
    var i : usize = 0
    while(i < len) {
        s.append(((i + phase) % 251) as char)
        i += 1
    }
    return s
}

// â”€â”€â”€ 1. Full CRUD lifecycle over one server, one client instance â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// GET â†’ POST â†’ PUT â†’ PATCH â†’ DELETE â†’ HEAD in sequence. Every request is a
// fresh URL parse + TLS handshake + HTTP exchange; one Client drives all six.
@test
@test.timeout(60000)
public func FULL_https_crud_lifecycle_single_server(env : &mut TestEnv) {
    const PORT : uint = 20300u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs00_cert.pem", "/tmp/tls_fs00_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    // GET /hello â€” status + body + custom response header
    var full_url1 = int_url(PORT, "/hello")
    var r1 = client.get(full_url1.to_view())
    if(r1 is Result.Err) { env.error("GET failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    if(x1.status != 200u) { env.error("GET expected 200") }
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.None) { env.error("GET body read failed"); test_kill_port(PORT as int); return }
    var Some(v1) = b1 else unreachable;
    if(!v1.equals_view("world")) { env.error("GET body mismatch") }
    var cust = x1.headers.get("X-Custom")
    if(cust is std.Option.None) { env.error("X-Custom header missing") }

    // POST /echo â€” 4 KiB pattern echoed byte-for-byte, content type propagated
    var payload = full_pattern(4096u, 3u)
    var full_url2 = int_url(PORT, "/echo")
    var r2 = client.post(full_url2.to_view(), payload.to_view(), "application/octet-stream")
    if(r2 is Result.Err) { env.error("POST failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    if(x2.status != 200u) { env.error("POST expected 200") }
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("POST body read failed"); test_kill_port(PORT as int); return }
    var Some(v2) = b2 else unreachable;
    if(!v2.equals_view(payload.to_view())) { env.error("POST echo mismatch") }
    var ct = x2.headers.get("X-Content-Type")
    if(ct is std.Option.None) { env.error("X-Content-Type missing") } else {
        var Some(ctv) = ct else unreachable;
        if(!ctv.equals_view("application/octet-stream")) { env.error("content type not propagated") }
    }

    // PUT /put
    var full_url3 = int_url(PORT, "/put")
    var r3 = client.put(full_url3.to_view(), string_view("data1"), "text/plain")
    if(r3 is Result.Err) { env.error("PUT failed"); test_kill_port(PORT as int); return }
    var Ok(x3) = r3 else unreachable;
    var b3 = x3.body.read_to_string()
    if(b3 is std.Option.None) { env.error("PUT body read failed"); test_kill_port(PORT as int); return }
    var Some(v3) = b3 else unreachable;
    if(!v3.equals_view("put:data1")) { env.error("PUT echo mismatch") }

    // PATCH /patch
    var full_url4 = int_url(PORT, "/patch")
    var r4 = client.patch(full_url4.to_view(), string_view("{}"), "application/json")
    if(r4 is Result.Err) { env.error("PATCH failed"); test_kill_port(PORT as int); return }
    var Ok(x4) = r4 else unreachable;
    if(x4.status != 200u) { env.error("PATCH expected 200") }

    // DELETE /del
    var full_url5 = int_url(PORT, "/del")
    var r5 = client.delete(full_url5.to_view())
    if(r5 is Result.Err) { env.error("DELETE failed"); test_kill_port(PORT as int); return }
    var Ok(x5) = r5 else unreachable;
    var b5 = x5.body.read_to_string()
    if(b5 is std.Option.None) { env.error("DELETE body read failed"); test_kill_port(PORT as int); return }
    var Some(v5) = b5 else unreachable;
    if(!v5.equals_view("deleted")) { env.error("DELETE body mismatch") }

    // HEAD /hello â€” headers must arrive even without reading a body
    var full_url6 = int_url(PORT, "/hello")
    var r6 = client.head(full_url6.to_view())
    if(r6 is Result.Err) { env.error("HEAD failed"); test_kill_port(PORT as int); return }
    var Ok(x6) = r6 else unreachable;
    if(x6.status != 200u) { env.error("HEAD expected 200") }
    if(x6.headers.get("X-Custom") is std.Option.None) { env.error("HEAD lost X-Custom header") }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 2. Binary upload/download roundtrip through TLS records â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// POST a 32 KiB binary pattern (upload fragmented across TLS records), then
// GET a 64 KiB download and verify every single byte.
@test
@test.timeout(60000)
public func FULL_https_binary_roundtrip_upload_download(env : &mut TestEnv) {
    const PORT : uint = 20301u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs01_cert.pem", "/tmp/tls_fs01_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    var upload = full_pattern(32768u, 11u)
    var full_url7 = int_url(PORT, "/echo")
    var r1 = client.post(full_url7.to_view(), upload.to_view(), "application/octet-stream")
    if(r1 is Result.Err) { env.error("binary upload failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.None) { env.error("upload body read failed"); test_kill_port(PORT as int); return }
    var Some(v1) = b1 else unreachable;
    if(v1.size() != 32768u) { env.error("upload echo length mismatch") }
    if(!v1.equals_view(upload.to_view())) { env.error("upload echo bytes mismatch") }

    var full_url8 = int_url(PORT, "/size/65536")
    var r2 = client.get(full_url8.to_view())
    if(r2 is Result.Err) { env.error("binary download failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("download body read failed"); test_kill_port(PORT as int); return }
    var Some(v2) = b2 else unreachable;
    if(v2.size() != 65536u) { env.error("download length mismatch") }
    var di : size_t = 0
    while(di < v2.size()) {
        if(v2.get(di) != ((di % 251) as char)) { env.error("download pattern mismatch"); break }
        di += 1
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 3. Routing matrix: all python-server routes against one instance â”€â”€â”€â”€â”€â”€â”€
// Status codes, query echo, header echo and chunked responses â€” six different
// routes exercised back-to-back on a single server process.
@test
@test.timeout(60000)
public func FULL_https_routing_matrix_all_routes(env : &mut TestEnv) {
    const PORT : uint = 20302u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs02_cert.pem", "/tmp/tls_fs02_key.pem", 10u)

    var client = Client()
    client.insecure_skip_verify()

    var full_url9 = int_url(PORT, "/status/201")
    var r1 = client.get(full_url9.to_view())
    if(r1 is Result.Err) { env.error("status/201 failed") } else {
        var Ok(x) = r1 else unreachable;
        if(x.status != 201u) { env.error("expected 201") }
    }

    var full_url10 = int_url(PORT, "/status/404")
    var r2 = client.get(full_url10.to_view())
    if(r2 is Result.Err) { env.error("status/404 failed") } else {
        var Ok(x) = r2 else unreachable;
        if(x.status != 404u) { env.error("expected 404") }
    }

    var full_url11 = int_url(PORT, "/status/500")
    var r3 = client.get(full_url11.to_view())
    if(r3 is Result.Err) { env.error("status/500 failed") } else {
        var Ok(x) = r3 else unreachable;
        if(x.status != 500u) { env.error("expected 500") }
    }

    var base_q = string("https://127.0.0.1:")
    base_q.append_uinteger(PORT as ubigint)
    base_q.append_view("/query")
    var uq = URL::parse(base_q.to_view())
    if(uq is std::Option.None) { env.error("query url parse failed"); test_kill_port(PORT as int); return }
    var Some(uxq) = uq else unreachable;
    var rb = RequestBuilder("GET", std::replace(&mut uxq, URL()))
    rb.query("a", "1")
    rb.query("b", "two")
    var rq = client.request(&rb)
    if(rq is Result.Err) { env.error("query request failed") } else {
        var Ok(x) = rq else unreachable;
        var bq = x.body.read_to_string()
        if(bq is std.Option.None) { env.error("query body read failed") } else {
            var Some(vq) = bq else unreachable;
            if(!vq.equals_view("a=1&b=two")) { env.error("query echo mismatch") }
        }
    }

    var base_h = string("https://127.0.0.1:")
    base_h.append_uinteger(PORT as ubigint)
    base_h.append_view("/hdrs")
    var uh = URL::parse(base_h.to_view())
    if(uh is std::Option.None) { env.error("hdrs url parse failed"); test_kill_port(PORT as int); return }
    var Some(uxh) = uh else unreachable;
    var rbh = RequestBuilder("GET", std::replace(&mut uxh, URL()))
    rbh.header("X-Test", "matrix-value")
    var rh = client.request(&rbh)
    if(rh is Result.Err) { env.error("hdrs request failed") } else {
        var Ok(x) = rh else unreachable;
        var bh = x.body.read_to_string()
        if(bh is std.Option.None) { env.error("hdrs body read failed") } else {
            var Some(vh) = bh else unreachable;
            if(!vh.contains("matrix-value")) { env.error("custom header did not reach server") }
            if(!vh.contains("chemical-client")) { env.error("default User-Agent missing") }
        }
    }

    var full_url12 = int_url(PORT, "/chunked")
    var rc = client.get(full_url12.to_view())
    if(rc is Result.Err) { env.error("chunked route failed") } else {
        var Ok(x) = rc else unreachable;
        if(!x.body.is_chunked()) { env.error("chunked flag not set") }
        var bc = x.body.read_to_string()
        if(bc is std.Option.None) { env.error("chunked body read failed") } else {
            var Some(vc) = bc else unreachable;
            if(!vc.equals_view("chunked-body-data")) { env.error("chunk reassembly mismatch") }
        }
    }

    test_kill_port(PORT as int)
}

// Shared counter for concurrent-client tests.
struct FullCapCounter {
    var success : uint;
    var lock : std.mutex
}

// â”€â”€â”€ 4. Four concurrent clients, parallel TLS handshakes, shared server â”€â”€â”€â”€â”€
@test
@test.timeout(90000)
public func FULL_https_concurrent_clients_parallel_handshakes(env : &mut TestEnv) {
    const PORT : uint = 20303u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs03_cert.pem", "/tmp/tls_fs03_key.pem", 26u)

    var cap : FullCapCounter = {
        success = 0u
        lock = std.mutex()
    }

    var t1 = std.concurrent.spawn(||(arg : *void) => {
        var c = arg as *mut FullCapCounter
        var client = http::Client()
        client.insecure_skip_verify()
        var u = string("https://127.0.0.1:20303/hello")
        for(var i=0u; i<6u; i++) {
            var res = client.get(u.to_view())
            if(res is Result.Ok) {
                var Ok(rr) = res else unreachable;
                if(rr.status == 200u) {
                    c.lock.lock()
                    c.success = c.success + 1u
                    c.lock.unlock()
                }
            }
        }
        return null
    }, &raw mut cap)

    var t2 = std.concurrent.spawn(||(arg : *void) => {
        var c = arg as *mut FullCapCounter
        var client = http::Client()
        client.insecure_skip_verify()
        var u = string("https://127.0.0.1:20303/hello")
        for(var i=0u; i<6u; i++) {
            var res = client.get(u.to_view())
            if(res is Result.Ok) {
                var Ok(rr) = res else unreachable;
                if(rr.status == 200u) {
                    c.lock.lock()
                    c.success = c.success + 1u
                    c.lock.unlock()
                }
            }
        }
        return null
    }, &raw mut cap)

    var t3 = std.concurrent.spawn(||(arg : *void) => {
        var c = arg as *mut FullCapCounter
        var client = http::Client()
        client.insecure_skip_verify()
        var u = string("https://127.0.0.1:20303/hello")
        for(var i=0u; i<6u; i++) {
            var res = client.get(u.to_view())
            if(res is Result.Ok) {
                var Ok(rr) = res else unreachable;
                if(rr.status == 200u) {
                    c.lock.lock()
                    c.success = c.success + 1u
                    c.lock.unlock()
                }
            }
        }
        return null
    }, &raw mut cap)

    var t4 = std.concurrent.spawn(||(arg : *void) => {
        var c = arg as *mut FullCapCounter
        var client = http::Client()
        client.insecure_skip_verify()
        var u = string("https://127.0.0.1:20303/hello")
        for(var i=0u; i<6u; i++) {
            var res = client.get(u.to_view())
            if(res is Result.Ok) {
                var Ok(rr) = res else unreachable;
                if(rr.status == 200u) {
                    c.lock.lock()
                    c.success = c.success + 1u
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

    if(cap.success != 24u) {
        env.error("concurrent clients: expected 24 successes")
        var msg = string("got ")
        msg.append_uinteger(cap.success as ubigint)
        env.error(msg.data())
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 5. Large POST: 100 KiB echoed exactly across many TLS records â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
@test
@test.timeout(60000)
public func FULL_https_large_post_100k_echo_exact(env : &mut TestEnv) {
    const PORT : uint = 20304u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs04_cert.pem", "/tmp/tls_fs04_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    var payload = full_pattern(100000u, 17u)
    var full_url13 = int_url(PORT, "/echo")
    var res = client.post(full_url13.to_view(), payload.to_view(), "application/octet-stream")
    if(res is Result.Err) { env.error("large POST failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("large POST expected 200") }
    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("large POST body read failed"); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(v.size() != 100000u) {
        env.error("large POST echo length mismatch")
        var msg = string("got ")
        msg.append_uinteger(v.size() as ubigint)
        env.error(msg.data())
    } else if(!v.equals_view(payload.to_view())) {
        env.error("large POST echo bytes mismatch")
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 6. State flow: a value from one response feeds the next request â”€â”€â”€â”€â”€â”€â”€â”€
// Reads X-Custom from /hello, sends it as X-Test to /hdrs; the server echoes
// it back â€” proving header extraction and injection across requests works.
@test
@test.timeout(60000)
public func FULL_https_header_value_state_flow_between_requests(env : &mut TestEnv) {
    const PORT : uint = 20305u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs05_cert.pem", "/tmp/tls_fs05_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    var full_url14 = int_url(PORT, "/hello")
    var r1 = client.get(full_url14.to_view())
    if(r1 is Result.Err) { env.error("state flow: first GET failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    var cust_opt = x1.headers.get("X-Custom")
    if(cust_opt is std.Option.None) { env.error("state flow: X-Custom missing"); test_kill_port(PORT as int); return }
    var Some(token) = cust_opt else unreachable;

    var base = string("https://127.0.0.1:")
    base.append_uinteger(PORT as ubigint)
    base.append_view("/hdrs")
    var u_opt = URL::parse(base.to_view())
    if(u_opt is std::Option.None) { env.error("state flow: url parse failed"); test_kill_port(PORT as int); return }
    var Some(u) = u_opt else unreachable;
    var rb = RequestBuilder("GET", std::replace(&mut u, URL()))
    rb.header("X-Test", token.data())

    var r2 = client.request(&rb)
    if(r2 is Result.Err) { env.error("state flow: second request failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("state flow: echo read failed"); test_kill_port(PORT as int); return }
    var Some(echoed) = b2 else unreachable;
    if(!echoed.contains("chemical-test|")) {
        env.error("state flow: token did not round-trip through both requests")
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 7. UTF-8 and control characters survive the whole stack â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Multi-byte UTF-8 sequences plus tab/newline content posted to /echo and
// compared byte-for-byte after the TLS roundtrip.
@test
@test.timeout(60000)
public func FULL_https_utf8_and_control_char_bodies(env : &mut TestEnv) {
    const PORT : uint = 20306u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs06_cert.pem", "/tmp/tls_fs06_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    // "HeÃ© llo âœ“!!\n" assembled from explicit UTF-8 bytes.
    var utf8_bytes : [14]u8 = [
        0x48 as u8, 0x65 as u8, 0xC3 as u8, 0xA9 as u8,
        0x6C as u8, 0x6C as u8, 0x6F as u8, 0x20 as u8,
        0xE2 as u8, 0x9C as u8, 0x93 as u8,
        0x21 as u8, 0x21 as u8, 0x0A as u8
    ]
    var utf8_body = string()
    var ui : size_t = 0
    while(ui < 14u) { utf8_body.append(utf8_bytes[ui] as char); ui += 1 }

    var full_url15 = int_url(PORT, "/echo")
    var r1 = client.post(full_url15.to_view(), utf8_body.to_view(), "text/plain; charset=utf-8")
    if(r1 is Result.Err) { env.error("utf8 POST failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.None) { env.error("utf8 body read failed"); test_kill_port(PORT as int); return }
    var Some(v1) = b1 else unreachable;
    if(v1.size() != 14u) { env.error("utf8 echo length mismatch") }
    var ci : size_t = 0
    while(ci < v1.size() && ci < 14u) {
        if((v1.get(ci) as u8) != utf8_bytes[ci]) { env.error("utf8 echo bytes mismatch"); break }
        ci += 1
    }

    var ctrl = string::make_no_len("a\tb\nc d")
    var full_url16 = int_url(PORT, "/echo")
    var r2 = client.post(full_url16.to_view(), ctrl.to_view(), "text/plain")
    if(r2 is Result.Err) { env.error("control-char POST failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("control-char body read failed"); test_kill_port(PORT as int); return }
    var Some(v2) = b2 else unreachable;
    if(!v2.equals_view(ctrl.to_view())) { env.error("control-char echo mismatch") }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 8. Empty-body POST semantics over TLS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// A POST with no body omits Content-Length; the server still sees the
// Content-Type header and answers with its own empty body.
@test
@test.timeout(60000)
public func FULL_https_empty_body_post_semantics(env : &mut TestEnv) {
    const PORT : uint = 20307u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs07_cert.pem", "/tmp/tls_fs07_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    var full_url17 = int_url(PORT, "/echo")
    var res = client.post(full_url17.to_view(), string_view(""), "application/json")
    if(res is Result.Err) { env.error("empty-body POST failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("empty-body POST expected 200") }
    var ct = r.headers.get("X-Content-Type")
    if(ct is std.Option.None) { env.error("content type header missing on empty body") } else {
        var Some(cv) = ct else unreachable;
        if(!cv.equals_view("application/json")) { env.error("content type mismatch on empty body") }
    }
    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("empty body read failed"); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(v.size() != 0u) { env.error("expected empty echoed body") }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 9. Raw handcrafted responses: parser edge cases end-to-end â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// The python rawsrv serves three literal byte sequences:
//   1. "HTTP/1.1 200 \r\n..."      â€” empty reason phrase
//   2. lowercase headers, no space after colon
//   3. HTTP/1.0 + duplicate X-Multi headers
// Each flows through the TLS layer into the response parser.
@test
@test.timeout(60000)
public func FULL_https_raw_response_parsing_variants(env : &mut TestEnv) {
    const PORT : uint = 20308u
    write_tls_python_utils()
    test_kill_port(PORT as int)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_fs08_cert.pem /tmp/tls_fs08_key.pem localhost ec"))
    var cmd = string()
    cmd.append_view("rawsrv /tmp/tls_fs08_cert.pem /tmp/tls_fs08_key.pem ")
    cmd.append_uinteger(PORT as ubigint)
    cmd.append_view(" 3")
    test_py_run_background(cmd.to_view())
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()
    var url = int_url(PORT, "/anything")

    // Response 1: empty reason phrase.
    var r1 = client.get(url.to_view())
    if(r1 is Result.Err) { env.error("raw variant 1 failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    if(x1.status != 200u) { env.error("raw variant 1 expected status 200") }
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.None) { env.error("raw variant 1 body read failed"); test_kill_port(PORT as int); return }
    var Some(v1) = b1 else unreachable;
    if(!v1.equals_view("OK")) { env.error("raw variant 1 body mismatch") }

    // Response 2: lowercase header names, no space after colon.
    var r2 = client.get(url.to_view())
    if(r2 is Result.Err) { env.error("raw variant 2 failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    if(x2.status != 201u) { env.error("raw variant 2 expected status 201") }
    var low = x2.headers.get("X-Lower")
    if(low is std.Option.None) { env.error("lowercase header lookup failed") } else {
        var Some(lv) = low else unreachable;
        if(!lv.equals_view("yes")) { env.error("lowercase header value mismatch") }
    }
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("raw variant 2 body read failed"); test_kill_port(PORT as int); return }
    var Some(v2) = b2 else unreachable;
    if(!v2.equals_view("hi")) { env.error("raw variant 2 body mismatch") }

    // Response 3: HTTP/1.0 reply with duplicated X-Multi headers.
    var r3 = client.get(url.to_view())
    if(r3 is Result.Err) { env.error("raw variant 3 failed"); test_kill_port(PORT as int); return }
    var Ok(x3) = r3 else unreachable;
    if(x3.status != 404u) { env.error("raw variant 3 expected status 404") }
    var multi = x3.headers.get("X-Multi")
    if(multi is std.Option.None) { env.error("duplicate header lookup failed") } else {
        var Some(mv) = multi else unreachable;
        if(!mv.equals_view("a")) { env.error("duplicate header should yield first value") }
    }
    var b3 = x3.body.read_to_string()
    if(b3 is std.Option.None) { env.error("raw variant 3 body read failed"); test_kill_port(PORT as int); return }
    var Some(v3) = b3 else unreachable;
    if(!v3.equals_view("nope")) { env.error("raw variant 3 body mismatch") }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 10. Silent server: TLS read timeout produces a clean error â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// hangsrv completes the handshake then never responds; the client's TLS read
// timeout must surface as Result.Err instead of hanging forever.
@test
@test.timeout(90000)
public func FULL_https_silent_server_read_timeout(env : &mut TestEnv) {
    const PORT : uint = 20309u
    write_tls_python_utils()
    test_kill_port(PORT as int)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_fs09_cert.pem /tmp/tls_fs09_key.pem localhost ec"))
    var cmd = string()
    cmd.append_view("hangsrv /tmp/tls_fs09_cert.pem /tmp/tls_fs09_key.pem ")
    cmd.append_uinteger(PORT as ubigint)
    test_py_run_background(cmd.to_view())
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    var full_url18 = int_url(PORT, "/slow")
    var res = client.get(full_url18.to_view())
    if(res is Result.Ok) {
        env.error("silent server must produce an error (read timeout)")
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 11. Plain HTTP â†” HTTPS alternation through one client â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Three rounds of plain-then-TLS requests on a single Client instance prove
// scheme dispatch remains stable under repetition.
@test
@test.timeout(90000)
public func FULL_https_plain_tls_alternation_same_client(env : &mut TestEnv) {
    const PLAIN_PORT : uint = 20310u
    const TLS_PORT : uint = 20311u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(PLAIN_PORT as int)
    test_kill_port(TLS_PORT as int)
    test_server_wait()

    var plain_cmd = test_py_interp()
    plain_cmd.append_view("-m http.server ")
    plain_cmd.append_uinteger(PLAIN_PORT as ubigint)
    plain_cmd.append_view(" --bind 127.0.0.1")
    test_run_bg(plain_cmd.data())

    test_py_run_foreground(string_view("cert /tmp/tls_fs11_cert.pem /tmp/tls_fs11_key.pem localhost ec"))
    full_start_httpsrv(env, TLS_PORT, "/tmp/tls_fs11_cert.pem", "/tmp/tls_fs11_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    var plain_url = string("http://127.0.0.1:")
    plain_url.append_uinteger(PLAIN_PORT as ubigint)
    plain_url.append_view("/")

    var round : uint = 0
    while(round < 3u) {
        var rp = client.get(plain_url.to_view())
        if(rp is Result.Err) { env.error("plain alternation round failed"); break }
        var Ok(xp) = rp else unreachable;
        if(xp.status != 200u) { env.error("plain alternation expected 200") }

        var full_url19 = int_url(TLS_PORT, "/hello")
        var rt = client.get(full_url19.to_view())
        if(rt is Result.Err) { env.error("tls alternation round failed"); break }
        var Ok(xt) = rt else unreachable;
        if(xt.status != 200u) { env.error("tls alternation expected 200") }
        var bt = xt.body.read_to_string()
        if(bt is std.Option.None) { env.error("tls alternation body read failed"); break }
        var Some(vt) = bt else unreachable;
        if(!vt.equals_view("world")) { env.error("tls alternation body mismatch") }

        round += 1u
    }

    test_kill_port(PLAIN_PORT as int)
    test_kill_port(TLS_PORT as int)
}

// â”€â”€â”€ 12. Thirty sequential fresh TLS handshakes, all verified â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
@test
@test.timeout(120000)
public func FULL_https_thirty_sequential_handshakes(env : &mut TestEnv) {
    const PORT : uint = 20312u
    const ROUNDS : uint = 30u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs12_cert.pem", "/tmp/tls_fs12_key.pem", ROUNDS + 2u)

    var client = Client()
    client.insecure_skip_verify()

    var url = int_url(PORT, "/hello")
    var ok_count : uint = 0
    var i : uint = 0
    while(i < ROUNDS) {
        var res = client.get(url.to_view())
        if(res is Result.Ok) {
            var Ok(r) = res else unreachable;
            if(r.status == 200u) {
                var b = r.body.read_to_string()
                if(b is std.Option.Some) {
                    var Some(v) = b else unreachable;
                    if(v.equals_view("world")) { ok_count += 1u }
                }
            }
        }
        i += 1u
    }

    if(ok_count != ROUNDS) {
        env.error("sequential handshakes: some rounds failed")
        var msg = string("ok=")
        msg.append_uinteger(ok_count as ubigint)
        env.error(msg.data())
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 13. Two-level CA chain verification through the Client â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// root â†’ intermediate â†’ leaf; the client trusts only the root. Exercises
// chain building beyond a single self-signed certificate.
@test
@test.timeout(90000)
public func FULL_https_two_level_ca_chain_verification(env : &mut TestEnv) {
    const PORT : uint = 20313u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(PORT as int)
    test_server_wait()
    test_py_run_foreground(string_view("ca /tmp/tls_fsi13 localhost inter"))
    var srv_cmd = string()
    srv_cmd.append_view("httpsrv /tmp/tls_fsi13_chain.crt /tmp/tls_fsi13_leaf.key ")
    srv_cmd.append_uinteger(PORT as ubigint)
    srv_cmd.append_view(" 8")
    test_py_run_background(srv_cmd.to_view())
    test_server_wait()

    var root = x509_crt_load_pem_file("/tmp/tls_fsi13_root.pem")
    if(root == null) { env.error("could not load generated root"); test_kill_port(PORT as int); return }

    var client = Client()
    client.set_ca_chain(root)

    var res = client.get(string_view("https://localhost:20313/hello"))
    if(res is Result.Err) {
        env.error("verified request via two-level chain failed")
        cert_chain_free(root)
        test_kill_port(PORT as int)
        return
    }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("chain test expected 200") }
    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("chain test body read failed"); cert_chain_free(root); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(!v.equals_view("world")) { env.error("chain test body mismatch") }

    cert_chain_free(root)
    test_kill_port(PORT as int)
}

// â”€â”€â”€ 14. Streaming Body.read with small buffers over TLS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Pulls a 50 KB response through 1000-byte reads so individual TLS records
// are delivered piecemeal; verifies every byte positionally.
@test
@test.timeout(60000)
public func FULL_https_streaming_body_small_reads(env : &mut TestEnv) {
    const PORT : uint = 20314u
    const TOTAL : size_t = 50000u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs14_cert.pem", "/tmp/tls_fs14_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    var full_url20 = int_url(PORT, "/size/50000")
    var res = client.get(full_url20.to_view())
    if(res is Result.Err) { env.error("streaming request failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;

    var total : size_t = 0
    var bad = false
    var chunk : [1000]u8
    while(true) {
        var n = r.body.read(&raw mut chunk[0], 1000u)
        if(n < 0) { bad = true; break }
        if(n == 0) { break }
        var i : size_t = 0
        while(i < (n as size_t)) {
            if(chunk[i] != ((total % 251) as u8)) { bad = true; break }
            total += 1
            i += 1
        }
        if(bad) { break }
    }

    if(bad || total != TOTAL) {
        env.error("streaming read mismatch")
        var msg = string("total=")
        msg.append_uinteger(total as ubigint)
        env.error(msg.data())
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 15. Uppercase HTTPS scheme parsed and executed live â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
@test
@test.timeout(60000)
public func FULL_https_uppercase_scheme_end_to_end(env : &mut TestEnv) {
    const PORT : uint = 20315u
    full_start_httpsrv(env, PORT, "/tmp/tls_fs15_cert.pem", "/tmp/tls_fs15_key.pem", 8u)

    var client = Client()
    client.insecure_skip_verify()

    var upper = string("HTTPS://127.0.0.1:")
    upper.append_uinteger(PORT as ubigint)
    upper.append_view("/hello")

    var res = client.get(upper.to_view())
    if(res is Result.Err) { env.error("uppercase-scheme request failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("uppercase scheme expected 200") }
    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("uppercase scheme body read failed"); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(!v.equals_view("world")) { env.error("uppercase scheme body mismatch") }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 16. Reverse direction: Python http.client method matrix over TLS 1.3 â”€â”€â”€
// Parse Content-Length strictly inside the header block [0, head_end).
func full_parse_content_length(buf : *u8, head_end : size_t) : size_t {
    var i : size_t = 0
    while(i + 15 < head_end) {
        if(srv_ci_has_prefix(buf, i, "content-length:")) {
            var j = i + 15
            while(j < head_end && buf[j] == 32) { j += 1 }
            var val : size_t = 0
            while(j < head_end && buf[j] >= 48 && buf[j] <= 57) {
                val = val * 10 + (buf[j] - 48) as size_t
                j += 1
            }
            return val
        }
        i += 1
    }
    return 0
}

// Read one complete HTTP request; unlike srv_read_request_stream this parses
// Content-Length within the header block, not the body region.
func full_read_request(ssl : *mut SSLContext, buf : *mut u8, buf_size : size_t,
                       filled : *mut size_t, consumed : *mut size_t) : int {
    while(true) {
        var head_end = srv_mem_find_crlfcrlf(buf, 0, *filled)
        if(head_end != SRV_NPOS) {
            var clen = full_parse_content_length(buf, head_end)
            if(*filled >= head_end + 4 + clen) {
                *consumed = head_end + 4 + clen
                return 0
            }
        }
        if(*filled >= buf_size) { return -2 } // oversized request
        var n = ssl_read(ssl, buf + *filled, (buf_size - *filled) as i32)
        if(n <= 0) { return -1 }
        *filled = *filled + n as size_t
    }
    return -1
}

// A Chemical TLS server accepts five sequential connections from a real
// Python client â€” GET, POST, PUT, DELETE, PATCH â€” echoing bodies/methods;
// Python validates each status and payload byte-for-byte.
@test
@test.timeout(90000)
public func FULL_python_client_method_matrix_over_tls(env : &mut TestEnv) {
    const PORT : uint = 20320u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(PORT as int)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_fm20_cert.pem /tmp/tls_fm20_key.pem localhost ec"))
    test_py_run_foreground(string_view("privkey /tmp/tls_fm20_key.pem /tmp/tls_fm20_priv.hex"))

    var cert = x509_crt_load_pem_file("/tmp/tls_fm20_cert.pem")
    if(cert == null) { env.error("methods matrix: failed to load server cert"); return }

    var priv_key = ec_privkey_load_hex_file("/tmp/tls_fm20_priv.hex")
    if(priv_key == null) {
        cert_free(cert); unsafe { dealloc cert }
        env.error("methods matrix: failed to load private key"); return
    }

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) {
        cert_free(cert); unsafe { dealloc cert }
        ecdsa_context_free(priv_key)
        env.error("methods matrix: listen failed"); return
    }

    var bg = test_py_interp()
    bg.append_view("/tmp/tls_utils.py hcli 127.0.0.1 ")
    bg.append_uinteger(PORT as ubigint)
    bg.append_view(" /tmp/hcli_fm20.txt methods - -")
    bg.append_view(" 2>/tmp/fm20_err.txt")
    test_run_bg(bg.data())
    test_server_wait()

    var rounds : int = 0
    var served_all = true
    while(rounds < 5) {
        var cs = srv_accept_with_retry(ls)
        if(cs == 0 as net::Socket) {
            env.error("methods matrix: client never connected")
            var errbuf : [512]u8
            var en = test_read_file("/tmp/fm20_err.txt", &raw mut errbuf[0], 511)
            if(en > 0) { errbuf[en] = 0; printf("[FM20 py-stderr] %s\n", &raw errbuf[0]) }
            served_all = false
            break
        }

        var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
        ssl_init(ssl_mem)
        ssl_set_socket(ssl_mem, cs)
        var cfg = ssl_config_init(SSL_IS_SERVER)
        cfg.own_cert = cert
        cfg.own_key = priv_key as *mut void
        cfg.max_tls_version = SSL_VERSION_TLS1_3
        ssl_set_config(ssl_mem, &raw mut cfg)

        var hret = ssl_handshake(ssl_mem)
        if(hret < 0) {
            env.error("methods matrix: server handshake failed")
            var dmsg = string("methods matrix: ret=")
            dmsg.append_integer(hret)
            dmsg.append_view(" alert=")
            dmsg.append_uinteger(ssl_mem.last_alert_desc as ubigint)
            dmsg.append_view(" round=")
            dmsg.append_integer(rounds)
            env.error(dmsg.data())
            var errbuf2 : [512]u8
            var en2 = test_read_file("/tmp/fm20_err.txt", &raw mut errbuf2[0], 511)
            if(en2 > 0) { errbuf2[en2] = 0; printf("[FM20 py-stderr] %s\n", &raw errbuf2[0]) }
            ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
            net::close_socket(cs)
            served_all = false
            break
        }

        var req_buf : [16384]u8
        var filled : size_t = 0
        var consumed : size_t = 0
        var rret = full_read_request(ssl_mem, &raw mut req_buf[0], 16384u,
                                            &raw mut filled, &raw mut consumed)
        if(rret != 0) {
            env.error("methods matrix: request read failed")
            ssl_close_notify(ssl_mem)
            ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
            net::close_socket(cs)
            served_all = false
            break
        }

        // Method token = request bytes up to the first space.
        var sp : size_t = 0
        while(sp < consumed && req_buf[sp] != 32u) { sp += 1 }

        var head_end = srv_mem_find_crlfcrlf(&raw req_buf[0], 0, consumed)
        var body_start = head_end + 4
        var body_len = consumed - body_start

        // GET or DELETE â†’ respond "M:<METHOD>"; otherwise echo the body.
        var is_get = (sp == 3u && req_buf[0] == 71u && req_buf[1] == 69u && req_buf[2] == 84u)
        var is_delete = (sp == 6u && req_buf[0] == 68u && req_buf[1] == 69u && req_buf[2] == 76u &&
                         req_buf[3] == 69u && req_buf[4] == 84u && req_buf[5] == 69u)

        if(is_get || is_delete) {
            var resp : [32]u8
            resp[0] = 77u   // 'M'
            resp[1] = 58u   // ':'
            var mi : size_t = 0
            while(mi < sp && mi < 28u) { resp[2 + mi] = req_buf[mi]; mi += 1 }
            srv_send_response(ssl_mem, "HTTP/1.1 200 OK", &raw resp[0], 2 + mi, false)
        } else {
            srv_send_response(ssl_mem, "HTTP/1.1 200 OK", (&raw req_buf[0]) + body_start, body_len, false)
        }

        ssl_close_notify(ssl_mem)
        ssl_free(ssl_mem)
        unsafe { dealloc ssl_mem }
        net::close_socket(cs)
        rounds += 1
    }

    if(served_all && rounds == 5) {
        srv_check_result_file(env, "/tmp/hcli_fm20.txt")
    }

    cert_free(cert)
    unsafe { dealloc cert }
    ecdsa_context_free(priv_key)
    net::close_socket(ls)
    test_kill_port(PORT as int)
}
