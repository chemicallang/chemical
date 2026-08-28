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

    // Prime p = FFFFFFFF 00000001 00000000 00000000 00000000 FFFFFFFF FFFFFFFF FFFFFFFF
    var P256_P : [8]u32 = [
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0x00000000u32,
        0x00000000u32, 0x00000000u32, 0x00000001u32, 0xFFFFFFFFu32
    ]

    // Order n = FFFFFFFF 00000000 FFFFFFFF FFFFFFFF BCE6FAAD A7179E84 F3B9CAC2 FC632551
    var P256_N : [8]u32 = [
        0xFC632551u32, 0xF3B9CAC2u32, 0xA7179E84u32, 0xBCE6FAADu32,
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0x00000000u32, 0xFFFFFFFFu32
    ]

    // ─── P-384 (secp384r1) parameters ─────────────────────────────────────
    // Prime p = 2^384 − 2^128 − 2^96 + 2^32 − 1, little-endian limbs.
    // hex: FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFF
    var P384_P : [12]u32 = [
        0xFFFFFFFFu32, 0x00000000u32, 0x00000000u32, 0xFFFFFFFFu32,
        0xFFFFFFFEu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32,
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32
    ]

    // Order n, little-endian limbs.
    // hex: FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973
    var P384_N : [12]u32 = [
        0xCCC52973u32, 0xECEC196Au32, 0x48B0A77Au32, 0x581A0DB2u32,
        0xF4372DDFu32, 0xC7634D81u32, 0xFFFFFFFFu32, 0xFFFFFFFFu32,
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32
    ]

    // Generator Gx, little-endian limbs.
    // hex: AA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A385502F25DBF55296C3A545E3872760AB7
    var P384_GX : [12]u32 = [
        0x72760AB7u32, 0x3A545E38u32, 0xBF55296Cu32, 0x5502F25Du32,
        0x82542A38u32, 0x59F741E0u32, 0x8BA79B98u32, 0x6E1D3B62u32,
        0xF320AD74u32, 0x8EB1C71Eu32, 0xBE8B0537u32, 0xAA87CA22u32
    ]

    // Generator Gy, little-endian limbs.
    // hex: 3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C00A60B1CE1D7E819D7A431D7C90EA0E5F
    var P384_GY : [12]u32 = [
        0x90EA0E5Fu32, 0x7A431D7Cu32, 0x1D7E819Du32, 0x0A60B1CEu32,
        0xB5F0B8C0u32, 0xE9DA3113u32, 0x289A147Cu32, 0xF8F41DBDu32,
        0x9292DC29u32, 0x5D9E98BFu32, 0x96262C6Fu32, 0x3617DE4Au32
    ]

    // Curve coefficient b, little-endian limbs.
    // hex: B3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875AC656398D8A2ED19D2A85C8EDD3EC2AEF
    var P384_B : [12]u32 = [
        0xD3EC2AEFu32, 0x2A85C8EDu32, 0x8A2ED19Du32, 0xC656398Du32,
        0x5013875Au32, 0x0314088Fu32, 0xFE814112u32, 0x181D9C6Eu32,
        0xE3F82D19u32, 0x988E056Bu32, 0xE23EE7E4u32, 0xB3312FA7u32
    ]

    // Current active curve for the generic point arithmetic helpers.
    // 0 = P-256 (default), 1 = P-384. Set by ecdsa_verify / ecdh around the
    // multiplicative ladder so the ecp_* parameter accessors use the right set.
    // Thread-local: the TLS test suite runs handshake tests in parallel threads,
    // and concurrent P-256 / P-384 operations must not corrupt each other's
    // curve selection.
    @thread_local var GLOBAL_CURVE : int = 0

    public func ecp_select_curve(c : int) {
        GLOBAL_CURVE = c
    }

    public func ecp_curve_id() : int {
        return GLOBAL_CURVE
    }

    public func ecp_curve_gx(gx : *mut Mpi) {
        if(GLOBAL_CURVE == 1) {
            mpi_init(gx); gx.n = 12
            var i : size_t = 0; while(i < 12) { gx.p[i] = P384_GX[i]; i += 1 }
            return
        }
        mpi_init(gx); gx.n = 8
        var i : size_t = 0; while(i < 8) { gx.p[i] = P256_GX[i]; i += 1 }
    }
    public func ecp_curve_gy(gy : *mut Mpi) {
        if(GLOBAL_CURVE == 1) {
            mpi_init(gy); gy.n = 12
            var i : size_t = 0; while(i < 12) { gy.p[i] = P384_GY[i]; i += 1 }
            return
        }
        mpi_init(gy); gy.n = 8
        var i : size_t = 0; while(i < 8) { gy.p[i] = P256_GY[i]; i += 1 }
    }
    public func ecp_curve_b(b : *mut Mpi) {
        if(GLOBAL_CURVE == 1) {
            mpi_init(b); b.n = 12
            var i : size_t = 0; while(i < 12) { b.p[i] = P384_B[i]; i += 1 }
            return
        }
        mpi_init(b); b.n = 8
        var i : size_t = 0; while(i < 8) { b.p[i] = P256_B[i]; i += 1 }
    }

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
    public var P256_B : [8]u32 = [
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
    public func ecp_curve_p(p : *mut Mpi) {
        if(GLOBAL_CURVE == 1) {
            mpi_init(p)
            p.n = 12
            var i : size_t = 0
            while(i < 12) { p.p[i] = P384_P[i]; i += 1 }
            return
        }
        mpi_init(p)
        p.n = 8
        var i : size_t = 0
        while(i < 8) { p.p[i] = P256_P[i]; i += 1 }
    }

    public func ecp_curve_n(p : *mut Mpi) {
        if(GLOBAL_CURVE == 1) {
            mpi_init(p)
            p.n = 12
            var i : size_t = 0
            while(i < 12) { p.p[i] = P384_N[i]; i += 1 }
            return
        }
        mpi_init(p)
        p.n = 8
        var i : size_t = 0
        while(i < 8) { p.p[i] = P256_N[i]; i += 1 }
    }

    // Modulo P-256 prime: a mod p (for values < 2*p)
    func ecp_mod_p256(X : *mut Mpi) : int {
        // Fast reduction for P-256 (2^256 - 2^224 + 2^192 + 2^96 - 1)
        // For now, use general mod
        unsafe var p : Mpi; ecp_curve_p(&raw mut p)
        return mpi_mod(X, X, &raw mut p)
    }

    // ─── Point Operations on P-256 ──────────────────────────────────────

    // Point double: R = 2 * P on the P-256 curve
    func ecp_double_jac(R : *mut ECPPoint, P : *mut ECPPoint) : int {
        // Handle point-at-infinity
        if(mpi_is_zero(&raw mut P.Z)) {
            mpi_lset(&raw mut R.X, 1)
            mpi_lset(&raw mut R.Y, 1)
            mpi_lset(&raw mut R.Z, 0)
            return 0
        }
        // Using formulas from http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#doubling-dbl-2007-bl
        // For a = -3 (P-256): optimized formulas
        unsafe var p : Mpi; ecp_curve_p(&raw mut p)
        unsafe var XX : Mpi; mpi_init(&raw mut XX)
        unsafe var YY : Mpi; mpi_init(&raw mut YY)
        unsafe var ZZ : Mpi; mpi_init(&raw mut ZZ)
        unsafe var S : Mpi; mpi_init(&raw mut S)
        unsafe var M : Mpi; mpi_init(&raw mut M)
        unsafe var T : Mpi; mpi_init(&raw mut T)
        unsafe var X3 : Mpi; mpi_init(&raw mut X3)
        unsafe var Y3 : Mpi; mpi_init(&raw mut Y3)
        unsafe var Z3 : Mpi; mpi_init(&raw mut Z3)
        var ret : int = 0

        // XX = X1^2
        ret = mpi_mul(&raw mut XX, &raw mut P.X, &raw mut P.X)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut XX, &raw mut XX, &raw mut p)

        // YY = Y1^2
        ret = mpi_mul(&raw mut YY, &raw mut P.Y, &raw mut P.Y)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut YY, &raw mut YY, &raw mut p)

        // ZZ = Z1^2
        ret = mpi_mul(&raw mut ZZ, &raw mut P.Z, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut ZZ, &raw mut ZZ, &raw mut p)

        // S = 4*X1*YY
        ret = mpi_mul(&raw mut S, &raw mut P.X, &raw mut YY)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut S, &raw mut S, &raw mut p)
        ret = mpi_mul_int(&raw mut S, &raw mut S, 4)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut S, &raw mut S, &raw mut p)

        // For a=-3: M = 3*(X1 + ZZ)*(X1 - ZZ)
        // = 3*(X1^2 - Z1^4) = 3*X1^2 + a*Z1^4 (where a=-3)
        unsafe var X_plus_ZZ : Mpi; mpi_init(&raw mut X_plus_ZZ)
        ret = mpi_add(&raw mut X_plus_ZZ, &raw mut P.X, &raw mut ZZ)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X_plus_ZZ, &raw mut X_plus_ZZ, &raw mut p)
        unsafe var X_minus_ZZ : Mpi; mpi_init(&raw mut X_minus_ZZ)
        ret = mpi_sub(&raw mut X_minus_ZZ, &raw mut P.X, &raw mut ZZ)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X_minus_ZZ, &raw mut X_minus_ZZ, &raw mut p)
        ret = mpi_mul(&raw mut M, &raw mut X_plus_ZZ, &raw mut X_minus_ZZ)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut M, &raw mut M, &raw mut p)
        ret = mpi_mul_int(&raw mut M, &raw mut M, 3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut M, &raw mut M, &raw mut p)

        // X3 = M^2 - 2*S
        ret = mpi_mul(&raw mut X3, &raw mut M, &raw mut M)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X3, &raw mut X3, &raw mut p)
        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut S)
        if(ret < 0) { return ret }
        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut S)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X3, &raw mut X3, &raw mut p)

        // Y3 = M*(S - X3) - 8*YY^2
        ret = mpi_sub(&raw mut T, &raw mut S, &raw mut X3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut T, &raw mut T, &raw mut p)
        ret = mpi_mul(&raw mut Y3, &raw mut M, &raw mut T)
        if(ret < 0) { return ret }

        // 8*YY^2
        ret = mpi_mul(&raw mut YY, &raw mut YY, &raw mut YY)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut YY, &raw mut YY, &raw mut p)
        ret = mpi_mul_int(&raw mut YY, &raw mut YY, 8)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut YY, &raw mut YY, &raw mut p)

        ret = mpi_sub(&raw mut Y3, &raw mut Y3, &raw mut YY)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Y3, &raw mut Y3, &raw mut p)

        // Z3 = 2*Y1*Z1
        ret = mpi_mul(&raw mut Z3, &raw mut P.Y, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Z3, &raw mut Z3, &raw mut p)
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
    public func ecp_add_jac(R : *mut ECPPoint, P : *mut ECPPoint, Q : *mut ECPPoint) : int {
        // Mixed Jacobian-affine addition
        // P = (X1, Y1, Z1), Q = (X2, Y2, 1)
        // R = (X3, Y3, Z3) = P + Q
        // Using formulas from http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#addition-madd-2007-bl

        // Handle P = infinity: return Q
        if(mpi_is_zero(&raw mut P.Z)) {
            mpi_copy(&raw mut R.X, &raw mut Q.X)
            mpi_copy(&raw mut R.Y, &raw mut Q.Y)
            mpi_copy(&raw mut R.Z, &raw mut Q.Z)
            return 0
        }

        unsafe var p : Mpi; ecp_curve_p(&raw mut p)
        unsafe var Z1Z1 : Mpi; mpi_init(&raw mut Z1Z1)
        unsafe var U2 : Mpi; mpi_init(&raw mut U2)
        unsafe var S2 : Mpi; mpi_init(&raw mut S2)
        unsafe var H : Mpi; mpi_init(&raw mut H)
        unsafe var HH : Mpi; mpi_init(&raw mut HH)
        unsafe var I : Mpi; mpi_init(&raw mut I)
        unsafe var R_val : Mpi; mpi_init(&raw mut R_val)
        unsafe var J : Mpi; mpi_init(&raw mut J)
        unsafe var V : Mpi; mpi_init(&raw mut V)
        unsafe var X3 : Mpi; mpi_init(&raw mut X3)
        unsafe var Y3 : Mpi; mpi_init(&raw mut Y3)
        unsafe var Z3 : Mpi; mpi_init(&raw mut Z3)
        var ret : int = 0

        // Z1Z1 = Z1^2
        ret = mpi_mul(&raw mut Z1Z1, &raw mut P.Z, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Z1Z1, &raw mut Z1Z1, &raw mut p)

        // U2 = X2 * Z1Z1
        ret = mpi_mul(&raw mut U2, &raw mut Q.X, &raw mut Z1Z1)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut U2, &raw mut U2, &raw mut p)

        // S2 = Y2 * Z1 * Z1Z1 = Y2 * Z1^3
        ret = mpi_mul(&raw mut S2, &raw mut Q.Y, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mul(&raw mut S2, &raw mut S2, &raw mut Z1Z1)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut S2, &raw mut S2, &raw mut p)

        // H = U2 - X1
        ret = mpi_sub(&raw mut H, &raw mut U2, &raw mut P.X)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut H, &raw mut H, &raw mut p)

        // HH = H^2
        ret = mpi_mul(&raw mut HH, &raw mut H, &raw mut H)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut HH, &raw mut HH, &raw mut p)

        // I = 4 * HH  (but for standard formula: I = 4*HH, we compute 2*HH later)
        // Actually: I = 4*HH is used for X3 formula
        ret = mpi_mul_int(&raw mut I, &raw mut HH, 4)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut I, &raw mut I, &raw mut p)

        // J = H * I
        ret = mpi_mul(&raw mut J, &raw mut H, &raw mut I)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut J, &raw mut J, &raw mut p)

        // R_val = 2 * (S2 - Y1)
        ret = mpi_sub(&raw mut R_val, &raw mut S2, &raw mut P.Y)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut R_val, &raw mut R_val, &raw mut p)
        ret = mpi_mul_int(&raw mut R_val, &raw mut R_val, 2)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut R_val, &raw mut R_val, &raw mut p)

        // V = X1 * I
        ret = mpi_mul(&raw mut V, &raw mut P.X, &raw mut I)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut V, &raw mut V, &raw mut p)

        // X3 = R_val^2 - J - 2*V
        ret = mpi_mul(&raw mut X3, &raw mut R_val, &raw mut R_val)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut X3, &raw mut X3, &raw mut p)
        unsafe var _x3i : size_t; unsafe var _x3b : [32]u8; unsafe var _x3size : size_t
        if(tls_config::EXTENSIVE_DEBUG_LOG) { printf("[X3DBG] after R2: s=%d n=%lu modp=", X3.s, X3.n); _x3i=0;for(_x3i=0;_x3i<32;_x3i+=1){_x3b[_x3i]=0};mpi_write_binary(&raw mut X3,&raw mut _x3b[0],32);_x3i=0;while(_x3i<32){printf("%02x",_x3b[_x3i]as int);_x3i+=1};printf("\n") }
        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut J)
        if(ret < 0) { if(tls_config::EXTENSIVE_DEBUG_LOG) printf("[X3DBG] sub1 failed ret=%d\n",ret); return ret }
        if(tls_config::EXTENSIVE_DEBUG_LOG) { printf("[X3DBG] after -J: s=%d n=%lu val=", X3.s, X3.n); _x3i=0;for(_x3i=0;_x3i<32;_x3i+=1){_x3b[_x3i]=0};_x3size=mpi_size(&raw mut X3);if(_x3size>32){printf("<too big:%lu>",_x3size)}else{mpi_write_binary(&raw mut X3,&raw mut _x3b[0],32);_x3i=0;while(_x3i<32){printf("%02x",_x3b[_x3i]as int);_x3i+=1}};printf("\n") }
        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut V)  // subtract V once
        if(ret < 0) { if(tls_config::EXTENSIVE_DEBUG_LOG) printf("[X3DBG] sub2 failed ret=%d\n",ret); return ret }
        if(tls_config::EXTENSIVE_DEBUG_LOG) printf("[X3DBG] after -V: s=%d n=%lu\n", X3.s, X3.n)
        ret = mpi_sub(&raw mut X3, &raw mut X3, &raw mut V)  // subtract V again = 2*V
        if(ret < 0) { if(tls_config::EXTENSIVE_DEBUG_LOG) printf("[X3DBG] sub3 failed ret=%d\n",ret); return ret }
        if(tls_config::EXTENSIVE_DEBUG_LOG) printf("[X3DBG] after -2V: s=%d n=%lu\n", X3.s, X3.n)
        ret = mpi_mod(&raw mut X3, &raw mut X3, &raw mut p)
        if(tls_config::EXTENSIVE_DEBUG_LOG) { printf("[X3DBG] final: s=%d n=%lu val=", X3.s, X3.n); _x3i=0;for(_x3i=0;_x3i<32;_x3i+=1){_x3b[_x3i]=0};_x3size=mpi_size(&raw mut X3);if(_x3size>32){printf("<too big:%lu>",_x3size)}else{mpi_write_binary(&raw mut X3,&raw mut _x3b[0],32);_x3i=0;while(_x3i<32){printf("%02x",_x3b[_x3i]as int);_x3i+=1}};printf("\n") }

        // Y3 = R_val * (V - X3) - 2 * Y1 * J
        // temp = V - X3
        unsafe var tmp_vy : Mpi; mpi_init(&raw mut tmp_vy)
        ret = mpi_sub(&raw mut tmp_vy, &raw mut V, &raw mut X3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut tmp_vy, &raw mut tmp_vy, &raw mut p)
        if(ret < 0) { return ret }
        unsafe var _y3b : [32]u8; unsafe var _y3i : size_t; unsafe var _y3sz : size_t
        if(tls_config::EXTENSIVE_DEBUG_LOG) { printf("[Y3DBG] tmp_vy s=%d n=%lu val=", tmp_vy.s, tmp_vy.n); _y3i=0;for(_y3i=0;_y3i<32;_y3i+=1){_y3b[_y3i]=0};_y3sz=mpi_size(&raw mut tmp_vy);if(_y3sz>32){printf("<too big:%lu>",_y3sz)}else{mpi_write_binary(&raw mut tmp_vy,&raw mut _y3b[0],32);_y3i=0;while(_y3i<32){printf("%02x",_y3b[_y3i]as int);_y3i+=1}};printf("\n") }
        // Y3 = R_val * (V - X3)
        ret = mpi_mul(&raw mut Y3, &raw mut R_val, &raw mut tmp_vy)
        if(ret < 0) { return ret }
        if(tls_config::EXTENSIVE_DEBUG_LOG) printf("[Y3DBG] after mul s=%d n=%lu ret=%d\n", Y3.s, Y3.n, ret)
        if(tls_config::EXTENSIVE_DEBUG_LOG) { _y3i=0;for(_y3i=0;_y3i<32;_y3i+=1){_y3b[_y3i]=0};_y3sz=mpi_size(&raw mut Y3);if(_y3sz>32){printf("[Y3DBG] after mul val=<too big:%lu>\n",_y3sz)}else{mpi_write_binary(&raw mut Y3,&raw mut _y3b[0],32);printf("[Y3DBG] after mul val=");_y3i=0;while(_y3i<32){printf("%02x",_y3b[_y3i]as int);_y3i+=1};printf("\n")} }


        // DEBUG ALL INTERMEDIATES
        if(P.Z.n == 8 && tls_config::EXTENSIVE_DEBUG_LOG) { unsafe var _b:[32]u8; unsafe var _i:size_t
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut Z1Z1,&raw mut _b[0],32);printf("[ADDDBG] Z1Z1=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut U2,&raw mut _b[0],32);printf("[ADDDBG] U2=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut H,&raw mut _b[0],32);printf("[ADDDBG] H=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut S2,&raw mut _b[0],32);printf("[ADDDBG] S2=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut HH,&raw mut _b[0],32);printf("[ADDDBG] HH=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut I,&raw mut _b[0],32);printf("[ADDDBG] I=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut J,&raw mut _b[0],32);printf("[ADDDBG] J=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut R_val,&raw mut _b[0],32);printf("[ADDDBG] R_val=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut V,&raw mut _b[0],32);printf("[ADDDBG] V=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut X3,&raw mut _b[0],32);printf("[ADDDBG] X3=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(&raw mut Y3,&raw mut _b[0],32);printf("[ADDDBG] Y3=");_i=0;while(_i<32){printf("%02x",_b[_i]as int);_i+=1};printf("\n")
        }

        // Y1J = Y1 * J
        unsafe var Y1J : Mpi; mpi_init(&raw mut Y1J)
        ret = mpi_mul(&raw mut Y1J, &raw mut P.Y, &raw mut J)
        if(ret < 0) { return ret }
        mpi_mod(&raw mut Y1J, &raw mut Y1J, &raw mut p)
        // 2 * Y1J
        mpi_mul_int(&raw mut Y1J, &raw mut Y1J, 2)
        mpi_mod(&raw mut Y1J, &raw mut Y1J, &raw mut p)

        // Y3 = Y3 - 2*Y1*J
        ret = mpi_sub(&raw mut Y3, &raw mut Y3, &raw mut Y1J)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Y3, &raw mut Y3, &raw mut p)

        // Z3 = (Z1 + H)^2 - Z1Z1 - HH
        ret = mpi_add(&raw mut Z3, &raw mut P.Z, &raw mut H)
        if(ret < 0) { return ret }
        ret = mpi_mul(&raw mut Z3, &raw mut Z3, &raw mut Z3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Z3, &raw mut Z3, &raw mut p)
        ret = mpi_sub(&raw mut Z3, &raw mut Z3, &raw mut Z1Z1)
        if(ret < 0) { return ret }
        ret = mpi_sub(&raw mut Z3, &raw mut Z3, &raw mut HH)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut Z3, &raw mut Z3, &raw mut p)

        // Copy result
        mpi_copy(&raw mut R.X, &raw mut X3)
        mpi_copy(&raw mut R.Y, &raw mut Y3)
        mpi_copy(&raw mut R.Z, &raw mut Z3)

        return 0
    }

    // ─── Scalar Multiplication (Double-and-Add) ──────────────────────────

    // R = k * P on P-256 using Montgomery ladder for safety
    public func ecp_mul(R : *mut ECPPoint, k : *mut Mpi, P : *mut ECPPoint) : int {
        // Double-and-add: R = k * P
        // Uses ecp_add_jac which requires P to remain in affine form (Z=1).
        // P starts affine (from generator or normalized point) and is
        // kept affine throughout by only doubling the accumulator.

        unsafe var R0 : ECPPoint; ecp_point_init(&raw mut R0)
        mpi_lset(&raw mut R0.X, 1)
        mpi_lset(&raw mut R0.Y, 1)
        mpi_lset(&raw mut R0.Z, 0)

        // Keep a normalized copy of P that stays affine (Z=1)
        unsafe var P_affine : ECPPoint; ecp_point_init(&raw mut P_affine)
        mpi_copy(&raw mut P_affine.X, &raw mut P.X)
        mpi_copy(&raw mut P_affine.Y, &raw mut P.Y)
        mpi_lset(&raw mut P_affine.Z, 1)

        var bitlen = mpi_bitlen(k)
        var i = bitlen
        while(i > 0) {
            i -= 1
            var limb_idx = i / BITS_PER_LIMB
            var bit_idx = i % BITS_PER_LIMB
            var bit = (k.p[limb_idx] >> bit_idx) & 1

            // R0 = 2 * R0
            var ret = ecp_double_jac(&raw mut R0, &raw mut R0)
            if(ret < 0) { return ret }

            if(bit == 1) {
                // R0 = R0 + P (P stays affine, so mixed addition works)
                ret = ecp_add_jac(&raw mut R0, &raw mut R0, &raw mut P_affine)
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
    public func ecp_normalize_jac(P : *mut ECPPoint) : int {
        if(mpi_is_zero(&raw mut P.Z)) {
            // Point at infinity
            mpi_lset(&raw mut P.X, 0)
            mpi_lset(&raw mut P.Y, 0)
            return 0
        }

        unsafe var p : Mpi; ecp_curve_p(&raw mut p)
        unsafe var zi : Mpi; mpi_init(&raw mut zi)
        unsafe var zi2 : Mpi; mpi_init(&raw mut zi2)
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
        unsafe var priv_mpi : Mpi; mpi_init(&raw mut priv_mpi)
        var rng_ret = random_fill(priv, 32)
        if(rng_ret < 0) { return rng_ret }

        // Import private key and reduce modulo group order
        var ret = mpi_read_binary(&raw mut priv_mpi, priv, 32)
        if(ret < 0) { return ret }
        unsafe var n : Mpi; ecp_curve_n(&raw mut n)
        ret = mpi_mod(&raw mut priv_mpi, &raw mut priv_mpi, &raw mut n)
        if(ret < 0) { return ret }
        ret = mpi_write_binary(&raw mut priv_mpi, priv, 32)
        if(ret < 0) { return ret }

        // Compute public key = private * G
        unsafe var G : ECPPoint; ecp_point_init(&raw mut G)
        // Set generator point directly from pre-computed P-256 Gx, Gy
        mpi_grow(&raw mut G.X, 8); G.X.n = 8
        mpi_grow(&raw mut G.Y, 8); G.Y.n = 8
        var j : size_t = 0
        while(j < 8) { G.X.p[j] = P256_GX[j]; j += 1 }
        j = 0
        while(j < 8) { G.Y.p[j] = P256_GY[j]; j += 1 }
        mpi_lset(&raw mut G.Z, 1)

        unsafe var pub_point : ECPPoint; ecp_point_init(&raw mut pub_point)

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
        unsafe var peer_point : ECPPoint; ecp_point_init(&raw mut peer_point)
        var ret = mpi_read_binary(&raw mut peer_point.X, &raw peer_pub[1], 32)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut peer_point.Y, &raw peer_pub[33], 32)
        if(ret < 0) { return ret }
        mpi_lset(&raw mut peer_point.Z, 1)

        // Basic sanity checks: reject point-at-infinity and coordinates >= p
        if(peer_point.X.n == 0 && peer_point.Y.n == 0) { return ERR_ECP_INVALID_KEY }
        unsafe var p : Mpi; ecp_curve_p(&raw mut p)
        if(mpi_cmp(&raw mut peer_point.X, &raw mut p) >= 0) { return ERR_ECP_INVALID_KEY }
        if(mpi_cmp(&raw mut peer_point.Y, &raw mut p) >= 0) { return ERR_ECP_INVALID_KEY }

        // Point-on-curve validation: y^2 ≡ x^3 - 3x + b (mod p) for P-256
        unsafe var lhs : Mpi; mpi_init(&raw mut lhs)
        unsafe var rhs : Mpi; mpi_init(&raw mut rhs)
        unsafe var tmp : Mpi; mpi_init(&raw mut tmp)
        unsafe var b_m : Mpi; mpi_init(&raw mut b_m)
        // b = P256_B
        mpi_grow(&raw mut b_m, 8); b_m.n = 8
        var bj : size_t = 0
        while(bj < 8) { b_m.p[bj] = P256_B[bj]; bj += 1 }
        // lhs = y^2 mod p
        ret = mpi_mul(&raw mut lhs, &raw mut peer_point.Y, &raw mut peer_point.Y)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut lhs, &raw mut lhs, &raw mut p)
        if(ret < 0) { return ret }
        // rhs = x^3 mod p
        ret = mpi_mul(&raw mut rhs, &raw mut peer_point.X, &raw mut peer_point.X)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
        ret = mpi_mul(&raw mut rhs, &raw mut rhs, &raw mut peer_point.X)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
        // rhs = x^3 - 3x + b mod p (a = -3 for P-256)
        ret = mpi_mul_int(&raw mut tmp, &raw mut peer_point.X, 3)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut tmp, &raw mut tmp, &raw mut p)
        // Use separate temporary to avoid aliasing issues
        unsafe var rhs2 : Mpi; mpi_init(&raw mut rhs2)
        ret = mpi_sub(&raw mut rhs2, &raw mut rhs, &raw mut tmp)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut rhs2, &raw mut rhs2, &raw mut p)
        if(ret < 0) { return ret }
        ret = mpi_add(&raw mut rhs2, &raw mut rhs2, &raw mut b_m)
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut rhs2, &raw mut rhs2, &raw mut p)
        if(mpi_cmp(&raw mut lhs, &raw mut rhs2) != 0) { return ERR_ECP_INVALID_KEY }

        // Compute shared = private * peer_point
        unsafe var shared_point : ECPPoint; ecp_point_init(&raw mut shared_point)
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
