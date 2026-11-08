// raw — legacy crypto context/out-parameter primitives.
//
// This file is the single declaration of `crypto::raw` in the module; nested
// namespaces cannot be reopened across files. The canonical owned-buffer API
// lives at the top-level `crypto` namespace (see `safe.ch`), and Base64 lives
// canonically in the `encoding` library.

public namespace crypto {

public namespace raw {

using std::Result;
using std::string_view;
using std::string;
using std::vector;

const BASE64_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

func base64_val(c : char) : int {
        unsafe {
        if(c >= 'A' && c <= 'Z') { return (c as int) - ('A' as int); }
        if(c >= 'a' && c <= 'z') { return (c as int) - ('a' as int) + 26; }
        if(c >= '0' && c <= '9') { return (c as int) - ('0' as int) + 52; }
        if(c == '+') { return 62; }
        if(c == '/') { return 63; }
        return -1;

        }
    }

public func base64_encode(data : *u8, data_len : size_t, out : *mut char, out_len : size_t) : Result<size_t, CryptoError> {
        unsafe {
        var encoded_len = ((data_len + 2) / 3) * 4;
        if(encoded_len + 1 > out_len) { return Result.Err(CryptoError.BufferTooSmall(encoded_len + 1)); }
        var i : size_t = 0;
        var pos : size_t = 0;
        while(i < data_len) {
            var group_start = i;
            var b0 = data[i]; i += 1;
            var b1 : u8 = 0; if(i < data_len) { b1 = data[i]; i += 1; }
            var b2 : u8 = 0; if(i < data_len) { b2 = data[i]; i += 1; }
            var triple = (b0 as u32) << 16 | (b1 as u32) << 8 | (b2 as u32);
            out[pos]     = BASE64_TABLE[((triple >> 18) & 0x3F) as size_t];
            out[pos + 1] = BASE64_TABLE[((triple >> 12) & 0x3F) as size_t];
            out[pos + 2] = BASE64_TABLE[((triple >> 6) & 0x3F) as size_t];
            out[pos + 3] = BASE64_TABLE[(triple & 0x3F) as size_t];
            var bytes_in_group = i - group_start;
            if(bytes_in_group == 1) { out[pos + 2] = '='; out[pos + 3] = '='; }
            else if(bytes_in_group == 2) { out[pos + 3] = '='; }
            pos += 4;
        }
        out[pos] = 0;
        return Result.Ok(encoded_len);

        }
    }

public func base64_decode(b64 : *char, b64_len : size_t, out : *mut u8, out_len : size_t) : Result<size_t, CryptoError> {
        unsafe {
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
            var group_start = i;
            var c0 = base64_val(b64[i]); i += 1;
            var c1 = base64_val(b64[i]); i += 1;
            var c2 : int = 0; if(i < effective_len) { c2 = base64_val(b64[i]); i += 1; }
            var c3 : int = 0; if(i < effective_len) { c3 = base64_val(b64[i]); i += 1; }
            if(c0 < 0 || c1 < 0) { return Result.Err(CryptoError.InvalidInput()); }
            var triple = ((c0 as u32) << 18) | ((c1 as u32) << 12);
            var chars_in_group = i - group_start;
            if(chars_in_group >= 3) {
                if(c2 < 0) { return Result.Err(CryptoError.InvalidInput()); }
                triple |= (c2 as u32) << 6;
            }
            if(chars_in_group >= 4) {
                if(c3 < 0) { return Result.Err(CryptoError.InvalidInput()); }
                triple |= c3 as u32;
            }
            out[pos] = ((triple >> 16) & 0xFF) as u8; pos += 1;
            if(chars_in_group >= 3) { out[pos] = ((triple >> 8) & 0xFF) as u8; pos += 1; }
            if(chars_in_group >= 4) { out[pos] = (triple & 0xFF) as u8; pos += 1; }
        }
        return Result.Ok(decoded_len);

        }
    }

const SHA256_DIGEST_LENGTH = 32;
const SHA256_BLOCK_SIZE = 64;

func rr(x : u32, n : u32) : u32 {
        unsafe {     return (x >> n) | (x << (32 - n));
        }
    }

public struct Sha256Context {
    var state : [8]u32;
    var count : u64;
    var buffer : [64]u8;
    var buffer_pos : size_t;
}

public func sha256_init(ctx : *mut Sha256Context) {
        unsafe {
        ctx.state[0] = 0x6A09E667u32;
        ctx.state[1] = 0xBB67AE85u32;
        ctx.state[2] = 0x3C6EF372u32;
        ctx.state[3] = 0xA54FF53Au32;
        ctx.state[4] = 0x510E527Fu32;
        ctx.state[5] = 0x9B05688Cu32;
        ctx.state[6] = 0x1F83D9ABu32;
        ctx.state[7] = 0x5BE0CD19u32;
        ctx.count = 0;
        ctx.buffer_pos = 0;

        }
    }

public func sha256_update(ctx : *mut Sha256Context, data : *u8, data_len : size_t) {
        unsafe {
        ctx.count += data_len as u64;
        var pos : size_t = 0;
        var space = 64 - ctx.buffer_pos;
        if(data_len >= space) {
        var i : size_t = 0;
        while(i < space) {
            ctx.buffer[ctx.buffer_pos + i] = data[pos + i]; i += 1; }
            pos += space;
            sha256_transform(ctx, &raw ctx.buffer[0]);
            ctx.buffer_pos = 0;
            while(pos + 63 < data_len) {
                sha256_transform(ctx, data + pos);
                pos += 64; }
        }
        var remaining = data_len - pos;
        var i : size_t = 0;
        while(i < remaining) {
            ctx.buffer[ctx.buffer_pos + i] = data[pos + i]; i += 1; }
        ctx.buffer_pos += remaining;

        }
    }

public func sha256_final(ctx : *mut Sha256Context, digest : *mut u8) {
        unsafe {
        var bits = ctx.count * 8;
        ctx.buffer[ctx.buffer_pos] = 0x80;
        ctx.buffer_pos += 1;
        var pad_len : size_t = 0;
        if(ctx.buffer_pos <= 56) { pad_len = 56 - ctx.buffer_pos; }
        else { pad_len = 64 - ctx.buffer_pos + 56; }
        var i : size_t = 0;
        while(i < pad_len) {
            if(ctx.buffer_pos < 64) {
                ctx.buffer[ctx.buffer_pos] = 0; ctx.buffer_pos += 1; }
            i += 1; }
        if(ctx.buffer_pos == 64) {
            sha256_transform(ctx, &raw ctx.buffer[0]);
            ctx.buffer_pos = 0;
            var j : size_t = 0;
            while(j < 56) { ctx.buffer[ctx.buffer_pos] = 0; ctx.buffer_pos += 1; j += 1; }
        }
        ctx.buffer[56] = ((bits >> 56) & 0xFFu64) as u8;
        ctx.buffer[57] = ((bits >> 48) & 0xFFu64) as u8;
        ctx.buffer[58] = ((bits >> 40) & 0xFFu64) as u8;
        ctx.buffer[59] = ((bits >> 32) & 0xFFu64) as u8;
        ctx.buffer[60] = ((bits >> 24) & 0xFFu64) as u8;
        ctx.buffer[61] = ((bits >> 16) & 0xFFu64) as u8;
        ctx.buffer[62] = ((bits >> 8) & 0xFFu64) as u8;
        ctx.buffer[63] = (bits & 0xFFu64) as u8;
        sha256_transform(ctx, &raw ctx.buffer[0]);
        var j2 : size_t = 0;
        while(j2 < 8) {
            var s = ctx.state[j2];
            digest[j2 * 4] = ((s >> 24) & 0xFFu32) as u8;
            digest[j2 * 4 + 1] = ((s >> 16) & 0xFFu32) as u8;
            digest[j2 * 4 + 2] = ((s >> 8) & 0xFFu32) as u8;
            digest[j2 * 4 + 3] = (s & 0xFFu32) as u8;
            j2 += 1; }

        }
    }

public func sha256_hash(data : *u8, data_len : size_t, digest : *mut u8) {
        unsafe {
        var ctx : Sha256Context;
        sha256_init(unsafe(&raw mut ctx));
        sha256_update(unsafe(&raw mut ctx), data, data_len);
        sha256_final(unsafe(&raw mut ctx), digest);

        }
    }

func sha256_transform(ctx : *mut Sha256Context, block : *u8) {
        unsafe {
        var w : [64]u32;
        var t : size_t = 0;
        while(t < 16) {
            var b0 = block[t*4] as u32; var b1 = block[t*4+1] as u32;
            var b2 = block[t*4+2] as u32; var b3 = block[t*4+3] as u32;
            w[t] = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
            t += 1; }
        while(t < 64) {
            var s0 = rr(w[t-15], 7) ^ rr(w[t-15], 18) ^ (w[t-15] >> 3);
            var s1 = rr(w[t-2], 17) ^ rr(w[t-2], 19) ^ (w[t-2] >> 10);
            w[t] = w[t-16] + s0 + w[t-7] + s1;
            t += 1; }
        var a = ctx.state[0]; var b = ctx.state[1];
        var c = ctx.state[2]; var d = ctx.state[3];
        var e = ctx.state[4]; var f = ctx.state[5];
        var g = ctx.state[6]; var h = ctx.state[7];
        t = 0;
        while(t < 64) {
            var S1 = rr(e, 6) ^ rr(e, 11) ^ rr(e, 25);
            var ch = (e & f) ^ ((~e) & g);
            var temp1 = h + S1 + ch + SHA256_K(t) + w[t];
            var S0 = rr(a, 2) ^ rr(a, 13) ^ rr(a, 22);
            var maj = (a & b) ^ (a & c) ^ (b & c);
            var temp2 = S0 + maj;
            h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
            t += 1; }
        ctx.state[0] += a; ctx.state[1] += b;
        ctx.state[2] += c; ctx.state[3] += d;
        ctx.state[4] += e; ctx.state[5] += f;
        ctx.state[6] += g; ctx.state[7] += h;

        }
    }

func SHA256_K(t : size_t) : u32 {
        unsafe {
        if(t == 0) { return 0x428A2F98u32; } if(t == 1) { return 0x71374491u32; }
        if(t == 2) { return 0xB5C0FBCFu32; } if(t == 3) { return 0xE9B5DBA5u32; }
        if(t == 4) { return 0x3956C25Bu32; } if(t == 5) { return 0x59F111F1u32; }
        if(t == 6) { return 0x923F82A4u32; } if(t == 7) { return 0xAB1C5ED5u32; }
        if(t == 8) { return 0xD807AA98u32; } if(t == 9) { return 0x12835B01u32; }
        if(t == 10) { return 0x243185BEu32; } if(t == 11) { return 0x550C7DC3u32; }
        if(t == 12) { return 0x72BE5D74u32; } if(t == 13) { return 0x80DEB1FEu32; }
        if(t == 14) { return 0x9BDC06A7u32; } if(t == 15) { return 0xC19BF174u32; }
        if(t == 16) { return 0xE49B69C1u32; } if(t == 17) { return 0xEFBE4786u32; }
        if(t == 18) { return 0x0FC19DC6u32; } if(t == 19) { return 0x240CA1CCu32; }
        if(t == 20) { return 0x2DE92C6Fu32; } if(t == 21) { return 0x4A7484AAu32; }
        if(t == 22) { return 0x5CB0A9DCu32; } if(t == 23) { return 0x76F988DAu32; }
        if(t == 24) { return 0x983E5152u32; } if(t == 25) { return 0xA831C66Du32; }
        if(t == 26) { return 0xB00327C8u32; } if(t == 27) { return 0xBF597FC7u32; }
        if(t == 28) { return 0xC6E00BF3u32; } if(t == 29) { return 0xD5A79147u32; }
        if(t == 30) { return 0x06CA6351u32; } if(t == 31) { return 0x14292967u32; }
        if(t == 32) { return 0x27B70A85u32; } if(t == 33) { return 0x2E1B2138u32; }
        if(t == 34) { return 0x4D2C6DFCu32; } if(t == 35) { return 0x53380D13u32; }
        if(t == 36) { return 0x650A7354u32; } if(t == 37) { return 0x766A0ABBu32; }
        if(t == 38) { return 0x81C2C92Eu32; } if(t == 39) { return 0x92722C85u32; }
        if(t == 40) { return 0xA2BFE8A1u32; } if(t == 41) { return 0xA81A664Bu32; }
        if(t == 42) { return 0xC24B8B70u32; } if(t == 43) { return 0xC76C51A3u32; }
        if(t == 44) { return 0xD192E819u32; } if(t == 45) { return 0xD6990624u32; }
        if(t == 46) { return 0xF40E3585u32; } if(t == 47) { return 0x106AA070u32; }
        if(t == 48) { return 0x19A4C116u32; } if(t == 49) { return 0x1E376C08u32; }
        if(t == 50) { return 0x2748774Cu32; } if(t == 51) { return 0x34B0BCB5u32; }
        if(t == 52) { return 0x391C0CB3u32; } if(t == 53) { return 0x4ED8AA4Au32; }
        if(t == 54) { return 0x5B9CCA4Fu32; } if(t == 55) { return 0x682E6FF3u32; }
        if(t == 56) { return 0x748F82EEu32; } if(t == 57) { return 0x78A5636Fu32; }
        if(t == 58) { return 0x84C87814u32; } if(t == 59) { return 0x8CC70208u32; }
        if(t == 60) { return 0x90BEFFFAu32; } if(t == 61) { return 0xA4506CEBu32; }
        if(t == 62) { return 0xBEF9A3F7u32; } if(t == 63) { return 0xC67178F2u32; }
        return 0;

        }
    }

const SHA512_DIGEST_LENGTH = 64;
const SHA384_DIGEST_LENGTH = 48;
const SHA512_BLOCK_SIZE = 128;

func rr64(x : u64, n : u64) : u64 {
        unsafe {     return (x >> n) | (x << (64 - n));
        }
    }

public struct Sha512Context {
    var state : [8]u64;
    var count : u64;          // total bytes fed (input never exceeds 2^64)
    var buffer : [128]u8;
    var buffer_pos : size_t;
}

// SHA-512 initial hash values (FIPS 180-4 §5.3.5).
func sha512_init_state(ctx : *mut Sha512Context) {
        unsafe {
        ctx.state[0] = 0x6A09E667F3BCC908u64;
        ctx.state[1] = 0xBB67AE8584CAA73Bu64;
        ctx.state[2] = 0x3C6EF372FE94F82Bu64;
        ctx.state[3] = 0xA54FF53A5F1D36F1u64;
        ctx.state[4] = 0x510E527FADE682D1u64;
        ctx.state[5] = 0x9B05688C2B3E6C1Fu64;
        ctx.state[6] = 0x1F83D9ABFB41BD6Bu64;
        ctx.state[7] = 0x5BE0CD19137E2179u64;

        }
    }

// SHA-384 initial hash values (FIPS 180-4 §5.3.4).
func sha384_init_state(ctx : *mut Sha512Context) {
        unsafe {
        ctx.state[0] = 0xCBBB9D5DC1059ED8u64;
        ctx.state[1] = 0x629A292A367CD507u64;
        ctx.state[2] = 0x9159015A3070DD17u64;
        ctx.state[3] = 0x152FECD8F70E5939u64;
        ctx.state[4] = 0x67332667FFC00B31u64;
        ctx.state[5] = 0x8EB44A8768581511u64;
        ctx.state[6] = 0xDB0C2E0D64F98FA7u64;
        ctx.state[7] = 0x47B5481DBEFA4FA4u64;

        }
    }

func sha512_ctx_init(ctx : *mut Sha512Context, init_sha384 : bool) {
        unsafe {
        if(init_sha384) {
            sha384_init_state(ctx)
        } else {
            sha512_init_state(ctx)
        }
        ctx.count = 0;
        ctx.buffer_pos = 0;

        }
    }

public func sha512_init(ctx : *mut Sha512Context) {
        unsafe {
        sha512_ctx_init(ctx, false)

        }
    }

public func sha384_init(ctx : *mut Sha512Context) {
        unsafe {
        sha512_ctx_init(ctx, true)

        }
    }

// Feed data into the context. sha384 and sha512 share the same buffering.
func sha512_update_impl(ctx : *mut Sha512Context, data : *u8, data_len : size_t) {
        unsafe {
        ctx.count = ctx.count + (data_len as u64);
        var pos : size_t = 0;
        var space = 128 - ctx.buffer_pos;
        if(data_len >= space) {
            var i : size_t = 0;
            while(i < space) {
                ctx.buffer[ctx.buffer_pos + i] = data[pos + i]; i += 1; }
            pos += space;
            sha512_transform(ctx, &raw ctx.buffer[0]);
            ctx.buffer_pos = 0;
            while(pos + 127 < data_len) {
                sha512_transform(ctx, data + pos);
                pos += 128; }
        }
        var remaining = data_len - pos;
        var i : size_t = 0;
        while(i < remaining) {
            ctx.buffer[ctx.buffer_pos + i] = data[pos + i]; i += 1; }
        ctx.buffer_pos += remaining;

        }
    }

public func sha512_update(ctx : *mut Sha512Context, data : *u8, data_len : size_t) {
        unsafe {
        sha512_update_impl(ctx, data, data_len)

        }
    }

public func sha384_update(ctx : *mut Sha512Context, data : *u8, data_len : size_t) {
        unsafe {
        sha512_update_impl(ctx, data, data_len)

        }
    }

// Finalize. `digest_len` is 64 for SHA-512 or 48 for SHA-384.
func sha512_final_impl(ctx : *mut Sha512Context, digest : *mut u8, digest_len : usize) {
        unsafe {
        var bits = (ctx.count as u64) * 8u64;

        // Pad: append 0x80 then zeros until length ≡ 112 mod 128, then the 128-bit
        // big-endian bit count.
        ctx.buffer[ctx.buffer_pos] = 0x80;
        ctx.buffer_pos += 1;
        var pad_len : size_t = 0;
        if(ctx.buffer_pos <= 112) { pad_len = 112 - ctx.buffer_pos; }
        else { pad_len = 128 - ctx.buffer_pos + 112; }
        var i : size_t = 0;
        while(i < pad_len) {
            if(ctx.buffer_pos < 128) {
                ctx.buffer[ctx.buffer_pos] = 0; ctx.buffer_pos += 1; }
            i += 1; }
        if(ctx.buffer_pos == 128) {
            sha512_transform(ctx, &raw ctx.buffer[0]);
            ctx.buffer_pos = 0;
            var j : size_t = 0;
            while(j < 112) { ctx.buffer[ctx.buffer_pos] = 0; ctx.buffer_pos += 1; j += 1; }
        }
        // 128-bit big-endian length: high 64 bits at buffer[112..119], low at [120..127].
        // For inputs < 2^64 bytes the high word is always zero.
        var hi : u64 = 0
        var lo = bits
        var k : size_t = 0;
        while(k < 8) {
            ctx.buffer[112 + k] = ((hi >> (56 - (k * 8))) & 0xFFu64) as u8;
            k += 1; }
        while(k < 16) {
            ctx.buffer[120 + (k - 8)] = ((lo >> (56 - ((k - 8) * 8))) & 0xFFu64) as u8;
            k += 1; }
        sha512_transform(ctx, &raw ctx.buffer[0]);

        // Only write `digest_len` bytes. SHA-384 truncates the 64-byte state to
        // its first 48 bytes, and callers only guarantee `digest_len` bytes of
        // output space (e.g. a [48]u8 for SHA-384). Writing all 8 words — or
        // zeroing the tail past digest_len — overflows the caller's buffer.
        var words : size_t = digest_len / 8u;
        var j2 : size_t = 0;
        while(j2 < words) {
            var s = ctx.state[j2];
            digest[j2 * 8] = ((s >> 56) & 0xFFu64) as u8;
            digest[j2 * 8 + 1] = ((s >> 48) & 0xFFu64) as u8;
            digest[j2 * 8 + 2] = ((s >> 40) & 0xFFu64) as u8;
            digest[j2 * 8 + 3] = ((s >> 32) & 0xFFu64) as u8;
            digest[j2 * 8 + 4] = ((s >> 24) & 0xFFu64) as u8;
            digest[j2 * 8 + 5] = ((s >> 16) & 0xFFu64) as u8;
            digest[j2 * 8 + 6] = ((s >> 8) & 0xFFu64) as u8;
            digest[j2 * 8 + 7] = (s & 0xFFu64) as u8;
            j2 += 1; }

        }
    }

public func sha512_final(ctx : *mut Sha512Context, digest : *mut u8) {
        unsafe {
        sha512_final_impl(ctx, digest, 64u)

        }
    }

public func sha384_final(ctx : *mut Sha512Context, digest : *mut u8) {
        unsafe {
        sha512_final_impl(ctx, digest, 48u)

        }
    }

public func sha512_hash(data : *u8, data_len : size_t, digest : *mut u8) {
        unsafe {
        var ctx : Sha512Context;
        sha512_init(unsafe(&raw mut ctx));
        sha512_update(unsafe(&raw mut ctx), data, data_len);
        sha512_final(unsafe(&raw mut ctx), digest);

        }
    }

public func sha384_hash(data : *u8, data_len : size_t, digest : *mut u8) {
        unsafe {
        var ctx : Sha512Context;
        sha384_init(unsafe(&raw mut ctx));
        sha384_update(unsafe(&raw mut ctx), data, data_len);
        sha384_final(unsafe(&raw mut ctx), digest);

        }
    }

// One SHA-512 compression round on a 128-byte block.
func sha512_transform(ctx : *mut Sha512Context, block : *u8) {
        unsafe {
        var w : [80]u64;
        var t : size_t = 0;
        while(t < 16) {
            var b0 = block[t*8] as u64; var b1 = block[t*8+1] as u64;
            var b2 = block[t*8+2] as u64; var b3 = block[t*8+3] as u64;
            var b4 = block[t*8+4] as u64; var b5 = block[t*8+5] as u64;
            var b6 = block[t*8+6] as u64; var b7 = block[t*8+7] as u64;
            w[t] = (b0 << 56) | (b1 << 48) | (b2 << 40) | (b3 << 32) |
                   (b4 << 24) | (b5 << 16) | (b6 << 8) | b7;
            t += 1; }
        while(t < 80) {
            var s0 = rr64(w[t-15], 1) ^ rr64(w[t-15], 8) ^ (w[t-15] >> 7);
            var s1 = rr64(w[t-2], 19) ^ rr64(w[t-2], 61) ^ (w[t-2] >> 6);
            w[t] = w[t-16] + s0 + w[t-7] + s1;
            t += 1; }
        var a = ctx.state[0]; var b = ctx.state[1];
        var c = ctx.state[2]; var d = ctx.state[3];
        var e = ctx.state[4]; var f = ctx.state[5];
        var g = ctx.state[6]; var h = ctx.state[7];
        t = 0;
        while(t < 80) {
            var S1 = rr64(e, 14) ^ rr64(e, 18) ^ rr64(e, 41);
            var ch = (e & f) ^ ((~e) & g);
            var temp1 = h + S1 + ch + SHA512_K(t) + w[t];
            var S0 = rr64(a, 28) ^ rr64(a, 34) ^ rr64(a, 39);
            var maj = (a & b) ^ (a & c) ^ (b & c);
            var temp2 = S0 + maj;
            h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
            t += 1; }
        ctx.state[0] += a; ctx.state[1] += b;
        ctx.state[2] += c; ctx.state[3] += d;
        ctx.state[4] += e; ctx.state[5] += f;
        ctx.state[6] += g; ctx.state[7] += h;

        }
    }

func SHA512_K(t : size_t) : u64 {
        unsafe {
        var k : [80]u64 = [
            0x428A2F98D728AE22u64, 0x7137449123EF65CDu64,
            0xB5C0FBCFEC4D3B2Fu64, 0xE9B5DBA58189DBBCu64,
            0x3956C25BF348B538u64, 0x59F111F1B605D019u64,
            0x923F82A4AF194F9Bu64, 0xAB1C5ED5DA6D8118u64,
            0xD807AA98A3030242u64, 0x12835B0145706FBEu64,
            0x243185BE4EE4B28Cu64, 0x550C7DC3D5FFB4E2u64,
            0x72BE5D74F27B896Fu64, 0x80DEB1FE3B1696B1u64,
            0x9BDC06A725C71235u64, 0xC19BF174CF692694u64,
            0xE49B69C19EF14AD2u64, 0xEFBE4786384F25E3u64,
            0x0FC19DC68B8CD5B5u64, 0x240CA1CC77AC9C65u64,
            0x2DE92C6F592B0275u64, 0x4A7484AA6EA6E483u64,
            0x5CB0A9DCBD41FBD4u64, 0x76F988DA831153B5u64,
            0x983E5152EE66DFABu64, 0xA831C66D2DB43210u64,
            0xB00327C898FB213Fu64, 0xBF597FC7BEEF0EE4u64,
            0xC6E00BF33DA88FC2u64, 0xD5A79147930AA725u64,
            0x06CA6351E003826Fu64, 0x142929670A0E6E70u64,
            0x27B70A8546D22FFCu64, 0x2E1B21385C26C926u64,
            0x4D2C6DFC5AC42AEDu64, 0x53380D139D95B3DFu64,
            0x650A73548BAF63DEu64, 0x766A0ABB3C77B2A8u64,
            0x81C2C92E47EDAEE6u64, 0x92722C851482353Bu64,
            0xA2BFE8A14CF10364u64, 0xA81A664BBC423001u64,
            0xC24B8B70D0F89791u64, 0xC76C51A30654BE30u64,
            0xD192E819D6EF5218u64, 0xD69906245565A910u64,
            0xF40E35855771202Au64, 0x106AA07032BBD1B8u64,
            0x19A4C116B8D2D0C8u64, 0x1E376C085141AB53u64,
            0x2748774CDF8EEB99u64, 0x34B0BCB5E19B48A8u64,
            0x391C0CB3C5C95A63u64, 0x4ED8AA4AE3418ACBu64,
            0x5B9CCA4F7763E373u64, 0x682E6FF3D6B2B8A3u64,
            0x748F82EE5DEFB2FCu64, 0x78A5636F43172F60u64,
            0x84C87814A1F0AB72u64, 0x8CC702081A6439ECu64,
            0x90BEFFFA23631E28u64, 0xA4506CEBDE82BDE9u64,
            0xBEF9A3F7B2C67915u64, 0xC67178F2E372532Bu64,
            0xCA273ECEEA26619Cu64, 0xD186B8C721C0C207u64,
            0xEADA7DD6CDE0EB1Eu64, 0xF57D4F7FEE6ED178u64,
            0x06F067AA72176FBAu64, 0x0A637DC5A2C898A6u64,
            0x113F9804BEF90DAEu64, 0x1B710B35131C471Bu64,
            0x28DB77F523047D84u64, 0x32CAAB7B40C72493u64,
            0x3C9EBE0A15C9BEBCu64, 0x431D67C49C100D4Cu64,
            0x4CC5D4BECB3E42B6u64, 0x597F299CFC657E2Au64,
            0x5FCB6FAB3AD6FAECu64, 0x6C44198C4A475817u64
        ]
        return k[t]

        }
    }

const MD5_DIGEST_LENGTH = 16;
const S11 : u32 = 7; const S12 : u32 = 12; const S13 : u32 = 17; const S14 : u32 = 22;
const S21 : u32 = 5; const S22 : u32 = 9; const S23 : u32 = 14; const S24 : u32 = 20;
const S31 : u32 = 4; const S32 : u32 = 11; const S33 : u32 = 16; const S34 : u32 = 23;
const S41 : u32 = 6; const S42 : u32 = 10; const S43 : u32 = 15; const S44 : u32 = 21;

public struct Md5Context {
    var state : [4]u32;
    var count : [2]u32;
    var buffer : [64]u8;
    var buffer_pos : size_t;
}

func F(x : u32, y : u32, z : u32) : u32 {
        unsafe {     return (x & y) | ((~x) & z);
        }
    }
func G(x : u32, y : u32, z : u32) : u32 {
        unsafe {     return (x & z) | (y & (~z));
        }
    }
func H(x : u32, y : u32, z : u32) : u32 {
        unsafe {     return x ^ y ^ z;
        }
    }
func I(x : u32, y : u32, z : u32) : u32 {
        unsafe {     return y ^ (x | (~z));
        }
    }

func left_rotate(x : u32, n : u32) : u32 {
        unsafe {     return (x << n) | (x >> (32 - n));
        }
    }

func FF(a : *mut u32, b : u32, c : u32, d : u32, x : u32, s : u32, ac : u32) {
        unsafe {
        a[0] = a[0] + F(b, c, d) + x + ac;
        a[0] = left_rotate(a[0], s); a[0] = a[0] + b;

        }
    }

func GG(a : *mut u32, b : u32, c : u32, d : u32, x : u32, s : u32, ac : u32) {
        unsafe {
        a[0] = a[0] + G(b, c, d) + x + ac;
        a[0] = left_rotate(a[0], s); a[0] = a[0] + b;

        }
    }

func HH(a : *mut u32, b : u32, c : u32, d : u32, x : u32, s : u32, ac : u32) {
        unsafe {
        a[0] = a[0] + H(b, c, d) + x + ac;
        a[0] = left_rotate(a[0], s); a[0] = a[0] + b;

        }
    }

func II(a : *mut u32, b : u32, c : u32, d : u32, x : u32, s : u32, ac : u32) {
        unsafe {
        a[0] = a[0] + I(b, c, d) + x + ac;
        a[0] = left_rotate(a[0], s); a[0] = a[0] + b;

        }
    }

public func md5_init(ctx : *mut Md5Context) {
        unsafe {
        ctx.state[0] = 0x67452301u32;
        ctx.state[1] = 0xEFCDAB89u32;
        ctx.state[2] = 0x98BADCFEu32;
        ctx.state[3] = 0x10325476u32;
        ctx.count[0] = 0; ctx.count[1] = 0;
        ctx.buffer_pos = 0;

        }
    }

public func md5_update(ctx : *mut Md5Context, data : *u8, data_len : size_t) {
        unsafe {
        var new_count = ctx.count[0] + ((data_len as u32) << 3);
        if(new_count < ctx.count[0]) { ctx.count[1] += 1; }
        ctx.count[0] = new_count;
        ctx.count[1] += (data_len >> 29) as u32;
        var pos : size_t = 0;
        var space = 64 - ctx.buffer_pos;
        if(data_len >= space) {
        var i : size_t = 0;
        while(i < space) {
            ctx.buffer[ctx.buffer_pos + i] = data[pos + i]; i += 1;
        }
        pos += space;
            md5_transform(ctx, &raw ctx.buffer[0]);
            while(pos + 63 < data_len) {
                md5_transform(ctx, data + pos);
                pos += 64;
            }
            ctx.buffer_pos = 0;
        }
        var remaining = data_len - pos;
        var i : size_t = 0;
        while(i < remaining) {
            ctx.buffer[ctx.buffer_pos + i] = data[pos + i]; i += 1;
        }
        ctx.buffer_pos += remaining;

        }
    }

public func md5_final(ctx : *mut Md5Context, digest : *mut u8) {
        unsafe {
        var count_bits = ctx.count[0];
        var count_bits_high = ctx.count[1];
        var pad_len : size_t = 0;
        if(ctx.buffer_pos < 56) { pad_len = 56 - ctx.buffer_pos; }
        else { pad_len = 120 - ctx.buffer_pos; }
        var padding : [128]u8;
        var i : size_t = 0;
        while(i < pad_len) {
            if(i == 0) { padding[i] = 0x80; }
            else { padding[i] = 0; }
            i += 1;
        }
        md5_update(ctx, &raw padding[0], pad_len);
        var count_bytes : [8]u8;
        count_bytes[0] = (count_bits & 0xFFu32) as u8;
        count_bytes[1] = ((count_bits >> 8) & 0xFFu32) as u8;
        count_bytes[2] = ((count_bits >> 16) & 0xFFu32) as u8;
        count_bytes[3] = ((count_bits >> 24) & 0xFFu32) as u8;
        count_bytes[4] = (count_bits_high & 0xFFu32) as u8;
        count_bytes[5] = ((count_bits_high >> 8) & 0xFFu32) as u8;
        count_bytes[6] = ((count_bits_high >> 16) & 0xFFu32) as u8;
        count_bytes[7] = ((count_bits_high >> 24) & 0xFFu32) as u8;
        md5_update(ctx, &raw count_bytes[0], 8);
        var j : size_t = 0;
        while(j < 4) {
            var s = ctx.state[j];
            digest[j * 4]     = (s & 0xFFu32) as u8;
            digest[j * 4 + 1] = ((s >> 8) & 0xFFu32) as u8;
            digest[j * 4 + 2] = ((s >> 16) & 0xFFu32) as u8;
            digest[j * 4 + 3] = ((s >> 24) & 0xFFu32) as u8;
            j += 1;
        }

        }
    }

public func md5_hash(data : *u8, data_len : size_t, digest : *mut u8) {
        unsafe {
        var ctx : Md5Context;
        md5_init(unsafe(&raw mut ctx));
        md5_update(unsafe(&raw mut ctx), data, data_len);
        md5_final(unsafe(&raw mut ctx), digest);

        }
    }

func md5_transform(ctx : *mut Md5Context, block : *u8) {
        unsafe {
        var a = ctx.state[0]; var b = ctx.state[1];
        var c = ctx.state[2]; var d = ctx.state[3];
        var x : [16]u32;
        var j : size_t = 0;
        while(j < 16) {
            x[j] = (block[j*4] as u32) | ((block[j*4+1] as u32) << 8) | ((block[j*4+2] as u32) << 16) | ((block[j*4+3] as u32) << 24);
            j += 1;
        }
        // Round 1
        FF(&raw mut a, b, c, d, x[0],  S11, 0xD76AA478u32);
        FF(&raw mut d, a, b, c, x[1],  S12, 0xE8C7B756u32);
        FF(&raw mut c, d, a, b, x[2],  S13, 0x242070DBu32);
        FF(&raw mut b, c, d, a, x[3],  S14, 0xC1BDCEEEu32);
        FF(&raw mut a, b, c, d, x[4],  S11, 0xF57C0FAFu32);
        FF(&raw mut d, a, b, c, x[5],  S12, 0x4787C62Au32);
        FF(&raw mut c, d, a, b, x[6],  S13, 0xA8304613u32);
        FF(&raw mut b, c, d, a, x[7],  S14, 0xFD469501u32);
        FF(&raw mut a, b, c, d, x[8],  S11, 0x698098D8u32);
        FF(&raw mut d, a, b, c, x[9],  S12, 0x8B44F7AFu32);
        FF(&raw mut c, d, a, b, x[10], S13, 0xFFFF5BB1u32);
        FF(&raw mut b, c, d, a, x[11], S14, 0x895CD7BEu32);
        FF(&raw mut a, b, c, d, x[12], S11, 0x6B901122u32);
        FF(&raw mut d, a, b, c, x[13], S12, 0xFD987193u32);
        FF(&raw mut c, d, a, b, x[14], S13, 0xA679438Eu32);
        FF(&raw mut b, c, d, a, x[15], S14, 0x49B40821u32);
        // Round 2
        GG(&raw mut a, b, c, d, x[1],  S21, 0xF61E2562u32);
        GG(&raw mut d, a, b, c, x[6],  S22, 0xC040B340u32);
        GG(&raw mut c, d, a, b, x[11], S23, 0x265E5A51u32);
        GG(&raw mut b, c, d, a, x[0],  S24, 0xE9B6C7AAu32);
        GG(&raw mut a, b, c, d, x[5],  S21, 0xD62F105Du32);
        GG(&raw mut d, a, b, c, x[10], S22, 0x02441453u32);
        GG(&raw mut c, d, a, b, x[15], S23, 0xD8A1E681u32);
        GG(&raw mut b, c, d, a, x[4],  S24, 0xE7D3FBC8u32);
        GG(&raw mut a, b, c, d, x[9],  S21, 0x21E1CDE6u32);
        GG(&raw mut d, a, b, c, x[14], S22, 0xC33707D6u32);
        GG(&raw mut c, d, a, b, x[3],  S23, 0xF4D50D87u32);
        GG(&raw mut b, c, d, a, x[8],  S24, 0x455A14EDu32);
        GG(&raw mut a, b, c, d, x[13], S21, 0xA9E3E905u32);
        GG(&raw mut d, a, b, c, x[2],  S22, 0xFCEFA3F8u32);
        GG(&raw mut c, d, a, b, x[7],  S23, 0x676F02D9u32);
        GG(&raw mut b, c, d, a, x[12], S24, 0x8D2A4C8Au32);
        // Round 3
        HH(&raw mut a, b, c, d, x[5],  S31, 0xFFFA3942u32);
        HH(&raw mut d, a, b, c, x[8],  S32, 0x8771F681u32);
        HH(&raw mut c, d, a, b, x[11], S33, 0x6D9D6122u32);
        HH(&raw mut b, c, d, a, x[14], S34, 0xFDE5380Cu32);
        HH(&raw mut a, b, c, d, x[1],  S31, 0xA4BEEA44u32);
        HH(&raw mut d, a, b, c, x[4],  S32, 0x4BDECFA9u32);
        HH(&raw mut c, d, a, b, x[7],  S33, 0xF6BB4B60u32);
        HH(&raw mut b, c, d, a, x[10], S34, 0xBEBFBC70u32);
        HH(&raw mut a, b, c, d, x[13], S31, 0x289B7EC6u32);
        HH(&raw mut d, a, b, c, x[0],  S32, 0xEAA127FAu32);
        HH(&raw mut c, d, a, b, x[3],  S33, 0xD4EF3085u32);
        HH(&raw mut b, c, d, a, x[6],  S34, 0x04881D05u32);
        HH(&raw mut a, b, c, d, x[9],  S31, 0xD9D4D039u32);
        HH(&raw mut d, a, b, c, x[12], S32, 0xE6DB99E5u32);
        HH(&raw mut c, d, a, b, x[15], S33, 0x1FA27CF8u32);
        HH(&raw mut b, c, d, a, x[2],  S34, 0xC4AC5665u32);
        // Round 4
        II(&raw mut a, b, c, d, x[0],  S41, 0xF4292244u32);
        II(&raw mut d, a, b, c, x[7],  S42, 0x432AFF97u32);
        II(&raw mut c, d, a, b, x[14], S43, 0xAB9423A7u32);
        II(&raw mut b, c, d, a, x[5],  S44, 0xFC93A039u32);
        II(&raw mut a, b, c, d, x[12], S41, 0x655B59C3u32);
        II(&raw mut d, a, b, c, x[3],  S42, 0x8F0CCC92u32);
        II(&raw mut c, d, a, b, x[10], S43, 0xFFEFF47Du32);
        II(&raw mut b, c, d, a, x[1],  S44, 0x85845DD1u32);
        II(&raw mut a, b, c, d, x[8],  S41, 0x6FA87E4Fu32);
        II(&raw mut d, a, b, c, x[15], S42, 0xFE2CE6E0u32);
        II(&raw mut c, d, a, b, x[6],  S43, 0xA3014314u32);
        II(&raw mut b, c, d, a, x[13], S44, 0x4E0811A1u32);
        II(&raw mut a, b, c, d, x[4],  S41, 0xF7537E82u32);
        II(&raw mut d, a, b, c, x[11], S42, 0xBD3AF235u32);
        II(&raw mut c, d, a, b, x[2],  S43, 0x2AD7D2BBu32);
        II(&raw mut b, c, d, a, x[9],  S44, 0xEB86D391u32);
        ctx.state[0] += a; ctx.state[1] += b;
        ctx.state[2] += c; ctx.state[3] += d;

        }
    }

// ---------------------------------------------------------------------------
// HMAC-SHA256
// ---------------------------------------------------------------------------

/// Compute HMAC-SHA256.
/// key: secret key
/// data: message data
/// digest: output buffer (at least 32 bytes)
public func hmac_sha256(key : *u8, key_len : size_t, data : *u8, data_len : size_t, digest : *mut u8) {
        unsafe {
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
    }

// ---------------------------------------------------------------------------
// HMAC-SHA384 (SHA-512 family — 128-byte block size, 48-byte digest)
// ---------------------------------------------------------------------------

/// Compute HMAC-SHA384.
/// key: secret key
/// data: message data
/// digest: output buffer (at least 48 bytes)
public func hmac_sha384(key : *u8, key_len : size_t, data : *u8, data_len : size_t, digest : *mut u8) {
        unsafe {
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
    }

// ---------------------------------------------------------------------------
// HMAC-MD5 (legacy, cryptographically broken)
// ---------------------------------------------------------------------------

/// Compute HMAC-MD5 (legacy only, not for security).
public func hmac_md5(key : *u8, key_len : size_t, data : *u8, data_len : size_t, digest : *mut u8) {
        unsafe {
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
    }

// ---------------------------------------------------------------------------
// Constant-time comparison
// ---------------------------------------------------------------------------

/// Constant-time comparison of two byte arrays.
/// Returns true if they are equal.
/// Safe against timing side-channel attacks.
public func constant_time_equal(a : *u8, b : *u8, len : size_t) : bool {
        unsafe {
        var diff : u8 = 0;
        var i : size_t = 0;
        while(i < len) {
            diff = diff | (a[i] ^ b[i]);
            i += 1;
        }
        return diff == 0;

        }
    }

} // namespace raw

} // end namespace crypto
