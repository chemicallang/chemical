using namespace std;

// Focused tests for `crypto::safe` — one-shot/streaming hashes, HMAC,
// constant-time comparison, and owned Base64 wrappers.

func crypto_safe_hex(p : *mut u8, n : size_t) : string {
    var view : [u8] = unsafe(std::span<u8>(p as *u8, n))
    return encoding::safe::hex_encode(view)
}

func crypto_safe_bytes_eq(v : &vector<u8>, expected : *u8, n : size_t) : bool {
    if(v.size() != n) { return false }
    var i : size_t = 0
    while(i < n) {
        if(v.get(i) != unsafe(expected[i])) { return false }
        i += 1
    }
    return true
}

// ---------------------------------------------------------------------------
// One-shot hashes
// ---------------------------------------------------------------------------

@test
func test_crypto_safe_sha256_abc(env : &mut TestEnv) {
    var data : [3]u8 = [0x61u8, 0x62u8, 0x63u8]
    var view : [u8] = data
    var digest = crypto::safe::sha256(view)
    if(!crypto_safe_hex(&raw mut digest[0], 32).equals_with_len(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", 64)) {
        env.error("SHA-256(abc) is wrong")
    }
}

@test
func test_crypto_safe_sha256_empty(env : &mut TestEnv) {
    var empty = vector<u8>()
    var view : [u8] = &empty
    var digest = crypto::safe::sha256(view)
    if(!crypto_safe_hex(&raw mut digest[0], 32).equals_with_len(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 64)) {
        env.error("SHA-256(empty) is wrong")
    }
}

@test
func test_crypto_safe_sha384_abc(env : &mut TestEnv) {
    var data : [3]u8 = [0x61u8, 0x62u8, 0x63u8]
    var view : [u8] = data
    var digest = crypto::safe::sha384(view)
    if(!crypto_safe_hex(&raw mut digest[0], 48).equals_with_len(
        "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7", 96)) {
        env.error("SHA-384(abc) is wrong")
    }
}

@test
func test_crypto_safe_sha512_abc(env : &mut TestEnv) {
    var data : [3]u8 = [0x61u8, 0x62u8, 0x63u8]
    var view : [u8] = data
    var digest = crypto::safe::sha512(view)
    if(!crypto_safe_hex(&raw mut digest[0], 64).equals_with_len(
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f", 128)) {
        env.error("SHA-512(abc) is wrong")
    }
}

@test
func test_crypto_safe_md5_abc(env : &mut TestEnv) {
    var data : [3]u8 = [0x61u8, 0x62u8, 0x63u8]
    var view : [u8] = data
    var digest = crypto::safe::md5(view)
    if(!crypto_safe_hex(&raw mut digest[0], 16).equals_with_len(
        "900150983cd24fb0d6963f7d28e17f72", 32)) {
        env.error("MD5(abc) is wrong")
    }
}

@test
func test_crypto_safe_large_hash(env : &mut TestEnv) {
    var data = vector<u8>()
    var i : size_t = 0
    while(i < 1000) {
        if(!data.try_push(0u8)) { env.error("test alloc failed"); return }
        i += 1
    }
    var view : [u8] = &data
    var digest = crypto::safe::sha256(view)
    if(!crypto_safe_hex(&raw mut digest[0], 32).equals_with_len(
        "541b3e9daa09b20bf85fa273e5cbd3e80185aa4ec298e765db87742b70138a53", 64)) {
        env.error("SHA-256(1000 zero bytes) is wrong")
    }
    var md5_digest = crypto::safe::md5(view)
    if(!crypto_safe_hex(&raw mut md5_digest[0], 16).equals_with_len(
        "ede3d3b685b4e137ba4cb2521329a75e", 32)) {
        env.error("MD5(1000 zero bytes) is wrong")
    }
}

// ---------------------------------------------------------------------------
// HMAC
// ---------------------------------------------------------------------------

@test
func test_crypto_safe_hmac_vectors(env : &mut TestEnv) {
    var key : [3]u8 = [0x6Bu8, 0x65u8, 0x79u8]
    var kv : [u8] = key
    var msg_view = std::string_view("The quick brown fox jumps over the lazy dog")
    var mv : [u8] = unsafe(std::span<u8>(msg_view.data() as *u8, msg_view.size()))

    var d256 = crypto::safe::hmac_sha256(kv, mv)
    if(!crypto_safe_hex(&raw mut d256[0], 32).equals_with_len(
        "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8", 64)) {
        env.error("HMAC-SHA256 vector is wrong")
    }
    var d384 = crypto::safe::hmac_sha384(kv, mv)
    if(!crypto_safe_hex(&raw mut d384[0], 48).equals_with_len(
        "d7f4727e2c0b39ae0f1e40cc96f60242d5b7801841cea6fc592c5d3e1ae50700582a96cf35e1e554995fe4e03381c237", 96)) {
        env.error("HMAC-SHA384 vector is wrong")
    }
    var dmd5 = crypto::safe::hmac_md5(kv, mv)
    if(!crypto_safe_hex(&raw mut dmd5[0], 16).equals_with_len(
        "80070713463e7749b90c2dc24911e275", 32)) {
        env.error("HMAC-MD5 vector is wrong")
    }
}

@test
func test_crypto_safe_hmac_long_key(env : &mut TestEnv) {
    // A key longer than the 64-byte block is hashed first; verify against the
    // one-shot implementation for equivalence with the documented behavior.
    var key = vector<u8>()
    var i : size_t = 0
    while(i < 100) {
        if(!key.try_push((i + 1) as u8)) { env.error("test alloc failed"); return }
        i += 1
    }
    var data : [1]u8 = [0x78u8]
    var kv : [u8] = &key
    var dv : [u8] = data
    var digest = crypto::safe::hmac_sha256(kv, dv)
    if(!crypto_safe_hex(&raw mut digest[0], 32).equals_with_len(
        "67126009ae2ed2ddebaa8af12475bc4b849aba4663d36357d46664373f561f9c", 64)) {
        env.error("HMAC-SHA256 long-key vector is wrong")
    }
}

// ---------------------------------------------------------------------------
// Constant-time comparison
// ---------------------------------------------------------------------------

@test
func test_crypto_safe_constant_time_equal(env : &mut TestEnv) {
    var a : [4]u8 = [1u8, 2u8, 3u8, 4u8]
    var b : [4]u8 = [1u8, 2u8, 3u8, 4u8]
    var c : [4]u8 = [1u8, 2u8, 3u8, 5u8]
    var short : [3]u8 = [1u8, 2u8, 3u8]
    var va : [u8] = a
    var vb : [u8] = b
    var vc : [u8] = c
    var vs : [u8] = short
    if(!crypto::safe::constant_time_equal(va, vb)) { env.error("equal slices") }
    if(crypto::safe::constant_time_equal(va, vc)) { env.error("differing bytes") }
    if(crypto::safe::constant_time_equal(va, vs)) { env.error("different lengths") }
    var empty = vector<u8>()
    var ve : [u8] = &empty
    if(!crypto::safe::constant_time_equal(ve, ve)) { env.error("empty slices") }
}

// ---------------------------------------------------------------------------
// Streaming hashers
// ---------------------------------------------------------------------------

@test
func test_crypto_safe_streaming_sha256(env : &mut TestEnv) {
    var a : [3]u8 = [0x61u8, 0x62u8, 0x63u8]
    var va : [u8] = a
    var hasher = crypto::safe::Sha256Hasher()
    hasher.update(va)
    var digest = hasher.finish()
    if(!crypto_safe_hex(&raw mut digest[0], 32).equals_with_len(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", 64)) {
        env.error("streaming SHA-256 is wrong")
    }
}

@test
func test_crypto_safe_streaming_chunked(env : &mut TestEnv) {
    // Split the same message into several updates and compare with one-shot.
    var part1 : [1]u8 = [0x61u8]
    var part2 : [1]u8 = [0x62u8]
    var part3 : [1]u8 = [0x63u8]
    var v1 : [u8] = part1
    var v2 : [u8] = part2
    var v3 : [u8] = part3

    var h256 = crypto::safe::Sha256Hasher()
    h256.update(v1)
    h256.update(v2)
    h256.update(v3)
    var d256 = h256.finish()
    if(!crypto_safe_hex(&raw mut d256[0], 32).equals_with_len(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", 64)) {
        env.error("chunked SHA-256 is wrong")
    }

    var h384 = crypto::safe::Sha384Hasher()
    h384.update(v1)
    h384.update(v2)
    h384.update(v3)
    var d384 = h384.finish()
    if(!crypto_safe_hex(&raw mut d384[0], 48).equals_with_len(
        "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7", 96)) {
        env.error("chunked SHA-384 is wrong")
    }

    var h512 = crypto::safe::Sha512Hasher()
    h512.update(v1)
    h512.update(v2)
    h512.update(v3)
    var d512 = h512.finish()
    if(!crypto_safe_hex(&raw mut d512[0], 64).equals_with_len(
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f", 128)) {
        env.error("chunked SHA-512 is wrong")
    }

    var hmd5 = crypto::safe::Md5Hasher()
    hmd5.update(v1)
    hmd5.update(v2)
    hmd5.update(v3)
    var dmd5 = hmd5.finish()
    if(!crypto_safe_hex(&raw mut dmd5[0], 16).equals_with_len(
        "900150983cd24fb0d6963f7d28e17f72", 32)) {
        env.error("chunked MD5 is wrong")
    }
}

@test
func test_crypto_safe_hasher_reset(env : &mut TestEnv) {
    var a : [3]u8 = [0x61u8, 0x62u8, 0x63u8]
    var va : [u8] = a
    var hasher = crypto::safe::Sha256Hasher()
    hasher.update(va)
    var _ = hasher.finish()
    hasher.reset()
    hasher.update(va)
    var digest = hasher.finish()
    if(!crypto_safe_hex(&raw mut digest[0], 32).equals_with_len(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", 64)) {
        env.error("SHA-256 after reset is wrong")
    }
}

// ---------------------------------------------------------------------------
// Base64 wrappers
// ---------------------------------------------------------------------------

@test
func test_crypto_safe_base64_roundtrip(env : &mut TestEnv) {
    var data : [5]u8 = [0x48u8, 0x65u8, 0x6Cu8, 0x6Cu8, 0x6Fu8]
    var view : [u8] = data
    var encoded = crypto::safe::base64_encode(view)
    if(encoded is Result.Err) { env.error("crypto base64_encode failed"); return }
    var Ok(text) = encoded else unsafe { unreachable }
    if(!text.equals_with_len("SGVsbG8=", 8)) { env.error("crypto base64 content"); return }

    var decoded = crypto::safe::base64_decode(text.to_view())
    if(decoded is Result.Err) { env.error("crypto base64_decode failed"); return }
    var Ok(bytes) = decoded else unsafe { unreachable }
    if(!crypto_safe_bytes_eq(&bytes, &raw data[0], 5)) {
        env.error("crypto base64 roundtrip mismatch")
    }
}

@test
func test_crypto_safe_base64_invalid(env : &mut TestEnv) {
    if(crypto::safe::base64_decode("A") is Result.Ok) {
        env.error("crypto base64 should reject length 1")
    }
    if(crypto::safe::base64_decode("!!!!") is Result.Ok) {
        env.error("crypto base64 should reject non-alphabet bytes")
    }
    if(crypto::safe::base64_decode("AA=A") is Result.Ok) {
        env.error("crypto base64 should reject interior padding")
    }
    var empty = crypto::safe::base64_decode("")
    if(empty is Result.Err) { env.error("crypto base64 empty decode failed"); return }
    var Ok(bytes) = empty else unsafe { unreachable }
    if(bytes.size() != 0) { env.error("crypto base64 empty should be empty") }
}
