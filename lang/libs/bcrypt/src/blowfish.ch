public namespace bcrypt {

public type BF_word = uint
public type BF_word_signed = int

public comptime const BF_N : u64 = 16

public struct BF_ctx {
    public var S : [4][256]BF_word
    // BF_N = 16, 16 + 2 => 18
    public var P : [18]BF_word
    
    @constructor
    public func new() {
        return BF_ctx {
            S : [],
            P : []
        }
    }
}

// Magic IV for 64 Blowfish encryptions
const BF_magic_w : [6]BF_word = [
    0x4F727068u, 0x65616E42u, 0x65686F6Cu,
    0x64657253u, 0x63727944u, 0x6F756274u
]

// BF_ROUND equivalent
func bf_round(ctx : &BF_ctx, L : &mut uint, R : &mut uint, n : int) {
    var tmp1 = (*L) & 0xFFu
    var tmp2 = ((*L) >> 8u) & 0xFFu
    var tmp3 = ((*L) >> 16u) & 0xFFu
    var tmp4 = ((*L) >> 24u) & 0xFFu
    
    var t1 = ctx.S[3][tmp1]
    var t2 = ctx.S[2][tmp2]
    var t3 = ctx.S[1][tmp3]
    t3 = t3 + ctx.S[0][tmp4]
    t3 = t3 ^ t2
    (*R) = (*R) ^ ctx.P[n + 1]
    t3 = t3 + t1
    (*R) = (*R) ^ t3
}

public func bf_encrypt(ctx : &BF_ctx, L : &mut uint, R : &mut uint) {
    (*L) = (*L) ^ ctx.P[0]
    bf_round(ctx, L, R, 0)
    bf_round(ctx, R, L, 1)
    bf_round(ctx, L, R, 2)
    bf_round(ctx, R, L, 3)
    bf_round(ctx, L, R, 4)
    bf_round(ctx, R, L, 5)
    bf_round(ctx, L, R, 6)
    bf_round(ctx, R, L, 7)
    bf_round(ctx, L, R, 8)
    bf_round(ctx, R, L, 9)
    bf_round(ctx, L, R, 10)
    bf_round(ctx, R, L, 11)
    bf_round(ctx, L, R, 12)
    bf_round(ctx, R, L, 13)
    bf_round(ctx, L, R, 14)
    bf_round(ctx, R, L, 15)
    var tmp = *R
    (*R) = *L
    (*L) = tmp ^ ctx.P[17]
}

// Internal Eksblowfish body
func bf_body(ctx : &mut BF_ctx) {
    var L = 0u
    var R = 0u
    for(var i = 0; i < 18; i += 2) {
        bf_encrypt(ctx, &mut L, &mut R)
        ctx.P[i] = L
        ctx.P[i+1] = R
    }
    for(var i = 0; i < 4; i++) {
        for(var j = 0; j < 256; j += 2) {
            bf_encrypt(ctx, &mut L, &mut R)
            ctx.S[i][j] = L
            ctx.S[i][j+1] = R
        }
    }
}

public func bf_set_key(key : std::string_view, expanded : [18]uint, initial : [18]uint, flags : u8) {
    var k_ptr = 0u
    var bug = (flags as uint) & 1u
    
    for(var i = 0; i < 18; i++) {
        var tmp : [2]uint = [0u, 0u]
        for(var j = 0; j < 4; j++) {
            tmp[0] <<= 8u
            var c = if(k_ptr < key.size()) key.get(k_ptr) as u8 else 0u8
            tmp[0] |= c as uint
            
            tmp[1] <<= 8u
            var sc = if(k_ptr < key.size()) key.get(k_ptr) as i8 else 0i8
            tmp[1] |= (sc as int) as uint
            
            if(k_ptr < key.size()) k_ptr++ else k_ptr = 0u
        }
        expanded[i] = tmp[bug]
        initial[i] = BF_P_INIT[i] ^ tmp[bug]
    }
}

const BF_ITOA64 = "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

func bf_encode(dst : &mut std::string, src : [6]uint, size : int) {
    var src_bytes = src as *u8
    var i = 0
    while(i < size) {
        var c1 = (*(src_bytes + i)) as uint; i++
        dst.append(BF_ITOA64[c1 >> 2u])
        c1 = (c1 & 0x03u) << 4u
        if(i >= size) {
            dst.append(BF_ITOA64[c1])
            break
        }
        
        var c2 = (*(src_bytes + i)) as uint; i++
        c1 |= c2 >> 4u
        dst.append(BF_ITOA64[c1])
        c1 = (c2 & 0x0fu) << 2u
        if(i >= size) {
            dst.append(BF_ITOA64[c1])
            break
        }
        
        c2 = (*(src_bytes + i)) as uint; i++
        c1 |= c2 >> 6u
        dst.append(BF_ITOA64[c1])
        dst.append(BF_ITOA64[c2 & 0x3fu])
    }
}

var BF_atoi64 : [96]u8 = [
    64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8, 0u8, 1u8,
    54u8, 55u8, 56u8, 57u8, 58u8, 59u8, 60u8, 61u8, 62u8, 63u8, 64u8, 64u8, 64u8, 64u8, 64u8, 64u8,
    64u8, 2u8, 3u8, 4u8, 5u8, 6u8, 7u8, 8u8, 9u8, 10u8, 11u8, 12u8, 13u8, 14u8, 15u8, 16u8,
    17u8, 18u8, 19u8, 20u8, 21u8, 22u8, 23u8, 24u8, 25u8, 26u8, 27u8, 64u8, 64u8, 64u8, 64u8, 64u8,
    64u8, 28u8, 29u8, 30u8, 31u8, 32u8, 33u8, 34u8, 35u8, 36u8, 37u8, 38u8, 39u8, 40u8, 41u8, 42u8,
    43u8, 44u8, 45u8, 46u8, 47u8, 48u8, 49u8, 50u8, 51u8, 52u8, 53u8, 64u8, 64u8, 64u8, 64u8, 64u8
]

func bf_decode(dst : [4]uint, src : std::string_view, size : int) : bool {
    var dptr = dst as *mut u8
    var sidx = 0u
    var count = 0
    
    while(count < size) {
        var c1 = if(sidx < src.size()) src.get(sidx++) else 0u8
        if(c1 < 0x20u8 || c1 >= 0x80u8) return false
        var b1 = BF_atoi64[c1 - 0x20u8]
        if(b1 > 63u8) return false
        
        var c2 = if(sidx < src.size()) src.get(sidx++) else 0u8
        if(c2 < 0x20u8 || c2 >= 0x80u8) return false
        var b2 = BF_atoi64[c2 - 0x20u8]
        if(b2 > 63u8) return false
        
        *(dptr + count) = (b1 << 2u) | ((b2 & 0x30u) >> 4u); count++
        if(count >= size) break
        
        var c3 = if(sidx < src.size()) src.get(sidx++) else 0u8
        if(c3 < 0x20u8 || c3 >= 0x80u8) return false
        var b3 = BF_atoi64[c3 - 0x20u8]
        if(b3 > 63u8) return false
        
        *(dptr + count) = ((b2 & 0x0Fu) << 4u) | ((b3 & 0x3Cu) >> 2u); count++
        if(count >= size) break
        
        var c4 = if(sidx < src.size()) src.get(sidx++) else 0u8
        if(c4 < 0x20u8 || c4 >= 0x80u8) return false
        var b4 = BF_atoi64[c4 - 0x20u8]
        if(b4 > 63u8) return false
        
        *(dptr + count) = ((b3 & 0x03u) << 6u) | b4; count++
    }
    return true
}

func bf_swap(x : []uint, count : int) {
    for(var i = 0; i < count; i++) {
        var tmp = x[i]
        tmp = (tmp << 16u) | (tmp >> 16u)
        x[i] = ((tmp & 0x00FF00FFu) << 8u) | ((tmp >> 8u) & 0x00FF00FFu)
    }
}

public func bf_crypt(key : std::string_view, setting : std::string_view) : std::string {
    if(setting.size() < 7u || setting.get(0) != '$' || setting.get(1) != '2') return std::string()
    
    var cost_val = (setting.get(4) - '0' as u8) * 10u8 + (setting.get(5) - '0' as u8)
    var count = 1u << cost_val
    
    var salt_binary : [4]uint = [0u, 0u, 0u, 0u]
    if(!bf_decode(salt_binary, setting.subview(7u, setting.size()), 16)) return std::string()
    bf_swap(salt_binary, 4)
    
    var expanded_key : [18]uint = [0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u]
    var ctx = BF_ctx.new()
    
    var subtype_flags = 0u8
    var st = setting.get(2)
    if(st == 'a') subtype_flags = 2u8
    else if(st == 'b') subtype_flags = 4u8
    else if(st == 'y') subtype_flags = 4u8
    else if(st == 'x') subtype_flags = 1u8
    
    bf_set_key(key, expanded_key, ctx.P, subtype_flags)
    
    for(var i = 0; i < 4; i++) {
        for(var j = 0; j < 256; j++) {
            ctx.S[i][j] = BF_S_INIT[i][j]
        }
    }
    
    var L = 0u; var R = 0u
    for(var i = 0; i < 18; i += 2) {
        L ^= salt_binary[i & 2]
        R ^= salt_binary[(i & 2) + 1]
        bf_encrypt(&ctx, &mut L, &mut R)
        ctx.P[i] = L
        ctx.P[i+1] = R
    }
    
    var sptr = 0
    while(sptr < 1024) {
        L ^= salt_binary[2]; R ^= salt_binary[3]
        bf_encrypt(&ctx, &mut L, &mut R)
        ctx.S[sptr/256][sptr%256] = L; sptr++
        ctx.S[sptr/256][sptr%256] = R; sptr++
        
        L ^= salt_binary[0]; R ^= salt_binary[1]
        bf_encrypt(&ctx, &mut L, &mut R)
        ctx.S[sptr/256][sptr%256] = L; sptr++
        ctx.S[sptr/256][sptr%256] = R; sptr++
    }
    
    while(count > 0u) {
        for(var i = 0; i < 18; i++) {
            ctx.P[i] ^= expanded_key[i]
        }
        
        bf_body(&mut ctx)
        
        for(var i = 0; i < 16; i += 4) {
            ctx.P[i] ^= salt_binary[0]; ctx.P[i+1] ^= salt_binary[1]
            ctx.P[i+2] ^= salt_binary[2]; ctx.P[i+3] ^= salt_binary[3]
        }
        ctx.P[16] ^= salt_binary[0]; ctx.P[17] ^= salt_binary[1]
        
        bf_body(&mut ctx)
        
        count--
    }
    
    var output_binary : [6]uint = [0u, 0u, 0u, 0u, 0u, 0u]
    for(var i = 0; i < 6; i += 2) {
        L = BF_magic_w[i]; R = BF_magic_w[i+1]
        for(var j = 0; j < 64; j++) {
            bf_encrypt(&ctx, &mut L, &mut R)
        }
        output_binary[i] = L; output_binary[i+1] = R
    }
    
    var res = std::string()
    res.append_view(setting.subview(0u, 29u))
    bf_swap(output_binary, 6)
    bf_encode(&mut res, output_binary, 23)
    
    return res
}

}
