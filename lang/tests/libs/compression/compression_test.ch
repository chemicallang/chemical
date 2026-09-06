using namespace std;

// ---------------------------------------------------------------------------

@test
func test_rle_roundtrip_simple(env : &mut TestEnv) {
    var input : [10]u8 = [ 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x44 ];
    var compressed : [64]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 10, &raw mut compressed[0], &raw mut comp_len, 64);
    if(r is Result.Err) { env.error("compress failed"); return; }
    var decompressed : [64]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
    if(r2 is Result.Err) { env.error("decompress failed"); return; }
    if(dec_len != 10) { env.error("decompressed length wrong"); return; }
    var i : size_t = 0;
    while(i < 10) {
        if(decompressed[i] != input[i]) { env.error("data mismatch after roundtrip"); return; }
        i += 1;
    }
}

@test
func test_rle_roundtrip_single_byte(env : &mut TestEnv) {
    var input : [1]u8 = [ 0x42 ];
    var compressed : [64]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 1, &raw mut compressed[0], &raw mut comp_len, 64);
    if(r is Result.Err) { env.error("compress failed"); return; }
    var decompressed : [64]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
    if(r2 is Result.Err) { env.error("decompress failed"); return; }
    if(!(dec_len == 1 && decompressed[0] == 0x42)) {
        env.error("single byte roundtrip failed");
    }
}

@test
func test_rle_roundtrip_all_same(env : &mut TestEnv) {
    var input : [20]u8;
    var i : size_t = 0;
    while(i < 20) { input[i] = 0xFF; i += 1; }
    var compressed : [64]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 20, &raw mut compressed[0], &raw mut comp_len, 64);
    if(r is Result.Err) { env.error("compress failed"); return; }
    var decompressed : [64]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
    if(r2 is Result.Err) { env.error("decompress failed"); return; }
    if(dec_len != 20) { env.error("decompressed length wrong"); return; }
    var j : size_t = 0;
    while(j < 20) {
        if(decompressed[j] != 0xFF) { env.error("data mismatch after roundtrip"); return; }
        j += 1;
    }
}

@test
func test_rle_roundtrip_alternating(env : &mut TestEnv) {
    var input : [6]u8 = [ 0x41, 0x42, 0x41, 0x42, 0x41, 0x42 ];
    var compressed : [64]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 6, &raw mut compressed[0], &raw mut comp_len, 64);
    if(r is Result.Err) { env.error("compress failed"); return; }
    var decompressed : [64]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
    if(r2 is Result.Err) { env.error("decompress failed"); return; }
    if(dec_len != 6) { env.error("decompressed length wrong"); return; }
    var k : size_t = 0;
    while(k < 6) {
        if(decompressed[k] != input[k]) { env.error("alternating roundtrip failed"); return; }
        k += 1;
    }
}

@test
func test_rle_decompress_empty(env : &mut TestEnv) {
    var compressed : [8]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0 ];
    var decompressed : [8]u8;
    var dec_len : size_t = 0;
    var r = compression::decompress(&raw compressed[0], 1, &raw mut decompressed[0], &raw mut dec_len, 8);
    if(r is Result.Err) { env.error("decompress failed"); return; }
    var Ok(len) = r else unreachable;
    if(len != 0) {
        env.error("RLE decompress of empty should return len 0");
    }
}
