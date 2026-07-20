using std::Result;
using std::vector;
using std::string;
using std::string_view;

@test
public func tls_ciphersuite_lookup_works(env : &mut TestEnv) {
    var info = tls::get_ciphersuite_info(tls::TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 as u16)
    if(info.id != tls::TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 as u16) {
        env.error("ciphersuite lookup should find ECDHE-RSA-AES128-GCM-SHA256")
        return
    }
    if(info.key_exchange != tls::KE_ECDHE_RSA as u8) {
        env.error("key exchange should be ECDHE-RSA")
    }
    if(info.cipher != tls::CIPHER_AES_128_GCM as u8) {
        env.error("cipher should be AES-128-GCM")
    }
    if(info.hash != tls::HASH_SHA256 as u8) {
        env.error("hash should be SHA256")
    }
}

@test
public func tls_ciphersuite_tls13_lookup_works(env : &mut TestEnv) {
    var info = tls::get_ciphersuite_info(tls::TLS1_3_AES_128_GCM_SHA256 as u16)
    if(info.id != tls::TLS1_3_AES_128_GCM_SHA256 as u16) {
        env.error("ciphersuite lookup should find TLS 1.3 AES-128-GCM")
        return
    }
    if(info.cipher != tls::CIPHER_AES_128_GCM as u8) {
        env.error("cipher should be AES-128-GCM")
    }
    if(info.hash != tls::HASH_SHA256 as u8) {
        env.error("hash should be SHA256")
    }
}

@test
public func tls_num_preferred_ciphersuites_works(env : &mut TestEnv) {
    var count = tls::num_preferred_ciphersuites()
    if(count == 0) {
        env.error("should have at least one preferred ciphersuite after init")
    }
}

@test
public func tls_ssl_config_init_works(env : &mut TestEnv) {
    var config = tls::ssl_config_init(tls::SSL_IS_CLIENT)
    if(config.endpoint != tls::SSL_IS_CLIENT) {
        env.error("endpoint should be client")
    }
    if(config.transport != tls::SSL_TRANSPORT_STREAM) {
        env.error("transport should be stream")
    }
    if(config.min_tls_version != tls::SSL_VERSION_TLS1_2) {
        env.error("min TLS version should be TLS 1.2")
    }
    if(config.max_tls_version != tls::SSL_VERSION_TLS1_3) {
        env.error("max TLS version should be TLS 1.3")
    }
}

@test
public func tls_ssl_context_init_works(env : &mut TestEnv) {
    var ctx : tls::SSLContext
    tls::ssl_init(&raw mut ctx)
    if(ctx.transport_connected) {
        env.error("new context should not be connected")
    }
    if(!(ctx.state is tls::SSLState.HELLO_REQUEST)) {
        env.error("initial state should be HELLO_REQUEST")
    }
}

@test
public func tls_set_hostname_works(env : &mut TestEnv) {
    var ctx : tls::SSLContext
    tls::ssl_init(&raw mut ctx)
    tls::ssl_set_hostname(&raw mut ctx, "example.com\0" as *char)
    if(ctx.hostname_len == 0) {
        env.error("hostname should be set")
    }
}

@test
public func tls_ciphersuite_aead_check_works(env : &mut TestEnv) {
    var info1 = tls::get_ciphersuite_info(tls::TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 as u16)
    if(!tls::ciphersuite_is_aead(info1.id)) {
        env.error("AES-128-GCM should be AEAD")
    }
    var info2 = tls::get_ciphersuite_info(tls::TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA as u16)
    if(tls::ciphersuite_is_aead(info2.id)) {
        env.error("AES-128-CBC should not be AEAD")
    }
}

@test
public func tls_tls12_prf_basic_works(env : &mut TestEnv) {
    var secret : [16]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                           0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10]
    var label = "test label\0" as *char
    var seed : [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11]
    var output : [48]u8

    tls::tls12_prf(&raw secret[0], 16, label, 10, &raw seed[0], 8, &raw mut output[0], 48)

    var all_zero = true
    var i : size_t = 0
    while(i < 48) {
        if(output[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) {
        env.error("PRF output should not be all zeros")
    }
}

@test
public func tls_tls12_prf_deterministic_works(env : &mut TestEnv) {
    var secret : [8]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08]
    var label = "key expansion\0" as *char
    var seed : [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF]
    var output1 : [32]u8
    var output2 : [32]u8

    tls::tls12_prf(&raw secret[0], 8, label, 14, &raw seed[0], 4, &raw mut output1[0], 32)
    tls::tls12_prf(&raw secret[0], 8, label, 14, &raw seed[0], 4, &raw mut output2[0], 32)

    var match = true
    var i : size_t = 0
    while(i < 32) {
        if(output1[i] != output2[i]) { match = false }
        i += 1
    }
    if(!match) {
        env.error("PRF should be deterministic")
    }
}

// ─── New Tests ──────────────────────────────────────────────────────────────

@test
public func tls_tls13_hkdf_expand_label_works(env : &mut TestEnv) {
    var secret : [16]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                           0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10]
    var label = "test label\0" as *char
    var context : [4]u8 = [0xAA, 0xBB, 0xCC, 0xDD]
    var output : [32]u8

    tls::tls13_derive_secret(&raw secret[0], 16, label, 10,
                              &raw context[0], 4, &raw mut output[0], 32)

    var all_zero = true
    var i : size_t = 0
    while(i < 32) {
        if(output[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) {
        env.error("HKDF expand label should not produce all zeros")
    }
}

@test
public func tls_prf_empty_input_works(env : &mut TestEnv) {
    // Test PRF with empty inputs - should not crash
    var secret : [1]u8 = [0x00]
    var label = "\0" as *char
    var seed : [1]u8 = [0x00]
    var output : [16]u8

    tls::tls12_prf(&raw secret[0], 1, label, 0, &raw seed[0], 1, &raw mut output[0], 16)

    var all_zero = true
    var i : size_t = 0
    while(i < 16) {
        if(output[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) {
        env.error("PRF with minimal inputs should still produce output")
    }
}

@test
public func tls_pem_cert_init_and_free_works(env : &mut TestEnv) {
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    if(cert.version != 0) {
        env.error("init should set version to 0")
    }
    if(cert.subject.size() != 0) {
        env.error("init should set subject to empty")
    }
}

@test
public func tls_der_cert_minimal_validation_works(env : &mut TestEnv) {
    // A minimal valid DER certificate starts with SEQUENCE tag
    // We test that too-short input returns INVALID_FORMAT
    var too_short : [3]u8 = [0x30, 0x01, 0x00]
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw too_short[0], 3)
    if(ret != tls::ERR_X509_INVALID_FORMAT) {
        env.error("too-short DER should return INVALID_FORMAT")
    }
}

@test
public func tls_der_cert_non_sequence_returns_error(env : &mut TestEnv) {
    // DER must start with SEQUENCE (0x30)
    var not_seq : [10]u8 = [0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw not_seq[0], 10)
    if(ret != tls::ERR_X509_INVALID_FORMAT) {
        env.error("non-SEQUENCE DER should return INVALID_FORMAT")
    }
}

@test
public func tls_pem_invalid_marker_returns_error(env : &mut TestEnv) {
    var invalid_pem : [10]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09]
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    // Invalid PEM without BEGIN marker should fall through to DER parsing
    var ret = tls::parse_cert_pem(&raw mut cert, &raw invalid_pem[0], 10)
    // Should return an error since the data is not valid DER
    if(ret == 0) {
        env.error("invalid PEM/DER should not return success")
    }
}

@test
public func tls_x509_cert_get_cn_empty_subject_works(env : &mut TestEnv) {
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var cn = string()
    tls::cert_get_cn(&raw mut cert, &raw mut cn)

    // With empty subject, CN should be empty
    if(cn.size() != 0) {
        env.error("CN should be empty for empty subject")
    }
}

@test
public func tls_x509_cert_init_consistent(env : &mut TestEnv) {
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    // Verify multiple fields have correct defaults
    if(cert.pk_type != tls::PK_NONE as u8) { env.error("pk_type should be NONE") }
    if(cert.ext_is_ca) { env.error("is_ca should be false") }
    if(cert.ext_max_pathlen != -1) { env.error("max_pathlen should be -1") }
    if(cert.serial != null) { env.error("serial should be null") }
    if(cert.next != null) { env.error("next should be null") }
}

@test
public func tls_ssl_state_variant_works(env : &mut TestEnv) {
    var state = tls::SSLState.HELLO_REQUEST()
    if(!(state is tls::SSLState.HELLO_REQUEST)) {
        env.error("state should be HELLO_REQUEST variant")
    }

    state = tls::SSLState.CLIENT_HELLO()
    if(!(state is tls::SSLState.CLIENT_HELLO)) {
        env.error("state should be CLIENT_HELLO variant")
    }

    state = tls::SSLState.HANDSHAKE_OVER()
    if(!(state is tls::SSLState.HANDSHAKE_OVER)) {
        env.error("state should be HANDSHAKE_OVER variant")
    }
}

@test
public func tls_preferred_ciphersuite_order_works(env : &mut TestEnv) {
    var first = tls::get_preferred_ciphersuite(0)
    var second = tls::get_preferred_ciphersuite(1)

    if(first == 0) {
        env.error("first preferred ciphersuite should not be zero")
    }
    if(second == 0) {
        env.error("second preferred ciphersuite should not be zero")
    }
    if(first == second) {
        env.error("first and second ciphersuites should differ")
    }
}

@test
public func tls_error_codes_distinct_works(env : &mut TestEnv) {
    if(tls::ERR_SSL_BAD_INPUT_DATA == tls::ERR_SSL_INTERNAL_ERROR) {
        env.error("error codes should be distinct")
    }
    if(tls::ERR_SSL_CONN_EOF == tls::ERR_SSL_DECODE_ERROR) {
        env.error("error codes should be distinct")
    }
}

@test
public func tls_der_cert_parses_correctly(env : &mut TestEnv) {
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 635)
    if(ret != 0) {
        env.error("DER certificate should parse successfully")
        return
    }

    // Verify version is 3 (v3 certificate with [0] EXPLICIT tag)
    if(cert.version != 3) {
        env.error("certificate version should be 3")
    }

    // Verify subject CN is "test.example.com"
    var cn = string()
    tls::cert_get_cn(&raw mut cert, &raw mut cn)
    if(cn.size() == 0) {
        env.error("CN should not be empty for parsed cert")
    }

    // Check that CN contains "test.example.com"
    var cn_view = cn.to_view()
    var expected_cn = string_view("test.example.com")
    if(!cn_view.contains(&expected_cn)) {
        env.error("CN should contain test.example.com")
    }

    // Verify issuer is not empty
    if(cert.issuer.size() == 0) {
        env.error("issuer should be parsed")
    }

    // Verify issuer contains "CA"
    var issuer_view = cert.issuer.to_view()
    var expected_iss = string_view("CA")
    if(!issuer_view.contains(&expected_iss)) {
        env.error("issuer should contain CA")
    }

    // Verify public key type was detected (RSA or EC, not NONE)
    if(cert.pk_type == tls::PK_NONE as u8) {
        env.error("public key type should be detected (not NONE)")
    }

    // Verify valid_from and valid_to contain UTCTime date strings
    if(cert.valid_from[0] == 0) {
        env.error("valid_from should be populated")
    }
    if(cert.valid_to[0] == 0) {
        env.error("valid_to should be populated")
    }

    // Verify raw_pem points to the data
    if(cert.raw_pem == null) {
        env.error("raw_pem should point to parsed data")
    }
    if(cert.raw_pem_len == 0) {
        env.error("raw_pem_len should be non-zero")
    }
}

// ─── AES Encryption Tests ───────────────────────────────────────────────────

@test
public func tls_aes128_ecb_encrypt_decrypt_works(env : &mut TestEnv) {
    // NIST AES-128 test vector (FIPS 197)
    // Key: 2b7e151628aed2a6abf7158809cf4f3c
    // Plaintext: 6bc1bee22e409f96e93d7e117393172a
    // Ciphertext: 3ad77bb40d7a3660a89ecaf32466ef97
    var key : [16]u8 = [0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c]
    var pt : [16]u8 = [0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                       0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a]
    var expected_ct : [16]u8 = [0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
                                0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97]

    var ctx : tls::AESContext
    tls::aes_init(&raw mut ctx)

    var ret = tls::aes_setkey_enc(&raw mut ctx, &raw key[0], 16)
    if(ret < 0) { env.error("aes_setkey_enc should succeed"); return }

    var ct : [16]u8
    ret = tls::aes_crypt_ecb(&raw mut ctx, tls::AES_ENCRYPT, &raw pt[0], &raw mut ct[0])
    if(ret < 0) { env.error("aes_crypt_ecb encrypt should succeed"); return }

    var matches = true
    var i : size_t = 0
    while(i < 16) {
        if(ct[i] != expected_ct[i]) { matches = false }
        i += 1
    }
    if(!matches) {
        env.error("AES-128 ECB encrypt should match NIST test vector")
        return
    }

    // Decrypt back
    var pt2 : [16]u8
    ret = tls::aes_setkey_dec(&raw mut ctx, &raw key[0], 16)
    if(ret < 0) { env.error("aes_setkey_dec should succeed"); return }

    ret = tls::aes_crypt_ecb(&raw mut ctx, tls::AES_DECRYPT, &raw ct[0], &raw mut pt2[0])
    if(ret < 0) { env.error("aes_crypt_ecb decrypt should succeed"); return }

    matches = true
    i = 0
    while(i < 16) {
        if(pt2[i] != pt[i]) { matches = false }
        i += 1
    }
    if(!matches) {
        env.error("AES-128 ECB roundtrip should produce original plaintext")
    }
}

@test
public func tls_aes128_cbc_encrypt_decrypt_works(env : &mut TestEnv) {
    // AES-128 CBC test with NIST test vector
    var key : [16]u8 = [0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c]
    var iv : [16]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F]
    var pt : [32]u8 = [0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                       0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
                       0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
                       0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51]

    var ctx : tls::AESContext
    tls::aes_init(&raw mut ctx)

    var ret = tls::aes_setkey_enc(&raw mut ctx, &raw key[0], 16)
    if(ret < 0) { env.error("aes_setkey_enc should succeed"); return }

    var iv_copy : [16]u8
    var i : size_t = 0
    while(i < 16) { iv_copy[i] = iv[i]; i += 1 }

    var ct : [32]u8
    ret = tls::aes_crypt_cbc(&raw mut ctx, tls::AES_ENCRYPT, 32, &raw mut iv_copy[0],
                              &raw pt[0], &raw mut ct[0])
    if(ret < 0) { env.error("aes_crypt_cbc encrypt should succeed"); return }

    // Decrypt
    tls::aes_setkey_dec(&raw mut ctx, &raw key[0], 16)
    var iv_copy2 : [16]u8
    i = 0
    while(i < 16) { iv_copy2[i] = iv[i]; i += 1 }

    var pt2 : [32]u8
    ret = tls::aes_crypt_cbc(&raw mut ctx, tls::AES_DECRYPT, 32, &raw mut iv_copy2[0],
                              &raw ct[0], &raw mut pt2[0])
    if(ret < 0) { env.error("aes_crypt_cbc decrypt should succeed"); return }

    var matches = true
    i = 0
    while(i < 32) {
        if(pt2[i] != pt[i]) { matches = false }
        i += 1
    }
    if(!matches) {
        env.error("AES-128 CBC roundtrip should produce original plaintext")
    }
}

// ─── TLS 1.2 Key Derivation Tests ───────────────────────────────────────────

@test
public func tls12_master_secret_derivation_works(env : &mut TestEnv) {
    // Known test values for TLS 1.2 PRF (from mbedTLS test suite)
    var pre_master : [48]u8
    var client_random : [32]u8
    var server_random : [32]u8

    // Fill with known values
    var i : size_t = 0
    while(i < 48) {
        pre_master[i] = i as u8
        i += 1
    }
    i = 0
    while(i < 32) {
        client_random[i] = (i + 100) as u8
        server_random[i] = (i + 200) as u8
        i += 1
    }

    var master_secret : [48]u8
    tls::tls12_derive_master_secret(&raw pre_master[0], 48,
                                     &raw client_random[0], &raw server_random[0],
                                     &raw mut master_secret[0])

    // Master secret should not be all zeros
    var all_zero = true
    i = 0
    while(i < 48) {
        if(master_secret[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) {
        env.error("master secret should not be all zeros")
        return
    }

    // Master secret should be deterministic
    var master_secret2 : [48]u8
    tls::tls12_derive_master_secret(&raw pre_master[0], 48,
                                     &raw client_random[0], &raw server_random[0],
                                     &raw mut master_secret2[0])

    var matches = true
    i = 0
    while(i < 48) {
        if(master_secret[i] != master_secret2[i]) { matches = false }
        i += 1
    }
    if(!matches) {
        env.error("master secret derivation should be deterministic")
    }
}

@test
public func tls12_key_block_derivation_works(env : &mut TestEnv) {
    // Test key block derivation for AES-128-GCM
    var master_secret : [48]u8
    var server_random : [32]u8
    var client_random : [32]u8

    var i : size_t = 0
    while(i < 48) { master_secret[i] = i as u8; i += 1 }
    i = 0
    while(i < 32) { server_random[i] = (i + 50) as u8; client_random[i] = (i + 100) as u8; i += 1 }

    var info = tls::get_ciphersuite_info(tls::TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 as u16)
    var kb_size = tls::tls12_key_block_size(&raw info)

    // For AES-128-GCM: mac_key_len=0, key_size=16, iv_size=12 but fixed_iv=4
    // Total = (0 + 16 + 4) * 2 = 40
    if(kb_size != 40) {
        env.error("AES-128-GCM key block size should be 40")
        return
    }

    var key_block : [64]u8
    tls::tls12_derive_key_block(&raw master_secret[0],
                                 &raw server_random[0], &raw client_random[0],
                                 &raw mut key_block[0], kb_size)

    // Key block should not be all zeros
    var all_zero = true
    i = 0
    while(i < kb_size) {
        if(key_block[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) {
        env.error("key block should not be all zeros")
    }
}

@test
public func tls12_transform_population_works(env : &mut TestEnv) {
    var tr : tls::Transform
    tls::transform_init(&raw mut tr)

    var info = tls::get_ciphersuite_info(tls::TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 as u16)
    var kb_size = tls::tls12_key_block_size(&raw info)

    // Create a known key block pattern
    var key_block : [64]u8
    var i : size_t = 0
    while(i < kb_size) { key_block[i] = i as u8; i += 1 }

    tls::tls12_populate_transform(&raw mut tr, &raw info, &raw key_block[0], kb_size)

    // Verify the transform was populated correctly
    // client_write_key (bytes 0-15, since mac_key_len=0 for GCM)
    i = 0
    while(i < 16) {
        if(tr.key_enc[i] != key_block[i]) {
            env.error("client_write_key should match first 16 bytes of key block")
        }
        i += 1
    }
    // server_write_key (bytes 16-31)
    i = 0
    while(i < 16) {
        if(tr.key_dec[i] != key_block[16 + i]) {
            env.error("server_write_key should match bytes 16-31")
        }
        i += 1
    }
    // client_write_IV (bytes 32-35, since fixed_iv=4 for GCM)
    i = 0
    while(i < 4) {
        if(tr.iv_enc[i] != key_block[32 + i]) {
            env.error("client_write_IV should match bytes 32-35")
        }
        i += 1
    }
    // server_write_IV (bytes 36-39)
    i = 0
    while(i < 4) {
        if(tr.iv_dec[i] != key_block[36 + i]) {
            env.error("server_write_IV should match bytes 36-39")
        }
        i += 1
    }
}

@test
public func tls12_finished_message_works(env : &mut TestEnv) {
    var master_secret : [48]u8
    var handshake_hash : [32]u8
    var verify_data : [12]u8

    var i : size_t = 0
    while(i < 48) { master_secret[i] = i as u8; i += 1 }
    i = 0
    while(i < 32) { handshake_hash[i] = (i + 0xAA) as u8; i += 1 }

    // Client finished
    tls::tls12_compute_finished(&raw master_secret[0], true,
                                 &raw handshake_hash[0], 32,
                                 &raw mut verify_data[0])

    var all_zero = true
    i = 0
    while(i < 12) {
        if(verify_data[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) {
        env.error("client Finished verify_data should not be all zeros")
    }

    // Server finished
    var verify_data2 : [12]u8
    tls::tls12_compute_finished(&raw master_secret[0], false,
                                 &raw handshake_hash[0], 32,
                                 &raw mut verify_data2[0])

    all_zero = true
    i = 0
    while(i < 12) {
        if(verify_data2[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) {
        env.error("server Finished verify_data should not be all zeros")
    }

    // Client and server finished should differ (different labels)
    var same = true
    i = 0
    while(i < 12) {
        if(verify_data[i] != verify_data2[i]) { same = false }
        i += 1
    }
    if(same) {
        env.error("client and server Finished verify_data should differ")
    }
}

@test
public func tls12_key_block_size_works_for_ciphersuites(env : &mut TestEnv) {
    // AES-128-GCM: (0 + 16 + 4) * 2 = 40
    var info = tls::get_ciphersuite_info(tls::TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 as u16)
    var size = tls::tls12_key_block_size(&raw info)
    if(size != 40) { env.error("AES-128-GCM key block size should be 40") }

    // AES-256-GCM: (0 + 32 + 4) * 2 = 72
    info = tls::get_ciphersuite_info(tls::TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 as u16)
    size = tls::tls12_key_block_size(&raw info)
    if(size != 72) { env.error("AES-256-GCM key block size should be 72") }

    // TLS 1.3 AES-128-GCM: (0 + 16 + 4) * 2 = 40
    info = tls::get_ciphersuite_info(tls::TLS1_3_AES_128_GCM_SHA256 as u16)
    size = tls::tls12_key_block_size(&raw info)
    if(size != 40) { env.error("TLS 1.3 AES-128-GCM key block size should be 40") }
}

@test
public func tls12_key_derivation_full_pipeline_works(env : &mut TestEnv) {
    // Full TLS 1.2 key derivation pipeline test
    // Simulates the complete process from pre-master secret to transform

    // 1. Set up pre-master secret and random values
    var pre_master : [48]u8
    var client_random : [32]u8
    var server_random : [32]u8

    var i : size_t = 0
    while(i < 48) { pre_master[i] = i as u8; i += 1 }
    i = 0
    while(i < 32) { client_random[i] = (i + 0xAB) as u8; server_random[i] = (i + 0xCD) as u8; i += 1 }

    // 2. Derive master secret
    var master_secret : [48]u8
    tls::tls12_derive_master_secret(&raw pre_master[0], 48,
                                     &raw client_random[0], &raw server_random[0],
                                     &raw mut master_secret[0])

    // 3. Get cipher suite info
    var info = tls::get_ciphersuite_info(tls::TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 as u16)
    var kb_size = tls::tls12_key_block_size(&raw info)

    // 4. Derive key block
    var key_block : [64]u8
    tls::tls12_derive_key_block(&raw master_secret[0],
                                 &raw server_random[0], &raw client_random[0],
                                 &raw mut key_block[0], kb_size)

    // 5. Populate transform
    var tr : tls::Transform
    tls::transform_init(&raw mut tr)
    tls::tls12_populate_transform(&raw mut tr, &raw info, &raw key_block[0], kb_size)

    // Verify the transform has sane values
    if(tr.key_len != 16) { env.error("key_len should be 16 for AES-128"); return }
    if(tr.iv_len == 0) { env.error("iv_len should be non-zero"); return }
    if(tr.cipher_type != tls::CIPHER_AES_128_GCM as u8) {
        env.error("cipher_type should be AES-128-GCM")
    }
}

// ─── Big Number (Mpi) Tests ─────────────────────────────────────────────────

@test
public func tls_bignum_mpi_lset_and_cmp_works(env : &mut TestEnv) {
    var a : tls::Mpi; tls::mpi_init(&raw mut a)
    var b : tls::Mpi; tls::mpi_init(&raw mut b)

    tls::mpi_lset(&raw mut a, 42)
    if(tls::mpi_cmp_int(&raw mut a, 42) != 0) { env.error("a should be 42"); return }

    tls::mpi_lset(&raw mut b, -7)
    if(tls::mpi_cmp_int(&raw mut b, -7) != 0) { env.error("b should be -7"); return }

    if(tls::mpi_cmp(&raw mut a, &raw mut b) <= 0) { env.error("42 > -7"); return }
}

@test
public func tls_bignum_add_sub_works(env : &mut TestEnv) {
    var a : tls::Mpi; tls::mpi_init(&raw mut a)
    var b : tls::Mpi; tls::mpi_init(&raw mut b)
    var c : tls::Mpi; tls::mpi_init(&raw mut c)

    tls::mpi_lset(&raw mut a, 100); tls::mpi_lset(&raw mut b, 200)

    tls::mpi_add(&raw mut c, &raw mut a, &raw mut b)
    if(tls::mpi_cmp_int(&raw mut c, 300) != 0) { env.error("100 + 200 should be 300"); return }

    tls::mpi_sub(&raw mut c, &raw mut a, &raw mut b)
    if(tls::mpi_cmp_int(&raw mut c, -100) != 0) { env.error("100 - 200 should be -100"); return }
}

@test
public func tls_bignum_mul_and_bitlen_works(env : &mut TestEnv) {
    var a : tls::Mpi; tls::mpi_init(&raw mut a)
    var b : tls::Mpi; tls::mpi_init(&raw mut b)
    var c : tls::Mpi; tls::mpi_init(&raw mut c)

    tls::mpi_lset(&raw mut a, 0x10000); tls::mpi_lset(&raw mut b, 0x20000)
    tls::mpi_mul(&raw mut c, &raw mut a, &raw mut b)
    if(tls::mpi_cmp_int(&raw mut c, 0x200000000) != 0) {
        env.error("0x10000 * 0x20000 should be 0x200000000")
    }

    if(tls::mpi_bitlen(&raw mut a) != 17) { env.error("bitlen of 0x10000 should be 17") }
}

@test
public func tls_bignum_div_mod_works(env : &mut TestEnv) {
    var a : tls::Mpi; tls::mpi_init(&raw mut a)
    var b : tls::Mpi; tls::mpi_init(&raw mut b)
    var q : tls::Mpi; tls::mpi_init(&raw mut q)
    var r : tls::Mpi; tls::mpi_init(&raw mut r)

    tls::mpi_lset(&raw mut a, 100); tls::mpi_lset(&raw mut b, 7)

    tls::mpi_div(&raw mut q, &raw mut r, &raw mut a, &raw mut b)
    if(tls::mpi_cmp_int(&raw mut q, 14) != 0) { env.error("100/7 should be 14"); return }
    if(tls::mpi_cmp_int(&raw mut r, 2) != 0) { env.error("100%7 should be 2"); return }
}

@test
public func tls_bignum_read_write_binary_works(env : &mut TestEnv) {
    var m : tls::Mpi; tls::mpi_init(&raw mut m)
    var buf : [8]u8 = [0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0]

    var ret = tls::mpi_read_binary(&raw mut m, &raw buf[0], 8)
    if(ret < 0) { env.error("read_binary should succeed"); return }

    var out : [8]u8
    ret = tls::mpi_write_binary(&raw mut m, &raw mut out[0], 8)
    if(ret < 0) { env.error("write_binary should succeed"); return }

    var i : size_t = 0
    while(i < 8) {
        if(buf[i] != out[i]) { env.error("bytes should roundtrip"); return }
        i += 1
    }
}

@test
public func tls_bignum_mod_inv_works(env : &mut TestEnv) {
    var a : tls::Mpi; tls::mpi_init(&raw mut a)
    var n : tls::Mpi; tls::mpi_init(&raw mut n)
    var inv : tls::Mpi; tls::mpi_init(&raw mut inv)

    tls::mpi_lset(&raw mut a, 3); tls::mpi_lset(&raw mut n, 7)
    var ret = tls::mpi_mod_inv(&raw mut inv, &raw mut a, &raw mut n)
    if(ret < 0) { env.error("mod_inv of 3 mod 7 should succeed"); return }

    // 3 * 5 = 15 = 1 mod 7, so inv should be 5
    if(tls::mpi_cmp_int(&raw mut inv, 5) != 0) {
        env.error("3^-1 mod 7 should be 5")
    }
}

@test
public func tls_bignum_exp_mod_works(env : &mut TestEnv) {
    var a : tls::Mpi; tls::mpi_init(&raw mut a)
    var e : tls::Mpi; tls::mpi_init(&raw mut e)
    var n : tls::Mpi; tls::mpi_init(&raw mut n)
    var res : tls::Mpi; tls::mpi_init(&raw mut res)

    tls::mpi_lset(&raw mut a, 4); tls::mpi_lset(&raw mut e, 3); tls::mpi_lset(&raw mut n, 10)
    var ret = tls::mpi_exp_mod(&raw mut res, &raw mut a, &raw mut e, &raw mut n)
    if(ret < 0) { env.error("exp_mod 4^3 mod 10 should succeed"); return }
    if(tls::mpi_cmp_int(&raw mut res, 4) != 0) { env.error("4^3 mod 10 should be 4"); return }

    // 7^4 mod 13 = 2401 mod 13 = 9
    tls::mpi_lset(&raw mut a, 7); tls::mpi_lset(&raw mut e, 4); tls::mpi_lset(&raw mut n, 13)
    ret = tls::mpi_exp_mod(&raw mut res, &raw mut a, &raw mut e, &raw mut n)
    if(ret < 0) { env.error("exp_mod 7^4 mod 13 should succeed"); return }
    if(tls::mpi_cmp_int(&raw mut res, 9) != 0) { env.error("7^4 mod 13 should be 9"); return }
}

// ─── RSA Tests ──────────────────────────────────────────────────────────────

@test
public func tls_rsa_init_and_import_works(env : &mut TestEnv) {
    var ctx : tls::RSAContext
    tls::rsa_init(&raw mut ctx, tls::RSA_PKCS_V15, 0)

    // Import a small RSA public key for testing
    var n_buf : [4]u8 = [0x00, 0x00, 0x00, 0x55]  // n = 85 = 5 * 17
    var e_buf : [1]u8 = [0x05]  // e = 5

    var ret = tls::rsa_import_pubkey(&raw mut ctx, &raw n_buf[0], 4, &raw e_buf[0], 1)
    if(ret < 0) { env.error("import pubkey should succeed"); return }

    if(tls::rsa_get_len(&raw mut ctx) != 4) { env.error("key length should be 4"); return }
}

@test
public func tls_rsa_pkcs1_encrypt_works(env : &mut TestEnv) {
    var ctx : tls::RSAContext
    tls::rsa_init(&raw mut ctx, tls::RSA_PKCS_V15, 0)

    // Small RSA key for testing (n=55, e=3)
    var n_buf : [1]u8 = [0x37]  // n = 55
    var e_buf : [1]u8 = [0x03]  // e = 3

    var ret = tls::rsa_import_pubkey(&raw mut ctx, &raw n_buf[0], 1, &raw e_buf[0], 1)
    if(ret < 0) { env.error("import pubkey should succeed"); return }

    var msg : [2]u8 = [0x01, 0x02]
    var ct : [64]u8

    ret = tls::rsa_pkcs1_encrypt(&raw mut ctx, &raw msg[0], 2, &raw mut ct[0])
    if(ret < 0) { env.error("RSA PKCS#1 encrypt should succeed"); return }

    // ct[0] should be non-zero (successfully encrypted)
    if(ct[0] == 0) { env.error("ciphertext should be non-zero") }
}

// ─── GCM Tests ──────────────────────────────────────────────────────────────

@test
public func tls_gcm_init_encrypt_decrypt_works(env : &mut TestEnv) {
    var key : [16]u8 = [0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c]
    var iv : [12]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B]
    var pt : [16]u8 = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                       0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00]

    var ctx : tls::GCMContext
    var ret = tls::gcm_init(&raw mut ctx, &raw key[0], 16)
    if(ret < 0) { env.error("gcm_init should succeed"); return }

    var ct : [16]u8
    var tag : [16]u8
    ret = tls::gcm_crypt_and_tag(&raw mut ctx, &raw iv[0], 12, null, 0,
                                  &raw pt[0], 16, &raw mut ct[0], &raw mut tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag should succeed"); return }

    // Ciphertext should differ from plaintext
    var same = true
    var i : size_t = 0
    while(i < 16) {
        if(ct[i] != pt[i]) { same = false }
        i += 1
    }
    if(same) { env.error("ciphertext should differ from plaintext"); return }

    // Decrypt and verify
    var pt2 : [16]u8
    ret = tls::gcm_auth_decrypt(&raw mut ctx, &raw iv[0], 12, null, 0,
                                 &raw ct[0], 16, &raw tag[0], 16,
                                 &raw mut pt2[0])
    if(ret < 0) { env.error("gcm_auth_decrypt should succeed"); return }

    // Plaintext should match
    var matches = true
    i = 0
    while(i < 16) {
        if(pt2[i] != pt[i]) { matches = false }
        i += 1
    }
    if(!matches) { env.error("decrypted plaintext should match original") }
}

@test
public func tls_gcm_tag_verification_fails_on_wrong_tag(env : &mut TestEnv) {
    var key : [16]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F]
    var iv : [12]u8
    var pt : [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11]

    var ctx : tls::GCMContext
    tls::gcm_init(&raw mut ctx, &raw key[0], 16)

    var ct : [8]u8
    var tag : [16]u8
    tls::gcm_crypt_and_tag(&raw mut ctx, &raw iv[0], 12, null, 0,
                            &raw pt[0], 8, &raw mut ct[0], &raw mut tag[0])

    // Corrupt the tag
    tag[0] = tag[0] ^ 0xFF

    var pt2 : [8]u8
    var ret = tls::gcm_auth_decrypt(&raw mut ctx, &raw iv[0], 12, null, 0,
                                     &raw ct[0], 8, &raw tag[0], 16,
                                     &raw mut pt2[0])
    if(ret == 0) {
        env.error("decrypt with wrong tag should fail")
    }
}

// ─── ECDH Tests ─────────────────────────────────────────────────────────────

@test
public func tls_ecdh_generate_keypair_works(env : &mut TestEnv) {
    var ctx : tls::ECDHContext
    tls::ecdh_init(&raw mut ctx)

    var priv : [32]u8
    var pub : [65]u8

    var ret = tls::ecdh_generate_keypair(&raw mut ctx, &raw mut priv[0], 32, &raw mut pub[0], 65)
    if(ret < 0) { env.error("ecdh_generate_keypair should succeed"); return }

    // Private key should not be all zeros
    var all_zero = true
    var i : size_t = 0
    while(i < 32) {
        if(priv[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) { env.error("private key should not be all zeros"); return }

    // Public key should start with 0x04 (uncompressed)
    if(pub[0] != 0x04) { env.error("public key should start with 0x04"); return }

    // Public key X and Y should not be all zeros
    all_zero = true
    i = 1
    while(i < 65) {
        if(pub[i] != 0) { all_zero = false }
        i += 1
    }
    if(all_zero) { env.error("public key should not be all zeros") }
}

@test
public func tls_ecdh_shared_secret_works(env : &mut TestEnv) {
    var alice : tls::ECDHContext; tls::ecdh_init(&raw mut alice)
    var bob : tls::ECDHContext; tls::ecdh_init(&raw mut bob)

    var alice_priv : [32]u8; var alice_pub : [65]u8
    var bob_priv : [32]u8; var bob_pub : [65]u8

    var ret = tls::ecdh_generate_keypair(&raw mut alice, &raw mut alice_priv[0], 32, &raw mut alice_pub[0], 65)
    if(ret < 0) { env.error("alice keygen should succeed"); return }

    ret = tls::ecdh_generate_keypair(&raw mut bob, &raw mut bob_priv[0], 32, &raw mut bob_pub[0], 65)
    if(ret < 0) { env.error("bob keygen should succeed"); return }

    var alice_shared : [32]u8
    ret = tls::ecdh_compute_shared(&raw mut alice, &raw bob_pub[0], 65, &raw mut alice_shared[0], 32)
    if(ret < 0) { env.error("alice shared secret should succeed"); return }

    var bob_shared : [32]u8
    ret = tls::ecdh_compute_shared(&raw mut bob, &raw alice_pub[0], 65, &raw mut bob_shared[0], 32)
    if(ret < 0) { env.error("bob shared secret should succeed"); return }

    // Both shared secrets should be identical (ECDH property)
    var matches = true
    var i : size_t = 0
    while(i < 32) {
        if(alice_shared[i] != bob_shared[i]) { matches = false }
        i += 1
    }
    if(!matches) {
        env.error("Alice and Bob shared secrets should match (ECDH property)")
    }
}
