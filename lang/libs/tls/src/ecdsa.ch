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
        var priv_key : Mpi    // Private key scalar (for signing)
        var curve_id : u16    // Named curve (TLS_GROUP_SECP256R1 etc.)
        var is_init : bool
        var has_private : bool
    }

    public func ecdsa_init(ctx : *mut ECDSAContext) {
        mpi_init(&raw mut ctx.pub_x)
        mpi_init(&raw mut ctx.pub_y)
        mpi_init(&raw mut ctx.priv_key)
        ctx.curve_id = 0
        ctx.is_init = false
        ctx.has_private = false
    }

    // ─── Import ECDSA Public Key ──────────────────────────────────────────

    // Import an uncompressed ECDSA public key (65 bytes: 04 || X || Y)
    public func ecdsa_import_pubkey(ctx : *mut ECDSAContext,
                                     pub_key : *u8, pub_key_len : size_t,
                                     curve : u16) : int {
        var coord : size_t = 32
        if(curve == TLS_GROUP_SECP384R1 as u16) { coord = 48 }
        if(pub_key_len < 65 || pub_key[0] != 0x04) {
            return ERR_ECP_BAD_INPUT_DATA
        }

        var ret = mpi_read_binary(&raw mut ctx.pub_x, &raw pub_key[1], coord)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut ctx.pub_y, &raw pub_key[1 + coord], coord)
        if(ret < 0) { return ret }

        ctx.curve_id = curve
        ctx.is_init = true
        return 0
    }

    // Import raw private key scalar (32 bytes for P-256)
    public func ecdsa_import_privkey(ctx : *mut ECDSAContext,
                                      priv_key : *u8, priv_key_len : size_t,
                                      curve : u16) : int {
        var ret = mpi_read_binary(&raw mut ctx.priv_key, priv_key, priv_key_len)
        if(ret < 0) { return ret }
        ctx.curve_id = curve
        ctx.has_private = true
        ctx.is_init = true
        return 0
    }

    // ─── ECDSA Signing (secp256r1) ────────────────────────────────────────

    public func ecdsa_sign(ctx : *mut ECDSAContext,
                            hash : *u8, hash_len : size_t,
                            sig_out : *mut u8, sig_out_len : *mut u16) : int {
        if(tls_config::DEBUG_LOG) printf("[ECDSA_DBG] has_priv=%d curve=%d\n", ctx.has_private as int, ctx.curve_id as int)
        if(!ctx.has_private) { if(tls_config::DEBUG_LOG) printf("[ECDSA_DBG] no private key!\n"); return ERR_ECDSA_VERIFY_FAILED }
        var coord_bytes : size_t = 32
        var curve_select : int = 0
        if(ctx.curve_id == TLS_GROUP_SECP384R1 as u16) {
            coord_bytes = 48
            curve_select = 1
        } else if(ctx.curve_id != TLS_GROUP_SECP256R1 as u16) {
            return ERR_ECP_FEATURE_UNAVAILABLE
        }
        ecp_select_curve(curve_select)

        var n : Mpi; ecp_curve_n(unsafe(&raw mut n))
        var e : Mpi; mpi_init(unsafe(&raw mut e))
        var ret = mpi_read_binary(unsafe(&raw mut e), hash, hash_len)
        if(tls_config::DEBUG_LOG) printf("[ECDSA_DBG] read_hash=%d\n", ret)
        if(ret < 0) { return ret }

        var G : ECPPoint; ecp_point_init(unsafe(&raw mut G))
        ecp_curve_gx(unsafe(&raw mut G.X))
        ecp_curve_gy(unsafe(&raw mut G.Y))
        mpi_lset(unsafe(&raw mut G.Z), 1)
        if(tls_config::DEBUG_LOG) printf("[ECDSA_DBG] G_ok\n")

        var k : Mpi; mpi_init(unsafe(&raw mut k))
        var r_val : Mpi; mpi_init(unsafe(&raw mut r_val))
        var s_val : Mpi; mpi_init(unsafe(&raw mut s_val))
        var k_inv : Mpi; mpi_init(unsafe(&raw mut k_inv))
        var temp : Mpi; mpi_init(unsafe(&raw mut temp))
        var R : ECPPoint; ecp_point_init(unsafe(&raw mut R))

        var attempts : i32 = 0
        var k_bytes : [48]u8
        var r_bytes : [48]u8
        var s_bytes : [48]u8
        var r_body_len : size_t = 0
        var s_body_len : size_t = 0

        while(attempts < 100) {
            ret = random_fill(&raw mut k_bytes[0], coord_bytes)
            if(ret < 0) { return ret }
            k_bytes[0] = (k_bytes[0] & 0x7F) as u8
            ret = mpi_read_binary(unsafe(&raw mut k), &raw k_bytes[0], coord_bytes)
            if(ret < 0) { return ret }
            ret = mpi_mod(unsafe(&raw mut k), unsafe(&raw mut k), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            if(mpi_cmp_int(unsafe(&raw mut k), 1) < 0) { attempts += 1; continue }

            ret = ecp_mul(unsafe(&raw mut R), unsafe(&raw mut k), unsafe(&raw mut G))
            if(ret < 0) { return ret }
            ret = ecp_normalize_jac(unsafe(&raw mut R))
            if(ret < 0) { return ret }

            ret = mpi_mod(unsafe(&raw mut r_val), unsafe(&raw mut R.X), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            if(mpi_cmp_int(unsafe(&raw mut r_val), 0) == 0) { attempts += 1; continue }

            ret = mpi_mod_inv(unsafe(&raw mut k_inv), unsafe(&raw mut k), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            ret = mpi_mul(unsafe(&raw mut temp), unsafe(&raw mut r_val), &raw mut ctx.priv_key)
            if(ret < 0) { return ret }
            ret = mpi_mod(unsafe(&raw mut temp), unsafe(&raw mut temp), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            ret = mpi_add(unsafe(&raw mut temp), unsafe(&raw mut temp), unsafe(&raw mut e))
            if(ret < 0) { return ret }
            ret = mpi_mod(unsafe(&raw mut temp), unsafe(&raw mut temp), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            ret = mpi_mul(unsafe(&raw mut s_val), unsafe(&raw mut k_inv), unsafe(&raw mut temp))
            if(ret < 0) { return ret }
            ret = mpi_mod(unsafe(&raw mut s_val), unsafe(&raw mut s_val), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            if(mpi_cmp_int(unsafe(&raw mut s_val), 0) == 0) { attempts += 1; continue }

            break
        }
        if(attempts >= 100) { return ERR_ECDSA_VERIFY_FAILED }

        ret = mpi_write_binary(unsafe(&raw mut r_val), &raw mut r_bytes[0], coord_bytes)
        if(ret < 0) { return ret }
        ret = mpi_write_binary(unsafe(&raw mut s_val), &raw mut s_bytes[0], coord_bytes)
        if(ret < 0) { return ret }

        // DER encode: SEQUENCE { INTEGER r, INTEGER s }
        var r_start : size_t = 0
        while(r_start < coord_bytes && r_bytes[r_start] == 0) { r_start += 1 }
        var r_need : bool = (r_start < coord_bytes && (r_bytes[r_start] & 0x80) != 0)
        if(r_need) { r_body_len = (coord_bytes - r_start) + 1 } else { r_body_len = coord_bytes - r_start }

        var s_start : size_t = 0
        while(s_start < coord_bytes && s_bytes[s_start] == 0) { s_start += 1 }
        var s_need : bool = (s_start < coord_bytes && (s_bytes[s_start] & 0x80) != 0)
        if(s_need) { s_body_len = (coord_bytes - s_start) + 1 } else { s_body_len = coord_bytes - s_start }

        var total_content = 2 + r_body_len + 2 + s_body_len

        var pos : size_t = 0
        sig_out[pos] = 0x30 as u8; pos += 1
        if(total_content < 128) {
            sig_out[pos] = total_content as u8; pos += 1
        } else {
            sig_out[pos] = 0x81 as u8; pos += 1
            sig_out[pos] = total_content as u8; pos += 1
        }

        sig_out[pos] = 0x02 as u8; pos += 1
        sig_out[pos] = r_body_len as u8; pos += 1
        if(r_need) { sig_out[pos] = 0; pos += 1 }
        var ri : size_t = 0
        while(ri < coord_bytes - r_start) { sig_out[pos + ri] = r_bytes[r_start + ri]; ri += 1 }
        pos += ri

        sig_out[pos] = 0x02 as u8; pos += 1
        sig_out[pos] = s_body_len as u8; pos += 1
        if(s_need) { sig_out[pos] = 0; pos += 1 }
        var si : size_t = 0
        while(si < coord_bytes - s_start) { sig_out[pos + si] = s_bytes[s_start + si]; si += 1 }
        pos += si

        *sig_out_len = pos as u16
        return 0
    }

    // ─── Parse ECDSA Signature (ASN.1 DER) ───────────────────────────────

    // ECDSA signature is ASN.1 DER encoded:
    // SEQUENCE { INTEGER r, INTEGER s }
    // Returns 0 on success and fills r_bytes and s_bytes (coord_bytes each,
    // 32 for P-256, 48 for P-384).
    func ecdsa_parse_signature_ext(sig : *u8, sig_len : size_t,
                                    r_out : *mut u8, s_out : *mut u8,
                                    coord_bytes : size_t) : int {
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
        // Zero-pad to coord_bytes
        var pad_r : size_t = 0
        if(r_bytes < coord_bytes) {
            pad_r = coord_bytes - r_bytes
        }
        var ri : size_t = 0
        while(ri < pad_r) { r_out[ri] = 0; ri += 1 }
        while(ri < coord_bytes && ri - pad_r < r_bytes) {
            r_out[ri] = sig[r_start + ri - pad_r]
            ri += 1
        }
        // r must fit in the coordinate width; leading 0x00 padding is handled
        // above, so a leftover non-zero r byte means the integer is too big.
        if(r_bytes > coord_bytes) {
            return ERR_ECDSA_BAD_SIGNATURE
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
        if(s_bytes < coord_bytes) {
            pad_s = coord_bytes - s_bytes
        }
        var si : size_t = 0
        while(si < pad_s) { s_out[si] = 0; si += 1 }
        while(si < coord_bytes && si - pad_s < s_bytes) {
            s_out[si] = sig[s_start + si - pad_s]
            si += 1
        }
        if(s_bytes > coord_bytes) {
            return ERR_ECDSA_BAD_SIGNATURE
        }

        return 0
    }

    // P-256 signature parse (kept for compatibility).
    func ecdsa_parse_signature(sig : *u8, sig_len : size_t,
                                r_out : *mut u8, s_out : *mut u8) : int {
        return ecdsa_parse_signature_ext(sig, sig_len, r_out, s_out, 32)
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
        var coord_bytes : size_t = 32
        var curve_select : int = 0
        if(ctx.curve_id == TLS_GROUP_SECP384R1 as u16) {
            coord_bytes = 48
            curve_select = 1
        } else if(ctx.curve_id != TLS_GROUP_SECP256R1 as u16) {
            return ERR_ECP_FEATURE_UNAVAILABLE
        }
        // Activate the matching curve constants for the ecp_* helpers.
        ecp_select_curve(curve_select)

        var sig_r : [64]u8
        var sig_s : [64]u8
        var ret = ecdsa_parse_signature_ext(sig, sig_len, &raw mut sig_r[0], &raw mut sig_s[0], coord_bytes)
        if(ret < 0) { return ret }

        // Import r and s as Mpi
        var r : Mpi; mpi_init(unsafe(&raw mut r))
        var s : Mpi; mpi_init(unsafe(&raw mut s))
        ret = mpi_read_binary(unsafe(&raw mut r), &raw sig_r[0], coord_bytes)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(unsafe(&raw mut s), &raw sig_s[0], coord_bytes)
        if(ret < 0) { return ret }

        // Get curve order n
        var n : Mpi; ecp_curve_n(unsafe(&raw mut n))

        // Verify 1 <= r, s <= n-1
        if(mpi_cmp_int(unsafe(&raw mut r), 1) < 0 || mpi_cmp(unsafe(&raw mut r), unsafe(&raw mut n)) >= 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }
        if(mpi_cmp_int(unsafe(&raw mut s), 1) < 0 || mpi_cmp(unsafe(&raw mut s), unsafe(&raw mut n)) >= 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        // e = HASH(message), truncated to the leftmost bitlen(n) bits.
        // Per FIPS 186-4, when the digest is longer than the curve order, use
        // the leftmost bits of the digest (not a right shift of a longer
        // read): for P-256 (bitlen 256) that is the first 32 bytes.
        var n_bitlen = mpi_bitlen(unsafe(&raw mut n))
        var e : Mpi; mpi_init(unsafe(&raw mut e))
        var e_bytes = (n_bitlen / 8) as size_t
        if((n_bitlen % 8) != 0) { e_bytes += 1 }
        var e_len = hash_len
        if(e_len > e_bytes) { e_len = e_bytes }
        ret = mpi_read_binary(unsafe(&raw mut e), hash, e_len)
        if(ret < 0) { return ret }
        // If the curve group size is not a whole byte multiple, mask the top
        // bits so the value fits exactly in bitlen(n) bits.
        if(e_bytes * 8 > (n_bitlen as size_t)) {
            var extra = e_bytes * 8 - (n_bitlen as size_t)
            if(extra > 0 && extra < 8) {
                ret = mpi_shift_r(unsafe(&raw mut e), extra)
                if(ret < 0) { return ret }
            }
        }

        // w = s^(-1) mod n
        var w : Mpi; mpi_init(unsafe(&raw mut w))
        ret = mpi_mod_inv(unsafe(&raw mut w), unsafe(&raw mut s), unsafe(&raw mut n))
        if(ret < 0) { return ERR_ECDSA_VERIFY_FAILED }

        // u1 = (e * w) mod n
        var u1 : Mpi; mpi_init(unsafe(&raw mut u1))
        ret = mpi_mul(unsafe(&raw mut u1), unsafe(&raw mut e), unsafe(&raw mut w))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut u1), unsafe(&raw mut u1), unsafe(&raw mut n))
        if(ret < 0) { return ret }

        // u2 = (r * w) mod n
        var u2 : Mpi; mpi_init(unsafe(&raw mut u2))
        ret = mpi_mul(unsafe(&raw mut u2), unsafe(&raw mut r), unsafe(&raw mut w))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut u2), unsafe(&raw mut u2), unsafe(&raw mut n))
        if(ret < 0) { return ret }

        // R = u1 * G + u2 * Q
        // Build generator point G from the active curve constants.
        var G : ECPPoint; ecp_point_init(unsafe(&raw mut G))
        ecp_curve_gx(unsafe(&raw mut G.X))
        ecp_curve_gy(unsafe(&raw mut G.Y))
        mpi_lset(unsafe(&raw mut G.Z), 1)

        // Build public key point Q from context
        var Q : ECPPoint; ecp_point_init(unsafe(&raw mut Q))
        mpi_copy(unsafe(&raw mut Q.X), &raw mut ctx.pub_x)
        mpi_copy(unsafe(&raw mut Q.Y), &raw mut ctx.pub_y)
        mpi_lset(unsafe(&raw mut Q.Z), 1)

        // R1 = u1 * G
        var R1 : ECPPoint; ecp_point_init(unsafe(&raw mut R1))
        ret = ecp_mul(unsafe(&raw mut R1), unsafe(&raw mut u1), unsafe(&raw mut G))
        if(ret < 0) { return ret }

        // R2 = u2 * Q
        var R2 : ECPPoint; ecp_point_init(unsafe(&raw mut R2))
        ret = ecp_mul(unsafe(&raw mut R2), unsafe(&raw mut u2), unsafe(&raw mut Q))
        if(ret < 0) { return ret }

        // R = R1 + R2
        // ecp_add_jac is a mixed Jacobian-affine addition and requires the
        // second operand to be in affine coordinates (Z = 1). R2 is the
        // un-normalized Jacobian result of ecp_mul, so normalize it first.
        var R : ECPPoint; ecp_point_init(unsafe(&raw mut R))
        ret = ecp_normalize_jac(unsafe(&raw mut R2))
        if(ret < 0) { return ret }
        ret = ecp_add_jac(unsafe(&raw mut R), unsafe(&raw mut R1), unsafe(&raw mut R2))
        if(ret < 0) { return ret }

        // Normalize R to affine
        ret = ecp_normalize_jac(unsafe(&raw mut R))
        if(ret < 0) { return ret }

        // If R is point at infinity, reject
        if(mpi_is_zero(unsafe(&raw mut R.Z))) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        // v = R.x mod n
        var v : Mpi; mpi_init(unsafe(&raw mut v))
        ret = mpi_mod(unsafe(&raw mut v), unsafe(&raw mut R.X), unsafe(&raw mut n))
        if(ret < 0) { return ret }

        // Verify v == r
        if(mpi_cmp(unsafe(&raw mut v), unsafe(&raw mut r)) != 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        return 0
    }

} // namespace tls
