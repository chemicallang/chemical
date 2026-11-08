using namespace std;

// Focused tests for `encoding::safe` — the owned-buffer encoding surface.
// Every case uses `&[u8]`/`&str` inputs and owned outputs; no raw buffers.

func encoding_safe_equals(s : string, expected : std::string_view) : bool {
    return s.equals_view(&expected)
}

func encoding_safe_bytes_eq(v : &vector<u8>, expected : *u8, n : size_t) : bool {
    if(v.size() != n) { return false }
    var i : size_t = 0
    while(i < n) {
        if(v.get(i) != unsafe(expected[i])) { return false }
        i += 1
    }
    return true
}

func encoding_safe_bytes_all(v : &vector<u8>, expected : u8) : bool {
    var i : size_t = 0
    while(i < v.size()) {
        if(v.get(i) != expected) { return false }
        i += 1
    }
    return true
}

// ---------------------------------------------------------------------------
// Base64
// ---------------------------------------------------------------------------

@test
func test_encoding_safe_base64_roundtrip(env : &mut TestEnv) {
    var data : [5]u8 = [0x48u8, 0x65u8, 0x6Cu8, 0x6Cu8, 0x6Fu8]
    var view : [u8] = data
    var encoded = encoding::safe::base64_encode(view)
    if(encoded is Result.Err) { env.error("base64_encode failed"); return }
    var Ok(text) = encoded else unsafe { unreachable }
    if(!encoding_safe_equals(text, "SGVsbG8=")) { env.error("base64 of Hello is wrong"); return }

    var decoded = encoding::safe::base64_decode(text.to_view())
    if(decoded is Result.Err) { env.error("base64_decode failed"); return }
    var Ok(bytes) = decoded else unsafe { unreachable }
    if(!encoding_safe_bytes_eq(&bytes, &raw data[0], 5)) { env.error("base64 roundtrip mismatch") }
}

@test
func test_encoding_safe_base64_partial_groups(env : &mut TestEnv) {
    var f : [1]u8 = [0x66u8]
    var fo : [2]u8 = [0x66u8, 0x6Fu8]
    var foo : [3]u8 = [0x66u8, 0x6Fu8, 0x6Fu8]
    var vf : [u8] = f
    var vfo : [u8] = fo
    var vfoo : [u8] = foo

    var ef = encoding::safe::base64_encode(vf)
    var efo = encoding::safe::base64_encode(vfo)
    var efoo = encoding::safe::base64_encode(vfoo)
    if(ef is Result.Err || efo is Result.Err || efoo is Result.Err) {
        env.error("base64_encode partial group failed")
        return
    }
    var Ok(sf) = ef else unsafe { unreachable }
    var Ok(sfo) = efo else unsafe { unreachable }
    var Ok(sfoo) = efoo else unsafe { unreachable }
    if(!encoding_safe_equals(sf, "Zg==")) { env.error("base64 f") }
    if(!encoding_safe_equals(sfo, "Zm8=")) { env.error("base64 fo") }
    if(!encoding_safe_equals(sfoo, "Zm9v")) { env.error("base64 foo") }

    // Unpadded decode (7 chars) must still produce 5 bytes.
    var unpadded = encoding::safe::base64_decode("SGVsbG8")
    if(unpadded is Result.Err) { env.error("unpadded base64 should decode"); return }
    var Ok(u) = unpadded else unsafe { unreachable }
    if(u.size() != 5) { env.error("unpadded base64 length") }
}

@test
func test_encoding_safe_base64_empty(env : &mut TestEnv) {
    var empty = vector<u8>()
    var view : [u8] = &empty
    var encoded = encoding::safe::base64_encode(view)
    if(encoded is Result.Err) { env.error("base64_encode empty failed"); return }
    var Ok(text) = encoded else unsafe { unreachable }
    if(text.size() != 0) { env.error("base64 of empty should be empty") }

    var decoded = encoding::safe::base64_decode("")
    if(decoded is Result.Err) { env.error("base64_decode empty failed"); return }
    var Ok(bytes) = decoded else unsafe { unreachable }
    if(bytes.size() != 0) { env.error("base64 decode of empty should be empty") }
}

@test
func test_encoding_safe_base64_invalid(env : &mut TestEnv) {
    if(encoding::safe::base64_decode("A") is Result.Ok) {
        env.error("base64 should reject length 1")
    }
    if(encoding::safe::base64_decode("!!!!") is Result.Ok) {
        env.error("base64 should reject non-alphabet bytes")
    }
    if(encoding::safe::base64_decode("AA=A") is Result.Ok) {
        env.error("base64 should reject interior padding")
    }
    if(encoding::safe::base64_decode("AAAA=") is Result.Ok) {
        env.error("base64 should reject padding on a non-multiple of 4")
    }
    if(encoding::safe::base64_decode("A===") is Result.Ok) {
        env.error("base64 should reject too much padding")
    }
    if(encoding::safe::base64_decode("====") is Result.Ok) {
        env.error("base64 should reject all-padding input")
    }
}

@test
func test_encoding_safe_base64_large(env : &mut TestEnv) {
    var data = vector<u8>()
    var i : size_t = 0
    while(i < 1024) {
        if(!data.try_push((i * 31 + 7) as u8)) { env.error("test alloc failed"); return }
        i += 1
    }
    var view : [u8] = &data
    var encoded = encoding::safe::base64_encode(view)
    if(encoded is Result.Err) { env.error("base64_encode large failed"); return }
    var Ok(text) = encoded else unsafe { unreachable }
    var decoded = encoding::safe::base64_decode(text.to_view())
    if(decoded is Result.Err) { env.error("base64_decode large failed"); return }
    var Ok(bytes) = decoded else unsafe { unreachable }
    if(bytes.size() != 1024) { env.error("large base64 length"); return }
    var j : size_t = 0
    while(j < 1024) {
        if(bytes.get(j) != data.get(j)) { env.error("large base64 roundtrip mismatch"); return }
        j += 1
    }
}

// ---------------------------------------------------------------------------
// Hex
// ---------------------------------------------------------------------------

@test
func test_encoding_safe_hex_roundtrip(env : &mut TestEnv) {
    var data : [5]u8 = [0x48u8, 0x65u8, 0x6Cu8, 0x6Cu8, 0x6Fu8]
    var view : [u8] = data
    if(!encoding_safe_equals(encoding::safe::hex_encode(view), "48656c6c6f")) {
        env.error("hex_encode Hello")
    }
    if(!encoding_safe_equals(encoding::safe::hex_encode_upper(view), "48656C6C6F")) {
        env.error("hex_encode_upper Hello")
    }
    var decoded = encoding::safe::hex_decode("48656C6C6F")
    if(decoded is Result.Err) { env.error("hex_decode failed"); return }
    var Ok(bytes) = decoded else unsafe { unreachable }
    if(!encoding_safe_bytes_eq(&bytes, &raw data[0], 5)) { env.error("hex roundtrip mismatch") }
}

@test
func test_encoding_safe_hex_empty(env : &mut TestEnv) {
    var empty = vector<u8>()
    var view : [u8] = &empty
    if(encoding::safe::hex_encode(view).size() != 0) { env.error("hex_encode empty") }
    var decoded = encoding::safe::hex_decode("")
    if(decoded is Result.Err) { env.error("hex_decode empty failed"); return }
    var Ok(bytes) = decoded else unsafe { unreachable }
    if(bytes.size() != 0) { env.error("hex_decode empty should be empty") }
}

@test
func test_encoding_safe_hex_invalid(env : &mut TestEnv) {
    if(encoding::safe::hex_decode("abc") is Result.Ok) {
        env.error("hex should reject odd length")
    }
    if(encoding::safe::hex_decode("0g") is Result.Ok) {
        env.error("hex should reject non-hex characters")
    }
    if(encoding::safe::hex_decode("4 4") is Result.Ok) {
        env.error("hex should reject whitespace")
    }
}

// ---------------------------------------------------------------------------
// URL percent-encoding
// ---------------------------------------------------------------------------

@test
func test_encoding_safe_url_roundtrip(env : &mut TestEnv) {
    if(!encoding_safe_equals(encoding::safe::url_encode("hello world"), "hello%20world")) {
        env.error("url_encode space")
    }
    if(!encoding_safe_equals(encoding::safe::url_encode("a/b?c=d&e"), "a%2Fb%3Fc%3Dd%26e")) {
        env.error("url_encode reserved")
    }
    var decoded = encoding::safe::url_decode("hello%20world")
    if(decoded is Result.Err) { env.error("url_decode failed"); return }
    var Ok(text) = decoded else unsafe { unreachable }
    if(!encoding_safe_equals(text, "hello world")) { env.error("url_decode space") }

    var plus = encoding::safe::url_decode("a+b")
    if(plus is Result.Err) { env.error("url_decode plus failed"); return }
    var Ok(plus_text) = plus else unsafe { unreachable }
    if(!encoding_safe_equals(plus_text, "a b")) { env.error("url_decode plus") }
}

@test
func test_encoding_safe_url_utf8_roundtrip(env : &mut TestEnv) {
    var source = string("héllo 😀")
    var encoded = encoding::safe::url_encode(source.to_view())
    var decoded = encoding::safe::url_decode(encoded.to_view())
    if(decoded is Result.Err) { env.error("url utf8 decode failed"); return }
    var Ok(text) = decoded else unsafe { unreachable }
    if(!text.equals_view(source.to_view())) { env.error("url utf8 roundtrip mismatch") }
}

@test
func test_encoding_safe_url_invalid(env : &mut TestEnv) {
    if(encoding::safe::url_decode("%") is Result.Ok) {
        env.error("url should reject a bare percent")
    }
    if(encoding::safe::url_decode("%2") is Result.Ok) {
        env.error("url should reject a truncated escape")
    }
    if(encoding::safe::url_decode("%zz") is Result.Ok) {
        env.error("url should reject non-hex escapes")
    }
    if(encoding::safe::url_decode("%FF") is Result.Ok) {
        env.error("url should reject escapes that decode to invalid UTF-8")
    }
}

@test
func test_encoding_safe_url_empty(env : &mut TestEnv) {
    if(encoding::safe::url_encode("").size() != 0) { env.error("url_encode empty") }
    var decoded = encoding::safe::url_decode("")
    if(decoded is Result.Err) { env.error("url_decode empty failed"); return }
    var Ok(text) = decoded else unsafe { unreachable }
    if(text.size() != 0) { env.error("url_decode empty should be empty") }
}

// ---------------------------------------------------------------------------
// UTF-8 / UTF-16
// ---------------------------------------------------------------------------

@test
func test_encoding_safe_utf8_validation(env : &mut TestEnv) {
    var ascii : [3]u8 = [0x61u8, 0x62u8, 0x63u8]
    var two : [2]u8 = [0xC3u8, 0xA9u8]             // é
    var three : [3]u8 = [0xE2u8, 0x82u8, 0xACu8]   // €
    var four : [4]u8 = [0xF0u8, 0x9Fu8, 0x98u8, 0x80u8] // 😀
    var va : [u8] = ascii
    var v2 : [u8] = two
    var v3 : [u8] = three
    var v4 : [u8] = four
    if(!encoding::safe::utf8_is_valid(va)) { env.error("utf8 ascii") }
    if(!encoding::safe::utf8_is_valid(v2)) { env.error("utf8 2-byte") }
    if(!encoding::safe::utf8_is_valid(v3)) { env.error("utf8 3-byte") }
    if(!encoding::safe::utf8_is_valid(v4)) { env.error("utf8 4-byte") }
    if(!encoding::safe::utf8_is_valid_str("héllo 😀")) { env.error("utf8 str") }

    var overlong : [2]u8 = [0xC0u8, 0x80u8]
    var surrogate : [3]u8 = [0xEDu8, 0xA0u8, 0x80u8]
    var truncated : [2]u8 = [0xE2u8, 0x82u8]
    var too_high : [4]u8 = [0xF5u8, 0x80u8, 0x80u8, 0x80u8]
    var lone_cont : [1]u8 = [0x80u8]
    var vo : [u8] = overlong
    var vs : [u8] = surrogate
    var vt : [u8] = truncated
    var vh : [u8] = too_high
    var vl : [u8] = lone_cont
    if(encoding::safe::utf8_is_valid(vo)) { env.error("utf8 should reject overlong") }
    if(encoding::safe::utf8_is_valid(vs)) { env.error("utf8 should reject surrogates") }
    if(encoding::safe::utf8_is_valid(vt)) { env.error("utf8 should reject truncation") }
    if(encoding::safe::utf8_is_valid(vh)) { env.error("utf8 should reject > U+10FFFF") }
    if(encoding::safe::utf8_is_valid(vl)) { env.error("utf8 should reject lone continuation") }
}

@test
func test_encoding_safe_utf8_decode_char(env : &mut TestEnv) {
    var four : [4]u8 = [0xF0u8, 0x9Fu8, 0x98u8, 0x80u8]
    var v4 : [u8] = four
    var decoded = encoding::safe::utf8_decode_char(v4, 0)
    if(decoded is Result.Err) { env.error("utf8_decode_char failed"); return }
    var Ok(ch) = decoded else unsafe { unreachable }
    if(ch.codepoint != 0x1F600u32) { env.error("utf8_decode_char codepoint") }
    if(ch.size != 4) { env.error("utf8_decode_char size") }

    if(encoding::safe::utf8_decode_char(v4, 4) is Result.Ok) {
        env.error("utf8_decode_char should reject out-of-range offset")
    }
    var truncated : [2]u8 = [0xE2u8, 0x82u8]
    var vt : [u8] = truncated
    if(encoding::safe::utf8_decode_char(vt, 0) is Result.Ok) {
        env.error("utf8_decode_char should reject truncation")
    }
}

@test
func test_encoding_safe_utf8_to_string(env : &mut TestEnv) {
    var valid : [2]u8 = [0xC3u8, 0xA9u8]
    var vv : [u8] = valid
    var made = encoding::safe::utf8_to_string(vv)
    if(made is Result.Err) { env.error("utf8_to_string valid failed"); return }
    var Ok(text) = made else unsafe { unreachable }
    if(!encoding_safe_equals(text, "é")) { env.error("utf8_to_string content") }

    var invalid : [2]u8 = [0xC0u8, 0x80u8]
    var vi : [u8] = invalid
    if(encoding::safe::utf8_to_string(vi) is Result.Ok) {
        env.error("utf8_to_string should reject invalid input")
    }
}

@test
func test_encoding_safe_utf16_roundtrip(env : &mut TestEnv) {
    var source = string("Hello 😀 é")
    var encoded = encoding::safe::utf8_to_utf16(source.to_view())
    if(encoded is Result.Err) { env.error("utf8_to_utf16 failed"); return }
    var Ok(units) = encoded else unsafe { unreachable }
    // "Hello " = 6, emoji = 2, " é" = 2 code units.
    if(units.size() != 10) { env.error("utf8_to_utf16 length"); return }

    var decoded = encoding::safe::utf16_to_utf8(&units)
    if(decoded is Result.Err) { env.error("utf16_to_utf8 failed"); return }
    var Ok(text) = decoded else unsafe { unreachable }
    if(!text.equals_view(source.to_view())) { env.error("utf16 roundtrip mismatch") }
}

@test
func test_encoding_safe_utf16_empty(env : &mut TestEnv) {
    var empty = vector<u8>()
    var view : [u8] = &empty
    var encoded = encoding::safe::utf8_bytes_to_utf16(view)
    if(encoded is Result.Err) { env.error("utf8_bytes_to_utf16 empty failed"); return }
    var Ok(units) = encoded else unsafe { unreachable }
    if(units.size() != 0) { env.error("utf8_bytes_to_utf16 empty should be empty") }

    var no_units = vector<u16>()
    var decoded = encoding::safe::utf16_to_utf8(&no_units)
    if(decoded is Result.Err) { env.error("utf16_to_utf8 empty failed"); return }
    var Ok(text) = decoded else unsafe { unreachable }
    if(text.size() != 0) { env.error("utf16_to_utf8 empty should be empty") }
}

@test
func test_encoding_safe_utf16_invalid(env : &mut TestEnv) {
    // Lone high surrogate.
    var lone_high : [1]u16 = [0xD83Du16]
    var vh : [u16] = lone_high
    if(encoding::safe::utf16_to_utf8(vh) is Result.Ok) {
        env.error("utf16 should reject a lone high surrogate")
    }
    // Lone low surrogate.
    var lone_low : [1]u16 = [0xDE00u16]
    var vl : [u16] = lone_low
    if(encoding::safe::utf16_to_utf8(vl) is Result.Ok) {
        env.error("utf16 should reject a lone low surrogate")
    }
    // High surrogate followed by a non-surrogate.
    var bad_pair : [2]u16 = [0xD83Du16, 0x0041u16]
    var vb : [u16] = bad_pair
    if(encoding::safe::utf16_to_utf8(vb) is Result.Ok) {
        env.error("utf16 should reject a broken surrogate pair")
    }
    // Truncated UTF-8 must be rejected by the byte API.
    var truncated : [2]u8 = [0xE2u8, 0x82u8]
    var vt : [u8] = truncated
    if(encoding::safe::utf8_bytes_to_utf16(vt) is Result.Ok) {
        env.error("utf8_bytes_to_utf16 should reject truncation")
    }
}
