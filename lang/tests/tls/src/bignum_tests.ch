// ============================================================================
// Bignum / RSA arithmetic tests
// ============================================================================
// The bignum in bignum.ch underpins every RSA and ECDSA operation, and it now
// carries code paths that hand-written vectors cannot reach by accident:
// Knuth Algorithm D long division (normalisation shift, qhat refinement,
// quotient-limb loop), the Montgomery-form exponentiation with its even-modulus
// fallback, the binary extended GCD, and RSA CRT recombination.
//
// These tests pin that behaviour three ways:
//
//   1. Known-answer vectors, every one of them cross-checked against Python's
//      arbitrary-precision integers. Hermetic: no Python needed at run time.
//   2. Algebraic invariants over random inputs (q*b + r == a, a*x mod n == 1,
//      exp_mod == repeated multiplication) which catch sign, aliasing and range
//      bugs that fixed vectors miss.
//   3. Live differential tests against Python (the INT_* functions), where the
//      reference is an independent implementation rather than a frozen value.
//
// Regression context: mpi_div used to walk one quotient BIT per iteration and
// re-shift the divisor each time, so one 2048 % 1024 reduction cost 464us
// against 20us for a whole 1024x1024 multiply. That single reduction dominates
// compute_r2 (R^2 mod N), which mpi_exp_mod runs on every call, which in turn
// made an RSA public operation ~7x slower than it needed to be. The vectors
// below pin the replacement limb-at-a-time divider against Python.
// ============================================================================

using namespace tls
using std::string_view
using std::vector

// ─── Hex helpers ───────────────────────────────────────────────────────────

// Parse a big-endian hex string into bytes. An odd digit count is aligned to
// the low nibble ("abc" == 0x0abc). Returns the byte count, or 0 if the value
// does not fit in out_max bytes.
func bt_hex_to_bytes(hex : *char, out : *mut u8, out_max : size_t) : size_t {
    var len : size_t = 0
    while(hex[len] != 0) { len += 1 }
    var nbytes = (len + 1) / 2
    if(nbytes == 0 || nbytes > out_max) { return 0 } else {}

    var i : size_t = 0
    while(i < nbytes) { out[i] = 0; i += 1 }

    i = 0
    while(i < len) {
        var digit = test_hex_char_val(hex[len - 1 - i])
        var byte_idx = nbytes - 1 - (i / 2)
        if((i % 2) == 0) { out[byte_idx] = digit as u8 }
        else { out[byte_idx] = out[byte_idx] | ((digit << 4) as u8) }
        i += 1
    }
    return nbytes
}

func bt_hex_to_mpi(hex : *char, m : *mut Mpi) : bool {
    var buf : [512]u8
    var n = bt_hex_to_bytes(hex, &raw mut buf[0], 512)
    if(n == 0) { return false } else {}
    mpi_read_binary(m, &raw mut buf[0], n)
    return true
}

// True when m's magnitude equals the hex string (exactly: no leading zeros are
// assumed in the hex, and a value that needs more bytes than the hex encodes
// fails).
func bt_mpi_eq_hex(m : *mut Mpi, hex : *char) : bool {
    var expect : [512]u8
    var n = bt_hex_to_bytes(hex, &raw mut expect[0], 512)
    if(n == 0) { return false } else {}
    var got : [512]u8
    var i : size_t = 0
    while(i < n) { got[i] = 0; i += 1 }
    if(mpi_write_binary(m, &raw mut got[0], n) < 0) { return false } else {}
    return test_bytes_eq(&raw got[0], &raw expect[0], n)
}

func bt_random_mpi(m : *mut Mpi, nbytes : size_t, top : u8) {
    var buf : [256]u8
    test_random_bytes(&raw mut buf[0], nbytes)
    buf[0] = top
    mpi_read_binary(m, &raw mut buf[0], nbytes)
}

// ─── Known-answer vectors ──────────────────────────────────────────────────

// a / b == q with remainder r, checked both against the vector and against the
// defining identity q*b + r == a. neg = 1 makes the dividend negative, which
// must produce a negative quotient and remainder (truncation toward zero).
func bt_div_check(env : &mut TestEnv, a_hex : *char, b_hex : *char,
                  q_hex : *char, r_hex : *char, neg : int) {
    var A : Mpi; mpi_init(&raw mut A)
    var B : Mpi; mpi_init(&raw mut B)
    if(!bt_hex_to_mpi(a_hex, &raw mut A)) { env.error("vector a did not parse"); return } else {}
    if(!bt_hex_to_mpi(b_hex, &raw mut B)) { env.error("vector b did not parse"); return } else {}
    if(neg == 1) { A.s = -1 }

    var Q : Mpi; mpi_init(&raw mut Q)
    var R : Mpi; mpi_init(&raw mut R)
    var ret = mpi_div(&raw mut Q, &raw mut R, &raw mut A, &raw mut B)
    if(ret != 0) { env.error("mpi_div returned an error"); return } else {}
    if(!bt_mpi_eq_hex(&raw mut Q, q_hex)) { env.error("quotient mismatch"); return } else {}
    if(!bt_mpi_eq_hex(&raw mut R, r_hex)) { env.error("remainder mismatch"); return } else {}

    var prod : Mpi; mpi_init(&raw mut prod)
    var sum : Mpi; mpi_init(&raw mut sum)
    mpi_mul(&raw mut prod, &raw mut Q, &raw mut B)
    mpi_add(&raw mut sum, &raw mut prod, &raw mut R)
    if(mpi_cmp(&raw mut sum, &raw mut A) != 0) { env.error("q*b + r != a"); return } else {}
}

@test
public func TEST_bignum_div_known_answers(env : &mut TestEnv) {
    // The dividend and divisor each have a leading zero byte, so their limb
    // counts are below their byte widths — the divider must work from limbs,
    // not byte lengths.
    bt_div_check(env, "10000000000000000", "200000000", "80000000", "0", 0)
    bt_div_check(env, "1", "1", "1", "0", 0)

    // Single-limb divisor.
    bt_div_check(env, "2e84496e7857dd86", "ba6f875d", "3fdf99be", "a4a7d180", 0)

    // Divisor top limb 0x00000001: the worst case for the qhat estimate,
    // because normalisation shifts the divisor a full 31 bits.
    bt_div_check(env, "e8fb90d7b938451ee325faa633406bc44dc2a627940eee3c", "1c1d8fac1",
                 "84960f07b8abf2d09a29209349a424ab5c259465", "e976c17", 0)

    // Divisor top bit already set: no normalisation shift at all.
    bt_div_check(env, "a2da95a83ec33dd6887e840043e58844c2354e2bb7740a63", "de1c6babd0055979",
                 "bbb39972a596e8a75206e28d5e941c04", "b1818124b9a1687f", 0)

    // Dividend < divisor: quotient 0, remainder is the dividend itself. This is
    // the early-return path that mpi_mod relies on to leave small values alone.
    bt_div_check(env, "123456789abcdef", "fedcba98765432101234567890", "0", "123456789abcdef", 0)

    // Dividend == divisor.
    bt_div_check(env, "ba4e6c3686ff0de26a7698065aab0a377f90ade7",
                 "ba4e6c3686ff0de26a7698065aab0a377f90ade7", "1", "0", 0)

    // Exact multiple: a multi-limb quotient with a zero remainder. Also pins the
    // canonical sign of a zero remainder (see mpi_div's zero-sign note).
    bt_div_check(env, "ebcadf8d5526e3067553010c4620ba224b1925ae1c5e0b13fee7677a",
                 "cf3d4e7b37d72e4af69787709d9b532a", "123456789abcdef0123456789", "0", 0)

    // A full-width quotient limb right at the u64 estimate boundary.
    bt_div_check(env, "b43d4317f6ff4f5bffffffff", "b43d43188b389064", "ffffffff",
                 "200402108b389063", 0)

    // 1280-bit dividend against a 640-bit divisor (40 and 20 limbs) — the
    // largest shape anything in the library uses.
    bt_div_check(env, "b37e6853cb2d34ea5865585254b1070f63eb18aa53bdf64dc34797c42393446abe564059b9ae5c8f1fca7da27744001a6aa45fe0a0f09780597538cbc54be01c0ef8e010faaced226972f683de11ee00366dadc088177abd25fbab1ba70b967adf354788d4dd79d3b5834f4cecb736d877f1caf0ba49c19fc0a9c8beb070e38434d57084ddfa7fa4ffe9ec11c63d5f77bb3a6a06131db61884f42b4b548a84a5",
                 "d80dad424245dc03dd8ba9d4f897181304f8c31cc30d0ea339a7ff57bffa07750a5d5bfe345986d33aa67f52682f860ede282d59e7b0dfa436cc71a5915405c0510323696a1510446d4337155352d63f",
                 "d4ae468fdf0c7003264bff735957dcae04e1d0cf1027e4c145dae9f646ab1b03c26292d48f4bc8e2e8cdf9d182679aabfb39f48c48f7353cf7142b56654ca1f657b4f71fa25d52306047ce351c5698a4",
                 "95e99105107aa836ec4583e5d9104105cca8e41bbf3b9a00e73b1bc3151be2b5d5a00d7e31498e0b285d651ffece42ed1eea3a4f5abe24d5b72ff625a290c96edf02ae46d294842fd2e31ddae719dc49", 0)

    // All-ones operands: (2^512-1) / (2^256-1) == 2^256 + 1 exactly.
    bt_div_check(env, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
                 "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
                 "10000000000000000000000000000000000000000000000000000000000000001", "0", 0)

    // Negative dividend: truncating division gives a negative quotient and a
    // negative remainder.
    bt_div_check(env, "cdebefa791f68f88f5d93c67c5bc543b51b8a7af8171a4c3",
                 "9d1a8c836bc9e141b4852afb", "14f8c741e81272c6f9abdc02d",
                 "213eb3dcf1d29ebe2c7dd6a4", 1)
    bt_div_check(env, "123579be02468abe013579bdf789", "1001",
                 "123456789abcdef0123456789", "0", 1)
}

@test
public func TEST_bignum_div_aliases_and_nulls(env : &mut TestEnv) {
    var A : Mpi; mpi_init(&raw mut A)
    var B : Mpi; mpi_init(&raw mut B)
    bt_random_mpi(&raw mut A, 64, 0x9C)
    bt_random_mpi(&raw mut B, 32, 0x81)

    // The quotient and remainder written over their own inputs. mpi_mod relies
    // on the r == a form (mpi_mod(x, x, n)), which is how the Montgomery setup
    // and the Miller-Rabin squaring loop call it.
    var A2 : Mpi; mpi_init(&raw mut A2); mpi_copy(&raw mut A2, &raw mut A)
    var B2 : Mpi; mpi_init(&raw mut B2); mpi_copy(&raw mut B2, &raw mut B)
    var ret = mpi_div(&raw mut A2, &raw mut B2, &raw mut A, &raw mut B)
    if(ret != 0) { env.error("aliased mpi_div failed"); return } else {}
    var prod : Mpi; mpi_init(&raw mut prod)
    var sum : Mpi; mpi_init(&raw mut sum)
    mpi_mul(&raw mut prod, &raw mut A2, &raw mut B)
    mpi_add(&raw mut sum, &raw mut prod, &raw mut B2)
    if(mpi_cmp(&raw mut sum, &raw mut A) != 0) { env.error("aliased division broke q*b + r == a"); return } else {}

    // Null quotient and null remainder are both legal.
    if(mpi_div(null, null, &raw mut A, &raw mut B) != 0) { env.error("null q/r rejected"); return } else {}
    if(mpi_div(null, &raw mut B2, &raw mut A, &raw mut B) != 0) { env.error("null q rejected"); return } else {}
    if(mpi_div(&raw mut A2, null, &raw mut A, &raw mut B) != 0) { env.error("null r rejected"); return } else {}

    // Division by zero.
    var zero : Mpi; mpi_init(&raw mut zero)
    if(mpi_div(&raw mut A2, &raw mut B2, &raw mut A, &raw mut zero) != ERR_MPI_DIVISION_BY_ZERO) {
        env.error("division by zero must return ERR_MPI_DIVISION_BY_ZERO"); return
    } else {}

    // A zero dividend is well defined.
    if(mpi_div(&raw mut A2, &raw mut B2, &raw mut zero, &raw mut B) != 0) {
        env.error("zero dividend rejected"); return
    } else {}
    if(!mpi_is_zero(&raw mut A2) || !mpi_is_zero(&raw mut B2)) {
        env.error("zero dividend must give q=0 and r=0"); return
    } else {}
}

// ─── Invariants over random inputs ─────────────────────────────────────────

@test
public func TEST_bignum_div_mod_invariants(env : &mut TestEnv) {
    var round : size_t = 0
    while(round < 150) {
        // 1..40 limbs of dividend, 1..20 limbs of divisor.
        var bbytes = 4 * (1 + (round % 20))
        var abytes = 4 * (1 + ((round * 7) % 40))
        var A : Mpi; mpi_init(&raw mut A)
        var B : Mpi; mpi_init(&raw mut B)
        bt_random_mpi(&raw mut A, abytes, 0x8F)
        bt_random_mpi(&raw mut B, bbytes, 0x03)

        var Q : Mpi; mpi_init(&raw mut Q)
        var R : Mpi; mpi_init(&raw mut R)
        if(mpi_div(&raw mut Q, &raw mut R, &raw mut A, &raw mut B) != 0) {
            env.error("mpi_div failed on random input"); return
        } else {}

        var prod : Mpi; mpi_init(&raw mut prod)
        var sum : Mpi; mpi_init(&raw mut sum)
        mpi_mul(&raw mut prod, &raw mut Q, &raw mut B)
        mpi_add(&raw mut sum, &raw mut prod, &raw mut R)
        if(mpi_cmp(&raw mut sum, &raw mut A) != 0) { env.error("q*b + r != a"); return } else {}
        if(R.s < 0) { env.error("remainder must be non-negative here"); return } else {}
        if(mpi_cmp(&raw mut R, &raw mut B) >= 0) { env.error("remainder must be < divisor"); return } else {}

        // mpi_mod returns the non-negative residue, including for negative
        // dividends, and must stay in [0, b).
        var neg : Mpi; mpi_init(&raw mut neg); mpi_copy(&raw mut neg, &raw mut A); neg.s = -1
        var modv : Mpi; mpi_init(&raw mut modv)
        if(mpi_mod(&raw mut modv, &raw mut neg, &raw mut B) != 0) {
            env.error("mpi_mod failed"); return
        } else {}
        if(modv.s < 0 || mpi_cmp(&raw mut modv, &raw mut B) >= 0) {
            env.error("mpi_mod result out of [0, b)"); return
        } else {}
        // a + (-a mod b) must be a multiple of b.
        var chk : Mpi; mpi_init(&raw mut chk)
        var rem : Mpi; mpi_init(&raw mut rem)
        mpi_add(&raw mut chk, &raw mut A, &raw mut modv)
        if(mpi_mod(&raw mut rem, &raw mut chk, &raw mut B) != 0) {
            env.error("mpi_mod failed on the check"); return
        } else {}
        if(!mpi_is_zero(&raw mut rem)) { env.error("a + (-a mod b) is not a multiple of b"); return } else {}

        round += 1
    }
}

// mpi_mod_small is the trial-division fast path used by RSA prime generation.
// It must agree with the generic reduction's low limb for every divisor shape.
@test
public func TEST_bignum_mod_small_matches_generic(env : &mut TestEnv) {
    var divisors : [8]u32 = [3 as u32, 7 as u32, 17 as u32, 997 as u32, 65521 as u32,
                             65537 as u32, 0x80000000u32, 0xFFFFFFFFu32]
    var d : size_t = 0
    while(d < 8) {
        var A : Mpi; mpi_init(&raw mut A)
        bt_random_mpi(&raw mut A, 128, 0xB3)

        var small = mpi_mod_small(&raw mut A, divisors[d])

        var D : Mpi; mpi_init(&raw mut D)
        mpi_lset(&raw mut D, divisors[d] as i64)
        var R : Mpi; mpi_init(&raw mut R)
        if(mpi_mod(&raw mut R, &raw mut A, &raw mut D) != 0) {
            env.error("mpi_mod failed in the mod_small cross-check"); return
        } else {}
        if(mpi_cmp_int(&raw mut R, small as i64) != 0) {
            env.error("mpi_mod_small disagrees with mpi_mod"); return
        } else {}

        // Small values, including 0 and 1, must reduce correctly too.
        var small_vals : [4]i64 = [0 as i64, 1 as i64, 2 as i64, 4294967295 as i64]
        var s : size_t = 0
        while(s < 4) {
            var V : Mpi; mpi_init(&raw mut V); mpi_lset(&raw mut V, small_vals[s])
            var want = (small_vals[s] % (divisors[d] as i64)) as u32
            if(mpi_mod_small(&raw mut V, divisors[d]) != want) {
                env.error("mpi_mod_small wrong for a small value"); return
            } else {}
            s += 1
        }
        d += 1
    }

    // Documented contract: a zero divisor is not used by the library (the
    // small-prime sieve only feeds primes) and yields 0 rather than trapping.
    var A : Mpi; mpi_init(&raw mut A)
    bt_random_mpi(&raw mut A, 32, 0x91)
    if(mpi_mod_small(&raw mut A, 0u32) != 0u32) {
        env.error("mpi_mod_small(_, 0) should be defined as 0"); return
    } else {}
}

// ─── Exponentiation ───────────────────────────────────────────────────────

// The reference is built from mpi_mul + mpi_mod, so this does not depend on any
// frozen constant: it checks the Montgomery path against plain arithmetic.
func bt_exp_reference(out : *mut Mpi, a : *mut Mpi, e : u32, n : *mut Mpi) {
    mpi_lset(out, 1)
    var i : u32 = 0
    var tmp : Mpi; mpi_init(unsafe(&raw mut tmp))
    while(i < e) {
        mpi_mul(unsafe(&raw mut tmp), out, a)
        mpi_mod(out, unsafe(&raw mut tmp), n)
        i += 1
    }
}

@test
public func TEST_bignum_exp_mod_matches_repeated_multiplication(env : &mut TestEnv) {
    var round : size_t = 0
    while(round < 40) {
        // Both parities of modulus: odd takes the Montgomery path, even takes
        // the mpi_exp_mod_fallback path.
        var nbytes : size_t = 4 + (round % 4) * 4
        var A : Mpi; mpi_init(&raw mut A)
        var N : Mpi; mpi_init(&raw mut N)
        bt_random_mpi(&raw mut A, nbytes, 0x37)
        bt_random_mpi(&raw mut N, nbytes, 0x80)
        if((round % 2) == 1) { N.p[0] = N.p[0] & 0xFFFFFFFEu32 }  // force even

        var e : u32 = 1 + ((round * 5) % 40)
        var E : Mpi; mpi_init(&raw mut E); mpi_lset(&raw mut E, e as i64)

        var got : Mpi; mpi_init(&raw mut got)
        if(mpi_exp_mod(&raw mut got, &raw mut A, &raw mut E, &raw mut N) != 0) {
            env.error("mpi_exp_mod failed"); return
        } else {}

        var want : Mpi; mpi_init(&raw mut want)
        bt_exp_reference(&raw mut want, &raw mut A, e, &raw mut N)
        if(mpi_cmp(&raw mut got, &raw mut want) != 0) {
            env.error("mpi_exp_mod != repeated multiplication"); return
        } else {}

        // Aliasing the base with the result, as mpi_is_prime does when it
        // re-squares in place.
        var alias : Mpi; mpi_init(&raw mut alias); mpi_copy(&raw mut alias, &raw mut A)
        if(mpi_exp_mod(&raw mut alias, &raw mut alias, &raw mut E, &raw mut N) != 0) {
            env.error("mpi_exp_mod with a == x failed"); return
        } else {}
        if(mpi_cmp(&raw mut alias, &raw mut want) != 0) {
            env.error("mpi_exp_mod with a == x gave a different answer"); return
        } else {}

        round += 1
    }
}

@test
public func TEST_bignum_exp_mod_edge_cases(env : &mut TestEnv) {
    var N : Mpi; mpi_init(&raw mut N)
    var A : Mpi; mpi_init(&raw mut A)
    var E : Mpi; mpi_init(&raw mut E)
    var X : Mpi; mpi_init(&raw mut X)

    bt_hex_to_mpi("1c1d8fac1b1e4f9a3d5c7e9b1d3f5a7c9e1b3d5f7a9c1e3b5d7f9a1c3e5b7d91", &raw mut N)
    // Top byte 0x0A keeps the base just below the modulus (N starts 0x1c), the
    // shape every real caller passes.
    bt_random_mpi(&raw mut A, 32, 0x0A)

    // e == 0 is 1 mod n (and 0 when n == 1).
    mpi_lset(&raw mut E, 0)
    if(mpi_exp_mod(&raw mut X, &raw mut A, &raw mut E, &raw mut N) != 0) { env.error("e=0 failed"); return } else {}
    if(mpi_cmp_int(&raw mut X, 1) != 0) { env.error("a^0 mod n != 1"); return } else {}

    // e == 1 is a mod n.
    mpi_lset(&raw mut E, 1)
    if(mpi_exp_mod(&raw mut X, &raw mut A, &raw mut E, &raw mut N) != 0) { env.error("e=1 failed"); return } else {}
    if(mpi_cmp(&raw mut X, &raw mut A) != 0) { env.error("a^1 mod n != a"); return } else {}

    // A base of zero is zero for any positive exponent.
    var zero : Mpi; mpi_init(&raw mut zero)
    mpi_lset(&raw mut E, 5)
    if(mpi_exp_mod(&raw mut X, &raw mut zero, &raw mut E, &raw mut N) != 0) { env.error("0^5 failed"); return } else {}
    if(!mpi_is_zero(&raw mut X)) { env.error("0^e mod n != 0"); return } else {}

    // Modulus 1 collapses everything to zero.
    var one : Mpi; mpi_init(&raw mut one); mpi_lset(&raw mut one, 1)
    if(mpi_exp_mod(&raw mut X, &raw mut A, &raw mut E, &raw mut one) != 0) { env.error("mod 1 failed"); return } else {}
    if(!mpi_is_zero(&raw mut X)) { env.error("a^e mod 1 != 0"); return } else {}

    // A non-positive modulus is rejected.
    var bad : Mpi; mpi_init(&raw mut bad)
    if(mpi_exp_mod(&raw mut X, &raw mut A, &raw mut E, &raw mut bad) != ERR_MPI_BAD_INPUT_DATA) {
        env.error("modulus 0 must be rejected"); return
    } else {}

    // A base larger than the modulus must be reduced, not truncated to the
    // modulus' limb count.
    var big : Mpi; mpi_init(&raw mut big)
    var prod : Mpi; mpi_init(&raw mut prod)
    mpi_lset(&raw mut prod, 7)
    mpi_add(&raw mut big, &raw mut N, &raw mut prod)    // big = N + 7
    mpi_lset(&raw mut E, 1)
    if(mpi_exp_mod(&raw mut X, &raw mut big, &raw mut E, &raw mut N) != 0) { env.error("a > n failed"); return } else {}
    if(mpi_cmp_int(&raw mut X, 7) != 0) { env.error("(n+7)^1 mod n != 7"); return } else {}
}

// ─── Modular inverse ─────────────────────────────────────────────────────

@test
public func TEST_bignum_mod_inv_invariants(env : &mut TestEnv) {
    var round : size_t = 0
    while(round < 40) {
        // Odd modulus (binary extended GCD) and even modulus (Euclidean loop).
        var A : Mpi; mpi_init(&raw mut A)
        var N : Mpi; mpi_init(&raw mut N)
        bt_random_mpi(&raw mut A, 32, 0x3B)
        bt_random_mpi(&raw mut N, 32, 0x81)
        N.p[0] = (N.p[0] | 1u32)
        if((round % 4) == 3) { N.p[0] = N.p[0] & 0xFFFFFFFEu32 }

        var X : Mpi; mpi_init(&raw mut X)
        var ret = mpi_mod_inv(&raw mut X, &raw mut A, &raw mut N)
        if(ret == 0) {
            var chk : Mpi; mpi_init(&raw mut chk)
            var one : Mpi; mpi_init(&raw mut one); mpi_lset(&raw mut one, 1)
            mpi_mul(&raw mut chk, &raw mut X, &raw mut A)
            if(mpi_mod(&raw mut chk, &raw mut chk, &raw mut N) != 0) {
                env.error("inverse check modulo failed"); return
            } else {}
            if(mpi_cmp(&raw mut chk, &raw mut one) != 0) {
                env.error("x is not a^-1 mod n for a claimed inverse"); return
            } else {}
            if(X.s < 0 || mpi_cmp(&raw mut X, &raw mut N) >= 0) {
                env.error("inverse outside [0, n)"); return
            } else {}
        } else {
            // A rejection is only allowed when no inverse exists.
            var g : Mpi; mpi_init(&raw mut g)
            mpi_gcd(&raw mut g, &raw mut A, &raw mut N)
            if(mpi_cmp_int(&raw mut g, 1) == 0) {
                env.error("rejected an invertible value"); return
            } else {}
        }
        round += 1
    }

    // Fixed points and degenerate inputs.
    var N : Mpi; mpi_init(&raw mut N)
    bt_hex_to_mpi("fffffffffffffffffffffffffffffffeffffffffffffffff", &raw mut N)
    var X : Mpi; mpi_init(&raw mut X)

    var one : Mpi; mpi_init(&raw mut one); mpi_lset(&raw mut one, 1)
    if(mpi_mod_inv(&raw mut X, &raw mut one, &raw mut N) != 0) { env.error("inv(1) failed"); return } else {}
    if(mpi_cmp_int(&raw mut X, 1) != 0) { env.error("inv(1) != 1"); return } else {}

    // inv(n-1) == n-1, since (n-1)^2 == 1 mod n.
    var nm1 : Mpi; mpi_init(&raw mut nm1)
    mpi_sub(&raw mut nm1, &raw mut N, &raw mut one)
    if(mpi_mod_inv(&raw mut X, &raw mut nm1, &raw mut N) != 0) { env.error("inv(n-1) failed"); return } else {}
    if(mpi_cmp(&raw mut X, &raw mut nm1) != 0) { env.error("inv(n-1) != n-1"); return } else {}

    // No inverse for a shared factor.
    var three : Mpi; mpi_init(&raw mut three); mpi_lset(&raw mut three, 3)
    var nine : Mpi; mpi_init(&raw mut nine); mpi_lset(&raw mut nine, 9)
    if(mpi_mod_inv(&raw mut X, &raw mut three, &raw mut nine) == 0) {
        env.error("inv(3) mod 9 must fail"); return
    } else {}

    // Moduli <= 1 are rejected outright.
    if(mpi_mod_inv(&raw mut X, &raw mut one, &raw mut one) != ERR_MPI_BAD_INPUT_DATA) {
        env.error("modulus 1 must be rejected"); return
    } else {}
}

// ─── RSA CRT ──────────────────────────────────────────────────────────────

// The CRT private operation must agree bit for bit with the plain c^d mod N
// path. Building the reference by importing only (N, D) into a second context
// means the comparison is against the slow path in this same library, so a
// mistake in DP/DQ/QP consolidation or the (m1 - m2) sign handling shows up
// immediately.
@test
public func TEST_rsa_crt_decrypt_matches_plain(env : &mut TestEnv) {
    var crt : RSAContext; rsa_init(&raw mut crt, RSA_PKCS_V15, 0)
    if(rsa_gen_key(&raw mut crt, 1024, 65537) < 0) { env.error("rsa_gen_key failed"); return } else {}

    var msg : [32]u8
    test_random_bytes(&raw mut msg[0], 32)
    var ct : [128]u8
    if(rsa_pkcs1_encrypt(&raw mut crt, &raw mut msg[0], 32, &raw mut ct[0]) < 0) {
        env.error("rsa_pkcs1_encrypt failed"); return
    } else {}

    var nl = rsa_get_len(&raw mut crt)
    var dec_crt : [256]u8
    var dl_crt : size_t = 256
    if(rsa_pkcs1_decrypt(&raw mut crt, &raw mut ct[0], nl, &raw mut dec_crt[0], &raw mut dl_crt, 256) < 0) {
        env.error("CRT decrypt failed"); return
    } else {}
    if(dl_crt != 32 || !test_bytes_eq(&raw mut dec_crt[0], &raw mut msg[0], 32)) {
        env.error("CRT decrypt did not return the plaintext"); return
    } else {}

    // Same key, but with only N and D imported: no CRT parameters, so
    // rsa_private takes the plain exponentiation path.
    var plain : RSAContext; rsa_init(&raw mut plain, RSA_PKCS_V15, 0)
    var nbuf : [128]u8
    var dbuf : [128]u8
    mpi_write_binary(&raw mut crt.N, &raw mut nbuf[0], 128)
    mpi_write_binary(&raw mut crt.D, &raw mut dbuf[0], 128)
    if(rsa_import_privkey(&raw mut plain, &raw mut nbuf[0], 128, &raw mut dbuf[0], 128) < 0) {
        env.error("rsa_import_privkey failed"); return
    } else {}

    var dec_plain : [256]u8
    var dl_plain : size_t = 256
    if(rsa_pkcs1_decrypt(&raw mut plain, &raw mut ct[0], nl, &raw mut dec_plain[0], &raw mut dl_plain, 256) < 0) {
        env.error("plain decrypt failed"); return
    } else {}
    if(dl_plain != dl_crt || !test_bytes_eq(&raw mut dec_plain[0], &raw mut dec_crt[0], dl_crt)) {
        env.error("CRT decrypt disagrees with the plain c^d mod N path"); return
    } else {}

    // The CRT parameters themselves must be consistent, since a wrong
    // QP would silently corrupt decryption for some inputs.
    var p1 : Mpi; mpi_init(&raw mut p1)
    var q1 : Mpi; mpi_init(&raw mut q1)
    var one : Mpi; mpi_init(&raw mut one); mpi_lset(&raw mut one, 1)
    mpi_sub(&raw mut p1, &raw mut crt.P, &raw mut one)
    mpi_sub(&raw mut q1, &raw mut crt.Q, &raw mut one)
    var t : Mpi; mpi_init(&raw mut t)
    if(mpi_mod(&raw mut t, &raw mut crt.DP, &raw mut p1) != 0 || mpi_cmp(&raw mut t, &raw mut crt.DP) != 0) {
        env.error("DP != D mod (P-1)"); return
    } else {}
    if(mpi_mod(&raw mut t, &raw mut crt.DQ, &raw mut q1) != 0 || mpi_cmp(&raw mut t, &raw mut crt.DQ) != 0) {
        env.error("DQ != D mod (Q-1)"); return
    } else {}
    var qqp : Mpi; mpi_init(&raw mut qqp)
    mpi_mul(&raw mut qqp, &raw mut crt.QP, &raw mut crt.Q)
    if(mpi_mod(&raw mut qqp, &raw mut qqp, &raw mut crt.P) != 0) {
        env.error("QP check failed"); return
    } else {}
    if(mpi_cmp_int(&raw mut qqp, 1) != 0) { env.error("QP*Q != 1 mod P"); return } else {}

    rsa_free(&raw mut crt)
    rsa_free(&raw mut plain)
}

// ─── Live differential tests against Python ───────────────────────────────

func bt_push_str(buf : *mut u8, sp : *mut size_t, s : *char) {
    var i : size_t = 0
    while(s[i] != 0) { buf[*sp] = s[i] as u8; *sp += 1; i += 1 }
}

// Append a value as fixed-width big-endian hex (64 bytes worth) so the Python
// side can compare whole numbers without any width negotiation.
func bt_push_hex128(buf : *mut u8, sp : *mut size_t, m : *mut Mpi) {
    var tmp : [64]u8
    var i : size_t = 0
    while(i < 64) { tmp[i] = 0; i += 1 }
    mpi_write_binary(m, &raw mut tmp[0], 64)
    i = 0
    while(i < 64) {
        buf[*sp] = test_nibble_to_hex(((tmp[i] as uint) >> 4) & 0xF) as u8; *sp += 1
        buf[*sp] = test_nibble_to_hex((tmp[i] as uint) & 0xF) as u8; *sp += 1
        i += 1
    }
}

// Append a value as nbytes of big-endian hex. The width is explicit because an
// RSA modulus does not fit in the 64 bytes the other helpers assume.
func bt_push_hex_mpi(buf : *mut u8, sp : *mut size_t, m : *mut Mpi, nbytes : size_t) {
    var tmp : [256]u8
    var i : size_t = 0
    while(i < nbytes) { tmp[i] = 0; i += 1 }
    mpi_write_binary(m, &raw mut tmp[0], nbytes)
    bt_push_hex_bytes(buf, sp, &raw mut tmp[0], nbytes)
}

// Compare an Mpi against a Python-emitted fixed-width hex value.
func bt_check_py_hex_n(env : &mut TestEnv, out : *vector<u8>, label : string_view,
                       m : *mut Mpi, nbytes : size_t) : bool {
    var expect : [256]u8
    if(test_parse_py_hex_label(out, label, &raw mut expect[0], nbytes) != nbytes) {
        env.error("could not parse Python output"); return false
    } else {}
    var got : [256]u8
    var i : size_t = 0
    while(i < nbytes) { got[i] = 0; i += 1 }
    if(mpi_write_binary(m, &raw mut got[0], nbytes) < 0) {
        env.error("value does not fit the expected width"); return false
    } else {}
    if(!test_bytes_eq(&raw got[0], &raw expect[0], nbytes)) { return false } else {}
    return true
}

// Append raw bytes as lowercase hex (the Python snippets consume bytes.fromhex).
func bt_push_hex_bytes(buf : *mut u8, sp : *mut size_t, data : *mut u8, len : size_t) {
    var i : size_t = 0
    while(i < len) {
        buf[*sp] = test_nibble_to_hex(((data[i] as uint) >> 4) & 0xF) as u8; *sp += 1
        buf[*sp] = test_nibble_to_hex((data[i] as uint) & 0xF) as u8; *sp += 1
        i += 1
    }
}

// Compare an Mpi against a Python-emitted fixed-width hex value.
func bt_check_py_hex(env : &mut TestEnv, out : *vector<u8>, label : string_view, m : *mut Mpi) : bool {
    var expect : [64]u8
    if(test_parse_py_hex_label(out, label, &raw mut expect[0], 64) != 64) {
        env.error("could not parse Python output"); return false
    } else {}
    var got : [64]u8
    var i : size_t = 0
    while(i < 64) { got[i] = 0; i += 1 }
    if(mpi_write_binary(m, &raw mut got[0], 64) < 0) {
        env.error("value does not fit in 64 bytes"); return false
    } else {}
    if(!test_bytes_eq(&raw got[0], &raw expect[0], 64)) { return false } else {}
    return true
}

// 256-bit operands; Python computes q and r with divmod().
@test
public func INT_bignum_division_vs_python(env : &mut TestEnv) {
    var A : Mpi; mpi_init(&raw mut A)
    var B : Mpi; mpi_init(&raw mut B)
    bt_random_mpi(&raw mut A, 32, 0xF1)
    bt_random_mpi(&raw mut B, 16, 0x01)   // top limb 1: full 31-bit normalisation

    var script : [2048]u8
    var sp : size_t = 0
    bt_push_str(&raw mut script[0], &raw mut sp, "a=int('")
    bt_push_hex128(&raw mut script[0], &raw mut sp, &raw mut A)
    bt_push_str(&raw mut script[0], &raw mut sp, "',16)\nb=int('")
    bt_push_hex128(&raw mut script[0], &raw mut sp, &raw mut B)
    bt_push_str(&raw mut script[0], &raw mut sp,
        "',16)\nq,r=divmod(a,b)\nprint('Q=%0128x'%q)\nprint('R=%0128x'%r)\n")

    var out = test_python_run_script(&raw mut script[0], sp, string_view("bn_div.py"))
    if(out.size() == 0) { env.error("python produced no output"); return } else {}

    var Q : Mpi; mpi_init(&raw mut Q)
    var R : Mpi; mpi_init(&raw mut R)
    if(mpi_div(&raw mut Q, &raw mut R, &raw mut A, &raw mut B) != 0) {
        env.error("mpi_div failed"); return
    } else {}
    if(!bt_check_py_hex(env, &raw mut out, string_view("Q="), &raw mut Q)) {
        env.error("quotient mismatch vs Python"); return
    } else {}
    if(!bt_check_py_hex(env, &raw mut out, string_view("R="), &raw mut R)) {
        env.error("remainder mismatch vs Python"); return
    } else {}

    // The same reduction through mpi_mod, which is what the modular
    // exponentiation's Montgomery setup calls.
    var M : Mpi; mpi_init(&raw mut M)
    if(mpi_mod(&raw mut M, &raw mut A, &raw mut B) != 0) { env.error("mpi_mod failed"); return } else {}
    if(!bt_check_py_hex(env, &raw mut out, string_view("R="), &raw mut M)) {
        env.error("mpi_mod mismatch vs Python"); return
    } else {}
}

// 192-bit base and modulus, 64-bit exponent, compared against pow().
@test
public func INT_bignum_exp_mod_vs_python(env : &mut TestEnv) {
    var A : Mpi; mpi_init(&raw mut A)
    var N : Mpi; mpi_init(&raw mut N)
    var E : Mpi; mpi_init(&raw mut E)
    bt_random_mpi(&raw mut A, 24, 0x6D)
    bt_random_mpi(&raw mut N, 24, 0x80)
    bt_random_mpi(&raw mut E, 8, 0x01)
    N.p[0] = (N.p[0] | 1u32)   // odd: Montgomery path

    var script : [2048]u8
    var sp : size_t = 0
    bt_push_str(&raw mut script[0], &raw mut sp, "a=int('")
    bt_push_hex128(&raw mut script[0], &raw mut sp, &raw mut A)
    bt_push_str(&raw mut script[0], &raw mut sp, "',16)\ne=int('")
    bt_push_hex128(&raw mut script[0], &raw mut sp, &raw mut E)
    bt_push_str(&raw mut script[0], &raw mut sp, "',16)\nn=int('")
    bt_push_hex128(&raw mut script[0], &raw mut sp, &raw mut N)
    bt_push_str(&raw mut script[0], &raw mut sp, "',16)\nprint('X=%0128x'%pow(a,e,n))\n")

    var out = test_python_run_script(&raw mut script[0], sp, string_view("bn_exp.py"))
    if(out.size() == 0) { env.error("python produced no output"); return } else {}

    var X : Mpi; mpi_init(&raw mut X)
    if(mpi_exp_mod(&raw mut X, &raw mut A, &raw mut E, &raw mut N) != 0) {
        env.error("mpi_exp_mod failed"); return
    } else {}
    if(!bt_check_py_hex(env, &raw mut out, string_view("X="), &raw mut X)) {
        env.error("mpi_exp_mod mismatch vs Python pow()"); return
    } else {}
}

// Random 256-bit odd modulus, compared against Python's modular inverse.
@test
public func INT_bignum_mod_inv_vs_python(env : &mut TestEnv) {
    var A : Mpi; mpi_init(&raw mut A)
    var N : Mpi; mpi_init(&raw mut N)
    bt_random_mpi(&raw mut A, 32, 0x2F)
    bt_random_mpi(&raw mut N, 32, 0x80)
    N.p[0] = (N.p[0] | 1u32)
    // Make an inverse certain to exist and keep the value inside the modulus.
    var g : Mpi; mpi_init(&raw mut g)
    mpi_gcd(&raw mut g, &raw mut A, &raw mut N)
    if(mpi_cmp_int(&raw mut g, 1) != 0) {
        var one : Mpi; mpi_init(&raw mut one); mpi_lset(&raw mut one, 1)
        mpi_add(&raw mut A, &raw mut A, &raw mut one)
    } else {}
    var rem : Mpi; mpi_init(&raw mut rem)
    mpi_mod(&raw mut rem, &raw mut A, &raw mut N)
    mpi_copy(&raw mut A, &raw mut rem)

    var script : [2048]u8
    var sp : size_t = 0
    bt_push_str(&raw mut script[0], &raw mut sp, "a=int('")
    bt_push_hex128(&raw mut script[0], &raw mut sp, &raw mut A)
    bt_push_str(&raw mut script[0], &raw mut sp, "',16)\nn=int('")
    bt_push_hex128(&raw mut script[0], &raw mut sp, &raw mut N)
    bt_push_str(&raw mut script[0], &raw mut sp, "',16)\nprint('X=%0128x'%pow(a,-1,n))\n")

    var out = test_python_run_script(&raw mut script[0], sp, string_view("bn_inv.py"))
    if(out.size() == 0) { env.error("python produced no output"); return } else {}

    var X : Mpi; mpi_init(&raw mut X)
    if(mpi_mod_inv(&raw mut X, &raw mut A, &raw mut N) != 0) {
        env.error("mpi_mod_inv failed on a coprime pair"); return
    } else {}
    if(!bt_check_py_hex(env, &raw mut out, string_view("X="), &raw mut X)) {
        env.error("mpi_mod_inv mismatch vs Python"); return
    } else {}
}

// The CRT private operation, checked against Python computing c^d mod N without
// CRT. This is the strongest available check on the recombination: Python's
// reference shares no code with the path under test.
@test
public func INT_rsa_crt_vs_python(env : &mut TestEnv) {
    var ctx : RSAContext; rsa_init(&raw mut ctx, RSA_PKCS_V15, 0)
    if(rsa_gen_key(&raw mut ctx, 1024, 65537) < 0) { env.error("rsa_gen_key failed"); return } else {}

    var msg : [16]u8
    test_random_bytes(&raw mut msg[0], 16)
    var nl = rsa_get_len(&raw mut ctx)
    var ct : [128]u8
    if(rsa_pkcs1_encrypt(&raw mut ctx, &raw mut msg[0], 16, &raw mut ct[0]) < 0) {
        env.error("rsa_pkcs1_encrypt failed"); return
    } else {}

    var dec : [256]u8
    var dl : size_t = 256
    if(rsa_pkcs1_decrypt(&raw mut ctx, &raw mut ct[0], nl, &raw mut dec[0], &raw mut dl, 256) < 0) {
        env.error("CRT decrypt failed"); return
    } else {}

    // 128 bytes = the 1024-bit modulus, so the script carries the full P/Q-free
    // key (N and D only) and Python recomputes c^d mod N from scratch.
    var script : [4096]u8
    var sp : size_t = 0
    bt_push_str(&raw mut script[0], &raw mut sp, "c=int.from_bytes(bytes.fromhex('")
    bt_push_hex_bytes(&raw mut script[0], &raw mut sp, &raw mut ct[0], 128)
    bt_push_str(&raw mut script[0], &raw mut sp, "'),'big')\nd=int('")
    bt_push_hex_mpi(&raw mut script[0], &raw mut sp, &raw mut ctx.D, 128)
    bt_push_str(&raw mut script[0], &raw mut sp, "',16)\nn=int('")
    bt_push_hex_mpi(&raw mut script[0], &raw mut sp, &raw mut ctx.N, 128)
    bt_push_str(&raw mut script[0], &raw mut sp, "',16)\nem=pow(c,d,n).to_bytes(128,'big')\n")
    bt_push_str(&raw mut script[0], &raw mut sp,
        "i=em.index(0,2)\nprint('M=%0256x'%int.from_bytes(em[i+1:],'big'))\n")

    var out = test_python_run_script(&raw mut script[0], sp, string_view("bn_crt.py"))
    if(out.size() == 0) { env.error("python produced no output"); return } else {}

    // dec holds the unpadded message; Python's reference unpads the same way.
    var expect : [128]u8
    if(test_parse_py_hex_label(&raw mut out, string_view("M="), &raw mut expect[0], 128) != 128) {
        env.error("could not parse Python output"); return
    } else {}
    var got : [128]u8
    var i : size_t = 0
    while(i < 128) { got[i] = 0; i += 1 }
    i = 0
    while(i < dl) { got[128 - dl + i] = dec[i]; i += 1 }
    if(!test_bytes_eq(&raw mut got[0], &raw mut expect[0], 128)) {
        env.error("CRT decrypt disagrees with Python's c^d mod N"); return
    } else {}

    rsa_free(&raw mut ctx)
}
