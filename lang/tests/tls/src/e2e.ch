using namespace tls
using namespace crypto
using std::string_view

@test
public func INT_smoke_test(env : &mut TestEnv) {
    if(SSL_VERSION_TLS1_3 != 0x0304) { env.error("TLS 1.3 version wrong") }
    if(SSL_VERSION_TLS1_2 != 0x0303) { env.error("TLS 1.2 version wrong") }
}

@test
public func INT_tls13_client(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19876)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19876_cert.pem /tmp/tls_19876_key.pem test.example.com ec"))
    test_py_run_background(string_view("srv /tmp/tls_19876_cert.pem /tmp/tls_19876_key.pem 19876 1.3"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19876u)
    if(ret < 0) {
        if(ret == ERR_SSL_HANDSHAKE_FAILURE) { env.error("TLS13: ERR_SSL_HANDSHAKE_FAILURE") }
        else if(ret == ERR_SSL_UNEXPECTED_MESSAGE) { env.error("TLS13: ERR_SSL_UNEXPECTED_MESSAGE") }
        else if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("TLS13: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("TLS13: alert = protocol_version(70)") }
            else if(ctx.last_alert_desc == 47) { env.error("TLS13: alert = illegal_parameter(47)") }
            else if(ctx.last_alert_desc == 50) { env.error("TLS13: alert = decode_error(50)") }
            else if(ctx.last_alert_desc == 51) { env.error("TLS13: alert = decrypt_error(51)") }
            else if(ctx.last_alert_desc == 10) { env.error("TLS13: alert = unexpected_message(10)") }
            else if(ctx.last_alert_desc == 86) { env.error("TLS13: alert = inappropriate_fallback(86)") }
            else if(ctx.last_alert_desc == 110) { env.error("TLS13: alert = unsupported_ext(110)") }
            else if(ctx.last_alert_desc == 112) { env.error("TLS13: alert = unrecognized_name(112)") }
            else { env.error("TLS13: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("TLS13: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("TLS13: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("TLS13: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("TLS13: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("TLS13: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("TLS13: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("TLS13: ERR_SSL_INVALID_RECORD") }
        else if(ret == ERR_SSL_NO_RNG) { env.error("TLS13: ERR_SSL_NO_RNG") }
        else { env.error("TLS13: unknown error") }
    } else {
        var req = "GET / HTTP/1.0\r\n\r\n"
        ssl_write(&raw mut ctx, req as *u8, 18)
        var buf : [512]u8
        var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
        if(n != 2 || buf[0] != 79 || buf[1] != 75) {
            // Expected the Python server's literal "OK" response, proving the
            // application-data decrypt path round-trips the real payload.
            env.error("TLS13: app-data response mismatch")
        }
        ssl_close_notify(&raw mut ctx)
    }
    ssl_free(&raw mut ctx)
    test_kill_port(19876)
}

@test
public func INT_x25519_handshake(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19878)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19878_cert.pem /tmp/tls_19878_key.pem test.example.com ec"))
    test_py_run_background(string_view("srv /tmp/tls_19878_cert.pem /tmp/tls_19878_key.pem 19878 1.3"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19878u)
    if(ret < 0) {
        if(ret == ERR_SSL_HANDSHAKE_FAILURE) { env.error("X25519: ERR_SSL_HANDSHAKE_FAILURE") }
        else if(ret == ERR_SSL_UNEXPECTED_MESSAGE) { env.error("X25519: ERR_SSL_UNEXPECTED_MESSAGE") }
        else if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("X25519: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("X25519: alert = protocol_version(70)") }
            else if(ctx.last_alert_desc == 47) { env.error("X25519: alert = illegal_parameter(47)") }
            else if(ctx.last_alert_desc == 50) { env.error("X25519: alert = decode_error(50)") }
            else if(ctx.last_alert_desc == 51) { env.error("X25519: alert = decrypt_error(51)") }
            else if(ctx.last_alert_desc == 110) { env.error("X25519: alert = unsupported_ext(110)") }
            else { env.error("X25519: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("X25519: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("X25519: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("X25519: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("X25519: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("X25519: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("X25519: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("X25519: ERR_SSL_INVALID_RECORD") }
        else if(ret == ERR_SSL_NO_RNG) { env.error("X25519: ERR_SSL_NO_RNG") }
        else { env.error("X25519: unknown error") }
    }
    ssl_free(&raw mut ctx)
    test_kill_port(19878)
}

@test
public func INT_tls12_client(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19877)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19877_cert.pem /tmp/tls_19877_key.pem test.example.com rsa"))
    // OpenSSL 3.x disables RSA key exchange by default; re-enable it client-side
    // (matches the cipher the Chemical TLS 1.2 client offers).
    test_py_run_background(string_view("srv /tmp/tls_19877_cert.pem /tmp/tls_19877_key.pem 19877 1.2 AES128-GCM-SHA256:@SECLEVEL=0"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_2
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19877u)
    if(ret < 0) {
        if(ret == ERR_SSL_HANDSHAKE_FAILURE) { env.error("TLS12: ERR_SSL_HANDSHAKE_FAILURE") }
        else if(ret == ERR_SSL_UNEXPECTED_MESSAGE) { env.error("TLS12: ERR_SSL_UNEXPECTED_MESSAGE") }
        else if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("TLS12: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("TLS12: alert = protocol_version(70)") }
            else if(ctx.last_alert_desc == 47) { env.error("TLS12: alert = illegal_parameter(47)") }
            else if(ctx.last_alert_desc == 50) { env.error("TLS12: alert = decode_error(50)") }
            else if(ctx.last_alert_desc == 51) { env.error("TLS12: alert = decrypt_error(51)") }
            else { env.error("TLS12: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("TLS12: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("TLS12: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("TLS12: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("TLS12: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("TLS12: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("TLS12: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("TLS12: ERR_SSL_INVALID_RECORD") }
        else { env.error("TLS12: unknown error") }
    } else {
        var req = "GET / HTTP/1.0\r\n\r\n"
        ssl_write(&raw mut ctx, req as *u8, 18)
        var buf : [512]u8
        var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
        if(n != 2 || buf[0] != 79 || buf[1] != 75) {
            env.error("TLS12: app-data response mismatch")
        }
        ssl_close_notify(&raw mut ctx)
    }
    ssl_free(&raw mut ctx)
    test_kill_port(19877)
}

// ─── TLS 1.3 KeyUpdate round-trip ──────────────────────────────────────────
// The Chemical client updates its send keys (KeyUpdate request_response=1) and
// exchanges app data. The Python/OpenSSL server must decrypt the KeyUpdate and
// the following record (both under the correct key generation), respond with
// its own KeyUpdate, and the client must decrypt the "OK" after rotating its
// receive keys.
@test
@test.timeout(60000)
public func INT_tls13_key_update_e2e(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19910)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19910_cert.pem /tmp/tls_19910_key.pem localhost ec"))
    test_py_run_background(string_view("srv /tmp/tls_19910_cert.pem /tmp/tls_19910_key.pem 19910 1.3"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)
    ssl_set_hostname(&raw mut ctx, "localhost")

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19910u)
    if(ret < 0) {
        env.error("TLS13 KeyUpdate: connect failed")
        ssl_free(&raw mut ctx); return
    }

    // Send KeyUpdate requesting the peer to rotate too.
    ret = tls13_send_key_update(&raw mut ctx, true)
    if(ret < 0) {
        env.error("TLS13 KeyUpdate: send failed")
        ssl_free(&raw mut ctx); return
    }

    // Send app data protected with the NEW send keys.
    var req = "GET / HTTP/1.0\r\n\r\n"
    ret = ssl_write(&raw mut ctx, req as *u8, 18)
    if(ret < 0) {
        env.error("TLS13 KeyUpdate: write failed")
        ssl_free(&raw mut ctx); return
    }

    // The server responds with its own KeyUpdate (update_not_requested) and
    // then "OK" under its NEW send keys. ssl_read must process the KeyUpdate
    // (rotate receive keys) before decrypting "OK".
    var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("TLS13 KeyUpdate: app-data mismatch after key update")
        ssl_free(&raw mut ctx); return
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(19910)
}

// ─── TLS 1.3 peer certificate population ───────────────────────────────────
// After a handshake, ssl.peer_cert must hold the server's certificate (a
// stable, library-owned copy) and hostname verification against it must pass.
@test
@test.timeout(60000)
public func INT_tls13_peer_cert(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19911)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19911_cert.pem /tmp/tls_19911_key.pem localhost ec"))
    test_py_run_background(string_view("srv /tmp/tls_19911_cert.pem /tmp/tls_19911_key.pem 19911 1.3"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)
    ssl_set_hostname(&raw mut ctx, "localhost")

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19911u)
    if(ret < 0) {
        env.error("TLS13 peer_cert: connect failed")
        ssl_free(&raw mut ctx); return
    }

    if(ctx.peer_cert == null) {
        env.error("TLS13 peer_cert: peer_cert not populated")
        ssl_free(&raw mut ctx); return
    }

    // The Python server cert has CN=localhost; verification must succeed.
    var hret = x509_verify_hostname(ctx.peer_cert, "localhost")
    if(hret != 0) {
        env.error("TLS13 peer_cert: hostname verification failed")
        ssl_free(&raw mut ctx); return
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(19911)
}

// ─── TLS 1.3 NewSessionTicket storage ──────────────────────────────────────
// The server's post-handshake NewSessionTicket must be stored in ssl.session
// (ticket + derived resumption key) as it flows through ssl_read.
@test
@test.timeout(60000)
public func INT_tls13_session_ticket(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19912)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19912_cert.pem /tmp/tls_19912_key.pem localhost ec"))
    test_py_run_background(string_view("srv /tmp/tls_19912_cert.pem /tmp/tls_19912_key.pem 19912 1.3"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)
    ssl_set_hostname(&raw mut ctx, "localhost")

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19912u)
    if(ret < 0) {
        env.error("TLS13 ticket: connect failed")
        ssl_free(&raw mut ctx); return
    }

    if(ctx.session == null) {
        env.error("TLS13 ticket: session not allocated")
        ssl_free(&raw mut ctx); return
    }

    // The server sends NewSessionTicket(s) before its "OK" app data.
    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)
    var buf : [512]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("TLS13 ticket: app-data mismatch")
        ssl_free(&raw mut ctx); return
    }

    // ssl_read processes the NewSessionTicket internally — verify storage.
    if(ctx.session.ticket == null || ctx.session.ticket_len == 0) {
        env.error("TLS13 ticket: NewSessionTicket not stored")
        ssl_free(&raw mut ctx); return
    }
    if(ctx.session.resumption_key_len != 32) {
        env.error("TLS13 ticket: resumption key not derived")
        ssl_free(&raw mut ctx); return
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(19912)
}

// ─── TLS 1.3 session resumption ────────────────────────────────────────────
// Connection 1 performs a full handshake and stores the server's
// NewSessionTicket + resumption PSK. Connection 2 offers the ticket via
// pre_shared_key (with a correct binder) against the SAME server process;
// the server resumes, skipping the certificate exchange.
@test
@test.timeout(60000)
public func INT_tls13_session_resumption(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19913)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19913_cert.pem /tmp/tls_19913_key.pem localhost ec"))
    test_py_run_background(string_view("srv2 /tmp/tls_19913_cert.pem /tmp/tls_19913_key.pem 19913"))
    test_server_wait()

    // ── Connection 1: full handshake + ticket acquisition ───────────
    var ctx1 : SSLContext; ssl_init(&raw mut ctx1)
    var cfg1 = ssl_config_init(SSL_IS_CLIENT)
    cfg1.authmode = SSL_VERIFY_NONE
    cfg1.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx1, &raw mut cfg1)
    ssl_set_hostname(&raw mut ctx1, "localhost")

    var ret = tls_connect(&raw mut ctx1, "127.0.0.1", 19913u)
    if(ret < 0) {
        env.error("resumption: conn1 connect failed")
        ssl_free(&raw mut ctx1); return
    }
    if(ctx1.peer_cert == null) {
        env.error("resumption: conn1 should present a peer certificate")
        ssl_free(&raw mut ctx1); return
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx1, req as *u8, 18)
    var buf : [512]u8
    var n = ssl_read(&raw mut ctx1, &raw mut buf[0], 512)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("resumption: conn1 app-data mismatch")
        ssl_free(&raw mut ctx1); return
    }
    if(ctx1.session == null || ctx1.session.ticket == null || ctx1.session.ticket_len == 0) {
        env.error("resumption: conn1 did not receive a session ticket")
        ssl_free(&raw mut ctx1); return
    }
    if(ctx1.session.resumption_key_len != 32) {
        env.error("resumption: conn1 did not derive a resumption key")
        ssl_free(&raw mut ctx1); return
    }
    ssl_close_notify(&raw mut ctx1)

    // ── Connection 2: offer the ticket for resumption ────────────────
    var ctx2 : SSLContext; ssl_init(&raw mut ctx2)
    var cfg2 = ssl_config_init(SSL_IS_CLIENT)
    cfg2.authmode = SSL_VERIFY_NONE
    cfg2.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx2, &raw mut cfg2)
    ssl_set_hostname(&raw mut ctx2, "localhost")

    // Copy the session (ticket + resumption PSK) from conn1 into ctx2.
    var tkt_copy = malloc(ctx1.session.ticket_len) as *mut u8
    if(tkt_copy != null) {
        var ci : size_t = 0
        while(ci < ctx1.session.ticket_len) {
            tkt_copy[ci] = ctx1.session.ticket[ci]
            ci += 1
        }
        if(ctx2.session != null) {
            ctx2.session.ticket = tkt_copy
            ctx2.session.ticket_len = ctx1.session.ticket_len
            ctx2.session.resumption_key_len = ctx1.session.resumption_key_len
            var ki : size_t = 0
            while(ki < 32) {
                ctx2.session.resumption_key[ki] = ctx1.session.resumption_key[ki]
                ki += 1
            }
        }
    }

    ret = tls_connect(&raw mut ctx2, "127.0.0.1", 19913u)
    if(ret < 0) {
        env.error("resumption: conn2 connect failed")
        ssl_free(&raw mut ctx1)
        ssl_free(&raw mut ctx2); return
    }

    // Resumption was accepted: the server did not send a Certificate, so
    // peer_cert stays null and psk_accepted is set.
    var resumed : bool = (ctx2.peer_cert == null)
    if(ctx2.handshake != null && ctx2.handshake.psk_accepted) { resumed = true }
    if(!resumed) {
        env.error("resumption: server did not resume (fell back to full handshake)")
        ssl_free(&raw mut ctx1)
        ssl_free(&raw mut ctx2); return
    }

    var req2 = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx2, req2 as *u8, 18)
    var buf2 : [512]u8
    var n2 = ssl_read(&raw mut ctx2, &raw mut buf2[0], 512)
    if(n2 != 2 || buf2[0] != 79 || buf2[1] != 75) {
        env.error("resumption: conn2 app-data mismatch after resume")
        ssl_free(&raw mut ctx1)
        ssl_free(&raw mut ctx2); return
    }

    ssl_close_notify(&raw mut ctx2)
    ssl_free(&raw mut ctx1)
    ssl_free(&raw mut ctx2)
    test_kill_port(19913)
}

// ─── ssl_free / delete cleanup ─────────────────────────────────────────────
// `delete ssl` must run the same full cleanup as ssl_free (via the @delete
// destructor), and mixing ssl_free + delete must not double-free.
@test
public func INT_ssl_delete_cleanup(env : &mut TestEnv) {
    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    if(ssl_mem == null) { env.error("alloc failed"); return }
    ssl_init(ssl_mem)

    // Attach resources so the destructor has something to release.
    var tr : Transform; transform_init(&raw mut tr)
    var tr_out = malloc(sizeof(Transform)) as *mut Transform
    *tr_out = tr
    ssl_mem.transform_out = tr_out
    var tr_in = malloc(sizeof(Transform)) as *mut Transform
    *tr_in = tr
    ssl_mem.transform_in = tr_in

    // Path 1: delete alone -> destructor calls ssl_free.
    delete ssl_mem

    // Path 2: explicit ssl_free then delete must be safe (idempotent cleanup).
    var ssl_mem2 = malloc(sizeof(SSLContext)) as *mut SSLContext
    if(ssl_mem2 == null) { env.error("alloc2 failed"); return }
    ssl_init(ssl_mem2)
    ssl_free(ssl_mem2)
    delete ssl_mem2
}

@test
public func INT_system_ca_bundle(env : &mut TestEnv) {
    var ca = load_system_ca_bundle()
    if(ca == null) {
        env.error("no system CA bundle found")
    } else {
        cert_free(ca)
        unsafe { dealloc ca }
    }
}

@test
public func INT_tls13_server_client(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19880)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19880_cert.pem /tmp/tls_19880_key.pem localhost ec"))
    // Export private key as hex for Chemical to load
    test_py_run_foreground(string_view("privkey /tmp/tls_19880_key.pem /tmp/tls_19880_priv.hex"))

    var cert = x509_crt_load_pem_file("/tmp/tls_19880_cert.pem")
    if(cert == null) { env.error("failed to load server cert"); return }

    var priv_key = ec_privkey_load_hex_file("/tmp/tls_19880_priv.hex")
    if(priv_key == null) {
        cert_free(cert); unsafe { dealloc cert }
        env.error("failed to load private key"); return
    }

    var server_sock = net::listen_addr("127.0.0.1", 19880u)
    if(server_sock == 0 as net::Socket) {
        cert_free(cert); unsafe { dealloc cert }
        ecdsa_context_free(priv_key)
        env.error("listen failed"); return
    }

    test_py_run_background(string_view("cli 127.0.0.1 19880 1.3"))
    test_server_wait()

    net::set_nonblocking(server_sock)
    var client_sock = net::accept_socket(server_sock) as net::Socket
    var accept_attempts = 0
    while(client_sock == 0 as net::Socket && accept_attempts < 50) {
        std::concurrent::sleep_ms(100u)
        client_sock = net::accept_socket(server_sock)
        accept_attempts += 1
    }
    if(client_sock == 0 as net::Socket) {
        env.error("no client connected")
        cert_free(cert); unsafe { dealloc cert }
        ecdsa_context_free(priv_key)
        net::close_socket(server_sock)
        return
    }

    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    ssl_init(ssl_mem)
    ssl_set_socket(ssl_mem, client_sock)

    var cfg = ssl_config_init(SSL_IS_SERVER)
    cfg.own_cert = cert
    cfg.own_key = priv_key as *mut void
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(ssl_mem, &raw mut cfg)

    var ret = ssl_handshake(ssl_mem)
    if(ret < 0) {
        env.error("TLS 1.3 server handshake failed against Python client")
    } else {
        var buf : [512]u8
        var n = ssl_read(ssl_mem, &raw mut buf[0], 512)
        // The Python client sends "GET / HTTP/1.0\r\n\r\n"; verify the decrypted bytes.
        var expect = "GET / HTTP/1.0\r\n\r\n" as *char
        var match = (n == 18)
        var mi : size_t = 0
        while(match && mi < 18) { if(buf[mi] != expect[mi] as u8) { match = false }; mi += 1 }
        if(!match) { env.error("TLS13 server: client request mismatch") }
        var resp = "HTTP/1.0 200 OK\r\n\r\n\0" as *char
        ssl_write(ssl_mem, resp as *u8, 19)
        ssl_close_notify(ssl_mem)
    }

    ssl_free(ssl_mem)
    unsafe { dealloc ssl_mem }
    cert_free(cert)
    unsafe { dealloc cert }
    ecdsa_context_free(priv_key)
    net::close_socket(server_sock)
    test_kill_port(19880)
}

@test
public func INT_ecdsa_server_client_x25519(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19882)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19882_cert.pem /tmp/tls_19882_key.pem localhost ec"))
    test_py_run_foreground(string_view("privkey /tmp/tls_19882_key.pem /tmp/tls_19882_priv.hex"))

    var cert = x509_crt_load_pem_file("/tmp/tls_19882_cert.pem")
    if(cert == null) { env.error("failed to load ECDSA cert"); return }

    var priv_key = ec_privkey_load_hex_file("/tmp/tls_19882_priv.hex")
    if(priv_key == null) {
        cert_free(cert); unsafe { dealloc cert }
        env.error("failed to load private key"); return
    }

    var server_sock = net::listen_addr("127.0.0.1", 19882u)
    if(server_sock == 0 as net::Socket) {
        cert_free(cert); unsafe { dealloc cert }
        ecdsa_context_free(priv_key)
        env.error("listen failed"); return
    }

    test_py_run_background(string_view("cli 127.0.0.1 19882 1.3"))
    test_server_wait()

    net::set_nonblocking(server_sock)
    var client_sock = net::accept_socket(server_sock) as net::Socket
    var accept_attempts = 0
    while(client_sock == 0 as net::Socket && accept_attempts < 50) {
        std::concurrent::sleep_ms(100u)
        client_sock = net::accept_socket(server_sock)
        accept_attempts += 1
    }
    if(client_sock == 0 as net::Socket) {
        env.error("no client")
        cert_free(cert); unsafe { dealloc cert }
        ecdsa_context_free(priv_key)
        net::close_socket(server_sock)
        return
    }

    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    ssl_init(ssl_mem)
    ssl_set_socket(ssl_mem, client_sock)
    var cfg = ssl_config_init(SSL_IS_SERVER)
    cfg.own_cert = cert
    cfg.own_key = priv_key as *mut void
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(ssl_mem, &raw mut cfg)

    var ret = ssl_handshake(ssl_mem)
    if(ret < 0) {
        env.error("ECDSA cert + x25519 server handshake failed")
    } else {
        var buf : [512]u8
        var n = ssl_read(ssl_mem, &raw mut buf[0], 512)
        var expect = "GET / HTTP/1.0\r\n\r\n" as *char
        var match = (n == 18)
        var mi : size_t = 0
        while(match && mi < 18) { if(buf[mi] != expect[mi] as u8) { match = false }; mi += 1 }
        if(!match) { env.error("ECDSA server: client request mismatch") }
        var resp = "HTTP/1.0 200 OK\r\n\r\n\0" as *char
        ssl_write(ssl_mem, resp as *u8, 19)
        ssl_close_notify(ssl_mem)
    }
    ssl_free(ssl_mem)
    unsafe { dealloc ssl_mem }
    cert_free(cert)
    unsafe { dealloc cert }
    ecdsa_context_free(priv_key)
    net::close_socket(server_sock)
    test_kill_port(19882)
}

@test
public func INT_ecdsa_client_handshake(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19883)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19883_cert.pem /tmp/tls_19883_key.pem 127.0.0.1 ec"))
    test_py_run_background(string_view("srv /tmp/tls_19883_cert.pem /tmp/tls_19883_key.pem 19883 1.3"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19883u)
    if(ret < 0) {
        if(ret == ERR_SSL_HANDSHAKE_FAILURE) { env.error("ECDSA: ERR_SSL_HANDSHAKE_FAILURE") }
        else if(ret == ERR_SSL_UNEXPECTED_MESSAGE) { env.error("ECDSA: ERR_SSL_UNEXPECTED_MESSAGE") }
        else if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("ECDSA: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("ECDSA: alert = protocol_version(70)") }
            else if(ctx.last_alert_desc == 47) { env.error("ECDSA: alert = illegal_parameter(47)") }
            else if(ctx.last_alert_desc == 50) { env.error("ECDSA: alert = decode_error(50)") }
            else if(ctx.last_alert_desc == 51) { env.error("ECDSA: alert = decrypt_error(51)") }
            else if(ctx.last_alert_desc == 110) { env.error("ECDSA: alert = unsupported_ext(110)") }
            else { env.error("ECDSA: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("ECDSA: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("ECDSA: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("ECDSA: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("ECDSA: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("ECDSA: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("ECDSA: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("ECDSA: ERR_SSL_INVALID_RECORD") }
        else if(ret == ERR_SSL_NO_RNG) { env.error("ECDSA: ERR_SSL_NO_RNG") }
        else { env.error("ECDSA: unknown error") }
    }
    ssl_free(&raw mut ctx)
    test_kill_port(19883)
}

@test
@test.timeout(60000)
public func INT_x509_extract_ecdsa_pubkey_works(env : &mut TestEnv) {
    write_tls_python_utils()
    test_py_run_foreground(string_view("cert /tmp/tls_ec_pub.crt /tmp/tls_ec_pub.key localhost ec"))
    test_py_run_foreground(string_view("cert /tmp/tls_rsa_pub.crt /tmp/tls_rsa_pub.key localhost rsa"))

    var ec_cert = x509_crt_load_pem_file("/tmp/tls_ec_pub.crt")
    if(ec_cert == null) { env.error("failed to load EC cert"); return }

    var ecdsa : ECDSAContext
    ecdsa_init(&raw mut ecdsa)
    var ret = x509_extract_ecdsa_pubkey(ec_cert, &raw mut ecdsa)
    if(ret != 0) {
        printf("[X509_EC] extract ret=%d\n", ret as int)
        cert_free(ec_cert); unsafe { dealloc ec_cert }
        env.error("x509_extract_ecdsa_pubkey should succeed on EC cert")
        return
    }
    if(!ecdsa.is_init) {
        cert_free(ec_cert); unsafe { dealloc ec_cert }
        env.error("extracted ECDSA context should be initialized"); return
    }

    // The extracted public key must verify the self-signed cert's own signature
    var vret = x509_verify_cert_ecdsa_signature(ec_cert, &raw mut ecdsa)
    if(vret != 0) {
        cert_free(ec_cert); unsafe { dealloc ec_cert }
        env.error("x509_verify_cert_ecdsa_signature with extracted key failed")
        return
    }

    // Extracting from an RSA cert must fail with PK type mismatch
    var rsa_cert = x509_crt_load_pem_file("/tmp/tls_rsa_pub.crt")
    if(rsa_cert == null) {
        cert_free(ec_cert); unsafe { dealloc ec_cert }
        env.error("failed to load RSA cert"); return
    }
    var e2 : ECDSAContext
    ecdsa_init(&raw mut e2)
    ret = x509_extract_ecdsa_pubkey(rsa_cert, &raw mut e2)
    if(ret == 0) { env.error("x509_extract_ecdsa_pubkey should reject an RSA cert") }

    cert_free(ec_cert)
    unsafe { dealloc ec_cert }
    cert_free(rsa_cert)
    unsafe { dealloc rsa_cert }
}

@test
@test.timeout(60000)
public func INT_tls_accept_rsa_server_client(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19885)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19885_cert.pem /tmp/tls_19885_key.pem localhost rsa"))
    test_py_run_foreground(string_view("privkey /tmp/tls_19885_key.pem /tmp/tls_19885_priv.txt"))

    var cert = x509_crt_load_pem_file("/tmp/tls_19885_cert.pem")
    if(cert == null) { env.error("failed to load RSA server cert"); return }

    var n_buf : [512]u8
    var d_buf : [512]u8
    var n_len : size_t = 0
    var d_len : size_t = 0
    test_parse_n_d_hex_file("/tmp/tls_19885_priv.txt\0" as *char,
                            &raw mut n_buf[0], 512, &raw mut n_len,
                            &raw mut d_buf[0], 512, &raw mut d_len)
    if(n_len == 0 || d_len == 0) { env.error("failed to parse RSA N/D"); return }

    var rsa_ctx : RSAContext
    rsa_init(&raw mut rsa_ctx, RSA_PKCS_V15, 0)
    var kret = rsa_import_privkey(&raw mut rsa_ctx, &raw n_buf[0], n_len, &raw d_buf[0], d_len)
    if(kret < 0) { env.error("failed to import RSA private key"); return }

    var server_sock = net::listen_addr("127.0.0.1", 19885u)
    if(server_sock == 0 as net::Socket) { env.error("listen failed"); return }

    // The Chemical TLS 1.2 server uses RSA key exchange (no forward secrecy),
    // which OpenSSL 3 disables by default. Enable the legacy cipher client-side.
    var cli_cmd = test_py_interp()
    cli_cmd.append_view("/tmp/tls_utils.py cli 127.0.0.1 19885 1.2 AES128-GCM-SHA256:@SECLEVEL=0 2>/tmp/tls_cli_err.txt")
    test_run_bg(cli_cmd.data())
    test_server_wait()
    test_cat_file(string_view("/tmp/tls_cli_err.txt"))

    net::set_nonblocking(server_sock)
    var client_sock = net::accept_socket(server_sock) as net::Socket
    var accept_attempts = 0
    while(client_sock == 0 as net::Socket && accept_attempts < 50) {
        std::concurrent::sleep_ms(100u)
        client_sock = net::accept_socket(server_sock)
        accept_attempts += 1
    }
    if(client_sock == 0 as net::Socket) {
        env.error("no client connected")
        net::close_socket(server_sock)
        return
    }

    var ssl_mem = tls_accept(client_sock, cert, &raw mut rsa_ctx)
    if(ssl_mem == null) {
        env.error("tls_accept server handshake failed against Python TLS 1.2 client")
    } else {
        var buf : [512]u8
        var n = ssl_read(ssl_mem, &raw mut buf[0], 512)
        var expect = "GET / HTTP/1.0\r\n\r\n" as *char
        var match = (n == 18)
        var mi : size_t = 0
        while(match && mi < 18) { if(buf[mi] != expect[mi] as u8) { match = false }; mi += 1 }
        if(!match) { env.error("tls_accept server: client request mismatch") }
        var resp = "HTTP/1.0 200 OK\r\n\r\n\0" as *char
        ssl_write(ssl_mem, resp as *u8, 19)
        ssl_close_notify(ssl_mem)
        ssl_free(ssl_mem)
        unsafe { dealloc ssl_mem }
    }

    cert_free(cert)
    unsafe { dealloc cert }
    net::close_socket(server_sock)
    test_kill_port(19885)
}

// ─── TLS 1.2 server with the CBC-HMAC cipher path ──────────────────────────
// tls_accept normally pins TLS_RSA_WITH_AES_128_GCM_SHA256. Passing a CBC
// suite exercises the server's CBC record layer (MAC key swap, CBC encrypt/
// decrypt with seq_num MAC) end-to-end against a real OpenSSL client.
@test
@test.timeout(60000)
public func INT_tls_accept_rsa_server_client_cbc(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19886)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19886_cert.pem /tmp/tls_19886_key.pem localhost rsa"))
    test_py_run_foreground(string_view("privkey /tmp/tls_19886_key.pem /tmp/tls_19886_priv.txt"))

    var cert = x509_crt_load_pem_file("/tmp/tls_19886_cert.pem")
    if(cert == null) { env.error("failed to load RSA server cert"); return }

    var n_buf : [512]u8
    var d_buf : [512]u8
    var n_len : size_t = 0
    var d_len : size_t = 0
    test_parse_n_d_hex_file("/tmp/tls_19886_priv.txt\0" as *char,
                            &raw mut n_buf[0], 512, &raw mut n_len,
                            &raw mut d_buf[0], 512, &raw mut d_len)
    if(n_len == 0 || d_len == 0) { env.error("failed to parse RSA N/D"); return }

    var rsa_ctx : RSAContext
    rsa_init(&raw mut rsa_ctx, RSA_PKCS_V15, 0)
    var kret = rsa_import_privkey(&raw mut rsa_ctx, &raw n_buf[0], n_len, &raw d_buf[0], d_len)
    if(kret < 0) { env.error("failed to import RSA private key"); return }

    var server_sock = net::listen_addr("127.0.0.1", 19886u)
    if(server_sock == 0 as net::Socket) { env.error("listen failed"); return }

    // OpenSSL 3 disables RSA key exchange by default; re-enable it client-side.
    var cli_cmd = test_py_interp()
    cli_cmd.append_view("/tmp/tls_utils.py cli 127.0.0.1 19886 1.2 AES128-SHA256:@SECLEVEL=0 2>/tmp/tls_cli_err_cbc.txt")
    test_run_bg(cli_cmd.data())
    test_server_wait()
    test_cat_file(string_view("/tmp/tls_cli_err_cbc.txt"))

    net::set_nonblocking(server_sock)
    var client_sock = net::accept_socket(server_sock) as net::Socket
    var accept_attempts = 0
    while(client_sock == 0 as net::Socket && accept_attempts < 50) {
        std::concurrent::sleep_ms(100u)
        client_sock = net::accept_socket(server_sock)
        accept_attempts += 1
    }
    if(client_sock == 0 as net::Socket) {
        env.error("no client connected")
        net::close_socket(server_sock)
        return
    }

    var ssl_mem = tls_accept(client_sock, cert, &raw mut rsa_ctx, TLS_RSA_WITH_AES_128_CBC_SHA256)
    if(ssl_mem == null) {
        env.error("tls_accept CBC server handshake failed against Python TLS 1.2 client")
    } else {
        if(ssl_mem.negotiated_ciphersuite != TLS_RSA_WITH_AES_128_CBC_SHA256 as u16) {
            env.error("tls_accept CBC: wrong negotiated ciphersuite")
        }
        var buf : [512]u8
        var n = ssl_read(ssl_mem, &raw mut buf[0], 512)
        var expect = "GET / HTTP/1.0\r\n\r\n" as *char
        var match = (n == 18)
        var mi : size_t = 0
        while(match && mi < 18) { if(buf[mi] != expect[mi] as u8) { match = false }; mi += 1 }
        if(!match) { env.error("tls_accept CBC server: client request mismatch") }
        var resp = "HTTP/1.0 200 OK\r\n\r\n\0" as *char
        ssl_write(ssl_mem, resp as *u8, 19)
        ssl_close_notify(ssl_mem)
        ssl_free(ssl_mem)
        unsafe { dealloc ssl_mem }
    }

    cert_free(cert)
    unsafe { dealloc cert }
    net::close_socket(server_sock)
    test_kill_port(19886)
}

// ─── TLS 1.2 client negotiating the CBC-HMAC path ──────────────────────────
// The Python server is restricted to TLS_RSA_WITH_AES_128_CBC_SHA256 and the
// Chemical client config offers only that suite, so the client's CBC record
// path (server_write MAC verify, CBC decrypt) is exercised end-to-end.
@test
@test.timeout(60000)
public func INT_tls12_client_cbc(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19887)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19887_cert.pem /tmp/tls_19887_key.pem test.example.com rsa"))
    test_py_run_background(string_view("srv /tmp/tls_19887_cert.pem /tmp/tls_19887_key.pem 19887 1.2 AES128-SHA256:@SECLEVEL=0"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_2
    config.ciphersuite_list[0] = TLS_RSA_WITH_AES_128_CBC_SHA256 as u16
    config.ciphersuite_count = 1
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19887u)
    if(ret < 0) {
        env.error("TLS12 CBC client handshake failed against Python server")
    } else {
        if(ctx.negotiated_ciphersuite != TLS_RSA_WITH_AES_128_CBC_SHA256 as u16) {
            env.error("TLS12 CBC client: wrong negotiated ciphersuite")
        }
        var req = "GET / HTTP/1.0\r\n\r\n"
        ssl_write(&raw mut ctx, req as *u8, 18)
        var buf : [512]u8
        var n = ssl_read(&raw mut ctx, &raw mut buf[0], 512)
        if(n != 2 || buf[0] != 79 || buf[1] != 75) {
            env.error("TLS12 CBC client: app-data response mismatch")
        }
        ssl_close_notify(&raw mut ctx)
    }
    ssl_free(&raw mut ctx)
    test_kill_port(19887)
}

// ─── Large multi-record live transfer ──────────────────────────────────────
// A real TLS 1.3 connection transfers 128KB from a Python server. This
// exercises record fragmentation, sequencing, and TCP reassembly across many
// ssl_read calls (each returning one decrypted record).
@test
@test.timeout(60000)
public func INT_tls13_large_payload_transfer(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19888)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19888_cert.pem /tmp/tls_19888_key.pem test.example.com ec"))
    test_py_run_background(string_view("bigsrv /tmp/tls_19888_cert.pem /tmp/tls_19888_key.pem 19888 131072"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19888u)
    if(ret < 0) {
        env.error("large transfer: handshake failed against Python server")
        ssl_free(&raw mut ctx)
        test_kill_port(19888)
        return
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    ssl_write(&raw mut ctx, req as *u8, 18)

    var total : size_t = 0
    var bad = false
    var buf : [17400]u8
    while(total < 131072) {
        var n = ssl_read(&raw mut ctx, &raw mut buf[0], 17400)
        if(n <= 0) { bad = true; break }
        var i : size_t = 0
        while(i < n as size_t) {
            var expected = (total % 251) as u8
            if(buf[i] != expected) { bad = true; break }
            total += 1
            i += 1
        }
        if(bad) { break }
    }
    if(bad || total != 131072) {
        env.error("large transfer: payload mismatch or incomplete")
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(19888)
}
