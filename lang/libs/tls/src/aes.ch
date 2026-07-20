// ============================================================================
// AES (Advanced Encryption Standard) — FIPS PUB 197
// ============================================================================
// Port of mbedTLS aes.h / aes.c to Chemical.
// Supports AES-128 and AES-256 with ECB and CBC modes.
// ============================================================================

public namespace tls {

    public comptime const AES_ERR_INVALID_KEY_LENGTH = -0x0020
    public comptime const AES_ERR_INVALID_INPUT_LENGTH = -0x0040

    // ─── AES Context ────────────────────────────────────────────────────────

    public struct AESContext {
        var nr : u32          // Number of rounds (10 for AES-128, 14 for AES-256)
        var rk : [68]u32      // Round key storage (60 for AES-256 + 8 spare)
        var buf : [16]u8      // Encryption/decryption buffer for CBC
    }

    public func aes_init(ctx : *mut AESContext) {
        ctx.nr = 0
        var i : size_t = 0
        while(i < 68) { ctx.rk[i] = 0; i += 1 }
        i = 0
        while(i < 16) { ctx.buf[i] = 0; i += 1 }
    }

    // ─── AES S-box and inverse S-box ───────────────────────────────────────

    var AES_SBOX : []u8 = [0x63 as u8, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,
        0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
        0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
        0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
        0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
        0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
        0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
        0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
        0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
        0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
        0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,
        0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
        0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
        0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
        0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
        0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
        0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,
        0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
        0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
        0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
        0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
        0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
        0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
        0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
        0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
        0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
        0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
        0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
        0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
        0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
        0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
        0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
    ]

    var AES_RSBOX : []u8 = [0x52 as u8, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38,
        0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
        0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87,
        0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
        0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D,
        0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
        0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2,
        0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
        0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,
        0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
        0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA,
        0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
        0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A,
        0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
        0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02,
        0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
        0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA,
        0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
        0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85,
        0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
        0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89,
        0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
        0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20,
        0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
        0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31,
        0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
        0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D,
        0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
        0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0,
        0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
        0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26,
        0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
    ]

    // ─── Rcon values for key expansion ──────────────────────────────────────

    func aes_rcon(i : size_t) : u32 {
        var rc : [10]u8 = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36]
        if(i < 10) { return rc[i] as u32 }
        return 0
    }

    // ─── AES Key Expansion ──────────────────────────────────────────────────

    func aes_key_expansion(ctx : *mut AESContext, key : *u8, key_len : size_t) : int {
        var nb : u32 = 4  // Number of columns (fixed for AES)
        var nk : u32 = 0  // Number of 32-bit words in the key
        var nr : u32 = 0  // Number of rounds

        if(key_len == 16) { nk = 4; nr = 10 }
        else if(key_len == 32) { nk = 8; nr = 14 }
        else { return AES_ERR_INVALID_KEY_LENGTH }

        ctx.nr = nr

        // Copy key into first Nk words of round keys
        var i : u32 = 0
        while(i < nk) {
            ctx.rk[i] = ((key[4*i] as u32) << 24) |
                        ((key[4*i+1] as u32) << 16) |
                        ((key[4*i+2] as u32) << 8) |
                        (key[4*i+3] as u32)
            i += 1
        }

        // Expand the key
        i = nk
        while(i < (nb * (nr + 1))) {
            var temp = ctx.rk[i - 1]
            if(i % nk == 0) {
                // RotWord + SubWord + Rcon
                temp = ((AES_SBOX[(temp >> 16) & 0xFF] as u32) << 24) |
                       ((AES_SBOX[(temp >> 8) & 0xFF] as u32) << 16) |
                       ((AES_SBOX[temp & 0xFF] as u32) << 8) |
                       (AES_SBOX[(temp >> 24) & 0xFF] as u32)
                temp = temp ^ (aes_rcon((i / nk) as size_t) << 24)
            } else if(nk > 6 && (i % nk) == 4) {
                // SubWord only (for AES-256)
                temp = ((AES_SBOX[(temp >> 24) & 0xFF] as u32) << 24) |
                       ((AES_SBOX[(temp >> 16) & 0xFF] as u32) << 16) |
                       ((AES_SBOX[(temp >> 8) & 0xFF] as u32) << 8) |
                       (AES_SBOX[temp & 0xFF] as u32)
            }
            ctx.rk[i] = ctx.rk[i - nk] ^ temp
            i += 1
        }

        return 0
    }

    // ─── AES ECB Encrypt (single block) ─────────────────────────────────────

    func aes_internal_encrypt(ctx : *mut AESContext, input : *u8, output : *mut u8) {
        var s0 : u32 = ((input[0] as u32) << 24) | ((input[1] as u32) << 16) |
                        ((input[2] as u32) << 8) | (input[3] as u32)
        var s1 : u32 = ((input[4] as u32) << 24) | ((input[5] as u32) << 16) |
                        ((input[6] as u32) << 8) | (input[7] as u32)
        var s2 : u32 = ((input[8] as u32) << 24) | ((input[9] as u32) << 16) |
                        ((input[10] as u32) << 8) | (input[11] as u32)
        var s3 : u32 = ((input[12] as u32) << 24) | ((input[13] as u32) << 16) |
                        ((input[14] as u32) << 8) | (input[15] as u32)

        // AddRoundKey (round 0)
        s0 ^= ctx.rk[0]; s1 ^= ctx.rk[1]; s2 ^= ctx.rk[2]; s3 ^= ctx.rk[3]

        var r : u32 = 1
        while(r < ctx.nr) {
            // SubBytes + ShiftRows + MixColumns + AddRoundKey
            var t0 : u32
            var t1 : u32
            var t2 : u32
            var t3 : u32

            t0 = (AES_SBOX[(s0 >> 24) & 0xFF] as u32) ^
                 ((AES_SBOX[(s1 >> 16) & 0xFF] as u32) << 8) ^
                 ((AES_SBOX[(s2 >> 8) & 0xFF] as u32) << 16) ^
                 (AES_SBOX[s3 & 0xFF] as u32) << 24
            t1 = (AES_SBOX[(s1 >> 24) & 0xFF] as u32) ^
                 ((AES_SBOX[(s2 >> 16) & 0xFF] as u32) << 8) ^
                 ((AES_SBOX[(s3 >> 8) & 0xFF] as u32) << 16) ^
                 (AES_SBOX[s0 & 0xFF] as u32) << 24
            t2 = (AES_SBOX[(s2 >> 24) & 0xFF] as u32) ^
                 ((AES_SBOX[(s3 >> 16) & 0xFF] as u32) << 8) ^
                 ((AES_SBOX[(s0 >> 8) & 0xFF] as u32) << 16) ^
                 (AES_SBOX[s1 & 0xFF] as u32) << 24
            t3 = (AES_SBOX[(s3 >> 24) & 0xFF] as u32) ^
                 ((AES_SBOX[(s0 >> 16) & 0xFF] as u32) << 8) ^
                 ((AES_SBOX[(s1 >> 8) & 0xFF] as u32) << 16) ^
                 (AES_SBOX[s2 & 0xFF] as u32) << 24

            // MixColumns using byte extraction
            var b0_b0 = (t0 >> 24) & 0xFF; var b0_b1 = (t0 >> 16) & 0xFF
            var b0_b2 = (t0 >> 8) & 0xFF; var b0_b3 = t0 & 0xFF
            var b1_b0 = (t1 >> 24) & 0xFF; var b1_b1 = (t1 >> 16) & 0xFF
            var b1_b2 = (t1 >> 8) & 0xFF; var b1_b3 = t1 & 0xFF
            var b2_b0 = (t2 >> 24) & 0xFF; var b2_b1 = (t2 >> 16) & 0xFF
            var b2_b2 = (t2 >> 8) & 0xFF; var b2_b3 = t2 & 0xFF
            var b3_b0 = (t3 >> 24) & 0xFF; var b3_b1 = (t3 >> 16) & 0xFF
            var b3_b2 = (t3 >> 8) & 0xFF; var b3_b3 = t3 & 0xFF

            s0 = (mul2(b0_b0) ^ mul3(b0_b1) ^ b0_b2 ^ b0_b3) << 24 |
                 (b0_b0 ^ mul2(b0_b1) ^ mul3(b0_b2) ^ b0_b3) << 16 |
                 (b0_b0 ^ b0_b1 ^ mul2(b0_b2) ^ mul3(b0_b3)) << 8 |
                 (mul3(b0_b0) ^ b0_b1 ^ b0_b2 ^ mul2(b0_b3))

            s1 = (mul2(b1_b0) ^ mul3(b1_b1) ^ b1_b2 ^ b1_b3) << 24 |
                 (b1_b0 ^ mul2(b1_b1) ^ mul3(b1_b2) ^ b1_b3) << 16 |
                 (b1_b0 ^ b1_b1 ^ mul2(b1_b2) ^ mul3(b1_b3)) << 8 |
                 (mul3(b1_b0) ^ b1_b1 ^ b1_b2 ^ mul2(b1_b3))

            s2 = (mul2(b2_b0) ^ mul3(b2_b1) ^ b2_b2 ^ b2_b3) << 24 |
                 (b2_b0 ^ mul2(b2_b1) ^ mul3(b2_b2) ^ b2_b3) << 16 |
                 (b2_b0 ^ b2_b1 ^ mul2(b2_b2) ^ mul3(b2_b3)) << 8 |
                 (mul3(b2_b0) ^ b2_b1 ^ b2_b2 ^ mul2(b2_b3))

            s3 = (mul2(b3_b0) ^ mul3(b3_b1) ^ b3_b2 ^ b3_b3) << 24 |
                 (b3_b0 ^ mul2(b3_b1) ^ mul3(b3_b2) ^ b3_b3) << 16 |
                 (b3_b0 ^ b3_b1 ^ mul2(b3_b2) ^ mul3(b3_b3)) << 8 |
                 (mul3(b3_b0) ^ b3_b1 ^ b3_b2 ^ mul2(b3_b3))

            // AddRoundKey
            s0 ^= ctx.rk[r * 4]
            s1 ^= ctx.rk[r * 4 + 1]
            s2 ^= ctx.rk[r * 4 + 2]
            s3 ^= ctx.rk[r * 4 + 3]

            r += 1
        }

        // Final round (no MixColumns)
        var t0_f = (AES_SBOX[(s0 >> 24) & 0xFF] as u32) ^
                   ((AES_SBOX[(s1 >> 16) & 0xFF] as u32) << 8) ^
                   ((AES_SBOX[(s2 >> 8) & 0xFF] as u32) << 16) ^
                   (AES_SBOX[s3 & 0xFF] as u32) << 24
        var t1_f = (AES_SBOX[(s1 >> 24) & 0xFF] as u32) ^
                   ((AES_SBOX[(s2 >> 16) & 0xFF] as u32) << 8) ^
                   ((AES_SBOX[(s3 >> 8) & 0xFF] as u32) << 16) ^
                   (AES_SBOX[s0 & 0xFF] as u32) << 24
        var t2_f = (AES_SBOX[(s2 >> 24) & 0xFF] as u32) ^
                   ((AES_SBOX[(s3 >> 16) & 0xFF] as u32) << 8) ^
                   ((AES_SBOX[(s0 >> 8) & 0xFF] as u32) << 16) ^
                   (AES_SBOX[s1 & 0xFF] as u32) << 24
        var t3_f = (AES_SBOX[(s3 >> 24) & 0xFF] as u32) ^
                   ((AES_SBOX[(s0 >> 16) & 0xFF] as u32) << 8) ^
                   ((AES_SBOX[(s1 >> 8) & 0xFF] as u32) << 16) ^
                   (AES_SBOX[s2 & 0xFF] as u32) << 24

        s0 = t0_f ^ ctx.rk[ctx.nr * 4]
        s1 = t1_f ^ ctx.rk[ctx.nr * 4 + 1]
        s2 = t2_f ^ ctx.rk[ctx.nr * 4 + 2]
        s3 = t3_f ^ ctx.rk[ctx.nr * 4 + 3]

        // Write output
        output[0] = ((s0 >> 24) & 0xFF) as u8
        output[1] = ((s0 >> 16) & 0xFF) as u8
        output[2] = ((s0 >> 8) & 0xFF) as u8
        output[3] = (s0 & 0xFF) as u8
        output[4] = ((s1 >> 24) & 0xFF) as u8
        output[5] = ((s1 >> 16) & 0xFF) as u8
        output[6] = ((s1 >> 8) & 0xFF) as u8
        output[7] = (s1 & 0xFF) as u8
        output[8] = ((s2 >> 24) & 0xFF) as u8
        output[9] = ((s2 >> 16) & 0xFF) as u8
        output[10] = ((s2 >> 8) & 0xFF) as u8
        output[11] = (s2 & 0xFF) as u8
        output[12] = ((s3 >> 24) & 0xFF) as u8
        output[13] = ((s3 >> 16) & 0xFF) as u8
        output[14] = ((s3 >> 8) & 0xFF) as u8
        output[15] = (s3 & 0xFF) as u8
    }

    // ─── AES ECB Decrypt (single block) ─────────────────────────────────────

    func aes_internal_decrypt(ctx : *mut AESContext, input : *u8, output : *mut u8) {
        var s0 : u32 = ((input[0] as u32) << 24) | ((input[1] as u32) << 16) |
                        ((input[2] as u32) << 8) | (input[3] as u32)
        var s1 : u32 = ((input[4] as u32) << 24) | ((input[5] as u32) << 16) |
                        ((input[6] as u32) << 8) | (input[7] as u32)
        var s2 : u32 = ((input[8] as u32) << 24) | ((input[9] as u32) << 16) |
                        ((input[10] as u32) << 8) | (input[11] as u32)
        var s3 : u32 = ((input[12] as u32) << 24) | ((input[13] as u32) << 16) |
                        ((input[14] as u32) << 8) | (input[15] as u32)

        // AddRoundKey (last round key for decryption)
        s0 ^= ctx.rk[ctx.nr * 4]
        s1 ^= ctx.rk[ctx.nr * 4 + 1]
        s2 ^= ctx.rk[ctx.nr * 4 + 2]
        s3 ^= ctx.rk[ctx.nr * 4 + 3]

        var r : u32 = ctx.nr - 1
        while(r > 0) {
            // InvShiftRows + InvSubBytes
            var t0 : u32
            var t1 : u32
            var t2 : u32
            var t3 : u32

            t0 = (AES_RSBOX[(s0 >> 24) & 0xFF] as u32) ^
                 ((AES_RSBOX[(s3 >> 16) & 0xFF] as u32) << 8) ^
                 ((AES_RSBOX[(s2 >> 8) & 0xFF] as u32) << 16) ^
                 (AES_RSBOX[s1 & 0xFF] as u32) << 24
            t1 = (AES_RSBOX[(s1 >> 24) & 0xFF] as u32) ^
                 ((AES_RSBOX[(s0 >> 16) & 0xFF] as u32) << 8) ^
                 ((AES_RSBOX[(s3 >> 8) & 0xFF] as u32) << 16) ^
                 (AES_RSBOX[s2 & 0xFF] as u32) << 24
            t2 = (AES_RSBOX[(s2 >> 24) & 0xFF] as u32) ^
                 ((AES_RSBOX[(s1 >> 16) & 0xFF] as u32) << 8) ^
                 ((AES_RSBOX[(s0 >> 8) & 0xFF] as u32) << 16) ^
                 (AES_RSBOX[s3 & 0xFF] as u32) << 24
            t3 = (AES_RSBOX[(s3 >> 24) & 0xFF] as u32) ^
                 ((AES_RSBOX[(s2 >> 16) & 0xFF] as u32) << 8) ^
                 ((AES_RSBOX[(s1 >> 8) & 0xFF] as u32) << 16) ^
                 (AES_RSBOX[s0 & 0xFF] as u32) << 24

            // InvMixColumns
            var b0 = t0; var b1 = t1; var b2 = t2; var b3 = t3

            var b0_b0 = (b0 >> 24) & 0xFF; var b0_b1 = (b0 >> 16) & 0xFF
            var b0_b2 = (b0 >> 8) & 0xFF; var b0_b3 = b0 & 0xFF
            var b1_b0 = (b1 >> 24) & 0xFF; var b1_b1 = (b1 >> 16) & 0xFF
            var b1_b2 = (b1 >> 8) & 0xFF; var b1_b3 = b1 & 0xFF
            var b2_b0 = (b2 >> 24) & 0xFF; var b2_b1 = (b2 >> 16) & 0xFF
            var b2_b2 = (b2 >> 8) & 0xFF; var b2_b3 = b2 & 0xFF
            var b3_b0 = (b3 >> 24) & 0xFF; var b3_b1 = (b3 >> 16) & 0xFF
            var b3_b2 = (b3 >> 8) & 0xFF; var b3_b3 = b3 & 0xFF

            s0 = (mul14(b0_b0) ^ mul11(b0_b1) ^ mul13(b0_b2) ^ mul9(b0_b3)) << 24 |
                 (mul9(b0_b0) ^ mul14(b0_b1) ^ mul11(b0_b2) ^ mul13(b0_b3)) << 16 |
                 (mul13(b0_b0) ^ mul9(b0_b1) ^ mul14(b0_b2) ^ mul11(b0_b3)) << 8 |
                 (mul11(b0_b0) ^ mul13(b0_b1) ^ mul9(b0_b2) ^ mul14(b0_b3))

            s1 = (mul14(b1_b0) ^ mul11(b1_b1) ^ mul13(b1_b2) ^ mul9(b1_b3)) << 24 |
                 (mul9(b1_b0) ^ mul14(b1_b1) ^ mul11(b1_b2) ^ mul13(b1_b3)) << 16 |
                 (mul13(b1_b0) ^ mul9(b1_b1) ^ mul14(b1_b2) ^ mul11(b1_b3)) << 8 |
                 (mul11(b1_b0) ^ mul13(b1_b1) ^ mul9(b1_b2) ^ mul14(b1_b3))

            s2 = (mul14(b2_b0) ^ mul11(b2_b1) ^ mul13(b2_b2) ^ mul9(b2_b3)) << 24 |
                 (mul9(b2_b0) ^ mul14(b2_b1) ^ mul11(b2_b2) ^ mul13(b2_b3)) << 16 |
                 (mul13(b2_b0) ^ mul9(b2_b1) ^ mul14(b2_b2) ^ mul11(b2_b3)) << 8 |
                 (mul11(b2_b0) ^ mul13(b2_b1) ^ mul9(b2_b2) ^ mul14(b2_b3))

            s3 = (mul14(b3_b0) ^ mul11(b3_b1) ^ mul13(b3_b2) ^ mul9(b3_b3)) << 24 |
                 (mul9(b3_b0) ^ mul14(b3_b1) ^ mul11(b3_b2) ^ mul13(b3_b3)) << 16 |
                 (mul13(b3_b0) ^ mul9(b3_b1) ^ mul14(b3_b2) ^ mul11(b3_b3)) << 8 |
                 (mul11(b3_b0) ^ mul13(b3_b1) ^ mul9(b3_b2) ^ mul14(b3_b3))

            // AddRoundKey
            s0 ^= ctx.rk[r * 4]
            s1 ^= ctx.rk[r * 4 + 1]
            s2 ^= ctx.rk[r * 4 + 2]
            s3 ^= ctx.rk[r * 4 + 3]

            if(r == 0) { break }
            r -= 1
        }

        // Final AddRoundKey (round 0)
        var t0_f = (AES_RSBOX[(s0 >> 24) & 0xFF] as u32) ^
                   ((AES_RSBOX[(s3 >> 16) & 0xFF] as u32) << 8) ^
                   ((AES_RSBOX[(s2 >> 8) & 0xFF] as u32) << 16) ^
                   (AES_RSBOX[s1 & 0xFF] as u32) << 24
        var t1_f = (AES_RSBOX[(s1 >> 24) & 0xFF] as u32) ^
                   ((AES_RSBOX[(s0 >> 16) & 0xFF] as u32) << 8) ^
                   ((AES_RSBOX[(s3 >> 8) & 0xFF] as u32) << 16) ^
                   (AES_RSBOX[s2 & 0xFF] as u32) << 24
        var t2_f = (AES_RSBOX[(s2 >> 24) & 0xFF] as u32) ^
                   ((AES_RSBOX[(s1 >> 16) & 0xFF] as u32) << 8) ^
                   ((AES_RSBOX[(s0 >> 8) & 0xFF] as u32) << 16) ^
                   (AES_RSBOX[s3 & 0xFF] as u32) << 24
        var t3_f = (AES_RSBOX[(s3 >> 24) & 0xFF] as u32) ^
                   ((AES_RSBOX[(s2 >> 16) & 0xFF] as u32) << 8) ^
                   ((AES_RSBOX[(s1 >> 8) & 0xFF] as u32) << 16) ^
                   (AES_RSBOX[s0 & 0xFF] as u32) << 24

        s0 = t0_f ^ ctx.rk[0]
        s1 = t1_f ^ ctx.rk[1]
        s2 = t2_f ^ ctx.rk[2]
        s3 = t3_f ^ ctx.rk[3]

        output[0] = ((s0 >> 24) & 0xFF) as u8
        output[1] = ((s0 >> 16) & 0xFF) as u8
        output[2] = ((s0 >> 8) & 0xFF) as u8
        output[3] = (s0 & 0xFF) as u8
        output[4] = ((s1 >> 24) & 0xFF) as u8
        output[5] = ((s1 >> 16) & 0xFF) as u8
        output[6] = ((s1 >> 8) & 0xFF) as u8
        output[7] = (s1 & 0xFF) as u8
        output[8] = ((s2 >> 24) & 0xFF) as u8
        output[9] = ((s2 >> 16) & 0xFF) as u8
        output[10] = ((s2 >> 8) & 0xFF) as u8
        output[11] = (s2 & 0xFF) as u8
        output[12] = ((s3 >> 24) & 0xFF) as u8
        output[13] = ((s3 >> 16) & 0xFF) as u8
        output[14] = ((s3 >> 8) & 0xFF) as u8
        output[15] = (s3 & 0xFF) as u8
    }

    // ─── Galois Field (2^8) Multiplication helpers ────────────────────────

    // Multiply by 2 in GF(2^8)
    func mul2(x : u32) : u32 {
        var r = x << 1
        if(x & 0x80) { r ^= 0x1B }
        return r & 0xFF
    }

    func mul3(x : u32) : u32 { return mul2(x) ^ x }

    func mul9(x : u32) : u32 { return mul2(mul2(mul2(x))) ^ x }

    func mul11(x : u32) : u32 { return mul2(mul2(mul2(x))) ^ mul2(x) ^ x }

    func mul13(x : u32) : u32 { return mul2(mul2(mul2(x))) ^ mul2(mul2(x)) ^ x }

    func mul14(x : u32) : u32 { return mul2(mul2(mul2(x))) ^ mul2(mul2(x)) ^ mul2(x) }

    // ============================================================================
    // Public AES API
    // ============================================================================

    // Set encryption key (16 or 32 bytes)
    public func aes_setkey_enc(ctx : *mut AESContext, key : *u8, key_len : size_t) : int {
        return aes_key_expansion(ctx, key, key_len)
    }

    // Set decryption key (16 or 32 bytes) — for AES, decryption key is derived from encryption key
    public func aes_setkey_dec(ctx : *mut AESContext, key : *u8, key_len : size_t) : int {
        return aes_key_expansion(ctx, key, key_len)
    }

    // AES-ECB encrypt/decrypt (single 16-byte block)
    public func aes_crypt_ecb(ctx : *mut AESContext, mode : u32, input : *u8, output : *mut u8) : int {
        if(mode == 0) { // Encrypt
            aes_internal_encrypt(ctx, input, output)
        } else { // Decrypt
            aes_internal_decrypt(ctx, input, output)
        }
        return 0
    }

    public comptime const AES_ENCRYPT : u32 = 0
    public comptime const AES_DECRYPT : u32 = 1

    // AES-CBC encrypt/decrypt
    public func aes_crypt_cbc(ctx : *mut AESContext, mode : u32, length : size_t,
                               iv : *mut u8, input : *u8, output : *mut u8) : int {
        if(length % 16 != 0) { return AES_ERR_INVALID_INPUT_LENGTH }

        if(mode == AES_ENCRYPT) {
            var i : size_t = 0
            while(i < length) {
                // XOR block with IV
                var j : size_t = 0
                while(j < 16) {
                    output[i + j] = input[i + j] ^ iv[j]
                    j += 1
                }
                // Encrypt block
                aes_internal_encrypt(ctx, &raw output[i], &raw mut output[i])
                // Update IV to ciphertext
                j = 0
                while(j < 16) {
                    iv[j] = output[i + j]
                    j += 1
                }
                i += 16
            }
        } else {
            // Decrypt
            var next_iv : [16]u8
            var i : size_t = 0
            while(i < length) {
                // Save input to next_iv for next iteration
                var j : size_t = 0
                while(j < 16) {
                    next_iv[j] = input[i + j]
                    j += 1
                }
                // Decrypt block
                aes_internal_decrypt(ctx, &raw input[i], &raw mut output[i])
                // XOR with IV
                j = 0
                while(j < 16) {
                    output[i + j] = output[i + j] ^ iv[j]
                    j += 1
                }
                // Update IV
                j = 0
                while(j < 16) {
                    iv[j] = next_iv[j]
                    j += 1
                }
                i += 16
            }
        }

        return 0
    }

} // namespace tls
