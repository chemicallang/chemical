using namespace tls
using namespace crypto
using std::string_view

@test
public func INT_tls13_record_against_python(env : &mut TestEnv) {
    unsafe var server_key : [16]u8; test_random_bytes(&raw mut server_key[0], 16)
    unsafe var server_iv : [12]u8; test_random_bytes(&raw mut server_iv[0], 12)
    unsafe var handshake_data : [23]u8; test_random_bytes(&raw mut handshake_data[0], 23)

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)

    unsafe var tr : Transform; transform_init(&raw mut tr)
    tr.cipher_type = CIPHER_AES_128_GCM as u8
    tr.key_len = 16; tr.iv_len = 12; tr.fixed_iv_len = 12; tr.mac_key_len = 0
    var i : size_t = 0
    while(i<16) { tr.key_enc[i] = server_key[i]; tr.key_dec[i] = server_key[i]; i+=1 }
    i=0; while(i<12) { tr.base_iv_enc[i] = server_iv[i]; tr.base_iv_dec[i] = server_iv[i]; i+=1 }

    var tr_out = malloc(sizeof(Transform)) as *mut Transform; *tr_out = tr; ctx.transform_out = tr_out
    var tr_in = malloc(sizeof(Transform)) as *mut Transform; *tr_in = tr; ctx.transform_in = tr_in
    i=0; while(i<8){ctx.in_ctr[i]=0; ctx.out_ctr[i]=0; i+=1}

    unsafe var chem_enc : [256]u8
    var chem_enc_len = tls13_encrypt_record(&raw mut ctx, SSL_MSG_HANDSHAKE as u8,
                                             &raw handshake_data[0], 23,
                                             &raw mut chem_enc[0], 256)
    if(chem_enc_len < 0) { env.error("tls13_encrypt_record failed"); return } else {}

    var outer_hdr = &raw chem_enc[0]
    var enc_payload = &raw chem_enc[5]
    var enc_payload_len : size_t = (chem_enc_len - 5) as size_t

    unsafe var sk_hex : [33]char; test_bytes_to_hex(&raw server_key[0], 16, &raw mut sk_hex[0])
    unsafe var siv_hex : [25]char; test_bytes_to_hex(&raw server_iv[0], 12, &raw mut siv_hex[0])
    unsafe var aad_hex : [11]char; test_bytes_to_hex(outer_hdr, 5, &raw mut aad_hex[0])
    unsafe var ep_hex : [121]char; test_bytes_to_hex(enc_payload, enc_payload_len, &raw mut ep_hex[0])

    // Python decrypt: pt = aesgcm.decrypt(nonce, ct+tag, aad)
    unsafe var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}

    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(sk_hex[si]!=0){script[sp]=sk_hex[si] as u8; sp+=1; si+=1}
    l = "'))\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "pt=aesgcm.decrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(siv_hex[si]!=0){script[sp]=siv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ep_hex[si]!=0){script[sp]=ep_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "print('IC='+hex(pt[-1]))\nprint('DATA='+pt[:-1].hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls13_rec_py.py"))

    unsafe var ic_hex : [4]u8
    var ic_len = test_parse_py_hex_label(&raw mut py_out, string_view("IC=0x"), &raw mut ic_hex[0], 4)
    var ic_val : uint = 0
    if(ic_len > 0) { ic_val = ic_hex[0] as uint } else {
        // Try without 0x prefix
        ic_len = test_parse_py_hex_label(&raw mut py_out, string_view("IC="), &raw mut ic_hex[0], 4)
        if(ic_len > 0) { ic_val = ic_hex[0] as uint } else { env.error("failed to parse inner content type"); return }
    }

    unsafe var py_data : [64]u8
    var data_len = test_parse_py_hex_label(&raw mut py_out, string_view("DATA="), &raw mut py_data[0], 23)
    if(data_len != 23) { env.error("failed to parse data from Python output"); return } else {}

    if(ic_val != (SSL_MSG_HANDSHAKE as uint)) {
        printf("[REC_TEST] inner_ct: chem=%d py=%d\n", SSL_MSG_HANDSHAKE as int, ic_val as int)
        env.error("inner content_type mismatch")
        return
    } else {}
    if(!test_bytes_eq(&raw py_data[0], &raw handshake_data[0], 23)) {
        env.error("decrypted data mismatch")
        return
    } else {}

    // Python encrypts, Chemical decrypts
    unsafe var pt_hex : [47]char; test_bytes_to_hex(&raw handshake_data[0], 23, &raw mut pt_hex[0])

    script[0]=0; sp=0; si=0
    hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(sk_hex[si]!=0){script[sp]=sk_hex[si] as u8; sp+=1; si+=1}
    l = "'))\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "inner=b''.join([bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes([0x16])])\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "aad=bytes([0x17,0x03,0x03,(24+16)>>8,(24+16)&0xff])\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "ct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(siv_hex[si]!=0){script[sp]=siv_hex[si] as u8; sp+=1; si+=1}
    l = "'),inner,aad)\nprint('REC='+aad.hex()+ct_tag.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    py_out = test_python_run_script(&raw script[0], sp, string_view("tls13_enc_py.py"))

    unsafe var rec_bytes : [128]u8
    var rec_len = test_parse_py_hex_label(&raw mut py_out, string_view("REC="), &raw mut rec_bytes[0], 45)
    if(rec_len != 45) { env.error("failed to parse REC from Python output"); return } else {}

    ctx.in_hdr[0] = rec_bytes[0]; ctx.in_hdr[1] = rec_bytes[1]
    ctx.in_hdr[2] = rec_bytes[2]; ctx.in_hdr[3] = rec_bytes[3]; ctx.in_hdr[4] = rec_bytes[4]
    i=0; while(i<8){ctx.in_ctr[i]=0; i+=1}

    unsafe var dec_buf : [256]u8
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
