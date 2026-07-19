using std::Result;

// ---------------------------------------------------------------------------
// Path Library Tests
// ---------------------------------------------------------------------------

@test
func test_path_basename_simple() {
    test("path::basename /usr/bin/gcc returns gcc", () => {
        var buf : [4096]char;
        var r = path::basename("/usr/bin/gcc", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 3 && buf[0] == 'g' && buf[1] == 'c' && buf[2] == 'c' && buf[3] == '\0';
    })
}

@test
func test_path_basename_root() {
    test("path::basename of / returns /", () => {
        var buf : [4096]char;
        var r = path::basename("/", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '/' && buf[1] == '\0';
    })
}

@test
func test_path_basename_empty() {
    test("path::basename of empty returns .", () => {
        var buf : [4096]char;
        var r = path::basename("", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '.' && buf[1] == '\0';
    })
}

@test
func test_path_dirname_simple() {
    test("path::dirname /usr/bin/gcc returns /usr/bin", () => {
        var buf : [4096]char;
        var r = path::dirname("/usr/bin/gcc", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
               buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0';
    })
}

@test
func test_path_dirname_root() {
    test("path::dirname of / returns /", () => {
        var buf : [4096]char;
        var r = path::dirname("/", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '/' && buf[1] == '\0';
    })
}

@test
func test_path_join_simple() {
    test("path::join /usr + bin = /usr/bin", () => {
        var buf : [4096]char;
        var r = path::join("/usr", "bin", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
               buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0';
    })
}

@test
func test_path_join_absolute_b() {
    test("path::join returns b if b is absolute", () => {
        var buf : [4096]char;
        var r = path::join("/usr", "/bin", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 4 && buf[0] == '/' && buf[1] == 'b' && buf[2] == 'i' && buf[3] == 'n' && buf[4] == '\0';
    })
}

@test
func test_path_join_trailing_sep() {
    test("path::join /usr/ + bin = /usr/bin no double sep", () => {
        var buf : [4096]char;
        var r = path::join("/usr/", "bin", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
               buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0';
    })
}

@test
func test_path_normalize_dotdot() {
    test("path::normalize resolves ..", () => {
        var buf : [4096]char;
        var r = path::normalize("/usr/bin/../lib/file.txt", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
               buf[4] == '/' && buf[5] == 'l' && buf[6] == 'i' && buf[7] == 'b' &&
               buf[8] == '/' && buf[9] == 'f' && buf[10] == 'i' && buf[11] == 'l' &&
               buf[12] == 'e' && buf[13] == '.' && buf[14] == 't' && buf[15] == 'x' &&
               buf[16] == 't' && buf[17] == '\0';
    })
}

@test
func test_path_normalize_dot() {
    test("path::normalize removes .", () => {
        var buf : [4096]char;
        var r = path::normalize("/usr/./bin", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
               buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0';
    })
}

@test
func test_path_normalize_double_sep() {
    test("path::normalize removes // separators", () => {
        var buf : [4096]char;
        var r = path::normalize("//usr///bin", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
               buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0';
    })
}

@test
func test_path_normalize_relative_dotdot() {
    test("path::normalize relative with ..", () => {
        var buf : [4096]char;
        var r = path::normalize("a/b/../c", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 4 && buf[0] == 'a' && buf[1] == '/' && buf[2] == 'c' && buf[3] == '\0';
    })
}

@test
func test_path_normalize_trailing_slash() {
    test("path::normalize removes trailing /", () => {
        var buf : [4096]char;
        var r = path::normalize("/usr/bin/", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
               buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0';
    })
}

@test
func test_path_normalize_empty() {
    test("path::normalize of empty returns .", () => {
        var buf : [4096]char;
        var r = path::normalize("", &raw mut buf[0], 4096);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 1 && buf[0] == '.' && buf[1] == '\0';
    })
}

@test
func test_path_is_absolute_true() {
    test("path::is_absolute /usr returns true", () => {
        return path::is_absolute("/usr");
    })
}

@test
func test_path_is_absolute_false() {
    test("path::is_absolute relative returns false", () => {
        return !path::is_absolute("relative");
    })
}

@test
func test_path_has_root() {
    test("path::has_root /usr returns true", () => {
        return path::has_root("/usr");
    })
}

@test
func test_path_extension() {
    test("path::extension file.txt returns .txt", () => {
        var buf : [256]char;
        var r = path::extension("file.txt", &raw mut buf[0], 256);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 4 && buf[0] == '.' && buf[1] == 't' && buf[2] == 'x' && buf[3] == 't' && buf[4] == '\0';
    })
}

@test
func test_path_extension_no_ext() {
    test("path::extension file returns empty", () => {
        var buf : [256]char;
        var r = path::extension("file", &raw mut buf[0], 256);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 0 && buf[0] == '\0';
    })
}

@test
func test_path_extension_hidden() {
    test("path::extension .gitignore returns empty", () => {
        var buf : [256]char;
        var r = path::extension(".gitignore", &raw mut buf[0], 256);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 0 && buf[0] == '\0';
    })
}

@test
func test_path_stem() {
    test("path::stem file.txt returns file", () => {
        var buf : [256]char;
        var r = path::stem("file.txt", &raw mut buf[0], 256);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 4 && buf[0] == 'f' && buf[1] == 'i' && buf[2] == 'l' && buf[3] == 'e' && buf[4] == '\0';
    })
}

@test
func test_path_stem_no_ext() {
    test("path::stem README returns README", () => {
        var buf : [256]char;
        var r = path::stem("README", &raw mut buf[0], 256);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 6 && buf[0] == 'R' && buf[1] == 'E' && buf[2] == 'A' && buf[3] == 'D' && buf[4] == 'M' && buf[5] == 'E' && buf[6] == '\0';
    })
}

// ---------------------------------------------------------------------------
// Encoding Library Tests
// ---------------------------------------------------------------------------

@test
func test_hex_encode() {
    test("encoding::hex_encode Hel = 48656c", () => {
        var buf : [128]char;
        var data : [3]u8 = [ 0x48, 0x65, 0x6C ];
        var r = encoding::hex_encode(&raw data[0], 3, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 6 && buf[0] == '4' && buf[1] == '8' && buf[2] == '6' && buf[3] == '5' && buf[4] == '6' && buf[5] == 'c' && buf[6] == '\0';
    })
}

@test
func test_hex_encode_empty() {
    test("encoding::hex_encode empty = empty", () => {
        var buf : [128]char;
        var r = encoding::hex_encode(&raw buf[0] as *u8, 0, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 0 && buf[0] == '\0';
    })
}

@test
func test_hex_decode() {
    test("encoding::hex_decode 48656C6C6F = Hello", () => {
        var buf : [64]u8;
        var r = encoding::hex_decode("48656C6C6F", &raw mut buf[0], 64);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 5 && buf[0] == 0x48 && buf[1] == 0x65 && buf[2] == 0x6C && buf[3] == 0x6C && buf[4] == 0x6F;
    })
}

@test
func test_hex_decode_invalid_odd() {
    test("encoding::hex_decode rejects odd length", () => {
        var buf : [64]u8;
        var r = encoding::hex_decode("486", &raw mut buf[0], 64);
        return r is Result.Err;
    })
}

@test
func test_hex_decode_invalid_char() {
    test("encoding::hex_decode rejects invalid char", () => {
        var buf : [64]u8;
        var r = encoding::hex_decode("4G", &raw mut buf[0], 64);
        return r is Result.Err;
    })
}

@test
func test_url_encode_simple() {
    test("encoding::url_encode leaves unreserved chars", () => {
        var buf : [128]char;
        var r = encoding::url_encode("hello", 5, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 5 && buf[0] == 'h' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'l' && buf[4] == 'o' && buf[5] == '\0';
    })
}

@test
func test_url_encode_space() {
    test("encoding::url_encode space = %20", () => {
        var buf : [128]char;
        var r = encoding::url_encode("hello world", 11, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return buf[0] == 'h' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'l' && buf[4] == 'o' &&
               buf[5] == '%' && buf[6] == '2' && buf[7] == '0' &&
               buf[8] == 'w' && buf[9] == 'o' && buf[10] == 'r' && buf[11] == 'l' && buf[12] == 'd' && buf[13] == '\0';
    })
}

@test
func test_url_encode_special() {
    test("encoding::url_encode encodes ? = %3F", () => {
        var buf : [128]char;
        var r = encoding::url_encode("a?b=c&d", 7, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return buf[0] == 'a' && buf[1] == '%' && buf[2] == '3' && buf[3] == 'F' &&
               buf[4] == 'b' && buf[5] == '%' && buf[6] == '3' && buf[7] == 'D' &&
               buf[8] == 'c' && buf[9] == '%' && buf[10] == '2' && buf[11] == '6' &&
               buf[12] == 'd' && buf[13] == '\0';
    })
}

@test
func test_url_decode() {
    test("encoding::url_decode hello+world%21 = hello world!", () => {
        var buf : [128]char;
        var r = encoding::url_decode("hello+world%21", 14, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 12 && buf[0] == 'h' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'l' && buf[4] == 'o' &&
               buf[5] == ' ' && buf[6] == 'w' && buf[7] == 'o' && buf[8] == 'r' && buf[9] == 'l' && buf[10] == 'd' && buf[11] == '!' && buf[12] == '\0';
    })
}

@test
func test_url_decode_simple() {
    test("encoding::url_decode passes through plain text", () => {
        var buf : [128]char;
        var r = encoding::url_decode("simple", 6, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 6 && buf[0] == 's' && buf[1] == 'i' && buf[2] == 'm' && buf[3] == 'p' && buf[4] == 'l' && buf[5] == 'e' && buf[6] == '\0';
    })
}

@test
func test_utf8_valid_ascii() {
    test("encoding::utf8_is_valid accepts ASCII", () => {
        return encoding::utf8_is_valid("hello", 5);
    })
}

@test
func test_utf8_valid_multi() {
    test("encoding::utf8_is_valid accepts café", () => {
        var utf8_bytes : [5]u8 = [ 0x63, 0x61, 0x66, 0xC3, 0xA9 ];
        return encoding::utf8_is_valid(&raw utf8_bytes[0] as *char, 5);
    })
}

@test
func test_utf8_invalid_bytes() {
    test("encoding::utf8_is_valid rejects 0xFF 0xFE", () => {
        var invalid : [2]u8 = [ 0xFF, 0xFE ];
        return !encoding::utf8_is_valid(&raw invalid[0] as *char, 2);
    })
}

@test
func test_utf8_invalid_continuation() {
    test("encoding::utf8_is_valid rejects bad continuation", () => {
        var invalid : [2]u8 = [ 0xC0, 0x00 ];
        return !encoding::utf8_is_valid(&raw invalid[0] as *char, 2);
    })
}

@test
func test_utf8_truncated() {
    test("encoding::utf8_is_valid rejects truncated seq", () => {
        var truncated : [2]u8 = [ 0xE0, 0xA0 ];
        return !encoding::utf8_is_valid(&raw truncated[0] as *char, 2);
    })
}

// ---------------------------------------------------------------------------
// Crypto Library Tests
// ---------------------------------------------------------------------------

@test
func test_sha256_hello() {
    test("SHA-256 of Hello is correct", () => {
        var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
        var digest : [32]u8;
        crypto::sha256_hash(&raw data[0], 5, &raw mut digest[0]);
        return digest[0] == 0x18 && digest[1] == 0x5F && digest[2] == 0x8D && digest[3] == 0xB3 &&
               digest[4] == 0x22 && digest[5] == 0x71 && digest[6] == 0xFE && digest[7] == 0x25 &&
               digest[8] == 0xF5 && digest[9] == 0x61 && digest[10] == 0xA6 && digest[11] == 0xFC &&
               digest[12] == 0x93 && digest[13] == 0x8B && digest[14] == 0x2E && digest[15] == 0x26 &&
               digest[16] == 0x43 && digest[17] == 0x06 && digest[18] == 0xEC && digest[19] == 0x30 &&
               digest[20] == 0x4E && digest[21] == 0xDA && digest[22] == 0x51 && digest[23] == 0x80 &&
               digest[24] == 0x07 && digest[25] == 0xD1 && digest[26] == 0x76 && digest[27] == 0x48 &&
               digest[28] == 0x26 && digest[29] == 0x38 && digest[30] == 0x19 && digest[31] == 0x69;
    })
}

@test
func test_sha256_empty() {
    test("SHA-256 of empty is correct", () => {
        var digest : [32]u8;
        crypto::sha256_hash(&raw digest[0] as *u8, 0, &raw mut digest[0]);
        return digest[0] == 0xE3 && digest[1] == 0xB0 && digest[2] == 0xC4 && digest[3] == 0x42 &&
               digest[4] == 0x98 && digest[5] == 0xFC && digest[6] == 0x1C && digest[7] == 0x14 &&
               digest[8] == 0x9A && digest[9] == 0xFB && digest[10] == 0xF4 && digest[11] == 0xC8 &&
               digest[12] == 0x99 && digest[13] == 0x6F && digest[14] == 0xB9 && digest[15] == 0x24 &&
               digest[16] == 0x27 && digest[17] == 0xAE && digest[18] == 0x41 && digest[19] == 0xE4 &&
               digest[20] == 0x64 && digest[21] == 0x9B && digest[22] == 0x93 && digest[23] == 0x4C &&
               digest[24] == 0xA4 && digest[25] == 0x95 && digest[26] == 0x99 && digest[27] == 0x1B &&
               digest[28] == 0x78 && digest[29] == 0x52 && digest[30] == 0xB8 && digest[31] == 0x55;
    })
}

@test
func test_md5_hello() {
    test("MD5 of Hello is correct", () => {
        var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
        var digest : [16]u8;
        crypto::md5_hash(&raw data[0], 5, &raw mut digest[0]);
        return digest[0] == 0x8B && digest[1] == 0x1A && digest[2] == 0x99 && digest[3] == 0x53 &&
               digest[4] == 0xC4 && digest[5] == 0x61 && digest[6] == 0x12 && digest[7] == 0x96 &&
               digest[8] == 0xA8 && digest[9] == 0x27 && digest[10] == 0xAB && digest[11] == 0xF8 &&
               digest[12] == 0xC4 && digest[13] == 0x78 && digest[14] == 0x04 && digest[15] == 0xD7;
    })
}

@test
func test_md5_empty() {
    test("MD5 of empty is correct", () => {
        var digest : [16]u8;
        crypto::md5_hash(&raw digest[0] as *u8, 0, &raw mut digest[0]);
        return digest[0] == 0xD4 && digest[1] == 0x1D && digest[2] == 0x8C && digest[3] == 0xD9 &&
               digest[4] == 0x8F && digest[5] == 0x00 && digest[6] == 0xB2 && digest[7] == 0x04 &&
               digest[8] == 0xE9 && digest[9] == 0x80 && digest[10] == 0x09 && digest[11] == 0x98 &&
               digest[12] == 0xEC && digest[13] == 0xF8 && digest[14] == 0x42 && digest[15] == 0x7E;
    })
}

@test
func test_base64_encode() {
    test("base64_encode of Hel = SGV s", () => {
        var buf : [128]char;
        var data : [3]u8 = [ 0x48, 0x65, 0x6C ];
        var r = crypto::base64_encode(&raw data[0], 3, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 4 && buf[0] == 'S' && buf[1] == 'G' && buf[2] == 'V' && buf[3] == 's' && buf[4] == '\0';
    })
}

@test
func test_base64_encode_padded() {
    test("base64_encode of Hell = SGVsbA==", () => {
        var buf : [128]char;
        var data : [4]u8 = [ 0x48, 0x65, 0x6C, 0x6C ];
        var r = crypto::base64_encode(&raw data[0], 4, &raw mut buf[0], 128);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 8 && buf[0] == 'S' && buf[1] == 'G' && buf[2] == 'V' && buf[3] == 's' &&
               buf[4] == 'b' && buf[5] == 'A' && buf[6] == '=' && buf[7] == '=' && buf[8] == '\0';
    })
}

@test
func test_base64_decode() {
    test("base64_decode of Hello b64 returns Hello", () => {
        var buf : [64]u8;
        var r = crypto::base64_decode("SGVsbG8=", 8, &raw mut buf[0], 64);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 5 && buf[0] == 0x48 && buf[1] == 0x65 && buf[2] == 0x6C && buf[3] == 0x6C && buf[4] == 0x6F;
    })
}

@test
func test_base64_roundtrip() {
    test("base64 round-trip preserves data", () => {
        var original : [6]u8 = [ 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF ];
        var encoded : [16]char;
        var decoded : [6]u8;
        var r = crypto::base64_encode(&raw original[0], 6, &raw mut encoded[0], 16);
        if(r is Result.Err) { return false; }
        var Ok(enc_len) = r else unreachable;
        var r2 = crypto::base64_decode(encoded, enc_len, &raw mut decoded[0], 6);
        if(r2 is Result.Err) { return false; }
        var Ok(dec_len) = r2 else unreachable;
        return dec_len == 6 && decoded[0] == 0xDE && decoded[1] == 0xAD && decoded[2] == 0xBE &&
               decoded[3] == 0xEF && decoded[4] == 0x00 && decoded[5] == 0xFF;
    })
}

@test
func test_hmac_sha256_not_zero() {
    test("HMAC-SHA256 produces non-zero output", () => {
        var key : [3]u8 = [ 0x6B, 0x65, 0x79 ];
        var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
        var digest : [32]u8;
        crypto::hmac_sha256(&raw key[0], 3, &raw data[0], 5, &raw mut digest[0]);
        return digest[0] != 0 || digest[1] != 0 || digest[2] != 0 || digest[3] != 0;
    })
}

@test
func test_constant_time_equal_same() {
    test("constant_time_equal returns true for equal buffers", () => {
        var a : [3]u8 = [ 0x01, 0x02, 0x03 ];
        var b : [3]u8 = [ 0x01, 0x02, 0x03 ];
        return crypto::constant_time_equal(&raw a[0], &raw b[0], 3);
    })
}

@test
func test_constant_time_equal_different() {
    test("constant_time_equal returns false for diff buffers", () => {
        var a : [3]u8 = [ 0x01, 0x02, 0x03 ];
        var b : [3]u8 = [ 0x01, 0xFF, 0x03 ];
        return !crypto::constant_time_equal(&raw a[0], &raw b[0], 3);
    })
}

@test
func test_constant_time_equal_empty() {
    test("constant_time_equal returns true for empty buffers", () => {
        var dummy : [1]u8 = [ 0 ];
        return crypto::constant_time_equal(&raw dummy[0], &raw dummy[0], 0);
    })
}

// ---------------------------------------------------------------------------
// Compression Library Tests
// ---------------------------------------------------------------------------

@test
func test_rle_roundtrip_simple() {
    test("RLE round-trip preserves data", () => {
        var input : [10]u8 = [ 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x44 ];
        var compressed : [64]u8;
        var comp_len : size_t = 0;
        var r = compression::compress(&raw input[0], 10, &raw mut compressed[0], &raw mut comp_len, 64);
        if(r is Result.Err) { return false; }
        var decompressed : [64]u8;
        var dec_len : size_t = 0;
        var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
        if(r2 is Result.Err) { return false; }
        if(dec_len != 10) { return false; }
        var i : size_t = 0;
        while(i < 10) {
            if(decompressed[i] != input[i]) { return false; }
            i += 1;
        }
        return true;
    })
}

@test
func test_rle_roundtrip_single_byte() {
    test("RLE round-trip single byte", () => {
        var input : [1]u8 = [ 0x42 ];
        var compressed : [64]u8;
        var comp_len : size_t = 0;
        var r = compression::compress(&raw input[0], 1, &raw mut compressed[0], &raw mut comp_len, 64);
        if(r is Result.Err) { return false; }
        var decompressed : [64]u8;
        var dec_len : size_t = 0;
        var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
        if(r2 is Result.Err) { return false; }
        return dec_len == 1 && decompressed[0] == 0x42;
    })
}

@test
func test_rle_roundtrip_all_same() {
    test("RLE round-trip all same byte (0xFF)", () => {
        var input : [20]u8;
        var i : size_t = 0;
        while(i < 20) { input[i] = 0xFF; i += 1; }
        var compressed : [64]u8;
        var comp_len : size_t = 0;
        var r = compression::compress(&raw input[0], 20, &raw mut compressed[0], &raw mut comp_len, 64);
        if(r is Result.Err) { return false; }
        var decompressed : [64]u8;
        var dec_len : size_t = 0;
        var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
        if(r2 is Result.Err) { return false; }
        if(dec_len != 20) { return false; }
        var j : size_t = 0;
        while(j < 20) {
            if(decompressed[j] != 0xFF) { return false; }
            j += 1;
        }
        return true;
    })
}

@test
func test_rle_roundtrip_alternating() {
    test("RLE round-trip alternating bytes", () => {
        var input : [6]u8 = [ 0x41, 0x42, 0x41, 0x42, 0x41, 0x42 ];
        var compressed : [64]u8;
        var comp_len : size_t = 0;
        var r = compression::compress(&raw input[0], 6, &raw mut compressed[0], &raw mut comp_len, 64);
        if(r is Result.Err) { return false; }
        var decompressed : [64]u8;
        var dec_len : size_t = 0;
        var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
        if(r2 is Result.Err) { return false; }
        if(dec_len != 6) { return false; }
        var k : size_t = 0;
        while(k < 6) {
            if(decompressed[k] != input[k]) { return false; }
            k += 1;
        }
        return true;
    })
}

@test
func test_rle_decompress_empty() {
    test("RLE decompress of empty works", () => {
        var compressed : [8]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0 ];
        var decompressed : [8]u8;
        var dec_len : size_t = 0;
        var r = compression::decompress(&raw compressed[0], 1, &raw mut decompressed[0], &raw mut dec_len, 8);
        if(r is Result.Err) { return false; }
        var Ok(len) = r else unreachable;
        return len == 0;
    })
}
