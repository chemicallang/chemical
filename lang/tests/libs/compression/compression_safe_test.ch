using namespace std;

// Focused tests for `compression` — owned-buffer RLE encode/decode with
// fallible allocation and strict malformed-stream rejection.

func compression_safe_bytes_eq(v : &vector<u8>, expected : *u8, n : size_t) : bool {
    if(v.size() != n) { return false }
    var i : size_t = 0
    while(i < n) {
        if(v.at(i) != unsafe(expected[i])) { return false }
        i += 1
    }
    return true
}

func compression_safe_all(v : &vector<u8>, expected : u8) : bool {
    var i : size_t = 0
    while(i < v.size()) {
        if(v.at(i) != expected) { return false }
        i += 1
    }
    return true
}

func compression_safe_roundtrip(env : &mut TestEnv, input : &[u8]) : bool {
    var compressed = compression::compress(input)
    if(compressed is Result.Err) { env.error("compress failed"); return false }
    var Ok(enc) = compressed else unsafe { unreachable }
    if(enc.size() == 0) { env.error("compress produced empty output"); return false }
    if(enc.at(enc.size() - 1) != 0) { env.error("compress output missing terminator"); return false }
    var enc_view : [u8] = &enc
    var decompressed = compression::decompress(enc_view)
    if(decompressed is Result.Err) { env.error("decompress failed"); return false }
    var Ok(dec) = decompressed else unsafe { unreachable }
    if(dec.size() != input.size()) { env.error("roundtrip length mismatch"); return false }
    if(!compression_safe_bytes_eq(&dec, input.data() as *mut u8, input.size())) {
        env.error("roundtrip bytes mismatch"); return false
    }
    return true
}

@test
func test_compression_safe_roundtrip_simple(env : &mut TestEnv) {
    var input : [10]u8 = [0x41u8, 0x41u8, 0x41u8, 0x41u8, 0x42u8, 0x42u8, 0x43u8, 0x44u8, 0x44u8, 0x44u8]
    var view : [u8] = input
    var _ = compression_safe_roundtrip(env, view)
}

@test
func test_compression_safe_roundtrip_single(env : &mut TestEnv) {
    var input : [1]u8 = [0x42u8]
    var view : [u8] = input
    var _ = compression_safe_roundtrip(env, view)
}

@test
func test_compression_safe_roundtrip_max_run(env : &mut TestEnv) {
    var input = vector<u8>()
    var i : size_t = 0
    while(i < 255) {
        if(!input.try_push(0xABu8)) { env.error("test alloc failed"); return }
        i += 1
    }
    var view : [u8] = &input
    var _ = compression_safe_roundtrip(env, view)
}

@test
func test_compression_safe_roundtrip_split_run(env : &mut TestEnv) {
    // 300 identical bytes exceed one u8 run counter, so the encoder must split.
    var input = vector<u8>()
    var i : size_t = 0
    while(i < 300) {
        if(!input.try_push(0x42u8)) { env.error("test alloc failed"); return }
        i += 1
    }
    var view : [u8] = &input
    var _ = compression_safe_roundtrip(env, view)

    var compressed = compression::compress(view)
    if(compressed is Result.Err) { env.error("compress split-run failed"); return }
    var Ok(enc) = compressed else unsafe { unreachable }
    // 255 + 45 -> two runs of two bytes, plus the terminator.
    if(enc.size() != 5) { env.error("split-run output should be 5 bytes") }
}

@test
func test_compression_safe_roundtrip_alternating(env : &mut TestEnv) {
    var input : [6]u8 = [0x41u8, 0x42u8, 0x41u8, 0x42u8, 0x41u8, 0x42u8]
    var view : [u8] = input
    var _ = compression_safe_roundtrip(env, view)
}

@test
func test_compression_safe_roundtrip_empty(env : &mut TestEnv) {
    var empty = vector<u8>()
    var view : [u8] = &empty
    var compressed = compression::compress(view)
    if(compressed is Result.Err) { env.error("compress empty failed"); return }
    var Ok(enc) = compressed else unsafe { unreachable }
    if(enc.size() != 1 || enc.at(0) != 0) {
        env.error("compress empty should be a single terminator")
        return
    }
    var enc_view : [u8] = &enc
    var decompressed = compression::decompress(enc_view)
    if(decompressed is Result.Err) { env.error("decompress empty failed"); return }
    var Ok(dec) = decompressed else unsafe { unreachable }
    if(dec.size() != 0) { env.error("decompress empty should be empty") }
}

@test
func test_compression_safe_roundtrip_all_zeros(env : &mut TestEnv) {
    var input = vector<u8>()
    var i : size_t = 0
    while(i < 50) {
        if(!input.try_push(0u8)) { env.error("test alloc failed"); return }
        i += 1
    }
    var view : [u8] = &input
    var _ = compression_safe_roundtrip(env, view)
}

@test
func test_compression_safe_roundtrip_large(env : &mut TestEnv) {
    var input = vector<u8>()
    var i : size_t = 0
    while(i < 5000) {
        // Runs of varying length plus noise exercise run splitting.
        var b : u8
        if((i / 37) % 2 == 0) {
            b = (i % 251) as u8
        } else {
            b = (i / 37) as u8
        }
        if(!input.try_push(b)) { env.error("test alloc failed"); return }
        i += 1
    }
    var view : [u8] = &input
    var _ = compression_safe_roundtrip(env, view)
}

@test
func test_compression_safe_decompress_invalid(env : &mut TestEnv) {
    // Truncated pair: a count with no following byte.
    var truncated : [1]u8 = [5u8]
    var vt : [u8] = truncated
    if(compression::decompress(vt) is Result.Ok) {
        env.error("decompress should reject a truncated pair")
    }

    // Missing terminator.
    var unterminated : [2]u8 = [2u8, 0x41u8]
    var vu : [u8] = unterminated
    if(compression::decompress(vu) is Result.Ok) {
        env.error("decompress should require a terminator")
    }

    // Trailing bytes after the terminator.
    var trailer : [4]u8 = [0u8, 1u8, 2u8, 3u8]
    var vtr : [u8] = trailer
    if(compression::decompress(vtr) is Result.Ok) {
        env.error("decompress should reject trailing bytes")
    }

    // Empty input has no terminator.
    var empty = vector<u8>()
    var ve : [u8] = &empty
    if(compression::decompress(ve) is Result.Ok) {
        env.error("decompress should reject empty input")
    }
}

@test
func test_compression_safe_decompress_terminal(env : &mut TestEnv) {
    var terminal : [1]u8 = [0u8]
    var view : [u8] = terminal
    var r = compression::decompress(view)
    if(r is Result.Err) { env.error("decompress terminal failed"); return }
    var Ok(out) = r else unsafe { unreachable }
    if(out.size() != 0) { env.error("decompress terminal should be empty") }
}

@test
func test_compression_safe_all_zero_runs(env : &mut TestEnv) {
    // A run of zero-valued bytes is data, not a terminator.
    var input : [3]u8 = [3u8, 0u8, 0u8]
    var view : [u8] = input
    var r = compression::decompress(view)
    if(r is Result.Err) { env.error("decompress zero run failed"); return }
    var Ok(out) = r else unsafe { unreachable }
    if(out.size() != 3 || !compression_safe_all(&out, 0u8)) {
        env.error("decompress zero run content")
    }
}
