// sha256 — SHA-256 hash implementation (FIPS 180-4).

public namespace crypto {

using std::Result;

const SHA256_DIGEST_LENGTH = 32;
const SHA256_BLOCK_SIZE = 64;

func rr(x : u32, n : u32) : u32 { return (x >> n) | (x << (32 - n)); }

public struct Sha256Context {
    var state : [8]u32;
    var count : u64;
    var buffer : [64]u8;
    var buffer_pos : size_t;
}

public func sha256_init(ctx : *mut Sha256Context) {
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

public func sha256_update(ctx : *mut Sha256Context, data : *u8, data_len : size_t) {
    ctx.count += data_len as u64;
    var pos : size_t = 0;
    var space = 64 - ctx.buffer_pos;
    if(data_len >= space) {
        var i : size_t = 0;
        while(i < space) {
            ctx.buffer[ctx.buffer_pos + i] = data[pos]; i += 1; }
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

public func sha256_final(ctx : *mut Sha256Context, digest : *mut u8) {
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

public func sha256_hash(data : *u8, data_len : size_t, digest : *mut u8) {
    var ctx : Sha256Context;
    sha256_init(&raw mut ctx);
    sha256_update(&raw mut ctx, data, data_len);
    sha256_final(&raw mut ctx, digest);
}

func sha256_transform(ctx : *mut Sha256Context, block : *u8) {
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

func SHA256_K(t : size_t) : u32 {
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

} // end namespace crypto
