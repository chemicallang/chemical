public namespace fs {

using std::Result;

func utf8_to_utf16(in_utf8 : *char, out_w : *mut u16, out_w_len : size_t) : Result<size_t, FsError> {
    // Safe UTF-8 -> UTF-16 converter: validates continuation bytes and avoids out-of-bounds reads.
    var i : size_t = 0;
    var wpos : size_t = 0;

    while (in_utf8[i] != 0) {
        var c = in_utf8[i] as u8;

        // ASCII
        if (c < 0x80) {
            if (wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = c as u16;
            i += 1;
            continue;
        }

        // 2-byte sequence: 110xxxxx 10xxxxxx
        if ((c & 0xE0) == 0xC0) {
            // need one continuation byte
            if (in_utf8[i+1] == 0) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }
            var c2 = in_utf8[i+1] as u8;
            if ((c2 & 0xC0) != 0x80) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            // decode and check overlong (must be >= 0x80)
            var code = (((c & 0x1F) as u32) << 6) | ((c2 & 0x3F) as u32);
            if (code < 0x80u32) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            if (wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = code as u16;
            i += 2;
            continue;
        }

        // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
        if ((c & 0xF0) == 0xE0) {
            // need two continuation bytes
            if (in_utf8[i+1] == 0 || in_utf8[i+2] == 0) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }
            var c2 = in_utf8[i+1] as u8;
            var c3 = in_utf8[i+2] as u8;
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            var code = (((c & 0x0F) as u32) << 12) | (((c2 & 0x3F) as u32) << 6) | ((c3 & 0x3F) as u32);

            // Reject overlong encoding and surrogate halves (U+D800..U+DFFF)
            if (code < 0x800u32) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }
            if (code >= 0xD800u32 && code <= 0xDFFFu32) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            if (wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = code as u16;
            i += 3;
            continue;
        }

        // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx -> surrogate pair in UTF-16
        if ((c & 0xF8) == 0xF0) {
            // need three continuation bytes
            if (in_utf8[i+1] == 0 || in_utf8[i+2] == 0 || in_utf8[i+3] == 0) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }
            var c2 = in_utf8[i+1] as u8;
            var c3 = in_utf8[i+2] as u8;
            var c4 = in_utf8[i+3] as u8;
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            var code = (((c & 0x07) as u32) << 18) | (((c2 & 0x3F) as u32) << 12) | (((c3 & 0x3F) as u32) << 6) | ((c4 & 0x3F) as u32);

            // minimal value for 4-byte is 0x10000 and max is 0x10FFFF
            if (code < 0x10000u32 || code > 0x10FFFFu32) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            // produce surrogate pair
            code -= 0x10000u32;
            var high = 0xD800u16 + ((code >> 10) as u16);
            var low  = 0xDC00u16 + ((code & 0x3FFu32) as u16);

            if (wpos + 2 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = high;
            out_w[wpos++] = low;
            i += 4;
            continue;
        }

        // anything else is invalid (e.g. 0x80..0xBF as a leading byte or 0xF8+)
        return Result.Err<size_t, FsError>(FsError.InvalidInput());
    }

    // null-terminate (ensure we still have space)
    if (wpos >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    out_w[wpos] = 0;
    return Result.Ok<size_t, FsError>(wpos);
}

func utf16_to_utf8(in_w : *u16, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var i : size_t = 0;
    var pos : size_t = 0;
    while(true) {
        var w = in_w[i];
        if(w == 0) { break; }
        if(w < 0x80) {
            if(pos + 1 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[pos++] = (w as char);
        } else if(w < 0x800) {
            if(pos + 2 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[pos++] = (0xC0 | ((w >> 6) & 0x1F)) as char;
            out[pos++] = (0x80 | (w & 0x3F)) as char;
        } else if(w >= 0xD800 && w <= 0xDBFF) {
            // surrogate pair
            var w2 = in_w[i+1];
            var code = 0x10000 + ((((w & 0x3FF) as u32) << 10) | ((w2 & 0x3FF) as u32));
            if(pos + 4 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[pos++] = (0xF0 | ((code >> 18) & 0x07)) as char;
            out[pos++] = (0x80 | ((code >> 12) & 0x3F)) as char;
            out[pos++] = (0x80 | ((code >> 6) & 0x3F)) as char;
            out[pos++] = (0x80 | (code & 0x3F)) as char;
            i += 1; // consumed extra
        } else {
            if(pos + 3 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[pos++] = (0xE0 | ((w >> 12) & 0x0F)) as char;
            out[pos++] = (0x80 | ((w >> 6) & 0x3F)) as char;
            out[pos++] = (0x80 | (w & 0x3F)) as char;
        }
        i++;
    }
    if(pos >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    out[pos] = 0;
    return Result.Ok<size_t, FsError>(pos);
}

}