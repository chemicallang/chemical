// ============================================================================
// ECDH — Elliptic Curve Diffie-Hellman (P-256 / secp256r1)
// ============================================================================
// Port of mbedTLS ecdh.h / ecdh.c and ecp.h / ecp.c to Chemical.
// Supports the secp256r1 (P-256) curve for ECDHE key exchange.
// ============================================================================

public namespace tls {

    // ─── ECDH Error Codes ───────────────────────────────────────────────────

    public comptime const ERR_ECP_BAD_INPUT_DATA = -0x4F80
    public comptime const ERR_ECP_BUFFER_TOO_SMALL = -0x4F00
    public comptime const ERR_ECP_FEATURE_UNAVAILABLE = -0x4E80
    public comptime const ERR_ECP_VERIFY_FAILED = -0x4E00
    public comptime const ERR_ECP_ALLOC_FAILED = -0x4D80
    public comptime const ERR_ECP_INVALID_KEY = -0x4C80

    // ─── secp256r1 (P-256) Curve Parameters ─────────────────────────────────

    // Prime p = 2^256 - 2^224 + 2^192 + 2^96 - 1
    var P256_P : [8]u32 = [
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0x00000000u32,
        0x00000000u32, 0x00000000u32, 0x00000001u32, 0xFFFFFFFFu32
    ]

    // Order n = FFFFFFFF 00000000 FFFFFFFF FFFFFFFF BCE6FAAD A7179E84 F3B9CAC2 FC632551
    var P256_N : [8]u32 = [
        0xFC632551u32, 0xF3B9CAC2u32, 0xA7179E84u32, 0xBCE6FAADu32,
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0x00000000u32, 0xFFFFFFFFu32
    ]

    // Generator Gx = 6B17D1F2 E12C4247 F8BCE6E5 63A440F2 77037D81 2DEB33A0 F4A13945 D898C296
    var P256_GX : [8]u32 = [
        0xD898C296u32, 0xF4A13945u32, 0x2DEB33A0u32, 0x77037D81u32,
        0x63A440F2u32, 0xF8BCE6E5u32, 0xE12C4247u32, 0x6B17D1F2u32
    ]

    // Generator Gy = 4FE342E2 FE1A7F9B 8EE7EB4A 7C0F9E16 2BCE3357 6B315ECE CBB64068 37BF51F5
    var P256_GY : [8]u32 = [
        0x37BF51F5u32, 0xCBB64068u32, 0x6B315ECEu32, 0x2BCE3357u32,
        0x7C0F9E16u32, 0x8EE7EB4Au32, 0xFE1A7F9Bu32, 0x4FE342E2u32
    ]

    // Curve coefficient b = 5AC635D8 AA3A93E7 B3EBBD55 769886BC 651D06B0 CC53B0F6 3BCE3C3E 27D2604B
    var P256_B : [8]u32 = [
        0x27D2604Bu32, 0x3BCE3C3Eu32, 0xCC53B0F6u32, 0x651D06B0u32,
        0x769886BCu32, 0xB3EBBD55u32, 0xAA3A93E7u32, 0x5AC635D8u32
    ]

    // ─── Elliptic Curve Point ───────────────────────────────────────────────

    public struct ECPPoint {
        var X : Mpi
        var Y : Mpi
        var Z : Mpi
    }

    public func ecp_point_init(p : *mut ECPPoint) {
        mpi_init(&raw mut p.X); mpi_init(&raw mut p.Y); mpi_init(&raw mut p.Z)
    }

    // ─── ECDH Key Pair ──────────────────────────────────────────────────────

    public struct ECDHContext {
        var priv_key : Mpi         // Private key (scalar)
        var pub_key : ECPPoint     // Public key (point)
        var is_init : bool
    }

    public func ecdh_init(ctx : *mut ECDHContext) {
        mpi_init(&raw mut ctx.priv_key)
        ecp_point_init(&raw mut ctx.pub_key)
        ctx.is_init = false
    }

    // ─── Modular arithmetic helpers for P-256 ────────────────────────────

    // Load curve parameter p into an Mpi
    func ecp_curve_p(p : *mut Mpi) {
        mpi_init(p)
        p.n = 8
        var i : size_t = 0
        while(i < 8) { p.p[i] = P256_P[i]; i += 1 }
    }

    func ecp_curve_n(p : *mut Mpi) {
        mpi_init(p)
        p.n = 8
        var i : size_t = 0
        while(i < 8) { p.p[i] = P256_N[i]; i += 1 }
    }

    // Modulo P-256 prime: a mod p (for values < 2*p)
    func ecp_mod_p256(X : *mut Mpi) : int {
        // Fast reduction for P-256 (2^256 - 2^224 + 2^192 + 2^96 - 1)
        // For now, use general mod
        var p : Mpi; ecp_curve_p(&raw mut p)
        return mpi_mod(X, X, &raw mut p)
    }

    // ─── Point Operations on P-256 ──────────────────────────────────────

    // Point double: R = 2 * P on the P-256 curve
    func ecp_double_jac(R : *mut ECPPoint, P : *mut ECPPoint) : int {
        // Jacobian coordinate doubling formula
        // For curve y^2 = x^3 + a*x + b with a = -3 (for P-256):
        // If P = (X1, Y1, Z1), then 2*P = (X3, Y3, Z3) where:
        // W = 3*X1^2 + a*Z1^4   (a = -3 mod p, so = 3*(X1 + Z1^2)*(X1 - Z1^2))
        // S = 4*X1*Y1^2
        // X3 = W^2 - 2*S
        // Y3 = W*(S - X3) - 8*Y1^4
        // Z3 = 2*Y1*Z1

        // For P-256, a = -3, so we can use: W = 3*(X1 + Z1^2)*(X1 - Z1^2)

        var p : Mpi; ecp_curve_p(&raw mut p)
        var t1 : Mpi; mpi_init(&raw mut t1)
        var t2 : Mpi; mpi_init(&raw mut t2)
        var t3 : Mpi; mpi_init(&raw mut t3)
        var X3 : Mpi; mpi_init(&raw mut X3)
        var Y3 : Mpi; mpi_init(&raw mut Y3)
        var Z3 : Mpi; mpi_init(&raw mut Z3)
        var ret : int = 0

        // t1 = Z1^2 mod p
        ret = mpi_mul(&raw mut t1, &raw mut P.Z, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t1, &raw mut t1, &raw mut p)
        if(ret < 0) { return ret }

        // t2 = X1 - t1 = X1 - Z1^2
        ret = mpi_sub(&raw mut t2, &raw mut P.X, &raw mut t1)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t2, &raw mut t2, &raw mut p)
        if(ret < 0) { return ret }

        // t1 = X1 + t1 = X1 + Z1^2
        ret = mpi_add(&raw mut t1, &raw mut P.X, &raw mut t1)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t1, &raw mut t1, &raw mut p)

        // t2 = t1 * t2 = (X1 + Z1^2) * (X1 - Z1^2) = X1^2 - Z1^4
        ret = mpi_mul(&raw mut t2, &raw mut t1, &raw mut t2)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t2, &raw mut t2, &raw mut p)

        // t2 = 3 * t2 = 3 * (X1^2 - Z1^4) = W
        ret = mpi_mul_int(&raw mut t2, &raw mut t2, 3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t2, &raw mut t2, &raw mut p)

        // t3 = Y1^2
        ret = mpi_mul(&raw mut t3, &raw mut P.Y, &raw mut P.Y)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t3, &raw mut t3, &raw mut p)

        // t1 = 4 * X1 * Y1^2 = 4 * X1 * t3
        ret = mpi_mul(&raw mut t1, &raw mut P.X, &raw mut t3)
        if(ret < 0) { return ret }
        ret = mpi_mul_int(&raw mut t1, &raw mut t1, 4)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t1, &raw mut t1, &raw mut p)

        // X3 = W^2 - 2*S = t2^2 - 2*t1
        ret = mpi_mul(&raw mut X3, &raw mut t2, &raw mut t2)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X3, &raw mut X3, &raw mut p)
        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut t1)
        if(ret < 0) { return ret }
        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut t1)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X3, &raw mut X3, &raw mut p)

        // Y3 = W*(S - X3) - 8*Y1^4
        // t3 = Y1^2 (already computed), so Y1^4 = t3^2
        ret = mpi_mul(&raw mut t3, &raw mut t3, &raw mut t3)
        if(ret < 0) { return ret }
        ret = mpi_mul_int(&raw mut t3, &raw mut t3, 8)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t3, &raw mut t3, &raw mut p)

        // t1 - X3 = S - X3
        ret = mpi_sub(&raw mut Y3, &raw mut t1, &raw mut X3)
        if(ret < 0) { return ret }
        ret = mpi_mul(&raw mut Y3, &raw mut t2, &raw mut Y3)
        if(ret < 0) { return ret }
        ret = mpi_sub(&raw mut Y3, &raw mut Y3, &raw mut t3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Y3, &raw mut Y3, &raw mut p)

        // Z3 = 2*Y1*Z1
        ret = mpi_mul(&raw mut Z3, &raw mut P.Y, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mul_int(&raw mut Z3, &raw mut Z3, 2)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Z3, &raw mut Z3, &raw mut p)

        // Copy result
        mpi_copy(&raw mut R.X, &raw mut X3)
        mpi_copy(&raw mut R.Y, &raw mut Y3)
        mpi_copy(&raw mut R.Z, &raw mut Z3)

        return 0
    }

    // Point addition: R = P + Q (with Q in affine coordinates, Z=1)
    func ecp_add_jac(R : *mut ECPPoint, P : *mut ECPPoint, Q : *mut ECPPoint) : int {
        // Mixed Jacobian-affine addition
        // P = (X1, Y1, Z1), Q = (X2, Y2, 1)
        // R = (X3, Y3, Z3) = P + Q

        var p : Mpi; ecp_curve_p(&raw mut p)
        var t1 : Mpi; mpi_init(&raw mut t1)
        var t2 : Mpi; mpi_init(&raw mut t2)
        var t3 : Mpi; mpi_init(&raw mut t3)
        var t4 : Mpi; mpi_init(&raw mut t4)
        var X3 : Mpi; mpi_init(&raw mut X3)
        var Y3 : Mpi; mpi_init(&raw mut Y3)
        var Z3 : Mpi; mpi_init(&raw mut Z3)
        var ret : int = 0

        // t1 = Z1^2
        ret = mpi_mul(&raw mut t1, &raw mut P.Z, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t1, &raw mut t1, &raw mut p)

        // t2 = Z1 * t1 = Z1^3
        ret = mpi_mul(&raw mut t2, &raw mut P.Z, &raw mut t1)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t2, &raw mut t2, &raw mut p)

        // t1 = t1 * X2 = Z1^2 * X2
        // t2 = t2 * Y2 = Z1^3 * Y2
        ret = mpi_mul(&raw mut t1, &raw mut t1, &raw mut Q.X)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t1, &raw mut t1, &raw mut p)

        ret = mpi_mul(&raw mut t2, &raw mut t2, &raw mut Q.Y)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t2, &raw mut t2, &raw mut p)

        // t3 = t1 - X1 = Z1^2 * X2 - X1 = H
        ret = mpi_sub(&raw mut t3, &raw mut t1, &raw mut P.X)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t3, &raw mut t3, &raw mut p)

        // t4 = t2 - Y1 = Z1^3 * Y2 - Y1 = R
        ret = mpi_sub(&raw mut t4, &raw mut t2, &raw mut P.Y)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t4, &raw mut t4, &raw mut p)

        // Z3 = Z1 * H = Z1 * t3
        ret = mpi_mul(&raw mut Z3, &raw mut P.Z, &raw mut t3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Z3, &raw mut Z3, &raw mut p)

        // H^2 = t3^2
        ret = mpi_mul(&raw mut t1, &raw mut t3, &raw mut t3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t1, &raw mut t1, &raw mut p)

        // H^3 = t1 * t3
        ret = mpi_mul(&raw mut t2, &raw mut t1, &raw mut t3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t2, &raw mut t2, &raw mut p)

        // X3 = R^2 - H^3 - 2*X1*H^2
        ret = mpi_mul(&raw mut X3, &raw mut t4, &raw mut t4)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X3, &raw mut X3, &raw mut p)

        ret = mpi_mul(&raw mut t3, &raw mut P.X, &raw mut t1)
        if(ret < 0) { return ret }
        ret = mpi_mul_int(&raw mut t3, &raw mut t3, 2)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t3, &raw mut t3, &raw mut p)

        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut t2)
        if(ret < 0) { return ret }
        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut t3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X3, &raw mut X3, &raw mut p)

        // Y3 = R*(X1*H^2 - X3) - Y1*H^3
        ret = mpi_sub(&raw mut t3, &raw mut t3, &raw mut X3)
        // Actually t3 = 2*X1*H^2, we want just X1*H^2. Let me redo:
        // t3 = X1 * H^2
        ret = mpi_mul(&raw mut t3, &raw mut P.X, &raw mut t1)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t3, &raw mut t3, &raw mut p)

        ret = mpi_sub(&raw mut t3, &raw mut t3, &raw mut X3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut t3, &raw mut t3, &raw mut p)

        ret = mpi_mul(&raw mut t3, &raw mut t4, &raw mut t3)
        if(ret < 0) { return ret }

        ret = mpi_mul(&raw mut t4, &raw mut P.Y, &raw mut t2)
        if(ret < 0) { return ret }

        ret = mpi_sub(&raw mut Y3, &raw mut t3, &raw mut t4)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Y3, &raw mut Y3, &raw mut p)

        // Copy result
        mpi_copy(&raw mut R.X, &raw mut X3)
        mpi_copy(&raw mut R.Y, &raw mut Y3)
        mpi_copy(&raw mut R.Z, &raw mut Z3)

        return 0
    }

    // ─── Scalar Multiplication (Double-and-Add) ──────────────────────────

    // R = k * P on P-256 using Montgomery ladder for safety
    func ecp_mul(R : *mut ECPPoint, k : *mut Mpi, P : *mut ECPPoint) : int {
        // Montgomery ladder: R = k * P
        // R0 = infinity, R1 = P
        // For each bit of k from MSB to LSB:
        //   if bit == 0: R1 = R0 + R1, R0 = 2 * R0
        //   if bit == 1: R0 = R0 + R1, R1 = 2 * R1
        // Result = R0

        // Initialize R0 = O (point at infinity = (1, 1, 0))
        var R0 : ECPPoint; ecp_point_init(&raw mut R0)
        mpi_lset(&raw mut R0.X, 1)
        mpi_lset(&raw mut R0.Y, 1)
        mpi_lset(&raw mut R0.Z, 0)

        var R1 : ECPPoint; ecp_point_init(&raw mut R1)
        mpi_copy(&raw mut R1.X, &raw mut P.X)
        mpi_copy(&raw mut R1.Y, &raw mut P.Y)
        mpi_copy(&raw mut R1.Z, &raw mut P.Z)

        var bitlen = mpi_bitlen(k)
        var i = bitlen
        while(i > 0) {
            i -= 1
            var limb_idx = i / BITS_PER_LIMB
            var bit_idx = i % BITS_PER_LIMB
            var bit = (k.p[limb_idx] >> bit_idx) & 1

            if(bit == 0) {
                // R1 = R0 + R1, R0 = 2 * R0
                var ret = ecp_add_jac(&raw mut R1, &raw mut R0, &raw mut R1)
                if(ret < 0) { return ret }
                ret = ecp_double_jac(&raw mut R0, &raw mut R0)
                if(ret < 0) { return ret }
            } else {
                // R0 = R0 + R1, R1 = 2 * R1
                var ret = ecp_add_jac(&raw mut R0, &raw mut R0, &raw mut R1)
                if(ret < 0) { return ret }
                ret = ecp_double_jac(&raw mut R1, &raw mut R1)
                if(ret < 0) { return ret }
            }
        }

        mpi_copy(&raw mut R.X, &raw mut R0.X)
        mpi_copy(&raw mut R.Y, &raw mut R0.Y)
        mpi_copy(&raw mut R.Z, &raw mut R0.Z)

        return 0
    }

    // ─── Point Normalization (Jacobian to Affine) ────────────────────────

    // Convert Jacobian coordinates to affine: (X/Z^2, Y/Z^3, 1)
    func ecp_normalize_jac(P : *mut ECPPoint) : int {
        if(mpi_is_zero(&raw mut P.Z)) {
            // Point at infinity
            mpi_lset(&raw mut P.X, 1)
            mpi_lset(&raw mut P.Y, 1)
            return 0
        }

        var p : Mpi; ecp_curve_p(&raw mut p)
        var zi : Mpi; mpi_init(&raw mut zi)
        var zi2 : Mpi; mpi_init(&raw mut zi2)
        var ret : int = 0

        // zi = Z^-1 mod p
        ret = mpi_mod_inv(&raw mut zi, &raw mut P.Z, &raw mut p)
        if(ret < 0) { return ret }

        // zi2 = zi^2 mod p
        ret = mpi_mul(&raw mut zi2, &raw mut zi, &raw mut zi)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut zi2, &raw mut zi2, &raw mut p)

        // X = X * zi^2 mod p
        ret = mpi_mul(&raw mut P.X, &raw mut P.X, &raw mut zi2)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut P.X, &raw mut P.X, &raw mut p)

        // Y = Y * zi * zi2 = Y * zi^3 mod p
        ret = mpi_mul(&raw mut P.Y, &raw mut P.Y, &raw mut zi)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut P.Y, &raw mut P.Y, &raw mut p)
        ret = mpi_mul(&raw mut P.Y, &raw mut P.Y, &raw mut zi2)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut P.Y, &raw mut P.Y, &raw mut p)

        mpi_lset(&raw mut P.Z, 1)

        return 0
    }

    // ─── ECDH Key Generation ─────────────────────────────────────────────

    // Generate ECDH key pair (private key + public key)
    // priv: output for private key (big-endian, 32 bytes)
    // pub: output for public key (65 bytes: 0x04 || X || Y)
    public func ecdh_generate_keypair(ctx : *mut ECDHContext,
                                       priv : *mut u8, priv_len : size_t,
                                       pub : *mut u8, pub_len : size_t) : int {
        if(priv_len < 32 || pub_len < 65) { return ERR_ECP_BUFFER_TOO_SMALL }

        // Generate private key from CSPRNG
        var priv_mpi : Mpi; mpi_init(&raw mut priv_mpi)
        var rng_ret = random_fill(priv, 32)
        if(rng_ret < 0) { return rng_ret }

        // Import private key and reduce modulo group order
        var ret = mpi_read_binary(&raw mut priv_mpi, priv, 32)
        if(ret < 0) { return ret }
        var n : Mpi; ecp_curve_n(&raw mut n)
        ret = mpi_mod(&raw mut priv_mpi, &raw mut priv_mpi, &raw mut n)
        if(ret < 0) { return ret }
        ret = mpi_write_binary(&raw mut priv_mpi, priv, 32)
        if(ret < 0) { return ret }

        // Compute public key = private * G
        var G : ECPPoint; ecp_point_init(&raw mut G)
        // Set generator point directly from pre-computed P-256 Gx, Gy
        mpi_grow(&raw mut G.X, 8); G.X.n = 8
        mpi_grow(&raw mut G.Y, 8); G.Y.n = 8
        var j : size_t = 0
        while(j < 8) { G.X.p[j] = P256_GX[j]; j += 1 }
        j = 0
        while(j < 8) { G.Y.p[j] = P256_GY[j]; j += 1 }
        mpi_lset(&raw mut G.Z, 1)

        var pub_point : ECPPoint; ecp_point_init(&raw mut pub_point)

        ret = ecp_mul(&raw mut pub_point, &raw mut priv_mpi, &raw mut G)
        if(ret < 0) { return ret }

        // Normalize to affine
        ret = ecp_normalize_jac(&raw mut pub_point)
        if(ret < 0) { return ret }

        // Export public key in uncompressed format: 04 || X (32 bytes) || Y (32 bytes)
        pub[0] = 0x04
        ret = mpi_write_binary(&raw mut pub_point.X, &raw mut pub[1], 32)
        if(ret < 0) { return ret }
        ret = mpi_write_binary(&raw mut pub_point.Y, &raw mut pub[33], 32)
        if(ret < 0) { return ret }

        // Store in context
        mpi_copy(&raw mut ctx.priv_key, &raw mut priv_mpi)
        mpi_copy(&raw mut ctx.pub_key.X, &raw mut pub_point.X)
        mpi_copy(&raw mut ctx.pub_key.Y, &raw mut pub_point.Y)
        mpi_copy(&raw mut ctx.pub_key.Z, &raw mut pub_point.Z)
        ctx.is_init = true

        return 0
    }

    // ─── ECDH Shared Secret ─────────────────────────────────────────────

    // Compute ECDH shared secret: shared = private * peer_public
    // peer_pub: peer's public key (65 bytes: 0x04 || X || Y)
    // shared: output for shared secret (32 bytes, X coordinate)
    public func ecdh_compute_shared(ctx : *mut ECDHContext,
                                     peer_pub : *u8, peer_pub_len : size_t,
                                     shared : *mut u8, shared_len : size_t) : int {
        if(!ctx.is_init) { return ERR_ECP_INVALID_KEY }
        if(peer_pub_len < 65 || peer_pub[0] != 0x04) {
            return ERR_ECP_BAD_INPUT_DATA
        }
        if(shared_len < 32) { return ERR_ECP_BUFFER_TOO_SMALL }

        // Import peer's public key
        var peer_point : ECPPoint; ecp_point_init(&raw mut peer_point)
        var ret = mpi_read_binary(&raw mut peer_point.X, &raw peer_pub[1], 32)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut peer_point.Y, &raw peer_pub[33], 32)
        if(ret < 0) { return ret }
        mpi_lset(&raw mut peer_point.Z, 1)

        // Basic sanity checks: reject point-at-infinity and coordinates >= p
        if(peer_point.X.n == 0 && peer_point.Y.n == 0) { return ERR_ECP_INVALID_KEY }
        var p : Mpi; ecp_curve_p(&raw mut p)
        if(mpi_cmp(&raw mut peer_point.X, &raw mut p) >= 0) { return ERR_ECP_INVALID_KEY }
        if(mpi_cmp(&raw mut peer_point.Y, &raw mut p) >= 0) { return ERR_ECP_INVALID_KEY }

        // Compute shared = private * peer_point
        var shared_point : ECPPoint; ecp_point_init(&raw mut shared_point)
        ret = ecp_mul(&raw mut shared_point, &raw mut ctx.priv_key, &raw mut peer_point)
        if(ret < 0) { return ret }

        // Normalize and extract X coordinate as shared secret
        ret = ecp_normalize_jac(&raw mut shared_point)
        if(ret < 0) { return ret }

        ret = mpi_write_binary(&raw mut shared_point.X, shared, 32)
        if(ret < 0) { return ret }

        return 0
    }

} // namespace tls
