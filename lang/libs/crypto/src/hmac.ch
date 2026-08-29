// hmac — HMAC implementation (RFC 2104) using SHA-256.
// Provides keyed-hash message authentication.

public namespace crypto {

using std::Result;

// ---------------------------------------------------------------------------
// HMAC-SHA256
// ---------------------------------------------------------------------------

/// Compute HMAC-SHA256.
/// key: secret key
/// data: message data
/// digest: output buffer (at least 32 bytes)
public func hmac_sha256(key : *u8, key_len : size_t, data : *u8, data_len : size_t, digest : *mut u8) {
    // If key is longer than block size (64 bytes), hash it first
    var actual_key : [64]u8;
    var actual_key_len : size_t = key_len;

    if(key_len > 64) {
        sha256_hash(key, key_len, &raw mut actual_key[0]);
        actual_key_len = 32;
        var zi : size_t = actual_key_len; while(zi < 64) { actual_key[zi] = 0; zi += 1 }
    } else {
        var i : size_t = 0;
        while(i < key_len) {
            actual_key[i] = key[i];
            i += 1;
        }
        while(i < 64) {
            actual_key[i] = 0;
            i += 1;
        }
    }

    // Compute inner hash: H((key ^ ipad) || message)
    var inner_key : [64]u8;
    var outer_key : [64]u8;
    const IPAD : u8 = 0x36;
    const OPAD : u8 = 0x5C;

    var i : size_t = 0;
    while(i < 64) {
        inner_key[i] = actual_key[i] ^ IPAD;
        outer_key[i] = actual_key[i] ^ OPAD;
        i += 1;
    }

    // Inner hash: SHA256(inner_key || data)
    var inner_ctx : Sha256Context;
    sha256_init(unsafe(&raw mut inner_ctx));
    sha256_update(unsafe(&raw mut inner_ctx), &raw inner_key[0], 64);
    sha256_update(unsafe(&raw mut inner_ctx), data, data_len);

    var inner_digest : [32]u8;
    sha256_final(unsafe(&raw mut inner_ctx), &raw mut inner_digest[0]);

    // Outer hash: SHA256(outer_key || inner_digest)
    var outer_ctx : Sha256Context;
    sha256_init(unsafe(&raw mut outer_ctx));
    sha256_update(unsafe(&raw mut outer_ctx), &raw outer_key[0], 64);
    sha256_update(unsafe(&raw mut outer_ctx), &raw inner_digest[0], 32);
    sha256_final(unsafe(&raw mut outer_ctx), digest);
}

// ---------------------------------------------------------------------------
// HMAC-SHA384 (SHA-512 family — 128-byte block size, 48-byte digest)
// ---------------------------------------------------------------------------

/// Compute HMAC-SHA384.
/// key: secret key
/// data: message data
/// digest: output buffer (at least 48 bytes)
public func hmac_sha384(key : *u8, key_len : size_t, data : *u8, data_len : size_t, digest : *mut u8) {
    // If key is longer than block size (128 bytes for the SHA-512 family),
    // hash it first. All locals are declared up front — mid-function
    // declarations can receive clobbered stack slots in the TCC backend.
    var actual_key : [128]u8;
    var inner_key : [128]u8;
    var outer_key : [128]u8;
    var inner_ctx : Sha512Context;
    var outer_ctx : Sha512Context;
    var inner_digest : [48]u8;
    var actual_key_len : size_t = key_len;
    var i : size_t = 0;

    if(key_len > 128) {
        sha384_hash(key, key_len, &raw mut actual_key[0]);
        actual_key_len = 48;
        i = 48; while(i < 128) { actual_key[i] = 0; i += 1 }
    } else {
        i = 0;
        while(i < key_len) {
            actual_key[i] = key[i];
            i += 1;
        }
        while(i < 128) {
            actual_key[i] = 0;
            i += 1;
        }
    }

    i = 0;
    while(i < 128) {
        inner_key[i] = actual_key[i] ^ (0x36 as u8);
        outer_key[i] = actual_key[i] ^ (0x5C as u8);
        i += 1;
    }

    // Inner hash: SHA384(inner_key || data)
    sha384_init(unsafe(&raw mut inner_ctx));
    sha384_update(unsafe(&raw mut inner_ctx), &raw inner_key[0], 128);
    sha384_update(unsafe(&raw mut inner_ctx), data, data_len);
    sha384_final(unsafe(&raw mut inner_ctx), &raw mut inner_digest[0]);

    // Outer hash: SHA384(outer_key || inner_digest)
    sha384_init(unsafe(&raw mut outer_ctx));
    sha384_update(unsafe(&raw mut outer_ctx), &raw outer_key[0], 128);
    sha384_update(unsafe(&raw mut outer_ctx), &raw inner_digest[0], 48);
    sha384_final(unsafe(&raw mut outer_ctx), digest);
}

// ---------------------------------------------------------------------------
// HMAC-MD5 (legacy, cryptographically broken)
// ---------------------------------------------------------------------------

/// Compute HMAC-MD5 (legacy only, not for security).
public func hmac_md5(key : *u8, key_len : size_t, data : *u8, data_len : size_t, digest : *mut u8) {
    var actual_key : [64]u8;
    var actual_key_len : size_t = key_len;

    if(key_len > 64) {
        md5_hash(key, key_len, &raw mut actual_key[0]);
        actual_key_len = 16;
        var zi : size_t = actual_key_len; while(zi < 64) { actual_key[zi] = 0; zi += 1 }
    } else {
        var i : size_t = 0;
        while(i < key_len) {
            actual_key[i] = key[i];
            i += 1;
        }
        while(i < 64) {
            actual_key[i] = 0;
            i += 1;
        }
    }

    var inner_key : [64]u8;
    var outer_key : [64]u8;
    const IPAD : u8 = 0x36;
    const OPAD : u8 = 0x5C;

    var i : size_t = 0;
    while(i < 64) {
        inner_key[i] = actual_key[i] ^ IPAD;
        outer_key[i] = actual_key[i] ^ OPAD;
        i += 1;
    }

    // Inner MD5
    var inner_ctx : Md5Context;
    md5_init(unsafe(&raw mut inner_ctx));
    md5_update(unsafe(&raw mut inner_ctx), &raw inner_key[0], 64);
    md5_update(unsafe(&raw mut inner_ctx), data, data_len);

    var inner_digest : [16]u8;
    md5_final(unsafe(&raw mut inner_ctx), &raw mut inner_digest[0]);

    // Outer MD5
    var outer_ctx : Md5Context;
    md5_init(unsafe(&raw mut outer_ctx));
    md5_update(unsafe(&raw mut outer_ctx), &raw outer_key[0], 64);
    md5_update(unsafe(&raw mut outer_ctx), &raw inner_digest[0], 16);
    md5_final(unsafe(&raw mut outer_ctx), digest);
}

// ---------------------------------------------------------------------------
// Constant-time comparison
// ---------------------------------------------------------------------------

/// Constant-time comparison of two byte arrays.
/// Returns true if they are equal.
/// Safe against timing side-channel attacks.
public func constant_time_equal(a : *u8, b : *u8, len : size_t) : bool {
    var diff : u8 = 0;
    var i : size_t = 0;
    while(i < len) {
        diff = diff | (a[i] ^ b[i]);
        i += 1;
    }
    return diff == 0;
}

} // end namespace crypto
