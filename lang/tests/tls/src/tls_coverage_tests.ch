// ============================================================================
// TLS Coverage Tests — filling gaps in the existing test suite
// ============================================================================

using namespace tls
using namespace crypto
using std::string
using std::string_view
using std::vector

// ─── Helper ────────────────────────────────────────────────────────────────

func tc_bytes_eq(a : *u8, b : *u8, len : size_t) : bool {
    var i : size_t = 0
    while(i < len) { if(a[i] != b[i]) { return false }; i += 1 }
    return true
}

func tc_bytes_not_zero(buf : *u8, len : size_t) : bool {
    var i : size_t = 0
    while(i < len) { if(buf[i] != 0) { return true }; i += 1 }
    return false
}

// ============================================================================
// SECTION 1: ChaCha20-Poly1305 Tests (via TLS 1.3 record layer)
// ============================================================================

func setup_chacha_record_test() : SSLContext {
    var ctx : SSLContext
    ssl_init(unsafe(&raw mut ctx))

    var key : [32]u8
    var i : size_t = 0
    while(i < 32) { key[i] = (i * 7 + 13) as u8; i += 1 }

    var iv : [12]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0a, 0x0b]

    var tr : Transform
    transform_init(unsafe(&raw mut tr))
    tr.cipher_type = CIPHER_CHACHA20_POLY1305 as u8
    tr.key_len = 32
    tr.iv_len = 12
    tr.fixed_iv_len = 12
    tr.mac_key_len = 0

    i = 0
    while(i < 32) { tr.key_enc[i] = key[i]; tr.key_dec[i] = key[i]; i += 1 }
    i = 0
    while(i < 12) { tr.base_iv_enc[i] = iv[i]; tr.base_iv_dec[i] = iv[i]; i += 1 }

    var tr_out = malloc(sizeof(Transform)) as *mut Transform
    *tr_out = tr
    unsafe { ctx.transform_out = tr_out }

    var tr_in = malloc(sizeof(Transform)) as *mut Transform
    *tr_in = tr
    unsafe { ctx.transform_in = tr_in }

    i = 0
    while(i < 8) { ctx.in_ctr[i] = 0; ctx.out_ctr[i] = 0; i += 1 }

    return unsafe(ctx)
}

@test
public func CHACHA_encrypt_decrypt_roundtrip_works(env : &mut TestEnv) {
    var ctx = setup_chacha_record_test()

    var pt : [16]u8 = [0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x43, 0x68,
                        0x61, 0x43, 0x68, 0x61, 0x21, 0x21, 0x21, 0x00]
    var output : [256]u8
    var enc_len = tls13_encrypt_record(unsafe(&raw mut ctx),
                                        SSL_MSG_APPLICATION_DATA as u8,
                                        &raw pt[0], 16,
                                        &raw mut output[0], 256)
    if(enc_len < 0) { env.error("chacha encrypt should succeed"); return }

    if(enc_len != 38) {
        env.error("chacha encrypted record length should be 38")
        return
    }

    ctx.in_hdr[0] = output[0]; ctx.in_hdr[1] = output[1]; ctx.in_hdr[2] = output[2]
    ctx.in_hdr[3] = output[3]; ctx.in_hdr[4] = output[4]
    var i : size_t = 0
    while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    var decrypted : [256]u8
    var inner_ct : u8 = 0
    var dec_len = tls13_decrypt_record(unsafe(&raw mut ctx),
                                        &raw output[5], (enc_len - 5) as size_t,
                                        &raw mut decrypted[0], 256,
                                        &raw mut inner_ct)
    if(dec_len < 0) { env.error("chacha decrypt should succeed"); return }
    if(dec_len != 16) { env.error("decrypted length should be 16"); return }
    if(!tc_bytes_eq(&raw decrypted[0], &raw pt[0], 16)) {
        env.error("chacha decrypted should match original")
    }
}

@test
public func CHACHA_tampered_ct_fails(env : &mut TestEnv) {
    var ctx = setup_chacha_record_test()

    var pt : [8]u8 = [0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE]
    var enc_out : [256]u8
    var enc_len = tls13_encrypt_record(unsafe(&raw mut ctx),
                                        SSL_MSG_APPLICATION_DATA as u8,
                                        &raw pt[0], 8,
                                        &raw mut enc_out[0], 256)
    if(enc_len < 0) { env.error("chacha encrypt should succeed"); return }

    enc_out[7] = enc_out[7] ^ 0xFF

    ctx.in_hdr[0] = enc_out[0]; ctx.in_hdr[1] = enc_out[1]; ctx.in_hdr[2] = enc_out[2]
    ctx.in_hdr[3] = enc_out[3]; ctx.in_hdr[4] = enc_out[4]
    var i : size_t = 0
    while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    var dec : [256]u8
    var inner_ct : u8 = 0
    var dec_len = tls13_decrypt_record(unsafe(&raw mut ctx),
                                        &raw enc_out[5], (enc_len - 5) as size_t,
                                        &raw mut dec[0], 256,
                                        &raw mut inner_ct)
    if(dec_len >= 0) {
        env.error("chacha decrypt of tampered CT should fail")
    }
}

@test
public func CHACHA_tampered_tag_fails(env : &mut TestEnv) {
    var ctx = setup_chacha_record_test()

    var pt : [4]u8 = [0x11, 0x22, 0x33, 0x44]
    var enc_out : [256]u8
    var enc_len = tls13_encrypt_record(unsafe(&raw mut ctx),
                                        SSL_MSG_APPLICATION_DATA as u8,
                                        &raw pt[0], 4,
                                        &raw mut enc_out[0], 256)
    if(enc_len < 0) { env.error("encrypt should succeed"); return }

    enc_out[(enc_len - 1) as size_t] = enc_out[(enc_len - 1) as size_t] ^ 0xFF

    ctx.in_hdr[0] = enc_out[0]; ctx.in_hdr[1] = enc_out[1]; ctx.in_hdr[2] = enc_out[2]
    ctx.in_hdr[3] = enc_out[3]; ctx.in_hdr[4] = enc_out[4]
    var i : size_t = 0
    while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    var dec : [256]u8
    var inner_ct : u8 = 0
    var dec_len = tls13_decrypt_record(unsafe(&raw mut ctx),
                                        &raw enc_out[5], (enc_len - 5) as size_t,
                                        &raw mut dec[0], 256,
                                        &raw mut inner_ct)
    if(dec_len >= 0) {
        env.error("chacha decrypt of tampered tag should fail")
    }
}

@test
public func CHACHA_empty_plaintext_works(env : &mut TestEnv) {
    var ctx = setup_chacha_record_test()

    var output : [256]u8
    var enc_len = tls13_encrypt_record(unsafe(&raw mut ctx),
                                        SSL_MSG_APPLICATION_DATA as u8,
                                        null, 0,
                                        &raw mut output[0], 256)
    if(enc_len < 0) { env.error("chacha encrypt of empty should succeed"); return }

    if(enc_len != 22) {
        env.error("chacha empty record length should be 22")
    }

    ctx.in_hdr[0] = output[0]; ctx.in_hdr[1] = output[1]; ctx.in_hdr[2] = output[2]
    ctx.in_hdr[3] = output[3]; ctx.in_hdr[4] = output[4]
    var i : size_t = 0
    while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    var dec : [256]u8
    var inner_ct : u8 = 0
    var dec_len = tls13_decrypt_record(unsafe(&raw mut ctx),
                                        &raw output[5], (enc_len - 5) as size_t,
                                        &raw mut dec[0], 256,
                                        &raw mut inner_ct)
    if(dec_len != 0) { env.error("chacha decrypt empty should return 0"); return }
}

@test
public func CHACHA_large_payload_works(env : &mut TestEnv) {
    var ctx = setup_chacha_record_test()

    var pt : [16384]u8
    var i : size_t = 0
    while(i < 16384) { pt[i] = (i % 251) as u8; i += 1 }

    var output : [20000]u8
    var enc_len = tls13_encrypt_record(unsafe(&raw mut ctx),
                                        SSL_MSG_APPLICATION_DATA as u8,
                                        &raw pt[0], 16384,
                                        &raw mut output[0], 20000)
    if(enc_len < 0) { env.error("chacha encrypt large should succeed"); return }

    ctx.in_hdr[0] = output[0]; ctx.in_hdr[1] = output[1]; ctx.in_hdr[2] = output[2]
    ctx.in_hdr[3] = output[3]; ctx.in_hdr[4] = output[4]
    i = 0
    while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    var dec : [16384]u8
    var inner_ct : u8 = 0
    var dec_len = tls13_decrypt_record(unsafe(&raw mut ctx),
                                        &raw output[5], (enc_len - 5) as size_t,
                                        &raw mut dec[0], 16384,
                                        &raw mut inner_ct)
    if(dec_len != 16384) {
        env.error("chacha decrypt large length mismatch")
        return
    }
    if(!tc_bytes_eq(&raw dec[0], &raw pt[0], 16384)) {
        env.error("chacha large payload roundtrip failed")
    }
}

@test
public func CHACHA_multiple_records_different_nonces(env : &mut TestEnv) {
    var ctx = setup_chacha_record_test()

    var pt1 : [4]u8 = [0xAA, 0xBB, 0xCC, 0xDD]
    var pt2 : [4]u8 = [0x11, 0x22, 0x33, 0x44]

    var buf1 : [256]u8
    var buf2 : [256]u8
    var len1 = tls13_encrypt_record(unsafe(&raw mut ctx), SSL_MSG_APPLICATION_DATA as u8,
                                    &raw pt1[0], 4, &raw mut buf1[0], 256)
    if(len1 < 0) { env.error("encrypt record 1 should succeed"); return }

    var len2 = tls13_encrypt_record(unsafe(&raw mut ctx), SSL_MSG_APPLICATION_DATA as u8,
                                    &raw pt2[0], 4, &raw mut buf2[0], 256)
    if(len2 < 0) { env.error("encrypt record 2 should succeed"); return }

    if(len1 == len2 && tc_bytes_eq(&raw buf1[5], &raw buf2[5], (len1 - 5) as size_t)) {
        env.error("chacha records with different seq nums should differ")
    }
}

// ============================================================================
// SECTION 2: ALPN Tests
// ============================================================================

@test
public func ALPN_set_works(env : &mut TestEnv) {
    var h2 = "h2\0" as *char
    var http11 = "http/1.1\0" as *char
    var protocols : [2]*char = [h2, http11]

    var config = ssl_config_init(SSL_IS_CLIENT)
    ssl_set_alpn_protocols(&raw mut config, &raw mut protocols[0], 2)

    if(config.alpn_count != 2) {
        env.error("alpn_count should be 2")
    }
}

@test
public func ALPN_negotiated_default_null(env : &mut TestEnv) {
    var ctx : SSLContext
    ssl_init(unsafe(&raw mut ctx))

    var negotiated = ssl_get_alpn_negotiated(unsafe(&raw mut ctx))
    if(negotiated != null) {
        env.error("alpn_negotiated should be null before handshake")
    }
}

// ============================================================================
// SECTION 3: SSL Server Config Tests
// ============================================================================

@test
@test.timeout(30000)
public func SERVER_CONFIG_set_own_rsa_key_works(env : &mut TestEnv) {
    var rsa_ctx : RSAContext
    rsa_init(unsafe(&raw mut rsa_ctx), RSA_PKCS_V15, 0)
    var ret = rsa_gen_key(unsafe(&raw mut rsa_ctx), 2048, 65537)
    if(ret < 0) { env.error("rsa_gen_key failed"); return }

    var config = ssl_config_init(SSL_IS_SERVER)
    ssl_set_own_rsa_key(&raw mut config, &raw mut rsa_ctx)

    if(unsafe(config.own_key) == null) {
        env.error("own_key should be set after ssl_set_own_rsa_key")
    }
}

@test
public func SERVER_CONFIG_default_authmode_works(env : &mut TestEnv) {
    var config = ssl_config_init(SSL_IS_SERVER)
    if(config.authmode != SSL_VERIFY_REQUIRED) {
        env.error("server default authmode should be VERIFY_REQUIRED")
    }
    if(config.endpoint != SSL_IS_SERVER) {
        env.error("endpoint should be SERVER")
    }
    if(config.session_tickets != 1) {
        env.error("session_tickets should be enabled by default")
    }
    if(config.extended_ms != 1) {
        env.error("extended_ms should be enabled by default")
    }
}

// ============================================================================
// SECTION 4: Random Number Generation Tests
// ============================================================================

@test
public func RANDOM_fill_nonzero_works(env : &mut TestEnv) {
    var buf : [32]u8
    var ret = random_fill(&raw mut buf[0], 32)
    if(ret < 0) { env.error("random_fill failed"); return }
    if(!tc_bytes_not_zero(&raw buf[0], 32)) {
        env.error("random_fill should produce non-zero bytes")
    }
}

@test
public func RANDOM_fill_different_calls_differ(env : &mut TestEnv) {
    var buf1 : [32]u8
    var buf2 : [32]u8
    random_fill(&raw mut buf1[0], 32)
    random_fill(&raw mut buf2[0], 32)

    var all_same = true
    var i : size_t = 0
    while(i < 32) { if(buf1[i] != buf2[i]) { all_same = false }; i += 1 }
    if(all_same) {
        env.error("two random_fill calls should not produce identical output")
    }
}

@test
public func RANDOM_32_fill_works(env : &mut TestEnv) {
    var buf : [32]u8
    var ret = random_32(unsafe(&raw mut buf))
    if(ret < 0) { env.error("random_32 failed"); return }
    if(!tc_bytes_not_zero(&raw buf[0], 32)) {
        env.error("random_32 should produce non-zero bytes")
    }
}

@test
public func RANDOM_48_fill_works(env : &mut TestEnv) {
    var buf : [48]u8
    var ret = random_48(unsafe(&raw mut buf))
    if(ret < 0) { env.error("random_48 failed"); return }
    if(!tc_bytes_not_zero(&raw buf[0], 48)) {
        env.error("random_48 should produce non-zero bytes")
    }
}

@test
public func RANDOM_32bit_varies(env : &mut TestEnv) {
    var v1 = random_32bit()
    var v2 = random_32bit()
    var v3 = random_32bit()
    if(v1 == v2 && v2 == v3) {
        env.error("random_32bit should produce varying values")
    }
}

// ============================================================================
// SECTION 5: Certificate Date Verification Tests
// ============================================================================

@test
public func CERT_DATE_check_works(env : &mut TestEnv) {
    var cert : X509Cert
    x509_cert_init(unsafe(&raw mut cert))

    var ret = parse_cert_der(unsafe(&raw mut cert), &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    ret = x509_check_date(unsafe(&raw mut cert))
    if(ret < 0) {
        env.error("x509_check_date should pass for our test cert")
    }
}

// ============================================================================
// SECTION 6: GCM Edge Case Tests
// ============================================================================

@test
public func GCM_empty_plaintext_roundtrip(env : &mut TestEnv) {
    var key : [16]u8 = [0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c]
    var iv : [12]u8 = [0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
                        0xde, 0xca, 0xf8, 0x88]

    var gcm_ctx : GCMContext
    gcm_init(unsafe(&raw mut gcm_ctx), &raw key[0], 16)

    var ct : [1]u8
    var tag : [16]u8
    var ret = gcm_crypt_and_tag(unsafe(&raw mut gcm_ctx), &raw mut iv[0], 12,
                                 null, 0, null, 0,
                                 &raw mut ct[0], &raw mut tag[0])
    if(ret < 0) { env.error("GCM encrypt empty should succeed"); return }

    if(!tc_bytes_not_zero(&raw tag[0], 16)) {
        env.error("GCM tag for empty plaintext should be non-zero")
    }

    var gcm_ctx2 : GCMContext
    gcm_init(unsafe(&raw mut gcm_ctx2), &raw key[0], 16)
    var dec : [1]u8
    ret = gcm_auth_decrypt(unsafe(&raw mut gcm_ctx2), &raw mut iv[0], 12,
                            null, 0, &raw mut ct[0], 0,
                            &raw mut tag[0], 16, &raw mut dec[0])
    if(ret < 0) { env.error("GCM decrypt empty should succeed"); return }
}

@test
public func GCM_wrong_key_fails(env : &mut TestEnv) {
    var key1 : [16]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10]
    var key2 : [16]u8 = [0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09,
                          0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01]
    var iv : [12]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0A, 0x0B]
    var pt : [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11]

    var gcm1 : GCMContext
    gcm_init(unsafe(&raw mut gcm1), &raw key1[0], 16)
    var ct : [8]u8
    var tag : [16]u8
    gcm_crypt_and_tag(unsafe(&raw mut gcm1), &raw mut iv[0], 12, null, 0,
                       &raw mut pt[0], 8, &raw mut ct[0], &raw mut tag[0])

    var gcm2 : GCMContext
    gcm_init(unsafe(&raw mut gcm2), &raw key2[0], 16)
    var dec : [8]u8
    var ret = gcm_auth_decrypt(unsafe(&raw mut gcm2), &raw mut iv[0], 12, null, 0,
                                &raw mut ct[0], 8, &raw mut tag[0], 16, &raw mut dec[0])
    if(ret == 0) {
        env.error("GCM decrypt with wrong key should fail")
    }
}

@test
public func GCM_wrong_iv_fails(env : &mut TestEnv) {
    var key : [16]u8 = [0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c]
    var iv1 : [12]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0A, 0x0B]
    var iv2 : [12]u8 = [0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8,
                         0xF7, 0xF6, 0xF5, 0xF4]
    var pt : [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11]

    var gcm1 : GCMContext
    gcm_init(unsafe(&raw mut gcm1), &raw key[0], 16)
    var ct : [8]u8
    var tag : [16]u8
    gcm_crypt_and_tag(unsafe(&raw mut gcm1), &raw mut iv1[0], 12, null, 0,
                       &raw mut pt[0], 8, &raw mut ct[0], &raw mut tag[0])

    var gcm2 : GCMContext
    gcm_init(unsafe(&raw mut gcm2), &raw key[0], 16)
    var dec : [8]u8
    var ret = gcm_auth_decrypt(unsafe(&raw mut gcm2), &raw mut iv2[0], 12, null, 0,
                                &raw mut ct[0], 8, &raw mut tag[0], 16, &raw mut dec[0])
    if(ret == 0) {
        env.error("GCM decrypt with wrong IV should fail")
    }
}

@test
public func GCM_wrong_aad_fails(env : &mut TestEnv) {
    var key : [16]u8 = [0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c]
    var iv : [12]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0A, 0x0B]
    var aad1 : [4]u8 = [0xAA, 0xBB, 0xCC, 0xDD]
    var aad2 : [4]u8 = [0x11, 0x22, 0x33, 0x44]
    var pt : [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11]

    var gcm1 : GCMContext
    gcm_init(unsafe(&raw mut gcm1), &raw key[0], 16)
    var ct : [8]u8
    var tag : [16]u8
    gcm_crypt_and_tag(unsafe(&raw mut gcm1), &raw mut iv[0], 12,
                       &raw mut aad1[0], 4, &raw mut pt[0], 8, &raw mut ct[0], &raw mut tag[0])

    var gcm2 : GCMContext
    gcm_init(unsafe(&raw mut gcm2), &raw key[0], 16)
    var dec : [8]u8
    var ret = gcm_auth_decrypt(unsafe(&raw mut gcm2), &raw mut iv[0], 12,
                                &raw mut aad2[0], 4,
                                &raw mut ct[0], 8, &raw mut tag[0], 16, &raw mut dec[0])
    if(ret == 0) {
        env.error("GCM decrypt with wrong AAD should fail")
    }
}

// ============================================================================
// SECTION 7: SHA-256 Hash Tests
// ============================================================================

@test
public func SHA256_abc_known_answer(env : &mut TestEnv) {
    var input : [3]u8
    input[0] = 'a' as u8; input[1] = 'b' as u8; input[2] = 'c' as u8
    var expected : [32]u8 = [
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    ]
    var actual : [32]u8
    sha256_hash(&raw input[0], 3, &raw mut actual[0])
    if(!tc_bytes_eq(&raw actual[0], &raw expected[0], 32)) {
        env.error("SHA-256('abc') known answer failed")
    }
}

@test
public func SHA256_empty_known_answer(env : &mut TestEnv) {
    var expected : [32]u8 = [
        0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
        0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
        0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
        0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
    ]
    var actual : [32]u8
    sha256_hash(null, 0, &raw mut actual[0])
    if(!tc_bytes_eq(&raw actual[0], &raw expected[0], 32)) {
        env.error("SHA-256('') known answer failed")
    }
}

@test
public func SHA256_deterministic(env : &mut TestEnv) {
    var input : [5]u8 = [0x01, 0x02, 0x03, 0x04, 0x05]
    var out1 : [32]u8
    var out2 : [32]u8
    sha256_hash(&raw input[0], 5, &raw mut out1[0])
    sha256_hash(&raw input[0], 5, &raw mut out2[0])
    if(!tc_bytes_eq(&raw out1[0], &raw out2[0], 32)) {
        env.error("SHA-256 should be deterministic")
    }
}

// ============================================================================
// SECTION 8: RSA PKCS#1 Encrypt/Decrypt Roundtrip
// ============================================================================

@test
@test.timeout(30000)
public func RSA_pkcs1_encrypt_decrypt_roundtrip(env : &mut TestEnv) {
    var rsa_ctx : RSAContext
    rsa_init(unsafe(&raw mut rsa_ctx), RSA_PKCS_V15, 0)
    var ret = rsa_gen_key(unsafe(&raw mut rsa_ctx), 2048, 65537)
    if(ret < 0) { env.error("rsa_gen_key failed"); return }

    var msg : [32]u8
    var i : size_t = 0
    while(i < 32) { msg[i] = (i as u8 + 0x41); i += 1 }

    var ct : [256]u8
    ret = rsa_pkcs1_encrypt(unsafe(&raw mut rsa_ctx), &raw msg[0], 32, &raw mut ct[0])
    if(ret < 0) { env.error("rsa_pkcs1_encrypt failed"); return }

    var same = true
    i = 0
    while(i < 32) { if(ct[i] != msg[i]) { same = false }; i += 1 }
    if(same) { env.error("RSA ciphertext should differ from plaintext") }

    var dec : [256]u8
    var dec_len : size_t = 256
    ret = rsa_pkcs1_decrypt(unsafe(&raw mut rsa_ctx), &raw ct[0], 256,
                             &raw mut dec[0], &raw mut dec_len, 32)
    if(ret < 0) { env.error("rsa_pkcs1_decrypt failed"); return }
    if(dec_len != 32) { env.error("decrypted length should be 32"); return }

    if(!tc_bytes_eq(&raw dec[0], &raw msg[0], 32)) {
        env.error("RSA decrypt should recover original message")
    }
}

// ============================================================================
// SECTION 9: RSA Key Consistency
// ============================================================================

@test
@test.timeout(30000)
public func RSA_keygen_consistent_len_works(env : &mut TestEnv) {
    var rsa_ctx : RSAContext
    rsa_init(unsafe(&raw mut rsa_ctx), RSA_PKCS_V15, 0)
    var ret = rsa_gen_key(unsafe(&raw mut rsa_ctx), 2048, 65537)
    if(ret < 0) { env.error("rsa_gen_key 2048 failed"); return }

    var len = rsa_get_len(unsafe(&raw mut rsa_ctx))
    if(len != 256) { env.error("RSA-2048 key length should be 256"); return }
}

// ============================================================================
// SECTION 10: BigNum Additional Tests
// ============================================================================

@test
public func BIGNUM_copy_works(env : &mut TestEnv) {
    var a : Mpi; mpi_init(unsafe(&raw mut a))
    mpi_lset(unsafe(&raw mut a), 12345)
    var b : Mpi; mpi_init(unsafe(&raw mut b))
    mpi_copy(unsafe(&raw mut b), unsafe(&raw mut a))
    if(mpi_cmp_int(unsafe(&raw mut b), 12345) != 0) {
        env.error("copy should preserve value")
    }
}

@test
public func BIGNUM_shift_left_right_roundtrip(env : &mut TestEnv) {
    var a : Mpi; mpi_init(unsafe(&raw mut a))
    mpi_lset(unsafe(&raw mut a), 42)
    mpi_shift_l(unsafe(&raw mut a), 8)
    if(mpi_cmp_int(unsafe(&raw mut a), 10752) != 0) {
        env.error("shift left 8 should give 10752")
    }
    mpi_shift_r(unsafe(&raw mut a), 8)
    if(mpi_cmp_int(unsafe(&raw mut a), 42) != 0) {
        env.error("shift right 8 should give back 42")
    }
}

@test
public func BIGNUM_gcd_works(env : &mut TestEnv) {
    var a : Mpi; mpi_init(unsafe(&raw mut a))
    var b : Mpi; mpi_init(unsafe(&raw mut b))
    var g : Mpi; mpi_init(unsafe(&raw mut g))
    mpi_lset(unsafe(&raw mut a), 48)
    mpi_lset(unsafe(&raw mut b), 18)
    mpi_gcd(unsafe(&raw mut g), unsafe(&raw mut a), unsafe(&raw mut b))
    if(mpi_cmp_int(unsafe(&raw mut g), 6) != 0) {
        env.error("gcd(48, 18) should be 6")
    }
}

@test
public func BIGNUM_is_zero_works(env : &mut TestEnv) {
    var a : Mpi; mpi_init(unsafe(&raw mut a))
    mpi_lset(unsafe(&raw mut a), 0)
    if(!mpi_is_zero(unsafe(&raw mut a))) {
        env.error("0 should be zero")
    }
    mpi_lset(unsafe(&raw mut a), 1)
    if(mpi_is_zero(unsafe(&raw mut a))) {
        env.error("1 should not be zero")
    }
}

// ============================================================================
// SECTION 11: SSL Context Edge Cases
// ============================================================================

@test
public func SSL_init_multiple_times_works(env : &mut TestEnv) {
    var ctx : SSLContext
    ssl_init(unsafe(&raw mut ctx))
    ssl_init(unsafe(&raw mut ctx))
    if(unsafe(ctx.transport_connected)) {
        env.error("re-initialized context should not be connected")
    }
}

@test
public func SSL_config_multiple_endpoints(env : &mut TestEnv) {
    var client_cfg = ssl_config_init(SSL_IS_CLIENT)
    if(client_cfg.endpoint != SSL_IS_CLIENT) {
        env.error("client config endpoint mismatch")
    }

    var server_cfg = ssl_config_init(SSL_IS_SERVER)
    if(server_cfg.endpoint != SSL_IS_SERVER) {
        env.error("server config endpoint mismatch")
    }

    if(client_cfg.transport != SSL_TRANSPORT_STREAM) {
        env.error("client transport should be stream")
    }
    if(server_cfg.transport != SSL_TRANSPORT_STREAM) {
        env.error("server transport should be stream")
    }
}

@test
public func SSL_hostname_set_works(env : &mut TestEnv) {
    var ctx : SSLContext
    ssl_init(unsafe(&raw mut ctx))
    ssl_set_hostname(unsafe(&raw mut ctx), "myserver.example.com\0" as *char)
    if(unsafe(ctx.hostname_len) == 0) {
        env.error("hostname_len should be non-zero")
    }
}

@test
public func SSL_close_notify_after_free_works(env : &mut TestEnv) {
    var ctx : SSLContext
    ssl_init(unsafe(&raw mut ctx))
    ssl_close_notify(unsafe(&raw mut ctx))
    ssl_free(unsafe(&raw mut ctx))
}

// ============================================================================
// SECTION 12: X.509 Certificate Additional Tests
// ============================================================================

@test
public func X509_cert_init_defaults_works(env : &mut TestEnv) {
    var cert : X509Cert
    x509_cert_init(unsafe(&raw mut cert))

    if(unsafe(cert.version) != 0) { env.error("version should be 0") }
    if(unsafe(cert.pk_type) != PK_NONE as u8) { env.error("pk_type should be NONE") }
    if(unsafe(cert.ext_is_ca)) { env.error("is_ca should be false") }
    if(unsafe(cert.ext_max_pathlen) != -1) { env.error("max_pathlen should be -1") }
    if(unsafe(cert.san_entries) != null) { env.error("san_entries should be null") }
    if(unsafe(cert.san_count) != 0) { env.error("san_count should be 0") }
    if(unsafe(cert.next) != null) { env.error("next should be null") }
}

@test
public func X509_parse_der_fields_works(env : &mut TestEnv) {
    var cert : X509Cert
    x509_cert_init(unsafe(&raw mut cert))

    var ret = parse_cert_der(unsafe(&raw mut cert), &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    if(unsafe(cert.subject.size()) == 0) { env.error("subject should not be empty") }
    if(unsafe(cert.issuer.size()) == 0) { env.error("issuer should not be empty") }
    if(unsafe(cert.valid_from[0]) == 0) { env.error("valid_from should be populated") }
    if(unsafe(cert.valid_to[0]) == 0) { env.error("valid_to should be populated") }
    if(unsafe(cert.serial) == null) { env.error("serial should be non-null") }
    if(unsafe(cert.raw_pem) == null) { env.error("raw_pem should be non-null") }
    if(unsafe(cert.raw_pem_len) == 0) { env.error("raw_pem_len should be non-zero") }
}

@test
public func X509_cert_get_cn_works(env : &mut TestEnv) {
    var cert : X509Cert
    x509_cert_init(unsafe(&raw mut cert))
    parse_cert_der(unsafe(&raw mut cert), &raw tls_tests::test_cert_data[0], 831)

    var cn = string()
    cert_get_cn(unsafe(&raw mut cert), &raw mut cn)

    if(cn.size() == 0) {
        env.error("CN should not be empty")
    }
}

@test
public func X509_empty_cert_hostname_fails(env : &mut TestEnv) {
    var cert : X509Cert
    x509_cert_init(unsafe(&raw mut cert))

    var ret = x509_verify_hostname(unsafe(&raw mut cert), "example.com" as *char)
    if(ret == 0) {
        env.error("hostname verify on empty cert should fail")
    }
}

// ============================================================================
// SECTION 13: TLS 1.2 Record Edge Cases
// ============================================================================

@test
public func TLS12_RECORD_empty_plaintext_works(env : &mut TestEnv) {
    var tr : Transform
    transform_init(unsafe(&raw mut tr))
    tr.cipher_type = CIPHER_AES_128_GCM as u8
    tr.key_len = 16
    tr.iv_len = 4
    tr.fixed_iv_len = 4
    tr.mac_key_len = 0

    var key : [16]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10]
    var iv : [4]u8 = [0x00, 0x01, 0x02, 0x03]
    var i : size_t = 0
    while(i < 16) { tr.key_enc[i] = key[i]; tr.key_dec[i] = key[i]; i += 1 }
    i = 0
    while(i < 4) { tr.base_iv_enc[i] = iv[i]; tr.base_iv_dec[i] = iv[i]; i += 1 }

    var seq_num : [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0]
    var encrypted : [128]u8
    var enc_len = tls12_encrypt_record(unsafe(&raw mut tr), &raw seq_num[0],
                                        SSL_MSG_APPLICATION_DATA as u8, 3, 3,
                                        null, 0, &raw mut encrypted[0], 128)
    if(enc_len < 0) { env.error("tls12 encrypt empty should succeed"); return }
}

@test
public func TLS12_finished_client_and_server_differ(env : &mut TestEnv) {
    var ms : [48]u8
    var i : size_t = 0
    while(i < 48) { ms[i] = i as u8; i += 1 }
    var hh : [32]u8
    i = 0
    while(i < 32) { hh[i] = (i + 0xAA) as u8; i += 1 }

    var client_fin : [12]u8
    var server_fin : [12]u8
    tls12_compute_finished(&raw ms[0], true, &raw hh[0], 32, &raw mut client_fin[0])
    tls12_compute_finished(&raw ms[0], false, &raw hh[0], 32, &raw mut server_fin[0])

    if(tc_bytes_eq(&raw client_fin[0], &raw server_fin[0], 12)) {
        env.error("client and server Finished should differ")
    }
}

// ============================================================================
// SECTION 14: TLS 1.3 Key Schedule Tests
// ============================================================================

@test
public func TLS13_key_schedule_init_zeroes(env : &mut TestEnv) {
    var ks : TLS13KeySchedule
    tls13_key_schedule_init(unsafe(&raw mut ks))

    var i : size_t = 0
    while(i < 48) {
        if(ks.early_secret[i] != 0) { env.error("early_secret should be zeroed"); return }
        if(ks.handshake_secret[i] != 0) { env.error("handshake_secret should be zeroed"); return }
        if(ks.master_secret[i] != 0) { env.error("master_secret should be zeroed"); return }
        i += 1
    }
}

@test
public func TLS13_derive_keys_deterministic(env : &mut TestEnv) {
    var ss : [32]u8
    var th : [32]u8
    var i : size_t = 0
    while(i < 32) { ss[i] = i as u8; th[i] = (i + 0x55) as u8; i += 1 }

    var ctx1 : SSLContext; ssl_init(unsafe(&raw mut ctx1))
    var cfg1 = ssl_config_init(SSL_IS_CLIENT)
    ssl_set_config(unsafe(&raw mut ctx1), &raw mut cfg1)
    tls13_derive_handshake_keys(unsafe(&raw mut ctx1), &raw ss[0], 32, &raw th[0])

    var ctx2 : SSLContext; ssl_init(unsafe(&raw mut ctx2))
    var cfg2 = ssl_config_init(SSL_IS_CLIENT)
    ssl_set_config(unsafe(&raw mut ctx2), &raw mut cfg2)
    tls13_derive_handshake_keys(unsafe(&raw mut ctx2), &raw ss[0], 32, &raw th[0])

    var tr1 = unsafe(ctx1.transform_out)
    var tr2 = unsafe(ctx2.transform_out)
    if(!tc_bytes_eq(&raw tr1.key_enc[0], &raw tr2.key_enc[0], 16)) {
        env.error("tls13_derive_handshake_keys should be deterministic")
    }
}

// ============================================================================
// SECTION 15: Cipher Suite Info Tests
// ============================================================================

@test
public func CIPHERSUITE_tls13_all_aead_works(env : &mut TestEnv) {
    var tls13_suites : [3]u16 = [
        TLS1_3_AES_128_GCM_SHA256 as u16,
        TLS1_3_AES_256_GCM_SHA384 as u16,
        TLS1_3_CHACHA20_POLY1305_SHA256 as u16
    ]
    var i : size_t = 0
    while(i < 3) {
        var info = get_ciphersuite_info(tls13_suites[i])
        if(info.id != tls13_suites[i]) {
            env.error("ciphersuite lookup should find TLS 1.3 suite")
            return
        }
        if(!ciphersuite_is_aead(info.id)) {
            env.error("all TLS 1.3 suites should be AEAD")
            return
        }
        i += 1
    }
}

@test
public func CIPHERSUITE_tls12_gcm_is_aead(env : &mut TestEnv) {
    var suites : [3]u16 = [
        TLS_RSA_WITH_AES_128_GCM_SHA256 as u16,
        TLS_RSA_WITH_AES_256_GCM_SHA384 as u16,
        TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 as u16
    ]
    var i : size_t = 0
    while(i < 3) {
        if(!ciphersuite_is_aead(suites[i])) {
            env.error("RSA-GCM ciphersuites should be AEAD")
            return
        }
        i += 1
    }
}

@test
public func CIPHERSUITE_tls12_cbc_not_aead(env : &mut TestEnv) {
    var suites : [2]u16 = [
        TLS_RSA_WITH_AES_128_CBC_SHA256 as u16,
        TLS_RSA_WITH_AES_256_CBC_SHA256 as u16
    ]
    var i : size_t = 0
    while(i < 2) {
        if(ciphersuite_is_aead(suites[i])) {
            env.error("CBC ciphersuites should NOT be AEAD")
            return
        }
        i += 1
    }
}

// ============================================================================
// SECTION 16: ECDH Additional Tests
// ============================================================================

@test
public func ECDH_generate_and_compute_shared_works(env : &mut TestEnv) {
    var ctx1 : ECDHContext
    ecdh_init(unsafe(&raw mut ctx1))
    var priv1 : [32]u8
    var pub1 : [65]u8
    var ret = ecdh_generate_keypair(unsafe(&raw mut ctx1), &raw mut priv1[0], 32, &raw mut pub1[0], 65)
    if(ret < 0) { env.error("ecdh_generate_keypair 1 failed"); return }

    var ctx2 : ECDHContext
    ecdh_init(unsafe(&raw mut ctx2))
    var priv2 : [32]u8
    var pub2 : [65]u8
    ret = ecdh_generate_keypair(unsafe(&raw mut ctx2), &raw mut priv2[0], 32, &raw mut pub2[0], 65)
    if(ret < 0) { env.error("ecdh_generate_keypair 2 failed"); return }

    var shared1 : [32]u8
    var shared2 : [32]u8
    ret = ecdh_compute_shared(unsafe(&raw mut ctx1), &raw pub2[0], 65, &raw mut shared1[0], 32)
    if(ret < 0) { env.error("ecdh_compute_shared 1 failed"); return }

    ret = ecdh_compute_shared(unsafe(&raw mut ctx2), &raw pub1[0], 65, &raw mut shared2[0], 32)
    if(ret < 0) { env.error("ecdh_compute_shared 2 failed"); return }

    if(!tc_bytes_eq(&raw shared1[0], &raw shared2[0], 32)) {
        env.error("ECDH shared secrets should match")
    }
}

@test
public func ECDH_different_keys_different_shared(env : &mut TestEnv) {
    var ctx1 : ECDHContext
    ecdh_init(unsafe(&raw mut ctx1))
    var priv1 : [32]u8
    var pub1 : [65]u8
    ecdh_generate_keypair(unsafe(&raw mut ctx1), &raw mut priv1[0], 32, &raw mut pub1[0], 65)

    var ctx2 : ECDHContext
    ecdh_init(unsafe(&raw mut ctx2))
    var priv2 : [32]u8
    var pub2 : [65]u8
    ecdh_generate_keypair(unsafe(&raw mut ctx2), &raw mut priv2[0], 32, &raw mut pub2[0], 65)

    var ctx3 : ECDHContext
    ecdh_init(unsafe(&raw mut ctx3))
    var priv3 : [32]u8
    var pub3 : [65]u8
    ecdh_generate_keypair(unsafe(&raw mut ctx3), &raw mut priv3[0], 32, &raw mut pub3[0], 65)

    var shared12 : [32]u8
    var shared13 : [32]u8
    ecdh_compute_shared(unsafe(&raw mut ctx1), &raw pub2[0], 65, &raw mut shared12[0], 32)
    ecdh_compute_shared(unsafe(&raw mut ctx1), &raw pub3[0], 65, &raw mut shared13[0], 32)

    if(tc_bytes_eq(&raw shared12[0], &raw shared13[0], 32)) {
        env.error("different keypairs should produce different shared secrets")
    }
}

// ============================================================================
// SECTION 17: TLS 1.2 Key Block Size Tests
// ============================================================================

@test
public func KEY_BLOCK_size_works(env : &mut TestEnv) {
    var info = get_ciphersuite_info(TLS_RSA_WITH_AES_128_GCM_SHA256 as u16)
    var sz = tls12_key_block_size(&raw info)
    if(sz != 40) { env.error("AES-128-GCM key block should be 40") }

    info = get_ciphersuite_info(TLS_RSA_WITH_AES_256_GCM_SHA384 as u16)
    sz = tls12_key_block_size(&raw info)
    if(sz != 72) { env.error("AES-256-GCM key block should be 72") }
}

// ============================================================================
// SECTION 18: X25519 Additional Tests
// ============================================================================

@test
public func X25519_keygen_produces_nonzero(env : &mut TestEnv) {
    var priv_k : [32]u8
    var pub_k : [32]u8
    var ret = x25519_generate_keypair(&raw mut priv_k[0], &raw mut pub_k[0])
    if(ret < 0) { env.error("x25519_generate_keypair failed"); return }

    if(!tc_bytes_not_zero(&raw pub_k[0], 32)) {
        env.error("x25519 public key should be non-zero")
    }
}

@test
public func X25519_shared_commutative(env : &mut TestEnv) {
    var priv_k : [32]u8
    var pub_k : [32]u8
    x25519_generate_keypair(&raw mut priv_k[0], &raw mut pub_k[0])

    var priv2 : [32]u8
    var pub2 : [32]u8
    x25519_generate_keypair(&raw mut priv2[0], &raw mut pub2[0])

    var shared1 : [32]u8
    var shared2 : [32]u8
    x25519_compute_shared(&raw priv_k[0], &raw pub2[0], &raw mut shared1[0])
    x25519_compute_shared(&raw priv2[0], &raw pub_k[0], &raw mut shared2[0])

    if(!tc_bytes_eq(&raw shared1[0], &raw shared2[0], 32)) {
        env.error("X25519 should be commutative: a*b == b*a")
    }
}

// ============================================================================
// SECTION 19: HKDF Tests
// ============================================================================

@test
public func HKDF_derived_secret_deterministic(env : &mut TestEnv) {
    var secret : [32]u8
    var i : size_t = 0
    while(i < 32) { secret[i] = i as u8; i += 1 }

    var out1 : [32]u8
    var out2 : [32]u8
    tls13_derive_secret(&raw secret[0], 32, "test\0" as *char, 4,
                         null, 0, &raw mut out1[0], 32)
    tls13_derive_secret(&raw secret[0], 32, "test\0" as *char, 4,
                         null, 0, &raw mut out2[0], 32)

    if(!tc_bytes_eq(&raw out1[0], &raw out2[0], 32)) {
        env.error("tls13_derive_secret should be deterministic")
    }
}

@test
public func HKDF_derived_secret_different_labels_differ(env : &mut TestEnv) {
    var secret : [32]u8
    var i : size_t = 0
    while(i < 32) { secret[i] = i as u8; i += 1 }

    var out1 : [32]u8
    var out2 : [32]u8
    tls13_derive_secret(&raw secret[0], 32, "label_a\0" as *char, 7,
                         null, 0, &raw mut out1[0], 32)
    tls13_derive_secret(&raw secret[0], 32, "label_b\0" as *char, 7,
                         null, 0, &raw mut out2[0], 32)

    if(tc_bytes_eq(&raw out1[0], &raw out2[0], 32)) {
        env.error("different labels should produce different secrets")
    }
}

@test
public func HKDF_derived_secret_different_secrets_differ(env : &mut TestEnv) {
    var sec1 : [32]u8
    var sec2 : [32]u8
    var i : size_t = 0
    while(i < 32) { sec1[i] = i as u8; sec2[i] = (i + 100) as u8; i += 1 }

    var out1 : [32]u8
    var out2 : [32]u8
    tls13_derive_secret(&raw sec1[0], 32, "test\0" as *char, 4,
                         null, 0, &raw mut out1[0], 32)
    tls13_derive_secret(&raw sec2[0], 32, "test\0" as *char, 4,
                         null, 0, &raw mut out2[0], 32)

    if(tc_bytes_eq(&raw out1[0], &raw out2[0], 32)) {
        env.error("different secrets should produce different outputs")
    }
}

// ============================================================================
// SECTION 20: Session Struct Tests
// ============================================================================

@test
public func SESSION_init_works(env : &mut TestEnv) {
    var sess = Session()
    if(sess.id_len != 0) { env.error("id_len should be 0") }
    if(sess.ciphersuite != 0) { env.error("ciphersuite should be 0") }
    if(sess.tls_version != 0) { env.error("tls_version should be 0") }
    if(sess.peer_cert != null) { env.error("peer_cert should be null") }
    if(sess.verify_result != 0) { env.error("verify_result should be 0") }
    if(sess.resumption_key_len != 0) { env.error("resumption_key_len should be 0") }
    if(sess.ticket != null) { env.error("ticket should be null") }
    if(sess.ticket_len != 0) { env.error("ticket_len should be 0") }
    if(sess.ticket_lifetime != 0) { env.error("ticket_lifetime should be 0") }
}

// ============================================================================
// SECTION 21: TLS State Machine Tests
// ============================================================================

@test
public func STATE_all_variants_exist(env : &mut TestEnv) {
    var s : SSLState
    s = SSLState.HELLO_REQUEST()
    if(!(s is SSLState.HELLO_REQUEST)) { env.error("HELLO_REQUEST"); return }
    s = SSLState.CLIENT_HELLO()
    if(!(s is SSLState.CLIENT_HELLO)) { env.error("CLIENT_HELLO"); return }
    s = SSLState.SERVER_HELLO()
    if(!(s is SSLState.SERVER_HELLO)) { env.error("SERVER_HELLO"); return }
    s = SSLState.HANDSHAKE_OVER()
    if(!(s is SSLState.HANDSHAKE_OVER)) { env.error("HANDSHAKE_OVER"); return }
    s = SSLState.ENCRYPTED_EXTENSIONS()
    if(!(s is SSLState.ENCRYPTED_EXTENSIONS)) { env.error("ENCRYPTED_EXTENSIONS"); return }
}

// ============================================================================
// SECTION 22: HMAC-SHA256 Additional Tests
// ============================================================================

@test
public func HMAC_SHA256_empty_inputs_works(env : &mut TestEnv) {
    var key : [1]u8 = [0x00]
    var msg : [1]u8 = [0x00]
    var tag : [32]u8
    hmac_sha256(&raw key[0], 0, &raw msg[0], 0, &raw mut tag[0])
    if(!tc_bytes_not_zero(&raw tag[0], 32)) {
        env.error("HMAC with empty inputs should produce non-zero output")
    }
}

@test
public func HMAC_SHA256_deterministic(env : &mut TestEnv) {
    var key : [16]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10]
    var msg : [5]u8 = [0x48, 0x65, 0x6C, 0x6C, 0x6F]
    var tag1 : [32]u8
    var tag2 : [32]u8
    hmac_sha256(&raw key[0], 16, &raw msg[0], 5, &raw mut tag1[0])
    hmac_sha256(&raw key[0], 16, &raw msg[0], 5, &raw mut tag2[0])
    if(!tc_bytes_eq(&raw tag1[0], &raw tag2[0], 32)) {
        env.error("HMAC-SHA256 should be deterministic")
    }
}

// ============================================================================
// SECTION 23: PRF Additional Tests
// ============================================================================

@test
public func PRF_tls12_deterministic(env : &mut TestEnv) {
    var secret : [16]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                            0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99]
    var label = "master secret\0" as *char
    var seed : [64]u8
    var i : size_t = 0
    while(i < 32) { seed[i] = (i as u8); seed[i + 32] = (i + 32) as u8; i += 1 }

    var out1 : [48]u8
    var out2 : [48]u8
    tls12_prf(&raw secret[0], 16, label, 13, &raw seed[0], 64, &raw mut out1[0], 48)
    tls12_prf(&raw secret[0], 16, label, 13, &raw seed[0], 64, &raw mut out2[0], 48)

    if(!tc_bytes_eq(&raw out1[0], &raw out2[0], 48)) {
        env.error("TLS 1.2 PRF should be deterministic")
    }
}

@test
public func PRF_tls12_different_secrets_differ(env : &mut TestEnv) {
    var sec1 : [16]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10]
    var sec2 : [16]u8 = [0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09,
                          0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01]
    var label = "key expansion\0" as *char
    var seed : [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF]

    var out1 : [32]u8
    var out2 : [32]u8
    tls12_prf(&raw sec1[0], 16, label, 14, &raw seed[0], 4, &raw mut out1[0], 32)
    tls12_prf(&raw sec2[0], 16, label, 14, &raw seed[0], 4, &raw mut out2[0], 32)

    if(tc_bytes_eq(&raw out1[0], &raw out2[0], 32)) {
        env.error("different secrets should produce different PRF output")
    }
}
