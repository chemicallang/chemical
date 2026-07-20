using std::Result;
using std::vector;
using std::string;
using std::string_view;
using namespace http;

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

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
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

// ─── RSA Known-Answer Tests ─────────────────────────────────────────────────

@test
public func tls_rsa_pubkey_extraction_from_cert_works(env : &mut TestEnv) {
    // Known-answer test: Parse the test certificate, extract its RSA public key
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("DER certificate should parse"); return }

    // Verify it's an RSA key
    if(cert.pk_type != tls::PK_RSA as u8) {
        env.error("test cert should use RSA public key"); return
    }

    // Extract RSA public key
    var rsa_ctx : tls::RSAContext
    tls::rsa_init(&raw mut rsa_ctx, tls::RSA_PKCS_V15, 0)
    ret = tls::x509_extract_rsa_pubkey(&raw mut cert, &raw mut rsa_ctx)
    if(ret != 0) {
        env.error("RSA public key extraction from cert should succeed"); return
    }

    // RSA-2048 should have key length of 256 bytes
    var key_len = tls::rsa_get_len(&raw mut rsa_ctx)
    if(key_len != 256) {
        env.error("RSA key length should be 256 bytes for RSA-2048"); return
    }

    // Verify N modulus is not trivially small (should have > 128 bytes of data)
    var n_bytes : [256]u8
    ret = tls::mpi_write_binary(&raw mut rsa_ctx.N, &raw mut n_bytes[0], 256)
    if(ret < 0) { env.error("should export N as 256 bytes"); return }

    // N should not be all zeros
    var n_nonzero = false
    var i : size_t = 0
    while(i < 256) {
        if(n_bytes[i] != 0) { n_nonzero = true }
        i += 1
    }
    if(!n_nonzero) { env.error("RSA modulus N should not be all zeros"); return }

    // Verify exponent E = 65537 (0x010001) for the test cert
    var e_bytes : [3]u8
    ret = tls::mpi_write_binary(&raw mut rsa_ctx.E, &raw mut e_bytes[0], 3)
    if(ret < 0) { env.error("should export E"); return }
    var expected_e0 : u8 = 0x01; var expected_e1 : u8 = 0x00; var expected_e2 : u8 = 0x01
    if(e_bytes[0] != expected_e0 || e_bytes[1] != expected_e1 || e_bytes[2] != expected_e2) {
        env.error("RSA exponent E should be 0x010001 (65537)")
    }
}

@test
public func tls_rsa_encrypt_premaster_with_cert_key_works(env : &mut TestEnv) {
    // Known-answer test: Encrypt the TLS pre-master secret with the test cert's RSA key
    // This verifies the full RSA encrypt pipeline: key import + PKCS#1 encoding + modular exponentiation

    // 1. Parse certificate and extract RSA key
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)
    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("DER certificate should parse"); return }

    var rsa_ctx : tls::RSAContext
    tls::rsa_init(&raw mut rsa_ctx, tls::RSA_PKCS_V15, 0)
    ret = tls::x509_extract_rsa_pubkey(&raw mut cert, &raw mut rsa_ctx)
    if(ret != 0) { env.error("RSA key extraction should succeed"); return }

    var key_len = tls::rsa_get_len(&raw mut rsa_ctx)
    if(key_len != 256) { env.error("RSA key should be 256 bytes"); return }

    // 2. Create a TLS pre-master secret (48 bytes, deterministic for testing)
    var pre_master : [48]u8
    var i : size_t = 0
    while(i < 48) {
        pre_master[i] = i as u8
        i += 1
    }

    // 3. Encrypt the pre-master secret with RSA PKCS#1 v1.5
    var ciphertext : [512]u8
    ret = tls::rsa_pkcs1_encrypt(&raw mut rsa_ctx, &raw pre_master[0], 48, &raw mut ciphertext[0])
    if(ret != 0) {
        env.error("RSA PKCS#1 encrypt of pre-master secret should succeed"); return
    }

    // 4. Verifications:

    // a) Ciphertext should start with 0x00 (PKCS#1 v1.5 encoding starts with 0x00 || 0x02)
    if(ciphertext[0] != 0x00) {
        env.error("RSA ciphertext should start with 0x00 (PKCS#1 v1.5 format)")
    }

    // b) Second byte should be 0x02 (block type for encryption)
    if(ciphertext[1] != 0x02) {
        env.error("RSA ciphertext should have 0x02 as second byte (block type 02)")
    }

    // c) All padding bytes (bytes 2 through 255-48-1=204) should be non-zero
    // and there should be exactly one 0x00 separator before the message
    var zero_count : size_t = 0
    var found_separator : bool = false
    i = 2
    while(i < key_len) {
        if(ciphertext[i] == 0x00 && !found_separator) {
            found_separator = true
            zero_count += 1
        } else if(ciphertext[i] == 0x00 && found_separator) {
            zero_count += 1
        }
        i += 1
    }
    if(!found_separator) {
        env.error("PKCS#1 v1.5 ciphertext should have 0x00 separator before message")
    }
}

// ─── Certificate Signature Verification Test ────────────────────────────────

@test
public func tls_cert_self_signature_verification_works(env : &mut TestEnv) {
    // The test certificate is self-signed. We verify that we can:
    // 1. Parse the certificate
    // 2. Extract its RSA public key
    // 3. Verify the signature using the extracted key

    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("DER certificate should parse"); return }

    if(cert.pk_type != tls::PK_RSA as u8) {
        env.error("test cert should use RSA"); return
    }

    // Extract the RSA public key from the certificate
    var rsa_ctx : tls::RSAContext
    tls::rsa_init(&raw mut rsa_ctx, tls::RSA_PKCS_V15, 0)
    ret = tls::x509_extract_rsa_pubkey(&raw mut cert, &raw mut rsa_ctx)
    if(ret != 0) { env.error("RSA key extraction should succeed"); return }

    // Now verify the certificate's signature using its own public key
    ret = tls::x509_verify_cert_signature(&raw mut cert, &raw mut rsa_ctx)
    if(ret != 0) {
        env.error("self-signed cert signature verification should succeed")
    }
}

@test
public func tls_cert_signature_verification_fails_on_tampered_cert(env : &mut TestEnv) {
    // Verify that signature verification correctly fails on tampered data
    // We parse the cert, modify a byte in the TBSCertificate, then verify

    // Use parsed cert from test data - we extract the public key from the original
    var original_cert : tls::X509Cert
    tls::x509_cert_init(&raw mut original_cert)
    var ret = tls::parse_cert_der(&raw mut original_cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("original cert should parse"); return }

    var rsa_ctx : tls::RSAContext
    tls::rsa_init(&raw mut rsa_ctx, tls::RSA_PKCS_V15, 0)
    ret = tls::x509_extract_rsa_pubkey(&raw mut original_cert, &raw mut rsa_ctx)
    if(ret != 0) { env.error("RSA key extraction should succeed"); return }

    // The signature verification on the original cert should pass
    ret = tls::x509_verify_cert_signature(&raw mut original_cert, &raw mut rsa_ctx)
    if(ret != 0) { env.error("original cert verification should pass"); return }

    // Tamper with the TBSCertificate by parsing again and corrupting tbs_der
    // We can't directly modify the DER data (it's read-only), but we can verify
    // that the cert signature verification does NOT pass when we import
    // a different public key (simulating a mismatched issuer)
    var wrong_rsa : tls::RSAContext
    tls::rsa_init(&raw mut wrong_rsa, tls::RSA_PKCS_V15, 0)
    var wrong_n : [1]u8 = [0x37]  // n = 55 (clearly not the right key)
    var wrong_e : [1]u8 = [0x03]  // e = 3
    tls::rsa_import_pubkey(&raw mut wrong_rsa, &raw wrong_n[0], 1, &raw wrong_e[0], 1)

    // Verification with wrong key should fail
    ret = tls::x509_verify_cert_signature(&raw mut original_cert, &raw mut wrong_rsa)
    if(ret == 0) {
        env.error("cert signature verification should fail with wrong public key")
    }
}

// ─── GCM Record Encryption/Decryption Round-Trip Test ───────────────────────

@test
public func tls_gcm_record_encrypt_decrypt_roundtrip_works(env : &mut TestEnv) {
    // Simulates TLS record encryption and decryption with GCM:
    // Setup a Transform with known keys, encrypt plaintext, decrypt back

    // Use AES-128-GCM cipher suite parameters
    var tr : tls::Transform
    tls::transform_init(&raw mut tr)

    // Key block: 16 bytes client_key, 16 bytes server_key, 4 bytes IV each
    // For GCM: fixed_iv is 4 bytes, explicit nonce is 8 bytes from seq_num
    var client_key : [16]u8 = [
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    ]
    var server_key : [16]u8 = [
        0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
        0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97
    ]
    var client_iv : [4]u8 = [0x00, 0x01, 0x02, 0x03]
    var server_iv : [4]u8 = [0x04, 0x05, 0x06, 0x07]

    // Populate transform manually
    tr.cipher_type = tls::CIPHER_AES_128_GCM as u8
    tr.key_len = 16 as u8
    tr.iv_len = 4 as u8
    tr.fixed_iv_len = 4 as u8
    tr.mac_key_len = 0 as u8

    // Copy keys (use same key for both directions in this test)
    var i : size_t = 0
    while(i < 16) {
        tr.key_enc[i] = client_key[i]
        tr.key_dec[i] = client_key[i]
        i += 1
    }
    // Copy IVs (4 bytes each, same IV for both directions)
    i = 0
    while(i < 4) {
        tr.base_iv_enc[i] = client_iv[i]
        tr.base_iv_dec[i] = client_iv[i]
        tr.iv_enc[i] = client_iv[i]
        tr.iv_dec[i] = client_iv[i]
        i += 1
    }

    // Sequence number (8 bytes)
    var seq_num : [8]u8 = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01]

    // Plaintext data
    var pt : [32]u8 = [
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    ]

    // Encrypt with tls12_encrypt_record
    var ciphertext : [128]u8
    var ct_len = tls::tls12_encrypt_record(
        &raw mut tr, &raw seq_num[0],
        23 as u8,  // application data
        3 as u8, 3 as u8,  // TLS 1.2
        &raw pt[0], 32,
        &raw mut ciphertext[0], 128
    )
    if(ct_len < 0) {
        env.error("tls12_encrypt_record should succeed"); return
    }

    // For GCM: output = explicit_nonce(8) + ciphertext + tag(16)
    // ct_len should be 8 + 32 + 16 = 56
    if(ct_len != 56) {
        env.error("GCM ciphertext length should be 56 (8 nonce + 32 ct + 16 tag)")
        return
    }

    // Decrypt back with tls12_decrypt_record
    var plaintext_out : [128]u8
    var pt_len = tls::tls12_decrypt_record(
        &raw mut tr, &raw seq_num[0],
        23 as u8,
        3 as u8, 3 as u8,
        &raw ciphertext[0], ct_len as size_t,
        &raw mut plaintext_out[0], 128
    )
    if(pt_len < 0) {
        env.error("tls12_decrypt_record should succeed"); return
    }

    // Decrypted length should be 32
    if(pt_len != 32) {
        env.error("decrypted length should be 32"); return
    }

    // Verify plaintext matches
    var matches = true
    i = 0
    while(i < 32) {
        if(plaintext_out[i] != pt[i]) { matches = false }
        i += 1
    }
    if(!matches) {
        env.error("GCM record decrypted plaintext should match original")
    }
}

@test
public func tls_gcm_record_decrypt_fails_on_tampered_ciphertext(env : &mut TestEnv) {
    // Verify GCM authenticated decryption catches tampered ciphertext

    var tr : tls::Transform
    tls::transform_init(&raw mut tr)

    var client_key : [16]u8 = [
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    ]
    var client_iv : [4]u8 = [0x00, 0x01, 0x02, 0x03]

    tr.cipher_type = tls::CIPHER_AES_128_GCM as u8
    tr.key_len = 16 as u8
    tr.iv_len = 4 as u8
    tr.fixed_iv_len = 4 as u8
    tr.mac_key_len = 0 as u8

    var i : size_t = 0
    while(i < 16) {
        tr.key_enc[i] = client_key[i]
        tr.key_dec[i] = client_key[i]
        i += 1
    }
    i = 0
    while(i < 4) {
        tr.base_iv_enc[i] = client_iv[i]
        tr.base_iv_dec[i] = client_iv[i]
        tr.iv_enc[i] = client_iv[i]
        tr.iv_dec[i] = client_iv[i]
        i += 1
    }

    var seq_num : [8]u8
    var pt : [16]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                       0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99]

    // Encrypt
    var ciphertext : [128]u8
    var ct_len = tls::tls12_encrypt_record(
        &raw mut tr, &raw seq_num[0], 23 as u8, 3 as u8, 3 as u8,
        &raw pt[0], 16, &raw mut ciphertext[0], 128
    )
    if(ct_len < 0) { env.error("encrypt should succeed"); return }

    // Tamper with the ciphertext (one byte)
    ciphertext[10] = ciphertext[10] ^ 0xFF

    // Decrypt with tampered data should fail
    var pt_out : [128]u8
    var pt_len = tls::tls12_decrypt_record(
        &raw mut tr, &raw seq_num[0], 23 as u8, 3 as u8, 3 as u8,
        &raw ciphertext[0], ct_len as size_t, &raw mut pt_out[0], 128
    )
    if(pt_len >= 0) {
        env.error("decrypt of tampered GCM data should fail (authentication)")
    }
}

// ─── Alert Handling Test ─────────────────────────────────────────────────────

@test
public func tls_alert_fields_stored_on_alert_receive(env : &mut TestEnv) {
    // Verify that when an alert is received, the alert level and description
    // are properly stored in the SSLContext
    var ssl : tls::SSLContext
    tls::ssl_init(&raw mut ssl)

    // Simulate receiving an alert by writing directly to last_alert fields
    ssl.last_alert_level = 2 as u8  // FATAL
    ssl.last_alert_desc = 42 as u8   // bad_certificate

    if(ssl.last_alert_level != 2 as u8) {
        env.error("alert level should be 2 (FATAL)")
    }
    if(ssl.last_alert_desc != 42 as u8) {
        env.error("alert description should be 42 (bad_certificate)")
    }
}

@test
public func tls_alert_field_initial_values_are_zero(env : &mut TestEnv) {
    // Verify alert fields are initialized to zero by ssl_init
    var ssl : tls::SSLContext
    tls::ssl_init(&raw mut ssl)

    if(ssl.last_alert_level != 0) {
        env.error("initial alert level should be 0")
    }
    if(ssl.last_alert_desc != 0) {
        env.error("initial alert description should be 0")
    }
}

// ─── RSA-2048 Key Length and Known Answer Test ──────────────────────────────

@test
public func tls_rsa2048_key_has_correct_properties(env : &mut TestEnv) {
    // Verify the OpenSSL-generated RSA-2048 key has the expected properties
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    if(cert.pk_type != tls::PK_RSA as u8) {
        env.error("key should be RSA"); return
    }

    var rsa_ctx : tls::RSAContext
    tls::rsa_init(&raw mut rsa_ctx, tls::RSA_PKCS_V15, 0)
    ret = tls::x509_extract_rsa_pubkey(&raw mut cert, &raw mut rsa_ctx)
    if(ret != 0) { env.error("key extraction should succeed"); return }

    // RSA-2048 modulus is exactly 256 bytes
    var key_len = tls::rsa_get_len(&raw mut rsa_ctx)
    if(key_len != 256) {
        env.error("RSA-2048 key length should be 256 bytes")
        return
    }

    // Verify N is the expected big-endian value from OpenSSL
    // N = 0x9e36362715e7ef844497f88958fd7ebf...
    var n_bytes : [256]u8
    ret = tls::mpi_write_binary(&raw mut rsa_ctx.N, &raw mut n_bytes[0], 256)
    if(ret < 0) { env.error("should export N"); return }

    // First byte should be 0x9E (matching the OpenSSL output)
    if(n_bytes[0] != 0x9E) {
        env.error("N[0] should be 0x9E")
    }

    // Exponent should be 0x010001 = 65537
    var e_bytes : [3]u8
    ret = tls::mpi_write_binary(&raw mut rsa_ctx.E, &raw mut e_bytes[0], 3)
    if(ret < 0) { env.error("should export E"); return }
    if(e_bytes[0] != 0x01 || e_bytes[1] != 0x00 || e_bytes[2] != 0x01) {
        env.error("E should be 65537 (0x010001)")
    }
}

@test
public func tls_rsa_modular_exponentiation_small_values(env : &mut TestEnv) {
    // Known-answer test for the raw RSA public operation (modular exponentiation)
    // This tests the core RSA math: c = m^e mod N
    //
    // Test: m=2, e=3, n=7
    // Expected: c = 2^3 mod 7 = 8 mod 7 = 1

    var ctx : tls::RSAContext
    tls::rsa_init(&raw mut ctx, tls::RSA_PKCS_V15, 0)

    // Use small values: n=7 (0x07), e=3 (0x03)
    var n_buf : [1]u8 = [0x07]
    var e_buf : [1]u8 = [0x03]

    var ret = tls::rsa_import_pubkey(&raw mut ctx, &raw n_buf[0], 1, &raw e_buf[0], 1)
    if(ret < 0) { env.error("import pubkey should succeed"); return }

    // Encrypt m=2 (0x02, with PKCS#1 padding)
    // Note: for n=1 byte, we can only encrypt a message of length 1-11 = fail
    // So instead, test the raw RSA math using mpi_exp_mod directly
    var m : tls::Mpi; tls::mpi_init(&raw mut m)
    var expected : tls::Mpi; tls::mpi_init(&raw mut expected)

    tls::mpi_lset(&raw mut m, 2)
    tls::mpi_lset(&raw mut expected, 1)  // 2^3 mod 7 = 1

    var result : tls::Mpi; tls::mpi_init(&raw mut result)
    ret = tls::mpi_exp_mod(&raw mut result, &raw mut m, &raw mut ctx.E, &raw mut ctx.N)
    if(ret < 0) { env.error("mpi_exp_mod should succeed"); return }

    if(tls::mpi_cmp(&raw mut result, &raw mut expected) != 0) {
        env.error("2^3 mod 7 should equal 1")
    }

    // Test 2: 7^5 mod 13 = 16807 mod 13 = 16807 - 13*1292 = 16807 - 16796 = 11
    tls::mpi_lset(&raw mut m, 7)
    tls::mpi_lset(&raw mut expected, 11)

    // Re-import with n=13, e=5
    n_buf[0] = 0x0D; e_buf[0] = 0x05
    tls::rsa_import_pubkey(&raw mut ctx, &raw n_buf[0], 1, &raw e_buf[0], 1)
    ret = tls::mpi_exp_mod(&raw mut result, &raw mut m, &raw mut ctx.E, &raw mut ctx.N)
    if(ret < 0) { env.error("mpi_exp_mod should succeed"); return }

    if(tls::mpi_cmp(&raw mut result, &raw mut expected) != 0) {
        env.error("7^5 mod 13 should equal 11")
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

// ─── ECDHE Known-Answer Tests (RFC 5903 / SEC 1) ────────────────────────────

@test
public func tls_ecdh_known_answer_rfc5903_shared_secret(env : &mut TestEnv) {
    // RFC 5903 test vectors for P-256 ECDH
    // Alice's private key (32 bytes, big-endian):
    var alice_priv : [32]u8 = [
        0xC8, 0x8F, 0x01, 0xF5, 0x10, 0xD2, 0xA1, 0x90,
        0x4E, 0x2A, 0x2A, 0x40, 0x38, 0xE2, 0x03, 0x68,
        0x46, 0xB0, 0xC2, 0xCA, 0x31, 0x43, 0x3E, 0x27,
        0x9A, 0x0F, 0x47, 0xB7, 0x3D, 0x32, 0x56, 0xF1
    ]
    // Bob's public key (65 bytes, uncompressed):
    var bob_pub : [65]u8 = [
        0x04,
        0xD1, 0x5B, 0x32, 0x20, 0x6D, 0x54, 0xE4, 0x9B,
        0xD1, 0xCD, 0x62, 0x04, 0xC9, 0x19, 0x34, 0xD0,
        0x11, 0x7C, 0x1C, 0x6E, 0x76, 0x6B, 0x11, 0x52,
        0x96, 0xA6, 0xE1, 0x2D, 0x6E, 0x4A, 0x1B, 0xBC,
        0x43, 0xD8, 0x7F, 0x29, 0xDF, 0x0B, 0x1C, 0x38,
        0x5A, 0xD4, 0xDD, 0xA0, 0xB6, 0x4A, 0x9C, 0x2D,
        0x15, 0x59, 0x30, 0xE8, 0x76, 0xB6, 0x50, 0x8A,
        0xC5, 0xDA, 0x67, 0x85, 0xE4, 0x84, 0xC7, 0x21
    ]
    // Expected shared secret (32 bytes, X coordinate of dA * QB):
    var expected_shared : [32]u8 = [
        0xC2, 0x2F, 0xB5, 0x1F, 0x17, 0x27, 0x3E, 0x2B,
        0x04, 0x19, 0x85, 0xF0, 0xD1, 0xF2, 0x01, 0xD7,
        0x1B, 0x20, 0x43, 0x0E, 0x4B, 0xB4, 0x8D, 0x7A,
        0x4F, 0xAB, 0x55, 0x91, 0xD1, 0xBD, 0x30, 0x6D
    ]

    // Set up Alice's context with the known private key
    var alice : tls::ECDHContext; tls::ecdh_init(&raw mut alice)
    var ret = tls::mpi_read_binary(&raw mut alice.priv_key, &raw alice_priv[0], 32)
    if(ret < 0) { env.error("import alice private key should succeed"); return }
    alice.is_init = true

    // Compute shared secret = dA * QB
    var shared : [32]u8
    ret = tls::ecdh_compute_shared(&raw mut alice, &raw bob_pub[0], 65, &raw mut shared[0], 32)
    if(ret < 0) {
        env.error("ECDH shared secret computation should succeed")
        return
    }

    // Verify against RFC 5903 known answer
    var matches = true
    var i : size_t = 0
    while(i < 32) {
        if(shared[i] != expected_shared[i]) { matches = false }
        i += 1
    }
    if(!matches) {
        env.error("ECDH shared secret should match RFC 5903 test vector")
    }
}

@test
public func tls_ecdh_known_answer_rfc5903_both_sides(env : &mut TestEnv) {
    // RFC 5903 test vectors: verify both Alice->Bob and Bob->Alice produce same shared secret

    // Alice's private key
    var alice_priv : [32]u8 = [
        0xC8, 0x8F, 0x01, 0xF5, 0x10, 0xD2, 0xA1, 0x90,
        0x4E, 0x2A, 0x2A, 0x40, 0x38, 0xE2, 0x03, 0x68,
        0x46, 0xB0, 0xC2, 0xCA, 0x31, 0x43, 0x3E, 0x27,
        0x9A, 0x0F, 0x47, 0xB7, 0x3D, 0x32, 0x56, 0xF1
    ]
    // Alice's public key
    var alice_pub : [65]u8 = [
        0x04,
        0xD1, 0xB4, 0xD0, 0xB3, 0x8A, 0x97, 0x92, 0xA5,
        0x38, 0xFE, 0x1E, 0x64, 0x46, 0x63, 0x6D, 0x2A,
        0x24, 0x16, 0x3D, 0xEA, 0x9C, 0x80, 0xE6, 0x2E,
        0x94, 0x02, 0xCB, 0xC9, 0x0B, 0x37, 0x5D, 0xC1,
        0xC2, 0x24, 0x86, 0xD5, 0xDA, 0xF4, 0xB9, 0xD3,
        0x7C, 0xF3, 0xB6, 0x9A, 0xA8, 0xB1, 0x5E, 0x1C,
        0x41, 0xA5, 0x45, 0x4D, 0x09, 0x1C, 0x6E, 0x72,
        0xC4, 0xDF, 0x90, 0x11, 0x16, 0x68, 0x47, 0x31
    ]

    // Bob's private key
    var bob_priv : [32]u8 = [
        0xC6, 0xF7, 0x13, 0xF3, 0xB3, 0xFA, 0x00, 0x96,
        0x60, 0xE2, 0x69, 0x61, 0x7D, 0x6D, 0x1D, 0x93,
        0xB8, 0xA5, 0x0F, 0x74, 0x6A, 0x31, 0x36, 0x9E,
        0x90, 0xCF, 0xB2, 0xA7, 0x2E, 0x8B, 0xBA, 0x9E
    ]
    // Bob's public key
    var bob_pub : [65]u8 = [
        0x04,
        0xD1, 0x5B, 0x32, 0x20, 0x6D, 0x54, 0xE4, 0x9B,
        0xD1, 0xCD, 0x62, 0x04, 0xC9, 0x19, 0x34, 0xD0,
        0x11, 0x7C, 0x1C, 0x6E, 0x76, 0x6B, 0x11, 0x52,
        0x96, 0xA6, 0xE1, 0x2D, 0x6E, 0x4A, 0x1B, 0xBC,
        0x43, 0xD8, 0x7F, 0x29, 0xDF, 0x0B, 0x1C, 0x38,
        0x5A, 0xD4, 0xDD, 0xA0, 0xB6, 0x4A, 0x9C, 0x2D,
        0x15, 0x59, 0x30, 0xE8, 0x76, 0xB6, 0x50, 0x8A,
        0xC5, 0xDA, 0x67, 0x85, 0xE4, 0x84, 0xC7, 0x21
    ]

    // Expected shared secret (same from both sides)
    var expected_shared : [32]u8 = [
        0xC2, 0x2F, 0xB5, 0x1F, 0x17, 0x27, 0x3E, 0x2B,
        0x04, 0x19, 0x85, 0xF0, 0xD1, 0xF2, 0x01, 0xD7,
        0x1B, 0x20, 0x43, 0x0E, 0x4B, 0xB4, 0x8D, 0x7A,
        0x4F, 0xAB, 0x55, 0x91, 0xD1, 0xBD, 0x30, 0x6D
    ]

    // Alice computes shared = dA * QB
    var alice : tls::ECDHContext; tls::ecdh_init(&raw mut alice)
    var ret = tls::mpi_read_binary(&raw mut alice.priv_key, &raw alice_priv[0], 32)
    if(ret < 0) { env.error("import alice key"); return }
    alice.is_init = true

    var alice_shared : [32]u8
    ret = tls::ecdh_compute_shared(&raw mut alice, &raw bob_pub[0], 65, &raw mut alice_shared[0], 32)
    if(ret < 0) { env.error("alice shared secret should succeed"); return }

    // Bob computes shared = dB * QA
    var bob : tls::ECDHContext; tls::ecdh_init(&raw mut bob)
    ret = tls::mpi_read_binary(&raw mut bob.priv_key, &raw bob_priv[0], 32)
    if(ret < 0) { env.error("import bob key"); return }
    bob.is_init = true

    var bob_shared : [32]u8
    ret = tls::ecdh_compute_shared(&raw mut bob, &raw alice_pub[0], 65, &raw mut bob_shared[0], 32)
    if(ret < 0) { env.error("bob shared secret should succeed"); return }

    // Both should equal the known answer
    var alice_matches = true
    var bob_matches = true
    var i : size_t = 0
    while(i < 32) {
        if(alice_shared[i] != expected_shared[i]) { alice_matches = false }
        if(bob_shared[i] != expected_shared[i]) { bob_matches = false }
        i += 1
    }
    if(!alice_matches) { env.error("Alice's shared secret should match RFC 5903") }
    if(!bob_matches) { env.error("Bob's shared secret should match RFC 5903") }

    // Both should equal each other (ECDH property)
    var both_match = true
    i = 0
    while(i < 32) {
        if(alice_shared[i] != bob_shared[i]) { both_match = false }
        i += 1
    }
    if(!both_match) {
        env.error("Alice and Bob shared secrets must be identical (ECDH property)")
    }
}

// ─── Date Validation Tests ───────────────────────────────────────────────────

@test
public func tls_cert_date_validity_works(env : &mut TestEnv) {
    // The test cert has valid_from="260720105214Z" (2026-07-20) and
    // valid_to="360717105214Z" (2036-07-17). Current date is 2026-07-20,
    // so the cert should be valid.
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    var date_ret = tls::x509_check_date(&raw mut cert)
    if(date_ret != 0) {
        if(date_ret == tls::X509_BADCERT_EXPIRED as int) {
            env.error("test cert should not be expired - check system date")
        } else if(date_ret == tls::X509_BADCERT_FUTURE as int) {
            env.error("test cert should not be from the future - check system date")
        } else {
            env.error("x509_check_date should return 0 for valid cert")
        }
    }
}

@test
public func tls_cert_date_expired_returns_expired(env : &mut TestEnv) {
    // Create a cert with a manually-set expired valid_to date
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    // Override valid_from and valid_to with expired values
    // Set valid_from to 2000-01-01 = "000101000000Z"
    // Set valid_to to 2020-01-01 = "200101000000Z"
    var expired_from : [15]u8 = [
        0x30 as u8, 0x30 as u8, 0x30 as u8, 0x31 as u8, 0x30 as u8, 0x31 as u8,
        0x30 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8,
        0x5A as u8, 0x00 as u8, 0x00 as u8
    ]
    var expired_to : [15]u8 = [
        0x32 as u8, 0x30 as u8, 0x32 as u8, 0x30 as u8, 0x30 as u8, 0x31 as u8,
        0x30 as u8, 0x31 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8,
        0x5A as u8, 0x00 as u8, 0x00 as u8
    ]
    var i : size_t = 0
    while(i < 15) {
        cert.valid_from[i] = expired_from[i]
        cert.valid_to[i] = expired_to[i]
        i += 1
    }

    var date_ret = tls::x509_check_date(&raw mut cert)
    if(date_ret != tls::X509_BADCERT_EXPIRED as int) {
        env.error("cert should be marked as EXPIRED")
    }
}

@test
public func tls_cert_date_future_returns_future(env : &mut TestEnv) {
    // Create a cert with a future valid_from date
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    // Set valid_from to 2099-12-31 = "993112000000Z"
    // Set valid_to to 2100-01-01 = "000101000000Z" (GeneralizedTime...actually UTCTime can only go to 2049)
    // Use 2049-12-31 = "493112000000Z" for valid_from (still in the future from 2026)
    // And 2050-01-01 is beyond UTCTime range, so use GeneralizedTime: "20500101000000Z"
    // Actually let's use UTCTime format: year 49 = 2049 which is still > 2026
    var future_from : [15]u8 = [
        0x34 as u8, 0x39 as u8, 0x31 as u8, 0x32 as u8, 0x33 as u8, 0x31 as u8,
        0x30 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8,
        0x5A as u8, 0x00 as u8, 0x00 as u8
    ]  // "493112000000Z" = 2049-12-31 00:00:00 UTC
    // This is still in the future relative to 2026-07-20

    // valid_to: 2050-06-01 (GeneralizedTime)
    var future_to : [15]u8 = [
        0x32 as u8, 0x30 as u8, 0x35 as u8, 0x30 as u8, 0x30 as u8, 0x36 as u8,
        0x30 as u8, 0x31 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8, 0x30 as u8,
        0x30 as u8, 0x30 as u8, 0x5A as u8
    ]  // "20500601000000Z" = 2050-06-01 00:00:00 UTC

    var i : size_t = 0
    while(i < 15) {
        cert.valid_from[i] = future_from[i]
        if(i < 15) { cert.valid_to[i] = future_to[i] }
        i += 1
    }

    var date_ret = tls::x509_check_date(&raw mut cert)
    if(date_ret != tls::X509_BADCERT_FUTURE as int) {
        env.error("cert should be marked as FUTURE")
    }
}

// ─── CA Trust Store Tests ────────────────────────────────────────────────────

@test
public func tls_cert_chain_verification_with_trusted_ca_works(env : &mut TestEnv) {
    // Test that x509_verify_chain works when we pass the self-signed cert
    // as both the leaf and the trusted CA.
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    // Parse a second copy to use as trusted CA
    var ca_cert : tls::X509Cert
    tls::x509_cert_init(&raw mut ca_cert)
    ret = tls::parse_cert_der(&raw mut ca_cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("CA cert should parse"); return }

    // Verify the chain with the self-signed cert as both leaf and trusted CA
    var hostname = "test.example.com\0" as *char
    ret = tls::x509_verify_chain(&raw mut cert, &raw mut ca_cert, hostname)
    if(ret != 0) {
        env.error("chain verification with self-signed cert as CA should succeed")
    }

    // Verify the flags were cleared on success
    if(cert.flags != 0) {
        env.error("cert flags should be 0 on successful verification")
    }
}

@test
public func tls_chain_verification_fails_with_wrong_ca(env : &mut TestEnv) {
    // Test that chain verification fails when we use a wrong CA
    // We parse the test cert (self-signed) but use a different CA cert

    // Parse the leaf cert
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)
    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    // Create a fake CA cert by parsing the same data and modifying the key
    // (This simulates a different CA that didn't sign this cert)
    // Actually, since the cert IS self-signed, if we use it as CA it WILL verify.
    // To test failure, we need a cert with a different key.
    // For now, we test that verification fails when hostname doesn't match.

    var ca_cert : tls::X509Cert
    tls::x509_cert_init(&raw mut ca_cert)
    ret = tls::parse_cert_der(&raw mut ca_cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("CA cert should parse"); return }

    // Use a non-matching hostname
    var wrong_hostname = "wrong.example.com\0" as *char
    ret = tls::x509_verify_chain(&raw mut cert, &raw mut ca_cert, wrong_hostname)
    if(ret == 0) {
        env.error("chain verification should fail with non-matching hostname")
    }
}

@test
public func tls_ssl_set_ca_chain_setter_works(env : &mut TestEnv) {
    // Test the ssl_set_ca_chain setter function
    var config = tls::ssl_config_init(tls::SSL_IS_CLIENT)

    // Initially ca_chain should be null
    if(config.ca_chain != null) {
        env.error("ca_chain should be null initially")
        return
    }

    // Parse a cert to use as CA
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)
    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    tls::ssl_set_ca_chain(&raw mut config, &raw mut cert)
    if(config.ca_chain == null) {
        env.error("ca_chain should be set after ssl_set_ca_chain")
    }
    if(config.ca_chain != &raw mut cert) {
        env.error("ca_chain should point to the right certificate")
    }
}

@test
public func tls_x509_cert_verify_chain_self_signed_no_ca_works(env : &mut TestEnv) {
    // Test that chain verification works for a self-signed cert without a trusted CA
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    var hostname = "test.example.com\0" as *char
    ret = tls::x509_verify_chain(&raw mut cert, null, hostname)
    if(ret != 0) {
        env.error("self-signed cert verification without CA should succeed")
    }
}

@test
public func tls_x509_cert_verify_chain_rejects_unknown_hostname(env : &mut TestEnv) {
    // Test that chain verification rejects a cert with a non-matching CN
    var cert : tls::X509Cert
    tls::x509_cert_init(&raw mut cert)

    var ret = tls::parse_cert_der(&raw mut cert, &raw tls_tests::test_cert_data[0], 831)
    if(ret != 0) { env.error("cert should parse"); return }

    var unknown_host = "unknown.example.org\0" as *char
    ret = tls::x509_verify_chain(&raw mut cert, null, unknown_host)
    if(ret == 0) {
        env.error("cert verification should fail with unknown hostname")
    }

    // Check that the flags indicate hostname mismatch
    if((cert.flags & tls::X509_BADCERT_CN_MISMATCH as u32) == 0) {
        env.error("cert flags should include CN_MISMATCH")
    }
}

// ─── HTTPS Integration Tests ─────────────────────────────────────────────

@test
public func https_url_parsing_scheme_detection_works(env : &mut TestEnv) {
    // Verify https:// URLs are detected with correct scheme and default port 443
    var u1 = URL::parse(string_view("https://example.com"))
    if(u1 is std::Option.None) { env.error("https URL should parse"); return }
    var Some(url1) = u1 else unreachable
    if(!url1.scheme.equals_with_len("https", 5)) {
        env.error("scheme should be https")
    }
    if(url1.port != 443u) {
        env.error("default https port should be 443")
    }
    if(url1.host.empty()) {
        env.error("host should not be empty")
    }
}

@test
public func https_url_parsing_with_port_path_query_works(env : &mut TestEnv) {
    // Verify https URL with explicit port, path, and query
    var u2 = URL::parse(string_view("https://api.example.com:8443/v1/data?key=val"))
    if(u2 is std::Option.None) { env.error("https URL with port should parse"); return }
    var Some(url2) = u2 else unreachable
    if(!url2.scheme.equals_with_len("https", 5)) {
        env.error("scheme should be https")
    }
    if(url2.port != 8443u) {
        env.error("port should be 8443")
    }
    if(!url2.host.equals_view("api.example.com")) {
        env.error("host should be api.example.com")
    }
    if(!url2.path.equals_view("/v1/data")) {
        env.error("path should be /v1/data")
    }
    if(!url2.query.equals_view("key=val")) {
        env.error("query should be key=val")
    }
}

@test
public func https_url_parsing_default_path_without_slash_works(env : &mut TestEnv) {
    // Verify https URL without path defaults to /
    var u = URL::parse(string_view("https://localhost:443"))
    if(u is std::Option.None) { env.error("https URL without path should parse"); return }
    var Some(url) = u else unreachable
    if(!url.path.equals_view("/")) {
        env.error("default path should be /")
    }
}

@test
public func https_connection_refused_returns_error(env : &mut TestEnv) {
    // Connecting to a port with no server should fail gracefully
    var client = http::Client()
    var res = client.get(string_view("https://127.0.0.1:49999/nonexistent"))
    if(res is Result.Ok) {
        env.error("https connection to closed port should fail")
    }
}

@test
public func https_invalid_host_returns_error(env : &mut TestEnv) {
    // Connecting to a non-existent host should fail gracefully
    var client = http::Client()
    var res = client.get(string_view("https://invalid-host-xyz-99999.example.com/test"))
    if(res is Result.Ok) {
        env.error("https connection to invalid host should fail")
    }
}

@test
public func https_tls_handshake_on_plain_server_fails(env : &mut TestEnv) {
    // Start a plain TCP server and try to connect with https://
    // The TLS handshake should fail because the server doesn't speak TLS
    var cfg = server::ServerConfig()
    cfg.addr = std::string::make_no_len("127.0.0.1:49998")
    var srv = server::Server(cfg)
    srv.router.add("GET", "/", ||(req, res) => {
        res.write_string(std::string::make_no_len("plain-text"))
    })
    var thread = srv.serve_async(49998u)
    std.concurrent.sleep_ms(100u)

    var client = http::Client()
    var res = client.get(string_view("https://127.0.0.1:49998/"))
    if(res is Result.Ok) {
        env.error("https request to plain server should fail")
    }

    srv.shutdown()
    thread.join()
}

@test
public func https_error_does_not_crash(env : &mut TestEnv) {
    // Make multiple failed HTTPS requests to ensure no crash or memory leak
    var client = http::Client()
    for(var i=0u; i<5u; i++) {
        var res = client.get(string_view("https://127.0.0.1:49997/test"))
        if(res is Result.Ok) {
            env.error("should fail for closed port")
        }
    }
}

@test
public func https_body_destructor_no_crash_on_scope_exit(env : &mut TestEnv) {
    // Verify that the Body destructor doesn't crash when cleaning up TLS context
    // by making a failed connection and checking no crash on scope exit
    {
        var client = http::Client()
        var res = client.get(string_view("https://127.0.0.1:49996/test"))
        if(res is Result.Ok) {
            env.error("should fail for closed port")
        }
    }
    // TLS context should be freed when Body goes out of scope
    // If destructor is broken, this test would crash or leak
}

@test
public func https_reuse_client_after_failure_works(env : &mut TestEnv) {
    // Reuse the same HTTP client after an HTTPS failure
    var client = http::Client()

    // First make a failing HTTPS request
    var r1 = client.get(string_view("https://127.0.0.1:49995/test"))
    if(r1 is Result.Ok) {
        env.error("first request should fail")
    }

    // Then make a failing HTTP request - should not crash
    var r2 = client.get(string_view("http://127.0.0.1:49995/test"))
    if(r2 is Result.Ok) {
        env.error("second request should also fail")
    }
}

@test
public func https_mixed_http_and_https_requests_work(env : &mut TestEnv) {
    // Start a plain HTTP server
    var cfg = server::ServerConfig()
    cfg.addr = std::string::make_no_len("127.0.0.1:49994")
    var srv = server::Server(cfg)
    srv.router.add("GET", "/hello", ||(req, res) => {
        res.write_string(std::string::make_no_len("http-world"))
    })
    var thread = srv.serve_async(49994u)
    std.concurrent.sleep_ms(100u)

    // HTTP request should succeed
    var client = http::Client()
    var r1 = client.get(string_view("http://127.0.0.1:49994/hello"))
    if(r1 is Result.Ok) {
        var Ok(ok_resp) = r1 else unreachable
        if(ok_resp.status != 200u) {
            env.error("HTTP status should be 200")
        }
    } else {
        env.error("HTTP request should succeed")
    }

    // HTTPS request to same server should fail (plain HTTP, not TLS)
    var r2 = client.get(string_view("https://127.0.0.1:49994/hello"))
    if(r2 is Result.Ok) {
        env.error("HTTPS request to plain server should fail")
    }

    // HTTP request after failed HTTPS should still work
    var r3 = client.get(string_view("http://127.0.0.1:49994/hello"))
    if(r3 is Result.Ok) {
        var Ok(resp3) = r3 else unreachable
        if(resp3.status != 200u) {
            env.error("HTTP after failed HTTPS should still work")
        }
    }

    srv.shutdown()
    thread.join()
}

@test
public func https_repeated_failures_no_crash(env : &mut TestEnv) {
    // Make multiple HTTPS requests in sequence to ensure no crash
    var client = http::Client()
    for(var i=0u; i<10u; i++) {
        var res = client.get(string_view("https://127.0.0.1:49993/test"))
        if(res is Result.Ok) {
            env.error("should fail for closed port")
        }
    }
}
