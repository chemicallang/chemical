// ============================================================================
// Full-Stack Integration: http::Client → TLS → net → Python HTTPS Server
// ============================================================================
// Every test drives a real HTTP request through the whole stack: URL parsing,
// RequestBuilder serialization, TLS handshake + record layer, socket I/O, and
// response parsing — against a live Python/OpenSSL HTTPS server.
//
// Success-path tests use either Client.insecure_skip_verify() (self-signed
// fixtures) or Client.set_ca_chain() (real chain + hostname verification).
// Ports used: 20200-20213.
// ============================================================================

using namespace tls
using namespace http
using std::Result
using std::string
using std::string_view

// Start the routed python HTTPS server used by most tests below.
func int_start_httpsrv(env : &mut TestEnv, port : uint, cert_path : string_view, key_path : string_view) {
    write_tls_python_utils()
    test_kill_port(port as int)
    test_server_wait()
    var cmd = string()
    cmd.append_view("httpsrv ")
    cmd.append_view(&cert_path)
    cmd.append_view(" ")
    cmd.append_view(&key_path)
    cmd.append_view(" ")
    cmd.append_uinteger(port as ubigint)
    cmd.append_view(" 8")
    test_py_run_background(cmd.to_view())
    test_server_wait()
}

func int_url(port : uint, path : string_view) : string {
    var s = string("https://127.0.0.1:")
    s.append_uinteger(port as ubigint)
    s.append_view(&path)
    return s
}

// ─── GET over TLS: status, body and response headers ────────────────────────
@test
@test.timeout(60000)
public func INT_https_get_200_body_headers(env : &mut TestEnv) {
    int_start_httpsrv(env, 20200u, "/tmp/tls_ih00_cert.pem", "/tmp/tls_ih00_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var url = int_url(20200u, "/hello")
    var res = client.get(url.to_view())
    if(res is Result.Err) {
        env.error("GET /hello over TLS failed")
        return
    }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("expected 200") }
    var body_opt = r.body.read_to_string()
    if(body_opt is std.Option.None) { env.error("body read failed"); return }
    var Some(body) = body_opt else unreachable;
    if(!body.equals_view("world")) { env.error("body mismatch") }

    var cust = r.headers.get("X-Custom")
    if(cust is std.Option.None) { env.error("X-Custom header missing") } else {
        var Some(cv) = cust else unreachable;
        if(!cv.equals_view("chemical-test")) { env.error("X-Custom value mismatch") }
    }

    test_kill_port(20200u)
}

// ─── POST over TLS: 8 KiB body echoed byte-for-byte ─────────────────────────
func int_pattern_string(len : usize, phase : uint) : string {
    var s = string()
    var i : usize = 0
    while(i < len) {
        s.append(((i + phase as usize) % 251) as char)
        i += 1
    }
    return s
}

@test
@test.timeout(60000)
public func INT_https_post_echo_roundtrip(env : &mut TestEnv) {
    int_start_httpsrv(env, 20201u, "/tmp/tls_ih01_cert.pem", "/tmp/tls_ih01_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var payload = int_pattern_string(8192u, 7u)
    var url = int_url(20201u, "/echo")
    var res = client.post(url.to_view(), payload.to_view(), "application/octet-stream")
    if(res is Result.Err) { env.error("POST /echo over TLS failed"); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("expected 200") }

    var body_opt = r.body.read_to_string()
    if(body_opt is std.Option.None) { env.error("body read failed"); return }
    var Some(body) = body_opt else unreachable;
    if(body.size() != 8192u) { env.error("echo length mismatch") }
    if(!body.equals_view(payload.to_view())) { env.error("echo bytes mismatch") }

    var ct = r.headers.get("X-Content-Type")
    if(ct is std.Option.None) { env.error("X-Content-Type missing") } else {
        var Some(ctv) = ct else unreachable;
        if(!ctv.equals_view("application/octet-stream")) { env.error("content type not propagated") }
    }

    test_kill_port(20201u)
}

// ─── Large download over TLS: 200 KB across many records ────────────────────
@test
@test.timeout(60000)
public func INT_https_large_download_200k(env : &mut TestEnv) {
    int_start_httpsrv(env, 20202u, "/tmp/tls_ih02_cert.pem", "/tmp/tls_ih02_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var url = int_url(20202u, "/size/200000")
    var res = client.get(url.to_view())
    if(res is Result.Err) { env.error("large download failed"); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("expected 200") }

    var body_opt = r.body.read_to_string()
    if(body_opt is std.Option.None) { env.error("body read failed"); return }
    var Some(body) = body_opt else unreachable;
    if(body.size() != 200000u) { env.error("download truncated or oversized") }

    // Spot-check the i%251 pattern at several offsets.
    if(body.get(0) != 0 as char) { env.error("pattern[0] mismatch") }
    if(body.get(1000) != (1000 % 251) as char) { env.error("pattern[1000] mismatch") }
    if(body.get(65536) != (65536 % 251) as char) { env.error("pattern[65536] mismatch") }
    if(body.get(199999) != (199999 % 251) as char) { env.error("pattern[last] mismatch") }

    test_kill_port(20202u)
}

// ─── Status codes propagate unchanged through the stack ─────────────────────
@test
@test.timeout(60000)
public func INT_https_status_codes_propagate(env : &mut TestEnv) {
    int_start_httpsrv(env, 20203u, "/tmp/tls_ih03_cert.pem", "/tmp/tls_ih03_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var u201 = int_url(20203u, "/status/201")
    var u404 = int_url(20203u, "/status/404")
    var u500 = int_url(20203u, "/status/500")

    var r1 = client.get(u201.to_view())
    if(r1 is Result.Err) { env.error("status/201 request failed") } else {
        var Ok(x1) = r1 else unreachable;
        if(x1.status != 201u) { env.error("expected 201") }
    }
    var r2 = client.get(u404.to_view())
    if(r2 is Result.Err) { env.error("status/404 request failed") } else {
        var Ok(x2) = r2 else unreachable;
        if(x2.status != 404u) { env.error("expected 404") }
    }
    var r3 = client.get(u500.to_view())
    if(r3 is Result.Err) { env.error("status/500 request failed") } else {
        var Ok(x3) = r3 else unreachable;
        if(x3.status != 500u) { env.error("expected 500") }
    }

    test_kill_port(20203u)
}

// ─── Query parameters survive URL building + TLS transport ──────────────────
@test
@test.timeout(60000)
public func INT_https_query_params_reach_server(env : &mut TestEnv) {
    int_start_httpsrv(env, 20204u, "/tmp/tls_ih04_cert.pem", "/tmp/tls_ih04_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var base = string("https://127.0.0.1:20204/query")
    var u_opt = URL::parse(base.to_view())
    if(u_opt is std::Option.None) { env.error("url parse failed"); return }
    var Some(u) = u_opt else unreachable;
    var rb = RequestBuilder("GET", std::replace(&mut u, URL()))
    rb.query("q", "hello-world")
    rb.query("n", "42")
    var res = client.request(&rb)
    if(res is Result.Err) { env.error("query request failed"); return }
    var Ok(r) = res else unreachable;

    var body_opt = r.body.read_to_string()
    if(body_opt is std::Option.None) { env.error("body read failed"); return }
    var Some(body) = body_opt else unreachable;
    // The python server echoes the raw query string it received.
    if(!body.equals_view("q=hello-world&n=42")) { env.error("query mismatch") }

    test_kill_port(20204u)
}

// ─── Custom request headers reach the server; default UA is sent ────────────
@test
@test.timeout(60000)
public func INT_https_custom_request_headers(env : &mut TestEnv) {
    int_start_httpsrv(env, 20205u, "/tmp/tls_ih05_cert.pem", "/tmp/tls_ih05_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var base = string("https://127.0.0.1:20205/hdrs")
    var u_opt = URL::parse(base.to_view())
    if(u_opt is std::Option.None) { env.error("url parse failed"); return }
    var Some(u) = u_opt else unreachable;
    var rb = RequestBuilder("GET", std::replace(&mut u, URL()))
    rb.header("X-Test", "chemical-rocks")
    var res = client.request(&rb)
    if(res is Result.Err) { env.error("headers request failed"); return }
    var Ok(r) = res else unreachable;

    var body_opt = r.body.read_to_string()
    if(body_opt is std::Option.None) { env.error("body read failed"); return }
    var Some(body) = body_opt else unreachable;
    // Server echoes "X-Test|User-Agent".
    if(!body.contains("chemical-rocks")) { env.error("custom header did not arrive") }
    if(!body.contains("chemical-client")) { env.error("default User-Agent not sent") }

    test_kill_port(20205u)
}

// ─── HEAD over TLS yields headers without a body ────────────────────────────
@test
@test.timeout(60000)
public func INT_https_head_request_empty_body(env : &mut TestEnv) {
    int_start_httpsrv(env, 20206u, "/tmp/tls_ih06_cert.pem", "/tmp/tls_ih06_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var url = int_url(20206u, "/hello")
    var res = client.head(url.to_view())
    if(res is Result.Err) { env.error("HEAD over TLS failed"); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("HEAD expected 200") }

    var cust = r.headers.get("X-Custom")
    if(cust is std.Option.None) { env.error("HEAD should still carry headers") }

    test_kill_port(20206u)
}

// ─── PUT / PATCH / DELETE over TLS ──────────────────────────────────────────
@test
@test.timeout(60000)
public func INT_https_put_patch_delete_work(env : &mut TestEnv) {
    int_start_httpsrv(env, 20207u, "/tmp/tls_ih07_cert.pem", "/tmp/tls_ih07_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var uput = int_url(20207u, "/put")
    var r1 = client.put(uput.to_view(), string_view("data1"), "text/plain")
    if(r1 is Result.Err) { env.error("PUT failed") } else {
        var Ok(x) = r1 else unreachable;
        var b1 = x.body.read_to_string()
        if(b1 is std.Option.None) { env.error("PUT body missing") } else {
            var Some(bb) = b1 else unreachable;
            if(!bb.equals_view("put:data1")) { env.error("PUT echo mismatch") }
        }
    }

    var upatch = int_url(20207u, "/patch")
    var r2 = client.patch(upatch.to_view(), string_view("{}"), "application/json")
    if(r2 is Result.Err) { env.error("PATCH failed") } else {
        var Ok(x2) = r2 else unreachable;
        if(x2.status != 200u) { env.error("PATCH expected 200") }
    }

    var udel = int_url(20207u, "/del")
    var r3 = client.delete(udel.to_view())
    if(r3 is Result.Err) { env.error("DELETE failed") } else {
        var Ok(x3) = r3 else unreachable;
        var b3 = x3.body.read_to_string()
        if(b3 is std.Option.None) { env.error("DELETE body missing") } else {
            var Some(bb3) = b3 else unreachable;
            if(!bb3.equals_view("deleted")) { env.error("DELETE body mismatch") }
        }
    }

    test_kill_port(20207u)
}

// ─── Transfer-Encoding: chunked responses reassemble correctly over TLS ─────
@test
@test
@test.timeout(60000)
public func INT_https_chunked_response_parsed(env : &mut TestEnv) {
    int_start_httpsrv(env, 20208u, "/tmp/tls_ih08_cert.pem", "/tmp/tls_ih08_key.pem")

    var client = Client()
    client.insecure_skip_verify()

    var url = int_url(20208u, "/chunked")
    var res = client.get(url.to_view())
    if(res is Result.Err) { env.error("chunked request failed"); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("chunked expected 200") }
    if(!r.body.is_chunked()) { env.error("response should be flagged chunked") }

    var body_opt = r.body.read_to_string()
    if(body_opt is std::Option.None) { env.error("chunked body read failed"); return }
    var Some(body) = body_opt else unreachable;
    // The python server splits 'chunked-body-data' into two chunks.
    if(!body.equals_view("chunked-body-data")) { env.error("chunk reassembly mismatch") }

    test_kill_port(20208u)
}

// ─── Verified HTTPS through http::Client with a caller-provided CA ──────────
// Full trust path: system bundle bypassed, custom root verifies the chain,
// SNI + hostname check use "localhost" (matches the leaf SAN).
@test
@test.timeout(60000)
public func INT_https_verified_with_custom_ca(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20209u)
    test_server_wait()
    test_py_run_foreground(string_view("ca /tmp/tls_vci20209 localhost"))
    test_py_run_background(string_view("httpsrv /tmp/tls_vci20209_chain.crt /tmp/tls_vci20209_leaf.key 20209 8"))
    test_server_wait()

    var root = x509_crt_load_pem_file("/tmp/tls_vci20209_root.pem")
    if(root == null) { env.error("could not load generated root"); return }

    var client = Client()
    client.set_ca_chain(root)

    var res = client.get(string_view("https://localhost:20209/hello"))
    if(res is Result.Err) {
        env.error("verified https via custom CA failed")
        cert_chain_free(root)
        test_kill_port(20209u)
        return
    }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("expected 200") }

    var body_opt = r.body.read_to_string()
    if(body_opt is std.Option.None) { env.error("body read failed"); cert_chain_free(root); return }
    var Some(body) = body_opt else unreachable;
    if(!body.equals_view("world")) { env.error("body mismatch on verified connection") }

    cert_chain_free(root)
    test_kill_port(20209u)
}

// ─── Untrusted chain must be rejected by the Client ─────────────────────────
// Server leaf is signed by CA-A; the client only trusts an unrelated CA-B.
@test
@test.timeout(60000)
public func INT_https_untrusted_ca_fails(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20210u)
    test_server_wait()
    test_py_run_foreground(string_view("ca /tmp/tls_uca20210a localhost"))
    test_py_run_foreground(string_view("ca /tmp/tls_uca20210b unrelated.example.com"))
    test_py_run_background(string_view("httpsrv /tmp/tls_uca20210a_chain.crt /tmp/tls_uca20210a_leaf.key 20210 8"))
    test_server_wait()

    var wrong_root = x509_crt_load_pem_file("/tmp/tls_uca20210b_root.pem")
    if(wrong_root == null) { env.error("could not load unrelated root"); return }

    var client = Client()
    client.set_ca_chain(wrong_root)

    var res = client.get(string_view("https://localhost:20210/hello"))
    if(res is Result.Ok) {
        env.error("handshake MUST fail when the server chains to an untrusted root")
        cert_chain_free(wrong_root)
        test_kill_port(20210u)
        return
    }

    cert_chain_free(wrong_root)
    test_kill_port(20210u)
}

// ─── Hostname mismatch caught during verification (IP vs DNS SAN) ───────────
// Chain verification passes (correct root), but the URL host "127.0.0.1"
// cannot match the certificate's DNSName(localhost). A control request with
// the matching name succeeds with the same client + trust anchor.
@test
@test.timeout(60000)
public func INT_https_hostname_mismatch_fails_verification(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20211u)
    test_server_wait()
    test_py_run_foreground(string_view("ca /tmp/tls_hnm20211 localhost"))
    test_py_run_background(string_view("httpsrv /tmp/tls_hnm20211_chain.crt /tmp/tls_hnm20211_leaf.key 20211 8"))
    test_server_wait()

    var root = x509_crt_load_pem_file("/tmp/tls_hnm20211_root.pem")
    if(root == null) { env.error("could not load root"); return }

    var client = Client()
    client.set_ca_chain(root)

    var res = client.get(string_view("https://127.0.0.1:20211/hello"))
    if(res is Result.Ok) {
        env.error("hostname mismatch (IP vs DNS SAN) must fail under verification")
        cert_chain_free(root)
        test_kill_port(20211u)
        return
    }

    // Control: the same server accepts the correct name.
    var res2 = client.get(string_view("https://localhost:20211/hello"))
    if(res2 is Result.Err) {
        env.error("control: same client should succeed with matching hostname")
        cert_chain_free(root)
        test_kill_port(20211u)
        return
    }

    cert_chain_free(root)
    test_kill_port(20211u)
}

// ─── One client instance switches between plain and TLS targets ─────────────
@test
@test.timeout(60000)
public func INT_http_plain_and_https_same_client(env : &mut TestEnv) {
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20212u)
    test_kill_port(20213u)
    test_server_wait()

    // Plain HTTP target: python's built-in file server (any 200 response works).
    var plain_cmd = test_py_interp()
    plain_cmd.append_view("-m http.server 20212 --bind 127.0.0.1")
    test_run_bg(plain_cmd.data())

    // TLS target: routed HTTPS server with a throwaway self-signed cert.
    test_py_run_foreground(string_view("cert /tmp/tls_ih12_cert.pem /tmp/tls_ih12_key.pem localhost ec"))
    test_py_run_background(string_view("httpsrv /tmp/tls_ih12_cert.pem /tmp/tls_ih12_key.pem 20213 8"))
    test_server_wait()
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    var r_plain = client.get(string_view("http://127.0.0.1:20212/"))
    if(r_plain is Result.Err) { env.error("plain http request through shared client failed") } else {
        var Ok(p1) = r_plain else unreachable;
        if(p1.status != 200u) { env.error("plain http expected 200") }
    }

    var r_tls = client.get(string_view("https://127.0.0.1:20213/hello"))
    if(r_tls is Result.Err) { env.error("https request through shared client failed") } else {
        var Ok(t1) = r_tls else unreachable;
        if(t1.status != 200u) { env.error("https expected 200") }
        var b_opt = t1.body.read_to_string()
        if(b_opt is std::Option.None) { env.error("https body read failed") } else {
            var Some(bd) = b_opt else unreachable;
            if(!bd.equals_view("world")) { env.error("https body mismatch") }
        }
    }

    // And back to plain again — scheme dispatch must remain stable.
    var r_plain2 = client.get(string_view("http://127.0.0.1:20212/"))
    if(r_plain2 is Result.Err) { env.error("second plain http request failed") }

    test_kill_port(20212u)
    test_kill_port(20213u)
}