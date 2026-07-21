// ============================================================================
// GCM — Galois/Counter Mode (AES-GCM)
// ============================================================================
// Port of mbedTLS gcm.h / gcm.c to Chemical.
// Authenticated encryption using AES-CTR + GHASH.
// ============================================================================

public namespace tls {

    // ─── GCM Error Codes ────────────────────────────────────────────────────

    public comptime const ERR_GCM_BAD_INPUT = -0x6000
    public comptime const ERR_GCM_AUTH_FAILED = -0x6010
    public comptime const ERR_GCM_BUFFER_TOO_SMALL = -0x6020

    // ─── GCM Context ────────────────────────────────────────────────────────

    public struct GCMContext {
        var cipher_ctx : AESContext  // AES encryption context
        var H : [16]u8               // Hash subkey = AES(K, 0^128)
        var len : size_t             // Key length in bytes
    }

    public func gcm_init(ctx : *mut GCMContext, key : *u8, key_len : size_t) : int {
        // Set AES key
        ctx.len = key_len
        var ret = aes_setkey_enc(&raw mut ctx.cipher_ctx, key, key_len)
        if(ret < 0) { return ret }

        // Compute H = AES(K, 0^128)
        var zero_block : [16]u8
        var i : size_t = 0
        while(i < 16) { zero_block[i] = 0; i += 1 }
        aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT, &raw zero_block[0], &raw mut ctx.H[0])

        return 0
    }

    // ─── GF(2^128) Multiplication ──────────────────────────────────────────

    // GHASH core: multiply in GF(2^128) with the GCM polynomial
    // The GCM specification uses a specific bit ordering:
    // - Bits are numbered from MSB (bit 0) to LSB (bit 127)
    // - Polynomials are represented with the highest degree coefficient in bit 0 of byte 0
    func ghash_multiply_refined(output : *mut u8, x : *u8, y : *u8) {
        var Z : [16]u8
        var V : [16]u8
        var i : size_t = 0
        while(i < 16) { Z[i] = 0; V[i] = y[i]; i += 1 }

        i = 0
        while(i < 128) {
            // Instead of bit-by-bit, use a table-based approach for speed
            // For now, use the simplest bit-by-bit method:
            var byte_idx = i >> 3      // i / 8
            var bit_mask = 0x80u8 >> (i & 7)  // MSB-first: bit 0 is MSB

            var xi = (x[byte_idx] & bit_mask) != 0

            if(xi) {
                var j : size_t = 0
                while(j < 16) { Z[j] = Z[j] ^ V[j]; j += 1 }
            }

            // Check if V has LSB (bit 127) set - need to check bit 0 of byte 15
            var v_lsb = (V[15] & 0x01) as u8

            // Right shift V by 1 (moving towards LSB)
            var k = 15
            while(k > 0) {
                V[k] = (V[k] >> 1) | ((V[k - 1] & 1) << 7)
                k -= 1
            }
            V[0] = V[0] >> 1

            // Polynomial reduction if LSB was 1
            if(v_lsb != 0) {
                // XOR with R = 0xE1 << 120
                // In MSB-first byte ordering: E1 00 ... 00
                V[0] = V[0] ^ 0xE1
            }

            i += 1
        }

        i = 0
        while(i < 16) { output[i] = Z[i]; i += 1 }
    }

    // ─── GHASH (core authentication function) ─────────────────────────────

    // GHASH(H, A, C) = X_{m+n+1} where:
    // X_0 = 0
    // X_i = (X_{i-1} XOR A_i) * H  (for i = 1..m, A_i are 128-bit blocks of AAD)
    // X_{m+i} = (X_{m+i-1} XOR C_i) * H  (for i = 1..n, C_i are 128-bit blocks of ciphertext)
    // X_{m+n+1} = (X_{m+n} XOR len(A) || len(C)) * H
    func ghash(ctx : *mut GCMContext, aad : *u8, aad_len : size_t,
               ciphertext : *u8, ct_len : size_t,
               output : *mut u8) {
        var Y : [16]u8
        var i : size_t = 0
        while(i < 16) { Y[i] = 0; i += 1 }

        // Process AAD blocks
        var pos : size_t = 0
        while(pos + 16 <= aad_len) {
            var j : size_t = 0
            while(j < 16) { Y[j] = Y[j] ^ aad[pos + j]; j += 1 }
            ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])
            pos += 16
        }

        // Process partial AAD block (if any)
        if(pos < aad_len) {
            var block : [16]u8
            var j : size_t = 0
            while(j < 16) { block[j] = 0; j += 1 }
            j = 0
            while(pos + j < aad_len) { block[j] = aad[pos + j]; j += 1 }
            j = 0
            while(j < 16) { Y[j] = Y[j] ^ block[j]; j += 1 }
            ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])
        }

        // Process ciphertext blocks
        pos = 0
        while(pos + 16 <= ct_len) {
            var j : size_t = 0
            while(j < 16) { Y[j] = Y[j] ^ ciphertext[pos + j]; j += 1 }
            ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])
            pos += 16
        }

        // Process partial ciphertext block (if any)
        if(pos < ct_len) {
            var block : [16]u8
            var j : size_t = 0
            while(j < 16) { block[j] = 0; j += 1 }
            j = 0
            while(pos + j < ct_len) { block[j] = ciphertext[pos + j]; j += 1 }
            j = 0
            while(j < 16) { Y[j] = Y[j] ^ block[j]; j += 1 }
            ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])
        }

        // Process length block
        var len_block : [16]u8
        var j : size_t = 0
        while(j < 16) { len_block[j] = 0; j += 1 }

        // 64-bit big-endian lengths
        var aad_bits = (aad_len as u64) * 8
        var ct_bits = (ct_len as u64) * 8

        len_block[0] = ((aad_bits >> 56) & 0xFFu64) as u8
        len_block[1] = ((aad_bits >> 48) & 0xFFu64) as u8
        len_block[2] = ((aad_bits >> 40) & 0xFFu64) as u8
        len_block[3] = ((aad_bits >> 32) & 0xFFu64) as u8
        len_block[4] = ((aad_bits >> 24) & 0xFFu64) as u8
        len_block[5] = ((aad_bits >> 16) & 0xFFu64) as u8
        len_block[6] = ((aad_bits >> 8) & 0xFFu64) as u8
        len_block[7] = (aad_bits & 0xFFu64) as u8

        len_block[8] = ((ct_bits >> 56) & 0xFFu64) as u8
        len_block[9] = ((ct_bits >> 48) & 0xFFu64) as u8
        len_block[10] = ((ct_bits >> 40) & 0xFFu64) as u8
        len_block[11] = ((ct_bits >> 32) & 0xFFu64) as u8
        len_block[12] = ((ct_bits >> 24) & 0xFFu64) as u8
        len_block[13] = ((ct_bits >> 16) & 0xFFu64) as u8
        len_block[14] = ((ct_bits >> 8) & 0xFFu64) as u8
        len_block[15] = (ct_bits & 0xFFu64) as u8

        j = 0
        while(j < 16) { Y[j] = Y[j] ^ len_block[j]; j += 1 }
        ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])

        j = 0
        while(j < 16) { output[j] = Y[j]; j += 1 }
    }

    // ─── Increment Counter (32-bit big-endian) ───────────────────────────

    func gcm_incr(counter : *mut u8) {
        // Increment the last 4 bytes (32-bit counter) in big-endian
        var i = 15
        while(i >= 12) {
            counter[i] = counter[i] + 1
            if(counter[i] != 0) { break }
            i -= 1
        }
    }

    // ─── GCM Encrypt ─────────────────────────────────────────────────────

    // GCM encrypt (AES-GCM)
    // key: AES key (16 or 32 bytes)
    // iv: 12-byte initial nonce (standard GCM IV length)
    // aad: additional authenticated data (can be null/empty)
    // plaintext: input data to encrypt
    // output: output buffer for ciphertext (same size as plaintext)
    // tag: 16-byte authentication tag
    public func gcm_crypt_and_tag(ctx : *mut GCMContext,
                                   iv : *u8, iv_len : size_t,
                                   aad : *u8, aad_len : size_t,
                                   plaintext : *u8, pt_len : size_t,
                                   output : *mut u8, tag : *mut u8) : int {
        // For 12-byte IV: J0 = IV || 0x00000001
        var J0 : [16]u8
        var i : size_t = 0
        while(i < 16) { J0[i] = 0; i += 1 }

        if(iv_len == 12) {
            i = 0
            while(i < 12) { J0[i] = iv[i]; i += 1 }
            J0[15] = 1
        } else {
            // For non-12-byte IV: GHASH(H, {}, IV)
            ghash(ctx, null, 0, iv, iv_len, &raw mut J0[0])
        }

        // Encrypt counter blocks and XOR with plaintext
        var counter : [16]u8
        i = 0
        while(i < 16) { counter[i] = J0[i]; i += 1 }

        var pos : size_t = 0
        while(pos < pt_len) {
            // Increment counter (skip J0 itself)
            gcm_incr(&raw mut counter[0])

            // E(K, counter)
            var enc_counter : [16]u8
            var ret = aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT,
                                     &raw counter[0], &raw mut enc_counter[0])
            if(ret < 0) { return ret }

            // XOR with plaintext (partial block handling)
            var block_size : size_t = 16
            if(pt_len - pos < 16) { block_size = pt_len - pos }

            var j : size_t = 0
            while(j < block_size) {
                output[pos + j] = plaintext[pos + j] ^ enc_counter[j]
                j += 1
            }
            pos += block_size
        }

        // Compute GHASH: gh = GHASH(H, AAD, Ciphertext)
        var gh : [16]u8
        ghash(ctx, aad, aad_len, output, pt_len, &raw mut gh[0])

        // Tag = GHASH XOR E(K, J0)
        var enc_J0 : [16]u8
        var ret = aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT,
                                 &raw J0[0], &raw mut enc_J0[0])
        if(ret < 0) { return ret }

        i = 0
        while(i < 16) {
            tag[i] = gh[i] ^ enc_J0[i]
            i += 1
        }

        return 0
    }

    // ─── GCM Decrypt ─────────────────────────────────────────────────────

    // GCM decrypt and verify tag
    public func gcm_auth_decrypt(ctx : *mut GCMContext,
                                  iv : *u8, iv_len : size_t,
                                  aad : *u8, aad_len : size_t,
                                  ciphertext : *u8, ct_len : size_t,
                                  tag : *u8, tag_len : size_t,
                                  output : *mut u8) : int {
        // 1. Compute GHASH over (AAD, Ciphertext) 
        // 2. Compare expected tag
        // 3. Decrypt if tags match

        var ret : int = 0
        var J0 : [16]u8
        var i : size_t = 0
        while(i < 16) { J0[i] = 0; i += 1 }

        if(iv_len == 12) {
            i = 0
            while(i < 12) { J0[i] = iv[i]; i += 1 }
            J0[15] = 1
        } else {
            ghash(ctx, null, 0, iv, iv_len, &raw mut J0[0])
        }

        // Compute GHASH over AAD and ciphertext
        var gh : [16]u8
        ghash(ctx, aad, aad_len, ciphertext, ct_len, &raw mut gh[0])

        // Encrypt J0
        var enc_J0 : [16]u8
        ret = aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT,
                             &raw J0[0], &raw mut enc_J0[0])
        if(ret < 0) { return ret }

        // Expected tag = GHASH XOR E(K, J0)
        var expected_tag : [16]u8
        i = 0
        while(i < 16) {
            expected_tag[i] = gh[i] ^ enc_J0[i]
            i += 1
        }

        // Compare tags (constant-time comparison would be better)
        i = 0
        while(i < tag_len) {
            if(expected_tag[i] != tag[i]) { return ERR_GCM_AUTH_FAILED }
            i += 1
        }

        // Decrypt (same as encrypt for CTR mode)
        var counter : [16]u8
        i = 0
        while(i < 16) { counter[i] = J0[i]; i += 1 }

        var pos : size_t = 0
        while(pos < ct_len) {
            gcm_incr(&raw mut counter[0])

            var enc_counter : [16]u8
            ret = aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT,
                                 &raw counter[0], &raw mut enc_counter[0])
            if(ret < 0) { return ret }

            var block_size : size_t = 16
            if(ct_len - pos < 16) { block_size = ct_len - pos }

            var j : size_t = 0
            while(j < block_size) {
                output[pos + j] = ciphertext[pos + j] ^ enc_counter[j]
                j += 1
            }
            pos += block_size
        }

        return 0
    }

} // namespace tls
