// ============================================================================
// ECDSA — Elliptic Curve Digital Signature Algorithm (secp256r1 / P-256)
// ============================================================================
// Implements ECDSA signature verification for TLS 1.3 CertificateVerify
// and X.509 certificate chain verification.
// ============================================================================

public namespace tls {

    // ─── ECDSA Error Codes ───────────────────────────────────────────────────

    public comptime const ERR_ECDSA_BAD_SIGNATURE = -0x5400
    public comptime const ERR_ECDSA_VERIFY_FAILED = -0x5480

    // ─── ECDSA Context ──────────────────────────────────────────────────────

    public struct ECDSAContext {
        var pub_x : Mpi       // Public key X coordinate (affine)
        var pub_y : Mpi       // Public key Y coordinate (affine)
        var curve_id : u16    // Named curve (TLS_GROUP_SECP256R1 etc.)
        var is_init : bool
    }

    public func ecdsa_init(ctx : *mut ECDSAContext) {
        mpi_init(&raw mut ctx.pub_x)
        mpi_init(&raw mut ctx.pub_y)
        ctx.curve_id = 0
        ctx.is_init = false
    }

    // ─── Import ECDSA Public Key ──────────────────────────────────────────

    // Import an uncompressed ECDSA public key (65 bytes: 04 || X || Y)
    public func ecdsa_import_pubkey(ctx : *mut ECDSAContext,
                                     pub_key : *u8, pub_key_len : size_t,
                                     curve : u16) : int {
        if(pub_key_len < 65 || pub_key[0] != 0x04) {
            return ERR_ECP_BAD_INPUT_DATA
        }

        var ret = mpi_read_binary(&raw mut ctx.pub_x, &raw pub_key[1], 32)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut ctx.pub_y, &raw pub_key[33], 32)
        if(ret < 0) { return ret }

        ctx.curve_id = curve
        ctx.is_init = true
        return 0
    }

    // ─── Parse ECDSA Signature (ASN.1 DER) ───────────────────────────────

    // ECDSA signature is ASN.1 DER encoded:
    // SEQUENCE { INTEGER r, INTEGER s }
    // Returns 0 on success and fills r_bytes and s_bytes (32 bytes each for P-256)
    func ecdsa_parse_signature(sig : *u8, sig_len : size_t,
                                r_out : *mut u8, s_out : *mut u8) : int {
        // Parse SEQUENCE
        var pos : size_t = 0
        if(pos >= sig_len) { return ERR_ECDSA_BAD_SIGNATURE }
        if(sig[pos] != 0x30) { return ERR_ECDSA_BAD_SIGNATURE }
        pos += 1
        // Sequence length
        var seq_len : size_t = sig[pos] as size_t
        if((seq_len & 0x80) != 0) {
            var nb = (seq_len & 0x7F) as size_t
            seq_len = 0
            pos += 1
            var j : size_t = 0
            while(j < nb) {
                seq_len = (seq_len << 8) | (sig[pos + j] as size_t)
                j += 1
            }
            pos += nb - 1
        }
        pos += 1
        var sig_end = pos + seq_len

        // Parse INTEGER r
        if(pos >= sig_end) { return ERR_ECDSA_BAD_SIGNATURE }
        if(sig[pos] != 0x02) { return ERR_ECDSA_BAD_SIGNATURE }
        pos += 1
        var r_len : size_t = sig[pos] as size_t
        pos += 1
        if(pos + r_len > sig_end) { return ERR_ECDSA_BAD_SIGNATURE }
        // Handle leading zero byte in ASN.1 INTEGER (sign)
        var r_start = pos
        var r_bytes : size_t = r_len
        if(sig[r_start] == 0x00 && r_len > 1) {
            r_start += 1
            r_bytes -= 1
        }
        // Zero-pad to 32 bytes for P-256
        var pad_r : size_t = 0
        if(r_bytes < 32) {
            pad_r = 32 - r_bytes
        }
        var ri : size_t = 0
        while(ri < pad_r) { r_out[ri] = 0; ri += 1 }
        while(ri < 32 && ri - pad_r < r_bytes) {
            r_out[ri] = sig[r_start + ri - pad_r]
            ri += 1
        }
        pos += r_len

        // Parse INTEGER s
        if(pos >= sig_end) { return ERR_ECDSA_BAD_SIGNATURE }
        if(sig[pos] != 0x02) { return ERR_ECDSA_BAD_SIGNATURE }
        pos += 1
        var s_len : size_t = sig[pos] as size_t
        pos += 1
        if(pos + s_len > sig_end) { return ERR_ECDSA_BAD_SIGNATURE }
        var s_start = pos
        var s_bytes : size_t = s_len
        if(sig[s_start] == 0x00 && s_len > 1) {
            s_start += 1
            s_bytes -= 1
        }
        var pad_s : size_t = 0
        if(s_bytes < 32) {
            pad_s = 32 - s_bytes
        }
        var si : size_t = 0
        while(si < pad_s) { s_out[si] = 0; si += 1 }
        while(si < 32 && si - pad_s < s_bytes) {
            s_out[si] = sig[s_start + si - pad_s]
            si += 1
        }

        return 0
    }

    // ─── ECDSA Signature Verification (secp256r1) ────────────────────────

    // Verify an ECDSA signature.
    // hash: message digest (32 bytes for SHA-256, 48 for SHA-384, 64 for SHA-512)
    // hash_len: length of hash in bytes
    // sig: ASN.1 DER-encoded ECDSA signature
    // sig_len: length of signature
    // Returns 0 if valid, ERR_ECDSA_VERIFY_FAILED if invalid.
    public func ecdsa_verify(ctx : *mut ECDSAContext,
                              hash : *u8, hash_len : size_t,
                              sig : *u8, sig_len : size_t) : int {
        if(!ctx.is_init) { return ERR_ECDSA_VERIFY_FAILED }
        if(ctx.curve_id != TLS_GROUP_SECP256R1 as u16) {
            return ERR_ECP_FEATURE_UNAVAILABLE
        }

        // Parse signature: extract r and s (32 bytes each for P-256)
        var sig_r : [32]u8
        var sig_s : [32]u8
        var ret = ecdsa_parse_signature(sig, sig_len, &raw mut sig_r[0], &raw mut sig_s[0])
        if(ret < 0) { return ret }

        // Import r and s as Mpi
        var r : Mpi; mpi_init(&raw mut r)
        var s : Mpi; mpi_init(&raw mut s)
        ret = mpi_read_binary(&raw mut r, &raw sig_r[0], 32)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut s, &raw sig_s[0], 32)
        if(ret < 0) { return ret }

        // Get curve order n
        var n : Mpi; ecp_curve_n(&raw mut n)

        // Verify 1 <= r, s <= n-1
        if(mpi_cmp_int(&raw mut r, 1) < 0 || mpi_cmp(&raw mut r, &raw mut n) >= 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }
        if(mpi_cmp_int(&raw mut s, 1) < 0 || mpi_cmp(&raw mut s, &raw mut n) >= 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        // Enforce low-S (BIP-62): s <= n/2
        var half_n : Mpi; mpi_init(&raw mut half_n)
        mpi_copy(&raw mut half_n, &raw mut n)
        ret = mpi_shift_r(&raw mut half_n, 1)
        if(ret < 0) { return ret }
        if(mpi_cmp(&raw mut s, &raw mut half_n) > 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        // e = HASH(message), truncated to bitlen(n)
        var n_bitlen = mpi_bitlen(&raw mut n)
        var e : Mpi; mpi_init(&raw mut e)
        // For P-256: n_bitlen = 256, hash_len <= 32
        var e_len = hash_len
        if(e_len > (n_bitlen / 8) + 1) { e_len = (n_bitlen / 8) + 1 }
        ret = mpi_read_binary(&raw mut e, hash, e_len)
        if(ret < 0) { return ret }
        // If hash bitlen > n_bitlen, shift right
        if((hash_len * 8) > n_bitlen as size_t) {
            var shift = (hash_len * 8) - n_bitlen as size_t
            ret = mpi_shift_r(&raw mut e, shift as size_t)
            if(ret < 0) { return ret }
        }

        // w = s^(-1) mod n
        var w : Mpi; mpi_init(&raw mut w)
        ret = mpi_mod_inv(&raw mut w, &raw mut s, &raw mut n)
        if(ret < 0) { return ERR_ECDSA_VERIFY_FAILED }

        // u1 = (e * w) mod n
        var u1 : Mpi; mpi_init(&raw mut u1)
        ret = mpi_mul(&raw mut u1, &raw mut e, &raw mut w)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut u1, &raw mut u1, &raw mut n)
        if(ret < 0) { return ret }

        // u2 = (r * w) mod n
        var u2 : Mpi; mpi_init(&raw mut u2)
        ret = mpi_mul(&raw mut u2, &raw mut r, &raw mut w)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut u2, &raw mut u2, &raw mut n)
        if(ret < 0) { return ret }

        // R = u1 * G + u2 * Q
        // Build generator point G from P-256 constants
        var G : ECPPoint; ecp_point_init(&raw mut G)
        mpi_grow(&raw mut G.X, 8); G.X.n = 8
        mpi_grow(&raw mut G.Y, 8); G.Y.n = 8
        var gi : size_t = 0
        while(gi < 8) { G.X.p[gi] = P256_GX[gi]; gi += 1 }
        gi = 0
        while(gi < 8) { G.Y.p[gi] = P256_GY[gi]; gi += 1 }
        mpi_lset(&raw mut G.Z, 1)

        // Build public key point Q from context
        var Q : ECPPoint; ecp_point_init(&raw mut Q)
        mpi_copy(&raw mut Q.X, &raw mut ctx.pub_x)
        mpi_copy(&raw mut Q.Y, &raw mut ctx.pub_y)
        mpi_lset(&raw mut Q.Z, 1)

        // R1 = u1 * G
        var R1 : ECPPoint; ecp_point_init(&raw mut R1)
        ret = ecp_mul(&raw mut R1, &raw mut u1, &raw mut G)
        if(ret < 0) { return ret }

        // R2 = u2 * Q
        var R2 : ECPPoint; ecp_point_init(&raw mut R2)
        ret = ecp_mul(&raw mut R2, &raw mut u2, &raw mut Q)
        if(ret < 0) { return ret }

        // R = R1 + R2
        var R : ECPPoint; ecp_point_init(&raw mut R)
        ret = ecp_add_jac(&raw mut R, &raw mut R1, &raw mut R2)
        if(ret < 0) { return ret }

        // Normalize R to affine
        ret = ecp_normalize_jac(&raw mut R)
        if(ret < 0) { return ret }

        // If R is point at infinity, reject
        if(mpi_is_zero(&raw mut R.Z)) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        // v = R.x mod n
        var v : Mpi; mpi_init(&raw mut v)
        ret = mpi_mod(&raw mut v, &raw mut R.X, &raw mut n)
        if(ret < 0) { return ret }

        // Verify v == r
        if(mpi_cmp(&raw mut v, &raw mut r) != 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        return 0
    }

} // namespace tls
