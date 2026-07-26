# TLS Integration Tests — Complete Knowledge Base

> **Purpose:** This document captures everything an AI needs to fix the TLS library integration tests (`./scripts/test.sh --tcc --tls`). It covers architecture, bugs found, testing methodology, and all known issues.

---

## 1. Project Overview

Chemical is a compiled language with two backends:
- **LLVM backend** (`Compiler`) — full LLVM/Clang pipeline
- **C translation backend** (`TCCCompiler`) — Chemical → C → TinyCC

The TLS library (`lang/libs/tls/`) is written in pure Chemical (no C FFI for crypto primitives). It implements:
- x25519 key exchange (26-bit limb field arithmetic)
- AES-128/256-GCM
- SHA-256, HMAC-SHA256, HKDF
- TLS 1.3 handshake subset

Tests for the TLS library are at `lang/tests/src/tls/` and invoked with `--tls` flag.

## 2. Build & Test System

### Build Commands
```bash
./scripts/setup.sh                     # Download libtcc, update submodules
./scripts/configure.sh --no-llvm       # CMake configure, no LLVM
./scripts/build.sh --tcc               # Build TCCCompiler
```

### Test Commands
```bash
./scripts/test.sh --tcc --tls          # Run TLS tests (builds TCCCompiler + compiles tests)
./scripts/test.sh --tcc --tls --no-build  # Skip rebuild, use existing binary
./scripts/test.sh --tcc --tls -bt      # Run under GDB, backtrace on crash
```

### Test Wiring

The test entry point is `lang/tests/build.lab`. When `--tls` is passed:
1. `build.lab` creates a `LabJobType::Compilation` job
2. It compiles `lang/tests/src/tls/` as a module
3. The TLS test module imports `tls` from `lang/libs/tls/`
4. The compiled executable is run

### Debugging Flow
```bash
# 1. Isolate the failing test to a standalone module
mkdir -p lang/compiled/my_test/src

# 2. Write chemical.mod
echo 'application my_test
source "src"
import cstd
import tls' > lang/compiled/my_test/chemical.mod

# 3. Write test code in lang/compiled/my_test/src/main.ch

# 4. Compile and run
cmake-build-debug/TCCCompiler lang/compiled/my_test/chemical.mod \
    -o lang/compiled/my_test/my_test.exe -v -bm-modules --no-cache
./lang/compiled/my_test/my_test.exe
```

## 3. C Codegen Gotchas (Critical!)

The TCCCompiler translates Chemical to C, then feeds to TinyCC. This introduces several pitfalls:

### 3.1 `u32` Wrapping is Preserved

Chemical `u32` maps to C `uint32_t`. All arithmetic wraps at 32 bits. This is correct.

### 3.2 `u64` Multiplications

Chemical `u64` maps to C `uint64_t`. Products up to 2^64 are safe. Beyond that, results are truncated.

### 3.3 `*mut [10]u32` vs `*mut u32` for Arrays

**Critical:** Chemical does NOT allow `a[i]` indexing on a `*mut [10]u32` (pointer-to-array). The compiler errors. You MUST use `*mut u32` (pointer-to-element) and pass `&raw mut arr[0]`.

This is already done correctly in the x25519 code.

### 3.4 `while(ci < 10 && cc > 0)` Short-Circuit Evaluation

C's `&&` short-circuits. If `ci >= 10`, `cc > 0` is NOT evaluated. This is correct behavior, same as Chemical.

### 3.5 Integer Constant Overflow Warnings

The C translation sometimes produces integer constant overflow warnings. These are generally harmless but may indicate code that works differently than expected.

## 4. TLS Library Structure

```
lang/libs/tls/
├── chemical.mod           # Module declaration
└── src/
    ├── x25519.ch          # x25519 (26-bit limb field arithmetic)
    ├── ssl.ch             # Main TLS implementation (~4300 lines)
    ├── gcm.ch             # AES-GCM
    ├── aes.ch             # AES encryption
    ├── sha256.ch          # SHA-256
    ├── hmac.ch            # HMAC-SHA256
    └── hkdf.ch            # HKDF key derivation
```

Other relevant libs:
```
lang/libs/crypto/src/
├── sha256.ch              # SHA-256 (referenced by tls)
└── hmac.ch                # HMAC-SHA256
```

## 5. x25519 Implementation Details

### 5.1 Field Representation

- **Prime:** `p = 2^255 - 19`
- **Limb size:** 26 bits
- **Limb count:** 10
- **Capacity:** 260 bits (10 × 26)
- **Reduction:** `2^260 ≡ 608 (mod p)`, `2^520 ≡ 369664 (mod p)`

### 5.2 Key Constants

```python
# Modulus in 26-bit limbs
P_LIMBS = [67108845, 67108863, 67108863, 67108863, 67108863,
           67108863, 67108863, 67108863, 67108863, 2097151]

# Offset for subtraction (2*P)
OFF = [0x3FFFFDA, 0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF,
       0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF, 0x3FFFFF]

# Curve constant a24 = (486662 - 2) / 4 = 121665
a24 = 121665
```

### 5.3 Field Operations

#### `felem_mul(out, a, b)` — Multiplication
```
1. lo[20] = schoolbook_product(a, b)    // 20-limb result
2. Carry normalize lo[0..19]             // each < 2^26
3. If carry > 0: lo[0] += carry * 369664   // 2^520 ≡ 369664
4. For i = 10..19: lo[i-10] += lo[i] * 608  // 2^260 ≡ 608
5. Final carry normalize lo[0..9]        // each < 2^26
6. If carry > 0: fold with 608, re-normalize
7. t9 reduction: out[9] >> 21 * 19 → out[0]  // 2^255 ≡ 19
```

**Bug history (FIXES APPLIED):**
- `prod & 0x3FFFFFF` was dropping upper bits — fixed to use full prod
- Per-element carry propagation was losing final carry — fixed to batch fold + single carry pass
- `369664` carry propagation loop could lose carry at position 9 — fixed by removing propagation (just add raw value)
- Missing t9 reduction at end — fixed by adding `out[9] >> 21 * 19` reduction

#### `felem_sub(out, a, b)` — Subtraction with Offset
```
out[i] = a[i] + OFF[i] - b[i]   // u32 wraps if negative
```

**Critical constraint:** This only works correctly when `b[i] <= a[i] + OFF[i]` for all `i`. If `b[i]` exceeds this, u32 wrapping adds `2^32 * 2^(26*i)` to the value, which is NOT a multiple of p for `i = 9` (`2^266 ≡ 38912 mod p`).

With the t9 reduction fix, `out[9] < 2^21 = 2097152 < OFF[9] = 4194303`, preventing the wrap at limb 9. But lower limbs can still wrap in theory (though with smaller errors).

#### `felem_encode(out, a)` — Serialization
```
1. Loop (max 32 iterations):
   a. Carry normalize t0..t9
   b. carry = t9 >> 21
   c. If carry == 0: break
   d. t0 += 19 * carry
   e. t9 &= 0x1FFFFF
2. Serialize 10 limbs to 32 bytes (little-endian)
```

This replaces the old broken encoding that used `t9 * 608`.

#### `felem_decode(out, data)` — Deserialization
Reads 32 bytes little-endian, produces 10 limbs of 26 bits each.

### 5.4 Montgomery Ladder

The ladder function `x25519_ladder` implements the standard Montgomery ladder for X25519:

```
Input: scalar[32] bytes (little-endian, clamped), u[32] bytes (u-coordinate)
Output: out[32] bytes (result u-coordinate)

1. Decode u to 10 limbs
2. Initialize (x2,z2) = (1,0), (x3,z3) = (u,1)
3. For bit = 254 down to 0:
   a. Conditional swap (x2,z2) ↔ (x3,z3) based on scalar bit
   b. A = x2 + z2, AA = A^2
   c. B = x2 - z2, BB = B^2
   d. E = AA - BB
   e. C = x3 + z3, D = x3 - z3
   f. DA = D * A, CB = C * B
   g. x3 = (DA + CB)^2
   h. z3 = u * (DA - CB)^2
   i. x2 = AA * BB
   j. a24E = a24 * E
   k. z2 = (AA + a24E) * E
4. Final conditional swap
5. Compute z2^(-1) = z2^(p-2) via exponentiation chain
6. result = x2 * z2^(-1)
7. Encode result to bytes
```

### 5.5 a24E Computation (a24 = 121665)

**Bug history (FIX APPLIED):**
The original code lost the carry from the last limb:
```chemical
while(a24carry > 0 && ci < 10)  // BUG: exits when ci >= 10, losing carry
```

Every ladder iteration lost ~243,330 of carry (~148 million mod p error).

**Fixed version:**
```chemical
while(a24carry > 0) {
    a24carry = a24carry * 608u64       // 2^260 ≡ 608 (mod p)
    ci = 0
    while(a24carry > 0 && ci < 10) {
        a24carry = a24carry + (a24E[ci] as u64)
        a24E[ci] = (a24carry & 0x3FFFFFFu64) as u32
        a24carry = a24carry >> 26
        ci += 1
    }
}
```

**Important:** The `* 608` must come BEFORE the addition, not after. It folds the carry representing overflow beyond 10 limbs (position 260), using `2^260 ≡ 608 (mod p)`.

## 6. Inversion Chain (z2^(p-2))

The exponent chain computes `z2^(-1) = z2^(p-2)` where `p-2 = 2^255 - 21`:

```
Start: z2_inv = 1
For j = 254 down to 5:            // 250 iterations
    z2_inv = z2_inv^2 * z2         // → z2^(2^250 - 1)

z2_inv = z2_inv^2                  // → z2^(2^251 - 2)
z2_inv = z2_inv^2                  // → z2^(2^252 - 4)
z2_inv = z2_inv * z2              // → z2^(2^252 - 3)
z2_inv = z2_inv^2                  // → z2^(2^253 - 6)
z2_inv = z2_inv^2                  // → z2^(2^254 - 12)
z2_inv = z2_inv * z2              // → z2^(2^254 - 11)
z2_inv = z2_inv^2                  // → z2^(2^255 - 22)
z2_inv = z2_inv * z2              // → z2^(2^255 - 21) = z2^(p-2)
```

This chain is verified correct.

## 7. All Known Bugs (Fixed)

| # | Function | Bug | Impact | Fix |
|---|----------|-----|--------|-----|
| 1 | `felem_mul` fold | `prod & 0x3FFFFFF` drops upper bits of `v*608` | Incorrect reduction of high limbs | Use full `prod` |
| 2 | `felem_mul` fold | Carry from `lo[0]` lost when inner loop `j=-1` doesn't execute | Incorrect carry propagation | Batch fold + single carry pass |
| 3 | `felem_mul` 369664 fold | Propagation loop exits at `ci=9`, losing final carry | Modular error of `cc*2^260` | Remove propagation (just add raw value) |
| 4 | `felem_mul` output | `out[9]` can be up to `2^26-1`, exceeding `OFF[9] = 2^22-1` | `felem_sub` wraps at limb 9, adding 38912 mod p | t9 reduction: `out[9] >> 21 * 19` |
| 5 | `felem_encode` | Used `t9 * 608` instead of `t9 >> 21 * 19` | Wrong reduction | Correct `2^255 ≡ 19 (mod p)` reduction |
| 6 | `a24E` loop | `while(ci < 10 && carry > 0)` loses carry at `ci=10` | ~148M mod p error per ladder iteration | Wrap-around with `* 608` |
| 7 | `a24E` wrap | Carry added to `a24E[0]` without `* 608` fold | Carry doesn't account for `2^260 ≡ 608` | Multiply by 608 BEFORE adding |

## 8. Remaining Unknown Issues

The Python simulation with all fixes matches the Python reference for all 255 ladder iterations (value-wise). However, the Chemical code still produces a different output. Possible causes:

### 8.1 C Translation Bugs

The Chemical → C translation might introduce subtle issues:
- `u32` cast of `u64` intermediate values
- Bool comparison of `u32` values
- Loop counter overflow (`size_t` vs `i32`)
- Integer promotion rules in C
- Right-shift of signed values

### 8.2 Aliasing Issues

In the ladder, variables like `AA`, `BB`, `E`, `DA`, `CB`, `x3`, `z3`, `x2`, `z2` are reused each iteration. The Chemical code might have aliasing between inputs and outputs of functions. For example:

```chemical
felem_mul(&raw mut x3[0], &raw mut DA[0], &raw mut CB[0])
```

Here `x3` overlaps with `DA` or `CB`? No, all are separate arrays. But `felem_mul` takes `out`, `a`, `b` as separate pointers. If `out` aliases `a` or `b`, the schoolbook product reads original input values while writing output, which is safe.

### 8.3 Debugging Checklist

If the Chemical output differs from the Python sim:

1. **Translate to C and inspect:** Build with `--emit-c` and check the generated C for the x25519 functions
2. **Add printf debugging:** Insert `printf` calls in the Chemical code to trace intermediate values
3. **Test field operations in isolation:** Create a minimal test that just does `felem_mul` on known values and verifies the result
4. **Check the full pipeline:** `felem_decode` → ladder → `felem_encode`
5. **Compare with Python sim at each step:** Use the same inputs, check intermediate values

### 8.4 Potential Remaining Bug Locations

- `felem_sq` just calls `felem_mul(out, a, a)` — aliasing `out` and `a` should be safe
- `x25519_ladder` variable reuse — arrays `A`, `AA`, `B`, `BB`, etc. are reused across iterations; ensure no stale data
- `felem_encode` loop termination — the `while(iter < 32)` loop might not converge for some inputs
- `felem_sub` lower limb wraps — even with t9 fixed, lower limbs (`i=0..8`) could still wrap causing small errors
- `a24E` loop — the Chemical fix might not exactly match the Python sim's version

## 9. Test Vectors

### RFC 7748 Section 6.1

```
Alice's private key, a:
  77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba5a1d92c2a

Alice's public key, X25519(a, 9):
  8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a

Bob's private key, b:
  5dab087e624a8a4b79e1edf02775359a8a674f16f27d8f835f2e63452ed31f44

Bob's public key, X25519(b, 9):
  de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f

Shared secret, X25519(a, X25519(b, 9)):
  4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742
```

### Scalar Clamping

```python
scalar[0] &= 0xF8   # Clear bottom 3 bits
scalar[31] &= 0x7F  # Clear top bit  
scalar[31] |= 0x40  # Set second-to-top bit
```

## 10. Python Verification Scripts

All scripts live in `lang/compiled/`:

| Script | Purpose | Status |
|--------|---------|--------|
| `x25519_test.py` | Basic field mul tests, inversion test | Fixed bugs 1-4 |
| `x25519_test2.py` | Fold algorithm comparison (ours, fixed, correct) | Documents bugs 1-2 |
| `x25519_test3.py` | Attempt2/3 verification | Fixed bugs 1-2 |
| `x25519_test4.py` | Three fold approach comparison | Bug 1-2 verification |
| `x25519_sim.py` | Complete Chemical ladder sim (u32 masking + correct fold) | References known state |
| `x25519_test_fixed.py` | Complete ladder with all fixes | Current working state |

### Key Python Verification

```python
# Simulates the EXACT Chemical algorithm for felem_mul:

def felem_mul(f, g):
    lo = [0]*20
    for i in range(10):
        for j in range(10):
            lo[i+j] += f[i] * g[j]
    
    # 20-limb carry normalize
    carry = 0
    for i in range(20):
        s = lo[i] + carry
        lo[i] = s & 0x3FFFFFF
        carry = s >> 26
    
    # 369664 fold (NO carry propagation beyond position 0)
    if carry > 0:
        lo[0] += carry * 369664
    
    # Batch fold lo[10..19] into lo[0..9]
    for i in range(10, 20):
        lo[i-10] += lo[i] * 608
        lo[i] = 0
    
    # Final carry pass
    carry = 0
    out = [0]*10
    for i in range(10):
        s = lo[i] + carry
        out[i] = s & 0x3FFFFFF
        carry = s >> 26
    
    # Final carry fold with 608
    if carry > 0:
        out[0] += carry * 608
        carry = 0
        for i in range(10):
            s = out[i] + carry
            out[i] = s & 0x3FFFFFF
            carry = s >> 26
    
    # t9 reduction: out[9] >> 21 * 19 → out[0]
    t9c = out[9] >> 21
    if t9c > 0:
        out[9] &= 0x1FFFFF
        out[0] += 19 * t9c
        rcc = 0
        for i in range(10):
            s = out[i] + rcc
            out[i] = s & 0x3FFFFFF
            rcc = s >> 26
        if rcc > 0:
            out[0] += rcc * 608
            rcc = 0
            for i in range(10):
                s = out[i] + rcc
                out[i] = s & 0x3FFFFFF
                rcc = s >> 26
    
    return out
```

## 11. Step-by-Step Fix Application

If you need to re-apply all fixes:

### Step 1: Fix `felem_mul` fold in `lang/libs/tls/src/x25519.ch`

Replace lines ~166-198 (the `i=19` loop + carry pass) with the batch fold approach:

```chemical
i = 10
while(i < 20) {
    lo[i - 10] = lo[i - 10] + (lo[i] * 608u64)
    lo[i] = 0
    i += 1
}
carry = 0
i = 0
while(i < 10) {
    var s : u64 = lo[i] + carry
    out[i] = (s & 0x3FFFFFFu64) as u32
    carry = s >> 26
    i += 1
}
if(carry > 0) {
    out[0] = out[0] + (carry * 608u64) as u32
    carry = 0
    i = 0
    while(i < 10) {
        var s : u64 = (out[i] as u64) + carry
        out[i] = (s & 0x3FFFFFFu64) as u32
        carry = s >> 26
        i += 1
    }
}
```

### Step 2: Remove 369664 carry propagation

```chemical
if(carry > 0) {
    lo[0] = lo[0] + carry * 369664u64
}
```

### Step 3: Add t9 reduction after final fold

```chemical
var t9c : u32 = out[9] >> 21
if(t9c > 0) {
    out[9] = out[9] & 0x1FFFFFu32
    out[0] = out[0] + (19u32 * t9c)
    var rcc : u64 = 0
    i = 0
    while(i < 10) {
        var s : u64 = (out[i] as u64) + rcc
        out[i] = (s & 0x3FFFFFFu64) as u32
        rcc = s >> 26
        i += 1
    }
    if(rcc > 0) {
        out[0] = out[0] + (rcc * 608u64) as u32
        rcc = 0
        i = 0
        while(i < 10) {
            var s : u64 = (out[i] as u64) + rcc
            out[i] = (s & 0x3FFFFFFu64) as u32
            rcc = s >> 26
            i += 1
        }
    }
}
```

### Step 4: Fix `felem_encode`

Replace the entire function with the loop-based reduction.

### Step 5: Fix `a24E` loop

Replace `while(a24carry > 0 && ci < 10)` with the wrap-around version.

## 12. Build and Test After Fixes

```bash
# 1. Rebuild the compiler (fixes are in Chemical source, not C++)
./scripts/build.sh --tcc

# 2. Test the standalone test
rm -rf lang/compiled/x25519_test/build
cmake-build-debug/TCCCompiler lang/compiled/x25519_test/chemical.mod \
    -o lang/compiled/x25519_test/x25519_test.exe -v -bm-modules --no-cache
./lang/compiled/x25519_test/x25519_test.exe

# 3. Run full TLS test suite
./scripts/test.sh --tcc --tls --no-build
```

## 13. Common Pitfalls

### 13.1 `int.from_bytes` vs Hex Literal

When testing in Python:
```python
# WRONG — hex literal is big-endian integer
scalar = 0x70076d0a...

# CORRECT — bytes in little-endian order
scalar = int.from_bytes(bytes([0x70, 0x07, ...]), 'little')
```

### 13.2 `felem_sub` Always Needs u32 Masking

In Python simulation, always mask felem_sub results:
```python
def felem_sub(a, b):
    out = [0]*10
    for i in range(10):
        v = a[i] + OFF[i] - b[i]
        out[i] = v & 0xFFFFFFFF  # CRITICAL: u32 wrap
    return out
```

### 13.3 `a24E` Computation Needs `E[ci] & 0xFFFFFFFF`

```python
a24carry += 121665 * (E[ci] & 0xFFFFFFFF)  # u32 mask before u64 cast
```

### 13.4 Building with `--no-cache`

Always use `--no-cache` when rebuilding after source changes to ensure the C translation is re-generated.

### 13.5 Chemical `import` Statements

`import` only works in `.mod` files (module declarations), NOT in `.ch` files (source). The `.mod` file declares dependencies; the `.ch` file just uses them.

## 14. Quick Reference: Mathematical Identities

```
2^255  ≡ 19   (mod 2^255 - 19)
2^256  ≡ 38   (mod p)
2^260  ≡ 608  (mod p)   [= 19 × 2^5]
2^520  ≡ 369664 (mod p) [= 608^2]

p = 2^255 - 19
a24 = (486662 - 2) / 4 = 121665

26-bit limb mask: 0x3FFFFFF = 67108863
21-bit limb mask: 0x1FFFFF  = 2097151
```

## 15. Important File Paths

| File | Purpose |
|------|---------|
| `lang/libs/tls/src/x25519.ch` | x25519 implementation (EDIT THIS) |
| `lang/libs/tls/src/ssl.ch` | Main TLS implementation |
| `lang/libs/tls/src/gcm.ch` | AES-GCM |
| `lang/libs/tls/src/aes.ch` | AES |
| `lang/libs/tls/chemical.mod` | TLS module declaration |
| `lang/tests/src/tls/` | TLS test source |
| `lang/tests/build.lab` | Build script (routes `--tls` flag) |
| `lang/compiled/x25519_test/` | Standalone x25519 test |
| `lang/compiled/x25519_test/src/main.ch` | Standalone test source |
| `lang/compiled/x25519_test.py` | Python verification |
| `lang/compiled/x25519_test_fixed.py` | Python sim with all fixes |
| `docs/x25519-debug.md` | Debug journal |
