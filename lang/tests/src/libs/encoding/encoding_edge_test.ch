using namespace std;

// ---------------------------------------------------------------------------
// Encoding Library Edge-Case Tests
// ---------------------------------------------------------------------------

@test
func test_hex_encode_upper(env : &mut TestEnv) {
    var buf : [128]char;
    var data : [3]u8 = [ 0xAB, 0xCD, 0xEF ];
    var r = encoding::hex_encode_upper(&raw data[0], 3, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("hex_encode_upper failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 6 && buf[0] == 'A' && buf[1] == 'B' && buf[2] == 'C' &&
         buf[3] == 'D' && buf[4] == 'E' && buf[5] == 'F' && buf[6] == '\0')) {
        env.error("hex_encode_upper of [0xAB,0xCD,0xEF] should return ABCDEF");
    }
}

@test
func test_hex_decode_mixed_case(env : &mut TestEnv) {
    var buf : [64]u8;
    var r = encoding::hex_decode("4aBcDeF1", &raw mut buf[0], 64);
    if(r is Result.Err) { env.error("hex_decode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == 0x4A && buf[1] == 0xBC && buf[2] == 0xDE && buf[3] == 0xF1)) {
        env.error("hex_decode should accept mixed case");
    }
}

@test
func test_hex_decode_empty(env : &mut TestEnv) {
    var buf : [64]u8;
    var r = encoding::hex_decode("", &raw mut buf[0], 64);
    if(r is Result.Err) { env.error("hex_decode empty failed"); return; }
    var Ok(len) = r else unreachable;
    if(len != 0) {
        env.error("hex_decode of empty should return len 0");
    }
}

@test
func test_hex_decode_to_vec(env : &mut TestEnv) {
    var sv = std::string_view("48656C6C6F");
    var r = encoding::hex_decode_to_vec(sv);
    if(r is Result.Err) { env.error("hex_decode_to_vec failed"); return; }
    var Ok(vec) = r else unreachable;
    if(vec.size() != 5) { env.error("expected 5 bytes"); return; }
    if(!(vec.get(0) == 0x48 && vec.get(1) == 0x65 && vec.get(2) == 0x6C &&
         vec.get(3) == 0x6C && vec.get(4) == 0x6F)) {
        env.error("hex_decode_to_vec content wrong");
    }
}

@test
func test_hex_decode_to_vec_invalid(env : &mut TestEnv) {
    var sv = std::string_view("4G");
    var r = encoding::hex_decode_to_vec(sv);
    if(!(r is Result.Err)) {
        env.error("hex_decode_to_vec should reject invalid hex");
    }
}

@test
func test_hex_encode_all_zeros(env : &mut TestEnv) {
    var buf : [128]char;
    var data : [4]u8 = [ 0, 0, 0, 0 ];
    var r = encoding::hex_encode(&raw data[0], 4, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == '0' && buf[1] == '0' && buf[2] == '0' && buf[3] == '0' &&
         buf[4] == '0' && buf[5] == '0' && buf[6] == '0' && buf[7] == '0' && buf[8] == '\0')) {
        env.error("hex_encode zeros should return 00000000");
    }
}

@test
func test_url_encode_empty(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_encode("", 0, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_encode empty failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 0 && buf[0] == '\0')) {
        env.error("url_encode of empty should return empty");
    }
}

@test
func test_url_encode_all_special(env : &mut TestEnv) {
    var buf : [128]char;
    var data : [3]u8 = [ 0x00, 0x7F, 0xFF ];
    var r = encoding::url_encode(&raw data[0] as *char, 3, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 9 && buf[0] == '%' && buf[1] == '0' && buf[2] == '0' &&
         buf[3] == '%' && buf[4] == '7' && buf[5] == 'F' &&
         buf[6] == '%' && buf[7] == 'F' && buf[8] == 'F' && buf[9] == '\0')) {
        env.error("url_encode of [0x00, 0x7F, 0xFF] should be percent-encoded");
    }
}

@test
func test_url_encode_query(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_encode_query("hello world", 11, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_encode_query failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(buf[5] == '+' && buf[6] == 'w')) {
        env.error("url_encode_query should encode space as +");
    }
}

@test
func test_url_decode_invalid_percent(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_decode("abc%XXdef", 9, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_decode failed"); return; }
    var Ok(len) = r else unreachable;
    // Invalid %XX should pass through literally
    if(!(buf[3] == '%' && buf[4] == 'X' && buf[5] == 'X')) {
        env.error("url_decode should pass through invalid %XX");
    }
}

@test
func test_url_decode_standalone_percent(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_decode("abc%", 4, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_decode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(buf[3] == '%' && buf[4] == '\0')) {
        env.error("url_decode should pass through solo %");
    }
}

@test
func test_utf8_valid_3byte(env : &mut TestEnv) {
    // U+0800 (0xE0 0xA0 0x80) - first 3-byte codepoint
    var bytes : [3]u8 = [ 0xE0, 0xA0, 0x80 ];
    if(!encoding::utf8_is_valid(&raw bytes[0] as *char, 3)) {
        env.error("utf8_is_valid should accept U+0800 (3-byte)");
    }
}

@test
func test_utf8_valid_4byte_emoji(env : &mut TestEnv) {
    // U+1F600 grinning face (0xF0 0x9F 0x98 0x80)
    var bytes : [4]u8 = [ 0xF0, 0x9F, 0x98, 0x80 ];
    if(!encoding::utf8_is_valid(&raw bytes[0] as *char, 4)) {
        env.error("utf8_is_valid should accept U+1F600 (emoji)");
    }
}

@test
func test_utf8_invalid_overlong(env : &mut TestEnv) {
    // Overlong encoding of '/' (0xC0 0xAF) - should be rejected
    var bytes : [2]u8 = [ 0xC0, 0xAF ];
    if(encoding::utf8_is_valid(&raw bytes[0] as *char, 2)) {
        env.error("utf8_is_valid should reject overlong encoding");
    }
}

@test
func test_utf8_invalid_surrogate(env : &mut TestEnv) {
    // U+D800 (0xED 0xA0 0x80) - surrogate half, should be rejected
    var bytes : [3]u8 = [ 0xED, 0xA0, 0x80 ];
    if(encoding::utf8_is_valid(&raw bytes[0] as *char, 3)) {
        env.error("utf8_is_valid should reject surrogate U+D800");
    }
}

@test
func test_utf8_invalid_over_max(env : &mut TestEnv) {
    // U+110000 (0xF4 0x90 0x80 0x80) - beyond Unicode max
    var bytes : [4]u8 = [ 0xF4, 0x90, 0x80, 0x80 ];
    if(encoding::utf8_is_valid(&raw bytes[0] as *char, 4)) {
        env.error("utf8_is_valid should reject codepoint beyond U+10FFFF");
    }
}

@test
func test_utf8_valid_ascii_range(env : &mut TestEnv) {
    // All ASCII bytes 0x01-0x7F should be valid
    var bytes : [127]u8;
    var i : size_t = 0;
    while(i < 127) { bytes[i] = (i + 1) as u8; i += 1; }
    if(!encoding::utf8_is_valid(&raw bytes[0] as *char, 127)) {
        env.error("utf8_is_valid should accept all ASCII bytes");
    }
}

@test
func test_utf8_char_len_all(env : &mut TestEnv) {
    if(encoding::utf8_char_len(0x00) != 1) { env.error("len of 0x00 should be 1"); return; }
    if(encoding::utf8_char_len(0x7F) != 1) { env.error("len of 0x7F should be 1"); return; }
    if(encoding::utf8_char_len(0xC0) != 2) { env.error("len of 0xC0 should be 2"); return; }
    if(encoding::utf8_char_len(0xE0) != 3) { env.error("len of 0xE0 should be 3"); return; }
    if(encoding::utf8_char_len(0xF0) != 4) { env.error("len of 0xF0 should be 4"); return; }
}

@test
func test_utf8_decode_single(env : &mut TestEnv) {
    var data : [1]u8 = [ 0x41 ];
    var cp : int = 0;
    var bytes_out : size_t = 0;
    var ok = encoding::utf8_decode(&raw data[0] as *char, 1, &raw mut cp, &raw mut bytes_out);
    if(!ok) { env.error("utf8_decode of 'A' should succeed"); return; }
    if(cp != 0x41) { env.error("codepoint should be 0x41"); }
    if(bytes_out != 1) { env.error("bytes should be 1"); }
}

@test
func test_utf8_decode_multi(env : &mut TestEnv) {
    var data : [2]u8 = [ 0xC3, 0xA9 ];
    var cp : int = 0;
    var bytes_out : size_t = 0;
    var ok = encoding::utf8_decode(&raw data[0] as *char, 2, &raw mut cp, &raw mut bytes_out);
    if(!ok) { env.error("utf8_decode of é should succeed"); return; }
    if(cp != 0xE9) { env.error("codepoint of é should be U+00E9"); }
    if(bytes_out != 2) { env.error("bytes should be 2"); }
}

@test
func test_utf8_decode_empty(env : &mut TestEnv) {
    var cp : int = 0;
    var bytes_out : size_t = 0;
    var ok = encoding::utf8_decode("", 0, &raw mut cp, &raw mut bytes_out);
    if(ok) { env.error("utf8_decode of empty should fail"); }
}
