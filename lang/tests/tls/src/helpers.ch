// Shared helpers for TLS integration tests

func test_nibble_to_hex(n : uint) : char {
    if(n < 10) { return (48 as char) + (n as char) }
    else { return (87 as char) + (n as char) }
}

func test_bytes_to_hex(data : *u8, len : size_t, hex_out : *mut char) {
    var i : size_t = 0
    while(i < len) {
        hex_out[i*2] = test_nibble_to_hex(((data[i] as uint)>>4)&0xF)
        hex_out[i*2+1] = test_nibble_to_hex((data[i] as uint)&0xF)
        i += 1
    }
    hex_out[len*2] = 0
}

func test_bytes_eq(a : *u8, b : *u8, len : size_t) : bool {
    var i : size_t = 0
    while(i < len) { if(a[i] != b[i]) { return false }; i += 1 }
    return true
}

func test_write_file(path : *char, data : *u8, len : size_t) : bool {
    var f = fopen(path, "wb\0" as *char)
    if(f == null) { return false } else {}
    fwrite(data as *mut void, 1 as size_t, len, f)
    fclose(f)
    return true
}

func test_read_file(path : *char, buf : *mut u8, max_len : size_t) : size_t {
    var f = fopen(path, "rb\0" as *char)
    if(f == null) { return 0 } else {}
    var total : size_t = 0
    while(total < max_len) {
        var n = fread((buf + total) as *mut void, 1 as size_t, max_len - total, f)
        if(n <= 0) { break } else {}
        total += n
    }
    fclose(f)
    return total
}

func test_hex_char_val(c : char) : uint {
    var v : uint = c as uint
    if(v >= 48u && v <= 57u) { return v - 48u }
    else if(v >= 65u && v <= 70u) { return v - 55u }
    else if(v >= 97u && v <= 102u) { return v - 87u }
    else { return 0 }
}

func test_hex_pair_byte(hi : char, lo : char) : u8 {
    return ((test_hex_char_val(hi) << 4) | test_hex_char_val(lo)) as u8
}
