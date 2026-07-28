# TLS Integration Test Failures (RESOLVED)

## Summary

All 24 TLS integration tests now PASS when run via `./scripts/test.sh --tcc --tls`. Chemical ↔ Python 3.14 / OpenSSL 3.5.5.

**Status**: 24/24 integration tests pass. Main test suite: 2014/2014 pass.

## Resolution of Test Failures

### 1. TLS 1.2 Finished Transcript Hash (RFC 5246 Section 7.4.9)
- **Bug**: `ServerFinished` calculation used `hs_hash` (transcript hash of CH...CKE) without including `ClientFinished`.
- **Fix**: In both `do_tls12_client_handshake` and `do_tls12_server_handshake`, `ClientFinished` is hashed into `hash_ctx` before computing `ServerFinished`.

### 2. TLS 1.2 Unencrypted ChangeCipherSpec
- **Bug**: `ssl.transform_out` was installed before `send_record(SSL_MSG_CHANGE_CIPHER_SPEC)`, causing `ChangeCipherSpec` to be sent encrypted.
- **Fix**: `ChangeCipherSpec` is sent in the clear first, then `ssl.transform_out` and `ssl.transform_in` are installed and sequence numbers reset.

### 3. TLS Version Check in `ssl_read_record`
- **Bug**: `ssl_read_record` entered the TLS 1.3 decrypt block whenever `in_hdr[0] == SSL_MSG_APPLICATION_DATA (23)` without checking `ssl.tls_version >= SSL_VERSION_TLS1_3`.
- **Fix**: Added `ssl.tls_version >= SSL_VERSION_TLS1_3` guard so TLS 1.2 application data records use `tls12_decrypt_record`.

### 4. TLS 1.3 Finished Message Length (RFC 8446 Section 4.4.4)
- **Bug**: TLS 1.3 `Finished` messages were sending and expecting 12-byte `verify_data` (TLS 1.2 length) instead of 32 bytes (SHA-256 digest length). OpenSSL threw `BAD_DIGEST_LENGTH`.
- **Fix**: Updated `Finished` message generation and verification in both `do_tls13_server_handshake` and `do_tls13_client_handshake` to use 32-byte `verify_data`.

### 5. Client Finished Verification & Application Keys in `do_tls13_server_handshake`
- **Bug**: `do_tls13_server_handshake` did not verify `ClientFinished` or hash `ClientFinished` into `transcript` before calling `sha256_final` to derive application traffic keys.
- **Fix**: Added `ClientFinished` verification and hashed `ClientFinished` into `transcript` before calling `tls13_derive_application_keys`.

## Certified Verified (proven against Python output)

### Crypto
- ✅ HKDF-Expand-Label: matching keys for TLS 1.3 handshake
- ✅ GCM encrypt/decrypt: matching ciphertext + tag
- ✅ TLS 1.2 PRF: matching `master_secret` and `key_block`
- ✅ RSA PKCS#1 v1.5: Python keylog proves `master_secret` matches
- ✅ X25519 ECDHE: `INT_x25519_handshake` passes
- ✅ ECDSA signing: produces valid DER signatures (Python-parsed correctly)

### Keylog Proof
```
Python keylog:  CLIENT_RANDOM <cr> <master_secret>
Chemical debug: master_secret=<identical hex>
→ RSA encryption/decryption works
→ PRF works
→ mismatch is in transcript hash for INT_tls12_client
```

## Remaining Failures

### INT_tls13_server_client & INT_ecdsa_server_client_x25519

**Current error**: `BAD_DIGEST_LENGTH` from Python's OpenSSL.

This error comes from `tls13_hkdf_expand_label` in OpenSSL — it means `EVP_MD_size(md) != hashlen` or `EVP_MD_size(md) <= 0`. It's a KEY SCHEDULE error, not a CertificateVerify or handshake error.

Since the Chemical CLIENT test (INT_tls13_client) passes against Python server, the HKDF is correct for the client role. The error appears only when Chemical is the SERVER. The same `tls13_derive_handshake_keys` function runs in both roles.

**To investigate**: 
1. Check if OpenSSL rejects the server's `supported_versions` or cipher suite → leads to wrong hash function selection
2. Verify the Finished message verification — the `server_handshake_traffic_secret` might differ even though the handshake traffic secret matches
3. Check if Python 3.14/OpenSSL 3.5.5 has a security-level check that rejects non-PFS ciphers or certain extensions

### INT_tls12_client

**Current error**: `ERR_SSL_HANDSHAKE_FAILURE` (Finished verify_data mismatch).

Master secret matches (keylog proof). PRF matches. The mismatch is in the transcript hash.

**Next steps**: Add per-message SHA-256 logging at each `ssl_hash_handshake_msg` call. Compare with a Python script that reconstructs the hash from the raw handshake bytes captured via socat.

## CertificateVerify Implementation Details

### ECDSA Signing (ecdsa.ch)
- `ecdsa_import_privkey(ctx, key_bytes, 32, TLS_GROUP_SECP256R1)` — imports raw 32-byte scalar
- `ecdsa_sign(ctx, hash, 32, sig_out, &sig_len)` — ECDSA sign with random k, DER encode
- DER encoding: handles sign byte (0x00 prefix when MSB set), short-form SEQUENCE
- All loop-scoped variables (`r_bytes`, `s_bytes`, `r_body_len`, `s_body_len`) declared outside while loop

### CertificateVerify message (ssl.ch)
- Content to sign: 64 spaces + `"TLS 1.3, server CertificateVerify"` + 0x00 + transcript_hash
- transcript_hash = SHA-256(CH + SH + EE + Cert)
- Content is SHA-256 hashed, then signed with ECDSA
- Signature algorithm: `0x0403` (ecdsa_secp256r1_sha256)
- Message format: `SignatureScheme(2) + sig_len(2) + signature(sig_len)`

### Private key loading
- Python `privkey` command extracts EC scalar from PEM via `cryptography`
- Chemical `ec_privkey_load_hex_file` reads 64-char hex, converts to 32 bytes, imports via `ecdsa_import_privkey`
- Test sets `cfg.own_key = priv_key as *mut void`

## Key Files

| File | Purpose |
|------|---------|
| `lang/libs/tls/src/ssl.ch` | Full SSL: handshake, record layer, HKDF, PRF, CertificateVerify |
| `lang/libs/tls/src/ecdsa.ch` | ECDSA context, signing, DER encoding |
| `lang/libs/tls/src/gcm.ch` | GCM encrypt/decrypt |
| `lang/libs/tls/src/x509_crt.ch` | PEM loading, DER parsing, certificate verification |
| `lang/libs/tls/src/rsa.ch` | RSA PKCS#1 encrypt/decrypt/sign |
| `lang/libs/tls/src/x25519.ch` | X25519 ECDHE |
| `lang/libs/net/posix/platform_api.ch` | `set_blocking`, `set_nonblocking`, fcntl |
| `lang/libs/net/src/main.ch` | `recv_all` (single syscall), `send_all` (loops) |
| `lang/tests/tls/src/e2e.ch` | Integration tests |
| `lang/tests/tls/src/helpers.ch` | Python script generator (cert, privkey, srv, cli) |

## Debugging Infrastructure

### SSL keylog (both client and server)
```python
ctx.keylog_filename = '/tmp/tls_keylog_' + port + '.txt'
```
TLS 1.3 format: `SERVER_HANDSHAKE_TRAFFIC_SECRET <client_random> <secret>`
TLS 1.2 format: `CLIENT_RANDOM <client_random> <master_secret>`

### Running single test
```bash
./scripts/test.sh --tcc --tls --no-build --test-ids 1073741832  # INT_tls13_server_client
./scripts/test.sh --tcc --tls --no-build --test-names INT_tls12_client
```

### Python HKDF verification
```python
import hashlib, hmac, struct
def hkdf_extract(s, ikm):
    return hmac.HMAC(s, ikm, hashlib.sha256).digest()
def hkdf_expand_label(secret, label, ctx, length):
    lbl = b"tls13 " + label.encode()
    s = struct.pack(">HB", length, len(lbl)) + lbl + struct.pack(">B", len(ctx)) + ctx
    out, T = b"", b""
    for i in range(1, 256):
        T = hmac.HMAC(secret, T + s + bytes([i]), hashlib.sha256).digest()
        out += T
        if len(out) >= length: return out[:length]
    return out[:length]
```

### Wire-level capture
```bash
# Requires: python3 /tmp/tls_utils.py cert ...  (generates cert)
# Chemical server on port 19988:
cmake-build-debug/TCCCompiler ...  # standalone module
# Python client via socat proxy:
socat -x TCP-LISTEN:19989,reuseaddr TCP:127.0.0.1:19988
python3 /tmp/tls_utils.py cli 127.0.0.1 19989 1.3
```

### Git hashes for reference
```
cd2e9b84e tls: fix certificate DER lifetime, update failure docs
26895d7f6 updates and fixes for tls         ← CertificateVerify + ecdsa signing
e0ce2038c updates to the tls lib           ← set_blocking + docs
fb854dbb7 tls library fixes                ← double seq_num fix
```

## Environment
- Python 3.14.4, OpenSSL 3.5.5 (27 Jan 2026)
- TCCCompiler backend (Chemical → C → TinyCC)
- All tests: localhost TCP
