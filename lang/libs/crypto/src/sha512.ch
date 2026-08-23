// sha512 — SHA-384 / SHA-512 hash implementation (FIPS 180-4).
// SHA-384 uses the same round function as SHA-512 with different initial
// state and a truncated (48-byte) digest.

public namespace crypto {

const SHA512_DIGEST_LENGTH = 64;
const SHA384_DIGEST_LENGTH = 48;
const SHA512_BLOCK_SIZE = 128;

func rr64(x : u64, n : u64) : u64 { return (x >> n) | (x << (64 - n)); }

public struct Sha512Context {
    var state : [8]u64;
    var count : u64;          // total bytes fed (input never exceeds 2^64)
    var buffer : [128]u8;
    var buffer_pos : size_t;
}

// SHA-512 initial hash values (FIPS 180-4 §5.3.5).
func sha512_init_state(ctx : *mut Sha512Context) {
    ctx.state[0] = 0x6A09E667F3BCC908u64;
    ctx.state[1] = 0xBB67AE8584CAA73Bu64;
    ctx.state[2] = 0x3C6EF372FE94F82Bu64;
    ctx.state[3] = 0xA54FF53A5F1D36F1u64;
    ctx.state[4] = 0x510E527FADE682D1u64;
    ctx.state[5] = 0x9B05688C2B3E6C1Fu64;
    ctx.state[6] = 0x1F83D9ABFB41BD6Bu64;
    ctx.state[7] = 0x5BE0CD19137E2179u64;
}

// SHA-384 initial hash values (FIPS 180-4 §5.3.4).
func sha384_init_state(ctx : *mut Sha512Context) {
    ctx.state[0] = 0xCBBB9D5DC1059ED8u64;
    ctx.state[1] = 0x629A292A367CD507u64;
    ctx.state[2] = 0x9159015A3070DD17u64;
    ctx.state[3] = 0x152FECD8F70E5939u64;
    ctx.state[4] = 0x67332667FFC00B31u64;
    ctx.state[5] = 0x8EB44A8768581511u64;
    ctx.state[6] = 0xDB0C2E0D64F98FA7u64;
    ctx.state[7] = 0x47B5481DBEFA4FA4u64;
}

func sha512_ctx_init(ctx : *mut Sha512Context, init_sha384 : bool) {
    if(init_sha384) {
        sha384_init_state(ctx)
    } else {
        sha512_init_state(ctx)
    }
    ctx.count = 0;
    ctx.buffer_pos = 0;
}

public func sha512_init(ctx : *mut Sha512Context) {
    sha512_ctx_init(ctx, false)
}

public func sha384_init(ctx : *mut Sha512Context) {
    sha512_ctx_init(ctx, true)
}

// Feed data into the context. sha384 and sha512 share the same buffering.
func sha512_update_impl(ctx : *mut Sha512Context, data : *u8, data_len : size_t) {
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

public func sha512_update(ctx : *mut Sha512Context, data : *u8, data_len : size_t) {
    sha512_update_impl(ctx, data, data_len)
}

public func sha384_update(ctx : *mut Sha512Context, data : *u8, data_len : size_t) {
    sha512_update_impl(ctx, data, data_len)
}

// Finalize. `digest_len` is 64 for SHA-512 or 48 for SHA-384.
func sha512_final_impl(ctx : *mut Sha512Context, digest : *mut u8, digest_len : usize) {
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

    var j2 : size_t = 0;
    while(j2 < 8) {
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

    // SHA-384 truncates to the first 48 bytes.
    if(digest_len < 64) {
        // zero out the tail so callers never read stale bytes
        var z = digest_len;
        while(z < 64) { digest[z] = 0; z += 1; }
    }
}

public func sha512_final(ctx : *mut Sha512Context, digest : *mut u8) {
    sha512_final_impl(ctx, digest, 64u)
}

public func sha384_final(ctx : *mut Sha512Context, digest : *mut u8) {
    sha512_final_impl(ctx, digest, 48u)
}

public func sha512_hash(data : *u8, data_len : size_t, digest : *mut u8) {
    unsafe var ctx : Sha512Context;
    sha512_init(&raw mut ctx);
    sha512_update(&raw mut ctx, data, data_len);
    sha512_final(&raw mut ctx, digest);
}

public func sha384_hash(data : *u8, data_len : size_t, digest : *mut u8) {
    unsafe var ctx : Sha512Context;
    sha384_init(&raw mut ctx);
    sha384_update(&raw mut ctx, data, data_len);
    sha384_final(&raw mut ctx, digest);
}

// One SHA-512 compression round on a 128-byte block.
func sha512_transform(ctx : *mut Sha512Context, block : *u8) {
    unsafe var w : [80]u64;
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

func SHA512_K(t : size_t) : u64 {
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

} // end namespace crypto