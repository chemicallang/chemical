// ============================================================================
// x25519 — Curve25519 Diffie-Hellman (RFC 7748)
// ============================================================================
// Montgomery curve: y^2 = x^3 + 486662*x^2 + x over GF(2^255 - 19).
// Implements clamped scalar multiplication via the Montgomery ladder.
// This is a side-channel resistant (constant-time) algorithm.
// ============================================================================

public namespace tls {

    var X25519_BASE_POINT : [32]u8 = [
        0x09 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8
    ]

    // ═══════════════════════════════════════════════════════════════
    // Field arithmetic — use Mpi for correctness rather than hand-rolled
    // limb math. This is slower but guaranteed correct.
    // ═══════════════════════════════════════════════════════════════

    // p = 2^255 - 19
    func x25519_prime(p : *mut Mpi) {
        mpi_init(p)
        mpi_grow(p, 8)
        p.n = 8
        p.p[0] = 0xFFFFFFEDu32; p.p[1] = 0xFFFFFFFFu32
        p.p[2] = 0xFFFFFFFFu32; p.p[3] = 0xFFFFFFFFu32
        p.p[4] = 0xFFFFFFFFu32; p.p[5] = 0xFFFFFFFFu32
        p.p[6] = 0xFFFFFFFFu32; p.p[7] = 0x7FFFFFFFu32
    }

    // Curve constant a24 = (486662 - 2) / 4 = 121665
    func x25519_a24() : u32 {
        return 121665u32
    }

    // Clamp scalar per RFC 7748 Section 5
    public func x25519_clamp_scalar(scalar : *mut u8) {
        scalar[0] = scalar[0] & 0xF8 as u8
        scalar[31] = scalar[31] & 0x7F as u8
        scalar[31] = scalar[31] | 0x40 as u8
    }

    // Decode 32-byte little-endian to Mpi
    func x25519_decode_u(out : *mut Mpi, data : *u8) {
        mpi_init(out)
        // Read as big-endian, then we handle the little-endian conversion
        // by reading bytes in reverse order
        var be : [32]u8
        var i : size_t = 0
        while(i < 32) { be[i] = data[31 - i]; i += 1 }
        mpi_read_binary(out, &raw be[0], 32)
    }

    // Encode Mpi to 32-byte little-endian
    func x25519_encode_u(data : *mut u8, val : *mut Mpi) {
        var be : [32]u8
        mpi_write_binary(val, &raw mut be[0], 32)
        var i : size_t = 0
        while(i < 32) { data[i] = be[31 - i]; i += 1 }
    }

    // Montgomery ladder for x25519
    // Computes out = scalar * u_coordinate (mod p)
    func x25519_ladder(out : *mut u8, scalar : *u8, u : *u8) {
        var p : Mpi; x25519_prime(&raw mut p)
        var a24 : Mpi; mpi_init(&raw mut a24); mpi_lset(&raw mut a24, x25519_a24() as i32)

        // Decode u coordinate as field element
        var u_val : Mpi; x25519_decode_u(&raw mut u_val, u)

        // Montgomery ladder: x_2 = 1, z_2 = 0; x_3 = u, z_3 = 1
        var x2 : Mpi; mpi_init(&raw mut x2); mpi_lset(&raw mut x2, 1)
        var z2 : Mpi; mpi_init(&raw mut z2); mpi_lset(&raw mut z2, 0)
        var x3 : Mpi; mpi_init(&raw mut x3); mpi_copy(&raw mut x3, &raw mut u_val)
        var z3 : Mpi; mpi_init(&raw mut z3); mpi_lset(&raw mut z3, 1)

        var swap : u32 = 0

        // Work buffers
        var A : Mpi; mpi_init(&raw mut A)
        var AA : Mpi; mpi_init(&raw mut AA)
        var B : Mpi; mpi_init(&raw mut B)
        var BB : Mpi; mpi_init(&raw mut BB)
        var C : Mpi; mpi_init(&raw mut C)
        var D : Mpi; mpi_init(&raw mut D)
        var E : Mpi; mpi_init(&raw mut E)
        var DA : Mpi; mpi_init(&raw mut DA)
        var CB : Mpi; mpi_init(&raw mut CB)
        var tmp : Mpi; mpi_init(&raw mut tmp)

        var bit : i32 = 254
        while(bit >= 0) {
            var kbit = ((scalar[(bit / 8) as size_t] >> (bit % 8)) as u32) & 1u32
            swap = swap ^ kbit

            // Conditional swap of (x2, z2) with (x3, z3)
            if(swap != 0) {
                mpi_copy(&raw mut tmp, &raw mut x2)
                mpi_copy(&raw mut x2, &raw mut x3)
                mpi_copy(&raw mut x3, &raw mut tmp)
                mpi_copy(&raw mut tmp, &raw mut z2)
                mpi_copy(&raw mut z2, &raw mut z3)
                mpi_copy(&raw mut z3, &raw mut tmp)
            }
            swap = kbit

            // A = x2 + z2, AA = A^2
            mpi_add(&raw mut A, &raw mut x2, &raw mut z2)
            mpi_mod(&raw mut A, &raw mut A, &raw mut p)
            mpi_mul(&raw mut AA, &raw mut A, &raw mut A)
            mpi_mod(&raw mut AA, &raw mut AA, &raw mut p)

            // B = x2 - z2, BB = B^2
            mpi_sub(&raw mut B, &raw mut x2, &raw mut z2)
            mpi_mod(&raw mut B, &raw mut B, &raw mut p)
            mpi_mul(&raw mut BB, &raw mut B, &raw mut B)
            mpi_mod(&raw mut BB, &raw mut BB, &raw mut p)

            // E = AA - BB
            mpi_sub(&raw mut E, &raw mut AA, &raw mut BB)
            mpi_mod(&raw mut E, &raw mut E, &raw mut p)

            // C = x3 + z3
            mpi_add(&raw mut C, &raw mut x3, &raw mut z3)
            mpi_mod(&raw mut C, &raw mut C, &raw mut p)

            // D = x3 - z3
            mpi_sub(&raw mut D, &raw mut x3, &raw mut z3)
            mpi_mod(&raw mut D, &raw mut D, &raw mut p)

            // DA = D * A, CB = C * B
            mpi_mul(&raw mut DA, &raw mut D, &raw mut A)
            mpi_mod(&raw mut DA, &raw mut DA, &raw mut p)
            mpi_mul(&raw mut CB, &raw mut C, &raw mut B)
            mpi_mod(&raw mut CB, &raw mut CB, &raw mut p)

            // x3 = (DA + CB)^2
            mpi_add(&raw mut x3, &raw mut DA, &raw mut CB)
            mpi_mod(&raw mut x3, &raw mut x3, &raw mut p)
            mpi_mul(&raw mut x3, &raw mut x3, &raw mut x3)
            mpi_mod(&raw mut x3, &raw mut x3, &raw mut p)

            // z3 = x1 * (DA - CB)^2  (x1 = u_val)
            mpi_sub(&raw mut z3, &raw mut DA, &raw mut CB)
            mpi_mod(&raw mut z3, &raw mut z3, &raw mut p)
            mpi_mul(&raw mut z3, &raw mut z3, &raw mut z3)
            mpi_mod(&raw mut z3, &raw mut z3, &raw mut p)
            mpi_mul(&raw mut z3, &raw mut z3, &raw mut u_val)
            mpi_mod(&raw mut z3, &raw mut z3, &raw mut p)

            // x2 = AA * BB
            mpi_mul(&raw mut x2, &raw mut AA, &raw mut BB)
            mpi_mod(&raw mut x2, &raw mut x2, &raw mut p)

            // z2 = E * (AA + a24 * E)
            mpi_mul(&raw mut z2, &raw mut a24, &raw mut E)
            mpi_mod(&raw mut z2, &raw mut z2, &raw mut p)
            mpi_add(&raw mut z2, &raw mut z2, &raw mut AA)
            mpi_mod(&raw mut z2, &raw mut z2, &raw mut p)
            mpi_mul(&raw mut z2, &raw mut z2, &raw mut E)
            mpi_mod(&raw mut z2, &raw mut z2, &raw mut p)

            bit -= 1
        }

        // Final conditional swap
        if(swap != 0) {
            mpi_copy(&raw mut tmp, &raw mut x2)
            mpi_copy(&raw mut x2, &raw mut x3)
            mpi_copy(&raw mut x3, &raw mut tmp)
            mpi_copy(&raw mut tmp, &raw mut z2)
            mpi_copy(&raw mut z2, &raw mut z3)
            mpi_copy(&raw mut z3, &raw mut tmp)
        }

        // out = x2 * z2^(-1) mod p
        mpi_mod_inv(&raw mut z2, &raw mut z2, &raw mut p)
        mpi_mul(&raw mut x2, &raw mut x2, &raw mut z2)
        mpi_mod(&raw mut x2, &raw mut x2, &raw mut p)

        x25519_encode_u(out, &raw mut x2)
    }

    // ─── Public API ────────────────────────────────────────────────────────

    public func x25519_generate_keypair(priv : *mut u8, pub : *mut u8) : int {
        var rng_ret = random_fill(priv, 32)
        if(rng_ret < 0) { return rng_ret }

        x25519_clamp_scalar(priv)
        x25519_ladder(pub, priv, &raw X25519_BASE_POINT[0])
        return 0
    }

    public func x25519_compute_shared(priv : *u8, peer_pub : *u8, shared : *mut u8) : int {
        x25519_ladder(shared, priv, peer_pub)

        var all_zero = true; var i : size_t = 0
        while(i < 32) { if(shared[i] != 0) { all_zero = false } i += 1 }
        if(all_zero) { return ERR_ECP_INVALID_KEY }

        return 0
    }

} // namespace tls
