// base64 — Base64 encoding and decoding (RFC 4648).
// Thin wrappers around crypto::base64_* for discoverability.

public namespace encoding {

using std::Result;
using std::string_view;
using std::string;
using std::vector;

// Encode raw bytes to base64 string.
// Caller provides output buffer. Returns encoded length or error.
public func base64_encode(data : *u8, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, EncodingError> {
    var r = crypto::base64_encode(data, data_len, out, out_len)
    if(r is Result.Err) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unreachable
    return Result.Ok(n)
}

// Decode base64 string to raw bytes.
// Caller provides output buffer. Returns decoded length or error.
public func base64_decode(b64 : *char, b64_len : size_t, out : *mut u8, out_len : size_t) : Result<size_t, EncodingError> {
    var r = crypto::base64_decode(b64, b64_len, out, out_len)
    if(r is Result.Err) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unreachable
    return Result.Ok(n)
}

// Encode raw bytes to base64 string (heap convenience).
// Allocates the output string internally.
public func base64_encode_to_string(data : *u8, data_len : size_t) : Result<string, EncodingError> {
    var encoded_len = ((data_len + 2) / 3) * 4
    var buf = malloc(encoded_len + 1) as *mut char
    if(buf == null) { return Result.Err(EncodingError.InvalidInput()) }
    var r = crypto::base64_encode(data, data_len, buf, encoded_len + 1)
    if(r is Result.Err) {
        free(buf)
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unreachable
    var s = string.constructor(buf as *char, n)
    free(buf)
    return Result.Ok(s)
}

// Decode base64 string to vector<u8> (heap convenience).
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
    var r = crypto::base64_decode(b64.data(), b64.size(), out.data() as *mut u8, decoded_len)
    if(r is Result.Err) {
        return Result.Err(EncodingError.InvalidInput())
    }
    var Ok(n) = r else unreachable
    if(n < decoded_len) { out.resize(n) }
    return Result.Ok(out)
}

} // end namespace encoding
