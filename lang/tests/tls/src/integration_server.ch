// ============================================================================
// Full-Stack Integration: Python HTTPS Client → net → TLS → Chemical Server
// ============================================================================
// The reverse direction: a Chemical TLS server (net accept + ssl_handshake)
// speaks minimal HTTP/1.x, and real Python http.client sessions validate the
// responses end-to-end. Covers GET validation, 50 KB POST echo reassembled
// across many TLS records, trickled (slow) uploads, HTTP/1.1 keep-alive with
// two requests on one connection (including coalesced-record handling), large
// streamed responses, verified clients trusting our CA root, TLS 1.2 RSA key
// exchange serving, and two concurrent clients served sequentially.
// Ports used: 20220-20227.
// ============================================================================

using namespace tls
using std::string
using std::string_view

// ─── Minimal HTTP parsing helpers (byte-level, case-insensitive headers) ────

// Sentinel for "not found" (max size_t value).
const SRV_NPOS : size_t = 18446744073709551615

func srv_mem_find_crlfcrlf(buf : *u8, start : size_t, end : size_t) : size_t {
    var i : size_t = start
    while(i + 3 < end) {
        if(buf[i] == 13 && buf[i+1] == 10 && buf[i+2] == 13 && buf[i+3] == 10) { return i }
        i += 1
    }
    return SRV_NPOS
}

func srv_ci_has_prefix(buf : *u8, pos : size_t, text : *char) : bool {
    var i : size_t = 0
    while(true) {
        var tc : u8 = text[i] as u8
        if(tc == 0) { return true }
        var bc = buf[pos + i]
        if(bc >= 65 && bc <= 90) { bc = bc + 32 }
        var tcc = tc
        if(tcc >= 65 && tcc <= 90) { tcc = tcc + 32 }
        if(bc != tcc) { return false }
        i += 1
    }
    return false
}

func srv_parse_content_length(buf : *u8, head_start : size_t, head_end : size_t) : size_t {
    var i : size_t = head_start
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

// Read one complete HTTP request out of a persistent buffer. `filled` carries
// bytes already buffered from earlier reads (pipelined/coalesced records); on
// success *consumed holds this request's total length so the caller can keep
// any trailing bytes of the next pipelined request on the same connection.
func srv_read_request_stream(ssl : *mut SSLContext, buf : *mut u8, buf_size : size_t,
                              filled : *mut size_t, consumed : *mut size_t) : int {
    while(true) {
        var head_end = srv_mem_find_crlfcrlf(buf, 0, *filled)
        if(head_end != SRV_NPOS) {
            // Content-Length lives in the header block [0, head_end).
            var clen = srv_parse_content_length(buf, 0, head_end)
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

// Send a complete HTTP/1.1 response with a Content-Length body.
func srv_send_response(ssl : *mut SSLContext, status_line : *char,
                        body : *u8, body_len : size_t, keep_alive : bool) : int {
    var hdr : [256]u8
    var hp : size_t = 0
    var s : size_t = 0
    while(status_line[s] != 0) { hdr[hp] = status_line[s] as u8; hp += 1; s += 1 }
    var ct = "Content-Type: text/plain\r\nContent-Length: "
    s = 0
    while(ct[s] != 0) { hdr[hp] = ct[s] as u8; hp += 1; s += 1 }

    var lenbuf : [24]u8
    var li : size_t = 24
    var v = body_len
    if(v == 0) { li -= 1; lenbuf[li] = 48 }
    while(v > 0) {
        li -= 1
        lenbuf[li] = (v % 10) as u8 + 48
        v /= 10
    }
    var di : size_t = li
    while(di < 24) { hdr[hp] = lenbuf[di]; hp += 1; di += 1 }

    var tail = "\r\nConnection: close\r\n\r\n"
    if(keep_alive) { tail = "\r\n\r\n" }
    s = 0
    while(tail[s] != 0) { hdr[hp] = tail[s] as u8; hp += 1; s += 1 }

    var wret = ssl_write(ssl, &raw hdr[0], hp as i32)
    if(wret < 0) { return wret }
    if(body_len > 0) {
        wret = ssl_write(ssl, body, body_len as i32)
        if(wret < 0) { return wret }
    }
    return 0
}

// Poll a python-written result file until it contains a verdict.
func srv_check_result_file(env : &mut TestEnv, path : *char) {
    var buf : [160]u8
    var polls : int = 0
    var got : size_t = 0
    while(polls < 50) {
        got = test_read_file(path, &raw mut buf[0], 159)
        if(got >= 9) {
            var ok = true
            var expect = "RESULT:OK\0" as *char
            var i : size_t = 0
            while(i < 9) {
                if(buf[i] != expect[i] as u8) { ok = false }
                i += 1
            }
            if(ok) { return }
            break
        }
        std::concurrent::sleep_ms(200u)
        polls += 1
    }
    env.error("python client reported failure:")
    if(got > 0) {
        buf[got] = 0
        printf("HCLI: %s\n", &raw buf[0])
    } else {
        env.error("result file empty or missing")
    }
}

// ─── Shared chemical-side TLS/HTTP connection handler ────────────────────────
// mode:
//   "get"       one GET /integrate round, then close
//   "post"      one POST /integrate round (body echoed), then close
//   "keepalive" two GET rounds on ONE connection without closing between
//   "bigresp"   answer the first GET with a streamed 300 KB pattern body
func srv_serve_one_connection(env : &mut TestEnv, cs : net::Socket, mode : string_view,
                               cert_path : *char, key_pem_path : *char, tag : string_view,
                               tls12_rsa : bool) : bool {
    var ok = true

    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    ssl_init(ssl_mem)
    ssl_set_socket(ssl_mem, cs)

    var cert = x509_crt_load_pem_file(cert_path)
    if(cert == null) {
        env.error("server cert failed to load")
        ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
        return false
    }

    var cfg = ssl_config_init(SSL_IS_SERVER)
    cfg.own_cert = cert
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    if(tls12_rsa) {
        cfg.min_tls_version = SSL_VERSION_TLS1_2
        cfg.max_tls_version = SSL_VERSION_TLS1_2
        cfg.ciphersuite_list[0] = TLS_RSA_WITH_AES_128_GCM_SHA256 as u16
        cfg.ciphersuite_count = 1
    }

    // Export the PEM private key to raw integers via python, then load it.
    var hex_path = string("/tmp/tls_srv_")
    hex_path.append_view(&tag)
    hex_path.append_view(".hex")
    var pk_cmd = string()
    pk_cmd.append_view("privkey ")
    pk_cmd.append_view(string_view(key_pem_path))
    pk_cmd.append_view(" ")
    pk_cmd.append_view(hex_path.to_view())
    test_py_run_foreground(pk_cmd.to_view())

    var rsa_ctx : RSAContext
    var using_rsa = false
    if(tls12_rsa) {
        var n_buf : [512]u8
        var d_buf : [512]u8
        var n_len : size_t = 0
        var d_len : size_t = 0
        test_parse_n_d_hex_file(hex_path.data(), &raw mut n_buf[0], 512, &raw mut n_len,
                                 &raw mut d_buf[0], 512, &raw mut d_len)
        if(n_len == 0 || d_len == 0) {
            env.error("failed to parse RSA key material")
            cert_free(cert); unsafe { dealloc cert }
            ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
            return false
        }
        rsa_init(unsafe(&raw mut rsa_ctx), RSA_PKCS_V15, 0)
        if(rsa_import_privkey(unsafe(&raw mut rsa_ctx), &raw n_buf[0], n_len, &raw d_buf[0], d_len) < 0) {
            env.error("RSA private key import failed")
            cert_free(cert); unsafe { dealloc cert }
            ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
            return false
        }
        cfg.own_key = unsafe(&raw mut rsa_ctx) as *mut void
        using_rsa = true
    } else {
        var priv_key = ec_privkey_load_hex_file(hex_path.data())
        if(priv_key == null) {
            env.error("EC private key load failed")
            cert_free(cert); unsafe { dealloc cert }
            ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
            return false
        }
        cfg.own_key = priv_key as *mut void
        ssl_set_config(ssl_mem, &raw mut cfg)

        var hret = ssl_handshake(ssl_mem)
        if(hret < 0) {
            env.error("TLS server handshake against python failed")
            ecdsa_context_free(priv_key)
            cert_free(cert); unsafe { dealloc cert }
            ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
            return false
        }

        ok = srv_serve_http_loop(env, ssl_mem, mode)
        ecdsa_context_free(priv_key)
        cert_free(cert); unsafe { dealloc cert }
        ssl_free(ssl_mem)
        unsafe { dealloc ssl_mem }
        return ok
    }

    ssl_set_config(ssl_mem, &raw mut cfg)
    var hret2 = ssl_handshake(ssl_mem)
    if(hret2 < 0) {
        env.error("TLS 1.2 server handshake against python failed")
        rsa_free(unsafe(&raw mut rsa_ctx))
        cert_free(cert); unsafe { dealloc cert }
        ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
        return false
    }
    ok = srv_serve_http_loop(env, ssl_mem, mode)
    rsa_free(unsafe(&raw mut rsa_ctx))
    cert_free(cert); unsafe { dealloc cert }
    ssl_free(ssl_mem)
    unsafe { dealloc ssl_mem }
    return ok
}

// The request/response exchange after a completed handshake.
func srv_serve_http_loop(env : &mut TestEnv, ssl_mem : *mut SSLContext, mode : string_view) : bool {
    const SRV_BUF_CAP : size_t = 70000
    var ok = true

    var req_buf : [SRV_BUF_CAP]u8
    var scratch : [64]u8
    var filled : size_t = 0
    var consumed : size_t = 0

    if(mode.equals("bigresp")) {
        var rret = srv_read_request_stream(ssl_mem, &raw mut req_buf[0], SRV_BUF_CAP,
                                            &raw mut filled, &raw mut consumed)
        if(rret != 0) {
            env.error("bigresp: request read failed")
            return false
        }
        const BIG : usize = 300000
        var hmsg = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 300000\r\nConnection: close\r\n\r\n"
        var hlen : size_t = 0
        while(hmsg[hlen] != 0) { hlen += 1 }
        if(ssl_write(ssl_mem, hmsg as *u8, hlen as i32) < 0) {
            env.error("bigresp: header write failed")
            return false
        }
        var chunk : [16384]u8
        var base : size_t = 0
        while(base < BIG) {
            var ci : size_t = 0
            while(ci < 16384) {
                chunk[ci] = ((base + ci) % 251) as u8
                ci += 1
            }
            var wlen = 16384 as size_t
            if(base + wlen > BIG) { wlen = BIG - base }
            if(ssl_write(ssl_mem, &raw chunk[0], wlen as i32) < 0) {
                env.error("bigresp: body write failed")
                return false
            }
            base += wlen
        }
        ssl_close_notify(ssl_mem)
        return true
    }

    var rounds : int = 1
    var keep = false
    if(mode.equals("keepalive")) { rounds = 2; keep = true }

    var round : int = 0
    while(round < rounds) {
        var rret = srv_read_request_stream(ssl_mem, &raw mut req_buf[0], SRV_BUF_CAP,
                                            &raw mut filled, &raw mut consumed)
        if(rret != 0) {
            env.error("request stream read failed")
            return false
        }

        var is_post = (consumed >= 4 && req_buf[0] == 80 && req_buf[1] == 79 &&
                       req_buf[2] == 83 && req_buf[3] == 84)

        var head_end = srv_mem_find_crlfcrlf(&raw req_buf[0], 0, consumed)
        var body_start = head_end + 4
        if(is_post) {
            srv_send_response(ssl_mem, "HTTP/1.1 200 OK",
                              (&raw req_buf[0]) + body_start, consumed - body_start, keep)
        } else {
            // Find "?a=N" within the request line.
            var line_end : size_t = 0
            while(line_end < consumed && req_buf[line_end] != 13) { line_end += 1 }
            var qpos : size_t = SRV_NPOS
            var li : size_t = 0
            while(li < line_end) {
                if(req_buf[li] == 63) { qpos = li; break }
                li += 1
            }
            if(qpos == SRV_NPOS) {
                var okb = "SRV_OK"
                srv_send_response(ssl_mem, "HTTP/1.1 200 OK", okb as *u8, 6, keep)
            } else {
                var si2 : size_t = 0
                var prefix = "SRV_OK:"
                while(prefix[si2] != 0) { scratch[si2] = prefix[si2] as u8; si2 += 1 }
                var qi = qpos + 1
                // Stop at the space separating the request target from the
                // HTTP version ("GET /integrate?a=1<SP>HTTP/1.1").
                while(qi < line_end && req_buf[qi] != 32 && si2 < 60) { scratch[si2] = req_buf[qi]; si2 += 1; qi += 1 }
                srv_send_response(ssl_mem, "HTTP/1.1 200 OK", scratch, si2, keep)
            }
        }

        // Preserve any pipelined bytes for the next round.
        var remain = filled - consumed
        if(remain > 0) {
            var mv : size_t = 0
            while(mv < remain) {
                req_buf[mv] = req_buf[consumed + mv]
                mv += 1
            }
        }
        filled = remain
        round += 1
    }
    ssl_close_notify(ssl_mem)
    return ok
}

// ─── Test-side launchers ─────────────────────────────────────────────────────

func srv_launch_hcli(port : uint, host : string_view, py_mode : string_view,
                      cafile : string_view, ciphers : string_view, result_file : string_view) {
    test_ensure_tmp_dir()
    // Truncate any stale verdict file so the poller never reads an old
    // RESULT:FAIL from a previous suite run before python finishes writing.
    var rf = string(&result_file)
    var zero : u8 = 0
    test_write_file(rf.data(), &raw zero, 0)
    var bg = test_py_interp()
    bg.append_view("/tmp/tls_utils.py hcli ")
    bg.append_view(&host)
    bg.append_view(" ")
    bg.append_uinteger(port as ubigint)
    bg.append_view(" ")
    bg.append_view(&result_file)
    bg.append_view(" ")
    bg.append_view(&py_mode)
    bg.append_view(" ")
    if(cafile.size() == 0u) { bg.append_view("-") } else { bg.append_view(&cafile) }
    bg.append_view(" ")
    if(ciphers.size() == 0u) { bg.append_view("-") } else { bg.append_view(&ciphers) }
    test_run_bg(bg.data())
}

func srv_accept_with_retry(ls : net::Socket) : net::Socket {
    net::set_nonblocking(ls)
    var cs = net::accept_socket(ls)
    var tries : int = 0
    while(cs == 0 as net::Socket && tries < 100) {
        std::concurrent::sleep_ms(100u)
        cs = net::accept_socket(ls)
        tries += 1
    }
    return cs
}

// ─── 1. Plain GET: python validates our status + body over TLS 1.3 ──────────
@test
@test.timeout(90000)
public func INT_srv_python_https_get_validated(env : &mut TestEnv) {
    const PORT : uint = 20220u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20220)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_is20_cert.pem /tmp/tls_is20_key.pem localhost ec"))

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) { env.error("listen failed"); return }

    srv_launch_hcli(PORT, "localhost", "get", "", "", "/tmp/hcli_r20.txt")

    var cs = srv_accept_with_retry(ls)
    if(cs == 0 as net::Socket) {
        env.error("client never connected")
        net::close_socket(ls)
        return
    }

    srv_serve_one_connection(env, cs, "get", "/tmp/tls_is20_cert.pem", "/tmp/tls_is20_key.pem", "20", false)
    srv_check_result_file(env, "/tmp/hcli_r20.txt")

    net::close_socket(cs)
    net::close_socket(ls)
    test_kill_port(20220)
}

// ─── 2. POST: 50 KB body reassembled across many TLS records, echoed ────────
@test
@test.timeout(90000)
public func INT_srv_python_post_50k_echo(env : &mut TestEnv) {
    const PORT : uint = 20221u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20221)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_is21_cert.pem /tmp/tls_is21_key.pem localhost ec"))

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) { env.error("listen failed"); return }

    srv_launch_hcli(PORT, "localhost", "post", "", "", "/tmp/hcli_r21.txt")

    var cs = srv_accept_with_retry(ls)
    if(cs == 0 as net::Socket) {
        env.error("client never connected")
        net::close_socket(ls)
        return
    }

    srv_serve_one_connection(env, cs, "post", "/tmp/tls_is21_cert.pem", "/tmp/tls_is21_key.pem", "21", false)
    srv_check_result_file(env, "/tmp/hcli_r21.txt")

    net::close_socket(cs)
    net::close_socket(ls)
    test_kill_port(20221)
}

// ─── 3. Slow upload: trickled writes must not break request framing ─────────
@test
@test.timeout(90000)
public func INT_srv_python_slowpost_reassembled(env : &mut TestEnv) {
    const PORT : uint = 20222u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20222)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_is22_cert.pem /tmp/tls_is22_key.pem localhost ec"))

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) { env.error("listen failed"); return }

    srv_launch_hcli(PORT, "localhost", "slowpost", "", "", "/tmp/hcli_r22.txt")

    var cs = srv_accept_with_retry(ls)
    if(cs == 0 as net::Socket) {
        env.error("client never connected")
        net::close_socket(ls)
        return
    }

    // Server behavior for slowpost is identical to post (read to Content-Length).
    srv_serve_one_connection(env, cs, "post", "/tmp/tls_is22_cert.pem", "/tmp/tls_is22_key.pem", "22", false)
    srv_check_result_file(env, "/tmp/hcli_r22.txt")

    net::close_socket(cs)
    net::close_socket(ls)
    test_kill_port(20222)
}

// ─── 4. Keep-alive: two requests on one connection (pipelining-safe) ────────
@test
@test.timeout(90000)
public func INT_srv_python_keepalive_two_requests(env : &mut TestEnv) {
    const PORT : uint = 20223u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20223)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_is23_cert.pem /tmp/tls_is23_key.pem localhost ec"))

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) { env.error("listen failed"); return }

    srv_launch_hcli(PORT, "localhost", "keepalive", "", "", "/tmp/hcli_r23.txt")

    var cs = srv_accept_with_retry(ls)
    if(cs == 0 as net::Socket) {
        env.error("client never connected")
        net::close_socket(ls)
        return
    }

    srv_serve_one_connection(env, cs, "keepalive", "/tmp/tls_is23_cert.pem", "/tmp/tls_is23_key.pem", "23", false)
    srv_check_result_file(env, "/tmp/hcli_r23.txt")

    net::close_socket(cs)
    net::close_socket(ls)
    test_kill_port(20223)
}

// ─── 5. Large streamed response: 300 KB written in 16 KB chunks ────────────
@test
@test.timeout(90000)
public func INT_srv_big_response_streamed_300k(env : &mut TestEnv) {
    const PORT : uint = 20224u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20224)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_is24_cert.pem /tmp/tls_is24_key.pem localhost ec"))

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) { env.error("listen failed"); return }

    srv_launch_hcli(PORT, "localhost", "bigresp", "", "", "/tmp/hcli_r24.txt")

    var cs = srv_accept_with_retry(ls)
    if(cs == 0 as net::Socket) {
        env.error("client never connected")
        net::close_socket(ls)
        return
    }

    srv_serve_one_connection(env, cs, "bigresp", "/tmp/tls_is24_cert.pem", "/tmp/tls_is24_key.pem", "24", false)
    srv_check_result_file(env, "/tmp/hcli_r24.txt")

    net::close_socket(cs)
    net::close_socket(ls)
    test_kill_port(20224)
}

// ─── 6. Verified python client trusts OUR CA root and hostname ──────────────
@test
@test.timeout(90000)
public func INT_srv_verified_client_trusts_root(env : &mut TestEnv) {
    const PORT : uint = 20225u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20225)
    test_server_wait()
    test_py_run_foreground(string_view("ca /tmp/tls_is25ca localhost"))

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) { env.error("listen failed"); return }

    srv_launch_hcli(PORT, "localhost", "get", "/tmp/tls_is25ca_root.pem", "", "/tmp/hcli_r25.txt")

    var cs = srv_accept_with_retry(ls)
    if(cs == 0 as net::Socket) {
        env.error("client never connected")
        net::close_socket(ls)
        return
    }

    srv_serve_one_connection(env, cs, "get", "/tmp/tls_is25ca_chain.crt", "/tmp/tls_is25ca_leaf.key", "25", false)
    srv_check_result_file(env, "/tmp/hcli_r25.txt")

    net::close_socket(cs)
    net::close_socket(ls)
    test_kill_port(20225)
}

// ─── 7. TLS 1.2 RSA-key-exchange server against a pinned python client ─────
@test
@test.timeout(90000)
public func INT_srv_tls12_rsa_get_validated(env : &mut TestEnv) {
    const PORT : uint = 20226u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20226)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_is26_cert.pem /tmp/tls_is26_key.pem localhost rsa"))

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) { env.error("listen failed"); return }

    // OpenSSL 3 disables static-RSA key exchange at the default level.
    srv_launch_hcli(PORT, "localhost", "get", "", "AES128-GCM-SHA256:@SECLEVEL=0", "/tmp/hcli_r26.txt")

    var cs = srv_accept_with_retry(ls)
    if(cs == 0 as net::Socket) {
        env.error("client never connected")
        net::close_socket(ls)
        return
    }

    srv_serve_one_connection(env, cs, "get", "/tmp/tls_is26_cert.pem", "/tmp/tls_is26_key.pem", "26", true)
    srv_check_result_file(env, "/tmp/hcli_r26.txt")

    net::close_socket(cs)
    net::close_socket(ls)
    test_kill_port(20226)
}

// ─── 8. Two clients arrive together; both get served sequentially ───────────
@test
@test.timeout(90000)
public func INT_srv_two_clients_both_served(env : &mut TestEnv) {
    const PORT : uint = 20227u
    write_tls_python_utils()
    test_ensure_tmp_dir()
    test_kill_port(20227)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_is27_cert.pem /tmp/tls_is27_key.pem localhost ec"))

    var ls = net::listen_addr("127.0.0.1", PORT)
    if(ls == 0 as net::Socket) { env.error("listen failed"); return }

    srv_launch_hcli(PORT, "localhost", "get", "", "", "/tmp/hcli_r27a.txt")
    srv_launch_hcli(PORT, "localhost", "get", "", "", "/tmp/hcli_r27b.txt")

    var ok_all = true

    var cs1 = srv_accept_with_retry(ls)
    if(cs1 == 0 as net::Socket) {
        env.error("first client never connected")
        net::close_socket(ls)
        return
    }
    if(!srv_serve_one_connection(env, cs1, "get", "/tmp/tls_is27_cert.pem", "/tmp/tls_is27_key.pem", "27a", false)) {
        ok_all = false
    }
    net::close_socket(cs1)

    var cs2 = srv_accept_with_retry(ls)
    if(cs2 == 0 as net::Socket) {
        env.error("second client never connected")
        net::close_socket(ls)
        return
    }
    if(!srv_serve_one_connection(env, cs2, "get", "/tmp/tls_is27_cert.pem", "/tmp/tls_is27_key.pem", "27b", false)) {
        ok_all = false
    }
    net::close_socket(cs2)

    srv_check_result_file(env, "/tmp/hcli_r27a.txt")
    srv_check_result_file(env, "/tmp/hcli_r27b.txt")
    if(!ok_all) { env.error("one of the served connections reported an error") }

    net::close_socket(ls)
    test_kill_port(20227)
}
