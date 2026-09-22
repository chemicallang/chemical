// ============================================================================
// TLS 1.2 SHA-384 PRF / key-derivation / Finished known-answer tests
// ============================================================================
// The AES_256_GCM_SHA384 family uses the SHA-384 PRF (RFC 5246 §5) instead of
// SHA-256, which is a completely separate code path: PRF, key-block size,
// transform population and the Finished verify_data all select on it. These
// vectors are cross-checked against Python's hmac/hashlib implementation of
// P_SHA384, so they pin the exact bytes rather than "is it non-zero".
//
// Regression: the SHA-384 path used to fail every real handshake even though
// the master secret matched the peer — `crypto::sha384_final` wrote 64 bytes
// into a 48-byte digest buffer and zeroed the caller's neighbouring stack
// slots (here: `sf_hash_len`), so the server Finished was compared against a
// PRF seeded with 0 bytes of transcript hash.
// ============================================================================

using namespace tls

func sha384_bytes_eq(a : *u8, b : *u8, len : size_t) : bool {
    var i : size_t = 0
    while(i < len) { if(a[i] != b[i]) { return false }; i += 1 }
    return true
}

// The 48-byte pre-master secret / random inputs shared by the vectors below:
//   pre_master = 03 03 02 03 04 .. 2F
//   client_random = server_random = 32 zero bytes
//   master_secret (SHA-384 PRF) = PRF384(pre_master, "master secret", cr+sr)
@test
public func tls12_sha384_prf_master_secret_known_answer(env : &mut TestEnv) {
    var pre_master : [48]u8
    pre_master[0] = 0x03
    pre_master[1] = 0x03
    var i : size_t = 2
    while(i < 48) { pre_master[i] = i as u8; i += 1 }

    var client_random : [32]u8
    var server_random : [32]u8
    i = 0
    while(i < 32) { client_random[i] = 0; server_random[i] = 0; i += 1 }

    var seed : [64]u8
    i = 0
    while(i < 32) {
        seed[i] = client_random[i]
        seed[i + 32] = server_random[i]
        i += 1
    }

    var expected : [48]u8 = [
        0x87, 0xA3, 0x97, 0x11, 0xC4, 0xB7, 0x5B, 0xA0,
        0x1B, 0x09, 0xA7, 0xD8, 0x50, 0x2C, 0x92, 0x42,
        0x51, 0x69, 0xE1, 0x36, 0xC4, 0x41, 0x6A, 0x73,
        0xBA, 0x46, 0x28, 0xD3, 0x82, 0x74, 0x2E, 0xDD,
        0x15, 0x9A, 0x9E, 0x29, 0xE8, 0xED, 0xBD, 0xC3,
        0x71, 0x6E, 0x22, 0xD2, 0xA8, 0xD9, 0xDA, 0xBA
    ]

    // label_len counts the 13 label characters only — the trailing NUL of the
    // C string literal is not part of the PRF input (RFC 5246 uses the label
    // as a byte string).
    var label = "master secret\0" as *char
    var actual : [48]u8
    tls12_prf_sha384(&raw pre_master[0], 48, label, 13, &raw seed[0], 64, &raw mut actual[0], 48)
    if(!sha384_bytes_eq(&raw actual[0], &raw expected[0], 48)) {
        env.error("tls12_prf_sha384 master-secret vector mismatch")
    }

    // Deriving through the high-level helper must agree with the raw PRF.
    var derived : [48]u8
    tls12_derive_master_secret(&raw pre_master[0], 48, &raw client_random[0], &raw server_random[0],
                               &raw mut derived[0], true)
    if(!sha384_bytes_eq(&raw derived[0], &raw expected[0], 48)) {
        env.error("tls12_derive_master_secret(use_sha384=true) mismatch")
    }
}

// key_block = PRF384(master_secret, "key expansion", server_random + client_random)
// 72 bytes is exactly what AES-256-GCM needs: (0 + 32 + 4) * 2.
@test
public func tls12_sha384_key_block_known_answer(env : &mut TestEnv) {
    var master_secret : [48]u8 = [
        0x87, 0xA3, 0x97, 0x11, 0xC4, 0xB7, 0x5B, 0xA0,
        0x1B, 0x09, 0xA7, 0xD8, 0x50, 0x2C, 0x92, 0x42,
        0x51, 0x69, 0xE1, 0x36, 0xC4, 0x41, 0x6A, 0x73,
        0xBA, 0x46, 0x28, 0xD3, 0x82, 0x74, 0x2E, 0xDD,
        0x15, 0x9A, 0x9E, 0x29, 0xE8, 0xED, 0xBD, 0xC3,
        0x71, 0x6E, 0x22, 0xD2, 0xA8, 0xD9, 0xDA, 0xBA
    ]
    var server_random : [32]u8
    var client_random : [32]u8
    var i : size_t = 0
    while(i < 32) { server_random[i] = 0; client_random[i] = 0; i += 1 }

    var expected : [72]u8 = [
        0xA5, 0x86, 0x39, 0x38, 0x19, 0x30, 0x0A, 0x8A,
        0xF5, 0x90, 0x1D, 0x6C, 0x24, 0x04, 0x68, 0xED,
        0x43, 0xB2, 0xFC, 0x24, 0x26, 0x97, 0xF8, 0xCE,
        0xED, 0xED, 0xB9, 0x62, 0xA9, 0x01, 0x33, 0x5F,
        0x76, 0x8D, 0x69, 0xEA, 0x7E, 0x80, 0x5C, 0x0C,
        0x2D, 0x47, 0x65, 0xBA, 0x59, 0x02, 0x82, 0x97,
        0x00, 0x31, 0xCE, 0x85, 0x35, 0xED, 0xA4, 0xD9,
        0xCA, 0x49, 0x20, 0xEF, 0xC4, 0xB7, 0x41, 0x96,
        0x94, 0xF0, 0xB1, 0x02, 0x46, 0x36, 0xDD, 0x6B
    ]

    var actual : [72]u8
    tls12_derive_key_block(&raw master_secret[0], &raw server_random[0], &raw client_random[0],
                           &raw mut actual[0], 72, true)
    if(!sha384_bytes_eq(&raw actual[0], &raw expected[0], 72)) {
        env.error("tls12_derive_key_block(use_sha384=true) 72-byte vector mismatch")
    }
}

// Populate an AES-256-GCM-SHA384 transform from the SHA-384 key block and
// check every slice lands where the peer expects it. A wrong key block (or a
// stale SHA-256-derived one) makes the peer reject the client Finished.
@test
public func tls12_sha384_transform_population_works(env : &mut TestEnv) {
    var master_secret : [48]u8 = [
        0x87, 0xA3, 0x97, 0x11, 0xC4, 0xB7, 0x5B, 0xA0,
        0x1B, 0x09, 0xA7, 0xD8, 0x50, 0x2C, 0x92, 0x42,
        0x51, 0x69, 0xE1, 0x36, 0xC4, 0x41, 0x6A, 0x73,
        0xBA, 0x46, 0x28, 0xD3, 0x82, 0x74, 0x2E, 0xDD,
        0x15, 0x9A, 0x9E, 0x29, 0xE8, 0xED, 0xBD, 0xC3,
        0x71, 0x6E, 0x22, 0xD2, 0xA8, 0xD9, 0xDA, 0xBA
    ]
    var server_random : [32]u8
    var client_random : [32]u8
    var i : size_t = 0
    while(i < 32) { server_random[i] = 0; client_random[i] = 0; i += 1 }

    var info = get_ciphersuite_info(TLS_RSA_WITH_AES_256_GCM_SHA384 as u16)
    if(info.hash != HASH_SHA384 as u8) {
        env.error("AES_256_GCM_SHA384 must select the SHA-384 PRF")
        return
    }
    var kb_size = tls12_key_block_size(&raw info)
    if(kb_size != 72) {
        env.error("AES_256_GCM_SHA384 key block size should be 72")
        return
    }

    var key_block : [72]u8
    tls12_derive_key_block(&raw master_secret[0], &raw server_random[0], &raw client_random[0],
                           &raw mut key_block[0], kb_size, true)

    // Layout for GCM: mac_key(0) | client_key(32) | server_key(32) | client_iv(4) | server_iv(4)
    var tr : Transform
    transform_init(unsafe(&raw mut tr))
    tls12_populate_transform(unsafe(&raw mut tr), &raw info, &raw key_block[0], kb_size)

    if(tr.key_len != 32) { env.error("AES-256 key_len should be 32") }
    if(tr.iv_len != 4) { env.error("GCM fixed iv_len should be 4") }
    if(tr.mac_key_len != 0) { env.error("GCM mac_key_len should be 0") }
    if(tr.cipher_type != CIPHER_AES_256_GCM as u8) { env.error("cipher_type should be AES-256-GCM") }

    var k : size_t = 0
    while(k < 32) {
        if(tr.key_enc[k] != key_block[k]) { env.error("client_write_key should be key_block[0..32]"); break }
        k += 1
    }
    k = 0
    while(k < 32) {
        if(tr.key_dec[k] != key_block[32 + k]) { env.error("server_write_key should be key_block[32..64]"); break }
        k += 1
    }
    k = 0
    while(k < 4) {
        if(tr.iv_enc[k] != key_block[64 + k]) { env.error("client_write_IV should be key_block[64..68]"); break }
        k += 1
    }
    k = 0
    while(k < 4) {
        if(tr.iv_dec[k] != key_block[68 + k]) { env.error("server_write_IV should be key_block[68..72]"); break }
        k += 1
    }
}

// verify_data = PRF384(master_secret, "<label> finished", SHA384(transcript))[0..11]
// Transcript hash fixed at 00 01 .. 2F.
@test
public func tls12_sha384_finished_known_answer(env : &mut TestEnv) {
    var master_secret : [48]u8 = [
        0x87, 0xA3, 0x97, 0x11, 0xC4, 0xB7, 0x5B, 0xA0,
        0x1B, 0x09, 0xA7, 0xD8, 0x50, 0x2C, 0x92, 0x42,
        0x51, 0x69, 0xE1, 0x36, 0xC4, 0x41, 0x6A, 0x73,
        0xBA, 0x46, 0x28, 0xD3, 0x82, 0x74, 0x2E, 0xDD,
        0x15, 0x9A, 0x9E, 0x29, 0xE8, 0xED, 0xBD, 0xC3,
        0x71, 0x6E, 0x22, 0xD2, 0xA8, 0xD9, 0xDA, 0xBA
    ]
    var transcript : [48]u8
    var i : size_t = 0
    while(i < 48) { transcript[i] = i as u8; i += 1 }

    var expected_server : [12]u8 = [0x1F, 0xAF, 0x99, 0x2C, 0xB1, 0xDF, 0x77, 0x67, 0x48, 0xE7, 0xE8, 0xCC]
    var expected_client : [12]u8 = [0x4D, 0x8A, 0x93, 0x0C, 0x6F, 0xE2, 0x60, 0x4B, 0xC5, 0x58, 0x2F, 0x43]

    var actual_server : [12]u8
    tls12_compute_finished(&raw master_secret[0], false, &raw transcript[0], 48,
                           &raw mut actual_server[0], true)
    if(!sha384_bytes_eq(&raw actual_server[0], &raw expected_server[0], 12)) {
        env.error("SHA-384 server Finished verify_data mismatch")
    }

    var actual_client : [12]u8
    tls12_compute_finished(&raw master_secret[0], true, &raw transcript[0], 48,
                           &raw mut actual_client[0], true)
    if(!sha384_bytes_eq(&raw actual_client[0], &raw expected_client[0], 12)) {
        env.error("SHA-384 client Finished verify_data mismatch")
    }

    // Client and server Finished must differ (different labels) and the SHA-384
    // vector must not accidentally equal its SHA-256 counterpart.
    if(sha384_bytes_eq(&raw actual_server[0], &raw actual_client[0], 12)) {
        env.error("client and server SHA-384 Finished must differ")
    }

    var actual_sha256 : [12]u8
    tls12_compute_finished(&raw master_secret[0], false, &raw transcript[0], 32,
                           &raw mut actual_sha256[0], false)
    if(sha384_bytes_eq(&raw actual_sha256[0], &raw expected_server[0], 12)) {
        env.error("SHA-256 Finished must not equal the SHA-384 vector")
    }
}
