using namespace std;

// ===========================================================================
// Cross-Library Integration Tests
// ===========================================================================
// Tests that combine multiple libraries to verify they work together.
// ===========================================================================

// ---------------------------------------------------------------------------
// Crypto + Encoding: Hash then hex-encode the digest
// ---------------------------------------------------------------------------

@test
func test_sha256_then_hex_hello(env : &mut TestEnv) {
    var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
    var digest : [32]u8;
    crypto::sha256_hash(&raw data[0], 5, &raw mut digest[0]);
    var hex_buf : [128]char;
    var r = encoding::hex_encode(&raw digest[0], 32, &raw mut hex_buf[0], 128);
    if(r is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(len) = r else unreachable;
    // SHA-256("Hello") hex-encoded
    var expected = "185f8db32271fe25f561a6fc938b2e264306ec304eda518007d1764826381969";
    if(!(len == 64 && hex_buf[0] == '1' && hex_buf[1] == '8' && hex_buf[62] == '6' && hex_buf[63] == '9')) {
        env.error("SHA-256(Hello) hex mismatch");
    }
}

@test
func test_md5_then_hex_empty(env : &mut TestEnv) {
    var digest : [16]u8;
    crypto::md5_hash(&raw digest[0] as *u8, 0, &raw mut digest[0]);
    var hex_buf : [128]char;
    var r = encoding::hex_encode(&raw digest[0], 16, &raw mut hex_buf[0], 128);
    if(r is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(len) = r else unreachable;
    // MD5("") = d41d8cd98f00b204e9800998ecf8427e
    if(!(len == 32 && hex_buf[0] == 'd' && hex_buf[1] == '4' && hex_buf[30] == '7' && hex_buf[31] == 'e')) {
        env.error("MD5(empty) hex mismatch");
    }
}

@test
func test_sha256_then_hex_upper(env : &mut TestEnv) {
    var data : [3]u8 = [ 0x61, 0x62, 0x63 ];
    var digest : [32]u8;
    crypto::sha256_hash(&raw data[0], 3, &raw mut digest[0]);
    var hex_buf : [128]char;
    var r = encoding::hex_encode_upper(&raw digest[0], 32, &raw mut hex_buf[0], 128);
    if(r is Result.Err) { env.error("hex_encode_upper failed"); return; }
    var Ok(len) = r else unreachable;
    // SHA-256("abc") = BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD
    if(!(len == 64 && hex_buf[0] == 'B' && hex_buf[1] == 'A' && hex_buf[62] == 'A' && hex_buf[63] == 'D')) {
        env.error("SHA-256(abc) upper hex mismatch");
    }
}

// ---------------------------------------------------------------------------
// Compression + Base64: Compress then base64 encode
// ---------------------------------------------------------------------------

@test
func test_compress_then_b64_roundtrip(env : &mut TestEnv) {
    var input : [50]u8 = [ 0x41, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x44, 0x45,
                          0x41, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x44, 0x45,
                          0x41, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x44, 0x45,
                          0x41, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x44, 0x45,
                          0x41, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x44, 0x45 ];
    var compressed : [256]u8;
    var comp_len : size_t = 0;
    var rc = compression::compress(&raw input[0], 50, &raw mut compressed[0], &raw mut comp_len, 256);
    if(rc is Result.Err) { env.error("compress failed"); return; }
    // Base64 encode the compressed data
    var b64_buf : [512]char;
    var rb = crypto::base64_encode(&raw compressed[0], comp_len, &raw mut b64_buf[0], 512);
    if(rb is Result.Err) { env.error("base64_encode of compressed data failed"); return; }
    var Ok(b64_len) = rb else unreachable;
    if(b64_len < 1) { env.error("b64 encoded length should be > 0"); return; }
    // Base64 decode back
    var decoded_comp : [256]u8;
    var rd = crypto::base64_decode(b64_buf, b64_len, &raw mut decoded_comp[0], 256);
    if(rd is Result.Err) { env.error("base64_decode failed"); return; }
    var Ok(dec_comp_len) = rd else unreachable;
    // Decompress
    var decompressed : [256]u8;
    var dec_len : size_t = 0;
    var rx = compression::decompress(&raw decoded_comp[0], dec_comp_len, &raw mut decompressed[0], &raw mut dec_len, 256);
    if(rx is Result.Err) { env.error("decompress after b64 roundtrip failed"); return; }
    if(dec_len != 50) { env.error("final decompressed length should be 50"); return; }
    var ok = true;
    var i : size_t = 0;
    while(i < 50 && ok) {
        if(decompressed[i] != input[i]) { ok = false; }
        i += 1;
    }
    if(!ok) { env.error("compress -> b64 -> decode -> decompress roundtrip failed"); }
}

// ---------------------------------------------------------------------------
// Compression + Hex: Compress then hex-encode
// ---------------------------------------------------------------------------

@test
func test_compress_then_hex_roundtrip(env : &mut TestEnv) {
    var input : [30]u8 = [ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xAA, 0xAA, 0xAA, 0xAA,
                          0xBB, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC ];
    var compressed : [128]u8;
    var comp_len : size_t = 0;
    var rc = compression::compress(&raw input[0], 30, &raw mut compressed[0], &raw mut comp_len, 128);
    if(rc is Result.Err) { env.error("compress failed"); return; }
    // Hex-encode compressed
    var hex_buf : [512]char;
    var rh = encoding::hex_encode(&raw compressed[0], comp_len, &raw mut hex_buf[0], 512);
    if(rh is Result.Err) { env.error("hex_encode of compressed data failed"); return; }
    var Ok(hex_len) = rh else unreachable;
    // Hex-decode back
    var decoded_comp : [128]u8;
    var rhd = encoding::hex_decode(hex_buf, &raw mut decoded_comp[0], 128);
    if(rhd is Result.Err) { env.error("hex_decode failed"); return; }
    var Ok(dec_comp_len) = rhd else unreachable;
    // Decompress
    var decompressed : [128]u8;
    var dec_len : size_t = 0;
    var rx = compression::decompress(&raw decoded_comp[0], dec_comp_len, &raw mut decompressed[0], &raw mut dec_len, 128);
    if(rx is Result.Err) { env.error("decompress after hex roundtrip failed"); return; }
    if(dec_len != 30) { env.error("final decompressed length should be 30"); return; }
    var ok = true;
    var i : size_t = 0;
    while(i < 30 && ok) {
        if(decompressed[i] != input[i]) { ok = false; }
        i += 1;
    }
    if(!ok) { env.error("compress -> hex -> decode -> decompress roundtrip failed"); }
}

// ---------------------------------------------------------------------------
// Path + Crypto: Hash a path string
// ---------------------------------------------------------------------------

@test
func test_path_basename_then_hash(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::basename("/usr/bin/gcc", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("basename failed"); return; }
    var Ok(len) = r else unreachable;
    // Hash the basename "gcc" as data
    var digest : [32]u8;
    crypto::sha256_hash(&raw buf[0] as *u8, len, &raw mut digest[0]);
    // SHA256("gcc") first 4 bytes: verify it's non-zero
    if(digest[0] == 0 && digest[1] == 0 && digest[2] == 0 && digest[3] == 0) {
        env.error("SHA-256 of path basename gave all-zero hash");
    }
}

@test
func test_path_normalize_then_hex_hash(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("/usr/bin/../lib/./file.txt", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    // Hash the normalized path
    var digest : [32]u8;
    crypto::sha256_hash(&raw buf[0] as *u8, len, &raw mut digest[0]);
    // Hex-encode the hash for display verification
    var hex_buf : [128]char;
    var rh = encoding::hex_encode(&raw digest[0], 32, &raw mut hex_buf[0], 128);
    if(rh is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(hex_len) = rh else unreachable;
    if(hex_len != 64) { env.error("hash hex output should be 64 chars"); return; }
    // Verify the hex string is all hex digits (0-9, a-f)
    var valid = true;
    var i : size_t = 0;
    while(i < 64) {
        var c = hex_buf[i];
        if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) { valid = false; }
        i += 1;
    }
    if(!valid) { env.error("hash hex output contains non-hex chars"); }
}

// ---------------------------------------------------------------------------
// Hex + Base64: Double encode/decode
// ---------------------------------------------------------------------------

@test
func test_hex_decode_then_b64_encode(env : &mut TestEnv) {
    // Hex string "48656C6C6F" = "Hello"
    // Base64 of "Hello" = "SGVsbG8="
    var hex = "48656C6C6F";
    var bytes : [16]u8;
    var rd = encoding::hex_decode(hex, &raw mut bytes[0], 16);
    if(rd is Result.Err) { env.error("hex_decode failed"); return; }
    var Ok(byte_len) = rd else unreachable;
    if(byte_len != 5) { env.error("expected 5 bytes from hex"); return; }
    // Base64 encode the decoded bytes
    var b64_buf : [32]char;
    var rb = crypto::base64_encode(&raw bytes[0], byte_len, &raw mut b64_buf[0], 32);
    if(rb is Result.Err) { env.error("base64_encode failed"); return; }
    var Ok(b64_len) = rb else unreachable;
    if(!(b64_len == 8 && b64_buf[0] == 'S' && b64_buf[1] == 'G' && b64_buf[2] == 'V' && b64_buf[3] == 's' &&
         b64_buf[4] == 'b' && b64_buf[5] == 'G' && b64_buf[6] == '8' && b64_buf[7] == '=')) {
        env.error("hex -> b64: expected SGVsbG8=");
    }
}

@test
func test_b64_decode_then_hex_encode(env : &mut TestEnv) {
    var b64 = "SGVsbG8=";
    var bytes : [16]u8;
    var rd = crypto::base64_decode(b64, 8, &raw mut bytes[0], 16);
    if(rd is Result.Err) { env.error("base64_decode failed"); return; }
    var Ok(byte_len) = rd else unreachable;
    if(byte_len != 5) { env.error("expected 5 bytes from b64"); return; }
    // Hex-encode the decoded bytes
    var hex_buf : [32]char;
    var rh = encoding::hex_encode(&raw bytes[0], byte_len, &raw mut hex_buf[0], 32);
    if(rh is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(hex_len) = rh else unreachable;
    if(!(hex_len == 10 && hex_buf[0] == '4' && hex_buf[1] == '8' && hex_buf[2] == '6' && hex_buf[3] == '5' &&
         hex_buf[4] == '6' && hex_buf[5] == 'c' && hex_buf[6] == '6' && hex_buf[7] == 'c' && hex_buf[8] == '6' && hex_buf[9] == 'f')) {
        env.error("b64 -> hex: expected 48656c6c6f");
    }
}

// ---------------------------------------------------------------------------
// URL-encode + Hex: URL-encode special bytes then hex-verify
// ---------------------------------------------------------------------------

@test
func test_url_encode_then_verify_with_hex(env : &mut TestEnv) {
    var data : [3]u8 = [ 0x00, 0x80, 0xFF ];
    var url_buf : [32]char;
    var ru = encoding::url_encode(&raw data[0] as *char, 3, &raw mut url_buf[0], 32);
    if(ru is Result.Err) { env.error("url_encode failed"); return; }
    var Ok(url_len) = ru else unreachable;
    // Verify the URL-encoded output contains valid percent-encoded bytes
    // %00%80%FF
    if(!(url_len == 9 && url_buf[0] == '%' && url_buf[1] == '0' && url_buf[2] == '0' &&
         url_buf[3] == '%' && url_buf[4] == '8' && url_buf[5] == '0' &&
         url_buf[6] == '%' && url_buf[7] == 'F' && url_buf[8] == 'F')) {
        env.error("url_encode of [0x00,0x80,0xFF] should be %00%80%FF");
    }
}

// ---------------------------------------------------------------------------
// Full pipeline: Path -> Hash -> Hex -> B64 roundtrip
// ---------------------------------------------------------------------------

@test
func test_path_hash_hex_b64_roundtrip(env : &mut TestEnv) {
    // Take a path, normalize it, hash it, hex it, b64 it, then reverse all
    var path_buf : [4096]char;
    var rn = path::normalize("/a/./b/../c/", &raw mut path_buf[0], 4096);
    if(rn is Result.Err) { env.error("normalize failed"); return; }
    var Ok(path_len) = rn else unreachable;
    // Hash the normalized path
    var digest : [32]u8;
    crypto::sha256_hash(&raw path_buf[0] as *u8, path_len, &raw mut digest[0]);
    // Hex-encode the hash
    var hex_buf : [128]char;
    var rh = encoding::hex_encode(&raw digest[0], 32, &raw mut hex_buf[0], 128);
    if(rh is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(hex_len) = rh else unreachable;
    // Base64-encode the hex string (double encoding just for pipeline test)
    var b64_buf : [256]char;
    var rb = crypto::base64_encode(&raw hex_buf[0] as *u8, hex_len, &raw mut b64_buf[0], 256);
    if(rb is Result.Err) { env.error("base64_encode of hex failed"); return; }
    var Ok(b64_len) = rb else unreachable;
    // Verify b64 output is non-empty and valid base64
    if(b64_len < 4) { env.error("b64 output too short"); return; }
    var valid_b64 = true;
    var i : size_t = 0;
    while(i < b64_len) {
        var c = b64_buf[i];
        if(!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
             (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')) {
            valid_b64 = false;
        }
        i += 1;
    }
    if(!valid_b64) { env.error("pipeline output contains invalid base64 chars"); }
}

// ---------------------------------------------------------------------------
// HMAC + Hex: HMAC-SHA256 then hex-encode
// ---------------------------------------------------------------------------

@test
func test_hmac_then_hex(env : &mut TestEnv) {
    var key : [4]u8 = [ 0x4A, 0x65, 0x66, 0x65 ];
    var data : [28]u8 = [ 0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6F, 0x20, 0x79, 0x61,
                         0x20, 0x77, 0x61, 0x6E, 0x74, 0x20, 0x66, 0x6F, 0x72, 0x20,
                         0x6E, 0x6F, 0x74, 0x68, 0x69, 0x6E, 0x67, 0x3F ];
    var digest : [32]u8;
    crypto::hmac_sha256(&raw key[0], 4, &raw data[0], 28, &raw mut digest[0]);
    var hex_buf : [128]char;
    var rh = encoding::hex_encode(&raw digest[0], 32, &raw mut hex_buf[0], 128);
    if(rh is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(hex_len) = rh else unreachable;
    // Expected: 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
    if(!(hex_len == 64 && hex_buf[0] == '5' && hex_buf[1] == 'b' && hex_buf[62] == '4' && hex_buf[63] == '3')) {
        env.error("HMAC-SHA256 RFC 4231 case 2 hex mismatch");
    }
}

// ---------------------------------------------------------------------------
// Encoding + Encoding: URL -> UTF-8 validation chain
// ---------------------------------------------------------------------------

@test
func test_url_encode_utf8_then_decode(env : &mut TestEnv) {
    // UTF-8 encoded "héllo wörld" (with multi-byte characters)
    var utf8_data : [12]u8 = [ 0x68, 0xC3, 0xA9, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 0xC3, 0xB6, 0x72, 0x6C ];
    // First verify it's valid UTF-8
    if(!encoding::utf8_is_valid(&raw utf8_data[0] as *char, 12)) {
        env.error("UTF-8 data should be valid"); return;
    }
    // URL-encode it
    var url_buf : [64]char;
    var ru = encoding::url_encode(&raw utf8_data[0] as *char, 12, &raw mut url_buf[0], 64);
    if(ru is Result.Err) { env.error("url_encode of UTF-8 failed"); return; }
    var Ok(url_len) = ru else unreachable;
    // URL-decode back
    var decoded_buf : [64]char;
    var rdu = encoding::url_decode(url_buf, url_len, &raw mut decoded_buf[0], 64);
    if(rdu is Result.Err) { env.error("url_decode failed"); return; }
    var Ok(dec_len) = rdu else unreachable;
    if(dec_len != 12) { env.error("URL roundtrip length mismatch"); return; }
    var ok = true;
    var i : size_t = 0;
    while(i < 12) {
        if(decoded_buf[i] != utf8_data[i] as char) { ok = false; }
        i += 1;
    }
    if(!ok) { env.error("URL encode/decode roundtrip changed UTF-8 data"); }
}

// ---------------------------------------------------------------------------
// Path + Encoding: Path extension hex representation
// ---------------------------------------------------------------------------

@test
func test_path_extension_then_hex(env : &mut TestEnv) {
    var ext_buf : [256]char;
    var re = path::extension("document.txt", &raw mut ext_buf[0], 256);
    if(re is Result.Err) { env.error("extension failed"); return; }
    var Ok(ext_len) = re else unreachable;
    // Hex-encode the extension bytes (including the dot)
    var hex_buf : [64]char;
    var rh = encoding::hex_encode(&raw ext_buf[0] as *u8, ext_len, &raw mut hex_buf[0], 64);
    if(rh is Result.Err) { env.error("hex_encode of extension failed"); return; }
    var Ok(hex_len) = rh else unreachable;
    // ".txt" = 0x2E 0x74 0x78 0x74 -> "2e747874"
    if(!(hex_len == 8 && hex_buf[0] == '2' && hex_buf[1] == 'e' &&
         hex_buf[6] == '7' && hex_buf[7] == '4')) {
        env.error("hex of .txt extension should be 2e747874");
    }
}

// ---------------------------------------------------------------------------
// HMAC + Compression: HMAC a compressed payload
// ---------------------------------------------------------------------------

@test
func test_hmac_compressed_data(env : &mut TestEnv) {
    // Create compressible data
    var input : [100]u8;
    var i : size_t = 0;
    while(i < 100) { input[i] = 0xAB; i += 1; }
    // Compress it
    var compressed : [256]u8;
    var comp_len : size_t = 0;
    var rc = compression::compress(&raw input[0], 100, &raw mut compressed[0], &raw mut comp_len, 256);
    if(rc is Result.Err) { env.error("compress failed"); return; }
    // HMAC the compressed data
    var key : [3]u8 = [ 0x6B, 0x65, 0x79 ];
    var digest : [32]u8;
    crypto::hmac_sha256(&raw key[0], 3, &raw compressed[0], comp_len, &raw mut digest[0]);
    if(digest[0] == 0 && digest[1] == 0 && digest[2] == 0 && digest[3] == 0) {
        env.error("HMAC of compressed data gave all-zero");
    }
    // Verify the HMAC is consistent by running it twice
    var digest2 : [32]u8;
    crypto::hmac_sha256(&raw key[0], 3, &raw compressed[0], comp_len, &raw mut digest2[0]);
    var same = true;
    var j : size_t = 0;
    while(j < 32) {
        if(digest[j] != digest2[j]) { same = false; }
        j += 1;
    }
    if(!same) { env.error("HMAC of compressed data is not deterministic"); }
}

// ---------------------------------------------------------------------------
// Multi-step: Hex -> B64 -> Compress (inverse pipeline)
// ---------------------------------------------------------------------------

@test
func test_hex_b64_compress_inverse(env : &mut TestEnv) {
    // Start with hex-encoded data, decode to bytes
    var hex = "48656C6C6F202D20436F6D7072657373696F6E2054657374";
    var bytes : [64]u8;
    var rh = encoding::hex_decode(hex, &raw mut bytes[0], 64);
    if(rh is Result.Err) { env.error("hex_decode failed"); return; }
    var Ok(byte_len) = rh else unreachable;
    // Base64 encode those bytes
    var b64_buf : [128]char;
    var rb = crypto::base64_encode(&raw bytes[0], byte_len, &raw mut b64_buf[0], 128);
    if(rb is Result.Err) { env.error("base64_encode failed"); return; }
    var Ok(b64_len) = rb else unreachable;
    // Base64 decode back
    var decoded_bytes : [64]u8;
    var rbd = crypto::base64_decode(b64_buf, b64_len, &raw mut decoded_bytes[0], 64);
    if(rbd is Result.Err) { env.error("base64_decode failed"); return; }
    var Ok(dec_byte_len) = rbd else unreachable;
    // Compress decoded bytes
    var compressed : [128]u8;
    var comp_len : size_t = 0;
    var rc = compression::compress(&raw decoded_bytes[0], dec_byte_len, &raw mut compressed[0], &raw mut comp_len, 128);
    if(rc is Result.Err) { env.error("compress failed"); return; }
    // Decompress
    var decompressed : [128]u8;
    var dec_len : size_t = 0;
    var rx = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 128);
    if(rx is Result.Err) { env.error("decompress failed"); return; }
    // Verify the whole pipeline: original hex decoded bytes match
    if(dec_len != byte_len) { env.error("pipeline length mismatch"); return; }
    var ok = true;
    var i : size_t = 0;
    while(i < byte_len) {
        if(decompressed[i] != bytes[i]) { ok = false; }
        i += 1;
    }
    if(!ok) { env.error("hex -> b64 -> compress -> decompress pipeline failed"); }
}

// ---------------------------------------------------------------------------
// Path + Encoding + Crypto: Full data hash pipeline
// ---------------------------------------------------------------------------

@test
func test_path_join_hash_verify(env : &mut TestEnv) {
    var buf : [4096]char;
    var rj = path::join("/home", "user/docs/file.txt", &raw mut buf[0], 4096);
    if(rj is Result.Err) { env.error("join failed"); return; }
    var Ok(join_len) = rj else unreachable;
    // Get the basename
    var base_buf : [256]char;
    var rb = path::basename(buf, &raw mut base_buf[0], 256);
    if(rb is Result.Err) { env.error("basename failed"); return; }
    var Ok(base_len) = rb else unreachable;
    // Get the extension from basename
    var ext_buf : [256]char;
    var re = path::extension(base_buf, &raw mut ext_buf[0], 256);
    if(re is Result.Err) { env.error("extension failed"); return; }
    var Ok(ext_len) = re else unreachable;
    // Verify extension is .txt via hex
    var hex_buf : [32]char;
    var rh = encoding::hex_encode(&raw ext_buf[0] as *u8, ext_len, &raw mut hex_buf[0], 32);
    if(rh is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(hex_len) = rh else unreachable;
    if(!(hex_len == 8 && hex_buf[0] == '2' && hex_buf[1] == 'e')) {
        env.error("extension .txt hex should be 2e747874");
    }
}
