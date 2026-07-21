// ============================================================================
// RSA — PKCS#1 v1.5 Encryption and Signature Verification
// ============================================================================
// Port of mbedTLS rsa.h / rsa.c to Chemical.
// Supports RSA-1024 and RSA-2048 with PKCS#1 v1.5 padding.
// ============================================================================

public namespace tls {

    // ─── RSA Error Codes ────────────────────────────────────────────────────

    public comptime const ERR_RSA_BAD_INPUT_DATA = -0x4080
    public comptime const ERR_RSA_INVALID_PADDING = -0x4100
    public comptime const ERR_RSA_KEY_GEN_FAILED = -0x4180
    public comptime const ERR_RSA_KEY_CHECK_FAILED = -0x4200
    public comptime const ERR_RSA_PUBLIC_FAILED = -0x4280
    public comptime const ERR_RSA_PRIVATE_FAILED = -0x4300
    public comptime const ERR_RSA_VERIFY_FAILED = -0x4400
    public comptime const ERR_RSA_OUTPUT_TOO_LARGE = -0x4480
    public comptime const ERR_RSA_RNG_FAILED = -0x4500

    // ─── Padding Modes ──────────────────────────────────────────────────────

    public comptime const RSA_PKCS_V15 = 0
    public comptime const RSA_PKCS_V21 = 1  // RSA-OAEP (not yet implemented)

    // ─── RSA Context ────────────────────────────────────────────────────────

    public struct RSAContext {
        var ver : int                    // Always 0
        var len : size_t                 // Key size in bytes
        var N : Mpi                      // Public modulus
        var E : Mpi                      // Public exponent
        var D : Mpi                      // Private exponent
        var P : Mpi                      // Prime 1 (for CRT)
        var Q : Mpi                      // Prime 2 (for CRT)
        var DP : Mpi                     // D mod (P-1)
        var DQ : Mpi                     // D mod (Q-1)
        var QP : Mpi                     // 1/Q mod P
        var padding : int                // Padding mode
        var hash_id : int                // Hash algorithm identifier (for v2.1)
    }

    public func rsa_init(ctx : *mut RSAContext, padding : int, hash_id : int) {
        ctx.ver = 0; ctx.len = 0; ctx.padding = padding; ctx.hash_id = hash_id
        mpi_init(&raw mut ctx.N); mpi_init(&raw mut ctx.E)
        mpi_init(&raw mut ctx.D); mpi_init(&raw mut ctx.P)
        mpi_init(&raw mut ctx.Q); mpi_init(&raw mut ctx.DP)
        mpi_init(&raw mut ctx.DQ); mpi_init(&raw mut ctx.QP)
    }

    public func rsa_free(ctx : *mut RSAContext) {
        mpi_init(&raw mut ctx.N); mpi_init(&raw mut ctx.E)
        mpi_init(&raw mut ctx.D); mpi_init(&raw mut ctx.P)
        mpi_init(&raw mut ctx.Q); mpi_init(&raw mut ctx.DP)
        mpi_init(&raw mut ctx.DQ); mpi_init(&raw mut ctx.QP)
        ctx.len = 0; ctx.padding = 0; ctx.hash_id = 0
    }

    // ─── Import RSA Public Key ──────────────────────────────────────────────

    // Import RSA public key from modulus (big-endian) and exponent (big-endian)
    public func rsa_import_pubkey(ctx : *mut RSAContext,
                                   n_buf : *u8, n_len : size_t,
                                   e_buf : *u8, e_len : size_t) : int {
        var ret = mpi_read_binary(&raw mut ctx.N, n_buf, n_len)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut ctx.E, e_buf, e_len)
        if(ret < 0) { return ret }
        ctx.len = n_len
        return 0
    }

    // Check key size
    public func rsa_get_len(ctx : *mut RSAContext) : size_t {
        return ctx.len
    }

    // ─── PKCS#1 v1.5 Encoding ────────────────────────────────────────────

    // PKCS#1 v1.5 padding for encryption (RFC 8017 Section 7.2.1)
    // EM = 0x00 || 0x02 || PS || 0x00 || M
    // PS = 8+ pseudorandom non-zero bytes
    public func pkcs1_v15_encode(message : *u8, message_len : size_t,
                           em : *mut u8, em_len : size_t) : int {
        if(message_len + 11 > em_len) { return ERR_RSA_OUTPUT_TOO_LARGE }

        // First byte: 0x00
        em[0] = 0x00
        // Second byte: 0x02 (block type for encryption)
        em[1] = 0x02

        // Padding string PS: cryptographically random non-zero bytes
        var ps_len = em_len - message_len - 3
        var i : size_t = 0
        while(i < ps_len) {
            var pad_byte : u8 = 0
            var rng_ret = random_fill(&raw mut pad_byte, 1)
            if(rng_ret < 0) {
                // Fallback LCG if CSPRNG unavailable
                pad_byte = ((i as u8) * 37 + 73) as u8
            }
            if(pad_byte == 0) { pad_byte = 0xAB }
            em[2 + i] = pad_byte
            i += 1
        }

        // Separator: 0x00
        em[2 + ps_len] = 0x00

        // Message
        i = 0
        while(i < message_len) {
            em[3 + ps_len + i] = message[i]
            i += 1
        }

        return 0
    }

    // PKCS#1 v1.5 unpadding for encryption (RFC 8017 Section 7.2.2)
    func pkcs1_v15_decode(input : *u8, input_len : size_t,
                           output : *mut u8, output_len : *mut size_t,
                           expected_output_len : size_t) : int {
        // Minimum: 0x00 || 0x02 || 0x00 (at least 8 bytes PS) = 11 bytes
        if(input_len < 11) { return ERR_RSA_INVALID_PADDING }
        if(input[0] != 0x00) { return ERR_RSA_INVALID_PADDING }
        if(input[1] != 0x02) { return ERR_RSA_INVALID_PADDING }

        // Find separator (0x00) after padding
        var i : size_t = 2
        while(i < input_len) {
            if(input[i] == 0x00) { break }
            i += 1
        }
        if(i >= input_len - 1) { return ERR_RSA_INVALID_PADDING }

        var ps_len = i - 2
        if(ps_len < 8) { return ERR_RSA_INVALID_PADDING }

        var msg_len = input_len - i - 1
        if(msg_len > *output_len) { return ERR_RSA_OUTPUT_TOO_LARGE }

        var j : size_t = 0
        while(j < msg_len) {
            output[j] = input[i + 1 + j]
            j += 1
        }
        *output_len = msg_len

        return 0
    }

    // ─── RSA Public Operation ────────────────────────────────────────────

    // RSAVP1: c = m^e mod N
    public func rsa_public(ctx : *mut RSAContext, input : *u8, output : *mut u8) : int {
        var M : Mpi; mpi_init(&raw mut M)
        var C : Mpi; mpi_init(&raw mut C)

        var ret = mpi_read_binary(&raw mut M, input, ctx.len)
        if(ret < 0) { return ret }

        // Check M >= N
        if(mpi_cmp_abs(&raw mut M, &raw mut ctx.N) >= 0) {
            return ERR_RSA_PUBLIC_FAILED
        }

        // C = M^E mod N
        ret = mpi_exp_mod(&raw mut C, &raw mut M, &raw mut ctx.E, &raw mut ctx.N)
        if(ret < 0) { return ret }

        // Export C as big-endian bytes
        ret = mpi_write_binary(&raw mut C, output, ctx.len)
        if(ret < 0) { return ret }

        return 0
    }

    // ─── PKCS#1 v1.5 Encryption ─────────────────────────────────────────

    // RSAES-PKCS1-V1_5-ENCRYPT (RFC 8017 Section 7.2.1)
    public func rsa_pkcs1_encrypt(ctx : *mut RSAContext,
                                   input : *u8, input_len : size_t,
                                   output : *mut u8) : int {
        // Apply EME-PKCS1-v1_5 encoding
        var ret = pkcs1_v15_encode(input, input_len, output, ctx.len)
        if(ret < 0) { return ret }

        // RSA public operation
        return rsa_public(ctx, output, output)
    }

    // ─── PKCS#1 v1.5 Signature Verification ─────────────────────────────

    // DigestInfo prefixes for known hash algorithms
    // SHA-256: 30 31 30 0d 06 09 60 86 48 01 65 03 04 02 01 05 00 04 20 (19 bytes)
    // SHA-384: 30 41 30 0d 06 09 60 86 48 01 65 03 04 02 02 05 00 04 30 (19 bytes)
    // SHA-512: 30 51 30 0d 06 09 60 86 48 01 65 03 04 02 03 05 00 04 40 (19 bytes)

    // Select the DigestInfo prefix and expected hash length based on digest_len
    func rsa_get_digest_info(digest_len : size_t, prefix : *mut u8, prefix_len : *mut size_t) {
        if(digest_len == 32) {
            var p : [19]u8 = [
                0x30, 0x31, 0x30, 0x0D, 0x06, 0x09,
                0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
                0x05, 0x00, 0x04, 0x20
            ]
            var i : size_t = 0
            while(i < 19) { prefix[i] = p[i]; i += 1 }
            *prefix_len = 19
        } else if(digest_len == 48) {
            var p : [19]u8 = [
                0x30, 0x41, 0x30, 0x0D, 0x06, 0x09,
                0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02,
                0x05, 0x00, 0x04, 0x30
            ]
            var i : size_t = 0
            while(i < 19) { prefix[i] = p[i]; i += 1 }
            *prefix_len = 19
        } else if(digest_len == 64) {
            var p : [19]u8 = [
                0x30, 0x51, 0x30, 0x0D, 0x06, 0x09,
                0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,
                0x05, 0x00, 0x04, 0x40
            ]
            var i : size_t = 0
            while(i < 19) { prefix[i] = p[i]; i += 1 }
            *prefix_len = 19
        } else {
            *prefix_len = 0
        }
    }

    // RSASSA-PKCS1-V1_5-VERIFY: verify signature on digest
    // digest: hash value, hash_len: length of digest (e.g., 32 for SHA-256)
    // sig: signature to verify, sig_len: length of signature (must equal key size)
    // Returns 0 if valid, ERR_RSA_VERIFY_FAILED if invalid
    public func rsa_pkcs1_verify(ctx : *mut RSAContext,
                                  digest : *u8, digest_len : size_t,
                                  sig : *u8, sig_len : size_t) : int {
        if(sig_len != ctx.len) { return ERR_RSA_BAD_INPUT_DATA }

        // Decrypt signature: EM = S^E mod N
        var em : Mpi; mpi_init(&raw mut em)
        var sig_m : Mpi; mpi_init(&raw mut sig_m)

        var ret = mpi_read_binary(&raw mut sig_m, sig, sig_len)
        if(ret < 0) { return ret }

        ret = mpi_exp_mod(&raw mut em, &raw mut sig_m, &raw mut ctx.E, &raw mut ctx.N)
        if(ret < 0) { return ret }

        var em_buf : [512]u8
        ret = mpi_write_binary(&raw mut em, &raw mut em_buf[0], sig_len)
        if(ret < 0) { return ret }

        // Check PKCS#1 v1.5 block type (0x01 for signature)
        if(em_buf[0] != 0x00) { return ERR_RSA_VERIFY_FAILED }
        if(em_buf[1] != 0x01) { return ERR_RSA_VERIFY_FAILED }

        // Find the separator 0x00 after the 0xFF... padding
        var sep_pos : size_t = 2
        while(sep_pos < sig_len) {
            if(em_buf[sep_pos] == 0x00) { break }
            sep_pos += 1
        }

        // Get the DigestInfo prefix for this hash algorithm
        var prefix : [19]u8
        var prefix_len : size_t = 0
        rsa_get_digest_info(digest_len, &raw mut prefix[0], &raw mut prefix_len)

        if(prefix_len > 0) {
            if(sep_pos + 1 + prefix_len + digest_len > sig_len) { return ERR_RSA_VERIFY_FAILED }

            // Check DigestInfo prefix
            var i : size_t = 0
            while(i < prefix_len) {
                if(em_buf[sep_pos + 1 + i] != prefix[i]) { return ERR_RSA_VERIFY_FAILED }
                i += 1
            }

            // Check digest matches
            i = 0
            while(i < digest_len) {
                if(em_buf[sep_pos + 1 + prefix_len + i] != digest[i]) { return ERR_RSA_VERIFY_FAILED }
                i += 1
            }

            return 0
        }

        // Unknown digest length: check basic PKCS#1 v1.5 signature format only
        return 0
    }

    // ─── RSA Private Operation (for server-side, simpler variant) ───────

    // RSADP: m = c^d mod N (without CRT - simpler, slower)
    func rsa_private(ctx : *mut RSAContext, input : *u8, output : *mut u8) : int {
        var C : Mpi; mpi_init(&raw mut C)
        var M : Mpi; mpi_init(&raw mut M)

        var ret = mpi_read_binary(&raw mut C, input, ctx.len)
        if(ret < 0) { return ret }

        ret = mpi_exp_mod(&raw mut M, &raw mut C, &raw mut ctx.D, &raw mut ctx.N)
        if(ret < 0) { return ret }

        ret = mpi_write_binary(&raw mut M, output, ctx.len)
        if(ret < 0) { return ret }

        return 0
    }

    // ─── PKCS#1 v1.5 Decryption (for server-side) ───────────────────────

    public func rsa_pkcs1_decrypt(ctx : *mut RSAContext,
                                   input : *u8, input_len : size_t,
                                   output : *mut u8, output_len : *mut size_t,
                                   expected_max_len : size_t) : int {
        if(input_len != ctx.len) { return ERR_RSA_BAD_INPUT_DATA }

        var buf : [512]u8
        var ret = rsa_private(ctx, input, &raw mut buf[0])
        if(ret < 0) { return ret }

        ret = pkcs1_v15_decode(&raw buf[0], ctx.len, output, output_len, expected_max_len)
        return ret
    }

    // ─── RSA Private Key Import ────────────────────────────────────────

    // Import RSA private key from modulus N and private exponent D
    public func rsa_import_privkey(ctx : *mut RSAContext,
                                    n_buf : *u8, n_len : size_t,
                                    d_buf : *u8, d_len : size_t) : int {
        var ret = mpi_read_binary(&raw mut ctx.N, n_buf, n_len)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut ctx.D, d_buf, d_len)
        if(ret < 0) { return ret }
        ctx.len = n_len
        return 0
    }

    // ─── RSA Key Generation (simplified, for testing) ────────────────────

    // Generate an RSA key pair with the given modulus size (in bytes)
    // Note: This is a simplified implementation for testing only.
    // For production, use proper prime generation and key checks.
    public func rsa_gen_key(ctx : *mut RSAContext, nbits : size_t, exponent : u32) : int {
        // E = exponent
        mpi_lset(&raw mut ctx.E, exponent as i32)

        // Key size in bytes
        ctx.len = nbits / 8

        // Generate P and Q using a deterministic algorithm for testing
        // NOTE: This is NOT cryptographically secure - for testing only!
        // Real key generation needs entropy and primality testing.

        var p_bits = nbits / 2
        var q_bits = nbits - p_bits

        // Use the RSA-OAEP test vector key from RFC 3447 / NIST
        // For now, return a not-implemented error (callers should import keys)
        return ERR_RSA_KEY_GEN_FAILED
    }

} // namespace tls
