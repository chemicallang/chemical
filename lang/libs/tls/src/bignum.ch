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

    public comptime const MAX_LIMBS : size_t = 256
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

    public func mpi_lset(m : *mut Mpi, val : i32) {
        if(val < 0) { m.s = -1 } else { m.s = 1 }
        var uv : u32 = 0
        if(val >= 0) { uv = val as u32 }
        else { uv = 0u32 - (val as u32) }
        m.p[0] = uv; m.n = 1
        var i : size_t = 1
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
        while(msb != 0) {
            if(top & msb) { return bits + 1 }
            bits += 1
            msb = msb >> 1
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

    public func mpi_cmp_int(m : *mut Mpi, val : i32) : int {
        var tmp : Mpi; mpi_init(&raw mut tmp); mpi_lset(&raw mut tmp, val)
        return mpi_cmp(m, &raw mut tmp)
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
        if(a.s * b.s >= 0) {
            ret = mpi_add_abs(x, a, b)
            if(ret >= 0) { x.s = a.s }
            return ret
        }
        if(mpi_cmp_abs(a, b) >= 0) {
            ret = mpi_sub_abs(x, a, b)
            if(ret >= 0) { x.s = a.s }
        } else {
            ret = mpi_sub_abs(x, b, a)
            if(ret >= 0) { x.s = b.s }
        }
        return ret
    }

    public func mpi_sub(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        var ret : int = 0
        if(a.s != b.s) {
            ret = mpi_add_abs(x, a, b)
            if(ret >= 0) { x.s = a.s }
            return ret
        }
        if(mpi_cmp_abs(a, b) >= 0) {
            ret = mpi_sub_abs(x, a, b)
            if(ret >= 0) { x.s = a.s }
        } else {
            ret = mpi_sub_abs(x, b, a)
            if(ret >= 0) { x.s = -a.s }
        }
        return ret
    }

    // ─── Multiplication ──────────────────────────────────────────────────

    public func mpi_mul(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        mpi_trim(a); mpi_trim(b)
        if(a.n == 0 || b.n == 0) { mpi_lset(x, 0); return 0 }
        var ret = mpi_grow(x, a.n + b.n)
        if(ret < 0) { return ret }
        var i : size_t = 0
        while(i < a.n + b.n) { x.p[i] = 0; i += 1 }
        i = 0
        while(i < a.n) {
            var carry : u64 = 0
            var j : size_t = 0
            while(j < b.n) {
                var prod = (a.p[i] as u64) * (b.p[j] as u64) + (x.p[i + j] as u64) + carry
                x.p[i + j] = (prod & 0xFFFFFFFFu64) as u32
                carry = prod >> 32; j += 1
            }
            if(carry > 0) { x.p[i + b.n] = carry as u32 }
            i += 1
        }
        x.n = a.n + b.n; x.s = a.s * b.s; mpi_trim(x); return 0
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
        var A : Mpi; mpi_init(&raw mut A); mpi_copy(&raw mut A, a); A.s = 1
        var B : Mpi; mpi_init(&raw mut B); mpi_copy(&raw mut B, b); B.s = 1

        if(mpi_cmp_abs(&raw mut A, &raw mut B) < 0) {
            if(q != null) { mpi_lset(q, 0) }
            if(r != null) { mpi_copy(r, &raw mut A); r.s = a.s }
            return 0
        }

        var Q : Mpi; mpi_init(&raw mut Q)
        var a_bits = mpi_bitlen(&raw mut A)
        var b_bits = mpi_bitlen(&raw mut B)
        var shift = a_bits - b_bits

        // Shift B left by 'shift' bits
        var ret = mpi_grow(&raw mut B, B.n + (shift / BITS_PER_LIMB) + 2)
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
        while(cur_shift > 0 || mpi_cmp_abs(&raw mut A, &raw mut B) >= 0) {
            if(mpi_cmp_abs(&raw mut A, &raw mut B) >= 0) {
                mpi_sub_abs(&raw mut A, &raw mut A, &raw mut B)
                // Set the quotient bit at position cur_shift
                var limb_idx = cur_shift / BITS_PER_LIMB
                var bit_idx = cur_shift % BITS_PER_LIMB
                if(Q.n <= limb_idx) { mpi_grow(&raw mut Q, limb_idx + 1); Q.n = limb_idx + 1 }
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

        mpi_trim(&raw mut Q); Q.s = sign
        if(q != null) { mpi_copy(q, &raw mut Q) }
        if(r != null) { mpi_trim(&raw mut A); A.s = a.s; mpi_copy(r, &raw mut A) }
        return 0
    }

    public func mpi_mod(r : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        return mpi_div(null, r, a, b)
    }

    // ─── Montgomery Modular Exponentiation ───────────────────────────────

    func montgomery_mul(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi, n : *mut Mpi, n_inv0 : u32) : int {
        // Need 2*n + 1 limbs for intermediate accumulation
        var work_limbs = n.n * 2 + 1
        var ret = mpi_grow(x, work_limbs)
        if(ret < 0) { return ret }
        var i : size_t = 0
        while(i < work_limbs) { x.p[i] = 0; i += 1 }; x.n = work_limbs

        i = 0
        while(i < n.n) {
            var ai : u32 = 0
            if(i < a.n) { ai = a.p[i] }
            var u_val = (x.p[0] as u64) + (ai as u64) * (b.p[0] as u64)
            u_val = u_val * (n_inv0 as u64)
            var carry : u64 = 0
            var j : size_t = 0
            while(j < n.n) {
                var bj : u32 = 0; if(j < b.n) { bj = b.p[j] }
                var prod = (ai as u64) * (bj as u64) + (u_val & 0xFFFFFFFFu64) * (n.p[j] as u64) + (x.p[i + j] as u64) + carry
                x.p[i + j] = (prod & 0xFFFFFFFFu64) as u32
                carry = prod >> 32; j += 1
            }
            var k : size_t = i + n.n
            while(carry > 0) {
                var sum = (x.p[k] as u64) + carry
                x.p[k] = (sum & 0xFFFFFFFFu64) as u32
                carry = sum >> 32; k += 1
            }
            i += 1
        }
        if(mpi_cmp_abs(x, n) >= 0) { mpi_sub_abs(x, x, n) }
        mpi_trim(x); return 0
    }

    func to_montgomery(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi, n_inv0 : u32, r2 : *mut Mpi) : int {
        return montgomery_mul(x, a, r2, n, n_inv0)
    }

    func from_montgomery(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi, n_inv0 : u32) : int {
        var one : Mpi; mpi_init(&raw mut one); mpi_lset(&raw mut one, 1)
        return montgomery_mul(x, a, &raw mut one, n, n_inv0)
    }

    func compute_r2(r2 : *mut Mpi, n : *mut Mpi) : int {
        var R : Mpi; mpi_init(&raw mut R)
        var ret = mpi_grow(&raw mut R, n.n + 1)
        if(ret < 0) { return ret }
        R.p[n.n] = 1; R.n = n.n + 1
        var R_mod : Mpi; mpi_init(&raw mut R_mod)
        ret = mpi_mod(&raw mut R_mod, &raw mut R, n)
        if(ret < 0) { return ret }
        ret = mpi_mul(r2, &raw mut R_mod, &raw mut R_mod)
        if(ret < 0) { return ret }
        ret = mpi_mod(r2, r2, n)
        return ret
    }

    public func mpi_exp_mod(x : *mut Mpi, a : *mut Mpi, e : *mut Mpi, n : *mut Mpi) : int {
        if(mpi_cmp_int(n, 0) <= 0 || (n.p[0] & 1) == 0) {
            return ERR_MPI_BAD_INPUT_DATA
        }
        mpi_trim(e)
        if(e.n == 0) { mpi_lset(x, 1); return 0 }

        // Precompute R^2 mod N
        var r2 : Mpi; mpi_init(&raw mut r2)
        var ret = compute_r2(&raw mut r2, n)
        if(ret < 0) { return ret }

        // Compute n_inv0 = -n0^-1 mod 2^32
        var n0 = n.p[0]
        var x0 : u32 = 2
        var cur : u32 = (n0 * x0) & 0xFFFFFFFFu32
        var limit : size_t = 0
        while(cur != 1) {
            var t = 0u32 - cur
            x0 = x0 * t
            cur = (n0 * x0) & 0xFFFFFFFFu32
            limit += 1; if(limit > 32) { break }
        }
        var n_inv0 = 0u32 - x0

        // Convert A to Montgomery representation
        var A_mont : Mpi; mpi_init(&raw mut A_mont)
        ret = to_montgomery(&raw mut A_mont, a, n, n_inv0, &raw mut r2)
        if(ret < 0) { return ret }

        // Start with Montgomery form of 1 (which is R mod N)
        var result : Mpi; mpi_init(&raw mut result)
        var one : Mpi; mpi_init(&raw mut one); mpi_lset(&raw mut one, 1)
        ret = to_montgomery(&raw mut result, &raw mut one, n, n_inv0, &raw mut r2)
        if(ret < 0) { return ret }

        // Left-to-right binary exponentiation
        var bitlen = mpi_bitlen(e)
        var i = bitlen
        while(i > 0) {
            i -= 1
            ret = montgomery_mul(&raw mut result, &raw mut result, &raw mut result, n, n_inv0)
            if(ret < 0) { return ret }
            var limb_idx = i / BITS_PER_LIMB
            var bit_idx = i % BITS_PER_LIMB
            if(e.p[limb_idx] & (1u32 << bit_idx)) {
                ret = montgomery_mul(&raw mut result, &raw mut result, &raw mut A_mont, n, n_inv0)
                if(ret < 0) { return ret }
            }
        }

        // Convert back
        ret = from_montgomery(x, &raw mut result, n, n_inv0)
        if(ret < 0) { return ret }

        // Handle negative base with odd exponent
        if(a.s < 0 && (e.p[0] & 1) && !mpi_is_zero(x)) {
            mpi_sub(x, n, x); x.s = -1
        }
        return 0
    }

    // ─── Modular Inverse (Binary Extended GCD) ───────────────────────────

    public func mpi_mod_inv(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi) : int {
        if(mpi_cmp_int(n, 1) <= 0) { return ERR_MPI_BAD_INPUT_DATA }
        var A : Mpi; mpi_init(&raw mut A)
        var B : Mpi; mpi_init(&raw mut B)
        var U : Mpi; mpi_init(&raw mut U)
        var V : Mpi; mpi_init(&raw mut V)

        mpi_mod(&raw mut A, a, n)
        mpi_copy(&raw mut B, n)
        mpi_lset(&raw mut U, 1); mpi_lset(&raw mut V, 0)

        while(!mpi_is_zero(&raw mut A)) {
            if((A.p[0] & 1) == 0) {
                var ret = mpi_shift_r(&raw mut A, 1)
                if(ret < 0) { return ret }
                if((U.p[0] & 1) != 0) { var ret2 = mpi_add(&raw mut U, &raw mut U, n); if(ret2 < 0) { return ret2 } }
                var ret3 = mpi_shift_r(&raw mut U, 1)
                if(ret3 < 0) { return ret3 }
            } else if((B.p[0] & 1) == 0) {
                var ret = mpi_shift_r(&raw mut B, 1)
                if(ret < 0) { return ret }
                if((V.p[0] & 1) != 0) { var ret2 = mpi_add(&raw mut V, &raw mut V, n); if(ret2 < 0) { return ret2 } }
                var ret3 = mpi_shift_r(&raw mut V, 1)
                if(ret3 < 0) { return ret3 }
            } else {
                if(mpi_cmp_abs(&raw mut A, &raw mut B) >= 0) {
                    var ret = mpi_sub_abs(&raw mut A, &raw mut A, &raw mut B)
                    if(ret < 0) { return ret }
                    if(mpi_cmp(&raw mut U, &raw mut V) < 0) { var ret2 = mpi_add(&raw mut U, &raw mut U, n); if(ret2 < 0) { return ret2 } }
                    var ret3 = mpi_sub(&raw mut U, &raw mut U, &raw mut V)
                    if(ret3 < 0) { return ret3 }
                } else {
                    var ret = mpi_sub_abs(&raw mut B, &raw mut B, &raw mut A)
                    if(ret < 0) { return ret }
                    if(mpi_cmp(&raw mut V, &raw mut U) < 0) { var ret2 = mpi_add(&raw mut V, &raw mut V, n); if(ret2 < 0) { return ret2 } }
                    var ret3 = mpi_sub(&raw mut V, &raw mut V, &raw mut U)
                    if(ret3 < 0) { return ret3 }
                }
            }
        }
        if(mpi_cmp_int(&raw mut B, 1) != 0) { return ERR_MPI_BAD_INPUT_DATA }
        mpi_mod(x, &raw mut V, n)
        return 0
    }

    // ─── Shift Operations ──────────────────────────────────────────────

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
        var i : size_t = 0
        while(i < size) {
            var byte_pos = size - 1 - i
            var limb = i / 4; var offset = (i % 4) * 8
            buf[byte_pos] = ((m.p[limb] >> offset) & 0xFFu32) as u8
            i += 1
        }
        i = size
        while(i < buflen) { buf[buflen - 1 - i] = 0; i += 1 }
        return 0
    }

    public func mpi_gcd(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        var A : Mpi; mpi_init(&raw mut A); mpi_copy(&raw mut A, a)
        var B : Mpi; mpi_init(&raw mut B); mpi_copy(&raw mut B, b)
        while(!mpi_is_zero(&raw mut B)) {
            var T : Mpi; mpi_init(&raw mut T)
            mpi_mod(&raw mut T, &raw mut A, &raw mut B)
            mpi_copy(&raw mut A, &raw mut B)
            mpi_copy(&raw mut B, &raw mut T)
        }
        mpi_copy(x, &raw mut A); x.s = 1; return 0
    }

} // namespace tls
