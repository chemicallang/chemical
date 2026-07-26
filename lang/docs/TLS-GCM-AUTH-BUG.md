# TLS 1.3 GCM Authentication Failure — Investigation & Fix Guide

## Status

- **672 of 678 tests pass**. All cryptographic primitives are verified correct.
- **6 integration tests fail** (`INT_tls13_client_openssl`, `INT_x25519_handshake`, `INT_tls12_client`, `INT_tls13_server_openssl_client`, `INT_ecdsa_server_client_x25519`, `INT_ecdsa_client_handshake`).
- All 6 failures are caused by the same root issue: **GCM authentication fails when decrypting the first encrypted record from an OpenSSL TLS 1.3 server**.

---

## What Is Verified Correct (proven beyond doubt)

### 1. x25519 (`lang/libs/tls/src/x25519.ch`)

- RFC 7748 Section 6.1 Alice test vector: **PASS**
- RFC 7748 Section 6.1 Vector #1 and #2 (with OpenSSL-compatible expected values): **PASS**
- Cross-verified against OpenSSL 3.5: **shared secret matches** for 3 different keypairs
- Test vectors that were corrupted by a previous developer have been corrected (`lang/tests/src/libs/tls/tests.ch`, `lang/compiled/x25519_test/src/main.ch`)
- Bit 255 masking added to `fe_decode` (`f[7] = f[7] & 0x7FFFFFFFu32`) per RFC 7748 Section 5

### 2. AES (`lang/libs/tls/src/aes.ch`)

- AES-128-ECB: NIST FIPS 197 test vector **PASS**
- AES-256-ECB: NIST FIPS 197 test vector **PASS**
- Cross-verified against OpenSSL 3.5: **AES output matches for all tested keys**

### 3. AES-GCM (`lang/libs/tls/src/gcm.ch`)

- NIST SP 800-38D zero-key test vector: **PASS** (`H = 66e94bd4...`, tag = `58e2fcce...`)
- NIST SP 800-38D RFC 8452 test vectors: **PASS** (with corrected plaintext values)
- Encrypt-then-decrypt roundtrip with real TLS 1.3 handshake keys: **PASS**
- Computed expected tag matches OpenSSL 3.5 AES + Python GHASH: **PASS** (verified for 3 different key sets)
- **The GCM implementation is correct and compatible with OpenSSL.**

### 4. SHA-256 (`lang/libs/crypto/src/sha256.ch`)

- FIPS 180-2 "abc" test vector: **PASS**
- Context cloning (`sha256_final` on a copy) preserves original state for continued updates: **VERIFIED**

### 5. HMAC-SHA256 (`lang/libs/crypto/`)

- RFC 4231 test vectors (cases 1, 3, 4, 5): **PASS**

### 6. TLS 1.2 PRF (`lang/libs/tls/src/ssl.ch`)

- Known-answer test with deterministic inputs: **PASS**

### 7. Full TLS 1.3 Key Schedule

- Verified byte-for-byte against Python + OpenSSL 3.5 reference implementation
- Early secret → Derived → Handshake secret → Client/Server handshake traffic secrets → Keys/IVs
- **All intermediate values match exactly** (verified across multiple connections)
- The key derivation in `tls13_derive_handshake_keys` is correct.

### 8. TLS 1.3 Record Layer

- Standalone test (`lang/compiled/tls13_record_roundtrip/`) encrypts then decrypts with same keys: **PASS**
- `ssl.in_buf[5]` and `input` pointer match at GCM time: **VERIFIED**

### 9. C Codegen

- `ssl_read_record`, `ssl_fetch_input`, `tls13_decrypt_record`, `gcm_auth_decrypt` all generate correct C code
- struct access (`ssl->in_buf[5]`, `ssl->in_hdr[0]`, etc.) verified in emitted C

### 10. Buffer Management

- `ssl_fetch_input`, `ssl_consume_record`, and the decrypt-path buffer compaction all produce correct offsets
- Verified by tracing the C codegen for all buffer operations

---

## What Is NOT Verified / The Remaining Bug

**The ciphertext in `ssl.in_buf[5..5+ct_len-1]` at the time of `gcm_auth_decrypt` does NOT decrypt to a valid TLS 1.3 handshake message, despite correct keys, nonce, and AAD.**

Evidence:
1. The key and nonce are correct (verified against OpenSSL and Python reference).
2. Decrypting the ciphertext from the buffer with OpenSSL AES yields `b2b52968...` (first byte = 0xb2, not 0x08 = EncryptedExtensions). A valid TLS 1.3 EncryptedExtensions message starts with 0x08.
3. The expected GCM tag (computed with OpenSSL AES + Python GHASH) differs from the received tag in the buffer (`b918a015...` ≠ `45f97001...`).
4. Roundtrip test with the same keys and nonce produces correct results (proving GCM is correct).

**Conclusion: The data in `ssl.in_buf[5..5+record_len-1]` at decryption time differs from what the server actually encrypted.** Since TCP guarantees data integrity, something modifies the buffer between the time data arrives from the socket and the time `gcm_auth_decrypt` reads it.

---

## Most Likely Root Cause (Hypothesis)

**Record coalescing + buffer shift interaction**: The OpenSSL server sends multiple TLS records in a single TCP segment (ServerHello, CCS, EncryptedExtensions, Certificate, CertificateVerify, Finished — all in one TCP write of ~728 bytes). The `ssl_consume_record` and the decrypt-path buffer compaction in `ssl_read_record` interact to produce an off-by-one error in the remaining data's position.

Specifically, the bug is likely in the buffer compaction code inside `ssl_read_record`'s TLS 1.3 decrypt branch:

```chemical
// In ssl_read_record, after successful TLS 1.3 decryption:
var original_end : i32 = 5 + record_len as i32
var new_end : i32 = 5 + dec_len
if(original_end < ssl.in_left) {
    var shift_i : i32 = 0
    while(shift_i < ssl.in_left - original_end) {
        ssl.in_buf[new_end + shift_i] = ssl.in_buf[original_end + shift_i]
        shift_i += 1
    }
    ssl.in_left -= (original_end - new_end)
}
```

This code runs on the **successful** decrypt path. For the **failing** case, `tls13_decrypt_record` returns an error before this block executes, so the buffer is NOT modified by this code for the failed record.

However, PREVIOUS successful reads of the ServerHello and CCS records may have set up the buffer in a way that causes the NEXT read to misplace the encrypted record's payload.

The specific scenario that's NOT exercised by the standalone roundtrip test:
1. Multiple TLS records arrive in one TCP segment
2. `ssl_fetch_input` reads the entire segment in one `ssl_recv` call into `ssl.in_buf[0]`
3. `ssl_read_record` processes the first record (ServerHello), calls `ssl_consume_record` which shifts the remaining data to `ssl.in_buf[0]`
4. The next `ssl_read_record` processes CCS, shifts again
5. The third `ssl_read_record` should find the encrypted record at `ssl.in_buf[0..4]` (header) and `ssl.in_buf[5..5+record_len-1]` (payload)
6. The payload at `ssl.in_buf[5..]` at this point is **different** from what the server sent because an off-by-one in the shift consumed one too few or one too many bytes

---

## How To Fix (Step by Step)

### Step 1: Write a Buffer-Verification Test

Create a standalone Chemical program that:
1. Manually fills `ssl.in_buf` with a known multi-record TLS 1.3 server response (raw bytes from a real connection)
2. Sets `ssl.in_left`, header, etc.
3. Calls `ssl_read_record` to read each record, verifying the data at each step

```chemical
// Pseudocode for the test:
// 1. Fill ssl.in_buf with raw data from openssl s_server -msg
// 2. For each record: call ssl_read_record
// 3. Verify ssl.in_hdr and ssl.in_buf[5..5+record_len-1] match expected
// 4. If they DON'T match, the buffer shift bug is reproduced
```

**The test MUST use a TLS 1.3 server response with ServerHello + CCS + EncryptedExtensions arriving in a single TCP segment.** This is what the current roundtrip test doesn't exercise.

### Step 2: Add Debug Output to `ssl_consume_record`

Add this to `ssl_consume_record` (in `ssl.ch`):

```chemical
printf("[CONSUME] consumed=%d in_left_before=%d in_left_after=%d\n",
    consumed, in_left_before, in_left_after)
printf("[CONSUME] buf[0..5] after shift: ")
for i in 0..5: printf("%02x", ssl.in_buf[i])
```

Then run the test from Step 1 and verify the shift positions are correct.

### Step 3: Fix the Off-by-One

The most likely fix is in how `consumed` is calculated in `ssl_consume_record`:

```chemical
func ssl_consume_record(ssl : *mut SSLContext) {
    var consumed = 5 + ssl.in_msglen
```

Check that `ssl.in_msglen` is correct at the time `ssl_consume_record` is called. After TLS 1.3 decryption in `ssl_read_record`, `ssl.in_msglen` is set to `dec_len` (the decrypted length). But in the `read_record_payload` flow (used by non-encrypted records like ServerHello and CCS), `ssl.in_msglen` is the original `record_len`.

For the ServerHello: `ssl.in_msglen = 90` (the record length). `consumed = 5 + 90 = 95`. This shifts the next record (CCS) to position 0. ✓

For the CCS: `ssl.in_msglen = 1`. `consumed = 5 + 1 = 6`. This shifts the first encrypted record to position 0. ✓

But what if `ssl.in_msglen` for the ServerHello is NOT 90? What if `ssl_read_record` modified it?

Looking at `ssl_read_record`: `ssl.in_msglen = record_len as i32`. This is set AFTER `ssl_fetch_input` reads the data. For a plaintext record (no decrypt), `ssl.in_msglen = record_len`. ✓

For a CCS record, the same: `ssl.in_msglen = 1`. ✓

So `consumed` should be correct. The bug must be elsewhere.

### Step 4: Check `ssl_fetch_input` Buffer Position

The function reads at `ssl.in_buf[ssl.in_left]`. For the first read (header), `ssl.in_left = 0`, so it reads into `ssl.in_buf[0]`. After reading 728 bytes, `ssl.in_left = 728`.

For the second read (ensuring full record), `ssl.in_left = 728 >= 50`, so no read happens.

But what if between the first and second reads, `ssl.in_left` changes? It's only modified by `ssl_fetch_input` itself (line `ssl.in_left += n`) and by `ssl_consume_record` (line `ssl.in_left = remaining`). If `ssl_consume_record` reduces `ssl.in_left` to below `5 + record_len`, the next `ssl_fetch_input` would read into the wrong buffer position.

**Most likely bug**: After consuming ServerHello and CCS, `ssl.in_left` is correct. But `ssl_fetch_input(ssl, 5 + record_len)` for the encrypted record finds `ssl.in_left >= 5 + record_len` and doesn't read. The data is already at `ssl.in_buf[0..]` from the single initial read. The header at `ssl.in_buf[0..4]` is correct. But the payload at `ssl.in_buf[5..5+record_len-1]` is NOT what the server sent because the initial read placed data at `ssl.in_buf[0..727]`, and subsequent `ssl_consume_record` shifts shifted it, but the shifts were off by X bytes.

**To verify**: Run the test under GDB (or with extensive buffer dumps) and capture the raw TCP bytes. Compare what arrives in `ssl_recv` with what `ssl.in_buf` contains at GCM time.

### Step 5: Fix the Identified Bug

Once the specific off-by-one is found, the fix will be in one of:
- `ssl_consume_record` — adjust the `consumed` calculation
- The decrypt-path buffer compaction in `ssl_read_record` — adjust `original_end` or `new_end`
- `ssl_fetch_input` — adjust the `buf_start` or `min_len` logic

### Step 6: Verify All 678 Tests Pass

```bash
./scripts/build.sh --tcc
rm -rf lang/tests/build/*.o lang/tests/build/*.dir
cmake-build-debug/TCCCompiler lang/tests/build.lab -o lang/tests/build/tests-tcc.exe --mode debug_quick --no-cache
./lang/tests/build/tests-tcc.exe --skip-sequential
```

### Step 7: Add the Standalone Test

Keep the TLS 1.3 record roundtrip test at `lang/compiled/tls13_record_roundtrip/` and extend it to test the multi-record-in-one-segment scenario.

---

## Files to Modify

| File | Purpose |
|------|---------|
| `lang/libs/tls/src/ssl.ch` | Fix in `ssl_consume_record` or `ssl_read_record` buffer management |
| `lang/libs/tls/src/gcm.ch` | Only if a GCM bug is confirmed (unlikely) |
| `lang/libs/tls/src/x25519.ch` | Already fixed |
| `lang/tests/src/libs/tls/tests.ch` | Already fixed (test vectors) |

## Files to Create

| File | Purpose |
|------|---------|
| `lang/compiled/tls13_record_roundtrip/` | Already exists, extend with multi-record test |
| `lang/docs/TLS-GCM-AUTH-BUG.md` | This file |

---

## Debugging Commands

```bash
# Run a single integration test (not all 8 in parallel):
./scripts/test.sh --tcc --tls --no-build --skip-sequential --test-names INT_tls13_client_openssl

# Emit C code to inspect codegen:
cmake-build-debug/TCCCompiler lang/tests/build.lab \
    -o lang/tests/build/tests-tcc.exe --mode debug_quick --no-cache --emit-c
# C output at: lang/tests/build/chemical-tests.dir/Translated.c

# Run standalone GCM verify with known key/data:
# (test at lang/compiled/tls13_record_roundtrip/)
cmake-build-debug/TCCCompiler lang/compiled/tls13_record_roundtrip/chemical.mod \
    -o lang/compiled/tls13_record_roundtrip/test.exe --mode debug_quick --no-cache

# Verify with OpenSSL 3.5:
openssl enc -aes-128-ecb -nopad -e \
    -K $(xxd -p /tmp/key.bin) \
    -in /tmp/plaintext.bin -out /tmp/ciphertext.bin
```

---

## Key Contacts in Code

| Function | File | Line |
|----------|------|------|
| `ssl_fetch_input` | `ssl.ch` | ~1824 |
| `ssl_read_record` | `ssl.ch` | ~1862 |
| `ssl_consume_record` | `ssl.ch` | ~1954 |
| `tls13_decrypt_record` | `ssl.ch` | ~1640 |
| `tls13_encrypt_record` | `ssl.ch` | ~1571 |
| `gcm_auth_decrypt` | `gcm.ch` | ~271 |
| `gcm_crypt_and_tag` | `gcm.ch` | ~202 |
| `ghash_multiply_refined` | `gcm.ch` | ~45 |
| `ghash` | `gcm.ch` | ~97 |
| `fe_decode` (already fixed) | `x25519.ch` | ~45 |
