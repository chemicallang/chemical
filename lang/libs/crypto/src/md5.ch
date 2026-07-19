// md5 — MD5 hash implementation (RFC 1321).
// NOTE: MD5 is cryptographically broken. For legacy compatibility only.

public namespace crypto {

using std::Result;

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

func F(x : u32, y : u32, z : u32) : u32 { return (x & y) | ((~x) & z); }
func G(x : u32, y : u32, z : u32) : u32 { return (x & z) | (y & (~z)); }
func H(x : u32, y : u32, z : u32) : u32 { return x ^ y ^ z; }
func I(x : u32, y : u32, z : u32) : u32 { return y ^ (x | (~z)); }

func left_rotate(x : u32, n : u32) : u32 { return (x << n) | (x >> (32 - n)); }

func FF(a : *mut u32, b : u32, c : u32, d : u32, x : u32, s : u32, ac : u32) {
    a[0] = a[0] + F(b, c, d) + x + ac;
    a[0] = left_rotate(a[0], s); a[0] = a[0] + b;
}

func GG(a : *mut u32, b : u32, c : u32, d : u32, x : u32, s : u32, ac : u32) {
    a[0] = a[0] + G(b, c, d) + x + ac;
    a[0] = left_rotate(a[0], s); a[0] = a[0] + b;
}

func HH(a : *mut u32, b : u32, c : u32, d : u32, x : u32, s : u32, ac : u32) {
    a[0] = a[0] + H(b, c, d) + x + ac;
    a[0] = left_rotate(a[0], s); a[0] = a[0] + b;
}

func II(a : *mut u32, b : u32, c : u32, d : u32, x : u32, s : u32, ac : u32) {
    a[0] = a[0] + I(b, c, d) + x + ac;
    a[0] = left_rotate(a[0], s); a[0] = a[0] + b;
}

public func md5_init(ctx : *mut Md5Context) {
    ctx.state[0] = 0x67452301u32;
    ctx.state[1] = 0xEFCDAB89u32;
    ctx.state[2] = 0x98BADCFEu32;
    ctx.state[3] = 0x10325476u32;
    ctx.count[0] = 0; ctx.count[1] = 0;
    ctx.buffer_pos = 0;
}

public func md5_update(ctx : *mut Md5Context, data : *u8, data_len : size_t) {
    var new_count = ctx.count[0] + ((data_len as u32) << 3);
    if(new_count < ctx.count[0]) { ctx.count[1] += 1; }
    ctx.count[0] = new_count;
    ctx.count[1] += (data_len >> 29) as u32;
    var pos : size_t = 0;
    var space = 64 - ctx.buffer_pos;
    if(data_len >= space) {
        var i : size_t = 0;
        while(i < space) {
            ctx.buffer[ctx.buffer_pos + i] = data[pos]; i += 1;
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

public func md5_final(ctx : *mut Md5Context, digest : *mut u8) {
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

public func md5_hash(data : *u8, data_len : size_t, digest : *mut u8) {
    var ctx : Md5Context;
    md5_init(&raw mut ctx);
    md5_update(&raw mut ctx, data, data_len);
    md5_final(&raw mut ctx, digest);
}

func md5_transform(ctx : *mut Md5Context, block : *u8) {
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

} // end namespace crypto
