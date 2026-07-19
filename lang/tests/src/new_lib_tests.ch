@test
func test_path_basename() {
    var buf : [4096]char;
    var r = path::basename("/usr/bin/gcc", &raw mut buf[0], 4096);
}

@test
func test_path_dirname() {
    var buf : [4096]char;
    var r = path::dirname("/usr/bin/gcc", &raw mut buf[0], 4096);
}

@test
func test_path_join() {
    var buf : [4096]char;
    var r = path::join("/usr", "bin", &raw mut buf[0], 4096);
}

@test
func test_path_normalize() {
    var buf : [4096]char;
    var r = path::normalize("/usr/bin/../lib/file.txt", &raw mut buf[0], 4096);
}

@test
func test_path_is_absolute() {
    var abs = path::is_absolute("/usr");
    var rel = path::is_absolute("relative");
}

@test
func test_hex_encode() {
    var buf : [128]char;
    var data : [3]u8 = [ 0x48, 0x65, 0x6C ];
    var r = encoding::hex_encode(&raw data[0], 3, &raw mut buf[0], 128);
}

@test
func test_hex_decode() {
    var buf : [64]u8;
    var r = encoding::hex_decode("48656C6C6F", &raw mut buf[0], 64);
}

@test
func test_url_encode() {
    var buf : [128]char;
    var r = encoding::url_encode("hello world", 11, &raw mut buf[0], 128);
}

@test
func test_url_decode() {
    var buf : [128]char;
    var r = encoding::url_decode("hello+world%21", 14, &raw mut buf[0], 128);
}

@test
func test_base64_encode() {
    var buf : [128]char;
    var data : [3]u8 = [ 0x48, 0x65, 0x6C ];
    var r = crypto::base64_encode(&raw data[0], 3, &raw mut buf[0], 128);
}

@test
func test_base64_decode() {
    var buf : [64]u8;
    var r = crypto::base64_decode("SGVsbG8=", 8, &raw mut buf[0], 64);
}

@test
func test_sha256() {
    var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
    var digest : [32]u8;
    crypto::sha256_hash(&raw data[0], 5, &raw mut digest[0]);
}

@test
func test_hmac_sha256() {
    var key : [3]u8 = [ 0x6B, 0x65, 0x79 ];
    var data : [5]u8 = [ 0x48, 0x65, 0x6C, 0x6C, 0x6F ];
    var digest : [32]u8;
    crypto::hmac_sha256(&raw key[0], 3, &raw data[0], 5, &raw mut digest[0]);
}

@test
func test_constant_time_equal() {
    var a : [3]u8 = [ 0x01, 0x02, 0x03 ];
    var b : [3]u8 = [ 0x01, 0x02, 0x03 ];
    var eq = crypto::constant_time_equal(&raw a[0], &raw b[0], 3);
}

@test
func test_rle_compress() {
    var input : [10]u8 = [ 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x44 ];
    var compressed : [64]u8;
    var comp_len : size_t = 0;
    var r = compression::compress(&raw input[0], 10, &raw mut compressed[0], &raw mut comp_len, 64);
}

@test
func test_rle_decompress() {
    var compressed : [8]u8 = [ 3, 0x41, 2, 0x42, 0, 0, 0, 0 ];
    var decompressed : [64]u8;
    var dec_len : size_t = 0;
    var r = compression::decompress(&raw compressed[0], 4, &raw mut decompressed[0], &raw mut dec_len, 64);
}
