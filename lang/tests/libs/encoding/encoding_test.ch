using namespace std;

// ---------------------------------------------------------------------------

@test
func test_hex_encode(env : &mut TestEnv) {
    var buf : [128]char;
    var data : [3]u8 = [ 0x48, 0x65, 0x6C ];
    var r = encoding::hex_encode(&raw data[0], 3, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("hex_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 6 && buf[0] == '4' && buf[1] == '8' && buf[2] == '6' && buf[3] == '5' && buf[4] == '6' && buf[5] == 'c' && buf[6] == '\0')) {
        env.error("hex_encode Hel should return 48656c");
    }
}

@test
func test_hex_encode_empty(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::hex_encode(&raw buf[0] as *u8, 0, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("hex_encode empty failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 0 && buf[0] == '\0')) {
        env.error("hex_encode of empty should return empty");
    }
}

@test
func test_hex_decode(env : &mut TestEnv) {
    var buf : [64]u8;
    var r = encoding::hex_decode("48656C6C6F", &raw mut buf[0], 64);
    if(r is Result.Err) { env.error("hex_decode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 5 && buf[0] == 0x48 && buf[1] == 0x65 && buf[2] == 0x6C && buf[3] == 0x6C && buf[4] == 0x6F)) {
        env.error("hex_decode 48656C6C6F should return Hello");
    }
}

@test
func test_hex_decode_invalid_odd(env : &mut TestEnv) {
    var buf : [64]u8;
    var r = encoding::hex_decode("486", &raw mut buf[0], 64);
    if(!(r is Result.Err)) {
        env.error("hex_decode should reject odd length");
    }
}

@test
func test_hex_decode_invalid_char(env : &mut TestEnv) {
    var buf : [64]u8;
    var r = encoding::hex_decode("4G", &raw mut buf[0], 64);
    if(!(r is Result.Err)) {
        env.error("hex_decode should reject invalid char");
    }
}

@test
func test_url_encode_simple(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_encode("hello", 5, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 5 && buf[0] == 'h' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'l' && buf[4] == 'o' && buf[5] == '\0')) {
        env.error("url_encode hello should return hello");
    }
}

@test
func test_url_encode_space(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_encode("hello world", 11, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(buf[0] == 'h' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'l' && buf[4] == 'o' &&
         buf[5] == '%' && buf[6] == '2' && buf[7] == '0' &&
         buf[8] == 'w' && buf[9] == 'o' && buf[10] == 'r' && buf[11] == 'l' && buf[12] == 'd' && buf[13] == '\0')) {
        env.error("url_encode hello world should encode space as %20");
    }
}

@test
func test_url_encode_special(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_encode("a?b=c&d", 7, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_encode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(buf[0] == 'a' && buf[1] == '%' && buf[2] == '3' && buf[3] == 'F' &&
         buf[4] == 'b' && buf[5] == '%' && buf[6] == '3' && buf[7] == 'D' &&
         buf[8] == 'c' && buf[9] == '%' && buf[10] == '2' && buf[11] == '6' &&
         buf[12] == 'd' && buf[13] == '\0')) {
        env.error("url_encode a?b=c&d should encode special chars");
    }
}

@test
func test_url_decode(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_decode("hello+world%21", 14, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_decode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 12 && buf[0] == 'h' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'l' && buf[4] == 'o' &&
         buf[5] == ' ' && buf[6] == 'w' && buf[7] == 'o' && buf[8] == 'r' && buf[9] == 'l' && buf[10] == 'd' && buf[11] == '!' && buf[12] == '\0')) {
        env.error("url_decode hello+world%21 should decode");
    }
}

@test
func test_url_decode_simple(env : &mut TestEnv) {
    var buf : [128]char;
    var r = encoding::url_decode("simple", 6, &raw mut buf[0], 128);
    if(r is Result.Err) { env.error("url_decode failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 6 && buf[0] == 's' && buf[1] == 'i' && buf[2] == 'm' && buf[3] == 'p' && buf[4] == 'l' && buf[5] == 'e' && buf[6] == '\0')) {
        env.error("url_decode simple should pass through");
    }
}

@test
func test_utf8_valid_ascii(env : &mut TestEnv) {
    if(!encoding::utf8_is_valid("hello", 5)) {
        env.error("utf8_is_valid should accept ASCII");
    }
}

@test
func test_utf8_valid_multi(env : &mut TestEnv) {
    var utf8_bytes : [5]u8 = [ 0x63, 0x61, 0x66, 0xC3, 0xA9 ];
    if(!encoding::utf8_is_valid(&raw utf8_bytes[0] as *char, 5)) {
        env.error("utf8_is_valid should accept café");
    }
}

@test
func test_utf8_invalid_bytes(env : &mut TestEnv) {
    var invalid : [2]u8 = [ 0xFF, 0xFE ];
    if(encoding::utf8_is_valid(&raw invalid[0] as *char, 2)) {
        env.error("utf8_is_valid should reject 0xFF 0xFE");
    }
}

@test
func test_utf8_invalid_continuation(env : &mut TestEnv) {
    var invalid : [2]u8 = [ 0xC0, 0x00 ];
    if(encoding::utf8_is_valid(&raw invalid[0] as *char, 2)) {
        env.error("utf8_is_valid should reject bad continuation");
    }
}

@test
func test_utf8_truncated(env : &mut TestEnv) {
    var truncated : [2]u8 = [ 0xE0, 0xA0 ];
    if(encoding::utf8_is_valid(&raw truncated[0] as *char, 2)) {
        env.error("utf8_is_valid should reject truncated seq");
    }
}

// ---------------------------------------------------------------------------
