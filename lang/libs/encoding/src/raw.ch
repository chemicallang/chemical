// raw — legacy pointer/out-buffer encoding entry points.
//
// This file is the single declaration of `encoding::raw` in the module; nested
// namespaces cannot be reopened across files. The canonical owned-buffer API
// lives at the top-level `encoding` namespace (see `safe.ch`).

public namespace encoding {

public namespace raw {

using std::Result;
using std::string_view;
using std::string;
using std::vector;

// Encode raw bytes to base64 string.
// Caller provides output buffer. Returns encoded length or error.
public func base64_encode(data : *u8, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
    var r = crypto::raw::base64_encode(data, data_len, out, out_len)
    if(r is Result.Err) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unsafe { unreachable }
    return Result.Ok(n)
}

// Decode base64 string to raw bytes.
// Caller provides output buffer. Returns decoded length or error.
public func base64_decode(b64 : *char, b64_len : size_t, out : *mut u8, out_len : size_t) : Result<size_t, EncodingError> {
    var r = crypto::raw::base64_decode(b64, b64_len, out, out_len)
    if(r is Result.Err) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unsafe { unreachable }
    return Result.Ok(n)
}

// Encode raw bytes to base64 string (legacy heap convenience).
// Allocates the output string internally.
public func base64_encode_to_string(data : *u8, data_len : size_t) : Result<string, EncodingError> {
    var encoded_len = ((data_len + 2) / 3) * 4
    var buf = unsafe(malloc(encoded_len + 1)) as *mut char
    if(buf == null) { return Result.Err(EncodingError.InvalidInput()) }
    var r = crypto::raw::base64_encode(data, data_len, buf, encoded_len + 1)
    if(r is Result.Err) {
        unsafe { free(buf) }
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unsafe { unreachable }
    var s = unsafe(string.constructor(buf as *char, n))
    unsafe { free(buf) }
    return Result.Ok(s)
}

// Decode base64 string to vector<u8> (legacy heap convenience).
// Allocates the output vector internally.
public func base64_decode_to_vec(b64 : string_view) : Result<vector<u8>, EncodingError> {
    if(b64.size() == 0) { return Result.Ok(vector<u8>()) }
    // Estimate decoded length
    var eff_len = b64.size()
    if(b64.get(eff_len - 1) == '=') { eff_len -= 1 }
    if(eff_len > 0 && b64.get(eff_len - 1) == '=') { eff_len -= 1 }
    var decoded_len = (eff_len / 4) * 3
    if(b64.size() % 4 == 2) { decoded_len += 1 }
    else if(b64.size() % 4 == 3) { decoded_len += 2 }

    var out = vector<u8>()
    out.resize(decoded_len)
    var r = crypto::raw::base64_decode(b64.data(), b64.size(), out.data() as *mut u8, decoded_len)
    if(r is Result.Err) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unsafe { unreachable }
    if(n < decoded_len) { out.resize(n) }
    return Result.Ok(out)
}

public func hex_encode(data : *u8, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
        unsafe {
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
    }

public func hex_encode_upper(data : *u8, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
        unsafe {
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
    }

public func hex_decode(hex : *char, out : *mut u8, out_len : size_t) : Result<size_t, EncodingError> {
        unsafe {
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
    }

/// URL-encode a string (percent-encoding).
/// Encodes everything except unreserved characters.
/// out_len should be at least 3 * input_len + 1.
public func url_encode(data : *char, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
        unsafe {
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
    }

/// URL-encode only the query component (encodes spaces as '+').
public func url_encode_query(data : *char, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
        unsafe {
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
    }

/// URL-decode a percent-encoded string (also decodes '+' as space).
/// out_len should be at least input_len + 1.
public func url_decode(data : *char, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
        unsafe {
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
    }

// ---------------------------------------------------------------------------
// UTF-8 validation
// ---------------------------------------------------------------------------

public func utf8_is_valid(data : *char, data_len : size_t) : bool {
        unsafe {
        var i : size_t = 0;
        while(i < data_len) {
            var c = data[i] as u8;
            if(c < 0x80) { i += 1; continue; }
            var seq_len : u8 = 0;
            var min_code : u32 = 0;
            if((c & 0xE0) == 0xC0) { seq_len = 2; min_code = 0x80; }
            else if((c & 0xF0) == 0xE0) { seq_len = 3; min_code = 0x800; }
            else if((c & 0xF8) == 0xF0) { seq_len = 4; min_code = 0x10000; }
            else { return false; }
            if(i + (seq_len as size_t) > data_len) { return false; }
            if(seq_len == 2) {
                var c2 = data[i + 1] as u8;
                if((c2 & 0xC0) != 0x80) { return false; }
                var code = (((c & 0x1F) as u32) << 6) | (c2 & 0x3F) as u32;
                if(code < min_code) { return false; }
            } else if(seq_len == 3) {
                var c2 = data[i + 1] as u8; var c3 = data[i + 2] as u8;
                if((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) { return false; }
                var code = (((c & 0x0F) as u32) << 12) | (((c2 & 0x3F) as u32) << 6) | ((c3 & 0x3F) as u32);
                if(code < min_code || (code >= 0xD800u32 && code <= 0xDFFFu32)) { return false; }
            } else if(seq_len == 4) {
                var c2 = data[i + 1] as u8; var c3 = data[i + 2] as u8; var c4 = data[i + 3] as u8;
                if((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) { return false; }
                var code = (((c & 0x07) as u32) << 18) | (((c2 & 0x3F) as u32) << 12) | (((c3 & 0x3F) as u32) << 6) | ((c4 & 0x3F) as u32);
                if(code < min_code || code > 0x10FFFFu32) { return false; }
            }
            i += seq_len as size_t;
        }
        return true;

        }
    }

// ---------------------------------------------------------------------------
// UTF-8 to UTF-16 conversion
// ---------------------------------------------------------------------------

public func utf8_to_utf16(in_utf8 : *char, out_w : *mut u16, out_w_len : size_t) : Result<size_t, EncodingError> {
        unsafe {
        var i : size_t = 0;
        var wpos : size_t = 0;
        while(in_utf8[i] != 0) {
            var c = in_utf8[i] as u8;
            if(c < 0x80) {
                if(wpos + 1 >= out_w_len) { return Result.Err(EncodingError.BufferTooSmall(wpos + 2)); }
                out_w[wpos] = c as u16; wpos += 1; i += 1; continue;
            }
            if((c & 0xE0) == 0xC0) {
                if(in_utf8[i+1] == 0) { return Result.Err(EncodingError.InvalidInput()); }
                var c2 = in_utf8[i+1] as u8;
                if((c2 & 0xC0) != 0x80) { return Result.Err(EncodingError.InvalidInput()); }
                var code = (((c & 0x1F) as u32) << 6) | ((c2 & 0x3F) as u32);
                if(code < 0x80u32) { return Result.Err(EncodingError.InvalidInput()); }
                if(wpos + 1 >= out_w_len) { return Result.Err(EncodingError.BufferTooSmall(wpos + 2)); }
                out_w[wpos] = code as u16; wpos += 1; i += 2; continue;
            }
            if((c & 0xF0) == 0xE0) {
                if(in_utf8[i+1] == 0 || in_utf8[i+2] == 0) { return Result.Err(EncodingError.InvalidInput()); }
                var c2 = in_utf8[i+1] as u8; var c3 = in_utf8[i+2] as u8;
                if((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) { return Result.Err(EncodingError.InvalidInput()); }
                var code = (((c & 0x0F) as u32) << 12) | (((c2 & 0x3F) as u32) << 6) | ((c3 & 0x3F) as u32);
                if(code < 0x800u32) { return Result.Err(EncodingError.InvalidInput()); }
                if(code >= 0xD800u32 && code <= 0xDFFFu32) { return Result.Err(EncodingError.InvalidInput()); }
                if(wpos + 1 >= out_w_len) { return Result.Err(EncodingError.BufferTooSmall(wpos + 2)); }
                out_w[wpos] = code as u16; wpos += 1; i += 3; continue;
            }
            if((c & 0xF8) == 0xF0) {
                if(in_utf8[i+1] == 0 || in_utf8[i+2] == 0 || in_utf8[i+3] == 0) { return Result.Err(EncodingError.InvalidInput()); }
                var c2 = in_utf8[i+1] as u8; var c3 = in_utf8[i+2] as u8; var c4 = in_utf8[i+3] as u8;
                if((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) { return Result.Err(EncodingError.InvalidInput()); }
                var code = (((c & 0x07) as u32) << 18) | (((c2 & 0x3F) as u32) << 12) | (((c3 & 0x3F) as u32) << 6) | ((c4 & 0x3F) as u32);
                if(code < 0x10000u32 || code > 0x10FFFFu32) { return Result.Err(EncodingError.InvalidInput()); }
                code -= 0x10000u32;
                var high = 0xD800u16 + ((code >> 10) as u16);
                var low  = 0xDC00u16 + ((code & 0x3FFu32) as u16);
                if(wpos + 2 >= out_w_len) { return Result.Err(EncodingError.BufferTooSmall(wpos + 3)); }
                out_w[wpos] = high; wpos += 1;
                out_w[wpos] = low; wpos += 1;
                i += 4; continue;
            }
            return Result.Err(EncodingError.InvalidInput());
        }
        if(wpos >= out_w_len) { return Result.Err(EncodingError.BufferTooSmall(wpos + 1)); }
        out_w[wpos] = 0;
        return Result.Ok(wpos);

        }
    }

// ---------------------------------------------------------------------------
// UTF-16 to UTF-8 conversion
// ---------------------------------------------------------------------------

public func utf16_to_utf8(in_w : *u16, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
        unsafe {
        var i : size_t = 0;
        var pos : size_t = 0;
        while(true) {
            var w = in_w[i];
            if(w == 0) { break; }
            if(w < 0x80) {
                if(pos + 1 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 2)); }
                out[pos] = w as char; pos += 1;
            } else if(w < 0x800) {
                if(pos + 2 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 3)); }
                out[pos] = (0xC0 | ((w >> 6) & 0x1F)) as char;
                out[pos+1] = (0x80 | (w & 0x3F)) as char;
                pos += 2;
            } else if(w >= 0xD800 && w <= 0xDBFF) {
                var w2 = in_w[i+1];
                if(w2 == 0 || w2 < 0xDC00 || w2 > 0xDFFF) { return Result.Err(EncodingError.InvalidInput()); }
                var code : u32 = 0x10000u32 + ((((w & 0x3FF) as u32) << 10) | ((w2 & 0x3FF) as u32));
                if(pos + 4 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 5)); }
                out[pos]   = (0xF0 | ((code >> 18) & 0x07)) as char;
                out[pos+1] = (0x80 | ((code >> 12) & 0x3F)) as char;
                out[pos+2] = (0x80 | ((code >> 6) & 0x3F)) as char;
                out[pos+3] = (0x80 | (code & 0x3F)) as char;
                pos += 4;
                i += 1;
            } else {
                if(pos + 3 >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 4)); }
                out[pos]   = (0xE0 | ((w >> 12) & 0x0F)) as char;
                out[pos+1] = (0x80 | ((w >> 6) & 0x3F)) as char;
                out[pos+2] = (0x80 | (w & 0x3F)) as char;
                pos += 3;
            }
            i += 1;
        }
        if(pos >= out_len) { return Result.Err(EncodingError.BufferTooSmall(pos + 1)); }
        out[pos] = 0;
        return Result.Ok(pos);

        }
    }

// ---------------------------------------------------------------------------
// UTF-8 helper functions
// ---------------------------------------------------------------------------


public func utf8_decode(data : *char, data_len : size_t, out_codepoint : *mut int, out_bytes : *mut size_t) : bool {
        unsafe {
        if(data_len == 0) { out_bytes[0] = 0; out_codepoint[0] = 0; return false; }
        var c = data[0] as u8;
        if(c < 0x80) { out_codepoint[0] = c as int; out_bytes[0] = 1; return true; }
        var seq_len = encoding::utf8_char_len(c);
        if(seq_len > data_len) { out_bytes[0] = data_len; out_codepoint[0] = -1; return false; }
        if(seq_len == 2) {
            var c2 = data[1] as u8;
            if((c2 & 0xC0) != 0x80) { out_bytes[0] = 1; out_codepoint[0] = -1; return false; }
            var code = (((c & 0x1F) as int) << 6) | ((c2 & 0x3F) as int);
            if(code < 0x80) { out_bytes[0] = 1; out_codepoint[0] = -1; return false; }
            out_codepoint[0] = code; out_bytes[0] = 2; return true;
        }
        if(seq_len == 3) {
            var c2 = data[1] as u8; var c3 = data[2] as u8;
            if((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) { out_bytes[0] = 1; out_codepoint[0] = -1; return false; }
            var code = (((c & 0x0F) as int) << 12) | (((c2 & 0x3F) as int) << 6) | ((c3 & 0x3F) as int);
            if(code < 0x800 || (code >= 0xD800 && code <= 0xDFFF)) { out_bytes[0] = 1; out_codepoint[0] = -1; return false; }
            out_codepoint[0] = code; out_bytes[0] = 3; return true;
        }
        if(seq_len == 4) {
            var c2 = data[1] as u8; var c3 = data[2] as u8; var c4 = data[3] as u8;
            if((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) { out_bytes[0] = 1; out_codepoint[0] = -1; return false; }
            var code = (((c & 0x07) as int) << 18) | (((c2 & 0x3F) as int) << 12) | (((c3 & 0x3F) as int) << 6) | ((c4 & 0x3F) as int);
            if(code < 0x10000 || code > 0x10FFFF) { out_bytes[0] = 1; out_codepoint[0] = -1; return false; }
            out_codepoint[0] = code; out_bytes[0] = 4; return true;
        }
        out_bytes[0] = 1; out_codepoint[0] = -1; return false;

        }
    }

} // namespace raw

} // end namespace encoding
