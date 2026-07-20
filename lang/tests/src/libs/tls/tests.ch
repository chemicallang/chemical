using std::Result;
using std::vector;
using std::string;

@test
public func tls_ciphersuite_lookup_works(env : &mut TestEnv) {
    // Test cipher suite info lookup for known suites
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
