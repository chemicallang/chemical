// hex — hex encoding and decoding utilities.

public namespace encoding {

using std::Result;

const HEX_DIGITS_LOWER = "0123456789abcdef";
const HEX_DIGITS_UPPER = "0123456789ABCDEF";

public func hex_encode(data : *u8, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
    var needed = data_len * 2;
    if(needed + 1 > out_len) { return Result.Err(EncodingError.BufferTooSmall(needed + 1)); }
    var pos : size_t = 0;
    var i : size_t = 0;
    while(i < data_len) {
        var b = data[i];
        out[pos] = HEX_DIGITS_LOWER[(b >> 4) as size_t] as char;
        pos += 1;
        out[pos] = HEX_DIGITS_LOWER[(b & 0x0F) as size_t] as char;
        pos += 1;
        i += 1;
    }
    out[pos] = 0;
    return Result.Ok(needed);
}

public func hex_encode_upper(data : *u8, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
    var needed = data_len * 2;
    if(needed + 1 > out_len) { return Result.Err(EncodingError.BufferTooSmall(needed + 1)); }
    var pos : size_t = 0;
    var i : size_t = 0;
    while(i < data_len) {
        var b = data[i];
        out[pos] = HEX_DIGITS_UPPER[(b >> 4) as size_t] as char;
        pos += 1;
        out[pos] = HEX_DIGITS_UPPER[(b & 0x0F) as size_t] as char;
        pos += 1;
        i += 1;
    }
    out[pos] = 0;
    return Result.Ok(needed);
}

public func hex_decode(hex : *char, out : *mut u8, out_len : size_t) : Result<size_t, EncodingError> {
    var hex_len : size_t = 0;
    while(hex[hex_len] != 0) { hex_len += 1; }
    if(hex_len % 2 != 0) { return Result.Err(EncodingError.InvalidInput()); }
    var byte_count = hex_len / 2;
    if(byte_count > out_len) { return Result.Err(EncodingError.BufferTooSmall(byte_count)); }
    var pos : size_t = 0;
    var i : size_t = 0;
    while(i < hex_len) {
        var high = hex_to_nibble(hex[i]);
        if(high < 0) { return Result.Err(EncodingError.InvalidInput()); }
        var low = hex_to_nibble(hex[i + 1]);
        if(low < 0) { return Result.Err(EncodingError.InvalidInput()); }
        out[pos] = ((high as u8) << 4) | (low as u8);
        pos += 1;
        i += 2;
    }
    return Result.Ok(byte_count);
}

public func hex_decode_to_vec(hex : std::string_view) : Result<std::vector<u8>, EncodingError> {
    if(hex.size() % 2 != 0) { return Result.Err(EncodingError.InvalidInput()); }
    var byte_count = hex.size() / 2;
    var vec = std::vector<u8>();
    vec.reserve(byte_count);
    var i : size_t = 0;
    while(i < hex.size()) {
        var high = hex_to_nibble(hex.data()[i]);
        if(high < 0) { return Result.Err(EncodingError.InvalidInput()); }
        var low = hex_to_nibble(hex.data()[i + 1]);
        if(low < 0) { return Result.Err(EncodingError.InvalidInput()); }
        vec.push(((high as u8) << 4) | (low as u8));
        i += 2;
    }
    return Result.Ok(vec);
}

func hex_to_nibble(c : char) : int {
    if(c >= '0' && c <= '9') { return (c as int) - ('0' as int); }
    if(c >= 'a' && c <= 'f') { return (c as int) - ('a' as int) + 10; }
    if(c >= 'A' && c <= 'F') { return (c as int) - ('A' as int) + 10; }
    return -1;
}

} // end namespace encoding
