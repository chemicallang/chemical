# Constant-Time Cryptography Requirements for Chemical TLS

## Problem

The TLS library's cryptographic primitives are not constant-time. This means
execution time varies depending on secret data (keys, plaintext, nonces),
leaking information through timing side channels. An attacker who can measure
execution time can extract cryptographic keys bit by bit.

## Affected Components

| Component | Operation | Vulnerability |
|-----------|-----------|--------------|
| AES (aes.ch) | S-box lookup | Table lookup address depends on key-scheduled round key and plaintext |
| AES (aes.ch) | MixColumns mul2/mul3 | Conditional XOR: `if(val & 0x80) { result ^= 0x1B }` leaks plaintext |
| GHASH (gcm.ch) | ghash_multiply_refined | Bit-by-bit loop: `if(xi) { Z ^= V }` leaks H subkey |
| RSA (rsa.ch) | mpi_exp_mod | Binary exponentiation: different operations for bit=0 vs bit=1 |
| ECDSA (ecdsa.ch) | ecp_mul (Montgomery ladder) | Already constant-time (both branches execute) |
| ECDH (ecdh.ch) | ecp_mul (Montgomery ladder) | Already constant-time ✓ |
| x25519 (x25519.ch) | Montgomery ladder with Mpi | Uses general Mpi ops which are NOT constant-time |
| Bignum (bignum.ch) | mpi_exp_mod | Binary exponentiation — leaks exponent bits |
| Bignum (bignum.ch) | mpi_mod_inv | Binary extended GCD — branch conditions reveal secret |

## What Chemical Needs (Language Features)

Chemical currently lacks:

1. **Constant-time conditional move** (`ct_select`):
   ```
   // Needed: select a if condition else b, without branching
   public func ct_select(condition : bool, a : u32, b : u32) : u32
   ```

2. **Constant-time byte/word equality** (`ct_eq`):
   ```
   // Needed: compare without branching (already have XOR accumulation pattern)
   public func ct_eq_u8(a : u8, b : u8) : u8  // returns 0xFF if equal, 0x00 if not
   ```

3. **Constant-time array comparison** (`ct_memcmp`):
   ```
   // Returns 0 if equal, nonzero if different — constant time
   public func ct_memcmp(a : *u8, b : *u8, len : size_t) : int
   ```

4. **Barrier intrinsics** (prevent compiler optimization):
   ```
   // Prevents compiler from optimizing away "dead" ct operations
   public func ct_barrier(val : *mut u8)
   ```

## Implementation Plan

### Phase 1: Add ct primitives to crypto library (`lang/libs/crypto/`)

Create `lang/libs/crypto/src/ct.ch`:

```chemical
public namespace crypto {

    // Constant-time select: returns a if cond else b
    public func ct_select_u8(cond : u8, a : u8, b : u8) : u8 {
        var mask = ((cond as i8) >> 7) as u8  // 0xFF if cond else 0x00
        return (a & mask) | (b & (!mask as u8))
    }

    // Constant-time equality: returns 0xFF if equal, 0x00 if not
    public func ct_eq_u8(a : u8, b : u8) : u8 {
        var diff = a ^ b
        diff = diff | (diff >> 4)
        diff = diff | (diff >> 2)
        diff = diff | (diff >> 1)
        return ((diff as i8) >> 7) as u8  // 0xFF if diff==0, else 0x00
    }

    // Constant-time memory comparison
    public func ct_memcmp(a : *u8, b : *u8, len : size_t) : u8 {
        var diff : u8 = 0
        var i : size_t = 0
        while(i < len) {
            diff = diff | (a[i] ^ b[i])
            i += 1
        }
        return diff  // 0 if equal, nonzero if different
    }
}
```

### Phase 2: AES constant-time

Replace the S-box lookup table with a bit-sliced implementation:

```
// Instead of: result = sbox[byte]
// Use bit-sliced S-box that computes output from input bits without
// table lookups. This requires 5-8 bitwise operations per bit.
//
// Reference: "Faster and Timing-Attack Resistant AES-GCM" (Kasper/Schwabe)
// Implementation in C: ~400 lines of bitwise ops per AES round
```

### Phase 3: GHASH constant-time

Replace bit-by-bit GHASH with table-based or carry-less multiplication:

```
// Instead of: if(xi) { Z ^= V } (branch per bit)
// Use word-at-a-time multiplication with precomputed table:
//
// For each byte in x:
//   idx = x[byte_pos]
//   Z ^= table[idx]  // lookup is constant-time (always reads from table)
//
// This requires a 256-entry * 16-byte table (4KB).
```

### Phase 4: RSA constant-time

Replace binary exponentiation with Montgomery ladder (same as ECDH):

```
// Instead of: if(bit == 0) { R = R^2 } else { R = R * R^2 * M }
// Use: R0 = 1, R1 = M
//      for each bit: if bit=0 { R1 = R0*R1; R0 = R0^2 }
//                    if bit=1 { R0 = R0*R1; R1 = R1^2 }
// Both branches do the same operations in swapped order.
```

### Phase 5: Bignum constant-time

The `bignum.ch` operations need word-level constant-time:

```
// mpi_add, mpi_sub: always operate on full limb count
// mpi_mul: always compute full product (no early termination)
// mpi_mod: constant-time reduction (always do full Barrett reduction)
// mpi_exp_mod: use Montgomery ladder (same as RSA above)
```

## Priority

1. **ct_memcmp** — already implemented in GCM tag comparison. Use consistently everywhere.
2. **RSA mpi_exp_mod** — CRITICAL. Private exponent leaks through timing.
3. **GHASH** — HIGH. H subkey is secret.
4. **AES S-box** — MEDIUM. Round keys depend on master key.
5. **ECDSA mpi_mod_inv** — LOW. Only used during signature verification (not generation).

## Current State

What IS constant-time:
- GCM tag comparison ✓ (XOR accumulation, fixed in CRIT-5 fix session)
- ECDH scalar multiplication ✓ (Montgomery ladder in ecp_mul)
- CBC MAC comparison ✓ (XOR accumulation in tls12_decrypt_record)
- x25519 scalar multiplication ✓ (Montgomery ladder in x25519_ladder, but uses Mpi ops which aren't ct)

What is NOT constant-time:
- AES encrypt/decrypt (S-box table + conditional XOR in MixColumns)
- GHASH multiplication (bit-by-bit with branch)
- RSA exponentiation (binary exponentiation with branch)
- Bignum modular inverse (binary GCD with branches)
- ECDSA verify (uses Mpi ops, not ct)

## Notes

- The Chemical compiler (TCC backend) may optimize away "unused" constant-time
  operations. A `ct_barrier` intrinsic or `volatile` equivalent is needed.
- True constant-time requires the generated C code to not have branches that
  depend on secret data. TCC's code generation must be verified.
- For production deployment, an external audit of the actual generated machine
  code timing behavior is recommended.
