// ============================================================================
// Reverse-direction extras: real python http.client against a Chemical
// TLS server (tls -> net -> minimal HTTP server loop).
// ============================================================================
// Complements INT_srv_* tests with keep-alive chains, HEAD semantics,
// a chunked-upload conformance probe and strict response-shape validation.
//
// CONFORMANCE probes assert RFC-correct behavior even where the current
// implementation likely falls short; failures there are genuine bugs.
//
// Ports used: 20460-20463.
// ============================================================================

using namespace tls
using namespace net
using namespace http
using std::Result
using std::string
using std::string_view

// Send a fully pre-composed response (headers + body) over TLS.
func sdv_send_raw(ssl : *mut SSLContext, resp : string) : int {
    if(resp.size() == 0u) { return -1 }
    return ssl_write(ssl, resp.data() as *u8, resp.size() as i32)
}

// Extract the request target (path[?query]) from the request line.
func sdv_request_target(buf : *u8, consumed : size_t, out : *mut u8, out_cap : size_t) : size_t {
    var sp1 : size_t = 0
    while(sp1 < consumed && buf[sp1] != 32u) { sp1 += 1 }
    if(sp1 >= consumed) { return 0 }
    var sp2 = sp1 + 1u
    while(sp2 < consumed && buf[sp2] != 32u) { sp2 += 1 }
    var len = sp2 - sp1 - 1u
    if(len > out_cap) { len = out_cap }
    var i : size_t = 0
    while(i < len) { out[i] = buf[sp1 + 1u + i]; i += 1 }
    return len
}

// Detect an exact method token at the start of the request.
func sdv_method_is(buf : *u8, m0 : u8, m1 : u8, m2 : u8, m3 : u8) : bool {
    if(buf[0] != m0) { return false }
    if(m1 != 0u && buf[1] != m1) { return false }
    if(m2 != 0u && buf[2] != m2) { return false }
    if(m3 != 0u && buf[3] != m3) { return false }
    return true
}

// Shared skeleton: cert/key setup, listen, launch python client mode,
// handshake, then dispatch to the mode-specific serving loop.
func sdv_serve_mode(env : &mut TestEnv, port : uint, tag : string_view,
                    py_mode : string_view, result_file : string_view,
                    serve_mode : string_view) : bool {
    write_tls_python_utils()
    write_http_extra_py()
    test_ensure_tmp_dir()
    test_kill_port(port as int)
    test_server_wait()

    xpy_gen_cert(tag)
    var cert_path = xpy_cert_path(tag)
    var key_path = xpy_key_path(tag)
    var hex_path = string("/tmp/sdv_")
    hex_path.append_view(&tag)
    hex_path.append_view(".hex")

    // Export the EC private key scalars via the shared tls_utils.py tooling.
    var pk = string("privkey ")
    pk.append_view(key_path.to_view())
    pk.append_view(" ")
    pk.append_view(hex_path.to_view())
    test_py_run_foreground(pk.to_view())

    var cert = x509_crt_load_pem_file(cert_path.data())
    if(cert == null) { env.error("cert load failed"); return false }

    var priv_key = ec_privkey_load_hex_file(hex_path.data())
    if(priv_key == null) {
        cert_free(cert); unsafe { dealloc cert }
        env.error("private key load failed")
        return false
    }

    var ls = listen_addr("127.0.0.1", port)
    if(ls == 0 as net::Socket) {
        cert_free(cert); unsafe { dealloc cert }
        ecdsa_context_free(priv_key)
        env.error("listen failed")
        return false
    }

    // Launch the python client (command kept so a flaky start can be retried).
    var bg = test_py_interp()
    bg.append_view("/tmp/http_extra.py ")
    bg.append_view(&py_mode)
    bg.append_view(" 127.0.0.1 ")
    bg.append_uinteger(port as ubigint)
    bg.append_view(" ")
    bg.append_view(&result_file)
    bg.append_view(" >nul 2>/tmp/sdv_err.txt")
    test_run_bg(bg.data())
    test_server_wait()

    var ok = true
    var served : int = 0

    // The handler decides how many connections to accept; modes here need one.
    const MAX_ACCEPTS : int = 1
    while(served < MAX_ACCEPTS) {
        var cs = srv_accept_with_retry(ls)
        if(cs == 0 as net::Socket) {
            // One retry: cold python starts (AV scan, import cost) can exceed
            // the accept window under load.
            env.info("[SDV] accept timed out; relaunching python client once")
            test_run_bg(bg.data())
            std::concurrent::sleep_ms(500u)
            cs = srv_accept_with_retry(ls)
        }
        if(cs == 0 as net::Socket) {
            env.error("python client never connected")
            var errbuf : [512]u8
            var en = test_read_file("/tmp/sdv_err.txt", &raw mut errbuf[0], 511)
            if(en > 0) {
                errbuf[en] = 0
                printf("[SDV py-stderr] %s\n", &raw errbuf[0])
            } else {
                printf("[SDV py-stderr] <empty — process produced no output>\n")
            }
            ok = false
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
            env.error("server handshake failed")
            var dmsg = string("handshake ret=")
            dmsg.append_integer(hret)
            env.error(dmsg.data())
            ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
            net::close_socket(cs)
            ok = false
            break
        }

        // Mode dispatch.
        if(serve_mode.equals("keepalive3")) {
            if(!sdv_loop_keepalive3(env, ssl_mem)) { ok = false }
        } else if(serve_mode.equals("head")) {
            if(!sdv_loop_head(env, ssl_mem)) { ok = false }
        } else if(serve_mode.equals("chunkup")) {
            if(!sdv_loop_chunkup(env, ssl_mem)) { ok = false }
        } else if(serve_mode.equals("strict")) {
            if(!sdv_loop_strict(env, ssl_mem)) { ok = false }
        } else {
            env.error("unknown serve mode")
            ok = false
        }

        ssl_close_notify(ssl_mem)
        ssl_free(ssl_mem)
        unsafe { dealloc ssl_mem }
        net::close_socket(cs)
        served += 1
    }

    if(ok) {
        srv_check_result_file(env, result_file.data())
    } else {
        var errbuf2 : [512]u8
        var en2 = test_read_file("/tmp/sdv_err.txt", &raw mut errbuf2[0], 511)
        if(en2 > 0) { errbuf2[en2] = 0; printf("[SDV py-stderr] %s\n", &raw errbuf2[0]) }
    }

    cert_free(cert)
    unsafe { dealloc cert }
    ecdsa_context_free(priv_key)
    net::close_socket(ls)
    test_kill_port(port as int)
    return ok
}

// ─── Serve loops ─────────────────────────────────────────────────────────────

// Three GETs over ONE connection ("P:<target>" bodies, keep-alive framing).
func sdv_loop_keepalive3(env : &mut TestEnv, ssl_mem : *mut SSLContext) : bool {
    const BUF_CAP : size_t = 16384u
    var req_buf : [BUF_CAP]u8
    var filled : size_t = 0
    var consumed : size_t = 0

    var round : int = 0
    while(round < 3) {
        var rret = srv_read_request_stream(ssl_mem, &raw mut req_buf[0], BUF_CAP,
                                            &raw mut filled, &raw mut consumed)
        if(rret != 0) { env.error("keepalive3: request read failed"); return false }

        var tgt : [256]u8
        var tlen = sdv_request_target(&raw req_buf[0], consumed, &raw mut tgt[0], 255u)

        var resp = string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ")
        var blen : usize = 2u + tlen
        resp.append_uinteger(blen as ubigint)
        resp.append_view("\r\n\r\nP:")
        var ti : size_t = 0
        while(ti < tlen) { resp.append(tgt[ti] as char); ti += 1 }

        if(sdv_send_raw(ssl_mem, resp) < 0) { env.error("keepalive3: response write failed"); return false }

        // Preserve pipelined leftovers for the next round.
        var remain = filled - consumed
        if(remain > 0) {
            var mv : size_t = 0
            while(mv < remain) { req_buf[mv] = req_buf[consumed + mv]; mv += 1 }
        }
        filled = remain
        round += 1
    }
    return true
}

// HEAD /res → headers only, WITH a Content-Length that must be ignored by the
// client (RFC 9110 §9.3.2).
func sdv_loop_head(env : &mut TestEnv, ssl_mem : *mut SSLContext) : bool {
    const BUF_CAP : size_t = 16384u
    var req_buf : [BUF_CAP]u8
    var filled : size_t = 0
    var consumed : size_t = 0

    var rret = srv_read_request_stream(ssl_mem, &raw mut req_buf[0], BUF_CAP,
                                        &raw mut filled, &raw mut consumed)
    if(rret != 0) { env.error("head: request read failed"); return false }
    if(!sdv_method_is(&raw req_buf[0], 72u, 69u, 65u, 68u)) { env.error("head: expected HEAD"); return false }

    var resp = string("HTTP/1.1 200 OK\r\nX-Srv: head\r\nContent-Length: 5\r\nConnection: close\r\n\r\n")
    if(sdv_send_raw(ssl_mem, resp) < 0) { env.error("head: response write failed"); return false }
    return true
}

// CONFORMANCE probe: python uploads a chunked request body. RFC 9112 §6
// requires servers to decode Transfer-Encoding: chunked. The Chemical server
// here dechunks the received records itself and echoes exactly the decoded
// bytes; python compares against the original payload, so any transport or
// framing corruption (or undecodable chunks) fails the roundtrip.
func sdv_loop_chunkup(env : &mut TestEnv, ssl_mem : *mut SSLContext) : bool {
    const BUF_CAP : size_t = 16384u
    var req_buf : [BUF_CAP]u8
    var filled : size_t = 0
    var consumed : size_t = 0

    // Bound the drain so a missing Content-Length cannot hang us forever.
    net::set_recv_timeout(ssl_mem.transport_socket, 1, 0)

    var rret = srv_read_request_stream(ssl_mem, &raw mut req_buf[0], BUF_CAP,
                                        &raw mut filled, &raw mut consumed)
    if(rret != 0) { env.error("chunkup: header read failed"); return false }

    // Accumulate body bytes until quiet.
    var raw_body : [4096]u8
    var raw_len : size_t = 0
    while(raw_len < 4096u) {
        var n = ssl_read(ssl_mem, (&raw mut raw_body[0]) + raw_len, (4096u - raw_len) as i32)
        if(n <= 0) { break }
        raw_len += (n as usize)
    }

    // Dechunk per RFC 9112 §7.1: hex-size [;ext] CRLF data CRLF ... 0 CRLF trailers.
    var decoded : [2048]u8
    var dlen : size_t = 0
    var pos : size_t = 0
    var done = false
    while(pos < raw_len) {
        // Find the chunk-size line terminator.
        var le = pos
        while(le + 1 < raw_len && !(raw_body[le] == 13u8 && raw_body[le + 1u] == 10u8)) { le += 1 }
        if(le + 1 >= raw_len) { break }

        // Parse hex size up to an optional ';' extension separator.
        var sz : usize = 0
        var hi = pos
        var any_hex = false
        while(hi < le) {
            var c = raw_body[hi]
            var v : int = -1
            if(c >= 48u8 && c <= 57u8) {
                v = (c - 48u8) as int
            } else if(c >= 97u8 && c <= 102u8) {
                v = (c - 87u8) as int
            } else if(c >= 65u8 && c <= 70u8) {
                v = (c - 55u8) as int
            }
            if(v < 0) { break }
            sz = sz * 16u + (v as usize)
            any_hex = true
            hi += 1
        }
        if(!any_hex) { break }

        pos = le + 2u
        if(sz == 0u) { done = true; break }

        var avail = raw_len - pos
        var take = sz
        if(take > avail) { take = avail }
        var k : usize = 0
        while(k < take && dlen < 2048u) {
            decoded[dlen] = raw_body[pos + k]
            dlen += 1
            k += 1
        }
        pos += take
        // Skip the data-terminating CRLF.
        if(pos + 1u < raw_len && raw_body[pos] == 13u8 && raw_body[pos + 1u] == 10u8) {
            pos += 2u
        }
    }

    // Verify the decoded payload matches what python sent.
    var want = "chunk-one-chunk-two!"
    var match = done && dlen == 20u
    if(match) {
        var wi : size_t = 0
        while(wi < dlen) {
            if(decoded[wi] != (want[wi] as u8)) { match = false; break }
            wi += 1
        }
    }
    if(!match) {
        env.error("[CONFORMANCE] chunked request body not decoded correctly")
        var m = string("decoded=")
        m.append_uinteger(dlen as ubigint)
        m.append_view(" complete=")
        var ds = string("false")
        if(done) { ds = string("true") }
        m.append_view(ds.to_view())
        env.error(m.data())
    }

    // Echo exactly what we decoded — python validates against the original.
    var resp = string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ")
    resp.append_uinteger(dlen as ubigint)
    resp.append_view("\r\nConnection: close\r\n\r\n")
    var di2 : size_t = 0
    while(di2 < dlen) { resp.append(decoded[di2] as char); di2 += 1 }
    if(sdv_send_raw(ssl_mem, resp) < 0) { env.error("chunkup: response write failed"); return false }
    return true
}

// Strict response shape: custom reason phrase + mixed-case duplicate-free
// headers validated by python's parser.
func sdv_loop_strict(env : &mut TestEnv, ssl_mem : *mut SSLContext) : bool {
    const BUF_CAP : size_t = 16384u
    var req_buf : [BUF_CAP]u8
    var filled : size_t = 0
    var consumed : size_t = 0

    var rret = srv_read_request_stream(ssl_mem, &raw mut req_buf[0], BUF_CAP,
                                        &raw mut filled, &raw mut consumed)
    if(rret != 0) { env.error("strict: request read failed"); return false }

    var resp = string("HTTP/1.1 201 Totally Fine\r\nX-Alpha: one\r\nx-beta: two\r\nContent-Length: 4\r\nConnection: close\r\n\r\nBody")
    if(sdv_send_raw(ssl_mem, resp) < 0) { env.error("strict: response write failed"); return false }
    return true
}

// ─── Tests ───────────────────────────────────────────────────────────────────

@test
@test.timeout(90000)
public func SRV_python_keepalive_three_requests_one_connection(env : &mut TestEnv) {
    sdv_serve_mode(env, 20460u, "ka3", "hcli3", "/tmp/sdv_ka3.txt", "keepalive3")
}

@test
@test.timeout(90000)
public func SRV_python_head_gets_headers_only_despite_content_length(env : &mut TestEnv) {
    sdv_serve_mode(env, 20461u, "hd61", "hhead", "/tmp/sdv_head.txt", "head")
}

@test
@test.timeout(90000)
public func SRV_python_chunked_upload_is_echoed_conformance(env : &mut TestEnv) {
    sdv_serve_mode(env, 20462u, "cu62", "hchunkup", "/tmp/sdv_chunkup.txt", "chunkup")
}

@test
@test.timeout(90000)
public func SRV_python_validates_reason_phrase_and_header_casing(env : &mut TestEnv) {
    sdv_serve_mode(env, 20463u, "st63", "hstrict", "/tmp/sdv_strict.txt", "strict")
}
