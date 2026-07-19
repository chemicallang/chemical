// base64 — Base64 encoding and decoding (RFC 4648).

public namespace crypto {

using std::Result;
using std::string_view;
using std::string;
using std::vector;

const BASE64_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

func base64_val(c : char) : int {
    if(c >= 'A' && c <= 'Z') { return (c as int) - ('A' as int); }
    if(c >= 'a' && c <= 'z') { return (c as int) - ('a' as int) + 26; }
    if(c >= '0' && c <= '9') { return (c as int) - ('0' as int) + 52; }
    if(c == '+') { return 62; }
    if(c == '/') { return 63; }
    return -1;
}

public func base64_encode(data : *u8, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, CryptoError> {
    var encoded_len = ((data_len + 2) / 3) * 4;
    if(encoded_len + 1 > out_len) { return Result.Err(CryptoError.BufferTooSmall(encoded_len + 1)); }
    var i : size_t = 0;
    var pos : size_t = 0;
    while(i < data_len) {
        var b0 = data[i]; i += 1;
        var b1 : u8 = 0; if(i < data_len) { b1 = data[i]; i += 1; }
        var b2 : u8 = 0; if(i < data_len) { b2 = data[i]; i += 1; }
        var triple = (b0 as u32) << 16 | (b1 as u32) << 8 | (b2 as u32);
        out[pos]     = BASE64_TABLE[((triple >> 18) & 0x3F) as size_t];
        out[pos + 1] = BASE64_TABLE[((triple >> 12) & 0x3F) as size_t];
        out[pos + 2] = BASE64_TABLE[((triple >> 6) & 0x3F) as size_t];
        out[pos + 3] = BASE64_TABLE[(triple & 0x3F) as size_t];
        var remaining = data_len - (i - 3);
        if(remaining == 0) { /* nothing */ }
        else if(remaining == 2) { out[pos + 2] = '='; out[pos + 3] = '='; }
        else if(remaining == 1) { out[pos + 3] = '='; }
        pos += 4;
    }
    out[pos] = 0;
    return Result.Ok(encoded_len);
}

public func base64_decode(b64 : *char, b64_len : size_t, out : *mut u8, out_len : size_t) : Result<size_t, CryptoError> {
    var effective_len = b64_len;
    if(effective_len > 0 && b64[effective_len - 1] == '=') { effective_len -= 1; }
    if(effective_len > 0 && b64[effective_len - 1] == '=') { effective_len -= 1; }
    var decoded_len = (effective_len / 4) * 3;
    if(effective_len % 4 == 1) { return Result.Err(CryptoError.InvalidInput()); }
    if(effective_len % 4 == 2) { decoded_len += 1; }
    if(effective_len % 4 == 3) { decoded_len += 2; }
    if(decoded_len > out_len) { return Result.Err(CryptoError.BufferTooSmall(decoded_len)); }
    var i : size_t = 0;
    var pos : size_t = 0;
    while(i < effective_len) {
        var c0 = base64_val(b64[i]); i += 1;
        var c1 = base64_val(b64[i]); i += 1;
        var c2 : int = 0; if(i < effective_len) { c2 = base64_val(b64[i]); i += 1; }
        var c3 : int = 0; if(i < effective_len) { c3 = base64_val(b64[i]); i += 1; }
        if(c0 < 0 || c1 < 0) { return Result.Err(CryptoError.InvalidInput()); }
        var triple = ((c0 as u32) << 18) | ((c1 as u32) << 12);
        var remaining = effective_len - (i - 4);
        if(remaining >= 2) {
            if(c2 < 0) { return Result.Err(CryptoError.InvalidInput()); }
            triple |= (c2 as u32) << 6;
        }
        if(remaining >= 3) {
            if(c3 < 0) { return Result.Err(CryptoError.InvalidInput()); }
            triple |= c3 as u32;
        }
        out[pos] = ((triple >> 16) & 0xFF) as u8; pos += 1;
        if(remaining >= 2) { out[pos] = ((triple >> 8) & 0xFF) as u8; pos += 1; }
        if(remaining >= 3) { out[pos] = (triple & 0xFF) as u8; pos += 1; }
    }
    return Result.Ok(decoded_len);
}

} // end namespace crypto
