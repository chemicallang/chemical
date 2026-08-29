// ============================================================================
// Big Number Math — Arbitrary Precision Integers
// ============================================================================
// Port of mbedTLS bignum core to Chemical.
// Supports RSA-2048 (64 x u32 limbs) and P-256 (8 x u32 limbs).
// Uses Montgomery multiplication for efficient modular exponentiation.
// ============================================================================

public namespace tls {

    public comptime const ERR_MPI_BAD_INPUT_DATA = -0x0010
    public comptime const ERR_MPI_INVALID_CHARACTER = -0x0020
    public comptime const ERR_MPI_BUFFER_TOO_SMALL = -0x0030
    public comptime const ERR_MPI_NEGATIVE_VALUE = -0x0040
    public comptime const ERR_MPI_DIVISION_BY_ZERO = -0x0050
    public comptime const ERR_MPI_ALLOC_FAILED = -0x0060

    public comptime const MAX_LIMBS : size_t = 512
    public comptime const BITS_PER_LIMB : size_t = 32

    public struct Mpi {
        var s : int
        var n : size_t
        var p : [MAX_LIMBS]u32
    }

    public func mpi_init(m : *mut Mpi) {
        m.s = 1; m.n = 0
        var i : size_t = 0
        while(i < MAX_LIMBS) { m.p[i] = 0; i += 1 }
    }

    public func mpi_lset(m : *mut Mpi, val : i64) {
        if(val < 0) { m.s = -1 } else { m.s = 1 }
        var uv : u64 = 0
        if(val >= 0) { uv = val as u64 }
        else { uv = 0u64 - (val as u64) }
        m.p[0] = (uv & 0xFFFFFFFFu64) as u32
        m.n = 1
        if(uv > 0xFFFFFFFFu64) {
            m.p[1] = (uv >> 32) as u32
            m.n = 2
        }
        var i = m.n
        while(i < MAX_LIMBS) { m.p[i] = 0; i += 1 }
    }

    public func mpi_grow(m : *mut Mpi, nlimbs : size_t) : int {
        if(nlimbs > MAX_LIMBS) { return ERR_MPI_ALLOC_FAILED }
        if(m.n < nlimbs) {
            var i = m.n
            while(i < nlimbs) { m.p[i] = 0; i += 1 }
            m.n = nlimbs
        }
        return 0
    }

    public func mpi_trim(m : *mut Mpi) {
        while(m.n > 0 && m.p[m.n - 1] == 0) { m.n -= 1 }
        if(m.n == 0) { m.s = 1 }
    }

    public func mpi_bitlen(m : *mut Mpi) : size_t {
        mpi_trim(m)
        if(m.n == 0) { return 0 }
        var top = m.p[m.n - 1]
        var bits : size_t = (m.n - 1) * BITS_PER_LIMB
        var msb : u32 = 0x80000000u32
        var i : size_t = 32
        while(msb != 0) {
            if(top & msb) { return bits + i }
            msb = msb >> 1
            i -= 1
        }
        return bits
    }

    public func mpi_size(m : *mut Mpi) : size_t {
        return (mpi_bitlen(m) + 7) / 8
    }

    public func mpi_copy(dst : *mut Mpi, src : *mut Mpi) {
        dst.s = src.s; dst.n = src.n
        var i : size_t = 0
        while(i < src.n) { dst.p[i] = src.p[i]; i += 1 }
        while(i < MAX_LIMBS) { dst.p[i] = 0; i += 1 }
    }

    public func mpi_cmp_abs(a : *mut Mpi, b : *mut Mpi) : int {
        mpi_trim(a); mpi_trim(b)
        if(a.n > b.n) { return 1 }
        if(a.n < b.n) { return -1 }
        var i = a.n
        while(i > 0) { i -= 1
            if(a.p[i] > b.p[i]) { return 1 }
            if(a.p[i] < b.p[i]) { return -1 }
        }
        return 0
    }

    public func mpi_cmp(a : *mut Mpi, b : *mut Mpi) : int {
        mpi_trim(a); mpi_trim(b)
        if(a.n == 0 && b.n == 0) { return 0 }
        if(a.n == 0) { return -b.s }
        if(b.n == 0) { return a.s }
        if(a.s != b.s) { return a.s }
        if(a.s > 0) { return mpi_cmp_abs(a, b) }
        return -mpi_cmp_abs(a, b)
    }

    public func mpi_cmp_int(m : *mut Mpi, val : i64) : int {
        var tmp : Mpi; mpi_init(unsafe(&raw mut tmp)); mpi_lset(unsafe(&raw mut tmp), val)
        return mpi_cmp(m, unsafe(&raw mut tmp))
    }

    public func mpi_is_zero(m : *mut Mpi) : bool {
        mpi_trim(m)
        return m.n == 0
    }

    // ─── Addition / Subtraction ──────────────────────────────────────────

    public func mpi_add_abs(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        var max_n = a.n; if(b.n > max_n) { max_n = b.n }
        var ret = mpi_grow(x, max_n + 1)
        if(ret < 0) { return ret }
        var carry : u64 = 0
        var i : size_t = 0
        while(i < max_n) {
            var av : u64 = 0; var bv : u64 = 0
            if(i < a.n) { av = a.p[i] as u64 }
            if(i < b.n) { bv = b.p[i] as u64 }
            var sum = av + bv + carry
            x.p[i] = (sum & 0xFFFFFFFFu64) as u32
            carry = sum >> 32; i += 1
        }
        if(carry > 0) { x.p[i] = carry as u32; x.n = max_n + 1 }
        else { x.n = max_n }
        x.s = 1; mpi_trim(x); return 0
    }

    public func mpi_sub_abs(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        if(mpi_cmp_abs(a, b) < 0) { return ERR_MPI_NEGATIVE_VALUE }
        var ret = mpi_grow(x, a.n)
        if(ret < 0) { return ret }
        var borrow : u64 = 0
        var i : size_t = 0
        while(i < a.n) {
            var av = a.p[i] as u64
            var bv : u64 = 0
            if(i < b.n) { bv = b.p[i] as u64 }
            var diff : u64 = 0
            if(av >= bv + borrow) { diff = av - bv - borrow; borrow = 0 }
            else { diff = (av + 0x100000000u64) - bv - borrow; borrow = 1 }
            x.p[i] = (diff & 0xFFFFFFFFu64) as u32; i += 1
        }
        x.n = a.n; x.s = 1; mpi_trim(x); return 0
    }

    public func mpi_add(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        var ret : int = 0
        // Same sign: add magnitudes
        if(a.s == b.s) {
            var asgn = a.s
            ret = mpi_add_abs(x, a, b)
            if(ret >= 0) { x.s = asgn }
            return ret
        }
        // Opposite signs: subtract smaller from larger
        if(mpi_cmp_abs(a, b) >= 0) {
            var asgn = a.s
            ret = mpi_sub_abs(x, a, b)
            if(ret >= 0) { x.s = asgn }
        } else {
            var bsgn = b.s
            ret = mpi_sub_abs(x, b, a)
            if(ret >= 0) { x.s = bsgn }
        }
        return ret
    }

    public func mpi_sub(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        var ret : int = 0
        // Opposite signs: add magnitudes
        if(a.s != b.s) {
            var asgn = a.s
            ret = mpi_add_abs(x, a, b)
            if(ret >= 0) { x.s = asgn }
            return ret
        }
        // Same sign: subtract smaller from larger
        if(mpi_cmp_abs(a, b) >= 0) {
            var asgn = a.s
            ret = mpi_sub_abs(x, a, b)
            if(ret >= 0) { x.s = asgn }
        } else {
            var asgn = a.s
            ret = mpi_sub_abs(x, b, a)
            if(ret >= 0) { x.s = -asgn }
        }
        return ret
    }

    // ─── Multiplication ──────────────────────────────────────────────────

    public func mpi_mul(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        mpi_trim(a); mpi_trim(b)
        if(a.n == 0 || b.n == 0) { mpi_lset(x, 0); return 0 }
        // Handle input/output aliasing: save a and b before zeroing x
        var a_sav : Mpi; mpi_init(unsafe(&raw mut a_sav))
        var b_sav : Mpi; mpi_init(unsafe(&raw mut b_sav))
        if(a == x) { mpi_copy(unsafe(&raw mut a_sav), a); a = unsafe(&raw mut a_sav) }
        if(b == x) { mpi_copy(unsafe(&raw mut b_sav), b); b = unsafe(&raw mut b_sav) }
        // a * b = sum over j of (a * b.p[j]) * 2^(32*j)
        mpi_lset(x, 0)
        var tmp : Mpi; mpi_init(unsafe(&raw mut tmp))
        var j : size_t = 0
        while(j < b.n) {
            var ret = mpi_mul_int(unsafe(&raw mut tmp), a, b.p[j])
            if(ret < 0) { return ret }
            if(j > 0) {
                ret = mpi_shift_l(unsafe(&raw mut tmp), j * BITS_PER_LIMB)
                if(ret < 0) { return ret }
            }
            ret = mpi_add(x, x, unsafe(&raw mut tmp))
            if(ret < 0) { return ret }
            j += 1
        }
        x.s = a.s * b.s
        mpi_trim(x)
        return 0
    }

    public func mpi_mul_int(x : *mut Mpi, a : *mut Mpi, b : u32) : int {
        if(b == 0 || mpi_is_zero(a)) { mpi_lset(x, 0); return 0 }
        var ret = mpi_grow(x, a.n + 1)
        if(ret < 0) { return ret }
        var carry : u64 = 0
        var i : size_t = 0
        while(i < a.n) {
            var prod = (a.p[i] as u64) * (b as u64) + carry
            x.p[i] = (prod & 0xFFFFFFFFu64) as u32
            carry = prod >> 32; i += 1
        }
        x.p[i] = carry as u32; x.n = a.n + 1; x.s = a.s; mpi_trim(x); return 0
    }

    // ─── Division (schoolbook long division) ─────────────────────────────

    public func mpi_div(q : *mut Mpi, r : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        mpi_trim(b)
        if(b.n == 0) { return ERR_MPI_DIVISION_BY_ZERO }
        if(a.n == 0) {
            if(q != null) { mpi_lset(q, 0) }
            if(r != null) { mpi_lset(r, 0) }
            return 0
        }

        var sign = a.s * b.s
        var A : Mpi; mpi_init(unsafe(&raw mut A)); mpi_copy(unsafe(&raw mut A), a); A.s = 1
        var B : Mpi; mpi_init(unsafe(&raw mut B)); mpi_copy(unsafe(&raw mut B), b); B.s = 1

        if(mpi_cmp_abs(unsafe(&raw mut A), unsafe(&raw mut B)) < 0) {
            if(q != null) { mpi_lset(q, 0) }
            if(r != null) {
                var a_sign = a.s
                mpi_copy(r, unsafe(&raw mut A))
                r.s = a_sign
            }
            return 0
        }

        var Q : Mpi; mpi_init(unsafe(&raw mut Q))
        var a_bits = mpi_bitlen(unsafe(&raw mut A))
        var b_bits = mpi_bitlen(unsafe(&raw mut B))
        var shift = a_bits - b_bits

        // Shift B left by 'shift' bits
        var ret = mpi_grow(unsafe(&raw mut B), B.n + (shift / BITS_PER_LIMB) + 2)
        if(ret < 0) { return ret }

        var limb_shift = shift / BITS_PER_LIMB
        var bit_shift = shift % BITS_PER_LIMB

        if(limb_shift > 0) {
            var i = B.n
            while(i > 0) { i -= 1; B.p[i + limb_shift] = B.p[i] }
            var j : size_t = 0
            while(j < limb_shift) { B.p[j] = 0; j += 1 }
            B.n += limb_shift
        }
        if(bit_shift > 0) {
            var carry : u64 = 0
            var i : size_t = 0
            while(i < B.n) {
                var val = (B.p[i] as u64) << bit_shift | carry
                B.p[i] = (val & 0xFFFFFFFFu64) as u32
                carry = val >> 32; i += 1
            }
            if(carry > 0) { B.p[B.n] = carry as u32; B.n += 1 }
        }

        var cur_shift = shift
        while(cur_shift > 0 || mpi_cmp_abs(unsafe(&raw mut A), unsafe(&raw mut B)) >= 0) {
            if(mpi_cmp_abs(unsafe(&raw mut A), unsafe(&raw mut B)) >= 0) {
                mpi_sub_abs(unsafe(&raw mut A), unsafe(&raw mut A), unsafe(&raw mut B))
                // Set the quotient bit at position cur_shift
                var limb_idx = cur_shift / BITS_PER_LIMB
                var bit_idx = cur_shift % BITS_PER_LIMB
                if(Q.n <= limb_idx) { mpi_grow(unsafe(&raw mut Q), limb_idx + 1); Q.n = limb_idx + 1 }
                Q.p[limb_idx] = Q.p[limb_idx] | (1u32 << bit_idx)
            }

            // Shift B right by 1
            var carry : u32 = 0
            var i = B.n
            while(i > 0) { i -= 1
                var val = (carry as u64) << 32 | (B.p[i] as u64)
                B.p[i] = (val >> 1) as u32
                carry = (val & 1) as u32
            }
            if(B.n > 0 && B.p[B.n - 1] == 0) { B.n -= 1 }

            if(cur_shift == 0) { break }
            cur_shift -= 1
        }

        mpi_trim(unsafe(&raw mut Q)); Q.s = sign
        if(q != null) { mpi_copy(q, unsafe(&raw mut Q)) }
        if(r != null) {
            var a_sign = a.s
            mpi_trim(unsafe(&raw mut A)); A.s = a_sign; mpi_copy(r, unsafe(&raw mut A))
        }
        return 0
    }

    public func mpi_mod(r : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        // Handle negative dividend: result must be non-negative modulo |b|
        var ret = mpi_div(null, r, a, b)
        if(ret < 0) { return ret }
        // If remainder is negative, add |b| to get positive residue
        if(r != null && r.s < 0) {
            var tmp : Mpi; mpi_init(unsafe(&raw mut tmp))
            ret = mpi_add(unsafe(&raw mut tmp), r, b)
            if(ret < 0) { return ret }
            mpi_copy(r, unsafe(&raw mut tmp))
        }
        return 0
    }

    // ─── Montgomery Modular Exponentiation ───────────────────────────────

    func montgomery_mul(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi, n : *mut Mpi, n_inv0 : u32) : int {
        // Handle input/output aliasing: if a or b alias x, save them first
        var a_sav : Mpi; mpi_init(unsafe(&raw mut a_sav))
        var b_sav : Mpi; mpi_init(unsafe(&raw mut b_sav))
        if(a == x) { mpi_copy(unsafe(&raw mut a_sav), a); a = unsafe(&raw mut a_sav) }
        if(b == x) { mpi_copy(unsafe(&raw mut b_sav), b); b = unsafe(&raw mut b_sav) }

        // Need n.n + 2 limbs: n for result + 1 for carry + 1 safety
        var work_limbs = n.n + 2
        var ret = mpi_grow(x, work_limbs)
        if(ret < 0) { return ret }
        var i : size_t = 0
        while(i < work_limbs) { x.p[i] = 0; i += 1 }; x.n = work_limbs

        // Coarsely Integrated Operand Scanning (CIOS) Montgomery multiplication
        i = 0
        while(i < n.n) {
            var ai : u32 = 0
            if(i < a.n) { ai = a.p[i] }

            // u = (T[0] + a[i]*b[0]) * n_inv0 mod 2^32
            var b0 = b.p[0]
            var t0 = (x.p[0] as u64) + (ai as u64) * (b0 as u64)
            var u_val = (t0 * (n_inv0 as u64)) & 0xFFFFFFFFu64

            // Step 1: Multiply-accumulate: T += a[i] * b
            // Step 2: Reduce: T += u * n; T >>= 32
            var carry_mul : u64 = 0
            var carry_red : u64 = 0
            var j : size_t = 0
            while(j < n.n) {
                var bj : u32 = 0; if(j < b.n) { bj = b.p[j] }
                var nj = n.p[j]

                // T[j] += a[i]*b[j] + carry_mul
                var mul_sum = (x.p[j] as u64) + (ai as u64) * (bj as u64) + carry_mul
                var mul_low = (mul_sum & 0xFFFFFFFFu64) as u32
                carry_mul = mul_sum >> 32

                // T[j] += u * n[j] + carry_red (use the multiply result)
                var red_sum = (mul_low as u64) + (u_val as u64) * (nj as u64) + carry_red
                x.p[j] = (red_sum & 0xFFFFFFFFu64) as u32
                carry_red = red_sum >> 32

                j += 1
            }

            // Add remaining carries
            var total_carry = carry_mul + carry_red
            var k : size_t = n.n
            while(total_carry > 0) {
                var sum = (x.p[k] as u64) + (total_carry & 0xFFFFFFFFu64)
                x.p[k] = (sum & 0xFFFFFFFFu64) as u32
                total_carry = (sum >> 32) + (total_carry >> 32)
                k += 1
            }

            // Shift T right by 1 limb (the lowest limb is 0 mod 2^32 and is dropped)
            var si : size_t = 0
            while(si < work_limbs - 1) {
                x.p[si] = x.p[si + 1]
                si += 1
            }
            x.p[work_limbs - 1] = 0

            i += 1
        }

        // After the n shifts, T occupies x[0..n.n] (n.n+1 limbs) with
        // x[n.n] being the carry limb (T may be >= R = 2^(32*n.n)).
        // Per mbedTLS mpi_core_montmul: compute X = T - N over n.n limbs,
        // track borrow, and select the result as follows:
        //   carry=1 or borrow=0  -> result is X (= T - N)
        //   carry=0 and borrow=1 -> result is T (T < N)
        var carry : u64 = x.p[n.n] as u64
        var X : Mpi; mpi_init(unsafe(&raw mut X))
        var borrow : u64 = 0
        var j : size_t = 0
        while(j < n.n) {
            var av = x.p[j] as u64
            var bv = n.p[j] as u64
            var diff : u64 = 0
            if(av >= bv + borrow) { diff = av - bv - borrow; borrow = 0 }
            else { diff = (av + 0x100000000u64) - bv - borrow; borrow = 1 }
            X.p[j] = (diff & 0xFFFFFFFFu64) as u32
            j += 1
        }
        X.n = n.n; X.s = 1
        var take_x : bool = false
        if(carry != 0) { take_x = true }
        else {
            if(borrow == 0) { take_x = true }
            else { take_x = false }
        }
        if(take_x) { mpi_copy(x, unsafe(&raw mut X)) }
        else {
            x.n = n.n
        }
        mpi_trim(x); return 0
    }

    func to_montgomery(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi, n_inv0 : u32, r2 : *mut Mpi) : int {
        return montgomery_mul(x, a, r2, n, n_inv0)
    }

    func from_montgomery(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi, n_inv0 : u32) : int {
        var one : Mpi; mpi_init(unsafe(&raw mut one)); mpi_lset(unsafe(&raw mut one), 1)
        return montgomery_mul(x, a, unsafe(&raw mut one), n, n_inv0)
    }

    func compute_r2(r2 : *mut Mpi, n : *mut Mpi) : int {
        var R : Mpi; mpi_init(unsafe(&raw mut R))
        var ret = mpi_grow(unsafe(&raw mut R), n.n + 1)
        if(ret < 0) { return ret }
        R.p[n.n] = 1; R.n = n.n + 1
        var R_mod : Mpi; mpi_init(unsafe(&raw mut R_mod))
        ret = mpi_mod(unsafe(&raw mut R_mod), unsafe(&raw mut R), n)
        if(ret < 0) { return ret }
        ret = mpi_mul(r2, unsafe(&raw mut R_mod), unsafe(&raw mut R_mod))
        if(ret < 0) { return ret }
        ret = mpi_mod(r2, r2, n)
        return ret
    }

    func mpi_exp_mod_fallback(x : *mut Mpi, a : *mut Mpi, e : *mut Mpi, n : *mut Mpi) : int {
        // Simple left-to-right binary exponentiation with modular reduction
        mpi_lset(x, 1)
        var bitlen = mpi_bitlen(e)
        var i = bitlen
        while(i > 0) {
            i -= 1
            var tmp : Mpi; mpi_init(unsafe(&raw mut tmp))
            var ret = mpi_mul(unsafe(&raw mut tmp), x, x)
            if(ret < 0) { return ret }
            ret = mpi_mod(x, unsafe(&raw mut tmp), n)
            if(ret < 0) { return ret }
            var limb_idx = i / BITS_PER_LIMB
            var bit_idx = i % BITS_PER_LIMB
            if(e.p[limb_idx] & (1u32 << bit_idx)) {
                ret = mpi_mul(unsafe(&raw mut tmp), x, a)
                if(ret < 0) { return ret }
                ret = mpi_mod(x, unsafe(&raw mut tmp), n)
                if(ret < 0) { return ret }
            }
        }
        return 0
    }

    public func mpi_exp_mod(x : *mut Mpi, a : *mut Mpi, e : *mut Mpi, n : *mut Mpi) : int {
        if(mpi_cmp_int(n, 0) <= 0) {
            return ERR_MPI_BAD_INPUT_DATA
        }
        mpi_trim(e)
        if(e.n == 0) { mpi_lset(x, 1); return 0 }

        // Even modulus: use fallback
        if((n.p[0] & 1) == 0) {
            return mpi_exp_mod_fallback(x, a, e, n)
        }

        // Precompute R^2 mod N
        var r2 : Mpi; mpi_init(unsafe(&raw mut r2))
        var ret = compute_r2(unsafe(&raw mut r2), n)
        if(ret < 0) { return ret }

        // Compute n_inv0 = -n0^-1 mod 2^32 using Newton's method
        var n0 = n.p[0]
        var x0 : u32 = 1
        var iter : size_t = 0
        while(iter < 5) {
            x0 = x0 * (2u32 - n0 * x0)
            iter += 1
        }
        var n_inv0 = 0u32 - x0

        // Convert A to Montgomery representation
        var A_mont : Mpi; mpi_init(unsafe(&raw mut A_mont))
        ret = to_montgomery(unsafe(&raw mut A_mont), a, n, n_inv0, unsafe(&raw mut r2))
        if(ret < 0) { return ret }

        // Start with Montgomery form of 1 (which is R mod N)
        var result : Mpi; mpi_init(unsafe(&raw mut result))
        var one : Mpi; mpi_init(unsafe(&raw mut one)); mpi_lset(unsafe(&raw mut one), 1)
        ret = to_montgomery(unsafe(&raw mut result), unsafe(&raw mut one), n, n_inv0, unsafe(&raw mut r2))
        if(ret < 0) { return ret }

        // Left-to-right binary exponentiation
        var bitlen = mpi_bitlen(e)
        var i = bitlen
        while(i > 0) {
            i -= 1
            ret = montgomery_mul(unsafe(&raw mut result), unsafe(&raw mut result), unsafe(&raw mut result), n, n_inv0)
            if(ret < 0) { return ret }
            var limb_idx = i / BITS_PER_LIMB
            var bit_idx = i % BITS_PER_LIMB
            if(e.p[limb_idx] & (1u32 << bit_idx)) {
                ret = montgomery_mul(unsafe(&raw mut result), unsafe(&raw mut result), unsafe(&raw mut A_mont), n, n_inv0)
                if(ret < 0) { return ret }
            }
        }

        // Convert back
        ret = from_montgomery(x, unsafe(&raw mut result), n, n_inv0)
        if(ret < 0) { return ret }

        // Handle negative base with odd exponent
        if(a.s < 0 && (e.p[0] & 1) && !mpi_is_zero(x)) {
            mpi_sub(x, n, x); x.s = -1
        }
        return 0
    }

    // ─── Modular Inverse (Extended Euclidean) ────────────────────────────

    // Computes x = a^-1 mod n via the extended Euclidean algorithm
    // (division-based). Unlike the binary extended GCD, this works for any
    // modulus (odd or even) and for any a coprime to n.
    public func mpi_mod_inv(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi) : int {
        if(mpi_cmp_int(n, 1) <= 0) { return ERR_MPI_BAD_INPUT_DATA }

        // r0 = n, r1 = a mod n
        var r0 : Mpi; mpi_init(unsafe(&raw mut r0))
        var r1 : Mpi; mpi_init(unsafe(&raw mut r1))
        mpi_copy(unsafe(&raw mut r0), n)
        var ret = mpi_mod(unsafe(&raw mut r1), a, n)
        if(ret < 0) { return ret }

        // t0 = 0, t1 = 1  (Bezout coefficients for r0 and r1)
        var t0 : Mpi; mpi_init(unsafe(&raw mut t0)); mpi_lset(unsafe(&raw mut t0), 0)
        var t1 : Mpi; mpi_init(unsafe(&raw mut t1)); mpi_lset(unsafe(&raw mut t1), 1)

        while(!mpi_is_zero(unsafe(&raw mut r1))) {
            // q = r0 / r1, r2 = r0 mod r1
            var q : Mpi; mpi_init(unsafe(&raw mut q))
            var r2 : Mpi; mpi_init(unsafe(&raw mut r2))
            ret = mpi_div(unsafe(&raw mut q), unsafe(&raw mut r2), unsafe(&raw mut r0), unsafe(&raw mut r1))
            if(ret < 0) { return ret }

            // t2 = t0 - q * t1
            var qt : Mpi; mpi_init(unsafe(&raw mut qt))
            ret = mpi_mul(unsafe(&raw mut qt), unsafe(&raw mut q), unsafe(&raw mut t1))
            if(ret < 0) { return ret }
            var t2 : Mpi; mpi_init(unsafe(&raw mut t2))
            ret = mpi_sub(unsafe(&raw mut t2), unsafe(&raw mut t0), unsafe(&raw mut qt))
            if(ret < 0) { return ret }

            // Shift: r0 = r1, r1 = r2; t0 = t1, t1 = t2
            mpi_copy(unsafe(&raw mut r0), unsafe(&raw mut r1))
            mpi_copy(unsafe(&raw mut r1), unsafe(&raw mut r2))
            mpi_copy(unsafe(&raw mut t0), unsafe(&raw mut t1))
            mpi_copy(unsafe(&raw mut t1), unsafe(&raw mut t2))
        }

        // r0 = gcd(a, n). If it's not 1, no inverse exists.
        if(mpi_cmp_int(unsafe(&raw mut r0), 1) != 0) {
            return ERR_MPI_BAD_INPUT_DATA
        }

        // t0 is the inverse (may be negative); reduce mod n
        mpi_mod(x, unsafe(&raw mut t0), n)
        return 0
    }

    // ─── Shift Operations ──────────────────────────────────────────────

    public func mpi_shift_l(m : *mut Mpi, count : size_t) : int {
        if(m.n == 0 || count == 0) { return 0 }
        var limb_shift = count / BITS_PER_LIMB
        var bit_shift = count % BITS_PER_LIMB
        var old_n = m.n
        // Grow to accommodate the shift
        var new_n = old_n + limb_shift + 1
        var ret = mpi_grow(m, new_n)
        if(ret < 0) { return ret }
        // Shift whole limbs left (copy from high to low so source limbs
        // are not clobbered), then zero the vacated low limbs.
        if(limb_shift > 0) {
            var i = new_n
            while(i > limb_shift) {
                i -= 1
                m.p[i] = m.p[i - limb_shift]
            }
            var j : size_t = 0
            while(j < limb_shift) { m.p[j] = 0; j += 1 }
            m.n = old_n + limb_shift
        }
        // Shift bits within limbs
        if(bit_shift > 0) {
            var carry : u32 = 0
            var i : size_t = 0
            while(i < m.n) {
                var val = ((m.p[i] as u64) << bit_shift) | (carry as u64)
                m.p[i] = (val & 0xFFFFFFFFu64) as u32
                carry = (val >> 32) as u32
                i += 1
            }
            if(carry > 0) { m.p[m.n] = carry; m.n += 1 }
        }
        mpi_trim(m)
        return 0
    }

    public func mpi_shift_r(m : *mut Mpi, count : size_t) : int {
        if(m.n == 0 || count == 0) { return 0 }
        var limb_shift = count / BITS_PER_LIMB
        var bit_shift = count % BITS_PER_LIMB
        if(limb_shift >= m.n) { mpi_lset(m, 0); return 0 }
        if(limb_shift > 0) {
            var i : size_t = 0
            while(i < m.n - limb_shift) { m.p[i] = m.p[i + limb_shift]; i += 1 }
            while(i < m.n) { m.p[i] = 0; i += 1 }
            m.n -= limb_shift
        }
        if(bit_shift > 0) {
            var carry : u32 = 0
            var i = m.n
            while(i > 0) { i -= 1
                var val = (carry << (BITS_PER_LIMB - bit_shift)) | (m.p[i] >> bit_shift)
                carry = m.p[i] & ((1u32 << bit_shift) - 1)
                m.p[i] = val
            }
            mpi_trim(m)
        }
        return 0
    }

    // ─── Import / Export ────────────────────────────────────────────────

    public func mpi_read_binary(m : *mut Mpi, buf : *u8, buflen : size_t) : int {
        mpi_init(m)
        if(buflen == 0) { return 0 }
        var limbs = (buflen * 8 + BITS_PER_LIMB - 1) / BITS_PER_LIMB
        var ret = mpi_grow(m, limbs)
        if(ret < 0) { return ret }
        var i : size_t = 0
        while(i < buflen) {
            var byte_val = buf[buflen - 1 - i] as u32
            var limb = i / 4; var offset = (i % 4) * 8
            m.p[limb] = m.p[limb] | (byte_val << offset)
            i += 1
        }
        m.n = limbs; m.s = 1; mpi_trim(m); return 0
    }

    public func mpi_write_binary(m : *mut Mpi, buf : *mut u8, buflen : size_t) : int {
        var size = mpi_size(m)
        if(buflen < size) { return ERR_MPI_BUFFER_TOO_SMALL }
        var pad = buflen - size
        var i : size_t = 0
        while(i < pad) { buf[i] = 0; i += 1 }
        var j : size_t = 0
        while(j < size) {
            buf[buflen - 1 - j] = ((m.p[j / 4] >> ((j % 4) * 8)) & 0xFFu32) as u8
            j += 1
        }
        return 0
    }

    public func mpi_gcd(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        var A : Mpi; mpi_init(unsafe(&raw mut A)); mpi_copy(unsafe(&raw mut A), a)
        var B : Mpi; mpi_init(unsafe(&raw mut B)); mpi_copy(unsafe(&raw mut B), b)
        while(!mpi_is_zero(unsafe(&raw mut B))) {
            var T : Mpi; mpi_init(unsafe(&raw mut T))
            mpi_mod(unsafe(&raw mut T), unsafe(&raw mut A), unsafe(&raw mut B))
            mpi_copy(unsafe(&raw mut A), unsafe(&raw mut B))
            mpi_copy(unsafe(&raw mut B), unsafe(&raw mut T))
        }
        mpi_copy(x, unsafe(&raw mut A)); x.s = 1; return 0
    }

} // namespace tls
