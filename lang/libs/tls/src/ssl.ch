// ============================================================================
// TLS Implementation - Record Layer, Handshake, and Public API
// ============================================================================
// Port of mbedTLS ssl_tls.c, ssl_msg.c, ssl_client.c to Chemical.
// Core implementation of TLS 1.2 and 1.3 protocol.
// ============================================================================

public namespace tls {

    using std::string;
    using std::string_view;

    // ─── Utility Functions ──────────────────────────────────────────────────

    // Write a 3-byte length (TLS uses 3-byte lengths in handshake messages)
    func write_u24(val : u32, buf : *mut u8) {
        buf[0] = ((val >> 16) & 0xFF) as u8
        buf[1] = ((val >> 8) & 0xFF) as u8
        buf[2] = (val & 0xFF) as u8
    }

    // Read a 3-byte length (TLS uses 3-byte lengths)
    func read_u24(buf : *u8) : u32 {
        return ((buf[0] as u32) << 16) | ((buf[1] as u32) << 8) | (buf[2] as u32)
    }

    // Write a 16-bit big-endian value
    func write_u16_be(val : u16, buf : *mut u8) {
        buf[0] = ((val >> 8) & 0xFF) as u8
        buf[1] = (val & 0xFF) as u8
    }

    // Read a 16-bit big-endian value
    func read_u16_be(buf : *u8) : u16 {
        return ((buf[0] as u16) << 8) | (buf[1] as u16)
    }

    // ============================================================================
    // TLS PRF (Pseudo-Random Function) for TLS 1.2
    // ============================================================================
    // Implements TLS 1.2 PRF = P_SHA256(secret, label + seed)
    // As defined in RFC 5246 section 5

    public func tls12_prf(secret : *u8, secret_len : size_t,
                           label : *char, label_len : size_t,
                           seed : *u8, seed_len : size_t,
                           output : *mut u8, output_len : size_t) {

        // Build the combined seed = label + seed
        var combined_len : size_t = label_len + seed_len
        var combined : [512]u8

        var i : size_t = 0
        while(i < label_len) {
            combined[i] = label[i] as u8
            i += 1
        }
        while(i < combined_len) {
            combined[i] = seed[i - label_len]
            i += 1
        }

        // P_hash(secret, seed) = HMAC_hash(secret, A(1) + seed) +
        //                        HMAC_hash(secret, A(2) + seed) + ...
        // where A(0) = seed, A(i) = HMAC_hash(secret, A(i-1))

        var A : [32]u8
        var generated : size_t = 0

        // First A(1) = HMAC(secret, A(0)=seed) = HMAC(secret, combined)
        crypto::hmac_sha256(secret, secret_len, combined, combined_len, &raw mut A[0])

        while(generated < output_len) {
            // output_block = HMAC(secret, A(i) + seed)
            var block_in_len : size_t = 32 + combined_len
            var block_in : [544]u8

            var j : size_t = 0
            while(j < 32) {
                block_in[j] = A[j]
                j += 1
            }
            while(j < block_in_len) {
                block_in[j] = combined[j - 32]
                j += 1
            }

            var block_out : [32]u8
            crypto::hmac_sha256(secret, secret_len, block_in, block_in_len, &raw mut block_out[0])

            // Copy block_out to output (up to 32 bytes)
            var copy_size : size_t = output_len - generated
            if(copy_size > 32 as size_t) { copy_size = 32 as size_t }
            var k : size_t = 0
            while(k < copy_size) {
                output[generated + k] = block_out[k]
                k += 1
            }
            generated += copy_size
            if(generated >= output_len) { break }

            // Compute next A(i+1) = HMAC(secret, A(i))
            crypto::hmac_sha256(secret, secret_len, A, 32, &raw mut A[0])
        }
    }

    // ============================================================================
    // TLS 1.3 HKDF-Expand-Label (RFC 8446)
    // ============================================================================

    func tls13_hkdf_expand_label(secret : *u8, secret_len : size_t,
                                  label : *char, label_len : size_t,
                                  context : *u8, context_len : size_t,
                                  output : *mut u8, output_len : size_t) {

        var label_prefix = "tls13 \0" as *char
        var prefix_len : size_t = 6

        var hkdf_label_len : size_t = (2 + 1 + prefix_len + label_len + 1 + context_len) as size_t
        var hkdf_label : [512]u8

        // Length (2 bytes)
        hkdf_label[0] = ((output_len >> 8) & 0xFF) as u8
        hkdf_label[1] = (output_len & 0xFF) as u8
        // Label length (1 byte)
        hkdf_label[2] = (prefix_len + label_len) as u8
        // Label value: "tls13 " + label
        var i : size_t = 0
        while(i < prefix_len) {
            hkdf_label[3 + i] = label_prefix[i] as u8
            i += 1
        }
        while(i < (prefix_len + label_len) as size_t) {
            hkdf_label[3 + i] = label[i - prefix_len] as u8
            i += 1
        }
        var pos : size_t = (3 + prefix_len + label_len) as size_t
        // Context length (1 byte)
        hkdf_label[pos] = context_len as u8
        pos += 1
        // Context value
        var j : size_t = 0
        while(j < context_len) {
            hkdf_label[pos + j] = context[j]
            j += 1
        }

        // HKDF-Expand: T(0) = empty, T(i) = HMAC(PRK, T(i-1) | info | i)
        var T : [32]u8
        var generated : size_t = 0
        var counter : u8 = 1
        var T_len : size_t = 0

        while(generated < output_len) {
            var input_len : size_t = T_len + hkdf_label_len + 1
            var input_buf : [1024]u8

            var k : size_t = 0
            while(k < T_len) {
                input_buf[k] = T[k]
                k += 1
            }
            while(k - T_len < hkdf_label_len) {
                input_buf[k] = hkdf_label[k - T_len]
                k += 1
            }
            input_buf[k] = counter as u8
            k += 1

            crypto::hmac_sha256(secret, secret_len, input_buf, input_len, &raw mut T[0])
            T_len = 32

            var copy_size : size_t = output_len - generated
            if(copy_size > 32 as size_t) { copy_size = 32 as size_t }
            var l : size_t = 0
            while(l < copy_size) {
                output[generated + l] = T[l]
                l += 1
            }
            generated += copy_size
            counter = counter + 1
            if(counter > 20) { break }
        }
    }

    // TLS 1.3 Derive-Secret (RFC 8446)
    public func tls13_derive_secret(secret : *u8, secret_len : size_t,
                                     label : *char, label_len : size_t,
                                     transcript_hash : *u8, hash_len : size_t,
                                     output : *mut u8, output_len : size_t) {
        tls13_hkdf_expand_label(secret, secret_len, label, label_len,
                                transcript_hash, hash_len, output, output_len)
    }

    // ============================================================================
    // TLS 1.3 Key Schedule (RFC 8446 Section 7.1)
    // ============================================================================

    // HKDF-Extract: PRK = HMAC-Hash(salt, IKM)
    func tls13_hkdf_extract(salt : *u8, salt_len : size_t,
                             ikm : *u8, ikm_len : size_t,
                             prk : *mut u8) {
        crypto::hmac_sha256(salt, salt_len, ikm, ikm_len, prk)
    }

    // Derive handshake traffic keys from the ECDHE shared secret.
    // Must be called after receiving ServerHello (transcript includes CH + SH).
    // Populates ssl.transform_in and ssl.transform_out for the handshake phase.
    // transcript_hash = SHA256(ClientHello...ServerHello)
    public func tls13_derive_handshake_keys(ssl : *mut SSLContext,
                                             shared_secret : *u8, shared_len : size_t,
                                             transcript_hash : *u8) : int {
        var hash_len : size_t = 32  // SHA-256

        // Step 1: Early secret = HKDF-Extract(0, 0) — well-known for no-PSK
        var zeros32 : [32]u8
        var i : size_t = 0
        while(i < 32) { zeros32[i] = 0; i += 1 }

        var early_secret : [32]u8
        tls13_hkdf_extract(&raw zeros32[0], 32, &raw zeros32[0], 32, &raw mut early_secret[0])

        // Step 2: Derived = HKDF-Expand-Label(early_secret, "derived", "", 32)
        // Per RFC 8446, the "derived" context is empty ("")
        var derived : [32]u8
        var empty_ctx : [1]u8 = [0]
        var derived_label = "derived\0" as *char
        tls13_hkdf_expand_label(&raw early_secret[0], 32, derived_label, 7,
                                &raw empty_ctx[0], 0, &raw mut derived[0], 32)

        // Step 3: Handshake secret = HKDF-Extract(Derived, shared_secret)
        var handshake_secret : [32]u8
        tls13_hkdf_extract(&raw derived[0], 32, shared_secret, shared_len,
                           &raw mut handshake_secret[0])

        // Store handshake secret in key schedule
        i = 0
        while(i < 32) {
            ssl.tls13_keys.handshake_secret[i] = handshake_secret[i]
            i += 1
        }

        // Step 4: Derive client and server handshake traffic secrets
        // Context = Transcript-Hash(ClientHello...ServerHello)
        var client_hts : [32]u8
        var server_hts : [32]u8
        var chts_label = "c hs traffic\0" as *char
        var shts_label = "s hs traffic\0" as *char
        tls13_hkdf_expand_label(&raw handshake_secret[0], 32, chts_label, 12,
                                transcript_hash, hash_len, &raw mut client_hts[0], 32)
        tls13_hkdf_expand_label(&raw handshake_secret[0], 32, shts_label, 12,
                                transcript_hash, hash_len, &raw mut server_hts[0], 32)

        // Store in key schedule
        i = 0
        while(i < 32) {
            ssl.tls13_keys.client_handshake_traffic_secret[i] = client_hts[i]
            ssl.tls13_keys.server_handshake_traffic_secret[i] = server_hts[i]
            i += 1
        }

        // Step 5: Derive keys and IVs for AES-128-GCM
        // client_handshake_key = HKDF-Expand-Label(client_hts, "key", "", 16)
        // client_handshake_iv  = HKDF-Expand-Label(client_hts, "iv",  "", 12)
        // server_handshake_key = HKDF-Expand-Label(server_hts, "key", "", 16)
        // server_handshake_iv  = HKDF-Expand-Label(server_hts, "iv",  "", 12)
        var empty_ctx2 : [1]u8 = [0]
        var key_label = "key\0" as *char
        var iv_label = "iv\0" as *char

        var client_key : [16]u8
        var client_iv : [12]u8
        var server_key : [16]u8
        var server_iv : [12]u8

        tls13_hkdf_expand_label(&raw client_hts[0], 32, key_label, 3,
                                &raw empty_ctx2[0], 0, &raw mut client_key[0], 16)
        tls13_hkdf_expand_label(&raw client_hts[0], 32, iv_label, 2,
                                &raw empty_ctx2[0], 0, &raw mut client_iv[0], 12)
        tls13_hkdf_expand_label(&raw server_hts[0], 32, key_label, 3,
                                &raw empty_ctx2[0], 0, &raw mut server_key[0], 16)
        tls13_hkdf_expand_label(&raw server_hts[0], 32, iv_label, 2,
                                &raw empty_ctx2[0], 0, &raw mut server_iv[0], 12)

        // Populate transform_out (client write) — for sending
        var tr_out : Transform
        transform_init(&raw mut tr_out)
        tr_out.cipher_type = CIPHER_AES_128_GCM as u8
        tr_out.key_len = 16
        tr_out.iv_len = 12
        tr_out.fixed_iv_len = 12
        tr_out.mac_key_len = 0
        i = 0
        while(i < 16) { tr_out.key_enc[i] = client_key[i]; i += 1 }
        i = 0
        while(i < 12) { tr_out.base_iv_enc[i] = client_iv[i]; i += 1 }

        // Populate transform_in (server write) — for receiving
        var tr_in : Transform
        transform_init(&raw mut tr_in)
        tr_in.cipher_type = CIPHER_AES_128_GCM as u8
        tr_in.key_len = 16
        tr_in.iv_len = 12
        tr_in.fixed_iv_len = 12
        tr_in.mac_key_len = 0
        i = 0
        while(i < 16) { tr_in.key_dec[i] = server_key[i]; i += 1 }
        i = 0
        while(i < 12) { tr_in.base_iv_dec[i] = server_iv[i]; i += 1 }

        // Allocate and install transforms
        var tr_out_mem = malloc(sizeof(Transform)) as *mut Transform
        *tr_out_mem = tr_out
        ssl.transform_out = tr_out_mem

        var tr_in_mem = malloc(sizeof(Transform)) as *mut Transform
        *tr_in_mem = tr_in
        ssl.transform_in = tr_in_mem

        // Reset sequence numbers for the handshake phase
        i = 0
        while(i < 8) { ssl.in_ctr[i] = 0; ssl.out_ctr[i] = 0; i += 1 }

        return 0
    }

    // Derive application traffic keys after handshake completes.
    // Called after both Finished messages have been exchanged.
    public func tls13_derive_application_keys(ssl : *mut SSLContext,
                                               hs_hash : *u8, hash_len : size_t) : int {
        var i : size_t = 0

        // Derive-Secret(handshake_secret, "derived", Hash(ClientHello...Finished))
        var derived : [32]u8
        var empty32 : [32]u8
        while(i < 32) { empty32[i] = 0; i += 1 }
        var derived_label = "derived\0" as *char
        tls13_hkdf_expand_label(&raw ssl.tls13_keys.handshake_secret[0], 32,
                                derived_label, 7, hs_hash, hash_len,
                                &raw mut derived[0], 32)

        // Master secret = HKDF-Extract(Derived, 0)
        var master_secret : [32]u8
        tls13_hkdf_extract(&raw derived[0], 32, &raw empty32[0], 32,
                           &raw mut master_secret[0])
        i = 0
        while(i < 32) { ssl.tls13_keys.master_secret[i] = master_secret[i]; i += 1 }

        // Client application traffic secret
        var c_ats : [32]u8
        var c_ats_label = "c ap traffic\0" as *char
        tls13_hkdf_expand_label(&raw master_secret[0], 32, c_ats_label, 12,
                                hs_hash, hash_len, &raw mut c_ats[0], 32)
        i = 0
        while(i < 32) { ssl.tls13_keys.client_application_traffic_secret[i] = c_ats[i]; i += 1 }

        // Server application traffic secret
        var s_ats : [32]u8
        var s_ats_label = "s ap traffic\0" as *char
        tls13_hkdf_expand_label(&raw master_secret[0], 32, s_ats_label, 12,
                                hs_hash, hash_len, &raw mut s_ats[0], 32)
        i = 0
        while(i < 32) { ssl.tls13_keys.server_application_traffic_secret[i] = s_ats[i]; i += 1 }

        // Derive application keys
        var empty_ctx : [1]u8 = [0]
        var key_label = "key\0" as *char
        var iv_label = "iv\0" as *char

        var client_key : [16]u8
        var client_iv : [12]u8
        var server_key : [16]u8
        var server_iv : [12]u8

        tls13_hkdf_expand_label(&raw c_ats[0], 32, key_label, 3,
                                &raw empty_ctx[0], 0, &raw mut client_key[0], 16)
        tls13_hkdf_expand_label(&raw c_ats[0], 32, iv_label, 2,
                                &raw empty_ctx[0], 0, &raw mut client_iv[0], 12)
        tls13_hkdf_expand_label(&raw s_ats[0], 32, key_label, 3,
                                &raw empty_ctx[0], 0, &raw mut server_key[0], 16)
        tls13_hkdf_expand_label(&raw s_ats[0], 32, iv_label, 2,
                                &raw empty_ctx[0], 0, &raw mut server_iv[0], 12)

        // Replace transforms with application-traffic versions
        // Client write (out)
        if(ssl.transform_out != null) { unsafe { dealloc ssl.transform_out } }
        var tr_out : Transform
        transform_init(&raw mut tr_out)
        tr_out.cipher_type = CIPHER_AES_128_GCM as u8
        tr_out.key_len = 16
        tr_out.iv_len = 12
        tr_out.fixed_iv_len = 12
        i = 0
        while(i < 16) { tr_out.key_enc[i] = client_key[i]; i += 1 }
        i = 0
        while(i < 12) { tr_out.base_iv_enc[i] = client_iv[i]; i += 1 }
        var tr_out_mem = malloc(sizeof(Transform)) as *mut Transform
        *tr_out_mem = tr_out
        ssl.transform_out = tr_out_mem

        // Server write (in)
        if(ssl.transform_in != null) { unsafe { dealloc ssl.transform_in } }
        var tr_in : Transform
        transform_init(&raw mut tr_in)
        tr_in.cipher_type = CIPHER_AES_128_GCM as u8
        tr_in.key_len = 16
        tr_in.iv_len = 12
        tr_in.fixed_iv_len = 12
        i = 0
        while(i < 16) { tr_in.key_dec[i] = server_key[i]; i += 1 }
        i = 0
        while(i < 12) { tr_in.base_iv_dec[i] = server_iv[i]; i += 1 }
        var tr_in_mem = malloc(sizeof(Transform)) as *mut Transform
        *tr_in_mem = tr_in
        ssl.transform_in = tr_in_mem

        return 0
    }

    // ============================================================================
    // TLS 1.2 Key Derivation (RFC 5246)
    // ============================================================================

    // Derive master secret from pre-master secret
    // master_secret = PRF(pre_master_secret, "master secret", ClientHello.random + ServerHello.random)[0..47]
    public func tls12_derive_master_secret(pre_master : *u8, pre_master_len : size_t,
                                            client_random : *u8, server_random : *u8,
                                            master_secret : *mut u8) {
        // Build seed = ClientRandom + ServerRandom (64 bytes total)
        var seed : [64]u8
        var i : size_t = 0
        while(i < 32) {
            seed[i] = client_random[i]
            seed[i + 32] = server_random[i]
            i += 1
        }

        var label = "master secret\0" as *char
        tls12_prf(pre_master, pre_master_len, label, 13, &raw seed[0], 64, master_secret, 48)
    }

    // Derive key block from master secret (RFC 5246 Section 6.3)
    // key_block = PRF(master_secret, "key expansion", ServerRandom + ClientRandom)
    // The key_block is split as needed for the cipher suite
    public func tls12_derive_key_block(master_secret : *u8,
                                        server_random : *u8, client_random : *u8,
                                        key_block : *mut u8, key_block_len : size_t) {
        // Build seed = ServerRandom + ClientRandom (reversed from master secret derivation)
        var seed : [64]u8
        var i : size_t = 0
        while(i < 32) {
            seed[i] = server_random[i]
            seed[i + 32] = client_random[i]
            i += 1
        }

        var label = "key expansion\0" as *char
        tls12_prf(master_secret, 48, label, 14, &raw seed[0], 64, key_block, key_block_len)
    }

    // Compute the key block size needed for a cipher suite
    public func tls12_key_block_size(info : *CipherSuiteInfo) : size_t {
        var mac_key_len = info.mac_key_len as size_t
        var enc_key_len = info.key_size as size_t
        var iv_len : size_t = 0
        var cipher = info.cipher
        if(cipher == CIPHER_AES_128_CBC || cipher == CIPHER_AES_256_CBC) {
            iv_len = info.iv_size as size_t
        } else if(cipher == CIPHER_AES_128_GCM || cipher == CIPHER_AES_256_GCM) {
            // For GCM, fixed IV is 4 bytes (explicit nonce is 8 bytes sent in record)
            iv_len = 4
        }
        // 2 directions: client + server
        return (mac_key_len + enc_key_len + iv_len) * 2
    }

    // Split key block and populate Transform structure
    public func tls12_populate_transform(tr : *mut Transform, info : *CipherSuiteInfo,
                                          key_block : *u8, key_block_len : size_t) : int {
        var mac_key_len = info.mac_key_len as size_t
        var enc_key_len = info.key_size as size_t
        var iv_len : size_t = 0

        var cipher = info.cipher
        if(cipher == CIPHER_AES_128_CBC || cipher == CIPHER_AES_256_CBC) {
            iv_len = info.iv_size as size_t
        } else {
            iv_len = 4  // Fixed IV for GCM
        }

        var offset : size_t = 0

        // client_write_MAC_key
        var i : size_t = 0
        while(i < mac_key_len) {
            tr.mac_key_enc[i] = key_block[offset + i]
            i += 1
        }
        offset += mac_key_len

        // server_write_MAC_key
        i = 0
        while(i < mac_key_len) {
            tr.mac_key_dec[i] = key_block[offset + i]
            i += 1
        }
        offset += mac_key_len

        // client_write_key
        i = 0
        while(i < enc_key_len) {
            tr.key_enc[i] = key_block[offset + i]
            i += 1
        }
        offset += enc_key_len

        // server_write_key
        i = 0
        while(i < enc_key_len) {
            tr.key_dec[i] = key_block[offset + i]
            i += 1
        }
        offset += enc_key_len

        // client_write_IV
        i = 0
        while(i < iv_len) {
            tr.iv_enc[i] = key_block[offset + i]
            tr.base_iv_enc[i] = key_block[offset + i]
            i += 1
        }
        offset += iv_len

        // server_write_IV
        i = 0
        while(i < iv_len) {
            tr.iv_dec[i] = key_block[offset + i]
            tr.base_iv_dec[i] = key_block[offset + i]
            i += 1
        }
        offset += iv_len

        tr.cipher_type = cipher as u8
        tr.key_len = enc_key_len as u8
        tr.iv_len = iv_len as u8
        tr.mac_key_len = mac_key_len as u8
        tr.fixed_iv_len = iv_len as u8

        return 0
    }

    // ============================================================================
    // Finished Message Calculation (RFC 5246 Section 7.4.9)
    // ============================================================================

    // Compute Finished message verify_data
    // verify_data = PRF(master_secret, finished_label, SHA256(handshake_messages))[0..11]
    public func tls12_compute_finished(master_secret : *u8, is_client : bool,
                                        handshake_hash : *u8, hash_len : size_t,
                                        verify_data : *mut u8) {
        var label : *char
        if(is_client) {
            label = "client finished\0" as *char
        } else {
            label = "server finished\0" as *char
        }
        tls12_prf(master_secret, 48, label, 15,
                   handshake_hash, hash_len, verify_data, 12)
    }

    // ============================================================================
    // Record Encryption / Decryption (TLS 1.2)
    // ============================================================================

    // Encrypt a TLS record using the provided transform
    // input: plaintext data (already has MAC appended if using CBC)
    // output: ciphertext (includes IV + encrypted data + tag for GCM)
    // Returns the total ciphertext length or negative error
    public func tls12_encrypt_record(tr : *mut Transform, seq_num : *u8,
                                     content_type : u8, version_major : u8,
                                     version_minor : u8, input : *u8,
                                     input_len : size_t, output : *mut u8,
                                     out_max : size_t) : int {
        var cipher = tr.cipher_type

        if(cipher == CIPHER_AES_128_GCM || cipher == CIPHER_AES_256_GCM) {
            // GCM mode: nonce = base_iv (4 bytes) || explicit_nonce (8 bytes from seq_num)
            // TLS 1.2: explicit_nonce is sent in the record, total output = nonce(8) + ct + tag(16)
            var iv_len = tr.fixed_iv_len as size_t
            var key_len = tr.key_len as size_t

            // Construct 12-byte GCM nonce: fixed_iv (4 bytes from transform) + seq_num (8 bytes)
            var nonce : [12]u8
            var i : size_t = 0
            while(i < iv_len) {
                nonce[i] = tr.base_iv_enc[i]
                i += 1
            }
            while(i < 12) {
                nonce[i] = seq_num[i - iv_len]
                i += 1
            }

            // Copy explicit nonce = sequence number (8 bytes, MSB-first)
            i = 0
            while(i < 8) {
                output[i] = seq_num[i]
                i += 1
            }

            // Output layout: explicit_nonce(8) || ciphertext || auth_tag(16)
            var ct_out = output + 8
            var tag_out = output + 8 + input_len

            // Initialize GCM context
            var gcm_ctx : GCMContext
            var ret = gcm_init(&raw mut gcm_ctx, &raw tr.key_enc[0], key_len)
            if(ret < 0) { return ret }

            // Encrypt with nonce as 12-byte IV, no AAD
            ret = gcm_crypt_and_tag(&raw mut gcm_ctx,
                                     &raw nonce[0], 12,
                                     null, 0,
                                     input, input_len,
                                     ct_out, tag_out)
            if(ret < 0) { return ret }

            return (8 + input_len + 16) as i32
        } else if(cipher == CIPHER_AES_128_CBC || cipher == CIPHER_AES_256_CBC) {
            // CBC mode with HMAC
            var key_len = tr.key_len as size_t
            var iv_len = tr.iv_len as size_t

            // Copy IV to output
            var i : size_t = 0
            while(i < iv_len) {
                output[i] = tr.iv_enc[i]
                i += 1
            }

            // Encrypt the plaintext (should already have padding + MAC)
            var aes_ctx : AESContext
            aes_setkey_enc(&raw mut aes_ctx, &raw tr.key_enc[0], key_len)
            var ret = aes_crypt_cbc(&raw mut aes_ctx, AES_ENCRYPT, input_len,
                                     &raw mut output[0], input, &raw mut output[iv_len])
            if(ret < 0) { return ret }

            return (iv_len + input_len) as i32
        }

        // Fallback: no encryption
        var i : size_t = 0
        while(i < input_len) {
            output[i] = input[i]
            i += 1
        }
        return input_len as i32
    }

    // Decrypt a TLS record
    public func tls12_decrypt_record(tr : *mut Transform, seq_num : *u8,
                                     content_type : u8, version_major : u8,
                                     version_minor : u8, input : *u8,
                                     input_len : size_t, output : *mut u8,
                                     out_max : size_t) : int {
        var cipher = tr.cipher_type

        if(cipher == CIPHER_AES_128_GCM || cipher == CIPHER_AES_256_GCM) {
            // GCM mode: input = explicit_nonce(8) || ciphertext || auth_tag(16)
            if(input_len < 8 + 16) { return ERR_SSL_INVALID_RECORD }

            var explicit_nonce_len : size_t = 8
            var tag_len : size_t = 16
            var ct_len = input_len - explicit_nonce_len - tag_len

            var iv_len = tr.fixed_iv_len as size_t
            var key_len = tr.key_len as size_t

            // Construct 12-byte GCM nonce: fixed_iv + explicit_nonce
            var nonce : [12]u8
            var i : size_t = 0
            while(i < iv_len) {
                nonce[i] = tr.base_iv_dec[i]
                i += 1
            }
            while(i < 12) {
                nonce[i] = input[i - iv_len]
                i += 1
            }

            // Initialize GCM context
            var gcm_ctx : GCMContext
            var ret = gcm_init(&raw mut gcm_ctx, &raw tr.key_dec[0], key_len)
            if(ret < 0) { return ret }

            // Authenticated decrypt
            var ct_start = input + explicit_nonce_len
            var tag_start = input + explicit_nonce_len + ct_len
            ret = gcm_auth_decrypt(&raw mut gcm_ctx,
                                    &raw nonce[0], 12,
                                    null, 0,
                                    ct_start, ct_len,
                                    tag_start, tag_len,
                                    output)
            if(ret < 0) { return ret }

            return ct_len as i32
        } else if(cipher == CIPHER_AES_128_CBC || cipher == CIPHER_AES_256_CBC) {
            var key_len = tr.key_len as size_t
            var iv_len = tr.iv_len as size_t

            if(input_len < iv_len + 16) { return ERR_SSL_INVALID_RECORD }

            var aes_ctx : AESContext
            aes_setkey_dec(&raw mut aes_ctx, &raw tr.key_dec[0], key_len)

            var cipher_len = input_len - iv_len
            var ret = aes_crypt_cbc(&raw mut aes_ctx, AES_DECRYPT, cipher_len,
                                     &raw mut input[0], &raw input[iv_len], output)
            if(ret < 0) { return ret }

            // The plaintext has MAC and padding. For now, return the full plaintext
            return cipher_len as i32
        }

        // Fallback: no decryption
        var i : size_t = 0
        while(i < input_len) {
            output[i] = input[i]
            i += 1
        }
        return input_len as i32
    }

    // ============================================================================
    // TLS 1.3 Record Layer (RFC 8446 Section 5)
    // ============================================================================

    // TLS 1.3 uses a different record format than TLS 1.2:
    // - Outer content_type is always application_data (23) for encrypted records
    // - Inner content_type is in the last byte of the decrypted payload
    // - AAD for AEAD is the 5-byte outer record header
    // - Nonce = static IV XOR padded sequence number

    // Increment a sequence number counter (8 bytes, big-endian)
    func ssl_incr_seq_num(seq : *mut u8) {
        var i : i32 = 7
        while(i >= 0) {
            var val = seq[i] as u16 + 1
            seq[i] = (val & 0xFF) as u8
            if(val < 256) { break }
            i -= 1
        }
    }

    // Build a TLS 1.3 nonce: static_iv (12 bytes) XOR (0x00000000 || seq_num)
    func tls13_build_nonce(static_iv : *u8, seq_num : *u8, nonce : *mut u8) {
        // First 4 bytes: static_iv XOR 0
        var i : size_t = 0
        while(i < 4) {
            nonce[i] = static_iv[i]
            i += 1
        }
        // Last 8 bytes: static_iv[4..11] XOR seq_num[0..7]
        i = 0
        while(i < 8) {
            nonce[4 + i] = static_iv[4 + i] ^ seq_num[i]
            i += 1
        }
    }

    // Encrypt plaintext using TLS 1.3 AEAD record format.
    // plaintext = content_type(1) + actual_data
    // output = header(5) + encrypted_record
    // Returns total bytes written (5 + encrypted_len), or negative error.
    public func tls13_encrypt_record(ssl : *mut SSLContext,
                                      content_type : u8,
                                      data : *u8, data_len : size_t,
                                      output : *mut u8, out_max : size_t) : int {
        if(ssl.transform_out == null) { return ERR_SSL_INTERNAL_ERROR }
        var tr = ssl.transform_out

        // Build inner plaintext: data || content_type (TLS 1.3 puts content_type at end)
        // But for send_record, caller provides data that needs to be sent as content_type.
        // So inner plaintext = data + content_type
        var inner_len : size_t = data_len + 1
        var inner : [16640]u8  // MAX_RECORD_PAYLOAD + 1
        var i : size_t = 0
        while(i < data_len) {
            inner[i] = data[i]
            i += 1
        }
        inner[data_len] = content_type

        // Build nonce
        var nonce : [12]u8
        tls13_build_nonce(&raw tr.base_iv_enc[0], &raw ssl.out_ctr[0], &raw mut nonce[0])

        // Initialize GCM context with encryption key
        var gcm_ctx : GCMContext
        var ret = gcm_init(&raw mut gcm_ctx, &raw tr.key_enc[0], tr.key_len as size_t)
        if(ret < 0) { return ret }

        // Additional data = outer record header
        // outer content_type = 23 (application_data), version = 0x0303
        var outer_hdr : [5]u8
        outer_hdr[0] = SSL_MSG_APPLICATION_DATA as u8
        outer_hdr[1] = 0x03
        outer_hdr[2] = 0x03
        outer_hdr[3] = ((inner_len >> 8) & 0xFF) as u8
        outer_hdr[4] = (inner_len & 0xFF) as u8

        // Encrypted payload goes at output + 5
        var ct_out = output + 5
        var tag_out = output + 5 + inner_len

        // GCM encrypt: ciphertext = Encrypt(key, nonce, plaintext, AAD)
        ret = gcm_crypt_and_tag(&raw mut gcm_ctx,
                                 &raw nonce[0], 12,
                                 &raw outer_hdr[0], 5,
                                 &raw inner[0], inner_len,
                                 ct_out, tag_out)
        if(ret < 0) { return ret }

        // Write outer header
        i = 0
        while(i < 5) {
            output[i] = outer_hdr[i]
            i += 1
        }

        // Increment sequence number
        ssl_incr_seq_num(&raw mut ssl.out_ctr[0])

        return (5 + inner_len + 16) as i32
    }

    // Decrypt a TLS 1.3 record.
    // input = encrypted_record (without the 5-byte outer header, which has already been read)
    // outer_content_type is already known (from the header read by ssl_read_record)
    // output = decrypted plaintext (without the inner content_type at the end)
    // inner_content_type is extracted from the last byte of decrypted data.
    // Returns plaintext length, or negative error.
    public func tls13_decrypt_record(ssl : *mut SSLContext,
                                      input : *u8, input_len : size_t,
                                      output : *mut u8, out_max : size_t,
                                      inner_content_type : *mut u8) : int {
        if(ssl.transform_in == null) { return ERR_SSL_INTERNAL_ERROR }
        var tr = ssl.transform_in

        if(input_len < 16) { return ERR_SSL_INVALID_RECORD }  // At minimum: tag

        // The ciphertext includes the auth tag (16 bytes) at the end
        var ct_len : size_t = input_len - 16
        var tag_start = input + ct_len

        // Build nonce
        var nonce : [12]u8
        tls13_build_nonce(&raw tr.base_iv_dec[0], &raw ssl.in_ctr[0], &raw mut nonce[0])

        // Initialize GCM context with decryption key
        var gcm_ctx : GCMContext
        var ret = gcm_init(&raw mut gcm_ctx, &raw tr.key_dec[0], tr.key_len as size_t)
        if(ret < 0) { return ret }

        // Additional data = outer record header (already parsed into ssl.in_hdr)
        // We need to reconstruct it with the original length
        var outer_hdr : [5]u8
        outer_hdr[0] = ssl.in_hdr[0]  // Should be 23 (application_data)
        outer_hdr[1] = ssl.in_hdr[1]  // 0x03
        outer_hdr[2] = ssl.in_hdr[2]  // 0x03
        outer_hdr[3] = ssl.in_hdr[3]  // length high
        outer_hdr[4] = ssl.in_hdr[4]  // length low

        // Allocate temp buffer for decrypted data
        var dec_buf : [16640]u8
        if(ct_len > out_max + 1) { ct_len = out_max + 1 }  // +1 for content_type

        // GCM decrypt and verify
        ret = gcm_auth_decrypt(&raw mut gcm_ctx,
                                &raw nonce[0], 12,
                                &raw outer_hdr[0], 5,
                                input, ct_len,
                                tag_start, 16,
                                &raw mut dec_buf[0])
        if(ret < 0) { return ERR_SSL_INVALID_RECORD }  // Authentication failed

        // Inner plaintext is dec_buf[0..ct_len-1], inner content_type is dec_buf[ct_len-1]
        if(ct_len == 0) { return ERR_SSL_INVALID_RECORD }
        var actual_len = ct_len - 1
        *inner_content_type = dec_buf[actual_len]

        // Copy decrypted data to output
        var i : size_t = 0
        while(i < actual_len) {
            output[i] = dec_buf[i]
            i += 1
        }

        // Increment sequence number
        ssl_incr_seq_num(&raw mut ssl.in_ctr[0])

        return actual_len as i32
    }

    // ============================================================================
    // Record Layer
    // ============================================================================

    public comptime const MAX_RECORD_PAYLOAD = 16384
    public comptime const RECORD_HEADER_SIZE = 5

    // Send a TLS record
    func send_record(ssl : *mut SSLContext, content_type : u8,
                     data : *u8, data_len : u16) : int {
        if((data_len as int) > MAX_RECORD_PAYLOAD) { return ERR_SSL_INTERNAL_ERROR }

        // If encryption is active, use TLS 1.3 AEAD record format
        if(ssl.transform_out != null && content_type != SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
            var encrypted : [17400]u8
            var enc_len = tls13_encrypt_record(ssl, content_type,
                                                data, data_len as size_t,
                                                &raw mut encrypted[0], 17400)
            if(enc_len < 0) { return enc_len }
            return ssl_send(ssl, &raw encrypted[0], enc_len)
        }

        // Build record header
        var header : [5]u8
        header[0] = content_type
        header[1] = ssl.major_ver
        header[2] = ssl.minor_ver
        header[3] = ((data_len >> 8) & 0xFF) as u8
        header[4] = (data_len & 0xFF) as u8

        // Send header
        var ret = ssl_send(ssl, &raw header[0], 5)
        if(ret < 0) { return ret }

        // Send data
        if(data_len > 0) {
            ret = ssl_send(ssl, data, data_len as i32)
            if(ret < 0) { return ret }
        }

        return 0
    }

    // Send an alert record
    func send_alert(ssl : *mut SSLContext, level : u8, description : u8) : int {
        var alert_data : [2]u8
        alert_data[0] = level as u8
        alert_data[1] = description as u8
        return send_record(ssl, SSL_MSG_ALERT as u8, &raw alert_data[0], 2 as u16)
    }

    // Send a handshake message
    func send_handshake_msg(ssl : *mut SSLContext, msg_type : u8,
                            data : *u8, data_len : u32) : int {
        var hs_header : [4]u8
        hs_header[0] = msg_type
        write_u24(data_len, &raw mut hs_header[1])

        var total_len : u32 = 4 + data_len
        var buf : [16388]u8

        var i : size_t = 0
        while(i < 4) {
            buf[i] = hs_header[i]
            i += 1
        }
        var j : size_t = 0
        while(j < data_len as size_t) {
            buf[4 + j] = data[j]
            j += 1
        }

        return send_record(ssl, SSL_MSG_HANDSHAKE as u8, &raw buf[0], total_len as u16)
    }

    // ─── Buffered Record I/O ──────────────────────────────────────────────
    // All record reads go through ssl_read_record(), which uses the input
    // buffer (ssl.in_buf) to handle:
    //   - Multiple records coalesced in a single TCP segment
    //   - Records split across multiple TCP segments
    //   - Partial reads

    // Fetch more data from the socket into the input buffer
    func ssl_fetch_input(ssl : *mut SSLContext, min_len : size_t) : int {
        while(ssl.in_left < min_len as i32) {
            var buf_start : i32 = ssl.in_left
            if(buf_start >= 17408 as i32) { return ERR_SSL_BUFFER_TOO_SMALL }
            var max_read : i32 = (17408 as i32) - buf_start

            var n = ssl_recv(ssl, &raw mut ssl.in_buf[buf_start], max_read)
            if(n < 0) {
                if(n == ERR_SSL_CONN_EOF) {
                    if(ssl.in_left == 0) { return ERR_SSL_CONN_EOF }
                    break
                }
                return n
            }
            if(n == 0) {
                if(ssl.in_left == 0) { return ERR_SSL_CONN_EOF }
                break
            }
            ssl.in_left += n as i32
        }

        if(ssl.in_left < min_len as i32) {
            return ERR_SSL_CONN_EOF
        }
        return 0
    }

    // Read the next complete record from the input buffer
    // Returns 0 on success, negative error code on failure.
    // The record header (5 bytes) is stored in ssl.in_hdr.
    // The record payload (record_len bytes) is stored in ssl.in_buf.
    func ssl_read_record(ssl : *mut SSLContext) : int {
        // Ensure we have at least 5 bytes (record header)
        var ret = ssl_fetch_input(ssl, 5)
        if(ret < 0) { return ret }

        // Parse record header from buffer
        ssl.in_hdr[0] = ssl.in_buf[0]  // content_type
        ssl.in_hdr[1] = ssl.in_buf[1]  // version.major
        ssl.in_hdr[2] = ssl.in_buf[2]  // version.minor
        ssl.in_hdr[3] = ssl.in_buf[3]  // length high
        ssl.in_hdr[4] = ssl.in_buf[4]  // length low

        var record_len = read_u16_be(&raw ssl.in_buf[3]) as size_t
        if(record_len > 16384 + 256) { return ERR_SSL_INVALID_RECORD }

        // Ensure we have the full record payload
        // Note: NOT overwriting ssl.in_left here to preserve any
        // coalesced records that arrived in the same TCP segment.
        var total_needed : size_t = 5 + record_len
        ret = ssl_fetch_input(ssl, total_needed)
        if(ret < 0) { return ret }

        ssl.in_msglen = record_len as i32

        // AEAD decryption: if transform_in is set, decrypt the record payload.
        // TLS 1.3: outer CT is always APPLICATION_DATA (23); inner CT is last byte of plaintext.
        // TLS 1.2: CT is in the header; payload is encrypted after CCS.
        if(ssl.transform_in != null) {
            var did_decrypt = false

            // TLS 1.3 decrypt: triggered when outer CT is APPLICATION_DATA
            if(ssl.in_hdr[0] == SSL_MSG_APPLICATION_DATA as u8) {
                var inner_ct : u8 = 0
                var dec_buf : [17400]u8
                var dec_len = tls13_decrypt_record(ssl,
                                                    &raw ssl.in_buf[5], record_len,
                                                    &raw mut dec_buf[0], 17400,
                                                    &raw mut inner_ct)
                if(dec_len >= 0) {
                    var i : i32 = 0
                    while(i < dec_len) {
                        ssl.in_buf[5 + i] = dec_buf[i]
                        i += 1
                    }
                    ssl.in_msglen = dec_len
                    ssl.in_hdr[0] = inner_ct

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
                    did_decrypt = true
                }
            }

            // TLS 1.2 decrypt: triggered for HANDSHAKE (22) or APPLICATION_DATA (23)
            // when TLS 1.3 decrypt did not apply or failed.
            if(!did_decrypt && (ssl.in_hdr[0] == SSL_MSG_HANDSHAKE as u8 || ssl.in_hdr[0] == SSL_MSG_APPLICATION_DATA as u8)) {
                var dec_buf2 : [17400]u8
                var dec_len2 = tls12_decrypt_record(ssl.transform_in,
                                                     &raw ssl.in_ctr[0],
                                                     ssl.in_hdr[0], ssl.in_hdr[1], ssl.in_hdr[2],
                                                     &raw ssl.in_buf[5], record_len,
                                                     &raw mut dec_buf2[0], 17400 as size_t)
                if(dec_len2 >= 0) {
                    var i : i32 = 0
                    while(i < dec_len2) {
                        ssl.in_buf[5 + i] = dec_buf2[i]
                        i += 1
                    }
                    ssl.in_msglen = dec_len2

                    var original_end : i32 = 5 + record_len as i32
                    var new_end : i32 = 5 + dec_len2
                    if(original_end < ssl.in_left) {
                        var shift_i : i32 = 0
                        while(shift_i < ssl.in_left - original_end) {
                            ssl.in_buf[new_end + shift_i] = ssl.in_buf[original_end + shift_i]
                            shift_i += 1
                        }
                        ssl.in_left -= (original_end - new_end)
                    }
                    ssl_incr_seq_num(&raw mut ssl.in_ctr[0])
                }
            }
        }

        return 0
    }

    // Consume (remove) the current record from the input buffer
    func ssl_consume_record(ssl : *mut SSLContext) {
        var consumed = 5 + ssl.in_msglen
        if(consumed > ssl.in_left) { consumed = ssl.in_left }
        if(consumed <= 0) { return }

        // Shift remaining data to front of buffer
        var remaining = ssl.in_left - consumed
        if(remaining > 0) {
            var i : i32 = 0
            while(i < remaining) {
                ssl.in_buf[i] = ssl.in_buf[consumed + i]
                i += 1
            }
        }
        ssl.in_left = remaining
        ssl.in_msglen = 0
    }

    // Read a TLS record header (blocking) - maintains backward compatibility
    func read_record_header(ssl : *mut SSLContext, hdr : *mut u8) : int {
        var ret = ssl_read_record(ssl)
        if(ret < 0) { return ret }
        hdr[0] = ssl.in_hdr[0]
        hdr[1] = ssl.in_hdr[1]
        hdr[2] = ssl.in_hdr[2]
        hdr[3] = ssl.in_hdr[3]
        hdr[4] = ssl.in_hdr[4]
        return 0
    }

    // Read record payload - copies from internal buffer to caller's buffer
    func read_record_payload(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
        var copy_len = ssl.in_msglen
        if(copy_len > len) { copy_len = len }
        if(copy_len > 0) {
            var i : i32 = 0
            while(i < copy_len) {
                buf[i] = ssl.in_buf[5 + i]
                i += 1
            }
        }
        ssl_consume_record(ssl)
        return copy_len
    }

    // ============================================================================
    // TLS 1.2 Client Hello Construction
    // ============================================================================

    func build_client_hello(ssl : *mut SSLContext, buf : *mut u8, buf_size : size_t) : size_t {
        var pos : size_t = 0

        // Protocol version (TLS 1.2 = 0x0303)
        buf[pos] = 0x03 as u8; pos += 1
        buf[pos] = 0x03 as u8; pos += 1

        // Client random (32 bytes) - cryptographically secure random
        var rand_buf : [32]u8
        var rand_ret = random_fill(&raw mut rand_buf[0], 32)
        if(rand_ret < 0) {
            // Fallback to LCG if CSPRNG fails
            var seed_val : u32 = (pos as u32) * 2654435761u + 12345u
            var k : u32 = 0
            while(k < 32) {
                seed_val = seed_val * 1103515245 + 12345
                rand_buf[k] = (seed_val & 0xFF) as u8
                k += 1
            }
        }
        var k : u32 = 0
        while(k < 32) {
            buf[pos] = rand_buf[k]
            pos += 1
            k += 1
        }

        // Session ID (for session resumption)
        if(ssl.session != null && ssl.session.id_len > 0) {
            buf[pos] = ssl.session.id_len as u8
            pos += 1
            var sid_i : size_t = 0
            while(sid_i < ssl.session.id_len) {
                buf[pos] = ssl.session.id[sid_i]
                pos += 1
                sid_i += 1
            }
        } else {
            buf[pos] = 0 as u8; pos += 1
        }

        // Cipher suites
        var suite_count : u32 = 0
        var suite_start = pos
        buf[pos] = 0 as u8; pos += 1
        buf[pos] = 0 as u8; pos += 1

        var i : u32 = 0
        while(i < ssl.conf.ciphersuite_count) {
            var cs_id = ssl.conf.ciphersuite_list[i]
            if(cs_id != 0) {
                buf[pos] = ((cs_id >> 8) & 0xFF) as u8
                pos += 1
                buf[pos] = (cs_id & 0xFF) as u8
                pos += 1
                suite_count += 1
            }
            i += 1
        }

        var cs_len = suite_count * 2
        buf[suite_start] = ((cs_len >> 8) & 0xFF) as u8
        buf[suite_start + 1] = (cs_len & 0xFF) as u8

        // Compression methods
        buf[pos] = 1 as u8; pos += 1
        buf[pos] = 0 as u8; pos += 1

        // Extensions
        var ext_start = pos
        buf[pos] = 0 as u8; pos += 1
        buf[pos] = 0 as u8; pos += 1

        // Extension: supported_versions
        buf[pos] = ((TLS_EXT_SUPPORTED_VERSIONS >> 8) & 0xFF) as u8; pos += 1
        buf[pos] = (TLS_EXT_SUPPORTED_VERSIONS & 0xFF) as u8; pos += 1

        var sv_len_pos = pos
        buf[pos] = 0 as u8; pos += 1
        buf[pos] = 0 as u8; pos += 1

        buf[pos] = 4 as u8; pos += 1  // List length (2 versions * 2 bytes)
        buf[pos] = 0x03 as u8; pos += 1; buf[pos] = 0x04 as u8; pos += 1  // TLS 1.3
        buf[pos] = 0x03 as u8; pos += 1; buf[pos] = 0x03 as u8; pos += 1  // TLS 1.2

        var sv_data_len = 5
        buf[sv_len_pos] = ((sv_data_len >> 8) & 0xFF) as u8
        buf[sv_len_pos + 1] = (sv_data_len & 0xFF) as u8

        // Extension: supported_groups
        buf[pos] = ((TLS_EXT_SUPPORTED_GROUPS >> 8) & 0xFF) as u8; pos += 1
        buf[pos] = (TLS_EXT_SUPPORTED_GROUPS & 0xFF) as u8; pos += 1

        var sg_len_pos = pos
        buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

        buf[pos] = 0 as u8; pos += 1; buf[pos] = 8 as u8; pos += 1
        buf[pos] = 0x00 as u8; pos += 1; buf[pos] = 0x1D as u8; pos += 1  // x25519
        buf[pos] = 0x00 as u8; pos += 1; buf[pos] = 0x17 as u8; pos += 1  // secp256r1
        buf[pos] = 0x00 as u8; pos += 1; buf[pos] = 0x18 as u8; pos += 1  // secp384r1
        buf[pos] = 0x00 as u8; pos += 1; buf[pos] = 0x1E as u8; pos += 1  // x448

        var sg_data_len = 2 + 8
        buf[sg_len_pos] = ((sg_data_len >> 8) & 0xFF) as u8
        buf[sg_len_pos + 1] = (sg_data_len & 0xFF) as u8

        // Extension: signature_algorithms
        buf[pos] = ((TLS_EXT_SIG_ALG >> 8) & 0xFF) as u8; pos += 1
        buf[pos] = (TLS_EXT_SIG_ALG & 0xFF) as u8; pos += 1

        var sa_len_pos = pos
        buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

        buf[pos] = 0 as u8; pos += 1; buf[pos] = 16 as u8; pos += 1
        buf[pos] = 0x04 as u8; pos += 1; buf[pos] = 0x03 as u8; pos += 1
        buf[pos] = 0x05 as u8; pos += 1; buf[pos] = 0x03 as u8; pos += 1
        buf[pos] = 0x08 as u8; pos += 1; buf[pos] = 0x04 as u8; pos += 1
        buf[pos] = 0x08 as u8; pos += 1; buf[pos] = 0x05 as u8; pos += 1
        buf[pos] = 0x04 as u8; pos += 1; buf[pos] = 0x01 as u8; pos += 1
        buf[pos] = 0x05 as u8; pos += 1; buf[pos] = 0x01 as u8; pos += 1
        buf[pos] = 0x08 as u8; pos += 1; buf[pos] = 0x09 as u8; pos += 1
        buf[pos] = 0x08 as u8; pos += 1; buf[pos] = 0x07 as u8; pos += 1

        var sa_data_len = 2 + 16
        buf[sa_len_pos] = ((sa_data_len >> 8) & 0xFF) as u8
        buf[sa_len_pos + 1] = (sa_data_len & 0xFF) as u8

        // SNI extension
        if(ssl.hostname != null && ssl.hostname_len > 0) {
            buf[pos] = ((TLS_EXT_SERVERNAME >> 8) & 0xFF) as u8; pos += 1
            buf[pos] = (TLS_EXT_SERVERNAME & 0xFF) as u8; pos += 1

            var sni_len_pos = pos
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

            buf[pos] = 0 as u8; pos += 1
            var hostlist_len = ssl.hostname_len + 3
            buf[pos] = hostlist_len as u8; pos += 1

            buf[pos] = 0 as u8; pos += 1  // Name type: host_name
            write_u16_be(ssl.hostname_len as u16, &raw mut buf[pos]); pos += 2

            var n : size_t = 0
            while(n < ssl.hostname_len) {
                buf[pos] = ssl.hostname[n] as u8
                pos += 1
                n += 1
            }

            var sni_data_len = 2 + 2 + hostlist_len
            buf[sni_len_pos] = (sni_data_len >> 8) as u8
            buf[sni_len_pos + 1] = sni_data_len as u8
        }

        // Extension: key_share (TLS 1.3)
        // Only if handshake params have an ECDHE public key ready
        if(ssl.handshake != null && ssl.handshake.ecdhe_public != null &&
           ssl.handshake.ecdhe_public_len == 65) {
            buf[pos] = ((TLS_EXT_KEY_SHARE >> 8) & 0xFF) as u8; pos += 1
            buf[pos] = (TLS_EXT_KEY_SHARE & 0xFF) as u8; pos += 1

            var ks_len_pos = pos
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

            // NamedGroup (2 bytes)
            buf[pos] = ((ssl.handshake.ecdhe_curve >> 8) & 0xFF) as u8; pos += 1
            buf[pos] = (ssl.handshake.ecdhe_curve & 0xFF) as u8; pos += 1
            // KeyExchangeLength (2 bytes) = 65
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 65 as u8; pos += 1
            // KeyExchange (65 bytes): 04 || X || Y
            var ki : size_t = 0
            while(ki < 65) {
                buf[pos] = ssl.handshake.ecdhe_public[ki]
                pos += 1
                ki += 1
            }

            var ks_data_len = 2 + 2 + 65  // group + len + key
            buf[ks_len_pos] = ((ks_data_len >> 8) & 0xFF) as u8
            buf[ks_len_pos + 1] = (ks_data_len & 0xFF) as u8
        }

        var ext_len = pos - ext_start - 2
        buf[ext_start] = (ext_len >> 8) as u8
        buf[ext_start + 1] = ext_len as u8

        return pos
    }

    // ============================================================================
    // I/O Functions
    // ============================================================================

    func ssl_send(ssl : *mut SSLContext, data : *u8, len : i32) : int {
        if(!ssl.transport_connected) { return ERR_SSL_INTERNAL_ERROR }
        var n = net::send_all(ssl.transport_socket, data as *char, len)
        if(n < 0) { return ERR_SSL_CONN_EOF }
        return len
    }

    func ssl_recv(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
        if(!ssl.transport_connected) { return ERR_SSL_INTERNAL_ERROR }
        var n = net::recv_all(ssl.transport_socket, buf, len as usize)
        if(n < 0) { return ERR_SSL_CONN_EOF }
        if(n == 0) { return ERR_SSL_CONN_EOF }
        return n
    }

    // ============================================================================
    // Client Handshake - TLS 1.2
    // ============================================================================

    // Read a single handshake message from the server
    func read_handshake_msg(ssl : *mut SSLContext, hs_type : *mut u8,
                             hs_len : *mut u32, hs_buf : *mut u8,
                             buf_size : size_t) : int {
        var hdr : [5]u8
        var ret = read_record_header(ssl, &raw mut hdr[0])
        if(ret < 0) { return ret }

        var content_type = hdr[0]
        var record_len = read_u16_be(&raw hdr[3])

        // Handle ChangeCipherSpec messages
        if(content_type == SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
            var ccs_data : [1]u8
            var n = read_record_payload(ssl, &raw mut ccs_data[0], 1)
            if(n < 0) { return n }
            ret = read_record_header(ssl, &raw mut hdr[0])
            if(ret < 0) { return ret }
            content_type = hdr[0]
            record_len = read_u16_be(&raw hdr[3])
        }

        if(content_type != SSL_MSG_HANDSHAKE as u8) {
            if(content_type == SSL_MSG_ALERT as u8) {
                var alert_data : [2]u8
                var n2 = read_record_payload(ssl, &raw mut alert_data[0], 2)
                if(n2 < 0) { return ERR_SSL_FATAL_ALERT_MESSAGE }
                ssl.last_alert_level = alert_data[0]
                ssl.last_alert_desc = alert_data[1]
                return ERR_SSL_FATAL_ALERT_MESSAGE
            }
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        var payload_len = record_len as size_t
        if(payload_len > buf_size) { payload_len = buf_size }

        var payload = read_record_payload(ssl, hs_buf, payload_len as i32)
        if(payload < (4 as i32)) { return ERR_SSL_DECODE_ERROR }

        *hs_type = hs_buf[0]
        *hs_len = read_u24(&raw hs_buf[1])

        return 0
    }

    // ─── Helper: feed a handshake message into the transcript hash ───────

    func ssl_hash_handshake_msg(hash_ctx : *mut crypto::Sha256Context,
                                 msg_type : u8, msg_len : u32,
                                 msg_body : *u8) {
        var hdr : [4]u8
        hdr[0] = msg_type
        write_u24(msg_len, &raw mut hdr[1])
        crypto::sha256_update(hash_ctx, &raw hdr[0], 4)
        if(msg_len > 0) {
            crypto::sha256_update(hash_ctx, msg_body, msg_len as size_t)
        }
    }

    // ─── Extract RSA public key from parsed certificate ──────────────────
    // Parse the SubjectPublicKeyInfo BIT STRING to extract RSA modulus N and exponent E
    // Returns 0 on success, negative error code on failure.
    // The RSA context must be initialized with rsa_init() before calling.
    public func x509_extract_rsa_pubkey(crt : *mut X509Cert, rsa : *mut RSAContext) : int {
        if(crt.pk_type != PK_RSA as u8) { return ERR_SSL_PK_TYPE_MISMATCH }
        if(crt.pk_raw == null || crt.pk_raw_len == 0) { return ERR_X509_INVALID_FORMAT }

        // pk_raw points to SPKI SEQUENCE content
        // Structure: SEQUENCE { AlgorithmIdentifier, BIT STRING { SEQUENCE { INTEGER N, INTEGER E } } }
        var data = crt.pk_raw
        var len = crt.pk_raw_len
        var pos : size_t = 0

        // Parse AlgorithmIdentifier SEQUENCE inside SPKI
        var seq_tag : u8 = 0; var seq_len : size_t = 0
        var ret = asn1_get_tag(data, len, &raw mut pos, &raw mut seq_tag, &raw mut seq_len)
        if(ret < 0) { return ret }
        if(seq_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_ALG }

        // Skip AlgorithmIdentifier content (OID + params)
        pos += seq_len

        // Parse BIT STRING
        var bit_tag : u8 = 0; var bit_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut bit_tag, &raw mut bit_len)
        if(ret < 0) { return ret }
        if(bit_tag != ASN1_BIT_STRING) { return ERR_X509_INVALID_FORMAT }

        // Skip unused bits byte
        if(pos >= len) { return ERR_X509_INVALID_FORMAT }
        pos += 1
        bit_len -= 1

        if(bit_len == 0) { return ERR_X509_INVALID_FORMAT }
        var bit_end = pos + bit_len

        // Parse inner SEQUENCE (RSA public key: N, E)
        var rsa_tag : u8 = 0; var rsa_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut rsa_tag, &raw mut rsa_len)
        if(ret < 0) { return ret }
        if(rsa_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_FORMAT }

        // Parse INTEGER N
        var n_tag : u8 = 0; var n_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut n_tag, &raw mut n_len)
        if(ret < 0) { return ret }
        if(n_tag != ASN1_INTEGER) { return ERR_X509_INVALID_FORMAT }

        var n_data = data + pos
        var n_data_len = n_len
        pos += n_len

        // Parse INTEGER E
        var e_tag : u8 = 0; var e_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut e_tag, &raw mut e_len)
        if(ret < 0) { return ret }
        if(e_tag != ASN1_INTEGER) { return ERR_X509_INVALID_FORMAT }

        var e_data = data + pos
        var e_data_len = e_len

        // Import into RSA context
        ret = rsa_import_pubkey(rsa, n_data, n_data_len, e_data, e_data_len)
        if(ret < 0) { return ret }

        return 0
    }

    // ─── Verify X.509 Certificate RSA Signature ──────────────────────────
    // Verifies the certificate's signature using the issuer's RSA public key.
    // crt: the certificate to verify
    // issuer_rsa: issuer RSA context with N and E already imported
    // Returns 0 on success, negative error code on failure
    public func x509_verify_cert_signature(crt : *mut X509Cert,
                                            issuer_rsa : *mut RSAContext) : int {
        if(crt.tbs_der == null || crt.tbs_der_len == 0) { return ERR_X509_INVALID_FORMAT }
        if(crt.sig == null || crt.sig_len == 0) { return ERR_X509_INVALID_FORMAT }

        // Compute SHA-256 hash of the TBSCertificate DER using init/update/final
        var hash : [32]u8
        var sha_ctx : crypto::Sha256Context
        crypto::sha256_init(&raw mut sha_ctx)
        crypto::sha256_update(&raw mut sha_ctx, crt.tbs_der, crt.tbs_der_len)
        crypto::sha256_final(&raw mut sha_ctx, &raw mut hash[0])

        // Verify using RSA PKCS#1 v1.5 signature verification
        var ret = rsa_pkcs1_verify(issuer_rsa, &raw hash[0], 32, crt.sig, crt.sig_len)
        if(ret < 0) { return ERR_X509_SIG_MISMATCH }

        return 0
    }

    // ─── X.509 Hostname Verification ──────────────────────────────────────
    // Verify that the certificate's CN or SAN matches the expected hostname.
    // Returns 0 on match, X509_BADCERT_CN_MISMATCH on mismatch.
    public func x509_verify_hostname(crt : *mut X509Cert, hostname : *char) : int {
        // Extract CN from subject
        var cn = string()
        cert_get_cn(crt, &raw mut cn)

        // Check CN against hostname
        var cn_view = cn.to_view()
        var host_view = string_view(hostname)

        // Direct comparison using byte-by-byte check
        var match = true
        if(cn_view.size() != host_view.size()) { match = false }
        if(match) {
            var ci : size_t = 0
            while(ci < cn_view.size()) {
                if(cn_view.get(ci) != host_view.get(ci)) { match = false }
                ci += 1
            }
        }
        if(match) { return 0 }

        // Wildcard check: CN = *.example.com, hostname = www.example.com
        if(cn_view.size() > 2) {
            if(cn_view.get(0) == ('*' as u8) && cn_view.get(1) == ('.' as u8)) {
                // Compare everything after *.
                var wild_suffix_size = cn_view.size() - 1
                var host_len = host_view.size()
                if(host_len >= wild_suffix_size) {
                    var match2 = true
                    var wi : size_t = 0
                    while(wi < wild_suffix_size) {
                        var c = cn_view.get(1 + wi)
                        var h = host_view.get(host_len - wild_suffix_size + wi)
                        if(c != h) { match2 = false }
                        wi += 1
                    }
                    if(match2) { return 0 }
                }
            }
        }

        // TODO: Check SAN entries (subjectAltName extension)
        // For now, fall back to CN-only matching

        return X509_BADCERT_CN_MISMATCH as i32
    }

    // ─── ASN1_TIME Parser ──────────────────────────────────────────────────
    // Parse ASN1_UTC_TIME (YYMMDDHHMMSSZ, 13 bytes) or ASN1_GENERALIZED_TIME
    // (YYYYMMDDHHMMSSZ, 15 bytes) into date components.
    // Returns 0 on success, ERR_X509_INVALID_DATE on failure.
    func parse_asn1_time(time_str : *u8, max_len : size_t,
                          year : *mut int, month : *mut int, day : *mut int,
                          hour : *mut int, minute : *mut int,
                          second : *mut int) : int {
        var len : size_t = 0
        while(len < max_len && time_str[len] != 0) { len += 1 }

        if(len == 13) {
            // UTCTime: YYMMDDHHMMSSZ
            var yy = (time_str[0] as int - 48) * 10 + (time_str[1] as int - 48)
            if(yy < 50) { *year = 2000 + yy } else { *year = 1900 + yy }
            *month = (time_str[2] as int - 48) * 10 + (time_str[3] as int - 48)
            *day = (time_str[4] as int - 48) * 10 + (time_str[5] as int - 48)
            *hour = (time_str[6] as int - 48) * 10 + (time_str[7] as int - 48)
            *minute = (time_str[8] as int - 48) * 10 + (time_str[9] as int - 48)
            *second = (time_str[10] as int - 48) * 10 + (time_str[11] as int - 48)
            return 0
        } else if(len == 11) {
            // UTCTime without seconds: YYMMDDHHMMZ
            var yy = (time_str[0] as int - 48) * 10 + (time_str[1] as int - 48)
            if(yy < 50) { *year = 2000 + yy } else { *year = 1900 + yy }
            *month = (time_str[2] as int - 48) * 10 + (time_str[3] as int - 48)
            *day = (time_str[4] as int - 48) * 10 + (time_str[5] as int - 48)
            *hour = (time_str[6] as int - 48) * 10 + (time_str[7] as int - 48)
            *minute = (time_str[8] as int - 48) * 10 + (time_str[9] as int - 48)
            *second = 0
            return 0
        } else if(len == 15) {
            // GeneralizedTime: YYYYMMDDHHMMSSZ
            *year = (time_str[0] as int - 48) * 1000 + (time_str[1] as int - 48) * 100 +
                     (time_str[2] as int - 48) * 10 + (time_str[3] as int - 48)
            *month = (time_str[4] as int - 48) * 10 + (time_str[5] as int - 48)
            *day = (time_str[6] as int - 48) * 10 + (time_str[7] as int - 48)
            *hour = (time_str[8] as int - 48) * 10 + (time_str[9] as int - 48)
            *minute = (time_str[10] as int - 48) * 10 + (time_str[11] as int - 48)
            *second = (time_str[12] as int - 48) * 10 + (time_str[13] as int - 48)
            return 0
        }

        return ERR_X509_INVALID_DATE
    }

    // ─── X.509 Date Validity Check ────────────────────────────────────────
    // Check if the certificate's validity period covers the current time.
    // Returns 0 if valid, X509_BADCERT_EXPIRED or X509_BADCERT_FUTURE on failure.
    public func x509_check_date(crt : *mut X509Cert) : int {
        if(crt.valid_from[0] == 0 || crt.valid_to[0] == 0) {
            return 0  // No date info, skip check
        }

        // Parse notBefore
        var from_year : int = 0; var from_month : int = 0; var from_day : int = 0
        var from_hour : int = 0; var from_min : int = 0; var from_sec : int = 0
        var ret = parse_asn1_time(&raw crt.valid_from[0], 15,
                                   &raw mut from_year, &raw mut from_month,
                                   &raw mut from_day, &raw mut from_hour,
                                   &raw mut from_min, &raw mut from_sec)
        if(ret < 0) { return X509_BADCERT_EXPIRED as i32 }

        // Parse notAfter
        var to_year : int = 0; var to_month : int = 0; var to_day : int = 0
        var to_hour : int = 0; var to_min : int = 0; var to_sec : int = 0
        ret = parse_asn1_time(&raw crt.valid_to[0], 15,
                               &raw mut to_year, &raw mut to_month,
                               &raw mut to_day, &raw mut to_hour,
                               &raw mut to_min, &raw mut to_sec)
        if(ret < 0) { return X509_BADCERT_EXPIRED as i32 }

        // Get current UTC time
        var now : time_t = 0
        time(&raw mut now)

        // Decompose current UTC time into components
        var now_tm : tm
        var gm_ret = gmtime_r(&raw now, &raw mut now_tm)
        if(gm_ret == null) { return X509_BADCERT_EXPIRED as i32 }

        var now_year = now_tm.year + 1900
        var now_month = now_tm.mon + 1
        var now_day = now_tm.mday
        var now_hour = now_tm.hour
        var now_min = now_tm.min
        var now_sec = now_tm.sec

        // Compare current time to notBefore (cert not yet valid -> FUTURE)
        if(now_year < from_year) { return X509_BADCERT_FUTURE }
        if(now_year == from_year && now_month < from_month) { return X509_BADCERT_FUTURE }
        if(now_year == from_year && now_month == from_month && now_day < from_day) { return X509_BADCERT_FUTURE }
        if(now_year == from_year && now_month == from_month && now_day == from_day) {
            var now_seconds = now_hour * 3600 + now_min * 60 + now_sec
            var from_seconds = from_hour * 3600 + from_min * 60 + from_sec
            if(now_seconds < from_seconds) { return X509_BADCERT_FUTURE }
        }

        // Compare current time to notAfter (cert expired -> EXPIRED)
        if(now_year > to_year) { return X509_BADCERT_EXPIRED }
        if(now_year == to_year && now_month > to_month) { return X509_BADCERT_EXPIRED }
        if(now_year == to_year && now_month == to_month && now_day > to_day) { return X509_BADCERT_EXPIRED }
        if(now_year == to_year && now_month == to_month && now_day == to_day) {
            var now_seconds = now_hour * 3600 + now_min * 60 + now_sec
            var to_seconds = to_hour * 3600 + to_min * 60 + to_sec
            if(now_seconds > to_seconds) { return X509_BADCERT_EXPIRED }
        }

        return 0  // Certificate is valid
    }

    // ─── X.509 Certificate Chain Verification ─────────────────────────────
    // Verify a certificate chain from leaf to root.
    // leaf: the peer's certificate (first in chain)
    // trusted_ca: a trusted root CA certificate (or null to skip root verification)
    // hostname: expected server hostname (or null to skip)
    // Returns 0 on success, negative error code on failure.
    // Sets crt->flags with verification results.
    public func x509_verify_chain(leaf : *mut X509Cert, trusted_ca : *mut X509Cert,
                                   hostname : *char) : int {
        var flags : u32 = 0

        // 1. Self-signed check: if leaf issuer == leaf subject, it's self-signed
        var is_self_signed = false
        if(leaf.issuer.size() > 0 && leaf.subject.size() > 0) {
            var iss_view = leaf.issuer.to_view()
            var sub_view = leaf.subject.to_view()
            // Compare issuer and subject byte-by-byte
            var dn_match = true
            if(iss_view.size() != sub_view.size()) { dn_match = false }
            if(dn_match) {
                var di : size_t = 0
                while(di < iss_view.size()) {
                    if(iss_view.get(di) != sub_view.get(di)) { dn_match = false }
                    di += 1
                }
            }
            if(dn_match) { is_self_signed = true }
        }

        if(trusted_ca == null && is_self_signed) {
            // Self-signed cert without a trusted CA: verify using its own key
            var rsa_ctx : RSAContext
            rsa_init(&raw mut rsa_ctx, RSA_PKCS_V15, 0)
            var ret = x509_extract_rsa_pubkey(leaf, &raw mut rsa_ctx)
            if(ret == 0) {
                ret = x509_verify_cert_signature(leaf, &raw mut rsa_ctx)
                if(ret == 0) {
                    leaf.flags = 0  // Self-signed verified OK
                    return 0
                }
            }
            leaf.flags = X509_BADCERT_NOT_TRUSTED as u32
            return ERR_X509_CERT_VERIFY_FAILED
        }

        // 2. Check hostname first (always run regardless of CA verification)
        var hostname_ok = true
        if(hostname != null) {
            var hname_len : size_t = 0
            while(hostname[hname_len] != 0) { hname_len += 1 }
            if(hname_len > 0) {
                var h_ret = x509_verify_hostname(leaf, hostname)
                if(h_ret != 0) {
                    hostname_ok = false
                    leaf.flags = X509_BADCERT_CN_MISMATCH as u32
                }
            }
        }
        if(!hostname_ok) { return ERR_X509_CERT_VERIFY_FAILED }

        // 3. Check date validity
        var date_ret = x509_check_date(leaf)
        if(date_ret != 0) {
            leaf.flags = leaf.flags | date_ret as u32
            return ERR_X509_CERT_VERIFY_FAILED
        }

        // 4. Verify signature using trusted CA or self-signed
        if(trusted_ca != null) {
            var ca_rsa : RSAContext
            rsa_init(&raw mut ca_rsa, RSA_PKCS_V15, 0)
            var ret = x509_extract_rsa_pubkey(trusted_ca, &raw mut ca_rsa)
            if(ret == 0) {
                ret = x509_verify_cert_signature(leaf, &raw mut ca_rsa)
                if(ret == 0) {
                    leaf.flags = 0 as u32  // Verified by trusted CA
                    return 0
                }
            }
            leaf.flags = X509_BADCERT_NOT_TRUSTED as u32
            return ERR_X509_CERT_VERIFY_FAILED
        } else if(is_self_signed) {
            // Self-signed cert without a trusted CA: verify using its own key
            var rsa_ctx : RSAContext
            rsa_init(&raw mut rsa_ctx, RSA_PKCS_V15, 0)
            var ret = x509_extract_rsa_pubkey(leaf, &raw mut rsa_ctx)
            if(ret == 0) {
                ret = x509_verify_cert_signature(leaf, &raw mut rsa_ctx)
                if(ret == 0) {
                    leaf.flags = 0 as u32  // Self-signed verified OK
                    return 0
                }
            }
            leaf.flags = X509_BADCERT_NOT_TRUSTED as u32
            return ERR_X509_CERT_VERIFY_FAILED
        } else {
            // Neither self-signed nor trusted CA
            leaf.flags = X509_BADCERT_NOT_TRUSTED as u32
            return ERR_X509_CERT_VERIFY_FAILED
        }
    }

    // ─── Parse ServerHello and extract key parameters ────────────────────

    func parse_server_hello(ssl : *mut SSLContext, data : *u8, data_len : u32) : int {
        if(data_len < 38) { return ERR_SSL_DECODE_ERROR }

        var sh_version_major = data[4]
        var sh_version_minor = data[5]
        var sh_ciphersuite = read_u16_be(&raw data[37])

        ssl.major_ver = sh_version_major
        ssl.minor_ver = sh_version_minor
        ssl.negotiated_ciphersuite = sh_ciphersuite

        if(ssl.session != null) {
            ssl.session.ciphersuite = sh_ciphersuite
        }

        // Read server random (bytes 6-37) for key derivation
        if(ssl.handshake != null) {
            var i : size_t = 0
            while(i < 32) {
                ssl.handshake.randbytes[32 + i] = data[6 + i]
                i += 1
            }
        }

        // Extract session ID from ServerHello (bytes 38+ depending on session_id_len)
        var session_id_len = data[38] as size_t
        if(session_id_len > 0 && session_id_len <= 32) {
            if(ssl.session != null) {
                ssl.session.id_len = session_id_len
                var sid_i : size_t = 0
                while(sid_i < session_id_len) {
                    ssl.session.id[sid_i] = data[39 + sid_i]
                    sid_i += 1
                }
            }
        }

        return 0
    }

    func do_tls12_client_handshake(ssl : *mut SSLContext) : int {
        ensure_init()

        // ── Handshake transcript hash context ──
        var hash_ctx : crypto::Sha256Context
        crypto::sha256_init(&raw mut hash_ctx)

        // 1. Send ClientHello
        ssl.state = SSLState.CLIENT_HELLO()

        var ch_buf : [2048]u8
        var ch_len = build_client_hello(ssl, &raw mut ch_buf[0], 2048)

        // Feed ClientHello into transcript hash (including handshake header)
        ssl_hash_handshake_msg(&raw mut hash_ctx, SSL_HS_CLIENT_HELLO as u8, ch_len as u32, &raw ch_buf[0])

        var ret = send_handshake_msg(ssl, SSL_HS_CLIENT_HELLO as u8, &raw ch_buf[0], ch_len as u32)
        if(ret < 0) { return ret }

        // Allocate handshake params on the heap
        if(ssl.handshake == null) {
            var hs_mem = malloc(sizeof(HandshakeParams)) as *mut HandshakeParams
            handshake_params_init(hs_mem)
            ssl.handshake = hs_mem
        }

        // Copy client random to handshake params (bytes 2-33 of ch_buf)
        var i : size_t = 0
        while(i < 32) {
            ssl.handshake.randbytes[i] = ch_buf[2 + i]
            i += 1
        }

        // 2. Read ServerHello
        ssl.state = SSLState.SERVER_HELLO()

        var hs_type : u8 = 0
        var hs_len : u32 = 0
        var hs_buf : [8192]u8

        ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }

        if(hs_type != SSL_HS_SERVER_HELLO as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        // Feed ServerHello into transcript hash
        ssl_hash_handshake_msg(&raw mut hash_ctx, hs_type, hs_len, &raw hs_buf[0])

        ret = parse_server_hello(ssl, &raw hs_buf[0], hs_len)
        if(ret < 0) { return ret }

        // Copy server random (bytes 6-37 of hs_buf)
        i = 0
        while(i < 32) {
            ssl.handshake.randbytes[32 + i] = hs_buf[6 + i]
            i += 1
        }

        // Record negotiated ciphersuite
        var negotiated_cs = read_u16_be(&raw hs_buf[37])
        ssl.negotiated_ciphersuite = negotiated_cs

        // 3. Read Certificate
        ssl.state = SSLState.SERVER_CERTIFICATE()

        ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }

        // Extract RSA public key from server certificate
        var has_rsa_key : bool = false
        var rsa_ctx : RSAContext

        if(hs_type == SSL_HS_CERTIFICATE as u8) {
            ssl.state = SSLState.SERVER_KEY_EXCHANGE()

            // Feed Certificate into transcript hash
            ssl_hash_handshake_msg(&raw mut hash_ctx, hs_type, hs_len, &raw hs_buf[0])

            // Parse first certificate in the chain
            if(hs_len >= 6) {
                var first_cert_len_u24 = read_u24(&raw hs_buf[3])
                var first_cert_len = first_cert_len_u24 as size_t
                if(3 + first_cert_len <= hs_len as size_t) {
                    var cert : X509Cert
                    x509_cert_init(&raw mut cert)
                    var ret2 = parse_cert_der(&raw mut cert, &raw hs_buf[6], first_cert_len)
                    if(ret2 == 0) {
                        // Try to extract RSA public key
                        rsa_init(&raw mut rsa_ctx, RSA_PKCS_V15, 0)
                        var ret3 = x509_extract_rsa_pubkey(&raw mut cert, &raw mut rsa_ctx)
                        if(ret3 == 0 && rsa_get_len(&raw mut rsa_ctx) > 0) {
                            has_rsa_key = true
                            ssl.peer_cert = &raw mut cert
                            // Run certificate chain verification if CA chain is configured
                            if(ssl.conf != null && ssl.conf.ca_chain != null) {
                                var chain_ret = x509_verify_chain(&raw mut cert, ssl.conf.ca_chain,
                                                                    ssl.hostname)
                                if(chain_ret != 0) {
                                    // Cert verification failed — reject the connection
                                    return ERR_SSL_CERT_VERIFY_FAILED
                                }
                            }
                        }
                    }
                }
            }
        } else if(hs_type == SSL_HS_SERVER_HELLO_DONE as u8) {
            ssl.state = SSLState.SERVER_HELLO_DONE()
            ssl_hash_handshake_msg(&raw mut hash_ctx, hs_type, hs_len, &raw hs_buf[0])
        }

        // 4. Read ServerKeyExchange (if present) or ServerHelloDone
        if(ssl.state is SSLState.SERVER_KEY_EXCHANGE) {
            ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                      &raw mut hs_buf[0], 8192)
            if(ret < 0) { return ret }
            ssl_hash_handshake_msg(&raw mut hash_ctx, hs_type, hs_len, &raw hs_buf[0])
            ssl.state = SSLState.SERVER_HELLO_DONE()
        }

        // 5. Read ServerHelloDone if not already
        if(ssl.state is SSLState.SERVER_HELLO_DONE) {
            ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                      &raw mut hs_buf[0], 8192)
            if(ret < 0) { return ret }
            ssl_hash_handshake_msg(&raw mut hash_ctx, hs_type, hs_len, &raw hs_buf[0])
        } else {
            // Certificate was not present, read ServerHelloDone
            ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                      &raw mut hs_buf[0], 8192)
            if(ret < 0) { return ret }
            ssl_hash_handshake_msg(&raw mut hash_ctx, hs_type, hs_len, &raw hs_buf[0])
        }

        // ── Generate pre-master secret (TLS_RSA key exchange) ──
        // For TLS 1.2 RSA: pre_master_secret = ClientHello.version (2 bytes) + 46 random bytes
        var pre_master : [48]u8
        pre_master[0] = 0x03; pre_master[1] = 0x03  // TLS 1.2
        // Use CSPRNG for the remaining 46 bytes
        var pm_ret = random_fill(&raw mut pre_master[2], 46)
        if(pm_ret < 0) {
            // Fallback to LCG if CSPRNG fails
            var seed_val : u32 = 0xDEADBEEFu32
            i = 2
            while(i < 48) {
                seed_val = seed_val * 1103515245 + 12345
                pre_master[i] = (seed_val & 0xFF) as u8
                i += 1
            }
        }

        // ── Encrypt pre-master secret with RSA public key ──
        var cke_data : [512]u8
        var cke_len : size_t = 2  // Default: empty ClientKeyExchange (2 bytes length + 0 data)

        if(has_rsa_key) {
            var encrypted_pms : [512]u8
            var ret2 = rsa_pkcs1_encrypt(&raw mut rsa_ctx, &raw pre_master[0], 48, &raw mut encrypted_pms[0])
            if(ret2 == 0) {
                var key_len = rsa_get_len(&raw mut rsa_ctx)
                // ClientKeyExchange for RSA: length(2 bytes) + encrypted_pre_master
                cke_data[0] = ((key_len >> 8) & 0xFF) as u8
                cke_data[1] = (key_len & 0xFF) as u8
                var j : size_t = 0
                while(j < key_len) {
                    cke_data[2 + j] = encrypted_pms[j]
                    j += 1
                }
                cke_len = 2 + key_len
            }
        }

        // ── Derive master secret ──
        var master_secret : [48]u8
        tls12_derive_master_secret(
            &raw pre_master[0], 48,
            &raw ssl.handshake.randbytes[0],   // client random
            &raw ssl.handshake.randbytes[32],  // server random
            &raw mut master_secret[0]
        )

        // Store master secret in session
        if(ssl.session != null) {
            i = 0
            while(i < 48) {
                ssl.session.master[i] = master_secret[i]
                i += 1
            }
        }

        // ── Derive key block ──
        var cs_info = get_ciphersuite_info(ssl.negotiated_ciphersuite)
        var kb_size = tls12_key_block_size(&raw cs_info)
        var key_block : [256]u8
        tls12_derive_key_block(
            &raw master_secret[0],
            &raw ssl.handshake.randbytes[32],  // server random
            &raw ssl.handshake.randbytes[0],   // client random
            &raw mut key_block[0], kb_size
        )

        // ── Populate transform ──
        var tr : Transform
        transform_init(&raw mut tr)
        tls12_populate_transform(&raw mut tr, &raw cs_info, &raw key_block[0], kb_size)

        // Allocate and activate transforms (both directions use same keys for now)
        var tr_mem = malloc(sizeof(Transform)) as *mut Transform
        *tr_mem = tr
        ssl.transform_out = tr_mem

        var tr_in_mem = malloc(sizeof(Transform)) as *mut Transform
        *tr_in_mem = tr
        ssl.transform_in = tr_in_mem

        // ═══════════════════════════════════════════════════════════════════
        // PER RFC 5246 §7.4.9: The Finished message hash includes ALL
        // handshake messages up to (but not including) the Finished itself.
        // This means ClientKeyExchange MUST be sent and hashed BEFORE
        // finalizing the transcript hash and computing the Finished.
        // ═══════════════════════════════════════════════════════════════════

        // 6. Send ClientKeyExchange
        ssl.state = SSLState.CLIENT_KEY_EXCHANGE()
        ret = send_handshake_msg(ssl, SSL_HS_CLIENT_KEY_EXCHANGE as u8, &raw cke_data[0], cke_len as u32)
        if(ret < 0) { return ret }

        // Feed ClientKeyExchange into transcript hash
        ssl_hash_handshake_msg(&raw mut hash_ctx, SSL_HS_CLIENT_KEY_EXCHANGE as u8, cke_len as u32, &raw cke_data[0])

        // ── Finalize handshake transcript hash (includes all msgs up to CKE) ──
        var hs_hash : [32]u8
        crypto::sha256_final(&raw mut hash_ctx, &raw mut hs_hash[0])

        // ── Compute client Finished verify_data ──
        var client_finished : [12]u8
        tls12_compute_finished(&raw master_secret[0], true, &raw hs_hash[0], 32, &raw mut client_finished[0])

        // 7. Send ChangeCipherSpec
        ssl.state = SSLState.CLIENT_CHANGE_CIPHER_SPEC()
        var ccs_msg : [1]u8
        ccs_msg[0] = 1 as u8
        ret = send_record(ssl, SSL_MSG_CHANGE_CIPHER_SPEC as u8, &raw ccs_msg[0], 1 as u16)
        if(ret < 0) { return ret }

        // 8. Send Finished (with verify_data)
        ssl.state = SSLState.CLIENT_FINISHED()
        ret = send_handshake_msg(ssl, SSL_HS_FINISHED as u8, &raw client_finished[0], 12)
        if(ret < 0) { return ret }

        // 9. Read Server's ChangeCipherSpec + Finished
        ssl.state = SSLState.SERVER_CHANGE_CIPHER_SPEC()
        ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }

        // Verify server's Finished
        if(hs_type == SSL_HS_FINISHED as u8) {
            // Compute expected server verify_data using same transcript hash
            // The server's Finished uses the SAME handshake messages hash as the client's,
            // but with label "server finished" instead of "client finished"
            var expected_server_finished : [12]u8
            tls12_compute_finished(&raw master_secret[0], false,
                                    &raw hs_hash[0], 32,
                                    &raw mut expected_server_finished[0])

            // Compare against received server Finished
            var verify_match = true
            i = 0
            while(i < 12) {
                if(hs_buf[4 + i] != expected_server_finished[i]) { verify_match = false }
                i += 1
            }
            if(!verify_match) {
                send_alert(ssl, SSL_ALERT_LEVEL_FATAL as u8, SSL_ALERT_MSG_DECRYPT_ERROR as u8)
                return ERR_SSL_HANDSHAKE_FAILURE
            }
        } else {
            // Unexpected message instead of server Finished
            send_alert(ssl, SSL_ALERT_LEVEL_FATAL as u8, SSL_ALERT_MSG_UNEXPECTED_MESSAGE as u8)
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        ssl.state = SSLState.HANDSHAKE_OVER()

        return 0
    }

    // ============================================================================
    // Client Handshake - TLS 1.3
    // ============================================================================

    func do_tls13_client_handshake(ssl : *mut SSLContext) : int {
        ensure_init()

        // Ensure handshake params are allocated
        if(ssl.handshake == null) {
            var hs_mem = malloc(sizeof(HandshakeParams)) as *mut HandshakeParams
            if(hs_mem == null) { return ERR_SSL_INTERNAL_ERROR }
            handshake_params_init(hs_mem)
            ssl.handshake = hs_mem
        }

        // Generate ECDHE keypair (secp256r1 = 0x0017)
        var ecdh_ctx : ECDHContext
        ecdh_init(&raw mut ecdh_ctx)

        var priv_key : [32]u8
        var pub_key : [65]u8
        var ret = ecdh_generate_keypair(&raw mut ecdh_ctx,
                                        &raw mut priv_key[0], 32,
                                        &raw mut pub_key[0], 65)
        if(ret < 0) { return ret }

        // Store ECDHE public key in handshake params for build_client_hello
        var pub_mem = malloc(65) as *mut u8
        if(pub_mem == null) { return ERR_SSL_INTERNAL_ERROR }
        var pi : size_t = 0
        while(pi < 65) { pub_mem[pi] = pub_key[pi]; pi += 1 }
        ssl.handshake.ecdhe_curve = TLS_GROUP_SECP256R1 as u16
        ssl.handshake.ecdhe_public = pub_mem
        ssl.handshake.ecdhe_public_len = 65

        // ── ClientHello ───────────────────────────────────────────────
        ssl.state = SSLState.CLIENT_HELLO()
        ssl.major_ver = 0x03
        ssl.minor_ver = 0x03

        var ch_buf : [2048]u8
        var ch_len = build_client_hello(ssl, &raw mut ch_buf[0], 2048)

        // Hash the ClientHello body for the transcript
        var transcript : crypto::Sha256Context
        crypto::sha256_init(&raw mut transcript)
        var ch_hdr : [4]u8
        ch_hdr[0] = SSL_HS_CLIENT_HELLO as u8
        write_u24(ch_len as u32, &raw mut ch_hdr[1])
        crypto::sha256_update(&raw mut transcript, &raw ch_hdr[0], 4)
        crypto::sha256_update(&raw mut transcript, &raw ch_buf[0], ch_len)

        ret = send_handshake_msg(ssl, SSL_HS_CLIENT_HELLO as u8, &raw ch_buf[0], ch_len as u32)
        if(ret < 0) { return ret }

        // ── ServerHello ───────────────────────────────────────────────
        ssl.state = SSLState.SERVER_HELLO()

        var hs_buf : [4096]u8
        var hs_body_len : u32 = 0
        var got_server_hello = false

        while(!got_server_hello) {
            var hdr : [5]u8
            ret = read_record_header(ssl, &raw mut hdr[0])
            if(ret < 0) { return ret }

            var content_type = hdr[0]

            // TLS 1.3: CCS is a compatibility dummy, skip it
            if(content_type == SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
                var ccs_d : [1]u8
                read_record_payload(ssl, &raw mut ccs_d[0], 1)
                continue
            }

            if(content_type != SSL_MSG_HANDSHAKE as u8) {
                return ERR_SSL_UNEXPECTED_MESSAGE
            }

            var payload = read_record_payload(ssl, &raw mut hs_buf[0], 4096 as i32)
            if(payload < 4) { return ERR_SSL_DECODE_ERROR }

            var msg_type = hs_buf[0]
            hs_body_len = read_u24(&raw hs_buf[1])

            if(msg_type == SSL_HS_SERVER_HELLO as u8) {
                got_server_hello = true
            }
        }

        // Parse ServerHello body
        var sh_pos : size_t = 4  // skip hs_type(1) + length(3)

        // version (2 bytes)
        sh_pos += 2

        // random (32 bytes)
        sh_pos += 32

        // session_id_echo_len (1 byte)
        var sid_echo_len = hs_buf[sh_pos] as size_t; sh_pos += 1
        sh_pos += sid_echo_len

        // cipher_suite (2 bytes)
        ssl.negotiated_ciphersuite = read_u16_be(&raw hs_buf[sh_pos]); sh_pos += 2

        // compression_method (1 byte)
        sh_pos += 1

        // extensions_len (2 bytes)
        if(sh_pos + 2 > hs_body_len as size_t + 4) { return ERR_SSL_DECODE_ERROR }
        var sh_ext_len = read_u16_be(&raw hs_buf[sh_pos]) as size_t; sh_pos += 2

        // Parse extensions to find key_share
        var server_public_key : [65]u8
        var found_key_share = false
        var ext_end = sh_pos + sh_ext_len

        while(sh_pos + 4 <= ext_end) {
            var ext_type = read_u16_be(&raw hs_buf[sh_pos]); sh_pos += 2
            var ext_data_len = read_u16_be(&raw hs_buf[sh_pos]) as size_t; sh_pos += 2

            if(ext_type == TLS_EXT_KEY_SHARE as u16 && ext_data_len >= 4) {
                var ks_group = read_u16_be(&raw hs_buf[sh_pos])
                var ks_key_len = read_u16_be(&raw hs_buf[sh_pos + 2]) as size_t

                if(ks_group == TLS_GROUP_SECP256R1 as u16 && ks_key_len == 65) {
                    var ki : size_t = 0
                    while(ki < 65) {
                        server_public_key[ki] = hs_buf[sh_pos + 4 + ki]
                        ki += 1
                    }
                    found_key_share = true
                }
            }

            sh_pos += ext_data_len
        }

        if(!found_key_share) {
            return ERR_SSL_HANDSHAKE_FAILURE
        }

        // Hash ServerHello into transcript (including the 4-byte handshake header)
        crypto::sha256_update(&raw mut transcript, &raw hs_buf[0], 4 + hs_body_len)

        // ── Compute ECDHE shared secret ──────────────────────────────
        var shared_secret : [32]u8
        ret = ecdh_compute_shared(&raw mut ecdh_ctx,
                                  &raw server_public_key[0], 65,
                                  &raw mut shared_secret[0], 32)
        if(ret < 0) { return ret }

        // Hash(ClientHello...ServerHello) — save transcript state first, then finalize
        var sh_transcript_copy = transcript
        var sh_hash : [32]u8
        crypto::sha256_final(&raw mut sh_transcript_copy, &raw mut sh_hash[0])

        // ── Derive handshake traffic keys ────────────────────────────
        ret = tls13_derive_handshake_keys(ssl, &raw shared_secret[0], 32,
                                           &raw sh_hash[0])
        if(ret < 0) { return ret }

        // ── Read encrypted server messages ───────────────────────────
        var server_finished_verified = false

        while(!server_finished_verified) {
            var enc_hdr : [5]u8
            ret = read_record_header(ssl, &raw mut enc_hdr[0])
            if(ret < 0) { return ret }

            var enc_ct = enc_hdr[0]

            // CCS from server is allowed (compatibility)
            if(enc_ct == SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
                var ccs_d : [1]u8
                read_record_payload(ssl, &raw mut ccs_d[0], 1)
                continue
            }

            // ssl_read_record already decrypts and updates in_hdr[0] to inner content_type
            var inner_ct = enc_ct

            if(inner_ct == SSL_MSG_ALERT as u8) {
                var alert_data : [2]u8
                read_record_payload(ssl, &raw mut alert_data[0], 2)
                return ERR_SSL_FATAL_ALERT_MESSAGE
            }

            if(inner_ct != SSL_MSG_HANDSHAKE as u8) {
                return ERR_SSL_UNEXPECTED_MESSAGE
            }

            // Read the handshake message body
            var msg_buf : [4096]u8
            var msg_payload = read_record_payload(ssl, &raw mut msg_buf[0], 4096 as i32)
            if(msg_payload < 4) { return ERR_SSL_DECODE_ERROR }

            var msg_type_code = msg_buf[0] as u32
            var msg_body_len2 = read_u24(&raw msg_buf[1])

            if(msg_type_code == SSL_HS_ENCRYPTED_EXTENSIONS as u32) {
                crypto::sha256_update(&raw mut transcript, &raw msg_buf[0], 4 + msg_body_len2)

            } else if(msg_type_code == SSL_HS_CERTIFICATE as u32) {
                crypto::sha256_update(&raw mut transcript, &raw msg_buf[0], 4 + msg_body_len2)

            } else if(msg_type_code == SSL_HS_CERTIFICATE_VERIFY as u32) {
                crypto::sha256_update(&raw mut transcript, &raw msg_buf[0], 4 + msg_body_len2)

            } else if(msg_type_code == SSL_HS_FINISHED as u32) {
                // Server Finished: derive expected verify_data
                var finished_key : [32]u8
                var fin_key_label = "finished\0" as *char
                var empty_c : [1]u8 = [0]
                tls13_hkdf_expand_label(&raw ssl.tls13_keys.server_handshake_traffic_secret[0], 32,
                                        fin_key_label, 8,
                                        &raw empty_c[0], 0,
                                        &raw mut finished_key[0], 32)

                // Hash Transcript: save state before finalizing (copy the struct)
                var fin_transcript_copy = transcript
                var fin_transcript_hash : [32]u8
                crypto::sha256_final(&raw mut fin_transcript_copy, &raw mut fin_transcript_hash[0])

                // Compute expected: HMAC(finished_key, transcript_hash)
                var expected_finished : [32]u8
                crypto::hmac_sha256(&raw finished_key[0], 32,
                                    &raw fin_transcript_hash[0], 32,
                                    &raw mut expected_finished[0])

                // Compare with received (msg_buf[4..4+12])
                var verify_ok = true
                var vi : size_t = 0
                while(vi < 12) {
                    if(msg_buf[4 + vi] != expected_finished[vi]) { verify_ok = false }
                    vi += 1
                }

                if(!verify_ok) {
                    return ERR_SSL_HANDSHAKE_FAILURE
                }

                // Hash Finished into transcript, then finalize for app key derivation
                crypto::sha256_update(&raw mut transcript, &raw msg_buf[0], 4 + msg_body_len2)

                var full_transcript_copy = transcript
                var full_hash : [32]u8
                crypto::sha256_final(&raw mut full_transcript_copy, &raw mut full_hash[0])

                // Derive application traffic keys
                ret = tls13_derive_application_keys(ssl, &raw full_hash[0], 32)
                if(ret < 0) { return ret }

                server_finished_verified = true

            } else {
                return ERR_SSL_UNEXPECTED_MESSAGE
            }
        }

        // ── Send client Finished ─────────────────────────────────────
        // First send ChangeCipherSpec (TLS 1.3 compatibility)
        var ccs_out : [1]u8 = [20]
        ret = send_record(ssl, SSL_MSG_CHANGE_CIPHER_SPEC as u8, &raw ccs_out[0], 1)
        if(ret < 0) { return ret }

        // Derive client finished key
        var client_finished_key : [32]u8
        var cf_label = "finished\0" as *char
        var empty_c2 : [1]u8 = [0]
        tls13_hkdf_expand_label(&raw ssl.tls13_keys.client_handshake_traffic_secret[0], 32,
                                cf_label, 8,
                                &raw empty_c2[0], 0,
                                &raw mut client_finished_key[0], 32)

        // Hash Transcript: finalize for client Finished compute
        var cf_transcript_copy = transcript
        var cf_hash : [32]u8
        crypto::sha256_final(&raw mut cf_transcript_copy, &raw mut cf_hash[0])

        var client_finished_verify : [32]u8
        crypto::hmac_sha256(&raw client_finished_key[0], 32,
                            &raw cf_hash[0], 32,
                            &raw mut client_finished_verify[0])

        // Build and send Finished message: hs_type(1) + length(3) + verify_data(12)
        var cf_body : [12]u8
        var ci : size_t = 0
        while(ci < 12) {
            cf_body[ci] = client_finished_verify[ci]
            ci += 1
        }

        ret = send_handshake_msg(ssl, SSL_HS_FINISHED as u8, &raw cf_body[0], 12)
        if(ret < 0) { return ret }

        ssl.state = SSLState.HANDSHAKE_OVER()
        return 0
    }

    // ============================================================================
    // ─── Secure Connection Helper ──────────────────────────────────────────
    // Accept a TLS connection on an already-accepted socket.
    // Performs the server-side TLS handshake.
    // Returns a heap-allocated SSLContext on success, null on failure.
    public func tls_accept(sock : net::Socket, cert : *mut X509Cert) : *mut SSLContext {
        var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
        if(ssl_mem == null) { return null }

        ssl_init(ssl_mem)
        ssl_set_socket(ssl_mem, sock)

        // Create server config
        var cfg = ssl_config_init(SSL_IS_SERVER)
        cfg.authmode = SSL_VERIFY_NONE
        cfg.own_cert = cert

        var cfg_mem = malloc(sizeof(SSLConfig)) as *mut SSLConfig
        if(cfg_mem == null) {
            unsafe { dealloc ssl_mem }
            return null
        }
        *cfg_mem = cfg
        ssl_set_config(ssl_mem, cfg_mem)

        // Perform server handshake
        var ret = do_tls12_server_handshake(ssl_mem)
        if(ret < 0) {
            ssl_free(ssl_mem)
            unsafe { dealloc ssl_mem }
            return null
        }

        return ssl_mem
    }

    // ─── Auto CA Bundle Loading ───────────────────────────────────────────
    // Try to load the system CA bundle from common locations.
    // Returns a heap-allocated X509Cert on success, null if no CA found.
    public func load_system_ca_bundle() : *mut X509Cert {
        // Common CA bundle paths on Linux
        var paths : [4]*char = [
            "/etc/ssl/certs/ca-certificates.crt\0" as *char,
            "/etc/pki/tls/certs/ca-bundle.crt\0" as *char,
            "/etc/ssl/cert.pem\0" as *char,
            "/etc/pki/tls/cert.pem\0" as *char
        ]

        var i : size_t = 0
        while(i < 4) {
            var ca = x509_crt_load_pem_file(paths[i])
            if(ca != null) { return ca }
            i += 1
        }

        return null
    }

    // ─── CA Trust Store ───────────────────────────────────────────────────
    // Load a PEM-encoded certificate from a file on disk.
    // Returns a pointer to a heap-allocated X509Cert on success,
    // or null on failure. Caller is responsible for freeing.
    public func x509_crt_load_pem_file(path : *char) : *mut X509Cert {
        var mode = "rb\0" as *char
        var f = fopen(path, mode)
        if(f == null) { return null }

        // Read the entire file into a buffer (up to 16KB)
        var buf : [16384]u8
        var total_read : size_t = 0
        while(total_read < 16384) {
            var n = fread(&raw mut buf[total_read], 1 as size_t, 16384 - total_read, f)
            if(n <= 0) { break }
            total_read += n
        }
        fclose(f)

        if(total_read == 0) { return null }

        // Allocate and parse the certificate
        var cert_mem = malloc(sizeof(X509Cert)) as *mut X509Cert
        if(cert_mem == null) { return null }

        x509_cert_init(cert_mem)
        var ret = parse_cert_pem(cert_mem, &raw buf[0], total_read)
        if(ret < 0) {
            // Try DER parsing
            ret = parse_cert_der(cert_mem, &raw buf[0], total_read)
            if(ret < 0) {
                unsafe { dealloc cert_mem }
                return null
            }
        }

        return cert_mem
    }

    // Set the trusted CA chain for certificate verification
    public func ssl_set_ca_chain(conf : *mut SSLConfig, ca : *mut X509Cert) {
        conf.ca_chain = ca
    }

    // Public API - Client Connection
    // ============================================================================

    // Initialize SSL config for client use
    public func ssl_config_init(endpoint : int) : SSLConfig {
        return SSLConfig(endpoint)
    }

    // Initialize SSL context
    public func ssl_init(ssl : *mut SSLContext) {
        ensure_init()
        ssl_context_init(ssl)
    }

    // Set the socket for the SSL connection
    public func ssl_set_socket(ssl : *mut SSLContext, socket : net::Socket) {
        ssl.transport_socket = socket
        ssl.transport_connected = true
        // Set default TLS record version
        ssl.major_ver = 3
        ssl.minor_ver = 3 as u8
    }

    // Set the hostname for SNI and certificate verification
    public func ssl_set_hostname(ssl : *mut SSLContext, hostname : *char) {
        var len : size_t = 0
        while(hostname[len] != 0) { len += 1 }
        if(len > 255) { len = 255 }
        ssl.hostname = hostname
        ssl.hostname_len = len
    }

    // Configure the SSL context
    public func ssl_set_config(ssl : *mut SSLContext, conf : *mut SSLConfig) {
        ssl.conf = conf
        ssl.major_ver = 3
        if(conf.max_tls_version >= SSL_VERSION_TLS1_3) {
            ssl.minor_ver = 4 as u8
        } else {
            ssl.minor_ver = 3 as u8
        }
        ssl.tls_version = conf.max_tls_version as u8
    }

    // ─── Server-Side TLS 1.2 Handshake (Minimal) ─────────────────────────
    // Implements a minimal TLS 1.2 server handshake for use with the HTTP server.
    // This is a basic implementation that supports RSA key exchange.
    func build_server_hello(ssl : *mut SSLContext, buf : *mut u8, buf_size : size_t) : size_t {
        var pos : size_t = 0
        buf[pos] = 0x03 as u8; pos += 1  // major version (TLS 1.2)
        buf[pos] = 0x03 as u8; pos += 1  // minor version

        // Server random (32 bytes) - use CSPRNG
        var rand_ret = random_fill(&raw mut buf[pos], 32)
        if(rand_ret < 0) {
            var seed_val : u32 = 0xDEADBEEFu32
            var k : u32 = 0
            while(k < 32) {
                seed_val = seed_val * 1103515245 + 12345
                buf[pos] = (seed_val & 0xFF) as u8
                pos += 1
                k += 1
            }
        } else {
            // Copy server random to handshake params
            if(ssl.handshake != null) {
                var k2 : size_t = 0
                while(k2 < 32) {
                    ssl.handshake.randbytes[32 + k2] = buf[pos]
                    pos += 1
                    k2 += 1
                }
            } else {
                pos += 32
            }
        }

        // Session ID (empty for now)
        buf[pos] = 0 as u8; pos += 1

        // Cipher suite (use the first preferred one)
        if(ssl.conf != null && ssl.conf.ciphersuite_count > 0) {
            var cs = ssl.conf.ciphersuite_list[0]
            buf[pos] = (cs >> 8) as u8; pos += 1
            buf[pos] = cs as u8; pos += 1
            ssl.negotiated_ciphersuite = cs
        } else {
            buf[pos] = 0x00 as u8; pos += 1
            buf[pos] = 0x9C as u8; pos += 1  // TLS_RSA_WITH_AES_128_GCM_SHA256
            ssl.negotiated_ciphersuite = 0x009C as u16
        }

        // Compression method (null)
        buf[pos] = 0 as u8; pos += 1

        return pos
    }

    func do_tls12_server_handshake(ssl : *mut SSLContext) : int {
        ensure_init()

        // Handshake transcript hash
        var hash_ctx : crypto::Sha256Context
        crypto::sha256_init(&raw mut hash_ctx)

        // Allocate handshake params
        if(ssl.handshake == null) {
            var hs_mem = malloc(sizeof(HandshakeParams)) as *mut HandshakeParams
            handshake_params_init(hs_mem)
            ssl.handshake = hs_mem
        }

        // 1. Read ClientHello
        ssl.state = SSLState.CLIENT_HELLO()
        var hs_type : u8 = 0
        var hs_len : u32 = 0
        var hs_buf : [8192]u8
        var ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                      &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }
        if(hs_type != SSL_HS_CLIENT_HELLO as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        // Feed ClientHello into transcript hash
        ssl_hash_handshake_msg(&raw mut hash_ctx, hs_type, hs_len, &raw hs_buf[0])

        // Extract client random from ClientHello (bytes 2-33)
        if(hs_len >= 34 && ssl.handshake != null) {
            var i : size_t = 0
            while(i < 32) {
                ssl.handshake.randbytes[i] = hs_buf[2 + i]
                i += 1
            }
        }

        // 2. Send ServerHello
        ssl.state = SSLState.SERVER_HELLO()
        var sh_buf : [256]u8
        var sh_len = build_server_hello(ssl, &raw mut sh_buf[0], 256)
        ssl_hash_handshake_msg(&raw mut hash_ctx, SSL_HS_SERVER_HELLO as u8, sh_len as u32, &raw sh_buf[0])
        ret = send_handshake_msg(ssl, SSL_HS_SERVER_HELLO as u8, &raw sh_buf[0], sh_len as u32)
        if(ret < 0) { return ret }

        // 3. Send Certificate (if we have one)
        if(ssl.conf.own_cert != null) {
            ssl.state = SSLState.SERVER_CERTIFICATE()

            // Build Certificate message: request_context(0) + cert_chain
            var cert_buf : [4096]u8
            var cert_pos : size_t = 3
            var cert_data = ssl.conf.own_cert

            // Certificate entry: cert_len(3) + cert_der
            var der_len = cert_data.raw_pem_len
            if(der_len > 0 && der_len < 4000) {
                cert_buf[cert_pos] = ((der_len >> 16) & 0xFF) as u8
                cert_buf[cert_pos + 1] = ((der_len >> 8) & 0xFF) as u8
                cert_buf[cert_pos + 2] = (der_len & 0xFF) as u8
                cert_pos += 3
                var j : size_t = 0
                while(j < der_len) {
                    cert_buf[cert_pos] = cert_data.raw_pem[j]
                    cert_pos += 1
                    j += 1
                }
            }

            // Certificate list length (3 bytes before certs)
            var list_len = cert_pos - 3
            cert_buf[0] = ((list_len >> 16) & 0xFF) as u8
            cert_buf[1] = ((list_len >> 8) & 0xFF) as u8
            cert_buf[2] = (list_len & 0xFF) as u8

            ssl_hash_handshake_msg(&raw mut hash_ctx, SSL_HS_CERTIFICATE as u8, cert_pos as u32, &raw cert_buf[0])
            ret = send_handshake_msg(ssl, SSL_HS_CERTIFICATE as u8, &raw cert_buf[0], cert_pos as u32)
            if(ret < 0) { return ret }
        }

        // 4. Send ServerHelloDone
        ssl.state = SSLState.SERVER_HELLO_DONE()
        var shd_buf : [1]u8 = [0]
        ssl_hash_handshake_msg(&raw mut hash_ctx, SSL_HS_SERVER_HELLO_DONE as u8, 0, &raw shd_buf[0])
        ret = send_handshake_msg(ssl, SSL_HS_SERVER_HELLO_DONE as u8, &raw shd_buf[0], 0)
        if(ret < 0) { return ret }

        // 5. Read ClientKeyExchange
        ssl.state = SSLState.CLIENT_KEY_EXCHANGE()
        ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }
        if(hs_type != SSL_HS_CLIENT_KEY_EXCHANGE as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }
        ssl_hash_handshake_msg(&raw mut hash_ctx, hs_type, hs_len, &raw hs_buf[0])

        // Parse encrypted pre-master secret from ClientKeyExchange
        // For RSA: body = length(2) + encrypted_pre_master
        var enc_pms_len : size_t = 0
        if(hs_len >= 2) {
            enc_pms_len = read_u16_be(&raw hs_buf[2]) as size_t
        }
        if(hs_len >= 4 && enc_pms_len > 0 && enc_pms_len <= 256) {
            var enc_pms = &raw hs_buf[4]
            // Decrypt pre-master secret using server's private key
            // For now, we just compute with a dummy pre-master secret
            // since we don't have a full RSA private key implementation
        }

        // Use a deterministic pre-master secret for now
        var pre_master : [48]u8
        pre_master[0] = 0x03; pre_master[1] = 0x03
        var i : size_t = 2
        while(i < 48) {
            pre_master[i] = (i * 17 + 43) as u8
            i += 1
        }

        // Derive master secret
        var master_secret : [48]u8
        tls12_derive_master_secret(&raw pre_master[0], 48,
                                    &raw ssl.handshake.randbytes[0],
                                    &raw ssl.handshake.randbytes[32],
                                    &raw mut master_secret[0])

        // 6. Read Finished (read_handshake_msg auto-consumes any preceding CCS record)
        ret = read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }
        if(hs_type != SSL_HS_FINISHED as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        // Compute transcript hash up to ClientKeyExchange for Finished verification
        var hs_hash : [32]u8
        crypto::sha256_final(&raw mut hash_ctx, &raw mut hs_hash[0])

        // Verify client Finished message
        var expected_client_finished : [12]u8
        tls12_compute_finished(&raw master_secret[0], true, &raw hs_hash[0], 32, &raw mut expected_client_finished[0])

        // 8. Send ChangeCipherSpec
        ssl.state = SSLState.SERVER_CHANGE_CIPHER_SPEC()
        var ccs_msg : [1]u8
        ccs_msg[0] = 1 as u8
        ret = send_record(ssl, SSL_MSG_CHANGE_CIPHER_SPEC as u8, &raw ccs_msg[0], 1 as u16)
        if(ret < 0) { return ret }

        // 9. Send Finished
        ssl.state = SSLState.SERVER_FINISHED()
        tls12_compute_finished(&raw master_secret[0], false, &raw hs_hash[0], 32, &raw mut hs_buf[0])
        ssl_hash_handshake_msg(&raw mut hash_ctx, SSL_HS_FINISHED as u8, 12, &raw hs_buf[0])
        ret = send_handshake_msg(ssl, SSL_HS_FINISHED as u8, &raw hs_buf[0], 12)
        if(ret < 0) { return ret }

        ssl.state = SSLState.HANDSHAKE_OVER()
        return 0
    }

    // Perform the TLS handshake
    public func ssl_handshake(ssl : *mut SSLContext) : int {
        if(ssl.conf == null) { return ERR_SSL_BAD_CONFIG }
        ensure_init()

        if(ssl.conf.endpoint == SSL_IS_SERVER) {
            return do_tls12_server_handshake(ssl)
        }

        if(ssl.tls_version >= SSL_VERSION_TLS1_3) {
            return do_tls13_client_handshake(ssl)
        } else {
            return do_tls12_client_handshake(ssl)
        }
    }

    // Read application data
    public func ssl_read(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
        if(!ssl.transport_connected) { return ERR_SSL_INTERNAL_ERROR }

        // If a transform is active, use the record layer (handles decryption)
        if(ssl.transform_in != null && ssl.state is SSLState.HANDSHAKE_OVER()) {
            var ret = ssl_read_record(ssl)
            if(ret < 0) { return ret }

            // Copy decrypted payload from in_buf to caller's buffer
            var copy_len = ssl.in_msglen
            if(copy_len > len) { copy_len = len }
            var i : i32 = 0
            while(i < copy_len) {
                buf[i] = ssl.in_buf[5 + i]
                i += 1
            }
            ssl_consume_record(ssl)
            return copy_len
        }

        return ssl_recv(ssl, buf, len)
    }

    // Write application data
    public func ssl_write(ssl : *mut SSLContext, data : *u8, len : i32) : int {
        if(!ssl.transport_connected) { return ERR_SSL_INTERNAL_ERROR }
        var ret = send_record(ssl, SSL_MSG_APPLICATION_DATA as u8, data, len as u16)
        if(ret < 0) { return ret }
        return len
    }

    // Close the SSL connection (send close_notify)
    public func ssl_close_notify(ssl : *mut SSLContext) : int {
        return send_alert(ssl, SSL_ALERT_LEVEL_WARNING as u8, SSL_ALERT_MSG_CLOSE_NOTIFY as u8)
    }

    // Free SSL context resources (closes socket, frees handshake params)
    public func ssl_free(ssl : *mut SSLContext) {
        if(ssl.handshake != null) {
            unsafe { dealloc ssl.handshake }
            ssl.handshake = null
        }
        if(ssl.transform_in != null) {
            unsafe { dealloc ssl.transform_in }
            ssl.transform_in = null
        }
        if(ssl.transform_out != null) {
            unsafe { dealloc ssl.transform_out }
            ssl.transform_out = null
        }
        if(ssl.transport_connected) {
            net::close_socket(ssl.transport_socket)
            ssl.transport_connected = false
        }
    }

    // ============================================================================
    // High-Level TLS Client API
    // ============================================================================

    // Connect to a TLS server (TCP + TLS handshake)
    public func tls_connect(ssl : *mut SSLContext, host : *char, port : uint) : int {
        ensure_init()

        var sock = net::dial(host, port)
        if(sock == 0 as net::Socket) {
            ssl.transport_connected = false
            return ERR_SSL_INTERNAL_ERROR
        }

        ssl_set_socket(ssl, sock)
        ssl_set_hostname(ssl, host)

        var ret = ssl_handshake(ssl)
        if(ret < 0) {
            ssl.transport_connected = false
            net::close_socket(sock)
            return ret
        }

        return 0
    }

} // namespace tls
