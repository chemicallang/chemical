// safe — owned-buffer crypto surface.
//
// One-shot hashes take `&[u8]` and return fixed-size digest arrays; HMAC takes
// `&[u8]` key/data pairs; equality comparison takes two byte slices. Nothing
// here exposes a caller-owned output buffer or a raw pointer. Streaming
// hashers own their context and only accept `&[u8]` chunks.
//
// The legacy raw context/out-parameter primitives live under `crypto::raw`.
// Base64 lives canonically in the `encoding` library.

public namespace crypto {

// ---------------------------------------------------------------------------
// One-shot hashes
// ---------------------------------------------------------------------------

public func sha256(data : &[u8]) : [32]u8 {
    var digest : [32]u8
    crypto::raw::sha256_hash(data.data(), data.size(), &raw mut digest[0])
    return digest
}

public func sha384(data : &[u8]) : [48]u8 {
    var digest : [48]u8
    crypto::raw::sha384_hash(data.data(), data.size(), &raw mut digest[0])
    return digest
}

public func sha512(data : &[u8]) : [64]u8 {
    var digest : [64]u8
    crypto::raw::sha512_hash(data.data(), data.size(), &raw mut digest[0])
    return digest
}

/// MD5 is legacy and cryptographically broken; provided for compatibility.
public func md5(data : &[u8]) : [16]u8 {
    var digest : [16]u8
    crypto::raw::md5_hash(data.data(), data.size(), &raw mut digest[0])
    return digest
}

// ---------------------------------------------------------------------------
// One-shot HMAC
// ---------------------------------------------------------------------------

public func hmac_sha256(key : &[u8], data : &[u8]) : [32]u8 {
    var digest : [32]u8
    crypto::raw::hmac_sha256(key.data(), key.size(), data.data(), data.size(), &raw mut digest[0])
    return digest
}

public func hmac_sha384(key : &[u8], data : &[u8]) : [48]u8 {
    var digest : [48]u8
    crypto::raw::hmac_sha384(key.data(), key.size(), data.data(), data.size(), &raw mut digest[0])
    return digest
}

/// HMAC-MD5 is legacy and cryptographically broken; provided for compatibility.
public func hmac_md5(key : &[u8], data : &[u8]) : [16]u8 {
    var digest : [16]u8
    crypto::raw::hmac_md5(key.data(), key.size(), data.data(), data.size(), &raw mut digest[0])
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
    return crypto::raw::constant_time_equal(a.data(), b.data(), a.size())
}

// ---------------------------------------------------------------------------
// Streaming hashers
// ---------------------------------------------------------------------------

/// Incremental SHA-256. Call `update` any number of times, then `finish`
/// once. `finish` consumes the hasher, so it cannot be reused afterwards;
/// call `reset` before `finish` to start a fresh message.
public struct Sha256Hasher {
    internal var ctx : raw::Sha256Context

    @constructor
    func make() {
        var ctx = zeroed<raw::Sha256Context>()
        crypto::raw::sha256_init(unsafe(&raw mut ctx))
        return Sha256Hasher { ctx : ctx }
    }

    func update(&mut self, data : &[u8]) {
        crypto::raw::sha256_update(unsafe(&raw mut self.ctx), data.data(), data.size())
    }

    func finish(self) : [32]u8 {
        var digest : [32]u8
        crypto::raw::sha256_final(unsafe(&raw mut self.ctx), &raw mut digest[0])
        return digest
    }

    func reset(&mut self) {
        crypto::raw::sha256_init(unsafe(&raw mut self.ctx))
    }
}

/// Incremental SHA-384. `finish` consumes the hasher; `reset` starts over.
public struct Sha384Hasher {
    internal var ctx : raw::Sha512Context

    @constructor
    func make() {
        var ctx = zeroed<raw::Sha512Context>()
        crypto::raw::sha384_init(unsafe(&raw mut ctx))
        return Sha384Hasher { ctx : ctx }
    }

    func update(&mut self, data : &[u8]) {
        crypto::raw::sha384_update(unsafe(&raw mut self.ctx), data.data(), data.size())
    }

    func finish(self) : [48]u8 {
        var digest : [48]u8
        crypto::raw::sha384_final(unsafe(&raw mut self.ctx), &raw mut digest[0])
        return digest
    }

    func reset(&mut self) {
        crypto::raw::sha384_init(unsafe(&raw mut self.ctx))
    }
}

/// Incremental SHA-512. `finish` consumes the hasher; `reset` starts over.
public struct Sha512Hasher {
    internal var ctx : raw::Sha512Context

    @constructor
    func make() {
        var ctx = zeroed<raw::Sha512Context>()
        crypto::raw::sha512_init(unsafe(&raw mut ctx))
        return Sha512Hasher { ctx : ctx }
    }

    func update(&mut self, data : &[u8]) {
        crypto::raw::sha512_update(unsafe(&raw mut self.ctx), data.data(), data.size())
    }

    func finish(self) : [64]u8 {
        var digest : [64]u8
        crypto::raw::sha512_final(unsafe(&raw mut self.ctx), &raw mut digest[0])
        return digest
    }

    func reset(&mut self) {
        crypto::raw::sha512_init(unsafe(&raw mut self.ctx))
    }
}

/// Incremental MD5 (legacy, cryptographically broken). `finish` consumes
/// the hasher; `reset` starts over.
public struct Md5Hasher {
    internal var ctx : raw::Md5Context

    @constructor
    func make() {
        var ctx = zeroed<raw::Md5Context>()
        crypto::raw::md5_init(unsafe(&raw mut ctx))
        return Md5Hasher { ctx : ctx }
    }

    func update(&mut self, data : &[u8]) {
        crypto::raw::md5_update(unsafe(&raw mut self.ctx), data.data(), data.size())
    }

    func finish(self) : [16]u8 {
        var digest : [16]u8
        crypto::raw::md5_final(unsafe(&raw mut self.ctx), &raw mut digest[0])
        return digest
    }

    func reset(&mut self) {
        crypto::raw::md5_init(unsafe(&raw mut self.ctx))
    }
}

} // namespace crypto
