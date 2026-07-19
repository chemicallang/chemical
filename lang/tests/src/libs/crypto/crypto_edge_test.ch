using namespace std;

// ---------------------------------------------------------------------------
// Crypto Library Edge-Case Tests
// ---------------------------------------------------------------------------

@test
func test_sha256_abc(env : &mut TestEnv) {
    var data : [3]u8 = [ 0x61, 0x62, 0x63 ];
    var digest : [32]u8;
    crypto::sha256_hash(&raw data[0], 3, &raw mut digest[0]);
    // SHA-256 of "abc" (FIPS 180-4 test vector)
    if(!(digest[0] == 0xBA && digest[1] == 0x78 && digest[2] == 0x16 && digest[3] == 0xBF &&
         digest[4] == 0x8F && digest[5] == 0x01 && digest[6] == 0xCF && digest[7] == 0xEA &&
         digest[8] == 0x41 && digest[9] == 0x41 && digest[10] == 0x40 && digest[11] == 0xDE &&
         digest[12] == 0x5D && digest[13] == 0xAE && digest[14] == 0x22 && digest[15] == 0x23 &&
         digest[16] == 0xB0 && digest[17] == 0x03 && digest[18] == 0x61 && digest[19] == 0xA3 &&
         digest[20] == 0x96 && digest[21] == 0x17 && digest[22] == 0x7A && digest[23] == 0x9C &&
         digest[24] == 0xB4 && digest[25] == 0x10 && digest[26] == 0xFF && digest[27] == 0x61 &&
         digest[28] == 0xF2 && digest[29] == 0x00 && digest[30] == 0x15 && digest[31] == 0xAD)) {
        env.error("SHA-256 of abc is wrong");
    }
}

@test
func test_sha256_streaming(env : &mut TestEnv) {
    var ctx : crypto::Sha256Context;
    crypto::sha256_init(&raw mut ctx);
    var part1 : [2]u8 = [ 0x48, 0x65 ];
    var part2 : [3]u8 = [ 0x6C, 0x6C, 0x6F ];
    crypto::sha256_update(&raw mut ctx, &raw part1[0], 2);
    crypto::sha256_update(&raw mut ctx, &raw part2[0], 3);
    var digest : [32]u8;
    crypto::sha256_final(&raw mut ctx, &raw mut digest[0]);
    // Same as SHA-256 of "Hello"
    if(!(digest[0] == 0x18 && digest[1] == 0x5F && digest[2] == 0x8D && digest[3] == 0xB3 &&
         digest[4] == 0x22 && digest[5] == 0x71 && digest[6] == 0xFE && digest[7] == 0x25)) {
        env.error("SHA-256 streaming of Hello is wrong");
    }
}

@test
func test_sha256_multi_block(env : &mut TestEnv) {
    // 100 bytes of 'a' - spans multiple 64-byte blocks
    var data : [100]u8;
    var i : size_t = 0;
    while(i < 100) { data[i] = 0x61; i += 1; }
    var digest : [32]u8;
    crypto::sha256_hash(&raw data[0], 100, &raw mut digest[0]);
    // SHA-256 of 100 'a' characters
    if(!(digest[0] == 0x9B && digest[1] == 0xD8 && digest[2] == 0x07 && digest[3] == 0x93)) {
        env.error("SHA-256 of 100 'a' bytes is wrong");
    }
}

@test
func test_md5_abc(env : &mut TestEnv) {
    var data : [3]u8 = [ 0x61, 0x62, 0x63 ];
    var digest : [16]u8;
    crypto::md5_hash(&raw data[0], 3, &raw mut digest[0]);
    // MD5 of "abc" (RFC 1321 test vector)
    if(!(digest[0] == 0x90 && digest[1] == 0x01 && digest[2] == 0x50 && digest[3] == 0x98 &&
         digest[4] == 0x3C && digest[5] == 0xD2 && digest[6] == 0x4F && digest[7] == 0xB0 &&
         digest[8] == 0xD6 && digest[9] == 0x96 && digest[10] == 0x3F && digest[11] == 0x7D &&
         digest[12] == 0x28 && digest[13] == 0xE1 && digest[14] == 0x7F && digest[15] == 0x72)) {
        env.error("MD5 of abc is wrong");
    }
}

@test
func test_md5_streaming(env : &mut TestEnv) {
    var ctx : crypto::Md5Context;
    crypto::md5_init(&raw mut ctx);
    var part1 : [2]u8 = [ 0x48, 0x65 ];
    var part2 : [3]u8 = [ 0x6C, 0x6C, 0x6F ];
    crypto::md5_update(&raw mut ctx, &raw part1[0], 2);
    crypto::md5_update(&raw mut ctx, &raw part2[0], 3);
    var digest : [16]u8;
    crypto::md5_final(&raw mut ctx, &raw mut digest[0]);
    // Same as MD5 of "Hello"
    if(!(digest[0] == 0x8B && digest[1] == 0x1A && digest[2] == 0x99 && digest[3] == 0x53)) {
        env.error("MD5 streaming of Hello is wrong");
    }
}

@test
func test_base64_encode_single_byte(env : &mut TestEnv) {
    var buf : [128]char;
    // Single byte: 0x61 = 'a' -> base64: "YQ=="
    var data : [1]u8 = [ 0x61 ];
    var r = crypto::base64_encode(&raw data[0], 1, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("base64_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == 'Y' && buf[1] == 'Q' && buf[2] == '=' && buf[3] == '=' && buf[4] == '\0')) {
        env.error("base64_encode single byte 'a' should return YQ==");
    }
}

@test
func test_base64_encode_two_bytes(env : &mut TestEnv) {
    var buf : [128]char;
    // Two bytes: 0x61 0x62 = "ab" -> base64: "YWI="
    var data : [2]u8 = [ 0x61, 0x62 ];
    var r = crypto::base64_encode(&raw data[0], 2, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("base64_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == 'Y' && buf[1] == 'W' && buf[2] == 'I' && buf[3] == '=' && buf[4] == '\0')) {
        env.error("base64_encode two bytes 'ab' should return YWI=");
    }
}

@test
func test_base64_decode_invalid(env : &mut TestEnv) {
    var buf : [64]u8;
    // Invalid base64 character '@'
    var r = crypto::base64_decode("SGVs@G8=", 8, &raw mut buf[0], 64);
    if(!(r is Result.Err)) {
        env.error("base64_decode should reject invalid character '@'");
    }
}

@test
func test_base64_decode_single_byte(env : &mut TestEnv) {
    var buf : [64]u8;
    var r = crypto::base64_decode("YQ==", 4, &raw mut buf[0], 64);
    if(r is Result.Err) { env.error("base64_decode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == 0x61)) {
        env.error("base64_decode YQ== should return 0x61");
    }
}

@test
func test_hmac_sha256_rfc4231_case2(env : &mut TestEnv) {
    // RFC 4231 Test Case 2
    // Key = "Jefe", Data = "what do ya want for nothing?"
    var key : [4]u8 = [ 0x4A, 0x65, 0x66, 0x65 ];
    var data : [28]u8 = [ 0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6F, 0x20, 0x79, 0x61,
                         0x20, 0x77, 0x61, 0x6E, 0x74, 0x20, 0x66, 0x6F, 0x72, 0x20,
                         0x6E, 0x6F, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x3F ];
    var digest : [32]u8;
    crypto::hmac_sha256(&raw key[0], 4, &raw data[0], 28, &raw mut digest[0]);
    // Expected: 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
    if(!(digest[0] == 0x5B && digest[1] == 0xDC && digest[2] == 0xC1 && digest[3] == 0x46 &&
         digest[4] == 0xBF && digest[5] == 0x60 && digest[6] == 0x75 && digest[7] == 0x4E &&
         digest[8] == 0x6A && digest[9] == 0x04 && digest[10] == 0x24 && digest[11] == 0x26 &&
         digest[12] == 0x08 && digest[13] == 0x95 && digest[14] == 0x75 && digest[15] == 0xC7 &&
         digest[16] == 0x5A && digest[17] == 0x00 && digest[18] == 0x3F && digest[19] == 0x08 &&
         digest[20] == 0x9D && digest[21] == 0x27 && digest[22] == 0x39 && digest[23] == 0x83 &&
         digest[24] == 0x9D && digest[25] == 0xEC && digest[26] == 0x58 && digest[27] == 0xB9 &&
         digest[28] == 0x64 && digest[29] == 0xEC && digest[30] == 0x38 && digest[31] == 0x43)) {
        env.error("HMAC-SHA256 RFC 4231 case 2 is wrong");
    }
}

@test
func test_constant_time_equal_byte_by_byte(env : &mut TestEnv) {
    // Test that equal buffers with different lengths still work
    var a : [5]u8 = [ 0x01, 0x02, 0x03, 0x04, 0x05 ];
    var b : [5]u8 = [ 0x01, 0x02, 0x03, 0x04, 0x05 ];
    if(!crypto::constant_time_equal(&raw a[0], &raw b[0], 1)) {
        env.error("constant_time_equal should match first byte");
    }
    if(!crypto::constant_time_equal(&raw a[0], &raw b[0], 5)) {
        env.error("constant_time_equal should match all 5 bytes");
    }
    if(!crypto::constant_time_equal(&raw a[0], &raw a[0], 0)) {
        env.error("constant_time_equal with len=0 should be true");
    }
    // Different at last byte only
    var c : [5]u8 = [ 0x01, 0x02, 0x03, 0x04, 0xFF ];
    if(crypto::constant_time_equal(&raw a[0], &raw c[0], 5)) {
        env.error("constant_time_equal should detect diff in last byte");
    }
}

@test
func test_base64_encode_empty(env : &mut TestEnv) {
    var buf : [128]char;
    var r = crypto::base64_encode(&raw buf[0] as *u8, 0, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("base64_encode empty failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 0 && buf[0] == '\0')) {
        env.error("base64_encode of empty should return empty string");
    }
}

@test
func test_sha256_zero_byte(env : &mut TestEnv) {
    var data : [1]u8 = [ 0 ];
    var digest : [32]u8;
    crypto::sha256_hash(&raw data[0], 1, &raw mut digest[0]);
    // SHA-256 of a single zero byte has this hash
    if(!(digest[0] == 0x6E && digest[1] == 0x34 && digest[2] == 0x0B && digest[3] == 0x9C)) {
        env.error("SHA-256 of zero byte is wrong");
    }
}

@test
func test_md5_zero_byte(env : &mut TestEnv) {
    var data : [1]u8 = [ 0 ];
    var digest : [16]u8;
    crypto::md5_hash(&raw data[0], 1, &raw mut digest[0]);
    // MD5 of a single zero byte
    if(!(digest[0] == 0x93 && digest[1] == 0xB8 && digest[2] == 0x85 && digest[3] == 0xAD)) {
        env.error("MD5 of zero byte is wrong");
    }
}

@test
func test_md5_longer_data(env : &mut TestEnv) {
    // MD5 of 1 million 'a' characters (RFC 1321 test 7, but smaller)
    var data : [1000]u8;
    var i : size_t = 0;
    while(i < 1000) { data[i] = 0x61; i += 1; }
    var digest : [16]u8;
    crypto::md5_hash(&raw data[0], 1000, &raw mut digest[0]);
    if(!(digest[0] == 0xAC && digest[1] == 0xAB && digest[2] == 0x82 && digest[3] == 0x63)) {
        env.error("MD5 of 1000 'a' bytes is wrong");
    }
}

@test
func test_base64_roundtrip_all_values(env : &mut TestEnv) {
    // Roundtrip all byte values 0x00 through 0xFF
    var original : [256]u8;
    var i : size_t = 0;
    while(i < 256) { original[i] = i as u8; i += 1; }
    var encoded : [512]char;
    var decoded : [256]u8;
    var r = crypto::base64_encode(&raw original[0], 256, &raw mut encoded[0], 512);
    if(r is Result.Err) { env.error("base64_encode failed"); return; }
    var Ok(enc_len) = r else unreachable;
    var r2 = crypto::base64_decode(encoded, enc_len, &raw mut decoded[0], 256);
    if(r2 is Result.Err) { env.error("base64_decode failed"); return; }
    var Ok(dec_len) = r2 else unreachable;
    if(dec_len != 256) { env.error("length mismatch"); return; }
    var j : size_t = 0;
    while(j < 256) {
        if(decoded[j] != original[j]) { env.error("data mismatch at byte"); return; }
        j += 1;
    }
}

@test
func test_hmac_sha256_empty_key(env : &mut TestEnv) {
    var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
    var digest : [32]u8;
    crypto::hmac_sha256(&raw digest[0] as *u8, 0, &raw data[0], 5, &raw mut digest[0]);
    // HMAC-SHA256 with empty key should produce non-zero output
    if(digest[0] == 0 && digest[1] == 0 && digest[2] == 0 && digest[3] == 0) {
        env.error("HMAC-SHA256 with empty key should be non-zero");
    }
}
