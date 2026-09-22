# TLS bignum & RSA — Remaining Work

**Purpose.** The TLS library's bignum (`lang/libs/tls/src/bignum.ch`) and RSA
(`lang/libs/tls/src/rsa.ch`) got a round of correctness and performance work that
took RSA-2048 from *unusable* to *usable*: a public operation is ~8x faster and a
private one ~3.4x faster, and three latent correctness bugs were fixed. This file
is the **actionable remainder** — what was deliberately left alone, why, and what
"done" means for each. Every item has a stable ID, the symptom, the root cause
location, the current state/workaround, and a definition of done with the exact
commands to verify it.

**Companion docs**

- [`constant-time-crypto.md`](./constant-time-crypto.md) — the constant-time policy
  RSA-1 below has to satisfy.
- [`../libs/tls/`](../libs/tls/) — sources; tests in `lang/tests/tls/`
  (`./scripts/test.sh --tcc --tls`).

## Ground rules for every item

1. **Measure before and after.** Every performance item below was sized with the
   probe described in *How to measure*. Numbers quoted here are from the TCC
   backend in `debug_quick`; LLVM numbers differ in absolute terms but not in
   ordering. Do not claim a win you did not measure — the keygen path in
   particular has enormous run-to-run variance (see *Appendix B*).
2. **Cross-check against Python, not against yourself.** Arithmetic changes are
   verified differentially against Python's arbitrary-precision integers
   (`lang/compiled/bn_check/verify.py`). A test that recomputes the expected value
   with the same algorithm under test proves nothing.
3. **Keep the fallbacks.** `mpi_div_bitwise`, `mpi_mod_inv_euclid` and
   `mpi_exp_mod_fallback` are live code paths (even/Euclid/`MAX_LIMBS` cases), not
   dead legacy. Removing one needs a replacement that covers its shape.
4. **Do not weaken a check to make a test pass.** `rsa_crt_ready` and the
   verification step inside `mpi_mod_inv` are deliberate: they trade a few
   microseconds for not returning a wrong plaintext on garbage input.
5. **Rebuild the compiler if you touch C++.** Not needed for `.ch` work: the test
   suite recompiles the libraries from source every run.

### Verification matrix (run after a fix)

```bash
./scripts/test.sh --tcc --tls          # 590 tests, the suite that matters here
./scripts/test.sh --llvm --tls         # same suite on the LLVM backend
```

Plus the differential harness (below) for anything that touches arithmetic. Both
suites must stay green; `--all` is for humans only.

---

## How to measure

The probes live under `lang/compiled/` (gitignored, so they may need recreating —
see item **BN-HARNESS**). Each is a standalone module compiled directly by the TCC
compiler in ~1 s:

```bash
# Primitive + end-to-end timings (mul/mod/div/inv/exp/gen_key/RSA ops)
cmake-build-debug/TCCCompiler lang/compiled/bn_perf/chemical.mod \
    -o lang/compiled/bn_perf/bn_perf.exe --mode debug_quick --no-cache
./lang/compiled/bn_perf/bn_perf.exe

# 459-case differential test against Python (div/mod/exp/inv)
cmake-build-debug/TCCCompiler lang/compiled/bn_check/chemical.mod \
    -o lang/compiled/bn_check/bn_check.exe --mode debug_quick --no-cache
./lang/compiled/bn_check/bn_check.exe > /tmp/bn_check.txt
python3 lang/compiled/bn_check/verify.py /tmp/bn_check.txt      # prints ALL MATCH

# RSA key + CRT decryption verified against Python's non-CRT pow(c,d,n)
cmake-build-debug/TCCCompiler lang/compiled/rsa_crt/chemical.mod \
    -o lang/compiled/rsa_crt/rsa_crt.exe --mode debug_quick --no-cache
./lang/compiled/rsa_crt/rsa_crt.exe > /tmp/rsa_crt.txt
python3 lang/compiled/rsa_crt/verify.py /tmp/rsa_crt.txt
```

Timing notes that will otherwise waste your time:

- `clock()` in Chemical is **CPU time** in microseconds, not wall time. Sleeping
  does not advance it, so it cannot calibrate itself.
- `debug_quick` TCC builds carry no optimisations. Absolute numbers are roughly
  3-5x worse than the LLVM backend; compare like with like.
- Single measurements swing by 2x under load (12-core machine shared with the test
  runner). Take medians of several runs before believing a <20% difference.

---

## What is already done (baseline for the items below)

Measured on the same probe, same machine, TCC backend:

| operation | before | after | gain |
|---|---|---|---|
| `2048 % 1024` reduction | 477 µs | 13.6 µs | 35x |
| `4096 % 2048` reduction | 1704 µs | 42 µs | 40x |
| 2048/1024 division | 437 µs | 17 µs | 25x |
| 1024-bit modular inverse | 4989 µs | 1199 µs | 4.2x |
| RSA-2048 public op | 10962 µs | 1390 µs | 7.9x |
| RSA-2048 private op | 155658 µs | 45206 µs | 3.4x |
| RSA-1024 public op | 3769 µs | 523 µs | 7.2x |
| RSA-1024 private op | 23375 µs | 10699 µs | 2.2x |

What produced that, and what a future reader must not accidentally undo:

- **`mpi_div_knuth`** — Knuth Algorithm D, one quotient limb per pass. The old
  `mpi_div` walked one quotient *bit* per iteration and re-shifted the whole
  divisor each time, so one 2n÷n reduction cost 464 µs while a full 1024x1024
  multiply costs 12 µs. `mpi_div` **dispatches**: `m == 0` goes to the bit-walker
  (Euclid steps in `mpi_mod_inv`/`mpi_gcd` are single-limb quotients, and Knuth's
  fixed setup cost makes it 1.4x slower there), everything else to Knuth, with the
  bit-walker again as the `MAX_LIMBS` fallback.
- **`mpi_mod_inv`** — binary extended GCD (HAC 14.61) for odd moduli, with the
  Euclidean loop kept for even ones. Every modulus the library inverts against in
  the hot path (RSA primes, prime fields, ECDSA's `n`) is odd.
- **`rsa_private_crt`** — RFC 8017 §5.1.2 recombination, gated by
  `rsa_crt_ready`, which validates the parameters structurally rather than trusting
  a flag.
- **`pkcs1_v15_encode`** — one `/dev/urandom` read per encryption instead of 110
  (RSA-1024) to 237 (RSA-2048). `random_fill` opens, reads and closes the file per
  call (~30 µs), so the padding loop alone cost several milliseconds.
- **Three correctness fixes found by the new tests**: `mpi_exp_mod` truncated a
  base with more limbs than the modulus instead of reducing it; the negative-base
  fixup negated an already-reduced result (`(-5)^1 mod 12` returned 5, not 7); and
  `mpi_exp_mod_fallback` clobbered the base when called as
  `mpi_exp_mod(x, x, e, n)`.
- **Tests** — `lang/tests/tls/src/bignum_tests.ch`, 12 tests: Python-verified
  known-answer vectors, random-input invariants, and live differential tests
  against Python for `divmod`, `pow`, `pow(a,-1,n)` and CRT decryption.

---

## Recommended order

| Order | ID | Title | Area | Effort |
|-------|----|-------|------|--------|
| 1 | **BN-MONT-SQR** | Modular squaring does twice the multiplies | `bignum.ch` | M |
| 2 | **BN-WINDOW** | Square-and-multiply, no windowing | `bignum.ch` | M |
| 3 | **BN-MONT-CACHE** | `R^2 mod N` recomputed on every `mpi_exp_mod` | `bignum.ch`, `rsa.ch` | M |
| 4 | **RSA-CRT-IMPORT** | Private-key import discards the CRT parameters | `rsa.ch`, `ssl.ch` | M |
| 5 | **RSA-PSS-OAEP** | `RSA_PKCS_V21` is a constant with no implementation | `rsa.ch` | L |
| 6 | **RSA-BLINDING** | Private operations are not blinded or constant-time | `rsa.ch` | L |
| 7 | **BN-INIT-ZERO** | Every temp zeroes 512 limbs | `bignum.ch` | S |
| 8 | **BN-MODINV-EVEN** | Even-modulus inverse is still division-based | `bignum.ch` | S |
| 9 | **BN-ADD-BACK-KAT** | The Algorithm D add-back branch has no test | tests | S |
| 10 | **BN-HARNESS** | The differential harness is not in the repo or CI | tests/tooling | S |
| 11 | **BN-PERF-GUARD** | No regression guard on the arithmetic hot paths | tests | S |
| 12 | **RSA-TINYKEY-FALLBACK** | Encryption silently degrades to raw RSA | `rsa.ch` | S |
| 13 | **TLS-BUILD-DIR** | `lang/tests/tls/build/` is not gitignored | `.gitignore` | S |
| 14 | **BN-LIMB64** | 32-bit limbs cap the achievable constant factor | `bignum.ch` | L |

Items 1-3 are the remaining *performance* work and are independent of each other;
together they are worth roughly 1.8x on private operations, 1.4x on key generation
and ~1.2x on public operations. Items 4-6 are the remaining *production* gaps.

TLS-BUILD-DIR and RSA-TINYKEY-FALLBACK are the two S-sized correctness/hygiene
items; everything else above is performance or scope. See
[Certificate-path validation](#certificate-path-validation-audited-2026-09-22) for the
`x509_verify_chain` audit (one live verification bypass, fixed; two gaps left).

---

## BN-MONT-SQR — modular squaring does twice the multiplies

**Symptom.** `2048-bit modexp with a 1025-bit exponent` costs 49 ms and did not
improve with the division work: it is `montgomery_mul`-bound, not reduction-bound.
Measured slope: 15.6 µs per exponent bit at 1024 bits, ~48 µs per bit at 2048 bits.

**Root cause.** `mpi_exp_mod`'s ladder calls `montgomery_mul(x, x, x, ...)` for
every squaring. CIOS computes `a[i]*a[j]` for all `n^2` operand pairs, so the
`i == j` terms (about half the work) are computed twice and the
`(a[i]*a[j], a[j]*a[i])` pairs are two identical multiplies.

**Why it is the top item.** Two thirds of the montmuls in any exponentiation are
squarings, and a dedicated squaring needs roughly `n^2/2` multiplies instead of
`n^2`. Expected: ~30% off every private operation, Miller-Rabin round and ECDSA
signature.

**Sketch.** Add `montgomery_sqr(x, a, n, n_inv0)` and call it from the ladder
wherever the multiplicands are the same pointer (`a == b` is already the signal —
the ladder passes `result` twice). The carry handling is the tricky part: the
doubled off-diagonal term `2*a[i]*a[j]` does not fit in 32 bits, so accumulate into
a 64-bit intermediate limb array rather than the CIOS `u32` window, or fold the
doubling into the borrow chain the way the existing multiply-subtract does.
Do **not** change `montgomery_mul`'s signature or semantics.

**Definition of done.** `TEST_bignum_exp_mod_matches_repeated_multiplication` and
the differential harness stay green, plus a new test that `montgomery_sqr(a)` and
`montgomery_mul(a, a)` agree for random operands (the ladder only exposes the
squaring path indirectly). Measured: exponentiation time drops by >=20% with the
per-bit slope re-measured.

## BN-WINDOW — square-and-multiply, no windowing

**Symptom.** The ladder multiplies for every set bit: ~0.5 multiplies per bit, so a
1025-bit exponent costs 1025 squarings + ~512 multiplies.

**Root cause.** `mpi_exp_mod` uses plain left-to-right binary exponentiation (both
the Montgomery path and `mpi_exp_mod_fallback`).

**Sketch.** A 4-bit window with a 16-entry Montgomery-form table turns ~512
multiplies into ~256 plus 14 setup multiplies; a signed-digit (NAF) window with 8
odd powers needs 7 setup multiplies and ~205 multiplies. Keep the existing plain
ladder for small exponents (`bitlen(e) < 32`): the public exponent 65537 is 17 bits
and would *lose* time to table setup. The table is 16 `Mpi`s (~32 KB of stack at
`MAX_LIMBS = 512`), or a per-call heap allocation if stack pressure matters.

**Definition of done.** Same tests as BN-MONT-SQR. Add a vector where the exponent
is a single high bit, one where it is all-ones, and one 1-bit exponent, so the
window boundary handling is pinned. Measured: multiply count per exponent bit drops
as predicted, and `e = 65537` does not regress.

## BN-MONT-CACHE — `R^2 mod N` recomputed on every `mpi_exp_mod`

**Symptom.** `mpi_exp_mod` with `e = 1` (i.e. the fixed cost: setup plus two
montmuls) measures 128 µs at 1024 bits and 355 µs at 2048 bits. For an RSA public
operation (17-bit exponent, 1.39 ms at 2048 bits) that fixed cost is ~20-25% of the
whole call.

**Root cause.** `compute_r2` runs on every entry to `mpi_exp_mod`, and callers that
exponentiate repeatedly with the same modulus pay it every time: Miller-Rabin's
rounds and its squaring loop, and the two halves of `rsa_private_crt`.

**Sketch.** Split out a `MontCtx { valid, n_inv0, r2 }`, build it once per modulus,
and add `mpi_exp_mod_mont(x, a, e, n, ctx)`; keep `mpi_exp_mod` as a wrapper that
builds a throwaway context. `rsa.ch` then holds one context per `RSAContext`
(built at keygen/import time) and reuses it across all exponentiation calls.
**Do not** introduce a module-level cache: two threads doing RSA with different
moduli would race and produce a wrong result — silently. That is the reason this
was not done as a memoisation inside `mpi_exp_mod`.

**Definition of done.** A test that builds a context once, runs several
exponentiations with it, and compares each against `mpi_exp_mod`. Measured: the
`e = 1` fixed cost drops to roughly one montmul, and keygen plus CRT private ops
improve in line with their exponentiation count.

## RSA-CRT-IMPORT — private-key import discards the CRT parameters

**Symptom.** CRT makes a 2048-bit private operation 3.4x faster, but only for keys
this library generated. A key loaded from a PEM/DER takes the slow path, because
`rsa_import_privkey` stores only `N` and `D`.

**Root cause.** `rsa_import_privkey(ctx, n_buf, n_len, d_buf, d_len)` has no
parameters for `P`, `Q`, `DP`, `DQ`, `QP`, and `ssl.ch` never imports an RSA
private key at all — only `x509_extract_rsa_pubkey` exists, so server-side RSA
signing/key-transport is not wired up. (TLS clients use RSA for encryption and for
verifying certificate signatures; both are public operations and already fast.)

**Sketch.** Add `rsa_import_privkey_crt(ctx, n, e, d, p, q)` computing
`DP = D mod (P-1)`, `DQ = D mod (Q-1)`, `QP = Q^-1 mod P` (all cheap now), and have
the ASN.1 parser in `ssl.ch` pass the components it already decodes. Keep
`rsa_crt_ready`'s structural check as the gate, so a partially-filled context falls
back rather than miscomputing.

**Definition of done.** A test that imports a key generated by Python
(`cryptography`), decrypts a ciphertext Python produced, and compares against
`pow(c, d, n)`. Measured: the imported key's private operation matches the
generated key's.

## RSA-PSS-OAEP — `RSA_PKCS_V21` is a constant with no implementation

**Symptom.** `RSA_PKCS_V21 = 1 // RSA-OAEP (not yet implemented)` in `rsa.ch`.
TLS 1.3 mandates RSA-PSS for RSA signatures
(`TLS1_3_SIG_RSA_PSS_RSAE_SHA256` etc. are already defined in `types.ch`), so an
RSA-authenticated TLS 1.3 handshake cannot complete with this library.

**Root cause.** Only PKCS#1 v1.5 encode/decode (`pkcs1_v15_encode`,
`pkcs1_v15_decode`) and v1.5 signature verification exist; there is no
EME-OAEP / EMSA-PSS encoding, no MGF1, and no `rsa_pkcs1_pss_sign`.

**Definition of done.** Known-answer tests against Python's
`cryptography` OAEP/PSS for both SHA-256 and SHA-384, a round-trip sign/verify test,
and a real TLS 1.3 handshake against an OpenSSL server configured for
`rsa_pss_rsae_sha256`. This is the largest item here and blocks RSA with TLS 1.3.

## RSA-BLINDING — private operations are not blinded or constant-time

**Symptom.** `rsa_private` and `rsa_private_crt` run on secret-dependent data with
no blinding, and `pkcs1_v15_decode` returns early on the first padding error.

**Root cause.** No `mbedtls_rsa_private`-style blinding step, and the unpadding
loop branches on input bytes. Both matter the moment the server side of a TLS
handshake signs or decrypts with an RSA key: unpadding is the classic
Bleichenbacher oracle, and an unblinded exponentiation leaks through timing.

**Definition of done.** Follow [`constant-time-crypto.md`](./constant-time-crypto.md):
add blinding (`c' = c * r^e`, then `m = m' * r^-1 mod N`) and make the unpadding
compare the whole candidate block instead of returning early. The test suite should
assert on behaviour (a corrupted padding byte still costs the same number of
iterations is hard to assert; assert that the return code and output are correct
for malformed padding) and the work should reference the constant-time doc's
methodology for measuring.

## BN-INIT-ZERO — every temp zeroes 512 limbs

**Symptom.** `mpi_init` zeroes all `MAX_LIMBS` (512) limbs and `mpi_copy` zeroes the
tail up to `MAX_LIMBS`. The hot paths create several temps per call, so a
2048-bit montmul pays ~2000 pure bookkeeping stores.

**Root cause.** `mpi_init`/`mpi_copy` are defensive about stale limbs above `.n`.
Reader discipline already means limbs `[.n, MAX_LIMBS)` are never read — every
consumer loops to `a.n` — so the zeroing is redundant for internal temporaries.

**Sketch.** Add internal `mpi_init_tmp` / `mpi_copy_n` that only set `s`/`n` and copy
`src.n` limbs, and use them **only** for locals that are fully written before being
read (the CIOS temporaries, `mpi_div_knuth`'s `U`/`V`/`Qd`/`Rr`, `mpi_exp_mod`'s
locals). Leave the public `mpi_init`/`mpi_copy` contract unchanged — module-level
`Mpi`s and anything reachable from C interop must stay zeroed.

**Definition of done.** The differential harness (which fuzzes sizes and signs
specifically to catch stale-limb reads) stays `ALL MATCH`, the TLS suite stays
green, and the profile shows the expected drop. This is S-sized but the only item
here that can turn into a silent wrong-answer bug, so it needs the fuzz harness
more than the others.

## BN-MODINV-EVEN — even-modulus inverse is still division-based

**Symptom.** `mpi_mod_inv` with an even modulus (key generation's
`D = E^-1 mod phi`, where `phi = (p-1)(q-1)` is even) keeps the Euclidean loop:
4.9 ms for 1024 bits, 19 ms for 2048 bits. Odd moduli use the binary GCD and are
4.2x faster.

**Why it is low priority.** It is paid twice per key generation (~25 ms against a
~1 s operation) and never on the handshake path (ECDSA's inverse is over an odd
prime and already takes the binary path).

**Sketch.** Either extend the binary form to even moduli, or fall back to a
limb-wise Lehmer/Euclid with the division dispatcher (the `m == 0` case is already
optimised, so the remaining cost is genuinely the number of steps).

**Definition of done.** The `inv_case(..., even)` vectors in the differential
harness stay green and the 1024/2048-bit even-modulus inverse drops to the same
order as the odd case.

## BN-ADD-BACK-KAT — the Algorithm D add-back branch has no test

**Symptom.** `mpi_div_knuth`'s "qhat was one too large: add the divisor back"
branch is never taken by the 459-case differential run, by the fuzz cases in
`bn_check`, or by the suite's random invariants.

**Root cause.** It is *supposed* to be unreachable in practice: the `qhat >= B`
test plus the `V[n-2]` refinement already reduce the estimate to at most one too
large, and hitting the remaining case needs a specific digit pattern. I searched
for a trigger (structured and randomised, ~60k candidates across the divisor
shapes that maximise the normalisation shift) and found none, so the branch has no
known vector.

**Why it still matters.** It is the safety net for a case the analysis says cannot
happen; if it were deleted or broken by a refactor, nothing would notice until the
one divisor shape that needs it appeared in production.

**Definition of done.** Either a vector that takes the branch (search harder: fix
`n = 2`, sweep the full 4-limb window space rather than a specials list, and
denormalise by `W >> s` with the low `s` bits cleared), or a test that drives the
branch through a test-only entry point, or a comment in the source recording the
exhaustive search and why it is unreachable. Document whichever one lands.

## BN-HARNESS — the differential harness is not in the repo or CI

**Symptom.** The 459-case Python differential run, the perf probe and the RSA CRT
verifier live in `lang/compiled/` (gitignored). They existed for one session and a
future reader has to rebuild them from this doc.

**Root cause.** `lang/compiled/` is the scratch directory by convention, and there
is no "slow arithmetic" suite.

**Sketch.** Promote the harness to `lang/tests/tls/src/` as a `@test` that generates
N random cases per run (the lane already has Python plumbing:
`test_python_run_script`, `test_parse_py_hex_label`, and the four `INT_*` tests in
`bignum_tests.ch` are the reduced form of it). Keep the case count small enough that
the suite stays fast, and let CI accumulate coverage across runs.

**Definition of done.** The harness runs under `./scripts/test.sh --tcc --tls`, and
the standalone probe can be deleted from `lang/compiled/`.

## BN-PERF-GUARD — no regression guard on the arithmetic hot paths

**Symptom.** The division fix was worth up to 40x and nothing in CI would have
caught its loss: the suite is all correctness, and the only timing assertions are
the generous per-test budgets in `tls_coverage_tests.ch`.

**Root cause.** No benchmark or budget check for the bignum.

**Sketch.** A `@test` that times a fixed workload (e.g. 200 reductions of a
2048-bit value by a 1024-bit modulus, 20 inverses) and fails only on a large
absolute budget — not a tight comparison, since the machine load in the suite is
variable. It exists to catch a 5x regression, not a 5% one.

**Definition of done.** The budget is set from measurement on a loaded machine
(with the reasoning recorded), and deliberately breaking `mpi_div`'s dispatch
makes it fail.

## RSA-TINYKEY-FALLBACK — encryption silently degrades to raw RSA

**Symptom.** When `message_len + 11 > ctx.len`, `rsa_pkcs1_encrypt` silently falls
back to raw RSA on the first `ctx.len` bytes, and `rsa_pkcs1_decrypt` does the same
for `ctx.len < 11`. It returns 0, so callers cannot tell.

**Root cause.** Added to keep toy-size test keys working:
`pkcs1_v15_encode` returns `ERR_RSA_OUTPUT_TOO_LARGE`, and the caller treats that as
"key too small, do it anyway".

**Why it is a real risk.** Raw RSA is not encryption. Any caller that hits the
fallback gets a value that looks like a ciphertext and is not. Real keys are >= 128
bytes so the branch is unreachable in production, which is exactly why it will go
unnoticed.

**Sketch.** Return `ERR_RSA_OUTPUT_TOO_LARGE` (mbedTLS behaviour) and adjust the
toy-key tests to either use `>= 11`-byte keys, call `rsa_public` directly, or assert
the error. If a raw-RSA entry point is genuinely wanted for tests, name it
`rsa_raw_public` so the degradation is explicit.

**Definition of done.** No silent path returns success without PKCS#1 padding; the
suite is green; a test asserts the error for an undersized key.

## TLS-BUILD-DIR — `lang/tests/tls/build/` is not gitignored

**Symptom.** Running the suite the task asks for leaves an untracked
`lang/tests/tls/build/` (`lab`, `modules`, `object_tcc.o`) in the tree.

**Root cause.** `.gitignore` has `lang/tests/build` (the main suite) but no rule for
the per-suite directories.

**Definition of done.** A `.gitignore` rule covering the per-suite build dirs
(`lang/tests/*/build/`), verified with `git status --porcelain` after a suite run.

## BN-LIMB64 — 32-bit limbs cap the achievable constant factor

**Symptom.** After the reduction work, everything is proportional to `n^2` 32-bit
multiplies, and each is a separate multiply-add step with casts; a 2048-bit montmul
is ~40-55 µs and a private operation is 45 ms.

**Root cause.** `BITS_PER_LIMB = 32` with `[512]u32` storage. 64-bit limbs would
halve `n` (and therefore quarter the `n^2` inner loop) at the cost of needing 128-bit
intermediates, which Chemical does not have — it would need the arm/x86 widening
multiply intrinsics (see the atomic/inline-asm patterns in `AGENTS.md`).

**Why it is last.** It is a rewrite of every limb loop including the ones just
verified, it changes the `Mpi` layout (and `MAX_LIMBS` semantics), and the payoff
overlaps with items 1-3 on the same hot loop. Do those first and re-measure; if
private operations are still too slow after BN-MONT-SQR and BN-WINDOW, this is the
next step, and it should be attempted behind the differential harness with the limb
width as a compile-time constant rather than a fork.

**Definition of done.** Not defined until items 1-3 land and the remaining cost is
measured and attributed.

---

## Certificate-path validation (audited 2026-09-22)

Not bignum work, but the same kind of question — "what does mbedTLS do here that
let us get away with not doing" — applied to `x509_verify_chain` in `ssl.ch`.
Three cases were examined; the first was a live verification bypass and is fixed.

### What was found and fixed

**A self-signed leaf was its own trust anchor.** The chain walk accepted a
self-signed certificate whenever its signature verified against its own key,
*regardless of the trust store*. Reproduced with the test certificate and a store
holding the same DER with its subject DN overwritten (so no subject lookup could
match): `x509_verify_chain(leaf, unrelated_store, "test.example.com")` returned 0.
With a full system CA bundle configured, any attacker's self-signed certificate for
the requested hostname therefore passed verification. Fixed by gating that branch on
`trusted_ca == null` (step 4a already accepts a self-signed certificate whose
subject the store *does* hold). Pinned by
`tls_self_signed_leaf_is_not_trusted_by_an_unrelated_store`, which also asserts the
control case and the unchanged no-store behaviour.

The existing `INT_https_untrusted_ca_fails` could not catch this: its server chains
to a proper (non-self-signed) leaf, which fails at step 4c anyway.

### X509-ANCHOR-DATE — the trust anchor's validity window is never checked

**Symptom.** Step 3 checks the leaf's dates and step 4b checks each intermediate's,
but the branch that terminates the walk at a trusted root (4a) returns success
without ever calling `x509_check_date` on that root. A leaf signed by an expired
certificate that is still present in a system bundle verifies.

**Root cause.** `x509_check_date` is called on the certificate being verified, not
on the anchor it chains to. mbedTLS checks every certificate in the path.

**Sketch.** Call `x509_check_date(ca_issuer)` before accepting it in 4a and OR the
result into `leaf.flags`, exactly as 4b does for intermediates. Decide explicitly
whether an expired anchor is a hard failure or a flag — mbedTLS makes it
`MBEDTLS_X509_BADCERT_EXPIRED` on the chain, which fails verification unless the
caller cleared that flag.

**Definition of done.** A test whose leaf is signed by a root whose validity window
is entirely in the past (both in the trust store) fails verification, and the
existing chain tests stay green.

### X509-CA-FLAG — basicConstraints CA=TRUE is never enforced

**Symptom.** Step 4b accepts any certificate in the peer chain as the signer of the
next one, as long as its subject matches the issuer and the signature checks out.
`crt.ext_is_ca` is parsed but never consulted.

**Root cause.** No `mbedtls_x509_crt_check_key_usage`/`is_ca` equivalent in the walk.

**Why it matters.** Anyone holding an ordinary end-entity certificate (a CA hands
these to any requester) can use its private key to sign a certificate naming a
domain they do not control, and present `[forged, their-real-leaf]` as the chain:
the forged leaf's issuer matches their real leaf's subject, and the real leaf chains
to a trusted root. The CA bit is the only thing that stops this.

**Sketch.** Reject a peer-chain signer whose `ext_is_ca` is false, and consider
`ext_max_pathlen` while walking. Non-CA certificates are still valid as the *leaf*.

**Definition of done.** A test with a forged leaf issued by a non-CA end-entity
certificate that itself chains to the trusted root fails verification, while the
same PKI with CA=TRUE on the intermediate still verifies.

---

## Appendix A — measured primitive costs (TCC, `debug_quick`, one run)

| primitive | 1024-bit | 2048-bit |
|---|---|---|
| `mpi_mul` | 12.5 µs | 42 µs |
| `mpi_mod` (2n % n) | 13.6 µs | 42 µs |
| `mpi_div` (2n / n) | 17 µs | 41 µs |
| `mpi_mod_inv` (odd) | 1199 µs | 4315 µs |
| `mpi_exp_mod` (`e = 1`, fixed cost) | 128 µs | 355 µs |
| `mpi_exp_mod` (17-bit `e`, the public exponent's shape) | 363 µs | 1225 µs |
| `mpi_exp_mod` (full-size `e`) | 7987 µs (513-bit e) | 49089 µs (1025-bit e) |

Derived: ~15.6 µs per exponent bit at 1024 bits, ~48 µs at 2048 bits, so one
montgomery_mul is roughly 8-15 µs and 40-55 µs respectively. Squaring is 2/3 of
those steps (item BN-MONT-SQR) and the fixed 128/355 µs is `compute_r2`
(item BN-MONT-CACHE).

## Appendix B — why keygen timings in this library cannot be trusted

`rsa_gen_key` measurements swing from ~200 ms to ~6 s for the *same* binary, because
the cost is dominated by how many Miller-Rabin modexps the prime search happens to
run, which depends on how many candidates survive the sieve and how often `P·Q`
lands short of the target bit length (in which case `Q` is regenerated from
scratch). A keygen measurement is only meaningful as a median of many runs, and a
5x "regression" in a single sample is noise. The documented key-search cap is 20000
candidates, which is ~55 standard deviations above the 1024-bit mean (355, same
order as its own standard deviation) — if that bound is ever actually hit, the RNG
is broken, not unlucky.
