using namespace std;

// ---------------------------------------------------------------------------

@test
func test_sha256_hello(env : &mut TestEnv) {
    var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
    var digest : [32]u8;
    crypto::sha256_hash(&raw data[0], 5, &raw mut digest[0]);
    if(!(digest[0] == 0x18 && digest[1] == 0x5F && digest[2] == 0x8D && digest[3] == 0xB3 &&
         digest[4] == 0x22 && digest[5] == 0x71 && digest[6] == 0xFE && digest[7] == 0x25 &&
         digest[8] == 0xF5 && digest[9] == 0x61 && digest[10] == 0xA6 && digest[11] == 0xFC &&
         digest[12] == 0x93 && digest[13] == 0x8B && digest[14] == 0x2E && digest[15] == 0x26 &&
         digest[16] == 0x43 && digest[17] == 0x06 && digest[18] == 0xEC && digest[19] == 0x30 &&
         digest[20] == 0x4E && digest[21] == 0xDA && digest[22] == 0x51 && digest[23] == 0x80 &&
         digest[24] == 0x07 && digest[25] == 0xD1 && digest[26] == 0x76 && digest[27] == 0x48 &&
         digest[28] == 0x26 && digest[29] == 0x38 && digest[30] == 0x19 && digest[31] == 0x69)) {
        env.error("SHA-256 of Hello is wrong");
    }
}

@test
func test_sha256_empty(env : &mut TestEnv) {
    var digest : [32]u8;
    crypto::sha256_hash(&raw digest[0] as *u8, 0, &raw mut digest[0]);
    if(!(digest[0] == 0xE3 && digest[1] == 0xB0 && digest[2] == 0xC4 && digest[3] == 0x42 &&
         digest[4] == 0x98 && digest[5] == 0xFC && digest[6] == 0x1C && digest[7] == 0x14 &&
         digest[8] == 0x9A && digest[9] == 0xFB && digest[10] == 0xF4 && digest[11] == 0xC8 &&
         digest[12] == 0x99 && digest[13] == 0x6F && digest[14] == 0xB9 && digest[15] == 0x24 &&
         digest[16] == 0x27 && digest[17] == 0xAE && digest[18] == 0x41 && digest[19] == 0xE4 &&
         digest[20] == 0x64 && digest[21] == 0x9B && digest[22] == 0x93 && digest[23] == 0x4C &&
         digest[24] == 0xA4 && digest[25] == 0x95 && digest[26] == 0x99 && digest[27] == 0x1B &&
         digest[28] == 0x78 && digest[29] == 0x52 && digest[30] == 0xB8 && digest[31] == 0x55)) {
        env.error("SHA-256 of empty is wrong");
    }
}

@test
func test_md5_hello(env : &mut TestEnv) {
    var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
    var digest : [16]u8;
    crypto::md5_hash(&raw data[0], 5, &raw mut digest[0]);
    if(!(digest[0] == 0x8B && digest[1] == 0x1A && digest[2] == 0x99 && digest[3] == 0x53 &&
         digest[4] == 0xC4 && digest[5] == 0x61 && digest[6] == 0x12 && digest[7] == 0x96 &&
         digest[8] == 0xA8 && digest[9] == 0x27 && digest[10] == 0xAB && digest[11] == 0xF8 &&
         digest[12] == 0xC4 && digest[13] == 0x78 && digest[14] == 0x04 && digest[15] == 0xD7)) {
        env.error("MD5 of Hello is wrong");
    }
}

@test
func test_md5_empty(env : &mut TestEnv) {
    var digest : [16]u8;
    crypto::md5_hash(&raw digest[0] as *u8, 0, &raw mut digest[0]);
    if(!(digest[0] == 0xD4 && digest[1] == 0x1D && digest[2] == 0x8C && digest[3] == 0xD9 &&
         digest[4] == 0x8F && digest[5] == 0x00 && digest[6] == 0xB2 && digest[7] == 0x04 &&
         digest[8] == 0xE9 && digest[9] == 0x80 && digest[10] == 0x09 && digest[11] == 0x98 &&
         digest[12] == 0xEC && digest[13] == 0xF8 && digest[14] == 0x42 && digest[15] == 0x7E)) {
        env.error("MD5 of empty is wrong");
    }
}

@test
func test_base64_encode(env : &mut TestEnv) {
    var buf : [128]char;
    var data : [3]u8 = [ 0x48, 0x65, 0x6C ];
    var r = crypto::base64_encode(&raw data[0], 3, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("base64_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == 'S' && buf[1] == 'G' && buf[2] == 'V' && buf[3] == 's' && buf[4] == '\0')) {
        env.error("base64_encode of Hel should return SGV s");
    }
}

@test
func test_base64_encode_padded(env : &mut TestEnv) {
    var buf : [128]char;
    var data : [4]u8 = [ 0x48, 0x65, 0x6C, 0x6C ];
    var r = crypto::base64_encode(&raw data[0], 4, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("base64_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == 'S' && buf[1] == 'G' && buf[2] == 'V' && buf[3] == 's' &&
         buf[4] == 'b' && buf[5] == 'A' && buf[6] == '=' && buf[7] == '=' && buf[8] == '\0')) {
        env.error("base64_encode of Hell should return SGVsbA==");
    }
}

@test
func test_base64_decode(env : &mut TestEnv) {
    var buf : [64]u8;
    var r = crypto::base64_decode("SGVsbG8=", 8, &raw mut buf[0], 64);
    if(r is Result.Err) { env.error("base64_decode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 5 && buf[0] == 0x48 && buf[1] == 0x65 && buf[2] == 0x6C && buf[3] == 0x6C && buf[4] == 0x6F)) {
        env.error("base64_decode of Hello b64 should return Hello");
    }
}

@test
func test_base64_roundtrip(env : &mut TestEnv) {
    var original : [6]u8 = [ 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF ];
    var encoded : [16]char;
    var decoded : [6]u8;
    var r = crypto::base64_encode(&raw original[0], 6, &raw mut encoded[0], 16);
    if(r is Result.Err) { env.error("base64_encode failed"); return; }
    var Ok(enc_len) = r else unreachable;
    var r2 = crypto::base64_decode(encoded, enc_len, &raw mut decoded[0], 6);
    if(r2 is Result.Err) { env.error("base64_decode failed"); return; }
    var Ok(dec_len) = r2 else unreachable;
    if(!(dec_len == 6 && decoded[0] == 0xDE && decoded[1] == 0xAD && decoded[2] == 0xBE &&
         decoded[3] == 0xEF && decoded[4] == 0x00 && decoded[5] == 0xFF)) {
        env.error("base64 roundtrip should preserve data");
    }
}

@test
func test_hmac_sha256_not_zero(env : &mut TestEnv) {
    var key : [3]u8 = [ 0x6B, 0x65, 0x79 ];
    var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
    var digest : [32]u8;
    crypto::hmac_sha256(&raw key[0], 3, &raw data[0], 5, &raw mut digest[0]);
    if(digest[0] == 0 && digest[1] == 0 && digest[2] == 0 && digest[3] == 0) {
        env.error("HMAC-SHA256 should produce non-zero output");
    }
}

@test
func test_constant_time_equal_same(env : &mut TestEnv) {
    var a : [3]u8 = [ 0x01, 0x02, 0x03 ];
    var b : [3]u8 = [ 0x01, 0x02, 0x03 ];
    if(!crypto::constant_time_equal(&raw a[0], &raw b[0], 3)) {
        env.error("constant_time_equal should return true for equal buffers");
    }
}

@test
func test_constant_time_equal_different(env : &mut TestEnv) {
    var a : [3]u8 = [ 0x01, 0x02, 0x03 ];
    var b : [3]u8 = [ 0x01, 0xFF, 0x03 ];
    if(crypto::constant_time_equal(&raw a[0], &raw b[0], 3)) {
        env.error("constant_time_equal should return false for different buffers");
    }
}

@test
func test_constant_time_equal_empty(env : &mut TestEnv) {
    var dummy : [1]u8 = [ 0 ];
    if(!crypto::constant_time_equal(&raw dummy[0], &raw dummy[0], 0)) {
        env.error("constant_time_equal should return true for empty buffers");
    }
}

// ---------------------------------------------------------------------------
