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
