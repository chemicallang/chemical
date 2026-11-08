// safe — owned-buffer crypto surface.
//
// One-shot hashes take `&[u8]` and return fixed-size digest arrays; HMAC takes
// `&[u8]` key/data pairs; equality comparison takes two byte slices. Nothing
// here exposes a caller-owned output buffer or a raw pointer. Streaming
// wrappers (`Sha256Hasher` and friends) own their context and only accept
// `&[u8]` chunks.
//
// They live under `crypto::safe` because Chemical has no function overloading:
// the raw primitives keep their original names in `crypto`.

public namespace crypto {

public namespace safe {

using std::Result;
using std::string;
using std::vector;

// ---------------------------------------------------------------------------
// One-shot hashes
// ---------------------------------------------------------------------------

public func sha256(data : &[u8]) : [32]u8 {
    var digest : [32]u8
    crypto::sha256_hash(data.data(), data.size(), &raw mut digest[0])
    return digest
}

public func sha384(data : &[u8]) : [48]u8 {
    var digest : [48]u8
    crypto::sha384_hash(data.data(), data.size(), &raw mut digest[0])
    return digest
}

public func sha512(data : &[u8]) : [64]u8 {
    var digest : [64]u8
    crypto::sha512_hash(data.data(), data.size(), &raw mut digest[0])
    return digest
}

/// MD5 is legacy and cryptographically broken; provided for compatibility.
public func md5(data : &[u8]) : [16]u8 {
    var digest : [16]u8
    crypto::md5_hash(data.data(), data.size(), &raw mut digest[0])
    return digest
}

// ---------------------------------------------------------------------------
// One-shot HMAC
// ---------------------------------------------------------------------------

public func hmac_sha256(key : &[u8], data : &[u8]) : [32]u8 {
    var digest : [32]u8
    crypto::hmac_sha256(key.data(), key.size(), data.data(), data.size(), &raw mut digest[0])
    return digest
}

public func hmac_sha384(key : &[u8], data : &[u8]) : [48]u8 {
    var digest : [48]u8
    crypto::hmac_sha384(key.data(), key.size(), data.data(), data.size(), &raw mut digest[0])
    return digest
}

/// HMAC-MD5 is legacy and cryptographically broken; provided for compatibility.
public func hmac_md5(key : &[u8], data : &[u8]) : [16]u8 {
    var digest : [16]u8
    crypto::hmac_md5(key.data(), key.size(), data.data(), data.size(), &raw mut digest[0])
    return digest
}

// ---------------------------------------------------------------------------
// Constant-time comparison
// ---------------------------------------------------------------------------

/// Compare two byte slices without data-dependent early exit. Length is not
/// secret (the shorter/longer shape is rejected up front).
public func constant_time_equal(a : &[u8], b : &[u8]) : bool {
    if(a.size() != b.size()) {
        return false
    }
    if(a.size() == 0) {
        return true
    }
    return crypto::constant_time_equal(a.data(), b.data(), a.size())
}

// ---------------------------------------------------------------------------
// Streaming hashers
// ---------------------------------------------------------------------------

/// Incremental SHA-256. Call `update` any number of times, then `finish` once.
public struct Sha256Hasher {
    internal var ctx : Sha256Context

    @constructor
    func make() {
        var ctx = zeroed<Sha256Context>()
        crypto::sha256_init(unsafe(&raw mut ctx))
        return Sha256Hasher { ctx : ctx }
    }

    func update(&mut self, data : &[u8]) {
        crypto::sha256_update(unsafe(&raw mut self.ctx), data.data(), data.size())
    }

    func finish(&mut self) : [32]u8 {
        var digest : [32]u8
        crypto::sha256_final(unsafe(&raw mut self.ctx), &raw mut digest[0])
        return digest
    }

    func reset(&mut self) {
        crypto::sha256_init(unsafe(&raw mut self.ctx))
    }
}

/// Incremental SHA-384.
public struct Sha384Hasher {
    internal var ctx : Sha512Context

    @constructor
    func make() {
        var ctx = zeroed<Sha512Context>()
        crypto::sha384_init(unsafe(&raw mut ctx))
        return Sha384Hasher { ctx : ctx }
    }

    func update(&mut self, data : &[u8]) {
        crypto::sha384_update(unsafe(&raw mut self.ctx), data.data(), data.size())
    }

    func finish(&mut self) : [48]u8 {
        var digest : [48]u8
        crypto::sha384_final(unsafe(&raw mut self.ctx), &raw mut digest[0])
        return digest
    }

    func reset(&mut self) {
        crypto::sha384_init(unsafe(&raw mut self.ctx))
    }
}

/// Incremental SHA-512.
public struct Sha512Hasher {
    internal var ctx : Sha512Context

    @constructor
    func make() {
        var ctx = zeroed<Sha512Context>()
        crypto::sha512_init(unsafe(&raw mut ctx))
        return Sha512Hasher { ctx : ctx }
    }

    func update(&mut self, data : &[u8]) {
        crypto::sha512_update(unsafe(&raw mut self.ctx), data.data(), data.size())
    }

    func finish(&mut self) : [64]u8 {
        var digest : [64]u8
        crypto::sha512_final(unsafe(&raw mut self.ctx), &raw mut digest[0])
        return digest
    }

    func reset(&mut self) {
        crypto::sha512_init(unsafe(&raw mut self.ctx))
    }
}

/// Incremental MD5 (legacy, cryptographically broken).
public struct Md5Hasher {
    internal var ctx : Md5Context

    @constructor
    func make() {
        var ctx = zeroed<Md5Context>()
        crypto::md5_init(unsafe(&raw mut ctx))
        return Md5Hasher { ctx : ctx }
    }

    func update(&mut self, data : &[u8]) {
        crypto::md5_update(unsafe(&raw mut self.ctx), data.data(), data.size())
    }

    func finish(&mut self) : [16]u8 {
        var digest : [16]u8
        crypto::md5_final(unsafe(&raw mut self.ctx), &raw mut digest[0])
        return digest
    }

    func reset(&mut self) {
        crypto::md5_init(unsafe(&raw mut self.ctx))
    }
}

// ---------------------------------------------------------------------------
// Base64 (owned output)
// ---------------------------------------------------------------------------

/// Encode bytes as padded standard Base64. Allocates the result.
public func base64_encode(data : &[u8]) : Result<string, CryptoError> {
    const n = data.size()
    // `ceil(n / 3) * 4` must not overflow.
    if(n > (((((0 as size_t) - 1) / 4) * 3))) {
        return Result.Err(CryptoError.InvalidInput())
    }
    const encoded_len = ((n + 2) / 3) * 4
    var out = vector<char>()
    if(!out.try_reserve(encoded_len + 1)) {
        return Result.Err(CryptoError.OutOfMemory())
    }
    var r = crypto::base64_encode(data.data(), n, out.data() as *mut char, encoded_len + 1)
    if(r is Result.Err) {
        return Result.Err(CryptoError.InvalidInput())
    }
    var Ok(written) = r else unsafe { unreachable }
    var s = string()
    s.append_with_len(out.data() as *char, written)
    return Result.Ok(s)
}

/// Decode standard Base64 (padded or unpadded), rejecting malformed input.
public func base64_decode(b64 : &str) : Result<vector<u8>, CryptoError> {
    const size = b64.size()
    if(size == 0) {
        return Result.Ok(vector<u8>())
    }
    var effective_len = size
    var padding : size_t = 0
    while(effective_len > 0 && padding < 2 && b64.get(effective_len - 1) == '=') {
        effective_len -= 1
        padding += 1
    }
    if(padding > 0 && size % 4 != 0) {
        return Result.Err(CryptoError.InvalidInput())
    }
    if(effective_len % 4 == 1) {
        return Result.Err(CryptoError.InvalidInput())
    }
    var i : size_t = 0
    while(i < effective_len) {
        if(base64_val(b64.get(i)) < 0) {
            return Result.Err(CryptoError.InvalidInput())
        }
        i += 1
    }
    var decoded_len = (effective_len / 4) * 3
    if(effective_len % 4 == 2) { decoded_len += 1 }
    else if(effective_len % 4 == 3) { decoded_len += 2 }

    var out = vector<u8>()
    if(!out.try_reserve(decoded_len)) {
        return Result.Err(CryptoError.OutOfMemory())
    }
    if(decoded_len == 0) {
        return Result.Ok(out)
    }
    out.resize(decoded_len)
    var r = crypto::base64_decode(b64.data(), size, out.data() as *mut u8, decoded_len)
    if(r is Result.Err) {
        return Result.Err(CryptoError.InvalidInput())
    }
    var Ok(n) = r else unsafe { unreachable }
    if(n < decoded_len) {
        out.resize(n)
    }
    return Result.Ok(out)
}

} // namespace safe

} // namespace crypto
