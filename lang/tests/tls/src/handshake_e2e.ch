// ============================================================================
// Complete Handshake E2E Matrix (vs Python/OpenSSL)
// ============================================================================
// Positive-path, full-stack handshakes covering ciphersuite pinning (TLS 1.3
// AES-256/ChaCha20), certificate flavors (RSA server cert), TLS 1.2 RSA key
// exchange suites (AES-256-GCM + AES-256-CBC), ALPN negotiation, verified
// handshakes with a real CA chain (root-only and leaf+intermediate chains),
// multi-round request/response over one connection, client→server uploads of
// many records (pattern-validated by the peer), max-fragment-boundary writes,
// parallel connection isolation, and clean-close EOF semantics.
// Ports used: 20100-20115.
// ============================================================================

using namespace tls
using std::string_view

// ─── TLS 1.3 pinned to TLS1_3_AES_256_GCM_SHA384 ────────────────────────────
@test
@test.timeout(60000)
public func E2E_tls13_pinned_aes256_gcm_sha384(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20100u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20100_cert.pem /tmp/tls_20100_key.pem test.example.com ec"))
    test_py_run_background(string_view("srv /tmp/tls_20100_cert.pem /tmp/tls_20100_key.pem 20100 1.3"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    config.ciphersuite_list[0] = TLS1_3_AES_256_GCM_SHA384 as u16
    config.ciphersuite_count = 1
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20100u)
    if(ret < 0) {
        env.error("AES256-SHA384: handshake failed")
        if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("AES256-SHA384: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("AES256-SHA384: alert = protocol_version(70)") }
        }
        ssl_free(&raw mut ctx)
        test_kill_port(20100u)
        return
    }

    if(ctx.negotiated_ciphersuite != TLS1_3_AES_256_GCM_SHA384 as u16) {
        env.error("AES256-SHA384: wrong negotiated ciphersuite")
        ssl_free(&raw mut ctx)
        test_kill_port(20100u)
        return
    }
    if(ctx.tls_version != SSL_VERSION_TLS1_3) {
        env.error("AES256-SHA384: expected TLS 1.3 negotiated version")
    }

    // DIAG
    unsafe var dhex : [97]char
    test_bytes_to_hex(&raw ctx.tls13_keys.handshake_secret[0], 48, &raw mut dhex[0])
    printf("DIAG_HS=%s\n", &raw dhex[0])
    test_bytes_to_hex(&raw ctx.tls13_keys.master_secret[0], 48, &raw mut dhex[0])
    printf("DIAG_MS=%s\n", &raw dhex[0])
    test_bytes_to_hex(&raw ctx.tls13_keys.resumption_master_secret[0], 48, &raw mut dhex[0])
    printf("DIAG_CFH=%s\n", &raw dhex[0])
    test_bytes_to_hex(&raw ctx.tls13_keys.exporter_master_secret[0], 48, &raw mut dhex[0])
    printf("DIAG_FULLH=%s\n", &raw dhex[0])
    test_bytes_to_hex(&raw ctx.tls13_keys.client_application_traffic_secret[0], 48, &raw mut dhex[0])
    printf("DIAG_CATS=%s\n", &raw dhex[0])
    test_bytes_to_hex(&raw ctx.tls13_keys.server_application_traffic_secret[0], 48, &raw mut dhex[0])
    printf("DIAG_SATS=%s\n", &raw dhex[0])

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("AES256-SHA384: app-data response mismatch")
    }
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20100u)
}

// ─── TLS 1.3 pinned to TLS1_3_CHACHA20_POLY1305_SHA256 ──────────────────────
@test
@test.timeout(60000)
public func E2E_tls13_pinned_chacha20_poly1305(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20101u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20101_cert.pem /tmp/tls_20101_key.pem test.example.com ec"))
    test_py_run_background(string_view("srv /tmp/tls_20101_cert.pem /tmp/tls_20101_key.pem 20101 1.3"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    config.ciphersuite_list[0] = TLS1_3_CHACHA20_POLY1305_SHA256 as u16
    config.ciphersuite_count = 1
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20101u)
    if(ret < 0) {
        // Legitimate gap if ChaCha20-Poly1305 is not implemented yet: record
        // the failure reason so it is visible in the suite output.
        env.error("CHACHA20: handshake failed")
        if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("CHACHA20: alert = handshake_failure(40)") }
        }
        ssl_free(&raw mut ctx)
        test_kill_port(20101u)
        return
    }

    if(ctx.negotiated_ciphersuite != TLS1_3_CHACHA20_POLY1305_SHA256 as u16) {
        env.error("CHACHA20: wrong negotiated ciphersuite")
        ssl_free(&raw mut ctx)
        test_kill_port(20101u)
        return
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("CHACHA20: app-data response mismatch")
    }
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20101u)
}

// ─── TLS 1.3 against an RSA-certificate server ──────────────────────────────
@test
@test.timeout(60000)
public func E2E_tls13_with_rsa_certificate(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20102u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20102_cert.pem /tmp/tls_20102_key.pem test.example.com rsa"))
    test_py_run_background(string_view("srv /tmp/tls_20102_cert.pem /tmp/tls_20102_key.pem 20102 1.3"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20102u)
    if(ret < 0) {
        env.error("RSA cert: TLS13 handshake failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20102u)
        return
    }

    if(ctx.peer_cert == null) {
        env.error("RSA cert: peer certificate not populated")
        ssl_free(&raw mut ctx)
        test_kill_port(20102u)
        return
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("RSA cert: app-data response mismatch")
    }
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20102u)
}

// ─── TLS 1.2 RSA key exchange pinned to AES-256-GCM-SHA384 ──────────────────
@test
@test.timeout(60000)
public func E2E_tls12_pinned_rsa_aes256_gcm(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20103u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20103_cert.pem /tmp/tls_20103_key.pem test.example.com rsa"))
    test_py_run_background(string_view("srv /tmp/tls_20103_cert.pem /tmp/tls_20103_key.pem 20103 1.2 AES256-GCM-SHA384:@SECLEVEL=0"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_2
    config.ciphersuite_list[0] = TLS_RSA_WITH_AES_256_GCM_SHA384 as u16
    config.ciphersuite_count = 1
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20103u)
    if(ret < 0) {
        if(ret == ERR_SSL_FATAL_ALERT_MESSAGE && ctx.last_alert_desc == 51) {
            env.error("TLS12 AES256-GCM: alert = decrypt_error(51)")
        }
        env.error("TLS12 AES256-GCM: handshake failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20103u)
        return
    }

    if(ctx.negotiated_ciphersuite != TLS_RSA_WITH_AES_256_GCM_SHA384 as u16) {
        env.error("TLS12 AES256-GCM: wrong negotiated ciphersuite")
    }
    if(ctx.tls_version != SSL_VERSION_TLS1_2) {
        env.error("TLS12 AES256-GCM: expected TLS 1.2 negotiated version")
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("TLS12 AES256-GCM: app-data response mismatch")
    }
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20103u)
}

// ─── TLS 1.2 RSA key exchange pinned to AES-256-CBC-SHA256 ──────────────────
@test
@test.timeout(60000)
public func E2E_tls12_pinned_rsa_aes256_cbc(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20104u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20104_cert.pem /tmp/tls_20104_key.pem test.example.com rsa"))
    test_py_run_background(string_view("srv /tmp/tls_20104_cert.pem /tmp/tls_20104_key.pem 20104 1.2 AES256-SHA256:@SECLEVEL=0"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_2
    config.ciphersuite_list[0] = TLS_RSA_WITH_AES_256_CBC_SHA256 as u16
    config.ciphersuite_count = 1
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20104u)
    if(ret < 0) {
        env.error("TLS12 AES256-CBC: handshake failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20104u)
        return
    }

    if(ctx.negotiated_ciphersuite != TLS_RSA_WITH_AES_256_CBC_SHA256 as u16) {
        env.error("TLS12 AES256-CBC: wrong negotiated ciphersuite")
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("TLS12 AES256-CBC: app-data response mismatch")
    }
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20104u)
}

// ─── ALPN negotiation (client offers http/1.1+h2, server prefers h2) ────────
@test
@test.timeout(60000)
public func E2E_alpn_negotiation_h2(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20105u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20105_cert.pem /tmp/tls_20105_key.pem localhost ec"))
    // Python server selects from ['h2']; Chemical client offers http/1.1 + h2.
    // 'DEFAULT' is an inert cipher-string placeholder so positional argv
    // reaches the alpn slot (argv[7]) without restricting the cipher set.
    test_py_run_background(string_view("srv /tmp/tls_20105_cert.pem /tmp/tls_20105_key.pem 20105 1.3 DEFAULT h2"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    unsafe var protos : [2]*char = [
        "http/1.1\0" as *char,
        "h2\0" as *char
    ]
    ssl_set_alpn_protocols(&raw mut config, &raw mut protos[0], 2)
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20105u)
    if(ret < 0) {
        env.error("ALPN: handshake failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20105u)
        return
    }

    var alpn = ssl_get_alpn_negotiated(&raw mut ctx)
    if(alpn == null) {
        env.error("ALPN: no protocol was negotiated")
        ssl_free(&raw mut ctx)
        test_kill_port(20105u)
        return
    }
    if(alpn[0] != 104 || alpn[1] != 50) { // 'h','2'
        env.error("ALPN: expected 'h2' to be negotiated")
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("ALPN: app-data response mismatch after negotiation")
    }
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20105u)
}

// ─── Verified handshake: client trusts the root CA of the server's leaf ─────
// Full trust path over a live connection: authmode stays REQUIRED (default),
// conf.ca_chain holds only the self-signed root, and the server presents its
// leaf. The handshake must succeed and hostname must verify against SAN.
@test
@test.timeout(60000)
public func E2E_verified_handshake_trusted_ca(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20106u)
    test_server_wait()
    test_py_run_foreground(string_view("ca /tmp/tls_vca20106 localhost"))
    test_py_run_background(string_view("srv /tmp/tls_vca20106_chain.crt /tmp/tls_vca20106_leaf.key 20106 1.3"))
    test_server_wait()

    var ca = x509_crt_load_pem_file("/tmp/tls_vca20106_root.pem")
    if(ca == null) { env.error("verified CA: could not load root pem"); return }

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.ca_chain = ca
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    // Connect by name so SNI + hostname verification use "localhost".
    var ret = tls_connect(&raw mut ctx, "localhost", 20106u)
    if(ret < 0) {
        env.error("verified CA: handshake with trusted root failed")
        if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("verified CA: ERR_SSL_CERT_VERIFY_FAILED") }
        cert_chain_free(ca)
        ssl_free(&raw mut ctx)
        test_kill_port(20106u)
        return
    }

    if(ctx.peer_cert == null) {
        env.error("verified CA: peer cert missing")
        cert_chain_free(ca)
        ssl_free(&raw mut ctx)
        test_kill_port(20106u)
        return
    }
    var hret = x509_verify_hostname(ctx.peer_cert, "localhost")
    if(hret != 0) { env.error("verified CA: hostname did not verify") }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("verified CA: app-data response mismatch")
    }
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    cert_chain_free(ca)
    test_kill_port(20106u)
}

// ─── Verified handshake through leaf + intermediate chain ───────────────────
// The server presents leaf+intermediate in one Certificate message; the client
// trusts ONLY the root. x509_verify_chain must walk the intermediate sent by
// the peer to reach the trusted anchor.
@test
@test.timeout(60000)
public func E2E_verified_handshake_intermediate_chain(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20107u)
    test_server_wait()
    test_py_run_foreground(string_view("ca /tmp/tls_vic20107 localhost inter"))
    test_py_run_background(string_view("srv /tmp/tls_vic20107_chain.crt /tmp/tls_vic20107_leaf.key 20107 1.3"))
    test_server_wait()

    var root = x509_crt_load_pem_file("/tmp/tls_vic20107_root.pem")
    if(root == null) { env.error("intermediate CA: could not load root pem"); return }

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.ca_chain = root
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "localhost", 20107u)
    if(ret < 0) {
        env.error("intermediate CA: verified handshake through chain failed")
        cert_chain_free(root)
        ssl_free(&raw mut ctx)
        test_kill_port(20107u)
        return
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("intermediate CA: app-data response mismatch")
    }
    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    cert_chain_free(root)
    test_kill_port(20107u)
}

// ─── Five sequential request/response rounds on ONE connection ──────────────
@test
@test.timeout(60000)
public func E2E_multi_roundtrip_single_connection(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20108u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20108_cert.pem /tmp/tls_20108_key.pem localhost ec"))
    test_py_run_background(string_view("mround /tmp/tls_20108_cert.pem /tmp/tls_20108_key.pem 20108 1.3 5 1"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20108u)
    if(ret < 0) {
        env.error("multi roundtrip: connect failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20108u)
        return
    }

    unsafe var buf : [64]u8
    var round : int = 0
    while(round < 5) {
        var msg = "PING\r\n\0" as *char
        var wret = ssl_write(&raw mut ctx, msg as *u8, 7)
        if(wret < 0) {
            env.error("multi roundtrip: write failed mid-connection")
            ssl_free(&raw mut ctx)
            test_kill_port(20108u)
            return
        }
        var n = ssl_read(&raw mut ctx, &raw mut buf[0], 64)
        if(n != 2 || buf[0] != 79 || buf[1] != 75) {
            env.error("multi roundtrip: response mismatch in round")
            ssl_free(&raw mut ctx)
            test_kill_port(20108u)
            return
        }
        round += 1
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20108u)
}

// ─── Fifty tiny request/response rounds (record sequencing both ways) ───────
@test
@test.timeout(60000)
public func E2E_many_small_roundtrips_fifty(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20109u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20109_cert.pem /tmp/tls_20109_key.pem localhost ec"))
    test_py_run_background(string_view("mround /tmp/tls_20109_cert.pem /tmp/tls_20109_key.pem 20109 1.3 50 1"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20109u)
    if(ret < 0) {
        env.error("small roundtrips: connect failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20109u)
        return
    }

    unsafe var buf : [64]u8
    var round : int = 0
    while(round < 50) {
        var msg = "x\0" as *char
        ssl_write(&raw mut ctx, msg as *u8, 1)
        var n = ssl_read(&raw mut ctx, &raw mut buf[0], 64)
        if(n != 2 || buf[0] != 79 || buf[1] != 75) {
            env.error("small roundtrips: mismatch at round; aborting")
            ssl_free(&raw mut ctx)
            test_kill_port(20109u)
            return
        }
        round += 1
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20109u)
}

// ─── Client→server upload: 96 KiB across six max-size records ───────────────
// The echo server validates every received byte against the i%251 pattern and
// answers OK/BAD, proving record fragmentation + reassembly preserved data.
@test
@test.timeout(60000)
public func E2E_upload_96kb_pattern_validated(env : &mut TestEnv) {
    const UPLOAD_SIZE : size_t = 98304   // 6 * 16384
    write_tls_python_utils()
    test_kill_port(20110u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20110_cert.pem /tmp/tls_20110_key.pem localhost ec"))
    test_py_run_background(string_view("echo /tmp/tls_20110_cert.pem /tmp/tls_20110_key.pem 20110 1 98304"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20110u)
    if(ret < 0) {
        env.error("upload 96k: connect failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20110u)
        return
    }

    unsafe var chunk : [16384]u8
    var ci : size_t = 0
    // The pattern is GLOBAL across the whole upload stream (i%251 over total
    // bytes). 16384 % 251 = 114, so each chunk starts at a shifted phase —
    // rebuild every chunk from its absolute base offset.
    var base : size_t = 0
    while(base < UPLOAD_SIZE) {
        ci = 0
        while(ci < 16384) {
            chunk[ci] = ((base + ci) % 251) as u8
            ci += 1
        }
        var wret = ssl_write(&raw mut ctx, &raw chunk[0], 16384)
        if(wret < 0) {
            env.error("upload 96k: chunk write failed")
            ssl_free(&raw mut ctx)
            test_kill_port(20110u)
            return
        }
        base += 16384
    }

    unsafe var resp : [16]u8
    var n = ssl_read(&raw mut ctx, &raw mut resp[0], 16)
    if(n != 2 || resp[0] != 79 || resp[1] != 75) {
        env.error("upload 96k: server rejected payload (BAD or short read)")
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20110u)
}

// ─── Client→server upload: exactly one max-fragment (16384 B) record ────────
@test
@test.timeout(60000)
public func E2E_upload_exact_max_fragment_16k(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20111u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20111_cert.pem /tmp/tls_20111_key.pem localhost ec"))
    test_py_run_background(string_view("echo /tmp/tls_20111_cert.pem /tmp/tls_20111_key.pem 20111 1 16384"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20111u)
    if(ret < 0) {
        env.error("upload 16k: connect failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20111u)
        return
    }

    unsafe var chunk : [16384]u8
    var ci : size_t = 0
    while(ci < 16384) { chunk[ci] = (ci % 251) as u8; ci += 1 }

    var wret = ssl_write(&raw mut ctx, &raw chunk[0], 16384)
    if(wret < 0) { env.error("upload 16k: exact-max write failed") }

    unsafe var resp : [16]u8
    var n = ssl_read(&raw mut ctx, &raw mut resp[0], 16)
    if(n != 2 || resp[0] != 79 || resp[1] != 75) {
        env.error("upload 16k: server rejected payload")
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20111u)
}

// ─── Two parallel connections to two servers stay isolated ──────────────────
@test
@test.timeout(60000)
public func E2E_two_connections_interleaved_isolated(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20112u)
    test_kill_port(20113u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20112_cert.pem /tmp/tls_20112_key.pem localhost ec"))
    test_py_run_background(string_view("mround /tmp/tls_20112_cert.pem /tmp/tls_20112_key.pem 20112 1.3 2 1"))
    test_py_run_background(string_view("mround /tmp/tls_20112_cert.pem /tmp/tls_20112_key.pem 20113 1.3 2 1"))
    test_server_wait()

    unsafe var ctxA : SSLContext; ssl_init(&raw mut ctxA)
    var cfgA = ssl_config_init(SSL_IS_CLIENT)
    cfgA.authmode = SSL_VERIFY_NONE
    cfgA.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctxA, &raw mut cfgA)

    unsafe var ctxB : SSLContext; ssl_init(&raw mut ctxB)
    var cfgB = ssl_config_init(SSL_IS_CLIENT)
    cfgB.authmode = SSL_VERIFY_NONE
    cfgB.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctxB, &raw mut cfgB)

    var retA = tls_connect(&raw mut ctxA, "127.0.0.1", 20112u)
    var retB = tls_connect(&raw mut ctxB, "127.0.0.1", 20113u)
    if(retA < 0) { env.error("parallel A: connect failed"); ssl_free(&raw mut ctxB); ssl_free(&raw mut ctxA); test_kill_port(20112u); test_kill_port(20113u); return }
    if(retB < 0) { env.error("parallel B: connect failed"); ssl_free(&raw mut ctxB); ssl_free(&raw mut ctxA); test_kill_port(20112u); test_kill_port(20113u); return }

    unsafe var bufA : [64]u8
    unsafe var bufB : [64]u8
    var ping = "p\0" as *char

    // Round 1: write A, write B, then read B first (cross order).
    ssl_write(&raw mut ctxA, ping as *u8, 1)
    ssl_write(&raw mut ctxB, ping as *u8, 1)
    var nb = ssl_read(&raw mut ctxB, &raw mut bufB[0], 64)
    var na = ssl_read(&raw mut ctxA, &raw mut bufA[0], 64)
    if(na != 2 || bufA[0] != 79 || bufA[1] != 75) { env.error("parallel: A round1 mismatch") }
    if(nb != 2 || bufB[0] != 79 || bufB[1] != 75) { env.error("parallel: B round1 mismatch") }

    // Round 2: reverse the write order too.
    ssl_write(&raw mut ctxB, ping as *u8, 1)
    ssl_write(&raw mut ctxA, ping as *u8, 1)
    na = ssl_read(&raw mut ctxA, &raw mut bufA[0], 64)
    nb = ssl_read(&raw mut ctxB, &raw mut bufB[0], 64)
    if(na != 2 || bufA[0] != 79 || bufA[1] != 75) { env.error("parallel: A round2 mismatch") }
    if(nb != 2 || bufB[0] != 79 || bufB[1] != 75) { env.error("parallel: B round2 mismatch") }

    ssl_close_notify(&raw mut ctxA)
    ssl_close_notify(&raw mut ctxB)
    ssl_free(&raw mut ctxA)
    ssl_free(&raw mut ctxB)
    test_kill_port(20112u)
    test_kill_port(20113u)
}

// ─── Clean close: reading after the peer's close_notify returns EOF ─────────
@test
@test.timeout(60000)
public func E2E_clean_close_returns_eof(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20114u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20114_cert.pem /tmp/tls_20114_key.pem localhost ec"))
    test_py_run_background(string_view("srv /tmp/tls_20114_cert.pem /tmp/tls_20114_key.pem 20114 1.3"))
    test_server_wait()

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20114u)
    if(ret < 0) {
        env.error("clean close: connect failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20114u)
        return
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    unsafe var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("clean close: first read mismatch")
        ssl_free(&raw mut ctx)
        test_kill_port(20114u)
        return
    }

    // The python server closed cleanly: the next read must surface EOF
    // (close_notify -> 0, or socket FIN -> ERR_SSL_CONN_EOF), never hang.
    var n2 = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n2 > 0) { env.error("clean close: unexpected extra application data") }

    ssl_free(&raw mut ctx)
    test_kill_port(20114u)
}

// ─── Chemical TLS 1.3 server pinned to AES-256-GCM-SHA384 vs Python client ──
@test
@test.timeout(60000)
public func E2E_tls13_server_pinned_aes256_gcm(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20115u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20115_cert.pem /tmp/tls_20115_key.pem localhost ec"))

    var cert = x509_crt_load_pem_file("/tmp/tls_20115_cert.pem")
    if(cert == null) { env.error("server aes256: failed to load cert"); return }

    var server_sock = net::listen_addr("127.0.0.1", 20115u)
    if(server_sock == 0 as net::Socket) {
        cert_free(cert); unsafe { dealloc cert }
        env.error("server aes256: listen failed"); return
    }

    test_py_run_background(string_view("cli 127.0.0.1 20115 1.3"))
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
        env.error("server aes256: no client connected")
        cert_free(cert); unsafe { dealloc cert }
        net::close_socket(server_sock)
        test_kill_port(20115u)
        return
    }

    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    ssl_init(ssl_mem)
    ssl_set_socket(ssl_mem, client_sock)

    var cfg = ssl_config_init(SSL_IS_SERVER)
    cfg.own_cert = cert
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    cfg.min_tls_version = SSL_VERSION_TLS1_3
    cfg.ciphersuite_list[0] = TLS1_3_AES_256_GCM_SHA384 as u16
    cfg.ciphersuite_count = 1
    ssl_set_config(ssl_mem, &raw mut cfg)

    // Server needs the private key: reuse the EC key loader used by other
    // server tests by exporting hex via python.
    test_py_run_foreground(string_view("privkey /tmp/tls_20115_key.pem /tmp/tls_20115_priv.hex"))
    var priv_key = ec_privkey_load_hex_file("/tmp/tls_20115_priv.hex")
    if(priv_key == null) {
        env.error("server aes256: failed to load private key")
        ssl_free(ssl_mem); unsafe { dealloc ssl_mem }
        cert_free(cert); unsafe { dealloc cert }
        net::close_socket(server_sock)
        test_kill_port(20115u)
        return
    }
    cfg.own_key = priv_key as *mut void
    ssl_set_config(ssl_mem, &raw mut cfg)

    var ret = ssl_handshake(ssl_mem)
    if(ret < 0) {
        env.error("server aes256: TLS 1.3 handshake with pinned suite failed")
    } else {
        if(ssl_mem.negotiated_ciphersuite != TLS1_3_AES_256_GCM_SHA384 as u16) {
            env.error("server aes256: wrong negotiated ciphersuite")
        }
        unsafe var buf : [512]u8
        var n = ssl_read(ssl_mem, &raw mut buf[0], 512)
        var expect = "GET / HTTP/1.0\r\n\r\n" as *char
        var match = (n == 18)
        var mi : size_t = 0
        while(match && mi < 18) {
            if(buf[mi] != expect[mi] as u8) { match = false }
            mi += 1
        }
        if(!match) { env.error("server aes256: client request mismatch") }
        var resp = "HTTP/1.0 200 OK\r\n\r\n\0" as *char
        ssl_write(ssl_mem, resp as *u8, 19)
        ssl_close_notify(ssl_mem)
    }

    ecdsa_context_free(priv_key)
    ssl_free(ssl_mem)
    unsafe { dealloc ssl_mem }
    cert_free(cert)
    unsafe { dealloc cert }
    net::close_socket(server_sock)
    test_kill_port(20115u)
}