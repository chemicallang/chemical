// ============================================================================
// API Surface + TLS 1.2 Session Ticket Tests
// ============================================================================
// Unit coverage for previously-unexercised public APIs (handshake_params_init,
// ecp curve selection, ssl_read_new_session_ticket guards) plus a live TLS 1.2
// session-ticket exchange against Python/OpenSSL.
// Ports used: 20130.
// ============================================================================

using namespace tls
using std::string_view

// ─── handshake_params_init zeroes all sensitive buffers ─────────────────────
@test
public func API_handshake_params_init_zeroes(env : &mut TestEnv) {
    var hs_mem = malloc(sizeof(HandshakeParams)) as *mut HandshakeParams
    if(hs_mem == null) { env.error("alloc failed"); return }

    // Poison the memory first so init provably writes the fields it owns.
    memset(hs_mem as *mut void, 0xAB, sizeof(HandshakeParams))

    handshake_params_init(hs_mem)

    var i : size_t = 0
    while(i < 64) {
        if(hs_mem.randbytes[i] != 0) { env.error("randbytes must be zeroed"); return }
        i += 1
    }
    i = 0
    while(i < 256) {
        if(hs_mem.premaster[i] != 0) { env.error("premaster must be zeroed"); return }
        i += 1
    }
    if(hs_mem.premaster_len != 0) { env.error("premaster_len must start at 0") }
    if(hs_mem.psk_len != 0) { env.error("psk_len must start at 0") }
    if(hs_mem.hello_retry_requested) { env.error("hello_retry_requested must start false") }
    if(hs_mem.psk_accepted) { env.error("psk_accepted must start false") }

    unsafe { dealloc hs_mem }
}

// ─── ecp_select_curve switches between P-256 and P-384 domain parameters ────
@test
public func API_ecp_curve_selection_params(env : &mut TestEnv) {
    // P-256 (GLOBAL_CURVE == 0)
    ecp_select_curve(0)
    if(ecp_curve_id() != 0) { env.error("curve id should be 0 for P-256"); return }

    var p : Mpi; var n : Mpi; var gx : Mpi; var gy : Mpi; var b : Mpi
    ecp_curve_p(&raw mut p); ecp_curve_n(&raw mut n)
    ecp_curve_gx(&raw mut gx); ecp_curve_gy(&raw mut gy); ecp_curve_b(&raw mut b)

    // P-256 prime: 2^256 - 2^224 + 2^192 + 2^96 - 1 → 256 bits
    if(mpi_bitlen(&raw mut p) != 256) { env.error("P-256 p bitlen should be 256"); return }
    // Group order n is also 256 bits
    if(mpi_bitlen(&raw mut n) != 256) { env.error("P-256 n bitlen should be 256"); return }

    // Generator coordinates are canonical 256-bit values (nonzero, < p).
    if(mpi_cmp(&raw mut gx, &raw mut p) >= 0) { env.error("P-256 Gx must be < p"); return }
    if(mpi_is_zero(&raw mut gx)) { env.error("P-256 Gx must not be zero"); return }
    if(mpi_cmp(&raw mut gy, &raw mut p) >= 0) { env.error("P-256 Gy must be < p"); return }

    // Curve equation check: y^2 mod p == (x^3 + a*x + b) mod p with a = p-3.
    var y2 : Mpi; var x3 : Mpi; var ax : Mpi; var a : Mpi
    var rhs : Mpi
    var three : Mpi
    mpi_lset(&raw mut three, 3)
    mpi_sub(&raw mut a, &raw mut p, &raw mut three)
    mpi_mul(&raw mut y2, &raw mut gy, &raw mut gy)
    mpi_mod(&raw mut y2, &raw mut y2, &raw mut p)
    mpi_mul(&raw mut x3, &raw mut gx, &raw mut gx)
    mpi_mul(&raw mut x3, &raw mut x3, &raw mut gx)
    mpi_mul(&raw mut ax, &raw mut a, &raw mut gx)
    mpi_add(&raw mut rhs, &raw mut x3, &raw mut ax)
    mpi_add(&raw mut rhs, &raw mut rhs, &raw mut b)
    mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
    if(mpi_cmp(&raw mut y2, &raw mut rhs) != 0) {
        env.error("P-256 generator must satisfy the curve equation")
        return
    }

    // P-384 (GLOBAL_CURVE == 1)
    ecp_select_curve(1)
    if(ecp_curve_id() != 1) { env.error("curve id should be 1 for P-384"); return }

    var p384 : Mpi; var gx384 : Mpi; var b384 : Mpi
    ecp_curve_p(&raw mut p384)
    ecp_curve_gx(&raw mut gx384)
    ecp_curve_b(&raw mut b384)

    // P-384 prime: 2^384 - 2^128 - 2^96 + 2^32 - 1 → 384 bits
    if(mpi_bitlen(&raw mut p384) != 384) { env.error("P-384 p bitlen should be 384"); return }
    if(mpi_bitlen(&raw mut gx384) == 0) { env.error("P-384 Gx must be nonzero"); return }
    if(mpi_cmp(&raw mut gx384, &raw mut p384) >= 0) { env.error("P-384 Gx must be < p"); return }

    // Restore the default so any later logic relying on global state is sane.
    ecp_select_curve(0)
}

// ─── ssl_read_new_session_ticket without a transport fails cleanly ──────────
@test
public func API_ssl_read_new_session_ticket_no_socket_fails(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(&raw mut ctx)

    var ret = ssl_read_new_session_ticket(&raw mut ctx)
    if(ret >= 0) {
        env.error("reading a session ticket with no transport must fail")
    }
    ssl_free(&raw mut ctx)
}

// ─── TLS 1.2 NewSessionTicket: received, decrypted and stored post-handshake
// OpenSSL servers send an encrypted NewSessionTicket right after ServerFinished
// in TLS 1.2. The client must decrypt it through the active transform, store
// the ticket + master secret in ssl.session, and keep the connection usable.
@test
@test.timeout(60000)
public func E2E_tls12_session_ticket_received_and_stored(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20130u)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20130_cert.pem /tmp/tls_20130_key.pem localhost rsa"))
    // OpenSSL 3 disables static-RSA suites at the default security level; pin
    // them server-side so the TLS 1.2 RSA key-exchange path is negotiated.
    // srv handles exactly one exchange (recv → OK), which is all we need here.
    test_py_run_background(string_view("srv /tmp/tls_20130_cert.pem /tmp/tls_20130_key.pem 20130 1.2 AES128-GCM-SHA256:@SECLEVEL=0"))
    test_server_wait()

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_2
    config.ciphersuite_list[0] = TLS_RSA_WITH_AES_128_GCM_SHA256 as u16
    config.ciphersuite_count = 1
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 20130u)
    if(ret < 0) {
        env.error("TLS12 ticket: connect failed")
        ssl_free(&raw mut ctx)
        test_kill_port(20130u)
        return
    }

    if(ctx.session == null) { env.error("TLS12 ticket: session not allocated"); ssl_free(&raw mut ctx); test_kill_port(20130u); return }

    // The server's NewSessionTicket arrives before its app-data response;
    // pull it explicitly through the dedicated post-handshake API.
    ret = ssl_read_new_session_ticket(&raw mut ctx)
    if(ret != 0) {
        // Legitimate gap: record what happened but do not mask it as success.
        env.error("TLS12 ticket: ssl_read_new_session_ticket did not accept the NST")
    } else {
        if(ctx.session.ticket == null || ctx.session.ticket_len == 0) {
            env.error("TLS12 ticket: ticket was not stored after processing NST")
        }
        var ms_nonzero = false
        var mi : size_t = 0
        while(mi < 48) {
            if(ctx.session.master[mi] != 0) { ms_nonzero = true }
            mi += 1
        }
        if(!ms_nonzero) { env.error("TLS12 ticket: master secret missing from session") }
    }

    // The connection must remain fully usable afterwards.
    var ping = "t\0" as *char
    ssl_write(&raw mut ctx, ping as *u8, 1)
    var buf : [64]u8
    var n = ssl_read(&raw mut ctx, &raw mut buf[0], 64)
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("TLS12 ticket: connection unusable after reading NST")
    }

    ssl_close_notify(&raw mut ctx)
    ssl_free(&raw mut ctx)
    test_kill_port(20130u)
}