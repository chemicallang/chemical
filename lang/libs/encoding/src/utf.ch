// utf — UTF-8 and UTF-16 conversion utilities.

public namespace encoding {

using std::Result;

// ---------------------------------------------------------------------------
// UTF-8 validation
// ---------------------------------------------------------------------------

public func utf8_is_valid(data : *char, data_len : size_t) : bool {
    var i : size_t = 0;
    while(i < data_len) {
        var c = data[i] as u8;
        if(c < 0x80) { i += 1; continue; }
        var seq_len : u8 = 0;
        if((c & 0xE0) == 0xC0) { seq_len = 2; }
        else if((c & 0xF0) == 0xE0) { seq_len = 3; }
        else if((c & 0xF8) == 0xF0) { seq_len = 4; }
        else { return false; }
        if(i + (seq_len as size_t) > data_len) { return false; }
        var j : size_t = 1;
        while(j < (seq_len as size_t)) {
            if((data[i + j] as u8 & 0xC0) != 0x80) { return false; }
            j += 1;
        }
        i += seq_len as size_t;
    }
    return true;
}

// ---------------------------------------------------------------------------
// UTF-8 to UTF-16 conversion
// ---------------------------------------------------------------------------

public func utf8_to_utf16(in_utf8 : *char, out_w : *mut u16, out_w_len : size_t) : Result<size_t, EncodingError> {
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

// ---------------------------------------------------------------------------
// UTF-16 to UTF-8 conversion
// ---------------------------------------------------------------------------

public func utf16_to_utf8(in_w : *u16, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
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

// ---------------------------------------------------------------------------
// UTF-8 helper functions
// ---------------------------------------------------------------------------

public func utf8_char_len(leading : u8) : size_t {
    if(leading < 0x80) { return 1; }
    if((leading & 0xE0) == 0xC0) { return 2; }
    if((leading & 0xF0) == 0xE0) { return 3; }
    if((leading & 0xF8) == 0xF0) { return 4; }
    return 1;
}

public func utf8_decode(data : *char, data_len : size_t, out_codepoint : *mut int, out_bytes : *mut size_t) : bool {
    if(data_len == 0) { out_bytes[0] = 0; out_codepoint[0] = 0; return false; }
    var c = data[0] as u8;
    if(c < 0x80) { out_codepoint[0] = c as int; out_bytes[0] = 1; return true; }
    var seq_len = utf8_char_len(c);
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

} // end namespace encoding
