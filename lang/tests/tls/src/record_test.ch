using namespace tls
using namespace crypto

// Test TLS 1.3 record encrypt/decrypt against Python
@test
public func INT_tls13_record_against_python(env : &mut TestEnv) {
    // Generate random keys and data
    var server_key : [16]u8; rand_bytes_gcm(&raw mut server_key[0], 16)
    var server_iv : [12]u8; rand_bytes_gcm(&raw mut server_iv[0], 12)
    var handshake_data : [23]u8; rand_bytes_gcm(&raw mut handshake_data[0], 23)  // Simulated EncryptedExtensions body

    // Set up Chemical transforms (like tls13_derive_handshake_keys would)
    var ctx : SSLContext; ssl_init(&raw mut ctx)
    // We need at minimum the transforms to be set up

    var tr : Transform; transform_init(&raw mut tr)
    tr.cipher_type = CIPHER_AES_128_GCM as u8
    tr.key_len = 16; tr.iv_len = 12; tr.fixed_iv_len = 12; tr.mac_key_len = 0
    var i : size_t = 0
    while(i<16) { tr.key_enc[i] = server_key[i]; tr.key_dec[i] = server_key[i]; i+=1 }
    i=0; while(i<12) { tr.base_iv_enc[i] = server_iv[i]; tr.base_iv_dec[i] = server_iv[i]; i+=1 }

    var tr_out = malloc(sizeof(Transform)) as *mut Transform; *tr_out = tr; ctx.transform_out = tr_out
    var tr_in = malloc(sizeof(Transform)) as *mut Transform; *tr_in = tr; ctx.transform_in = tr_in
    // Reset sequence numbers
    i=0; while(i<8){ctx.in_ctr[i]=0; ctx.out_ctr[i]=0; i+=1}

    // Encrypt with Chemical TLS 1.3 record layer
    var chem_enc : [256]u8
    var chem_enc_len = tls13_encrypt_record(&raw mut ctx, SSL_MSG_HANDSHAKE as u8,
                                             &raw handshake_data[0], 23,
                                             &raw mut chem_enc[0], 256)
    if(chem_enc_len < 0) { env.error("tls13_encrypt_record failed"); return } else {}

    // The outer header is at chem_enc[0..4], the encrypted payload at chem_enc[5..chem_enc_len-1]
    var outer_hdr = &raw chem_enc[0]
    var enc_payload = &raw chem_enc[5]
    var enc_payload_len : size_t = (chem_enc_len - 5) as size_t

    // Verify against Python
    var sk_hex : [33]char; test_bytes_to_hex(&raw server_key[0], 16, &raw mut sk_hex[0])
    var siv_hex : [25]char; test_bytes_to_hex(&raw server_iv[0], 12, &raw mut siv_hex[0])
    var aad_hex : [11]char; test_bytes_to_hex(outer_hdr, 5, &raw mut aad_hex[0])
    var ep_hex : [121]char; test_bytes_to_hex(enc_payload, enc_payload_len, &raw mut ep_hex[0])

    var script : [1024]char; var sp : size_t = 0
    var hdr = "#!/usr/bin/python3\nfrom cryptography.hazmat.primitives.ciphers.aead import AESGCM\n\0" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si]; sp+=1; si+=1}

    var l = "aesgcm=AESGCM(bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(sk_hex[si]!=0){script[sp]=sk_hex[si]; sp+=1; si+=1}
    l="'))\n\0" as *char; si=0; while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    // Decrypt with Python: pt = aesgcm.decrypt(nonce_bytes, ct+tag, aad_bytes)
    l="pt=aesgcm.decrypt(bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(siv_hex[si]!=0){script[sp]=siv_hex[si]; sp+=1; si+=1}
    l="'),bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(ep_hex[si]!=0){script[sp]=ep_hex[si]; sp+=1; si+=1}
    l="'),bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si]; sp+=1; si+=1}
    l="'))\n\0" as *char; si=0; while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    // Python decrypts; check inner content_type (last byte) and handshake data
    l="print('IC='+hex(pt[-1]))\nprint('DATA='+pt[:-1].hex())\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    script[sp]=0

    test_write_file("/tmp/tls13_rec_py.py", &raw script[0] as *mut u8, sp)
    system("python3 /tmp/tls13_rec_py.py > /tmp/tls13_rec_out.txt 2>/dev/null\0" as *char)

    var py_out : [512]char
    test_read_file("/tmp/tls13_rec_out.txt", &raw mut py_out[0] as *mut u8, 512)

    // Parse inner content type and data
    var pos : size_t = 0
    while(pos<500){if(py_out[pos]=='I' as u8 && py_out[pos+1]=='C' as u8 && py_out[pos+2]=='=' as u8){pos+=3;break}else{}pos+=1}
    // IC is in hex format like "0x16" - parse it (HEX FIX: shift by 4 before ORing new nibble)
    var ic_val : uint = 0
    while(pos<500 && py_out[pos]!=10 as u8) {  // until newline
        if(py_out[pos] == '0' as u8 && py_out[pos+1] == 'x' as u8) { pos+=2 } else {}
        ic_val = (ic_val << 4) | test_hex_char_val(py_out[pos]); pos+=1
    }

    // Find DATA=
    while(pos<500){if(py_out[pos]=='D' as u8 && py_out[pos+1]=='A' as u8 && py_out[pos+2]=='T' as u8 && py_out[pos+3]=='A' as u8 && py_out[pos+4]=='=' as u8){pos+=5;break}else{}pos+=1}
    var py_data : [64]u8
    i=0; while(i<23){py_data[i]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; i+=1}

    // Verify inner content type
    if(ic_val != (SSL_MSG_HANDSHAKE as uint)) {
        printf("[REC_TEST] inner_ct: chem=%d py=%d\n", SSL_MSG_HANDSHAKE as int, ic_val as int)
        env.error("inner content_type mismatch")
        return
    } else {}

    // Verify data
    if(!test_bytes_eq(&raw py_data[0], &raw handshake_data[0], 23)) {
        env.error("decrypted data mismatch")
        return
    } else {}

    // Now test decrypt direction: Python encrypts, Chemical decrypts
    var pt_hex : [47]char; test_bytes_to_hex(&raw handshake_data[0], 23, &raw mut pt_hex[0])

    // Python script to encrypt
    script[0]=0; sp=0; si=0
    hdr = "#!/usr/bin/python3\nfrom cryptography.hazmat.primitives.ciphers.aead import AESGCM\n\0" as *char
    while(hdr[si]!=0){script[sp]=hdr[si]; sp+=1; si+=1}
    l="aesgcm=AESGCM(bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(sk_hex[si]!=0){script[sp]=sk_hex[si]; sp+=1; si+=1}
    l="'))\n\0" as *char; si=0; while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    // Inner plaintext = data + content_type byte
    l="inner=b''.join([bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si]; sp+=1; si+=1}
    l="'),bytes([0x16])])\n\0" as *char; si=0; while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    // Build AAD: outer header = [0x17, 0x03, 0x03, high_len, low_len]
    // Python encrypts: ct_tag = aesgcm.encrypt(nonce, inner, aad)
    l="aad=bytes([0x17,0x03,0x03,(24+16)>>8,(24+16)&0xff])\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    l="ct_tag=aesgcm.encrypt(bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(siv_hex[si]!=0){script[sp]=siv_hex[si]; sp+=1; si+=1}
    l="'),inner,aad)\nprint('REC='+aad.hex()+ct_tag.hex())\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    script[sp]=0

    test_write_file("/tmp/tls13_enc_py.py", &raw script[0] as *mut u8, sp)
    system("python3 /tmp/tls13_enc_py.py > /tmp/tls13_enc_out.txt 2>/dev/null\0" as *char)

    py_out[0]=0
    test_read_file("/tmp/tls13_enc_out.txt", &raw mut py_out[0] as *mut u8, 512)

    // Parse REC=<hex>
    pos=0
    while(pos<500){if(py_out[pos]=='R' as u8 && py_out[pos+1]=='E' as u8 && py_out[pos+2]=='C' as u8 && py_out[pos+3]=='=' as u8){pos+=4;break}else{}pos+=1}
    // REC = aad_hex + ct_tag_hex = 5 bytes aad (10 hex) + (23+1+16)=40 bytes ct_tag (80 hex) = 90 hex chars
    // Parse the full REC hex string into bytes
    var rec_bytes : [128]u8
    var rbi : size_t = 0
    while(rbi < 45 && pos + 1 < 500) {
        // Check for newline
        if(py_out[pos] == 10 as u8 || py_out[pos] == 0) { break } else {}
        rec_bytes[rbi] = test_hex_pair_byte(py_out[pos], py_out[pos+1])
        pos+=2; rbi+=1
    }

    // rec_bytes = aad(5) + ct(24) + tag(16) = 45 bytes
    // Set up SSLContext for decrypt
    ctx.in_hdr[0] = rec_bytes[0]; ctx.in_hdr[1] = rec_bytes[1]
    ctx.in_hdr[2] = rec_bytes[2]; ctx.in_hdr[3] = rec_bytes[3]; ctx.in_hdr[4] = rec_bytes[4]
    // Reset in_ctr to 0
    i=0; while(i<8){ctx.in_ctr[i]=0; i+=1}

    var dec_buf : [256]u8
    var inner_ct : u8 = 0
    var dec_len = tls13_decrypt_record(&raw mut ctx, &raw rec_bytes[5], 40, &raw mut dec_buf[0], 256, &raw mut inner_ct)
    if(dec_len < 0) {
        printf("[REC_TEST] tls13_decrypt_record failed ret=%d\n", dec_len)
        env.error("Chemical TLS 1.3 decrypt failed")
        return
    } else {}

    if(dec_len != 23) {
        printf("[REC_TEST] dec_len=%d expected 23\n", dec_len)
        env.error("decrypted length wrong")
        return
    } else {}
    if(inner_ct != SSL_MSG_HANDSHAKE as u8) {
        printf("[REC_TEST] inner_ct=%d expected 22\n", inner_ct as int)
        env.error("inner content_type wrong")
        return
    } else {}
    if(!test_bytes_eq(&raw dec_buf[0], &raw handshake_data[0], 23)) {
        env.error("decrypted data mismatch")
        return
    } else {}
}
