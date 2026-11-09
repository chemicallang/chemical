// safe — owned-buffer encoding surface (no raw pointers, no caller buffers).
//
// The legacy pointer/out-buffer entry points live under `encoding::raw`
// (`base64.ch`/`hex.ch`/`url.ch`/`utf.ch`). These canonical functions take
// `&[u8]`/`&str`, return owned `string`/`vector<u8>`, and report every
// validation failure through `Result<_, EncodingError>`.

public namespace encoding {

using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ---------------------------------------------------------------------------
// Base64 (RFC 4648)
// ---------------------------------------------------------------------------

func base64_char_value(c : char) : int {
    if(c >= 'A' && c <= 'Z') { return (c as int) - ('A' as int) }
    if(c >= 'a' && c <= 'z') { return (c as int) - ('a' as int) + 26 }
    if(c >= '0' && c <= '9') { return (c as int) - ('0' as int) + 52 }
    if(c == '+') { return 62 }
    if(c == '/') { return 63 }
    return -1
}

/// Encode bytes as padded standard Base64. Allocates the result.
public func base64_encode(data : &[u8]) : Result<string, EncodingError> {
    const n = data.size()
    // `ceil(n / 3) * 4` must not overflow.
    if(n > (((((0 as size_t) - 1) / 4) * 3))) {
        return Result.Err(EncodingError.InvalidInput())
    }
    const encoded_len = ((n + 2) / 3) * 4
    var out = vector<char>()
    if(!out.try_reserve(encoded_len + 1)) {
        return Result.Err(EncodingError.OutOfMemory())
    }
    var r = crypto::raw::base64_encode(data.data(), n, out.data() as *mut char, encoded_len + 1)
    if(r is Result.Err) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(written) = r else unsafe { unreachable }
    var s = string()
    s.append_with_len(out.data() as *char, written)
    return Result.Ok(s)
}

/// Decode standard Base64 (padded or unpadded). Rejects non-alphabet
/// characters, stray padding, and impossible lengths.
public func base64_decode(b64 : &str) : Result<vector<u8>, EncodingError> {
    const size = b64.size()
    if(size == 0) {
        return Result.Ok(vector<u8>())
    }
    var effective_len = size
    var padding : size_t = 0
    while(effective_len > 0 && padding < 2 && b64.at(effective_len - 1) == '=') {
        effective_len -= 1
        padding += 1
    }
    // Padding is only legal in the last two positions, and a padded input must
    // be a multiple of four.
    if(padding > 0 && size % 4 != 0) {
        return Result.Err(EncodingError.InvalidInput())
    }
    if(effective_len % 4 == 1) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var i : size_t = 0
    while(i < effective_len) {
        if(base64_char_value(b64.at(i)) < 0) {
            return Result.Err(EncodingError.InvalidInput())
        }
        i += 1
    }
    var decoded_len = (effective_len / 4) * 3
    if(effective_len % 4 == 2) { decoded_len += 1 }
    else if(effective_len % 4 == 3) { decoded_len += 2 }

    var out = vector<u8>()
    if(!out.try_reserve(decoded_len)) {
        return Result.Err(EncodingError.OutOfMemory())
    }
    if(decoded_len == 0) {
        return Result.Ok(out)
    }
    out.resize(decoded_len)
    var r = crypto::raw::base64_decode(b64.data(), size, out.data() as *mut u8, decoded_len)
    if(r is Result.Err) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unsafe { unreachable }
    if(n < decoded_len) {
        out.resize(n)
    }
    return Result.Ok(out)
}

// ---------------------------------------------------------------------------
// Hex
// ---------------------------------------------------------------------------

/// Encode bytes as lowercase hex. Allocates the result.
public func hex_encode(data : &[u8]) : Result<string, EncodingError> {
    const n = data.size()
    if(n > ((((0 as size_t) - 1) - 1) / 2)) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var out = string()
    out.reserve(n * 2 + 1)
    var i : size_t = 0
    while(i < n) {
        var b = unsafe(*data.at(i))
        out.append(HEX_DIGITS_LOWER[((b >> 4) & 0x0F) as size_t])
        out.append(HEX_DIGITS_LOWER[(b & 0x0F) as size_t])
        i += 1
    }
    return Result.Ok(out)
}

/// Encode bytes as uppercase hex. Allocates the result.
public func hex_encode_upper(data : &[u8]) : Result<string, EncodingError> {
    const n = data.size()
    if(n > ((((0 as size_t) - 1) - 1) / 2)) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var out = string()
    out.reserve(n * 2 + 1)
    var i : size_t = 0
    while(i < n) {
        var b = unsafe(*data.at(i))
        out.append(HEX_DIGITS_UPPER[((b >> 4) & 0x0F) as size_t])
        out.append(HEX_DIGITS_UPPER[(b & 0x0F) as size_t])
        i += 1
    }
    return Result.Ok(out)
}

/// Decode a hex string (case-insensitive). Rejects odd lengths and non-hex
/// characters.
public func hex_decode(hex : &str) : Result<vector<u8>, EncodingError> {
    return hex_decode_view(hex.to_view())
}

// ---------------------------------------------------------------------------
// Percent (URL) encoding
// ---------------------------------------------------------------------------

/// Percent-encode every byte except the RFC 3986 unreserved set. Allocates.
public func url_encode(value : &str) : Result<string, EncodingError> {
    const n = value.size()
    if(n > (((0 as size_t) - 1) - 1) / 3) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var out = string()
    out.reserve(n * 3 + 1)
    var i : size_t = 0
    while(i < n) {
        var c = value.at(i)
        if(is_unreserved(c)) {
            out.append(c)
        } else {
            out.append('%')
            var b = c as u8
            out.append(HEX_DIGITS_UPPER[((b >> 4) & 0x0F) as size_t])
            out.append(HEX_DIGITS_UPPER[(b & 0x0F) as size_t])
        }
        i += 1
    }
    return Result.Ok(out)
}

/// Decode a percent-encoded string. `+` decodes to a space. Malformed
/// escapes (`%` not followed by two hex digits) and invalid UTF-8 are
/// rejected instead of being passed through.
public func url_decode(value : &str) : Result<string, EncodingError> {
    const n = value.size()
    if(n == ((0 as size_t) - 1)) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var out = vector<u8>()
    if(!out.try_reserve(n + 1)) {
        return Result.Err(EncodingError.OutOfMemory())
    }
    var i : size_t = 0
    while(i < n) {
        var c = value.at(i)
        if(c == '%') {
            if(i + 2 >= n) {
                return Result.Err(EncodingError.InvalidInput())
            }
            var high = hex_to_nibble(value.at(i + 1))
            var low = hex_to_nibble(value.at(i + 2))
            if(high < 0 || low < 0) {
                return Result.Err(EncodingError.InvalidInput())
            }
            if(!out.try_push((((high as u8) << 4) | (low as u8)))) {
                return Result.Err(EncodingError.OutOfMemory())
            }
            i += 3
        } else if(c == '+') {
            if(!out.try_push(' ' as u8)) {
                return Result.Err(EncodingError.OutOfMemory())
            }
            i += 1
        } else {
            if(!out.try_push(c as u8)) {
                return Result.Err(EncodingError.OutOfMemory())
            }
            i += 1
        }
    }
    var out_view : [u8] = &out
    if(!utf8_is_valid(out_view)) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var s = string()
    s.append_with_len(out.data() as *char, out.size())
    return Result.Ok(s)
}

// ---------------------------------------------------------------------------
// UTF-8 helpers
// ---------------------------------------------------------------------------

/// One decoded UTF-8 scalar value and the number of bytes it occupied.
@direct_init
public struct Utf8Char {
    public var codepoint : u32
    public var size : size_t
}

/// Strict UTF-8 validation (rejects overlong forms, surrogates, > U+10FFFF).
public func utf8_is_valid(bytes : &[u8]) : bool {
    var i : size_t = 0
    const n = bytes.size()
    while(i < n) {
        var c = unsafe(*bytes.at(i))
        if(c <= 0x7F) {
            i += 1
        } else if(c >= 0xC2 && c <= 0xDF) {
            if(i + 1 >= n) { return false }
            var c1 = unsafe(*bytes.at(i + 1))
            if(c1 < 0x80 || c1 > 0xBF) { return false }
            i += 2
        } else if(c == 0xE0) {
            if(i + 2 >= n) { return false }
            var c1 = unsafe(*bytes.at(i + 1))
            var c2 = unsafe(*bytes.at(i + 2))
            if(c1 < 0xA0 || c1 > 0xBF || c2 < 0x80 || c2 > 0xBF) { return false }
            i += 3
        } else if((c >= 0xE1 && c <= 0xEC) || c == 0xEE || c == 0xEF) {
            if(i + 2 >= n) { return false }
            var c1 = unsafe(*bytes.at(i + 1))
            var c2 = unsafe(*bytes.at(i + 2))
            if(c1 < 0x80 || c1 > 0xBF || c2 < 0x80 || c2 > 0xBF) { return false }
            i += 3
        } else if(c == 0xED) {
            // Exclude UTF-16 surrogate range U+D800..U+DFFF.
            if(i + 2 >= n) { return false }
            var c1 = unsafe(*bytes.at(i + 1))
            var c2 = unsafe(*bytes.at(i + 2))
            if(c1 < 0x80 || c1 > 0x9F || c2 < 0x80 || c2 > 0xBF) { return false }
            i += 3
        } else if(c == 0xF0) {
            if(i + 3 >= n) { return false }
            var c1 = unsafe(*bytes.at(i + 1))
            var c2 = unsafe(*bytes.at(i + 2))
            var c3 = unsafe(*bytes.at(i + 3))
            if(c1 < 0x90 || c1 > 0xBF) { return false }
            if(c2 < 0x80 || c2 > 0xBF || c3 < 0x80 || c3 > 0xBF) { return false }
            i += 4
        } else if(c >= 0xF1 && c <= 0xF3) {
            if(i + 3 >= n) { return false }
            var c1 = unsafe(*bytes.at(i + 1))
            var c2 = unsafe(*bytes.at(i + 2))
            var c3 = unsafe(*bytes.at(i + 3))
            if(c1 < 0x80 || c1 > 0xBF) { return false }
            if(c2 < 0x80 || c2 > 0xBF || c3 < 0x80 || c3 > 0xBF) { return false }
            i += 4
        } else if(c == 0xF4) {
            if(i + 3 >= n) { return false }
            var c1 = unsafe(*bytes.at(i + 1))
            var c2 = unsafe(*bytes.at(i + 2))
            var c3 = unsafe(*bytes.at(i + 3))
            if(c1 < 0x80 || c1 > 0x8F) { return false }
            if(c2 < 0x80 || c2 > 0xBF || c3 < 0x80 || c3 > 0xBF) { return false }
            i += 4
        } else {
            return false
        }
    }
    return true
}

/// A `&str` is UTF-8 by construction; this exists so callers can use the same
/// predicate for both input shapes.
public func utf8_is_valid_str(value : &str) : bool {
    return true
}

/// Byte length of the UTF-8 sequence introduced by `leading` (1 for ASCII and
/// for invalid lead bytes, matching the legacy helper).
public func utf8_char_len(leading : u8) : size_t {
    if(leading < 0x80) { return 1 }
    if((leading & 0xE0) == 0xC0) { return 2 }
    if((leading & 0xF0) == 0xE0) { return 3 }
    if((leading & 0xF8) == 0xF0) { return 4 }
    return 1
}

/// Decode one UTF-8 scalar at `offset`. Returns the scalar and the number of
/// bytes consumed, or `InvalidInput` for malformed/truncated input.
public func utf8_decode_char(bytes : &[u8], offset : size_t) : Result<Utf8Char, EncodingError> {
    const n = bytes.size()
    if(offset >= n) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var c = unsafe(*bytes.at(offset))
    if(c <= 0x7F) {
        return Result.Ok(Utf8Char { codepoint : c as u32, size : 1 })
    }
    if(c >= 0xC2 && c <= 0xDF) {
        if(offset + 1 >= n) { return Result.Err(EncodingError.InvalidInput()) }
        var c1 = unsafe(*bytes.at(offset + 1))
        if(c1 < 0x80 || c1 > 0xBF) { return Result.Err(EncodingError.InvalidInput()) }
        var cp : u32 = (((c & 0x1F) as u32) << 6) | ((c1 & 0x3F) as u32)
        return Result.Ok(Utf8Char { codepoint : cp, size : 2 })
    }
    if(c >= 0xE0 && c <= 0xEF) {
        if(offset + 2 >= n) { return Result.Err(EncodingError.InvalidInput()) }
        var c1 = unsafe(*bytes.at(offset + 1))
        var c2 = unsafe(*bytes.at(offset + 2))
        var min1 : u8 = 0x80
        var max1 : u8 = 0xBF
        if(c == 0xE0) { min1 = 0xA0 }
        else if(c == 0xED) { max1 = 0x9F }
        if(c1 < min1 || c1 > max1) { return Result.Err(EncodingError.InvalidInput()) }
        if(c2 < 0x80 || c2 > 0xBF) { return Result.Err(EncodingError.InvalidInput()) }
        var cp : u32 = (((c & 0x0F) as u32) << 12) | (((c1 & 0x3F) as u32) << 6) | ((c2 & 0x3F) as u32)
        return Result.Ok(Utf8Char { codepoint : cp, size : 3 })
    }
    if(c >= 0xF0 && c <= 0xF4) {
        if(offset + 3 >= n) { return Result.Err(EncodingError.InvalidInput()) }
        var c1 = unsafe(*bytes.at(offset + 1))
        var c2 = unsafe(*bytes.at(offset + 2))
        var c3 = unsafe(*bytes.at(offset + 3))
        var min1 : u8 = 0x80
        var max1 : u8 = 0xBF
        if(c == 0xF0) { min1 = 0x90 }
        else if(c == 0xF4) { max1 = 0x8F }
        if(c1 < min1 || c1 > max1) { return Result.Err(EncodingError.InvalidInput()) }
        if(c2 < 0x80 || c2 > 0xBF || c3 < 0x80 || c3 > 0xBF) {
            return Result.Err(EncodingError.InvalidInput())
        }
        var cp : u32 = (((c & 0x07) as u32) << 18) | (((c1 & 0x3F) as u32) << 12) |
                       (((c2 & 0x3F) as u32) << 6) | ((c3 & 0x3F) as u32)
        return Result.Ok(Utf8Char { codepoint : cp, size : 4 })
    }
    return Result.Err(EncodingError.InvalidInput())
}

/// Copy validated UTF-8 bytes into an owned `string`.
public func utf8_to_string(bytes : &[u8]) : Result<string, EncodingError> {
    if(!utf8_is_valid(bytes)) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var s = string()
    s.append_with_len(bytes.data() as *char, bytes.size())
    return Result.Ok(s)
}

// ---------------------------------------------------------------------------
// UTF-8 <-> UTF-16 (owned outputs)
// ---------------------------------------------------------------------------

/// Convert UTF-8 to UTF-16. Rejects malformed input instead of truncating.
public func utf8_bytes_to_utf16(bytes : &[u8]) : Result<vector<u16>, EncodingError> {
    const n = bytes.size()
    // Each input byte contributes at most one UTF-16 code unit (a 4-byte
    // sequence contributes two), so `n` is a safe upper bound.
    var out = vector<u16>()
    if(!out.try_reserve(n + 1)) {
        return Result.Err(EncodingError.OutOfMemory())
    }
    var offset : size_t = 0
    while(offset < n) {
        var decoded = utf8_decode_char(bytes, offset)
        if(decoded is Result.Err) {
            return Result.Err(EncodingError.InvalidInput())
        }
        var Ok(ch) = decoded else unsafe { unreachable }
        if(ch.codepoint <= 0xFFFFu32) {
            if(!out.try_push(ch.codepoint as u16)) {
                return Result.Err(EncodingError.OutOfMemory())
            }
        } else {
            var v = ch.codepoint - 0x10000u32
            var high = (0xD800u16 + ((v >> 10) as u16))
            var low = (0xDC00u16 + ((v & 0x3FFu32) as u16))
            if(!out.try_push(high)) {
                return Result.Err(EncodingError.OutOfMemory())
            }
            if(!out.try_push(low)) {
                return Result.Err(EncodingError.OutOfMemory())
            }
        }
        offset += ch.size
    }
    return Result.Ok(out)
}

/// Convert a validated `&str` to UTF-16.
public func utf8_to_utf16(value : &str) : Result<vector<u16>, EncodingError> {
    var view = value.to_view()
    var bytes : [u8] = unsafe(std::span<u8>(view.data() as *u8, view.size()))
    return utf8_bytes_to_utf16(bytes)
}

/// Convert UTF-16 to UTF-8. Rejects lone surrogates; always emits valid UTF-8.
public func utf16_to_utf8(value : &[u16]) : Result<string, EncodingError> {
    const n = value.size()
    if(n > (((0 as size_t) - 1) - 1) / 3) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var out = vector<u8>()
    if(!out.try_reserve(n * 3 + 1)) {
        return Result.Err(EncodingError.OutOfMemory())
    }
    var i : size_t = 0
    while(i < n) {
        var w = unsafe(*value.at(i))
        if(w < 0xD800 || w > 0xDFFF) {
            if(w < 0x80) {
                if(!out.try_push(w as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
            } else if(w < 0x800) {
                if(!out.try_push((0xC0 | ((w >> 6) & 0x1F)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
                if(!out.try_push((0x80 | (w & 0x3F)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
            } else {
                if(!out.try_push((0xE0 | ((w >> 12) & 0x0F)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
                if(!out.try_push((0x80 | ((w >> 6) & 0x3F)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
                if(!out.try_push((0x80 | (w & 0x3F)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
            }
            i += 1
        } else if(w >= 0xD800 && w <= 0xDBFF) {
            if(i + 1 >= n) { return Result.Err(EncodingError.InvalidInput()) }
            var w2 = unsafe(*value.at(i + 1))
            if(w2 < 0xDC00 || w2 > 0xDFFF) { return Result.Err(EncodingError.InvalidInput()) }
            var cp : u32 = 0x10000u32 + ((((w & 0x3FF) as u32) << 10) | ((w2 & 0x3FF) as u32))
            if(!out.try_push((0xF0 | ((cp >> 18) & 0x07)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
            if(!out.try_push((0x80 | ((cp >> 12) & 0x3F)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
            if(!out.try_push((0x80 | ((cp >> 6) & 0x3F)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
            if(!out.try_push((0x80 | (cp & 0x3F)) as u8)) { return Result.Err(EncodingError.OutOfMemory()) }
            i += 2
        } else {
            // Lone low surrogate.
            return Result.Err(EncodingError.InvalidInput())
        }
    }
    var s = string()
    s.append_with_len(out.data() as *char, out.size())
    return Result.Ok(s)
}

} // end namespace encoding
