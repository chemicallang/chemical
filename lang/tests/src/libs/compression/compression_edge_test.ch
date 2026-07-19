using namespace std;

// ---------------------------------------------------------------------------
// Compression Library Edge-Case Tests
// ---------------------------------------------------------------------------

@test
func test_rle_all_unique(env : &mut TestEnv) {
    var input : [10]u8 = [ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 ];
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
        if(decompressed[i] != input[i]) { env.error("all-unique roundtrip failed"); return; }
        i += 1;
    }
}

@test
func test_rle_max_run(env : &mut TestEnv) {
    // 255 identical bytes = max run length (should be 1 run)
    var input : [255]u8;
    var i : size_t = 0;
    while(i < 255) { input[i] = 0xAB; i += 1; }
    var compressed : [512]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 255, &raw mut compressed[0], &raw mut comp_len, 512);
    if(r is Result.Err) { env.error("compress failed"); return; }
    // Should compress to 2 bytes (run-length=255, byte-value=0xAB)
    if(comp_len < 1) { env.error("compressed data too short"); return; }
    var decompressed : [512]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 512);
    if(r2 is Result.Err) { env.error("decompress failed"); return; }
    if(dec_len != 255) { env.error("decompressed length should be 255"); return; }
    var j : size_t = 0;
    while(j < 255) {
        if(decompressed[j] != 0xAB) { env.error("max-run data mismatch"); return; }
        j += 1;
    }
}

@test
func test_rle_overflow_run(env : &mut TestEnv) {
    // 300 identical bytes > max run of 255 (should split into multiple runs)
    var input : [300]u8;
    var i : size_t = 0;
    while(i < 300) { input[i] = 0x42; i += 1; }
    var compressed : [512]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 300, &raw mut compressed[0], &raw mut comp_len, 512);
    if(r is Result.Err) { env.error("compress failed"); return; }
    var decompressed : [512]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 512);
    if(r2 is Result.Err) { env.error("decompress failed"); return; }
    if(dec_len != 300) { env.error("decompressed length should be 300"); return; }
    var j : size_t = 0;
    while(j < 300) {
        if(decompressed[j] != 0x42) { env.error("overflow-run data mismatch"); return; }
        j += 1;
    }
}

@test
func test_rle_compress_empty(env : &mut TestEnv) {
    var compressed : [8]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw compressed[0] as *u8, 0, &raw mut compressed[0], &raw mut comp_len, 8);
    if(r is Result.Err) { env.error("compress empty failed"); return; }
    // Compressing empty should produce some valid output
    var decompressed : [8]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 8);
    if(r2 is Result.Err) { env.error("decompress of empty-compressed failed"); return; }
    if(dec_len != 0) {
        env.error("compress empty -> decompress should produce 0 bytes");
    }
}

@test
func test_rle_all_zeros(env : &mut TestEnv) {
    // All zeros, runs of 0x00
    var input : [50]u8;
    var i : size_t = 0;
    while(i < 50) { input[i] = 0; i += 1; }
    var compressed : [128]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 50, &raw mut compressed[0], &raw mut comp_len, 128);
    if(r is Result.Err) { env.error("compress failed"); return; }
    var decompressed : [128]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 128);
    if(r2 is Result.Err) { env.error("decompress failed"); return; }
    if(dec_len != 50) { env.error("decompressed length wrong"); return; }
    var j : size_t = 0;
    while(j < 50) {
        if(decompressed[j] != 0) { env.error("all-zeros data mismatch"); return; }
        j += 1;
    }
}

@test
func test_rle_mixed_runs(env : &mut TestEnv) {
    // Pattern: 3xA, 5xB, 2xC, 1xD, 7xE
    var input : [18]u8 = [ 0x41, 0x41, 0x41, 0x42, 0x42, 0x42, 0x42, 0x42,
                          0x43, 0x43, 0x44, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45 ];
    var compressed : [64]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 18, &raw mut compressed[0], &raw mut comp_len, 64);
    if(r is Result.Err) { env.error("compress failed"); return; }
    var decompressed : [64]u8;
    var dec_len : size_t = 0;
    var r2 = compression::decompress(&raw compressed[0], comp_len, &raw mut decompressed[0], &raw mut dec_len, 64);
    if(r2 is Result.Err) { env.error("decompress failed"); return; }
    if(dec_len != 18) { env.error("decompressed length wrong"); return; }
    var k : size_t = 0;
    while(k < 18) {
        if(decompressed[k] != input[k]) { env.error("mixed-runs data mismatch"); return; }
        k += 1;
    }
}

@test
func test_rle_invalid_decompress(env : &mut TestEnv) {
    // Corrupted data: a run with count=0 should signal end
    var corrupted : [4]u8 = [ 5, 0x41, 0, 0x42 ];
    var decompressed : [64]u8;
    var dec_len : size_t = 0;
    var r = compression::decompress(&raw corrupted[0], 4, &raw mut decompressed[0], &raw mut dec_len, 64);
    // A zero-count run signals end, so should return len=5 with just AAA
    if(r is Result.Err) { env.error("decompress with end marker should succeed"); return; }
    var Ok(len) = r else unreachable;
    if(len != 5) {
        env.error("decompress with count=0 should return len=5 (5 A's)");
    }
}
