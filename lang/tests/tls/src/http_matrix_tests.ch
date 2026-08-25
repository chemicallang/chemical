// ============================================================================
// HTTP matrix + stress tests (part 1) â€” sizes, byte cycles, headers, queries
// ============================================================================
// Each test drives complete HTTP request/response cycles through the whole
// stack (http::Client -> TLS -> net) against live python servers. Sizes,
// byte patterns are chosen to stress boundary conditions: record
// fragmentation, buffer growth and signed-char handling.
//
// Ports used: 20430-20434.
// ============================================================================

using namespace tls
using namespace http
using std::Result
using std::string
using std::string_view
using std::vector

func mtx_url(port : uint, path : string_view) : string {
    var s = string("https://127.0.0.1:")
    s.append_uinteger(port as ubigint)
    s.append_view(&path)
    return s
}

// i%256 full byte-cycle pattern (differs from the %251 helpers elsewhere).
func mtx_pattern256(len : usize) : string {
    var s = string()
    var i : usize = 0
    while(i < len) {
        s.append((i % 256) as char)
        i += 1
    }
    return s
}

// â”€â”€â”€ 1. Download size boundary matrix â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// 1, 1023, 16383, 16384 (max TLS plaintext), 16385, 65536 â€” each must arrive
// complete with the exact i%251 pattern.
@test
@test.timeout(90000)
public func MATRIX_download_size_boundaries(env : &mut TestEnv) {
    const PORT : uint = 20430u
    xpy_force_kill_port(PORT)
    int_start_httpsrv(env, PORT, "/tmp/tls_mtx30_cert.pem", "/tmp/tls_mtx30_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var sizes = vector<uint>()
    sizes.push_back(1u)
    sizes.push_back(1023u)
    sizes.push_back(16383u)
    sizes.push_back(16384u)
    sizes.push_back(16385u)
    sizes.push_back(65536u)

    var si : size_t = 0
    while(si < sizes.size()) {
        var n = sizes.get(si)
        var path = string("/size/")
        path.append_uinteger(n as ubigint)
        var u = mtx_url(PORT, path.to_view())
        var res = client.get(u.to_view())
        if(res is Result.Err) {
            env.error("download failed")
            var m = string("size=")
            m.append_uinteger(n as ubigint)
            env.error(m.data())
            test_kill_port(PORT as int)
            return
        }
        var Ok(r) = res else unreachable;
        if(r.status != 200u) { env.error("download expected 200") }
        var b = r.body.read_to_string()
        if(b is std.Option.None) {
            env.error("download body read failed")
            var m2 = string("size=")
            m2.append_uinteger(n as ubigint)
            env.error(m2.data())
            test_kill_port(PORT as int)
            return
        }
        var Some(v) = b else unreachable;
        if(v.size() != (n as usize)) {
            env.error("download length mismatch")
            var m3 = string("size=")
            m3.append_uinteger(n as ubigint)
            m3.append_view(" got ")
            m3.append_uinteger(v.size() as ubigint)
            env.error(m3.data())
            test_kill_port(PORT as int)
            return
        }
        // Spot-check the pattern at start/middle/end.
        if((v.get(0) as u8) != ((0 % 251) as u8)) { env.error("pattern[0] mismatch") }
        var mid = v.size() / 2u
        if((v.get(mid) as u8) != ((mid % 251) as u8)) { env.error("pattern[mid] mismatch") }
        var last = v.size() - 1u
        if((v.get(last) as u8) != ((last % 251) as u8)) { env.error("pattern[last] mismatch") }
        si += 1
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 2. Upload size boundary matrix (echo roundtrips) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
@test
@test.timeout(90000)
public func MATRIX_upload_size_boundaries(env : &mut TestEnv) {
    const PORT : uint = 20431u
    xpy_force_kill_port(PORT)
    int_start_httpsrv(env, PORT, "/tmp/tls_mtx31_cert.pem", "/tmp/tls_mtx31_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var u0 = mtx_url(PORT, "/echo")
    var r0 = client.post(u0.to_view(), string_view(""), "text/plain")
    if(r0 is Result.Err) { env.error("empty POST failed"); test_kill_port(PORT as int); return }
    var Ok(x0) = r0 else unreachable;
    var b0 = x0.body.read_to_string()
    if(b0 is std.Option.None) { env.error("empty POST body read failed"); test_kill_port(PORT as int); return }
    var Some(v0) = b0 else unreachable;
    if(v0.size() != 0u) { env.error("empty POST should echo empty") }

    var sizes = vector<usize>()
    sizes.push_back(1u)
    sizes.push_back(1024u)
    sizes.push_back(16384u)
    sizes.push_back(16385u)

    var si : size_t = 0
    var phase : usize = 5u
    while(si < sizes.size()) {
        var n = sizes.get(si)
        var payload = full_pattern(n, phase)
        phase += 3u
        var u = mtx_url(PORT, "/echo")
        var res = client.post(u.to_view(), payload.to_view(), "application/octet-stream")
        if(res is Result.Err) {
            env.error("upload failed")
            var m = string("size=")
            m.append_uinteger(n as ubigint)
            env.error(m.data())
            test_kill_port(PORT as int)
            return
        }
        var Ok(r) = res else unreachable;
        var b = r.body.read_to_string()
        if(b is std.Option.None) {
            env.error("upload echo read failed")
            var m2 = string("size=")
            m2.append_uinteger(n as ubigint)
            env.error(m2.data())
            test_kill_port(PORT as int)
            return
        }
        var Some(v) = b else unreachable;
        if(!v.equals_view(payload.to_view())) {
            env.error("upload echo mismatch")
            var m3 = string("size=")
            m3.append_uinteger(n as ubigint)
            env.error(m3.data())
            test_kill_port(PORT as int)
            return
        }
        si += 1
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 3. Full 0..255 byte cycle survives upload+echo over TLS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Catches signed-char / sign-extension bugs that the usual %251 pattern misses.
@test
@test.timeout(60000)
public func MATRIX_full_byte_cycle_roundtrip(env : &mut TestEnv) {
    const PORT : uint = 20432u
    xpy_force_kill_port(PORT)
    int_start_httpsrv(env, PORT, "/tmp/tls_mtx32_cert.pem", "/tmp/tls_mtx32_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var payload = mtx_pattern256(65536u)
    var u = mtx_url(PORT, "/echo")
    var res = client.post(u.to_view(), payload.to_view(), "application/octet-stream")
    if(res is Result.Err) { env.error("byte-cycle POST failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;
    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("byte-cycle echo read failed"); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(v.size() != 65536u) { env.error("byte-cycle length mismatch") }
    var i : size_t = 0
    while(i < v.size()) {
        if((v.get(i) as u8) != ((i % 256) as u8)) {
            env.error("byte-cycle mismatch")
            var m = string("at offset ")
            m.append_uinteger(i as ubigint)
            env.error(m.data())
            break
        }
        i += 1
    }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 4. 120 response headers: presence + case-insensitive lookup â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
@test
@test.timeout(60000)
public func MATRIX_many_response_headers_lookup(env : &mut TestEnv) {
    const PORT : uint = 20433u
    write_http_extra_py()
    xpy_kill_and_wait(PORT)
    xpy_gen_cert("mtx33")
    var cmd = xpy_server_cmd("hdrsrv", "mtx33", PORT)
    cmd.append_view(" 2 120 8")
    xpy_bg(cmd.to_view())
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    var u = mtx_url(PORT, "/many")
    var res = client.get(u.to_view())
    if(res is Result.Err) {
        // Server startup can lag under load; give it one more chance.
        std::concurrent::sleep_ms(1500u)
        res = client.get(u.to_view())
    }
    if(res is Result.Err) { env.error("many-headers request failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;

    // Exact-name lookups at both ends plus the middle.
    var checks = vector<uint>()
    checks.push_back(0u)
    checks.push_back(60u)
    checks.push_back(119u)
    var ci : size_t = 0
    while(ci < checks.size()) {
        var nm = string("X-H-")
        nm.append_uinteger(checks.get(ci) as ubigint)
        var opt = r.headers.get(nm.data())
        if(opt is std.Option.None) { env.error("header missing from response") } else {
            var Some(hv) = opt else unreachable;
            if(!hv.equals_view("vvvvvvvv")) { env.error("header value mismatch") }
        }
        ci += 1
    }

    // Case-insensitive lookup variant.
    var low = r.headers.get("x-h-61")
    if(low is std.Option.None) { env.error("lowercase header lookup failed") }

    test_kill_port(PORT as int)
}

// â”€â”€â”€ 5. Very large query string (~8 KB) survives builder + TLS + parse â”€â”€â”€â”€â”€â”€
@test
@test.timeout(60000)
public func MATRIX_large_query_string_roundtrip(env : &mut TestEnv) {
    const PORT : uint = 20434u
    xpy_force_kill_port(PORT)
    int_start_httpsrv(env, PORT, "/tmp/tls_mtx34_cert.pem", "/tmp/tls_mtx34_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var big = string()
    var i : usize = 0
    while(i < 8000u) { big.append('x'); i += 1 }

    var base = string("https://127.0.0.1:20434/query")
    var u_opt = URL::parse(base.to_view())
    if(u_opt is std::Option.None) { env.error("url parse failed"); test_kill_port(PORT as int); return }
    var Some(u) = u_opt else unreachable;
    var rb = RequestBuilder("GET", std::replace(&mut u, URL()))
    rb.query("q", big.to_view())
    rb.query("tail", "end")

    var res = client.request(&rb)
    if(res is Result.Err) { env.error("large query request failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;

    var expected = string("q=")
    expected.append_string(&big)
    expected.append_view("&tail=end")

    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("large query body read failed"); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(!v.equals_view(expected.to_view())) {
        env.error("large query echo mismatch")
        var m = string("got len ")
        m.append_uinteger(v.size() as ubigint)
        m.append_view(" want len ")
        m.append_uinteger(expected.size() as ubigint)
        env.error(m.data())
    }

    test_kill_port(PORT as int)
}
