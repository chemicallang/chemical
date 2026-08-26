// ============================================================================
// Handshake Failure Paths (vs Python/OpenSSL)
// ============================================================================
// Negative-path handshakes: untrusted CAs, hostname mismatches, version
// mismatches, no-common-cipher alerts, plain-TLS-less peers, raw HTTP against
// a Chemical TLS server, oversized writes, double close_notify, guards after
// disconnect, and refused connections. Every failure must be graceful:
// a negative return code (or surfaced alert), never a crash or hang.
// Ports used: 20120-20128.
// ============================================================================

using namespace tls
using std::string_view

// ─── Verified handshake with an UNRELATED trusted CA must fail ──────────────
// The server presents a leaf signed by CA-A; the client only trusts CA-B.
// With authmode REQUIRED (default) the handshake must be rejected.
@test
@test.timeout(60000)
public func NEG_verified_handshake_untrusted_ca_fails(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20120u)
    test_server_wait()
    // CA-A signs the server leaf; CA-B is the unrelated trust anchor.
    test_py_run_foreground(string_view("ca /tmp/tls_nega20120 localhost"))
    test_py_run_foreground(string_view("ca /tmp/tls_negb20120 unused.example.com"))
    test_py_run_background(string_view("srv /tmp/tls_nega20120_chain.crt /tmp/tls_nega20120_leaf.key 20120 1.3"))
    test_server_wait()

    var wrong_ca = x509_crt_load_pem_file("/tmp/tls_negb20120_root.pem")
    if(wrong_ca == null) { env.error("untrusted CA: could not load CA-B root"); return }

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.ca_chain = wrong_ca
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "localhost", 20120u)
    if(ret >= 0) {
        env.error("untrusted CA: handshake MUST fail when the root is unrelated")
        ssl_close_notify(&raw mut ctx)
        ssl_free(&raw mut ctx)
        cert_chain_free(wrong_ca)
        test_kill_port(20120u)
        return
    }

    if(ret != ERR_SSL_CERT_VERIFY_FAILED && ret != ERR_SSL_FATAL_ALERT_MESSAGE &&
       ret != ERR_SSL_HANDSHAKE_FAILURE && ret != ERR_SSL_DECODE_ERROR &&
       ret != ERR_SSL_UNEXPECTED_MESSAGE) {
        // Any negative code proves rejection; this documents the actual one.
        env.error("untrusted CA: rejected (unexpected-but-negative code)")
    }

    cert_chain_free(wrong_ca)
    ssl_free(&raw mut ctx)
    test_kill_port(20120u)
}

// ─── Hostname mismatch must fail when authmode is REQUIRED ──────────────────
// Cert SAN/CN is localhost; connecting by IP sets hostname "127.0.0.1".
@test
@test.timeout(60000)
public func NEG_hostname_mismatch_fails_when_verifying(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20121u)
    test_server_wait()
    test_py_run_foreground(string_view("ca /tmp/tls_hnm20121 localhost"))
    test_py_run_background(string_view("srv /tmp/tls_hnm20121_chain.crt /tmp/tls_hnm20121_leaf.key 20121 1.3"))
    test_server_wait()

    var ca = x509_crt_load_pem_file("/tmp/tls_hnm20121_root.pem")
    if(ca == null) { env.error("hostname mismatch: could not load root"); return }

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.ca_chain = ca
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    // tls_connect sets SNI/hostname to the host argument; the IP cannot match
    // the certificate's DNSName(localhost), so verification must reject.
    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20121u)
    if(ret >= 0) {
        env.error("hostname mismatch: handshake MUST fail under REQUIRED authmode")
        ssl_close_notify(&raw mut ctx)
        ssl_free(&raw mut ctx)
        cert_chain_free(ca)
        test_kill_port(20121u)
        return
    }

    ssl_free(&raw mut ctx)
    cert_chain_free(ca)
    test_kill_port(20121u)
}

// ─── No common cipher: server answers handshake_failure(40) ─────────────────
// Client pins an ECDSA-authenticated suite while the server holds an RSA
// certificate — no mutually usable suite exists. min=max=TLS 1.2 disables the
// automatic 1.3→1.2 fallback retry so the alert is observable directly.
@test
@test.timeout(60000)
public func NEG_no_common_cipher_alert40_tls12(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20122u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20122_cert.pem /tmp/tls_20122_key.pem localhost rsa"))
    test_py_run_background(string_view("srv /tmp/tls_20122_cert.pem /tmp/tls_20122_key.pem 20122 1.2"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.min_tls_version = SSL_VERSION_TLS1_2
    config.max_tls_version = SSL_VERSION_TLS1_2
    // ECDSA suite vs RSA certificate → zero overlap with the server's offers.
    config.ciphersuite_list[0] = TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 as u16
    config.ciphersuite_count = 1
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20122u)
    if(ret >= 0) {
        env.error("no common cipher: handshake MUST fail (SHA-1 suite disabled)")
        ssl_close_notify(&raw mut ctx)
        ssl_free(&raw mut ctx)
        test_kill_port(20122u)
        return
    }
    if(ret == ERR_SSL_FATAL_ALERT_MESSAGE && ctx.last_alert_desc != 40) {
        // A fatal alert arrived but was not handshake_failure — document it.
        env.error("no common cipher: fatal alert seen but description differs")
    }

    ssl_free(&raw mut ctx)
    test_kill_port(20122u)
}

// ─── Version mismatch: TLS-1.2-only client vs TLS-1.3-only server ───────────
// The server must answer protocol_version(70); the client surfaces it as a
// fatal-alert error rather than crashing or hanging.
@test
@test.timeout(60000)
public func NEG_version_mismatch_alert70(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20123u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20123_cert.pem /tmp/tls_20123_key.pem localhost ec"))
    // srv2 pins both minimum_version AND maximum_version to TLS 1.3.
    test_py_run_background(string_view("srv2 /tmp/tls_20123_cert.pem /tmp/tls_20123_key.pem 20123"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.min_tls_version = SSL_VERSION_TLS1_2
    config.max_tls_version = SSL_VERSION_TLS1_2
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20123u)
    if(ret >= 0) {
        env.error("version mismatch: handshake MUST fail (client is 1.2-only)")
        ssl_close_notify(&raw mut ctx)
        ssl_free(&raw mut ctx)
        test_kill_port(20123u)
        return
    }
    if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
        if(ctx.last_alert_desc != 70) {
            env.error("version mismatch: fatal alert seen but description differs")
        }
    }

    ssl_free(&raw mut ctx)
    test_kill_port(20123u)
}

// ─── Plain TCP peer (not TLS): handshake must fail without crash/hang ───────
@test
@test.timeout(60000)
public func NEG_plain_tcp_peer_handshake_fails_gracefully(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20124u)
    test_server_wait()
    // Payload must be a single shell token (no spaces): any non-TLS bytes do.
    test_py_run_background(string_view("plaintcp 20124 HELLO 1"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20124u)
    if(ret >= 0) {
        env.error("plain TCP peer: handshake MUST fail against non-TLS server")
        ssl_close_notify(&raw mut ctx)
        ssl_free(&raw mut ctx)
        test_kill_port(20124u)
        return
    }
    if(ctx.transport_connected) {
        env.error("plain TCP peer: transport should not remain connected")
    }

    ssl_free(&raw mut ctx)
    test_kill_port(20124u)
}

// ─── Raw HTTP bytes into a Chemical TLS server: graceful failure ────────────
// A non-TLS client dumps plaintext at our TLS 1.3 server; ssl_handshake must
// reject it cleanly instead of crashing.
@test
@test.timeout(60000)
public func NEG_raw_http_to_chemical_tls_server_fails(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20125u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20125_cert.pem /tmp/tls_20125_key.pem localhost ec"))

    var cert = x509_crt_load_pem_file("/tmp/tls_20125_cert.pem")
    if(cert == null) { env.error("raw http: failed to load cert"); return }

    var server_sock = net::listen_addr("127.0.0.1", 20125u)
    if(server_sock == 0 as net::Socket) {
        cert_free(cert); unsafe { dealloc cert }
        env.error("raw http: listen failed"); return
    }

    // rawcli CONNECTS to our listening TLS server and dumps plain
    // HTTP-ish text at it — the hostile-input case for the handshake.
    // (plaintcp is a listener itself and would collide with listen_addr.)
    test_py_run_background(string_view("rawcli 127.0.0.1 20125 GET"))
    test_server_wait()

    net::set_nonblocking(server_sock)
    var client_sock = net::accept_socket(server_sock) as net::Socket
    var attempts : int = 0
    while(client_sock == 0 as net::Socket && attempts < 50) {
        std::concurrent::sleep_ms(100u)
        client_sock = net::accept_socket(server_sock) as net::Socket
        attempts += 1
    }
    if(client_sock == 0 as net::Socket) {
        env.error("raw http: no connection arrived")
        cert_free(cert); unsafe { dealloc cert }
        net::close_socket(server_sock)
        test_kill_port(20125u)
        return
    }

    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    ssl_init(ssl_mem)
    ssl_set_socket(ssl_mem, client_sock)

    var cfg = ssl_config_init(SSL_IS_SERVER)
    cfg.own_cert = cert
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(ssl_mem, &raw mut cfg)

    var ret = ssl_handshake(ssl_mem)
    if(ret >= 0) {
        env.error("raw http: TLS server handshake MUST fail against plaintext input")
    }

    ssl_free(ssl_mem)
    unsafe { dealloc ssl_mem }
    cert_free(cert)
    unsafe { dealloc cert }
    net::close_socket(server_sock)
    test_kill_port(20125u)
}

// ─── Oversized ssl_write (>16384) must be rejected, context stays usable ────
@test
@test.timeout(60000)
public func NEG_oversized_write_fragments_cleanly(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20126u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20126_cert.pem /tmp/tls_20126_key.pem localhost ec"))
    // echo_srv drains exactly <size> bytes before replying, so closing with
    // unread data never triggers a TCP reset mid-test.
    test_py_run_background(string_view("echo /tmp/tls_20126_cert.pem /tmp/tls_20126_key.pem 20126 1 20001"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20126u)
    if(ret < 0) {
        env.error("oversized write: connect failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20126u)
        return
    }

    // MAX_RECORD_PAYLOAD is 16384: writes larger than one record must be
    // fragmented into multiple valid records (SSL_write semantics), never
    // truncated or emitted as an oversized frame. The payload matches the
    // python echo_srv expectation (i%251) so its integrity check passes.
    unsafe var big : [20001]u8
    var bi : size_t = 0
    while(bi < 20001u) { big[bi] = ((bi % 251) as u8); bi += 1 }
    var wret = ssl_write(&raw mut ctx, &raw big[0], 20001)
    if(wret != 20001) {
        env.error("oversized write: ssl_write must fragment and report full length")
    }

    // The context must remain fully usable after the fragmented write: the
    // draining peer answers with "OK" once all 20001 bytes arrived intact.
    unsafe var buf : [64]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 64)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("oversized write: context unusable after fragmented write")
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20126u)
}

// ─── Double close_notify followed by free must be safe ──────────────────────
@test
@test.timeout(60000)
public func NEG_double_close_notify_safe(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20127u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20127_cert.pem /tmp/tls_20127_key.pem localhost ec"))
    test_py_run_background(string_view("mround /tmp/tls_20127_cert.pem /tmp/tls_20127_key.pem 20127 1.3 1 1"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20127u)
    if(ret < 0) {
        env.error("double close_notify: connect failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20127u)
        return
    }

    var ping = "p\0" as *char
    ssl_write(&raw mut ctx, ping as *u8, 1)
    unsafe var buf : [64]u8
    ssl_read(&raw mut ctx, &raw mut buf[0], 64)

    // Two close_notifies in a row: the second may fail (peer gone) but must
    // never corrupt state or crash.
    ssl_close_notify(&raw mut ctx)
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20127u)
}

// ─── Read/write guards on a context that was never connected ────────────────
@test
public func NEG_ssl_ops_without_connection_return_errors(env : &mut TestEnv) {
    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)

    unsafe var buf : [64]u8
    var rret = ssl_read(&raw mut ctx, &raw mut buf[0], 64)
    if(rret >= 0) { env.error("ssl_read on unconnected context must fail") }

    var msg = "x\0" as *char
    var wret = ssl_write(&raw mut ctx, msg as *u8, 1)
    if(wret >= 0) { env.error("ssl_write on unconnected context must fail") }

    // close_notify without a transport must also be a clean error.
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
}

// ─── Connection refused: fast clean failure, no hang ────────────────────────
@test
public func NEG_connect_refused_returns_error_fast(env : &mut TestEnv) {
    // Nothing listens here; dial must fail and surface a negative code.
    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20999u)
    if(ret >= 0) {
        env.error("connect refused: tls_connect must fail on a closed port")
        ssl_free(&raw mut ctx)
        return
    }
    if(ctx.transport_connected) {
        env.error("connect refused: transport should not be connected")
    }
    ssl_free(&raw mut ctx)
}