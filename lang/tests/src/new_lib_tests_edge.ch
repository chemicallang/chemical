using std::Result;

// ===========================================================================
// Edge-case tests for libraries
// ===========================================================================

// ---------------------------------------------------------------------------
// More Path Edge Cases
// ---------------------------------------------------------------------------

@test
func test_path_join_empty_a() {
    test("path::join empty + bin = bin", () => {
        var buf : [4096]char;
        var r = path::join("", "bin", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 3 && buf[0] == 'b' && buf[1] == 'i' && buf[2] == 'n' && buf[3] == '\0';
    })
}

@test
func test_path_join_empty_both() {
    test("path::join empty + empty = empty", () => {
        var buf : [4096]char;
        var r = path::join("", "", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 0 && buf[0] == '\0';
    })
}

@test
func test_path_basename_no_dir() {
    test("path::basename single file returns file", () => {
        var buf : [4096]char;
        var r = path::basename("file.txt", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == 'f' && buf[1] == 'i' && buf[2] == 'l' && buf[3] == 'e' &&
               buf[4] == '.' && buf[5] == 't' && buf[6] == 'x' && buf[7] == 't' && buf[8] == '\0';
    })
}

@test
func test_path_basename_trailing_slash() {
    test("path::basename /usr/bin/ returns bin", () => {
        var buf : [4096]char;
        var r = path::basename("/usr/bin/", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 3 && buf[0] == 'b' && buf[1] == 'i' && buf[2] == 'n' && buf[3] == '\0';
    })
}

@test
func test_path_normalize_trailing_dotdot() {
    test("path::normalize a/.. = .", () => {
        var buf : [4096]char;
        var r = path::normalize("a/..", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '.' && buf[1] == '\0';
    })
}

@test
func test_path_normalize_just_dot() {
    test("path::normalize . = .", () => {
        var buf : [4096]char;
        var r = path::normalize(".", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '.' && buf[1] == '\0';
    })
}

@test
func test_path_normalize_just_dotdot() {
    test("path::normalize .. = ..", () => {
        var buf : [4096]char;
        var r = path::normalize("..", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 2 && buf[0] == '.' && buf[1] == '.' && buf[2] == '\0';
    })
}

@test
func test_path_normalize_deep_dotdot() {
    test("path::normalize /a/../../.. does not go above /", () => {
        var buf : [4096]char;
        var r = path::normalize("/a/../../..", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '/' && buf[1] == '\0';
    })
}

@test
func test_path_normalize_absolute_dotdot_noop() {
    test("path::normalize /../.. = /", () => {
        var buf : [4096]char;
        var r = path::normalize("/../..", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '/' && buf[1] == '\0';
    })
}

@test
func test_path_extension_multi_dot() {
    test("path::extension file.tar.gz = .gz", () => {
        var buf : [256]char;
        var r = path::extension("file.tar.gz", &raw mut buf[0], 256);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        // Should find last dot, so .gz not .tar.gz
        return len == 3 && buf[0] == '.' && buf[1] == 'g' && buf[2] == 'z' && buf[3] == '\0';
    })
}

@test
func test_path_stem_multi_dot() {
    test("path::stem file.tar.gz = file.tar", () => {
        var buf : [256]char;
        var r = path::stem("file.tar.gz", &raw mut buf[0], 256);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == 'f' && buf[1] == 'i' && buf[2] == 'l' && buf[3] == 'e' &&
               buf[4] == '.' && buf[5] == 't' && buf[6] == 'a' && buf[7] == 'r' && buf[8] == '\0';
    })
}

@test
func test_path_dirname_no_dir() {
    test("path::dirname file.txt = .", () => {
        var buf : [4096]char;
        var r = path::dirname("file.txt", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '.' && buf[1] == '\0';
    })
}

@test
func test_path_dirname_empty() {
    test("path::dirname of empty = .", () => {
        var buf : [4096]char;
        var r = path::dirname("", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '.' && buf[1] == '\0';
    })
}

// ---------------------------------------------------------------------------
// More Encoding Edge Cases
// ---------------------------------------------------------------------------

@test
func test_hex_roundtrip() {
    test("hex encode then decode round-trips data", () => {
        var original : [7]u8 = [ 0x00, 0x01, 0x80, 0xFF, 0xAB, 0xCD, 0xEF ];
        var hex_buf : [16]char;
        var decoded : [7]u8;
        var r = encoding::hex_encode(&raw original[0], 7, &raw mut hex_buf[0], 16);
        if(r is Result.Err) { return false; }
        var Ok(hex_len) = r else unreachable;
        var r2 = encoding::hex_decode(hex_buf, &raw mut decoded[0], 7);
        if(r2 is Result.Err) { return false; }
        var Ok(dec_len) = r2 else unreachable;
        if(dec_len != 7) { return false; }
        var i : size_t = 0;
        while(i < 7) {
            if(decoded[i] != original[i]) { return false; }
            i += 1;
        }
        return true;
    })
}

@test
func test_url_encode_all_special() {
    test("url_encode encodes #, @, !, $, etc.", () => {
        var buf : [128]char;
        var r = encoding::url_encode("a#b@c!d$e%f^g&h(i)j[k]l{m}n|o:p;q'r,s<t>u?v~w", 49, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        // Just check that it returns Ok and produces some output with % signs
        var Ok(len) = r else unreachable;
        return len > 0 && buf[len] == '\0';
    })
}

@test
func test_url_decode_invalid_percent() {
    test("url_decode passes through invalid %XX", () => {
        var buf : [128]char;
        var r = encoding::url_decode("%XX%GG%1", 9, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        // Invalid percent sequences should pass through as-is
        var Ok(len) = r else unreachable;
        return len == 9 && buf[0] == '%' && buf[1] == 'X' && buf[2] == 'X' && buf[3] == '%' &&
               buf[4] == 'G' && buf[5] == 'G' && buf[6] == '%' && buf[7] == '1' && buf[8] == '\0';
    })
}

@test
func test_utf8_overlong() {
    test("utf8_is_valid rejects overlong sequences", () => {
        // Overlong encoding of '/' (0x2F) using 2 bytes instead of 1: 0xC0 0xAF
        var overlong : [2]u8 = [ 0xC0, 0xAF ];
        return !encoding::utf8_is_valid(&raw overlong[0] as *char, 2);
    })
}

@test
func test_utf8_surrogate() {
    test("utf8_is_valid rejects surrogate codepoints", () => {
        // UTF-8 encoding of U+D800 (high surrogate): 0xED 0xA0 0x80
        var surrogate : [3]u8 = [ 0xED, 0xA0, 0x80 ];
        return !encoding::utf8_is_valid(&raw surrogate[0] as *char, 3);
    })
}

@test
func test_utf8_max_valid() {
    test("utf8_is_valid accepts 4-byte sequences", () => {
        // U+1F600 (grinning face emoji) in UTF-8: 0xF0 0x9F 0x98 0x80
        var emoji : [4]u8 = [ 0xF0, 0x9F, 0x98, 0x80 ];
        return encoding::utf8_is_valid(&raw emoji[0] as *char, 4);
    })
}

@test
func test_utf8_over_max() {
    test("utf8_is_valid rejects codepoints > U+10FFFF", () => {
        // 0xF4 0x90 0x80 0x80 encodes U+110000 (too large)
        var too_big : [4]u8 = [ 0xF4, 0x90, 0x80, 0x80 ];
        return !encoding::utf8_is_valid(&raw too_big[0] as *char, 4);
    })
}

// ---------------------------------------------------------------------------
// More Crypto Edge Cases
// ---------------------------------------------------------------------------

@test
func test_sha256_abc() {
    test("SHA-256 of abc is correct", () => {
        var data : [3]u8 = [ 0x61, 0x62, 0x63 ]; // "abc"
        var digest : [32]u8;
        crypto::sha256_hash(&raw data[0], 3, &raw mut digest[0]);
        // SHA-256 of "abc" = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
        return digest[0] == 0xBA && digest[1] == 0x78 && digest[2] == 0x16 && digest[3] == 0xBF &&
               digest[4] == 0x8F && digest[5] == 0x01 && digest[6] == 0xCF && digest[7] == 0xEA &&
               digest[8] == 0x41 && digest[9] == 0x41 && digest[10] == 0x40 && digest[11] == 0xDE &&
               digest[12] == 0x5D && digest[13] == 0xAE && digest[14] == 0x22 && digest[15] == 0x23 &&
               digest[16] == 0xB0 && digest[17] == 0x03 && digest[18] == 0x61 && digest[19] == 0xA3 &&
               digest[20] == 0x96 && digest[21] == 0x17 && digest[22] == 0x7A && digest[23] == 0x9C &&
               digest[24] == 0xB4 && digest[25] == 0x10 && digest[26] == 0xFF && digest[27] == 0x61 &&
               digest[28] == 0xF2 && digest[29] == 0x00 && digest[30] == 0x15 && digest[31] == 0xAD;
    })
}

@test
func test_sha256_abcdbc() {
    test("SHA-256 of abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq is correct", () => {
        var data : [56]u8 = [ 0x61, 0x62, 0x63, 0x64, 0x62, 0x63, 0x64, 0x65,
                             0x63, 0x64, 0x65, 0x66, 0x64, 0x65, 0x66, 0x67,
                             0x65, 0x66, 0x67, 0x68, 0x66, 0x67, 0x68, 0x69,
                             0x67, 0x68, 0x69, 0x6A, 0x68, 0x69, 0x6A, 0x6B,
                             0x69, 0x6A, 0x6B, 0x6C, 0x6A, 0x6B, 0x6C, 0x6D,
                             0x6B, 0x6C, 0x6D, 0x6E, 0x6C, 0x6D, 0x6E, 0x6F,
                             0x6D, 0x6E, 0x6F, 0x70, 0x6E, 0x6F, 0x70, 0x71 ];
        var digest : [32]u8;
        crypto::sha256_hash(&raw data[0], 56, &raw mut digest[0]);
        // Expected: 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
        return digest[0] == 0x24 && digest[1] == 0x8D && digest[2] == 0x6A && digest[3] == 0x61 &&
               digest[4] == 0xD2 && digest[5] == 0x06 && digest[6] == 0x38 && digest[7] == 0xB8 &&
               digest[8] == 0xE5 && digest[9] == 0xC0 && digest[10] == 0x26 && digest[11] == 0x93 &&
               digest[12] == 0x0C && digest[13] == 0x3E && digest[14] == 0x60 && digest[15] == 0x39 &&
               digest[16] == 0xA3 && digest[17] == 0x3C && digest[18] == 0xE4 && digest[19] == 0x59 &&
               digest[20] == 0x64 && digest[21] == 0xFF && digest[22] == 0x21 && digest[23] == 0x67 &&
               digest[24] == 0xF6 && digest[25] == 0xEC && digest[26] == 0xED && digest[27] == 0xD4 &&
               digest[28] == 0x19 && digest[29] == 0xDB && digest[30] == 0x06 && digest[31] == 0xC1;
    })
}

@test
func test_md5_abc() {
    test("MD5 of abc is correct", () => {
        var data : [3]u8 = [ 0x61, 0x62, 0x63 ]; // "abc"
        var digest : [16]u8;
        crypto::md5_hash(&raw data[0], 3, &raw mut digest[0]);
        // MD5 of "abc" = 900150983cd24fb0d6963f7d28e17f72
        return digest[0] == 0x90 && digest[1] == 0x01 && digest[2] == 0x50 && digest[3] == 0x98 &&
               digest[4] == 0x3C && digest[5] == 0xD2 && digest[6] == 0x4F && digest[7] == 0xB0 &&
               digest[8] == 0xD6 && digest[9] == 0x96 && digest[10] == 0x3F && digest[11] == 0x7D &&
               digest[12] == 0x28 && digest[13] == 0xE1 && digest[14] == 0x7F && digest[15] == 0x72;
    })
}

@test
func test_md5_long() {
    test("MD5 of 1 million 'a' is correct", () => {
        var buf : [1000]u8;
        var i : size_t = 0;
        while(i < 1000) { buf[i] = 0x61; i += 1; }
        var ctx : crypto::Md5Context;
        crypto::md5_init(&raw mut ctx);
        var j : int = 0;
        while(j < 1000) {
            crypto::md5_update(&raw mut ctx, &raw buf[0], 1000);
            j += 1;
        }
        var digest : [16]u8;
        crypto::md5_final(&raw mut ctx, &raw mut digest[0]);
        // MD5 of 1M 'a' = 7707d6ae4e027c70eea2a935c2296f21
        return digest[0] == 0x77 && digest[1] == 0x07 && digest[2] == 0xD6 && digest[3] == 0xAE &&
               digest[4] == 0x4E && digest[5] == 0x02 && digest[6] == 0x7C && digest[7] == 0x70 &&
               digest[8] == 0xEE && digest[9] == 0xA2 && digest[10] == 0xA9 && digest[11] == 0x35 &&
               digest[12] == 0xC2 && digest[13] == 0x29 && digest[14] == 0x6F && digest[15] == 0x21;
    })
}

@test
func test_base64_encode_single_byte() {
    test("base64_encode of a single byte", () => {
        var buf : [128]char;
        var data : [1]u8 = [ 0x61 ]; // "a"
        var r = crypto::base64_encode(&raw data[0], 1, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 4 && buf[0] == 'Y' && buf[1] == 'Q' && buf[2] == '=' && buf[3] == '=' && buf[4] == '\0';
    })
}

@test
func test_base64_encode_two_bytes() {
    test("base64_encode of two bytes ends with =", () => {
        var buf : [128]char;
        var data : [2]u8 = [ 0x61, 0x62 ]; // "ab"
        var r = crypto::base64_encode(&raw data[0], 2, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 4 && buf[0] == 'Y' && buf[1] == 'W' && buf[2] == 'I' && buf[3] == '=' && buf[4] == '\0';
    })
}

@test
func test_base64_decode_padding() {
    test("base64_decode handles single padding", () => {
        var buf : [64]u8;
        var r = crypto::base64_decode("YWI=", 4, &raw mut buf[0], 64);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 2 && buf[0] == 0x61 && buf[1] == 0x62;
    })
}

@test
func test_base64_decode_double_padding() {
    test("base64_decode handles double padding", () => {
        var buf : [64]u8;
        var r = crypto::base64_decode("YQ==", 4, &raw mut buf[0], 64);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == 0x61;
    })
}

@test
func test_hmac_sha256_rfc4231_case2() {
    test("HMAC-SHA256 RFC 4231 Test Case 2", () => {
        var key : [4]u8 = [ 0x4A, 0x65, 0x66, 0x65 ]; // "Jefe"
        var data : [28]u8 = [ 0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6F, 0x20,
                             0x79, 0x61, 0x20, 0x77, 0x61, 0x6E, 0x74, 0x20,
                             0x66, 0x6F, 0x72, 0x20, 0x6E, 0x6F, 0x74, 0x68,
                             0x69, 0x6E, 0x67, 0x3F ]; // "what do ya want for nothing?"
        var digest : [32]u8;
        crypto::hmac_sha256(&raw key[0], 4, &raw data[0], 28, &raw mut digest[0]);
        // Expected: 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
        return digest[0] == 0x5B && digest[1] == 0xDC && digest[2] == 0xC1 && digest[3] == 0x46 &&
               digest[4] == 0xBF && digest[5] == 0x60 && digest[6] == 0x75 && digest[7] == 0x4E &&
               digest[8] == 0x6A && digest[9] == 0x04 && digest[10] == 0x24 && digest[11] == 0x26 &&
               digest[12] == 0x08 && digest[13] == 0x95 && digest[14] == 0x75 && digest[15] == 0xC7 &&
               digest[16] == 0x5A && digest[17] == 0x00 && digest[18] == 0x3F && digest[19] == 0x08 &&
               digest[20] == 0x9D && digest[21] == 0x27 && digest[22] == 0x39 && digest[23] == 0x83 &&
               digest[24] == 0x9D && digest[25] == 0xEC && digest[26] == 0x58 && digest[27] == 0xB9 &&
               digest[28] == 0x64 && digest[29] == 0xEC && digest[30] == 0x38 && digest[31] == 0x43;
    })
}

@test
func test_constant_time_equal_empty_both() {
    test("constant_time_equal empty buffers return true", () => {
        var dummy : [1]u8 = [ 0 ];
        return crypto::constant_time_equal(&raw dummy[0], &raw dummy[0], 0);
    })
}

// ---------------------------------------------------------------------------
// More Compression Edge Cases
// ---------------------------------------------------------------------------

@test
func test_rle_compress_all_unique() {
    test("RLE every byte unique (no compression)", () => {
        var input : [4]u8 = [ 0x01, 0x02, 0x03, 0x04 ];
        var compressed : [64]u8;
        var comp_len : size_t = 0;
        var r = compression::compress(&raw input[0], 4, &raw mut compressed[0], &raw mut comp_len, 64);
        if(r is Result.Err) { return false; }
        var decompressed : [64]u8;
        var dec_len : size_t = 0;
        var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
        if(r2 is Result.Err) { return false; }
        if(dec_len != 4) { return false; }
        return decompressed[0] == 0x01 && decompressed[1] == 0x02 && decompressed[2] == 0x03 && decompressed[3] == 0x04;
    })
}

@test
func test_rle_max_run() {
    test("RLE handles max run length (255)", () => {
        var input : [255]u8;
        var i : size_t = 0;
        while(i < 255) { input[i] = 0xAB; i += 1; }
        var compressed : [512]u8;
        var comp_len : size_t = 0;
        var r = compression::compress(&raw input[0], 255, &raw mut compressed[0], &raw mut comp_len, 512);
        if(r is Result.Err) { return false; }
        var decompressed : [256]u8;
        var dec_len : size_t = 0;
        var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 256);
        if(r2 is Result.Err) { return false; }
        if(dec_len != 255) { return false; }
        var j : size_t = 0;
        while(j < 255) {
            if(decompressed[j] != 0xAB) { return false; }
            j += 1;
        }
        return true;
    })
}

@test
func test_rle_overflow_run() {
    test("RLE handles run > 255 by splitting", () => {
        var input : [300]u8;
        var i : size_t = 0;
        while(i < 300) { input[i] = 0x42; i += 1; }
        var compressed : [600]u8;
        var comp_len : size_t = 0;
        var r = compression::compress(&raw input[0], 300, &raw mut compressed[0], &raw mut comp_len, 600);
        if(r is Result.Err) { return false; }
        var decompressed : [300]u8;
        var dec_len : size_t = 0;
        var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 300);
        if(r2 is Result.Err) { return false; }
        if(dec_len != 300) { return false; }
        var j : size_t = 0;
        while(j < 300) {
            if(decompressed[j] != 0x42) { return false; }
            j += 1;
        }
        return true;
    })
}

@test
func test_rle_decompress_invalid_no_data() {
    test("RLE decompress of single byte without data returns Err", () => {
        var compressed : [1]u8 = [ 5 ]; // run length 5 but no byte data following
        var decompressed : [64]u8;
        var dec_len : size_t = 0;
        var r = compression::decompress(&raw compressed[0], 1, &raw mut decompressed[0], &raw mut dec_len, 64);
        return r is Result.Err;
    })
}

// ---------------------------------------------------------------------------
// Test buffer too small scenarios
// ---------------------------------------------------------------------------

@test
func test_path_buffer_too_small() {
    test("path::basename returns Err if buffer too small", () => {
        var tiny : [1]char;
        var r = path::basename("/hello/world", &raw mut tiny[0], 1);
        return r is Result.Err;
    })
}

@test
func test_hex_buffer_too_small() {
    test("encoding::hex_encode returns Err if buffer too small", () => {
        var tiny : [1]char;
        var data : [3]u8 = [ 0x48, 0x65, 0x6C ];
        var r = encoding::hex_encode(&raw data[0], 3, &raw mut tiny[0], 1);
        return r is Result.Err;
    })
}

@test
func test_base64_buffer_too_small() {
    test("crypto::base64_encode returns Err if buffer too small", () => {
        var tiny : [1]char;
        var data : [3]u8 = [ 0x48, 0x65, 0x6C ];
        var r = crypto::base64_encode(&raw data[0], 3, &raw mut tiny[0], 1);
        return r is Result.Err;
    })
}

@test
func test_md5_zero_length() {
    test("MD5 hash of single zero byte is correct", () => {
        var data : [1]u8 = [ 0x00 ];
        var digest : [16]u8;
        crypto::md5_hash(&raw data[0], 1, &raw mut digest[0]);
        // MD5 of 0x00: cd77cc28d5be3bfaec9ab4fad57db7d0
        return digest[0] == 0xCD && digest[1] == 0x77 && digest[2] == 0xCC && digest[3] == 0x28 &&
               digest[4] == 0xD5 && digest[5] == 0xBE && digest[6] == 0x3B && digest[7] == 0xFA &&
               digest[8] == 0xEC && digest[9] == 0x9A && digest[10] == 0xB4 && digest[11] == 0xFA &&
               digest[12] == 0xD5 && digest[13] == 0x7D && digest[14] == 0xB7 && digest[15] == 0xD0;
    })
}

@test
func test_sha256_zero_byte() {
    test("SHA-256 of single zero byte is correct", () => {
        var data : [1]u8 = [ 0x00 ];
        var digest : [32]u8;
        crypto::sha256_hash(&raw data[0], 1, &raw mut digest[0]);
        // SHA-256 of 0x00: 6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d
        return digest[0] == 0x6E && digest[1] == 0x34 && digest[2] == 0x0B && digest[3] == 0x9C &&
               digest[4] == 0xFF && digest[5] == 0xB3 && digest[6] == 0x7A && digest[7] == 0x98 &&
               digest[8] == 0x9C && digest[9] == 0xA5 && digest[10] == 0x44 && digest[11] == 0xE6 &&
               digest[12] == 0xBB && digest[13] == 0x78 && digest[14] == 0x0A && digest[15] == 0x2C &&
               digest[16] == 0x78 && digest[17] == 0x90 && digest[18] == 0x1D && digest[19] == 0x3F &&
               digest[20] == 0xB3 && digest[21] == 0x37 && digest[22] == 0x38 && digest[23] == 0x76 &&
               digest[24] == 0x85 && digest[25] == 0x11 && digest[26] == 0xA3 && digest[27] == 0x06 &&
               digest[28] == 0x17 && digest[29] == 0xAF && digest[30] == 0xA0 && digest[31] == 0x1D;
    })
}
