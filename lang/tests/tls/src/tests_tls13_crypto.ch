// ============================================================================
// TLS 1.3 Crypto & Config Unit Tests
//
// These tests exercise the TLS 1.3 record layer (encrypt/decrypt), key
// derivation (HKDF-based handshake and application keys), config API, and
// key update — all WITHOUT external processes or network I/O.
//
// The integration tests (lang/tests/tls/) time out because they spawn OpenSSL
// servers and try to handshake.  These unit tests isolate the cryptographic
// primitives and state transitions so failures can be caught early and
// debugged without OpenSSL.
// ============================================================================

using namespace tls

// ─── Helper: byte-level comparison ─────────────────────────────────────────

func bytes_eq(a : *u8, b : *u8, len : size_t) : bool {
    var i : size_t = 0
    while(i < len) {
        if(a[i] != b[i]) { return false }
        i += 1
    }
    return true
}

func bytes_not_all_zero(buf : *u8, len : size_t) : bool {
    var i : size_t = 0
    while(i < len) {
        if(buf[i] != 0) { return true }
        i += 1
    }
    return false
}

// ─── Helper: set up an SSLContext with symmetric transforms for testing ────
// Creates a context with the SAME key for both encrypt and decrypt directions,
// so we can test the TLS 1.3 record layer without needing two peers.

func setup_symmetric_record_test() : SSLContext {
    // NOTE: ssl.conf is intentionally NOT set here.  The record-layer functions
    // (tls13_encrypt_record / tls13_decrypt_record) only need transform_out/in
    // and sequence counters, never ssl.conf.  Setting config from a stack
    // variable would create a dangling pointer when this helper returns.

    unsafe var ctx : SSLContext
    ssl_init(&raw mut ctx)

    // Known AES-128-GCM key (from FIPS 197 test vector)
    var key : [16]u8 = [
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    ]
    // Known IV (12 bytes for TLS 1.3 GCM)
    var iv : [12]u8 = [
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b
    ]

    // Create symmetric transform for encrypt (key_enc) and decrypt (key_dec)
    unsafe var tr : Transform
    transform_init(&raw mut tr)
    tr.cipher_type = CIPHER_AES_128_GCM as u8
    tr.key_len = 16
    tr.iv_len = 12
    tr.fixed_iv_len = 12
    tr.mac_key_len = 0

    var i : size_t = 0
    while(i < 16) {
        tr.key_enc[i] = key[i]
        tr.key_dec[i] = key[i]
        i += 1
    }
    i = 0
    while(i < 12) {
        tr.base_iv_enc[i] = iv[i]
        tr.base_iv_dec[i] = iv[i]
        i += 1
    }

    // Allocate and assign both directions (same key for self-test)
    var tr_out = malloc(sizeof(Transform)) as *mut Transform
    *tr_out = tr
    ctx.transform_out = tr_out

    var tr_in = malloc(sizeof(Transform)) as *mut Transform
    *tr_in = tr
    ctx.transform_in = tr_in

    // Reset sequence numbers (all zeros)
    i = 0
    while(i < 8) {
        ctx.in_ctr[i] = 0
        ctx.out_ctr[i] = 0
        i += 1
    }

    return ctx
}

// ═══════════════════════════════════════════════════════════════════════════
// Config API Tests
// ═══════════════════════════════════════════════════════════════════════════

@test
public func tls_ssl_set_config_works(env : &mut TestEnv) {
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.authmode = SSL_VERIFY_NONE
    cfg.min_tls_version = SSL_VERSION_TLS1_2
    cfg.max_tls_version = SSL_VERSION_TLS1_3

    unsafe var ctx : SSLContext
    ssl_init(&raw mut ctx)
    ssl_set_config(&raw mut ctx, &raw mut cfg)

    if(ctx.conf == null) {
        env.error("ctx.conf should not be null after ssl_set_config")
        return
    }
    if(ctx.conf.endpoint != SSL_IS_CLIENT) {
        env.error("endpoint should match config")
    }
    if(ctx.conf.authmode != SSL_VERIFY_NONE) {
        env.error("authmode should match config")
    }
    if(ctx.conf.min_tls_version != SSL_VERSION_TLS1_2) {
        env.error("min_tls_version should match config")
    }
    if(ctx.conf.max_tls_version != SSL_VERSION_TLS1_3) {
        env.error("max_tls_version should match config")
    }
}

@test
public func tls_ssl_config_init_server_works(env : &mut TestEnv) {
    var cfg = ssl_config_init(SSL_IS_SERVER)

    if(cfg.endpoint != SSL_IS_SERVER) {
        env.error("server config endpoint should be SSL_IS_SERVER")
    }
    if(cfg.transport != SSL_TRANSPORT_STREAM) {
        env.error("server config transport should be stream")
    }
    if(cfg.min_tls_version != SSL_VERSION_TLS1_2) {
        env.error("server config min version should be TLS 1.2")
    }
    if(cfg.max_tls_version != SSL_VERSION_TLS1_3) {
        env.error("server config max version should be TLS 1.3")
    }
    if(cfg.authmode != SSL_VERIFY_REQUIRED) {
        env.error("server config authmode should be VERIFY_REQUIRED")
    }
    if(cfg.own_cert != null) {
        env.error("server config own_cert should be null initially")
    }
    if(cfg.own_key != null) {
        env.error("server config own_key should be null initially")
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// TLS 1.3 Key Derivation Tests
// ═══════════════════════════════════════════════════════════════════════════

@test
public func tls13_derive_handshake_keys_works(env : &mut TestEnv) {
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.authmode = SSL_VERIFY_NONE

    unsafe var ctx : SSLContext
    ssl_init(&raw mut ctx)
    ssl_set_config(&raw mut ctx, &raw mut cfg)

    // Known shared secret (32 bytes, from ECDHE)
    unsafe var shared_secret : [32]u8
    var i : size_t = 0
    while(i < 32) {
        shared_secret[i] = i as u8
        i += 1
    }

    // Known transcript hash (SHA-256 of ClientHello + ServerHello)
    unsafe var transcript_hash : [32]u8
    i = 0
    while(i < 32) {
        transcript_hash[i] = (i + 0x55) as u8
        i += 1
    }

    var ret = tls13_derive_handshake_keys(&raw mut ctx,
                                           &raw shared_secret[0], 32,
                                           &raw transcript_hash[0])
    if(ret < 0) {
        env.error("tls13_derive_handshake_keys should succeed")
        return
    }

    // Verify transforms were allocated
    if(ctx.transform_out == null) {
        env.error("transform_out should be non-null after key derivation")
        return
    }
    if(ctx.transform_in == null) {
        env.error("transform_in should be non-null after key derivation")
        return
    }

    // Verify keys are non-zero
    if(!bytes_not_all_zero(&raw ctx.transform_out.key_enc[0], 16)) {
        env.error("client write key should be non-zero")
    }
    if(!bytes_not_all_zero(&raw ctx.transform_in.key_dec[0], 16)) {
        env.error("server write key should be non-zero")
    }

    // Client and server keys should differ (different roles)
    if(bytes_eq(&raw ctx.transform_out.key_enc[0], &raw ctx.transform_in.key_dec[0], 16)) {
        env.error("client and server write keys should differ for proper TLS")
    }

    // Verify cipher type is set
    if(ctx.transform_out.cipher_type != CIPHER_AES_128_GCM as u8) {
        env.error("cipher_type should be AES-128-GCM")
    }
    if(ctx.transform_out.key_len != 16) {
        env.error("key_len should be 16 for AES-128")
    }
    if(ctx.transform_out.iv_len != 12) {
        env.error("iv_len should be 12 for TLS 1.3")
    }
}

@test
public func tls13_derive_application_keys_works(env : &mut TestEnv) {
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.authmode = SSL_VERIFY_NONE

    unsafe var ctx : SSLContext
    ssl_init(&raw mut ctx)
    ssl_set_config(&raw mut ctx, &raw mut cfg)

    // Derive handshake keys first (prerequisite)
    unsafe var shared_secret : [32]u8
    unsafe var transcript_hash : [32]u8
    var i : size_t = 0
    while(i < 32) {
        shared_secret[i] = i as u8
        transcript_hash[i] = (i + 0x55) as u8
        i += 1
    }

    var ret = tls13_derive_handshake_keys(&raw mut ctx,
                                           &raw shared_secret[0], 32,
                                           &raw transcript_hash[0])
    if(ret < 0) {
        env.error("handshake key derivation should succeed")
        return
    }

    // Save handshake keys for comparison
    unsafe var hs_key_enc : [16]u8
    unsafe var hs_key_dec : [16]u8
    i = 0
    while(i < 16) {
        hs_key_enc[i] = ctx.transform_out.key_enc[i]
        hs_key_dec[i] = ctx.transform_in.key_dec[i]
        i += 1
    }

    // Now derive application keys
    unsafe var hs_hash : [32]u8
    i = 0
    while(i < 32) {
        hs_hash[i] = (i + 0xAA) as u8
        i += 1
    }

    ret = tls13_derive_application_keys(&raw mut ctx,
                                         &raw hs_hash[0], 32)
    if(ret < 0) {
        env.error("tls13_derive_application_keys should succeed")
        return
    }

    // Verify transforms still exist (old ones freed, new ones allocated)
    if(ctx.transform_out == null) {
        env.error("transform_out should be non-null after application key derivation")
        return
    }
    if(ctx.transform_in == null) {
        env.error("transform_in should be non-null after application key derivation")
        return
    }

    // Application keys should differ from handshake keys
    if(bytes_eq(&raw ctx.transform_out.key_enc[0], &raw hs_key_enc[0], 16)) {
        env.error("application client key should differ from handshake client key")
    }
    if(bytes_eq(&raw ctx.transform_in.key_dec[0], &raw hs_key_dec[0], 16)) {
        env.error("application server key should differ from handshake server key")
    }

    // Application keys should be non-zero
    if(!bytes_not_all_zero(&raw ctx.transform_out.key_enc[0], 16)) {
        env.error("application client key should be non-zero")
    }
    if(!bytes_not_all_zero(&raw ctx.transform_in.key_dec[0], 16)) {
        env.error("application server key should be non-zero")
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// TLS 1.3 Record Layer Encrypt/Decrypt Tests
// ═══════════════════════════════════════════════════════════════════════════

@test
public func tls13_encrypt_decrypt_roundtrip_works(env : &mut TestEnv) {
    var ctx = setup_symmetric_record_test()

    // Plaintext data
    var pt : [24]u8 = [
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    ]

    // Encrypt
    unsafe var output : [256]u8
    var enc_len = tls13_encrypt_record(&raw mut ctx,
                                        SSL_MSG_APPLICATION_DATA as u8,
                                        &raw pt[0], 24,
                                        &raw mut output[0], 256)
    if(enc_len < 0) {
        env.error("tls13_encrypt_record should succeed")
        return
    }

    // TLS 1.3 record: 5-byte header + inner(24+1) + 16 byte tag = 46
    if(enc_len != 46) {
        env.error("TLS 1.3 encrypted record length should be 46 (5 hdr + 25 inner + 16 tag)")
        return
    }

    // Verify outer header fields
    if(output[0] != SSL_MSG_APPLICATION_DATA as u8) {
        env.error("outer content_type should be APPLICATION_DATA (23)")
    }
    if(output[1] != 0x03 || output[2] != 0x03) {
        env.error("outer legacy version should be 0x0303")
    }
    // length field should equal inner_len + 16 (tag)
    var record_len = ((output[3] as u16) << 8) | (output[4] as u16)
    if(record_len != 41) {
        env.error("record length should be 41 (25 inner + 16 tag)")
    }

    // Set up in_hdr for decryption (must match the 5-byte outer header)
    ctx.in_hdr[0] = output[0]
    ctx.in_hdr[1] = output[1]
    ctx.in_hdr[2] = output[2]
    ctx.in_hdr[3] = output[3]
    ctx.in_hdr[4] = output[4]

    // in_ctr must match out_ctr at encryption time (both started at 0)
    var i : size_t = 0
    while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    // Decrypt: pass encrypted data without the 5-byte header
    unsafe var decrypted : [256]u8
    var inner_ct : u8 = 0
    var dec_len = tls13_decrypt_record(&raw mut ctx,
                                        &raw output[5], (enc_len - 5) as size_t,
                                        &raw mut decrypted[0], 256,
                                        &raw mut inner_ct)
    if(dec_len < 0) {
        env.error("tls13_decrypt_record should succeed")
        return
    }

    // Decrypted length should be 24 (original plaintext, content_type byte removed)
    if(dec_len != 24) {
        env.error("decrypted length should be 24")
        return
    }

    // Verify inner content_type was extracted
    if(inner_ct != SSL_MSG_APPLICATION_DATA as u8) {
        env.error("inner content_type should be APPLICATION_DATA (23)")
    }

    // Verify decrypted plaintext matches
    if(!bytes_eq(&raw decrypted[0], &raw pt[0], 24)) {
        env.error("decrypted plaintext should match original")
    }
}

@test
public func tls13_encrypt_decrypt_multiple_records(env : &mut TestEnv) {
    var ctx = setup_symmetric_record_test()

    // Record 1: "Hello, "
    var pt1 : [7]u8 = [0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20]
    unsafe var buf1 : [256]u8
    var len1 = tls13_encrypt_record(&raw mut ctx, SSL_MSG_APPLICATION_DATA as u8,
                                    &raw pt1[0], 7, &raw mut buf1[0], 256)
    if(len1 < 0) { env.error("encrypt record 1 should succeed"); return }

    // Record 2: "World!"
    var pt2 : [6]u8 = [0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21]
    unsafe var buf2 : [256]u8
    var len2 = tls13_encrypt_record(&raw mut ctx, SSL_MSG_APPLICATION_DATA as u8,
                                    &raw pt2[0], 6, &raw mut buf2[0], 256)
    if(len2 < 0) { env.error("encrypt record 2 should succeed"); return }

    // Two records must differ (different sequence numbers → different nonces)
    if(len1 == len2 && bytes_eq(&raw buf1[5], &raw buf2[5], (len1 - 5) as size_t)) {
        env.error("two records encrypted with different sequence numbers should differ")
    }

    // Decrypt record 1 (in_ctr = 0, matching out_ctr=0 at encryption time)
    ctx.in_hdr[0] = buf1[0]; ctx.in_hdr[1] = buf1[1]; ctx.in_hdr[2] = buf1[2]
    ctx.in_hdr[3] = buf1[3]; ctx.in_hdr[4] = buf1[4]
    var i : size_t = 0
    while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    unsafe var dec1 : [256]u8
    var inner_ct1 : u8 = 0
    var dlen1 = tls13_decrypt_record(&raw mut ctx, &raw buf1[5], (len1 - 5) as size_t,
                                      &raw mut dec1[0], 256, &raw mut inner_ct1)
    if(dlen1 < 0) { env.error("decrypt record 1 should succeed"); return }
    if(dlen1 != 7) { env.error("decrypted length 1 should be 7"); return }
    if(!bytes_eq(&raw dec1[0], &raw pt1[0], 7)) {
        env.error("decrypted record 1 should match original")
    }

    // Decrypt record 2 (in_ctr incremented to 1 by previous decrypt,
    // matching out_ctr=1 at encryption time)
    ctx.in_hdr[0] = buf2[0]; ctx.in_hdr[1] = buf2[1]; ctx.in_hdr[2] = buf2[2]
    ctx.in_hdr[3] = buf2[3]; ctx.in_hdr[4] = buf2[4]

    unsafe var dec2 : [256]u8
    var inner_ct2 : u8 = 0
    var dlen2 = tls13_decrypt_record(&raw mut ctx, &raw buf2[5], (len2 - 5) as size_t,
                                      &raw mut dec2[0], 256, &raw mut inner_ct2)
    if(dlen2 < 0) { env.error("decrypt record 2 should succeed"); return }
    if(dlen2 != 6) { env.error("decrypted length 2 should be 6"); return }
    if(!bytes_eq(&raw dec2[0], &raw pt2[0], 6)) {
        env.error("decrypted record 2 should match original")
    }
}

@test
public func tls13_encrypt_decrypt_different_content_types(env : &mut TestEnv) {
    var ctx = setup_symmetric_record_test()

    var test_data : [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF]
    var content_types : [3]u8 = [
        SSL_MSG_HANDSHAKE as u8,
        SSL_MSG_APPLICATION_DATA as u8,
        SSL_MSG_ALERT as u8
    ]

    // Encrypt and decrypt each content type in sequence.
    // Sequence numbers stay in lockstep: encrypt->out_ctr, decrypt->in_ctr
    var ct_idx : size_t = 0
    while(ct_idx < 3) {
        var ct = content_types[ct_idx]

        unsafe var enc_out : [256]u8
        var enc_len = tls13_encrypt_record(&raw mut ctx, ct,
                                            &raw test_data[0], 4,
                                            &raw mut enc_out[0], 256)
        if(enc_len < 0) {
            env.error("encrypt with content_type should succeed")
            return
        }

        // Set up header for decryption
        ctx.in_hdr[0] = enc_out[0]; ctx.in_hdr[1] = enc_out[1]
        ctx.in_hdr[2] = enc_out[2]; ctx.in_hdr[3] = enc_out[3]
        ctx.in_hdr[4] = enc_out[4]

        // Decrypt (in_ctr auto-incremented from previous decrypt, matches out_ctr)
        unsafe var dec : [256]u8
        var inner_ct : u8 = 0
        var dec_len = tls13_decrypt_record(&raw mut ctx,
                                            &raw enc_out[5], (enc_len - 5) as size_t,
                                            &raw mut dec[0], 256, &raw mut inner_ct)
        if(dec_len < 0) {
            env.error("decrypt with content_type should succeed")
            return
        }
        if(dec_len != 4) {
            env.error("decrypted length should be 4")
            return
        }
        if(inner_ct != ct) {
            env.error("inner content_type should match what was encrypted")
        }
        if(!bytes_eq(&raw dec[0], &raw test_data[0], 4)) {
            env.error("decrypted data should match original")
        }

        ct_idx += 1
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// GCM Authentication Failure Test (TLS 1.3)
// ═══════════════════════════════════════════════════════════════════════════

@test
public func tls13_decrypt_tampered_ct_fails(env : &mut TestEnv) {
    var ctx = setup_symmetric_record_test()

    // Encrypt
    var pt : [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11]
    unsafe var enc_out : [256]u8
    var enc_len = tls13_encrypt_record(&raw mut ctx, SSL_MSG_APPLICATION_DATA as u8,
                                        &raw pt[0], 8, &raw mut enc_out[0], 256)
    if(enc_len < 0) { env.error("encrypt should succeed"); return }

    // Tamper with one byte of the ciphertext
    // ciphertext starts at offset 5, inner_len=9 (8 data + 1 ct), tag=16
    var tamper_pos : size_t = (5 + 4) as size_t  // middle of ciphertext data
    enc_out[tamper_pos] = enc_out[tamper_pos] ^ 0xFF

    // Set up decrypt
    ctx.in_hdr[0] = enc_out[0]; ctx.in_hdr[1] = enc_out[1]
    ctx.in_hdr[2] = enc_out[2]; ctx.in_hdr[3] = enc_out[3]
    ctx.in_hdr[4] = enc_out[4]
    var i : size_t = 0
    while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    // Decrypt should fail (GCM authentication tag mismatch)
    unsafe var dec : [256]u8
    var inner_ct : u8 = 0
    var dec_len = tls13_decrypt_record(&raw mut ctx,
                                        &raw enc_out[5], (enc_len - 5) as size_t,
                                        &raw mut dec[0], 256, &raw mut inner_ct)
    if(dec_len >= 0) {
        env.error("decrypt of tampered TLS 1.3 record should fail (GCM auth)")
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// TLS 1.3 Key Update Test
// ═══════════════════════════════════════════════════════════════════════════

@test
public func tls13_update_send_keys_works(env : &mut TestEnv) {
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.authmode = SSL_VERIFY_NONE

    unsafe var ctx : SSLContext
    ssl_init(&raw mut ctx)
    ssl_set_config(&raw mut ctx, &raw mut cfg)

    // Derive handshake keys
    unsafe var shared_secret : [32]u8
    unsafe var transcript_hash : [32]u8
    var i : size_t = 0
    while(i < 32) {
        shared_secret[i] = i as u8
        transcript_hash[i] = (i + 0x55) as u8
        i += 1
    }
    var ret = tls13_derive_handshake_keys(&raw mut ctx,
                                           &raw shared_secret[0], 32,
                                           &raw transcript_hash[0])
    if(ret < 0) { env.error("handshake key derivation should succeed"); return }

    // Derive application keys (prerequisite for key update per RFC 8446)
    unsafe var hs_hash : [32]u8
    i = 0
    while(i < 32) {
        hs_hash[i] = (i + 0xAA) as u8
        i += 1
    }
    ret = tls13_derive_application_keys(&raw mut ctx, &raw hs_hash[0], 32)
    if(ret < 0) { env.error("application key derivation should succeed"); return }

    // Capture pre-update keys
    unsafe var pre_key_enc : [16]u8
    unsafe var pre_iv_enc : [12]u8
    i = 0
    while(i < 16) {
        pre_key_enc[i] = ctx.transform_out.key_enc[i]
        if(i < 12) { pre_iv_enc[i] = ctx.transform_out.base_iv_enc[i] }
        i += 1
    }

    // Perform key update
    ret = tls13_update_send_keys(&raw mut ctx)
    if(ret < 0) {
        env.error("tls13_update_send_keys should succeed")
        return
    }

    // Verify keys changed
    if(bytes_eq(&raw ctx.transform_out.key_enc[0], &raw pre_key_enc[0], 16)) {
        env.error("send key should change after key update")
    }
    if(bytes_eq(&raw ctx.transform_out.base_iv_enc[0], &raw pre_iv_enc[0], 12)) {
        env.error("send IV should change after key update")
    }

    // Verify new keys are non-zero
    if(!bytes_not_all_zero(&raw ctx.transform_out.key_enc[0], 16)) {
        env.error("new send key should be non-zero")
    }
    if(!bytes_not_all_zero(&raw ctx.transform_out.base_iv_enc[0], 12)) {
        env.error("new send IV should be non-zero")
    }

    // After key update, sequence numbers should be reset
    var seq_all_zero = true
    i = 0
    while(i < 8) {
        if(ctx.out_ctr[i] != 0) { seq_all_zero = false }
        i += 1
    }
    if(!seq_all_zero) {
        env.error("send sequence number should be reset after key update")
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Close Notify Without Socket (should not crash)
// ═══════════════════════════════════════════════════════════════════════════

@test
public func tls_ssl_close_notify_no_socket_works(env : &mut TestEnv) {
    unsafe var ctx : SSLContext
    ssl_init(&raw mut ctx)
    // No socket or config set — just ensures ssl_close_notify doesn't crash

    ssl_close_notify(&raw mut ctx)
}

// ═══════════════════════════════════════════════════════════════════════════
// SSL Config Null Fields Test
// ═══════════════════════════════════════════════════════════════════════════

@test
public func tls_ssl_set_ca_chain_null_works(env : &mut TestEnv) {
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    // ca_chain should be null by default
    if(cfg.ca_chain != null) {
        env.error("new config ca_chain should be null")
        return
    }
    // ssl_set_ca_chain with null should be a no-op
    ssl_set_ca_chain(&raw mut cfg, null)
    if(cfg.ca_chain != null) {
        env.error("ca_chain should remain null after setting to null")
    }
}
