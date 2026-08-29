// ChaCha20 + Poly1305 AEAD (RFC 8439) for TLS 1.3 ChaCha20-Poly1305-SHA256.
// Pure-Chemical implementation, no external dependencies.

// ─── ChaCha20 ────────────────────────────────────────────────────────────────

func chacha20_rotl(x : u32, n : u32) : u32 {
    return (x << n) | (x >> (32u - n))
}

func chacha20_quarter_round(s : *mut u32, a : size_t, b : size_t, c : size_t, d : size_t) {
    s[a] = s[a] + s[b]; s[d] = s[d] ^ s[a]; s[d] = chacha20_rotl(s[d], 16u)
    s[c] = s[c] + s[d]; s[b] = s[b] ^ s[c]; s[b] = chacha20_rotl(s[b], 12u)
    s[a] = s[a] + s[b]; s[d] = s[d] ^ s[a]; s[d] = chacha20_rotl(s[d], 8u)
    s[c] = s[c] + s[d]; s[b] = s[b] ^ s[c]; s[b] = chacha20_rotl(s[b], 7u)
}

// Produce one 64-byte ChaCha20 keystream block.
// key: 32 bytes, counter: 32-bit block counter, nonce: 12 bytes.
func chacha20_block(key : *u8, counter : u32, nonce : *u8, out : *mut u8) {
    var state : [16]u32
    // "expand 32-byte k" constants
    state[0] = 0x61707865u
    state[1] = 0x3320646eu
    state[2] = 0x79622d32u
    state[3] = 0x6b206574u
    var i : size_t = 0
    while(i < 8) {
        state[4 + i] = ((key[4*i] as u32) |
                        ((key[4*i + 1] as u32) << 8) |
                        ((key[4*i + 2] as u32) << 16) |
                        ((key[4*i + 3] as u32) << 24))
        i += 1
    }
    state[12] = counter
    state[13] = ((nonce[0] as u32) | ((nonce[1] as u32) << 8) |
                ((nonce[2] as u32) << 16) | ((nonce[3] as u32) << 24))
    state[14] = ((nonce[4] as u32) | ((nonce[5] as u32) << 8) |
                ((nonce[6] as u32) << 16) | ((nonce[7] as u32) << 24))
    state[15] = ((nonce[8] as u32) | ((nonce[9] as u32) << 8) |
                ((nonce[10] as u32) << 16) | ((nonce[11] as u32) << 24))

    var w : [16]u32
    var j : size_t = 0
    while(j < 16) { w[j] = state[j]; j += 1 }

    var round : size_t = 0
    while(round < 10) {
        chacha20_quarter_round(&raw mut w[0], 0, 4, 8, 12)
        chacha20_quarter_round(&raw mut w[0], 1, 5, 9, 13)
        chacha20_quarter_round(&raw mut w[0], 2, 6, 10, 14)
        chacha20_quarter_round(&raw mut w[0], 3, 7, 11, 15)
        chacha20_quarter_round(&raw mut w[0], 0, 5, 10, 15)
        chacha20_quarter_round(&raw mut w[0], 1, 6, 11, 12)
        chacha20_quarter_round(&raw mut w[0], 2, 7, 8, 13)
        chacha20_quarter_round(&raw mut w[0], 3, 4, 9, 14)
        round += 1
    }

    j = 0
    while(j < 16) {
        var val = w[j] + state[j]
        out[4*j] = (val & 0xFFu) as u8
        out[4*j + 1] = ((val >> 8) & 0xFFu) as u8
        out[4*j + 2] = ((val >> 16) & 0xFFu) as u8
        out[4*j + 3] = ((val >> 24) & 0xFFu) as u8
        j += 1
    }
}

// XOR `len` bytes of `input` with ChaCha20 keystream starting at block `counter_start`.
func chacha20_xor(key : *u8, nonce : *u8, counter_start : u32,
                  input : *u8, len : size_t, output : *mut u8) {
    if(len == 0) { return }
    var block : [64]u8
    var pos : size_t = 0
    var ctr = counter_start
    while(pos < len) {
        chacha20_block(key, ctr, nonce, &raw mut block[0])
        var n : size_t = 64
        if(len - pos < 64) { n = len - pos }
        var k : size_t = 0
        while(k < n) {
            output[pos + k] = input[pos + k] ^ block[k]
            k += 1
        }
        pos += n
        ctr += 1u
    }
}

// ─── Poly1305 (RFC 8439 §2.5), 26-bit limb implementation ─────────────────────

public struct Poly1305State {
    var r : [5]u32
    var h : [5]u32
    var s : [16]u8
    var buf : [16]u8
    var buflen : size_t
}

func poly1305_load_r(state : *mut Poly1305State, key : *u8) {
    var k = key
    // Clamp r per RFC 8439 §2.5: r &= 0x0ffffffc0ffffffc0ffffffc0fffffff.
    state.r[0] = (k[0] as u32 | (k[1] as u32 << 8) | (k[2] as u32 << 16) | (k[3] as u32 << 24)) & 0x3ffffffu
    state.r[1] = ((k[3] as u32 >> 2) | (k[4] as u32 << 6) | (k[5] as u32 << 14) | (k[6] as u32 << 22)) & 0x3ffff03u
    state.r[2] = ((k[6] as u32 >> 4) | (k[7] as u32 << 4) | (k[8] as u32 << 12) | (k[9] as u32 << 20)) & 0x3ffc0ffu
    state.r[3] = ((k[9] as u32 >> 6) | (k[10] as u32 << 2) | (k[11] as u32 << 10) | (k[12] as u32 << 18)) & 0x3f03fffu
    state.r[4] = ((k[12] as u32 >> 8) | (k[13] as u32) | (k[14] as u32 << 8) | (k[15] as u32 << 16)) & 0x00fffffu
}

func poly1305_init(state : *mut Poly1305State, key : *u8) {
    poly1305_load_r(state, key)
    state.h[0] = 0u; state.h[1] = 0u; state.h[2] = 0u; state.h[3] = 0u; state.h[4] = 0u
    var i : size_t = 0
    while(i < 16) { state.s[i] = key[16 + i]; state.buf[i] = 0u as u8; i += 1 }
    state.buflen = 0
}

// Process one full (16-byte) or partial (final_len-byte) block into h.
func poly1305_process(state : *mut Poly1305State, block : *u8, is_final : bool, final_len : size_t) {
    var h = &raw mut state.h[0]
    h[0] = h[0] + ((block[0] as u32 | (block[1] as u32 << 8) | (block[2] as u32 << 16) | (block[3] as u32 << 24)) & 0x3ffffffu)
    h[1] = h[1] + (((block[3] as u32 >> 2) | (block[4] as u32 << 6) | (block[5] as u32 << 14) | (block[6] as u32 << 22)) & 0x3ffffffu)
    h[2] = h[2] + (((block[6] as u32 >> 4) | (block[7] as u32 << 4) | (block[8] as u32 << 12) | (block[9] as u32 << 20)) & 0x3ffffffu)
    h[3] = h[3] + (((block[9] as u32 >> 6) | (block[10] as u32 << 2) | (block[11] as u32 << 10) | (block[12] as u32 << 18)) & 0x3ffffffu)
    h[4] = h[4] + (((block[12] as u32 >> 8) | (block[13] as u32) | (block[14] as u32 << 8) | (block[15] as u32 << 16)) & 0x3ffffffu)

    // Append the 0x01 terminator at bit (8 * processed_len).
    var term_bit : size_t = 128
    if(is_final) { term_bit = 8 * final_len }
    var limb : size_t = term_bit / 26
    var bit : size_t = term_bit % 26
    h[limb] = h[limb] + (1u << bit)

    // h = h * r  (mod 2^130 - 5), 26-bit limbs.
    var r0 = state.r[0]; var r1 = state.r[1]; var r2 = state.r[2]
    var r3 = state.r[3]; var r4 = state.r[4]
    var t : [5]u64
    t[0] = (h[0] as u64) * (r0 as u64) + (h[1] as u64) * (5u64 * (r4 as u64)) + (h[2] as u64) * (5u64 * (r3 as u64)) + (h[3] as u64) * (5u64 * (r2 as u64)) + (h[4] as u64) * (5u64 * (r1 as u64))
    t[1] = (h[0] as u64) * (r1 as u64) + (h[1] as u64) * (r0 as u64) + (h[2] as u64) * (5u64 * (r4 as u64)) + (h[3] as u64) * (5u64 * (r3 as u64)) + (h[4] as u64) * (5u64 * (r2 as u64))
    t[2] = (h[0] as u64) * (r2 as u64) + (h[1] as u64) * (r1 as u64) + (h[2] as u64) * (r0 as u64) + (h[3] as u64) * (5u64 * (r4 as u64)) + (h[4] as u64) * (5u64 * (r3 as u64))
    t[3] = (h[0] as u64) * (r3 as u64) + (h[1] as u64) * (r2 as u64) + (h[2] as u64) * (r1 as u64) + (h[3] as u64) * (r0 as u64) + (h[4] as u64) * (5u64 * (r4 as u64))
    t[4] = (h[0] as u64) * (r4 as u64) + (h[1] as u64) * (r3 as u64) + (h[2] as u64) * (r2 as u64) + (h[3] as u64) * (r1 as u64) + (h[4] as u64) * (r0 as u64)

    var c : u64 = 0
    h[0] = (t[0] & 0x3ffffffu) as u32; c = t[0] >> 26
    t[1] = t[1] + c
    h[1] = (t[1] & 0x3ffffffu) as u32; c = t[1] >> 26
    t[2] = t[2] + c
    h[2] = (t[2] & 0x3ffffffu) as u32; c = t[2] >> 26
    t[3] = t[3] + c
    h[3] = (t[3] & 0x3ffffffu) as u32; c = t[3] >> 26
    t[4] = t[4] + c
    h[4] = (t[4] & 0x3ffffffu) as u32; c = t[4] >> 26
    h[0] = h[0] + ((c & 0xffffffffu) as u32 * 5u)
    c = (h[0] as u64) >> 26
    h[0] = (h[0] & 0x3ffffffu)
    h[1] = h[1] + (c as u32)
}

func poly1305_update(state : *mut Poly1305State, data : *u8, len : size_t) {
    var pos : size_t = 0
    if(state.buflen > 0) {
        while(pos < len && state.buflen < 16) {
            state.buf[state.buflen] = data[pos]
            state.buflen += 1
            pos += 1
        }
        if(state.buflen == 16) {
            poly1305_process(state, &raw state.buf[0], false, 0)
            state.buflen = 0
        }
    }
    while(pos + 16 <= len) {
        poly1305_process(state, data + pos, false, 0)
        pos += 16
    }
    while(pos < len) {
        state.buf[state.buflen] = data[pos]
        state.buflen += 1
        pos += 1
    }
}

func poly1305_finish(state : *mut Poly1305State, tag : *mut u8) {
    if(state.buflen > 0) {
        // Zero-fill and process the final partial block.
        var i : size_t = state.buflen
        while(i < 16) { state.buf[i] = 0u as u8; i += 1 }
        poly1305_process(state, &raw state.buf[0], true, state.buflen)
    }

    // Final reduction: conditionally subtract p = 2^130 - 5.
    var g : [5]u32
    var c : u32 = 0
    g[0] = state.h[0] + 5u; c = g[0] >> 26; g[0] = g[0] & 0x3ffffffu
    g[1] = state.h[1] + c; c = g[1] >> 26; g[1] = g[1] & 0x3ffffffu
    g[2] = state.h[2] + c; c = g[2] >> 26; g[2] = g[2] & 0x3ffffffu
    g[3] = state.h[3] + c; c = g[3] >> 26; g[3] = g[3] & 0x3ffffffu
    g[4] = state.h[4] + c - (1u << 26)

    // nb = 0 if h >= p (select g = h - p), else 0xffffffff (select h).
    var nb : u32 = 0u - (g[4] >> 31)
    var k : size_t = 0
    while(k < 5) {
        state.h[k] = g[k] ^ (nb & (state.h[k] ^ g[k]))
        k += 1
    }

    // Serialize h (little-endian, 26-bit limbs) into 16 bytes.
    var mac : [16]u8
    mac[0] = (state.h[0] & 0xFFu) as u8
    mac[1] = ((state.h[0] >> 8) & 0xFFu) as u8
    mac[2] = ((state.h[0] >> 16) & 0xFFu) as u8
    mac[3] = (((state.h[0] >> 24) | (state.h[1] << 2)) & 0xFFu) as u8
    mac[4] = ((state.h[1] >> 6) & 0xFFu) as u8
    mac[5] = ((state.h[1] >> 14) & 0xFFu) as u8
    mac[6] = (((state.h[1] >> 22) | (state.h[2] << 4)) & 0xFFu) as u8
    mac[7] = ((state.h[2] >> 4) & 0xFFu) as u8
    mac[8] = ((state.h[2] >> 12) & 0xFFu) as u8
    mac[9] = (((state.h[2] >> 20) | (state.h[3] << 6)) & 0xFFu) as u8
    mac[10] = ((state.h[3] >> 2) & 0xFFu) as u8
    mac[11] = ((state.h[3] >> 10) & 0xFFu) as u8
    mac[12] = ((state.h[3] >> 18) & 0xFFu) as u8
    mac[13] = (state.h[4] & 0xFFu) as u8
    mac[14] = ((state.h[4] >> 8) & 0xFFu) as u8
    mac[15] = ((state.h[4] >> 16) & 0xFFu) as u8

    // tag = (h + s) mod 2^128
    var f : u32 = 0u
    var m : size_t = 0
    while(m < 16) {
        var v = (mac[m] as u32) + (state.s[m] as u32) + f
        tag[m] = (v & 0xFFu) as u8
        f = v >> 8
        m += 1
    }
}

// ─── AEAD (RFC 8439 §2.8) ─────────────────────────────────────────────────────

// key: 32 bytes, nonce: 12 bytes.
func chacha20_poly1305_encrypt(key : *u8, nonce : *u8,
                               aad : *u8, aad_len : size_t,
                               plaintext : *u8, pt_len : size_t,
                               ciphertext : *mut u8, tag : *mut u8) {
    // One-time Poly1305 key = first 32 bytes of ChaCha20 keystream at counter 0.
    var otk_block : [64]u8
    chacha20_block(key, 0u, nonce, &raw mut otk_block[0])

    // Encrypt plaintext with keystream starting at counter 1.
    chacha20_xor(key, nonce, 1u, plaintext, pt_len, ciphertext)

    // Build Poly1305 MAC over aad || pad16(aad) || ciphertext || pad16(ct) || le64(aad_len) || le64(pt_len)
    var st : Poly1305State
    poly1305_init(unsafe(&raw mut st), &raw mut otk_block[0])
    poly1305_update(unsafe(&raw mut st), aad, aad_len)
    // Zero-pad AAD to a 16-byte boundary (RFC 8439 §2.8.1).
    var pad_aad = (16u - (aad_len % 16u)) % 16u
    if(pad_aad > 0u) {
        var zp : [16]u8
        var zi : size_t = 0
        while(zi < 16) { zp[zi] = 0u as u8; zi += 1 }
        poly1305_update(unsafe(&raw mut st), &raw zp[0], pad_aad)
    }
    poly1305_update(unsafe(&raw mut st), ciphertext, pt_len)
    // Zero-pad ciphertext to a 16-byte boundary.
    var pad_ct = (16u - (pt_len % 16u)) % 16u
    if(pad_ct > 0u) {
        var zp2 : [16]u8
        var zi2 : size_t = 0
        while(zi2 < 16) { zp2[zi2] = 0u as u8; zi2 += 1 }
        poly1305_update(unsafe(&raw mut st), &raw zp2[0], pad_ct)
    }
    var lens : [16]u8
    var i : size_t = 0
    while(i < 16) { lens[i] = 0u as u8; i += 1 }
    var aad_bits = aad_len as u64
    var pt_bits = pt_len as u64
    i = 0
    while(i < 8) { lens[i] = ((aad_bits >> (8 * i)) & 0xFFu) as u8; i += 1 }
    i = 0
    while(i < 8) { lens[8 + i] = ((pt_bits >> (8 * i)) & 0xFFu) as u8; i += 1 }
    poly1305_update(unsafe(&raw mut st), &raw mut lens[0], 16)
    poly1305_finish(unsafe(&raw mut st), tag)
}

// Returns 0 on success (tag verified), negative on authentication failure.
func chacha20_poly1305_decrypt(key : *u8, nonce : *u8,
                               aad : *u8, aad_len : size_t,
                               ciphertext : *u8, ct_len : size_t,
                               tag : *u8, plaintext : *mut u8) : int {
    var otk_block : [64]u8
    chacha20_block(key, 0u, nonce, &raw mut otk_block[0])

    var st : Poly1305State
    poly1305_init(unsafe(&raw mut st), &raw mut otk_block[0])
    poly1305_update(unsafe(&raw mut st), aad, aad_len)
    // Zero-pad AAD to a 16-byte boundary (RFC 8439 §2.8.1).
    var pad_aad = (16u - (aad_len % 16u)) % 16u
    if(pad_aad > 0u) {
        var zp : [16]u8
        var zi : size_t = 0
        while(zi < 16) { zp[zi] = 0u as u8; zi += 1 }
        poly1305_update(unsafe(&raw mut st), &raw zp[0], pad_aad)
    }
    poly1305_update(unsafe(&raw mut st), ciphertext, ct_len)
    // Zero-pad ciphertext to a 16-byte boundary.
    var pad_ct = (16u - (ct_len % 16u)) % 16u
    if(pad_ct > 0u) {
        var zp2 : [16]u8
        var zi2 : size_t = 0
        while(zi2 < 16) { zp2[zi2] = 0u as u8; zi2 += 1 }
        poly1305_update(unsafe(&raw mut st), &raw zp2[0], pad_ct)
    }
    var lens : [16]u8
    var i : size_t = 0
    while(i < 16) { lens[i] = 0u as u8; i += 1 }
    var aad_bits = aad_len as u64
    var ct_bits = ct_len as u64
    i = 0
    while(i < 8) { lens[i] = ((aad_bits >> (8 * i)) & 0xFFu) as u8; i += 1 }
    i = 0
    while(i < 8) { lens[8 + i] = ((ct_bits >> (8 * i)) & 0xFFu) as u8; i += 1 }
    poly1305_update(unsafe(&raw mut st), &raw mut lens[0], 16)
    var expected : [16]u8
    poly1305_finish(unsafe(&raw mut st), &raw mut expected[0])

    var diff : u8 = 0u
    i = 0
    while(i < 16) { diff = diff | (expected[i] ^ tag[i]); i += 1 }
    if(diff != 0u) { return -1 }

    chacha20_xor(key, nonce, 1u, ciphertext, ct_len, plaintext)
    return 0
}
