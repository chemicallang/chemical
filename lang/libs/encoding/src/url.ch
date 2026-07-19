// url — URL encoding (percent-encoding) and decoding utilities.

public namespace encoding {

using std::Result;

// ---------------------------------------------------------------------------
// Percent-encoding (URL encoding)
// ---------------------------------------------------------------------------

/// Check if a character is unreserved (doesn't need percent-encoding).
func is_unreserved(c : char) : bool {
    // unreserved = ALPHA / DIGIT / '-' / '.' / '_' / '~'
    if(c >= 'a' && c <= 'z') { return true; }
    if(c >= 'A' && c <= 'Z') { return true; }
    if(c >= '0' && c <= '9') { return true; }
    if(c == '-' || c == '.' || c == '_' || c == '~') { return true; }
    return false;
}

/// URL-encode a string (percent-encoding).
/// Encodes everything except unreserved characters.
/// out_len should be at least 3 * input_len + 1.
public func url_encode(data : *char, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
    var pos : size_t = 0;
    var i : size_t = 0;
    while(i < data_len) {
        var c = data[i];
        if(is_unreserved(c)) {
            if(pos + 1 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 2)); }
            out[pos] = c;
            pos += 1;
        } else {
            if(pos + 3 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 4)); }
            out[pos] = '%';
            out[pos+1] = HEX_DIGITS_UPPER[((c as u8) >> 4) as size_t] as char;
            out[pos+2] = HEX_DIGITS_UPPER[((c as u8) & 0x0F) as size_t] as char;
            pos += 3;
        }
        i += 1;
    }
    out[pos] = 0;
    return Result.Ok(pos);
}

/// URL-encode only the query component (encodes spaces as '+').
public func url_encode_query(data : *char, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
    var pos : size_t = 0;
    var i : size_t = 0;
    while(i < data_len) {
        var c = data[i];
        if(c == ' ') {
            if(pos + 1 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 2)); }
            out[pos] = '+';
            pos += 1;
        } else if(is_unreserved(c)) {
            if(pos + 1 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 2)); }
            out[pos] = c;
            pos += 1;
        } else {
            if(pos + 3 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 4)); }
            out[pos] = '%';
            out[pos+1] = HEX_DIGITS_UPPER[((c as u8) >> 4) as size_t] as char;
            out[pos+2] = HEX_DIGITS_UPPER[((c as u8) & 0x0F) as size_t] as char;
            pos += 3;
        }
        i += 1;
    }
    out[pos] = 0;
    return Result.Ok(pos);
}

/// URL-decode a percent-encoded string (also decodes '+' as space).
/// out_len should be at least input_len + 1.
public func url_decode(data : *char, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
    var pos : size_t = 0;
    var i : size_t = 0;
    while(i < data_len) {
        var c = data[i];
        if(c == '%' && i + 2 < data_len) {
            var high = hex_to_nibble(data[i+1]);
            var low = hex_to_nibble(data[i+2]);
            if(high >= 0 && low >= 0) {
                if(pos + 1 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 2)); }
                out[pos] = ((high as u8) << 4 | (low as u8)) as char;
                pos += 1;
                i += 3;
                continue;
            }
        }
        if(c == '+') {
            if(pos + 1 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 2)); }
            out[pos] = ' ';
            pos += 1;
        } else {
            if(pos + 1 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 2)); }
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    out[pos] = 0;
    return Result.Ok(pos);
}

} // end namespace encoding
