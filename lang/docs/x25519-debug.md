# x25519 Field Arithmetic Debug Journal

## Files

- `lang/libs/tls/src/x25519.ch` — Main x25519 implementation (26-bit limbs, 10 limbs).
- `lang/compiled/x25519_test/src/main.ch` — Standalone RFC 7748 test.
- `lang/compiled/x25519_test.py` through `x25519_test4.py` — Python verification scripts.
- `lang/compiled/x25519_sim.py` — Python sim of the Chemical ladder.
- `lang/compiled/x25519_test_fixed.py` — Python sim with all fixes applied.

## Bug 1: `felem_mul` fold loses upper bits of `v * 608`

**File:** `lang/libs/tls/src/x25519.ch:178` (original code)

```chemical
lo[i - 10] = lo[i - 10] + (prod & 0x3FFFFFFu64)  // BUG: masks with 0x3FFFFFF
```

`prod = v * 608` where `v < 2^26`, so `prod` can be up to `~2^35`. Masking with `0x3FFFFFF` discards the upper ~9 bits.

**Fix:** Add the full `prod` without masking:

```chemical
lo[i - 10] = lo[i - 10] + (lo[i] * 608u64)
```

## Bug 2: `felem_mul` fold carry from `lo[0]` is lost

When `i=10`, the inner carry propagation loop starts at `k = i-11 = -1`, so the loop never executes and the carry from the addition to `lo[0]` is discarded.

**Fix:** Replaced the complex per-element carry fold with a batch fold + single final carry pass:

```chemical
i = 10
while(i < 20) {
    lo[i - 10] = lo[i - 10] + (lo[i] * 608u64)
    lo[i] = 0
    i += 1
}
// Single carry normalize pass:
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
    // re-normalize
}
```

## Bug 3: `felem_sub` u32 wrapping on non-canonical inputs

**Root cause:** `felem_sub` uses an offset array `OFF = 2*P` to make subtraction non-negative per-limb. But when inputs come from `felem_mul` (which can have `out[9]` up to `2^26-1 = 67108863`), the top limb offset `OFF[9] = 4194303 = 2^22-1` is exceeded, causing u32 wrapping. The wrap adds `2^266` to the value, which ≡ `38912 (mod p)`.

**Fix:** Reduce `out[9]` to 21 bits at the end of `felem_mul`:

```chemical
var t9c : u32 = out[9] >> 21
if(t9c > 0) {
    out[9] = out[9] & 0x1FFFFFu32
    out[0] = out[0] + (19u32 * t9c)
    // re-normalize
}
```

This ensures `out[9] < 2^21 = 2097152 < OFF[9] = 4194303`, preventing u32 wrapping in subsequent `felem_sub` calls.

## Bug 4: `felem_encode` used wrong reduction

The old `felem_encode` used `r = t9 * 608` and folded into `t0..t4` with a buggy carry pattern. The correct reduction uses `2^255 ≡ 19 (mod p)`:

```chemical
carry = t9 >> 21
if(carry == 0) { break }
t0 = t0 + (19u32 * carry)
t9 = t9 & 0x1FFFFFu32
```

After the reduction loop, `t9 < 2^21` and the value is in `[0, 2^255)`. Multiple iterations handle values up to `32*p`.

## Bug 5: `a24E` carry is lost in the second while loop

**Root cause:** The original code had `while(a24carry > 0 && ci < 10)` — when `ci >= 10`, the loop exits even with carry remaining. Every ladder iteration loses a carry of ~243,330, representing ~148 million mod p error.

**Fix:** Fold the carry with 608 (for `2^260 ≡ 608` mod p) and wrap around:

```chemical
while(a24carry > 0) {
    a24carry = a24carry * 608u64
    ci = 0
    while(a24carry > 0 && ci < 10) {
        a24carry = a24carry + (a24E[ci] as u64)
        a24E[ci] = (a24carry & 0x3FFFFFFu64) as u32
        a24carry = a24carry >> 26
        ci += 1
    }
}
```

**Important:** The `* 608` MUST come BEFORE the addition, not after. The carry represents overflow beyond 10 limbs, which is `2^260` per unit, and `2^260 ≡ 608 (mod p)`.

## Current State

After all fixes, the Python simulation matches the Python reference for ALL 255 ladder iterations when using the correct scalar. The RFC 7748 test vector:

```
Alice priv: 77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba5a1d92c2a
Expected:   8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a
```

The reference `X25519(a, 9)` gives the expected value. The Python sim of the Chemical ladder (with all fixes) gives a DIFFERENT value, indicating there may still be remaining issues in the Chemical code.

## Remaining Unknowns

- The Chemical code still produces a different output from the Python sim, meaning at least one more bug exists.
- Possible issues:
  1. Felem_sq is called with a pointer to the same array as the output, causing aliasing issues
  2. The Chemical compiler might generate wrong C code for some constructs
  3. There's an additional bug not yet identified

## Test Commands

```bash
# Build compiler
./scripts/build.sh --tcc

# Compile x25519
cmake-build-debug/TCCCompiler lang/compiled/x25519_test/chemical.mod \
    -o lang/compiled/x25519_test/x25519_test.exe -v -bm-modules --no-cache

# Run
./lang/compiled/x25519_test/x25519_test.exe
```
