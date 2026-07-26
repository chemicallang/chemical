// ============================================================================
// x25519 — Curve25519 Diffie-Hellman (RFC 7748)
// ============================================================================
// Uses direct 32-bit limb arithmetic over GF(2^255 - 19).
// 8 limbs of 32 bits each, little-end internally.
// Multiplication splits 32-bit limbs into 16-bit halves for 64-bit safety.
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

    // ─── Field Element (8 x u32, little-endian limbs) ────────────────────
    // p = 2^255 - 19
    // p limbs: [0xFFFFFFED, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    //            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7FFFFFFF]

    // Set all limbs to 0
    public func fe_zero(f : *mut u32) {
        var i : size_t = 0
        while(i < 8) { f[i] = 0; i += 1 }
    }

    // Copy field element
    func fe_copy(dst : *mut u32, src : *u32) {
        var i : size_t = 0
        while(i < 8) { dst[i] = src[i]; i += 1 }
    }

    // Set to value (only small values work, up to 2^31)
    public func fe_set_small(f : *mut u32, val : u32) {
        fe_zero(f)
        f[0] = val
    }

    // ─── Decode 32-byte little-endian to 8 limbs ─────────────────────────
    public func fe_decode(f : *mut u32, data : *u8) {
        var i : size_t = 0
        while(i < 8) {
            var off = i * 4
            f[i] = (data[off] as u32) |
                   ((data[off + 1] as u32) << 8) |
                   ((data[off + 2] as u32) << 16) |
                   ((data[off + 3] as u32) << 24)
            i += 1
        }
        // Mask off bit 255 per RFC 7748 Section 5
        f[7] = f[7] & 0x7FFFFFFFu32
    }

    // ─── Encode 8 limbs to 32-byte little-endian ─────────────────────────
    public func fe_encode(data : *mut u8, f : *u32) {
        var i : size_t = 0
        while(i < 8) {
            var off = i * 4
            data[off] = (f[i] & 0xFF) as u8
            data[off + 1] = ((f[i] >> 8) & 0xFF) as u8
            data[off + 2] = ((f[i] >> 16) & 0xFF) as u8
            data[off + 3] = ((f[i] >> 24) & 0xFF) as u8
            i += 1
        }
    }

    // ─── Compare two field elements (return -1, 0, or 1) ─────────────────
    func fe_cmp(a : *u32, b : *u32) : int {
        var i : i32 = 7
        while(i >= 0) {
            if(a[i as size_t] > b[i as size_t]) { return 1 }
            if(a[i as size_t] < b[i as size_t]) { return -1 }
            i -= 1
        }
        return 0
    }

    // ─── Conditional copy (constant-time) ──────────────────────────────
    // If mask == 0xFFFFFFFF, copy src to dst. If mask == 0, don't.
    func fe_ccopy(dst : *mut u32, src : *u32, mask : u32) {
        var i : size_t = 0
        while(i < 8) {
            dst[i] = dst[i] ^ (mask & (dst[i] ^ src[i]))
            i += 1
        }
    }

    // ─── Carry propagation (full reduction) ─────────────────────────────
    // Takes a partially-reduced element and fully propagates carries.
    // Ensures each limb < 2^32.
    func fe_carry(f : *mut u64) {
        var c : u64 = 0
        var i : size_t = 0
        while(i < 8) {
            f[i] = f[i] + c
            c = f[i] >> 32
            f[i] = f[i] & 0xFFFFFFFFu64
            i += 1
        }
        // If there's a carry above limb 7, multiply by 19 and add to limb 0
        // since 2^256 ≡ 2 * 2^255 ≡ 2 * 19 = 38 mod p
        // Actually: 2^256 = 2 * 2^255 ≡ 2 * 19 = 38 (mod p)
        if(c > 0) {
            f[0] = f[0] + c * 38
            // Propagate any carry from this addition
            var c2 = f[0] >> 32
            f[0] = f[0] & 0xFFFFFFFFu64
            if(c2 > 0) {
                f[1] = f[1] + c2
                c2 = f[1] >> 32; f[1] = f[1] & 0xFFFFFFFFu64
                if(c2 > 0) {
                    f[2] = f[2] + c2; c2 = f[2] >> 32; f[2] = f[2] & 0xFFFFFFFFu64
                    if(c2 > 0) {
                        f[3] = f[3] + c2; c2 = f[3] >> 32; f[3] = f[3] & 0xFFFFFFFFu64
                        if(c2 > 0) {
                            f[4] = f[4] + c2; c2 = f[4] >> 32; f[4] = f[4] & 0xFFFFFFFFu64
                            if(c2 > 0) {
                                f[5] = f[5] + c2; c2 = f[5] >> 32; f[5] = f[5] & 0xFFFFFFFFu64
                                if(c2 > 0) {
                                    f[6] = f[6] + c2; c2 = f[6] >> 32; f[6] = f[6] & 0xFFFFFFFFu64
                                    if(c2 > 0) {
                                        f[7] = f[7] + c2
                                        // Final carry from limb 7 would wrap again via 38
                                        var c3 = f[7] >> 32
                                        f[7] = f[7] & 0xFFFFFFFFu64
                                        if(c3 > 0) {
                                            f[0] = f[0] + c3 * 38
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ─── Conditional subtraction of p ──────────────────────────────────
    // If f >= p, subtract p from f (in-place).
    func fe_strong_reduce(f : *mut u32) {
        // p limbs
        var p_limbs : [8]u32 = [
            0xFFFFFFEDu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32,
            0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0x7FFFFFFFu32
        ]
        // Check if f >= p
        var ge = true
        var i : i32 = 7
        while(i >= 0) {
            var idx = i as size_t
            if(f[idx] > p_limbs[idx]) { ge = true; break }
            if(f[idx] < p_limbs[idx]) { ge = false; break }
            i -= 1
        }
        if(ge) {
            // Subtract p from f
            var borrow : u64 = 0
            i = 0
            while(i < 8) {
                var diff = (f[i as size_t] as u64) - (p_limbs[i as size_t] as u64) - borrow
                f[i as size_t] = (diff & 0xFFFFFFFFu64) as u32
                borrow = (diff >> 63)
                i += 1
            }
        }
    }

    // ─── Addition mod p (fully reduced) ────────────────────────────────
    public func fe_add(c : *mut u32, a : *u32, b : *u32) {
        var t : [8]u64
        var i : size_t = 0
        while(i < 8) {
            t[i] = (a[i] as u64) + (b[i] as u64)
            i += 1
        }
        fe_carry(&raw mut t[0])
        // Convert to u32 and store in c
        var j : size_t = 0
        while(j < 8) {
            c[j] = t[j] as u32
            j += 1
        }
        fe_strong_reduce(c)
    }

    // Debug flag
    comptime const X25519_DEBUG = true

    func fe_debug_print(msg : *char, f : *u32) {
        if(!X25519_DEBUG) { return }
        var bytes : [32]u8
        fe_encode(&raw mut bytes[0], f)
        printf("[FEDBG] %s: ", msg)
        var i : size_t = 0
        while(i < 8) {
            printf("%08x ", f[i])
            i += 1
        }
        printf("|| ")
        i = 0
        while(i < 32) {
            printf("%02x", bytes[i] as int)
            i += 1
        }
        printf("\n")
    }

    // ─── Subtraction mod p (fully reduced) ─────────────────────────────
    // Computes c = a - b mod p = (a + (p - b)) mod p
    public func fe_sub(c : *mut u32, a : *u32, b : *u32) {
        // Step 1: compute p - b using borrow subtraction (b < p, so this is exact)
        var t : [8]u64
        var borrow : u64 = 0
        var i : size_t = 0
        while(i < 8) {
            var p_limb : u64 = 0xFFFFFFFFu64
            if(i == 0) { p_limb = 0xFFFFFFEDu64 }
            if(i == 7) { p_limb = 0x7FFFFFFFu64 }
            var diff = p_limb - (b[i] as u64) - borrow
            t[i] = diff & 0xFFFFFFFFu64
            borrow = diff >> 63
            i += 1
        }
        // After this, t = p - b (borrow should be 0 since b < p)
        
        // Step 2: add a to (p - b): result = a + (p - b)
        var carry : u64 = 0
        i = 0
        while(i < 8) {
            var sum = (a[i] as u64) + t[i] + carry
            t[i] = sum & 0xFFFFFFFFu64
            carry = sum >> 32
            i += 1
        }
        
        // Step 3: handle carry out of limb 7 (2^256 ≡ 38 mod p)
        if(carry > 0) {
            t[0] = t[0] + carry * 38
            var c2 = t[0] >> 32
            t[0] = t[0] & 0xFFFFFFFFu64
            i = 1
            while(i < 8 && c2 > 0) {
                t[i] = t[i] + c2
                c2 = t[i] >> 32
                t[i] = t[i] & 0xFFFFFFFFu64
                i += 1
            }
            // If still carry after limb 7, wrap again
            if(c2 > 0) {
                t[0] = t[0] + c2 * 38
            }
        }
        
        // Store as u32 and strong reduce
        var j : size_t = 0
        while(j < 8) {
            c[j] = t[j] as u32
            j += 1
        }
        fe_strong_reduce(c)
    }

    // ─── 32x32 → 64-bit multiplication ──────────────────────────────────
    // Direct multiplication (C/LLVM handles u64 promotion correctly).
    func mul32(r_lo : *mut u64, r_hi : *mut u64, x : u32, y : u32) {
        var prod = (x as u64) * (y as u64)
        *r_lo = prod & 0xFFFFFFFFu64
        *r_hi = prod >> 32
    }

    // ─── Multiplication mod p ──────────────────────────────────────────
    // Schoolbook multiplication of two 8-limb numbers, then reduce mod p.
    public func fe_mul(c : *mut u32, a : *u32, b : *u32) {
        // Compute 16 partial products (256-bit * 256-bit = 512-bit result)
        var t : [16]u64
        fe_zero_u64_arr(&raw mut t[0], 16)

        var i : size_t = 0
        while(i < 8) {
            var j : size_t = 0
            while(j < 8) {
                var lo : u64 = 0; var hi : u64 = 0
                mul32(&raw mut lo, &raw mut hi, a[i], b[j])
                t[i + j] = t[i + j] + lo
                t[i + j + 1] = t[i + j + 1] + hi
                j += 1
            }
            i += 1
        }

        // Propagate carries through the 16-limb result
        var carry : u64 = 0
        i = 0
        while(i < 16) {
            t[i] = t[i] + carry
            carry = t[i] >> 32
            t[i] = t[i] & 0xFFFFFFFFu64
            i += 1
        }

        // Reduce mod p: 2^256 ≡ 38 mod p (since 2^256 = 2 * 2^255 ≡ 2 * 19 = 38)
        var r0 = t[0] + t[8] * 38 + carry * 1444
        var r1 = t[1] + t[9] * 38
        var r2 = t[2] + t[10] * 38
        var r3 = t[3] + t[11] * 38
        var r4 = t[4] + t[12] * 38
        var r5 = t[5] + t[13] * 38
        var r6 = t[6] + t[14] * 38
        var r7 = t[7] + t[15] * 38

        // Propagate carries
        var c0 = r0 >> 32; r0 = r0 & 0xFFFFFFFFu64
        r1 = r1 + c0; var c1 = r1 >> 32; r1 = r1 & 0xFFFFFFFFu64
        r2 = r2 + c1; var c2 = r2 >> 32; r2 = r2 & 0xFFFFFFFFu64
        r3 = r3 + c2; var c3 = r3 >> 32; r3 = r3 & 0xFFFFFFFFu64
        r4 = r4 + c3; var c4 = r4 >> 32; r4 = r4 & 0xFFFFFFFFu64
        r5 = r5 + c4; var c5 = r5 >> 32; r5 = r5 & 0xFFFFFFFFu64
        r6 = r6 + c5; var c6 = r6 >> 32; r6 = r6 & 0xFFFFFFFFu64
        r7 = r7 + c6; var c7 = r7 >> 32; r7 = r7 & 0xFFFFFFFFu64

        // If any carry remains in c7, multiply by 38 and add to r0
        if(c7 > 0) {
            r0 = r0 + c7 * 38
            var _c = r0 >> 32
            r0 = r0 & 0xFFFFFFFFu64
            if(_c > 0) {
                r1 = r1 + _c; _c = r1 >> 32; r1 = r1 & 0xFFFFFFFFu64
                if(_c > 0) {
                    r2 = r2 + _c; _c = r2 >> 32; r2 = r2 & 0xFFFFFFFFu64
                    if(_c > 0) {
                        r3 = r3 + _c; _c = r3 >> 32; r3 = r3 & 0xFFFFFFFFu64
                        if(_c > 0) {
                            r4 = r4 + _c; _c = r4 >> 32; r4 = r4 & 0xFFFFFFFFu64
                            if(_c > 0) {
                                r5 = r5 + _c; _c = r5 >> 32; r5 = r5 & 0xFFFFFFFFu64
                                if(_c > 0) {
                                    r6 = r6 + _c; _c = r6 >> 32; r6 = r6 & 0xFFFFFFFFu64
                                    if(_c > 0) {
                                        r7 = r7 + _c
                                        var _c2 = r7 >> 32
                                        r7 = r7 & 0xFFFFFFFFu64
                                        if(_c2 > 0) {
                                            r0 = r0 + _c2 * 38
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Store as u32
        c[0] = r0 as u32; c[1] = r1 as u32; c[2] = r2 as u32; c[3] = r3 as u32
        c[4] = r4 as u32; c[5] = r5 as u32; c[6] = r6 as u32; c[7] = r7 as u32
        
        fe_strong_reduce(c)
    }

    // ─── Square (just calls fe_mul for correctness) ─────────────────────
    func fe_sq(c : *mut u32, a : *u32) {
        fe_mul(c, a, a)
    }

    // ─── Helper: zero u64 array ─────────────────────────────────────────
    func fe_zero_u64_arr(arr : *mut u64, n : size_t) {
        var i : size_t = 0
        while(i < n) { arr[i] = 0; i += 1 }
    }

    // ─── Modular inverse using Fermat's little theorem ──────────────────
    // a^(p-2) mod p where p-2 = 2^255 - 21
    // Uses square-and-multiply.
    func fe_inv(c : *mut u32, a : *u32) {
        // Exponent = 2^255 - 21 = 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEB
        // Bits: 254=1, 253=1, ..., 5=1, 4=0, 3=1, 2=0, 1=1, 0=1
        // Last byte 0xEB = 11101011
        //
        // Square-and-multiply from bit 254 down to 0:
        // Start with r = 1, then for each bit: r = r^2, if bit set: r = r * a
        
        var r : [8]u32; fe_set_small(&raw mut r[0], 1)
        
        var bit : i32 = 254
        while(bit >= 0) {
            fe_sq(&raw mut r[0], &raw r[0])
            
            // Check if bit of 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEB is set
            // 0xEB = 1110 1011, so last byte bits: 7=1,6=1,5=1,4=0,3=1,2=0,1=1,0=1
            // All other bytes (above byte 0) are 0xFF = all bits set
            var do_mul : bool = false
            var byte_pos = (bit / 8) as size_t
            var bit_in_byte = (bit % 8) as u32
            if(byte_pos > 0) {
                // For bytes 1-31 (bits 8-254), all bits are 1 (0xFF)
                do_mul = true
            } else {
                // For byte 0 (bits 0-7), check each bit of 0xEB
                if(bit_in_byte == 0 || bit_in_byte == 1 || bit_in_byte == 3 || bit_in_byte == 5 || bit_in_byte == 6 || bit_in_byte == 7) {
                    do_mul = true
                }
            }
            
            if(do_mul) {
                fe_mul(&raw mut r[0], &raw r[0], a)
            }
            bit -= 1
        }
        
        fe_copy(c, &raw r[0])
    }

    // ─── Conditional swap (constant-time using XOR mask) ───────────────
    func fe_cswap(a : *mut u32, b : *mut u32, sw : u32) {
        // Create a mask that's all-1s if sw == 1, all-0s if sw == 0
        var mask : u32 = 0u32 - sw  // 0 -> 0, 1 -> 0xFFFFFFFF
        var i : size_t = 0
        while(i < 8) {
            var diff = mask & (a[i] ^ b[i])
            a[i] = a[i] ^ diff
            b[i] = b[i] ^ diff
            i += 1
        }
    }

    // Clamp scalar per RFC 7748 Section 5
    public func x25519_clamp_scalar(scalar : *mut u8) {
        scalar[0] = scalar[0] & 0xF8 as u8
        scalar[31] = scalar[31] & 0x7F as u8
        scalar[31] = scalar[31] | 0x40 as u8
    }

    // ─── Montgomery Ladder ──────────────────────────────────────────────
    // Implements RFC 7748 Section 5.

    public func x25519_ladder(out : *mut u8, scalar : *u8, u : *u8) {
        // Decode u coordinate
        var u_fe : [8]u32
        fe_decode(&raw mut u_fe[0], u)

        if(X25519_DEBUG) {
            var _enc : [32]u8; fe_encode(&raw mut _enc[0], &raw u_fe[0])
            printf("[LADDER] u_decoded: ");
            var _xi : size_t = 0; while(_xi < 8) { printf("%08x ", u_fe[_xi]); _xi += 1 }
            printf(" => "); _xi = 0; while(_xi < 32) { printf("%02x", _enc[_xi] as int); _xi += 1 }
            printf("\n")
        }

        // State variables
        var x2 : [8]u32; fe_set_small(&raw mut x2[0], 1)   // X2 = 1
        var z2 : [8]u32; fe_zero(&raw mut z2[0])             // Z2 = 0
        var x3 : [8]u32; fe_copy(&raw mut x3[0], &raw u_fe[0])  // X3 = u
        var z3 : [8]u32; fe_set_small(&raw mut z3[0], 1)     // Z3 = 1

        // a24 = 121665
        var a24 : [8]u32; fe_set_small(&raw mut a24[0], 121665)

        // Temp variables
        var A : [8]u32; var AA : [8]u32
        var B : [8]u32; var BB : [8]u32
        var E : [8]u32; var C : [8]u32; var D : [8]u32
        var DA : [8]u32; var CB : [8]u32
        var T : [8]u32; var DT : [8]u32

        var swap : u32 = 0

        var bit : i32 = 254
        while(bit >= 0) {
            var byte_idx = (bit / 8) as size_t
            var bit_idx = (bit % 8) as u32
            var kbit = ((scalar[byte_idx] >> bit_idx) as u32) & 1u32

            swap = swap ^ kbit

            fe_cswap(&raw mut x2[0], &raw mut x3[0], swap)
            fe_cswap(&raw mut z2[0], &raw mut z3[0], swap)

            swap = kbit

            // A = x2 + z2, AA = A^2
            fe_add(&raw mut A[0], &raw x2[0], &raw z2[0])
            fe_sq(&raw mut AA[0], &raw A[0])

            // B = x2 - z2, BB = B^2
            fe_sub(&raw mut B[0], &raw x2[0], &raw z2[0])
            fe_sq(&raw mut BB[0], &raw B[0])

            // E = AA - BB
            fe_sub(&raw mut E[0], &raw AA[0], &raw BB[0])

            // C = x3 + z3
            fe_add(&raw mut C[0], &raw x3[0], &raw z3[0])

            // D = x3 - z3
            fe_sub(&raw mut D[0], &raw x3[0], &raw z3[0])

            // DA = D * A, CB = C * B
            fe_mul(&raw mut DA[0], &raw D[0], &raw A[0])
            fe_mul(&raw mut CB[0], &raw C[0], &raw B[0])

            // x3 = (DA + CB)^2
            fe_add(&raw mut x3[0], &raw DA[0], &raw CB[0])
            fe_sq(&raw mut x3[0], &raw x3[0])

            // z3 = u * (DA - CB)^2
            fe_sub(&raw mut z3[0], &raw DA[0], &raw CB[0])
            fe_sq(&raw mut z3[0], &raw z3[0])
            fe_mul(&raw mut z3[0], &raw z3[0], &raw u_fe[0])

            // x2 = AA * BB
            fe_mul(&raw mut x2[0], &raw AA[0], &raw BB[0])

            // z2 = E * (AA + a24 * E)
            fe_mul(&raw mut T[0], &raw a24[0], &raw E[0])
            fe_add(&raw mut DT[0], &raw AA[0], &raw T[0])
            fe_mul(&raw mut z2[0], &raw E[0], &raw DT[0])

            bit -= 1
        }

        // Final conditional swap
        fe_cswap(&raw mut x2[0], &raw mut x3[0], swap)
        fe_cswap(&raw mut z2[0], &raw mut z3[0], swap)

        // Result = x2 * z2^(-1) mod p
        var z2_inv : [8]u32
        fe_inv(&raw mut z2_inv[0], &raw z2[0])
        fe_mul(&raw mut x2[0], &raw x2[0], &raw z2_inv[0])

        // Encode to 32 bytes little-endian
        fe_encode(out, &raw x2[0])
    }

    // Generate an x25519 keypair
    public func x25519_generate_keypair(priv : *mut u8, pub : *mut u8) : int {
        var rng_ret = random_fill(priv, 32)
        if(rng_ret < 0) { return rng_ret }

        x25519_clamp_scalar(priv)
        x25519_ladder(pub, priv, &raw X25519_BASE_POINT[0])
        return 0
    }

    // Compute shared secret from private key and peer public key
    public func x25519_compute_shared(priv : *u8, peer_pub : *u8, shared : *mut u8) : int {
        // Clamp the private key per RFC 7748 Section 5
        var clamped_priv : [32]u8
        var ci : size_t = 0
        while(ci < 32) {
            clamped_priv[ci] = priv[ci]
            ci += 1
        }
        x25519_clamp_scalar(&raw mut clamped_priv[0])
        x25519_ladder(shared, &raw clamped_priv[0], peer_pub)

        // Check for all-zero output (low-order point)
        var all_zero = true
        var i : size_t = 0
        while(i < 32) {
            if(shared[i] != 0) { all_zero = false }
            i += 1
        }
        if(all_zero) { return ERR_ECP_INVALID_KEY }

        return 0
    }

} // namespace tls
