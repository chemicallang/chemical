using namespace tls
using namespace crypto

@test
public func INT_hkdf_against_python(env : &mut TestEnv) {
    // Use a python script to compute TLS 1.3 handshake keys from random shared_secret + transcript_hash
    // Then compare with Chemical's HKDF

    // Generate random shared_secret and transcript_hash
    var shared_secret : [32]u8
    rand_bytes_gcm(&raw mut shared_secret[0], 32)
    var transcript_hash : [32]u8
    rand_bytes_gcm(&raw mut transcript_hash[0], 32)

    var ss_hex : [65]char; test_bytes_to_hex(&raw shared_secret[0], 32, &raw mut ss_hex[0])
    var th_hex : [65]char; test_bytes_to_hex(&raw transcript_hash[0], 32, &raw mut th_hex[0])

    // Python script to compute TLS 1.3 handshake keys
    var script : [2048]char; var sp : size_t = 0
    var hdr = "#!/usr/bin/python3\nimport hashlib,hmac\n\0" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si]; sp+=1; si+=1}

    var l = "def hkdf_expand_label(secret,label,ctx,length):\n hkl=length.to_bytes(2,'big');fl=b'tls13 '+label;hkl+=bytes([len(fl)]);hkl+=fl;hkl+=bytes([len(ctx)]);hkl+=ctx\n o=b'';t=b''\n for i in range(1,(length+31)//32+1):\n  t=hmac.new(secret,t+hkl+bytes([i]),hashlib.sha256).digest();o+=t\n return o[:length]\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    l="es=hmac.new(bytes(32),bytes(32),hashlib.sha256).digest()\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    l="d=hkdf_expand_label(es,b'derived',b'',32)\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    l="hs=hmac.new(d,bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(ss_hex[si]!=0){script[sp]=ss_hex[si]; sp+=1; si+=1}
    l="'),hashlib.sha256).digest()\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    // client/server handshake traffic secrets
    l="chts=hkdf_expand_label(hs,b'c hs traffic',bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si]; sp+=1; si+=1}
    l="'),32)\nshts=hkdf_expand_label(hs,b's hs traffic',bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si]; sp+=1; si+=1}
    l="'),32)\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    // keys
    l="ck=hkdf_expand_label(chts,b'key',b'',16);civ=hkdf_expand_label(chts,b'iv',b'',12)\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    l="sk=hkdf_expand_label(shts,b'key',b'',16);siv=hkdf_expand_label(shts,b'iv',b'',12)\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}

    // print
    l="print('CK='+ck.hex());print('CIV='+civ.hex());print('SK='+sk.hex());print('SIV='+siv.hex())\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    script[sp]=0

    test_write_file("/tmp/hkdf_py.py", &raw script[0] as *mut u8, sp)
    system("python3 /tmp/hkdf_py.py > /tmp/hkdf_py_out.txt 2>/dev/null\0" as *char)

    // Read and parse python output
    var py_out : [512]char
    test_read_file("/tmp/hkdf_py_out.txt", &raw mut py_out[0] as *mut u8, 512)
    var py_ck : [16]u8; var py_civ : [12]u8; var py_sk : [16]u8; var py_siv : [12]u8
    var pos : size_t = 0

    // Parse each key
    while(pos < 500) {
        if(py_out[pos]=='C' as u8 && py_out[pos+1]=='K' as u8 && py_out[pos+2]=='=' as u8) { pos+=3; break } else {}
        pos+=1
    }
    var i : size_t = 0; while(i<16){py_ck[i]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; i+=1}
    while(pos<500){if(py_out[pos]=='C' as u8 && py_out[pos+1]=='I' as u8 && py_out[pos+2]=='V' as u8 && py_out[pos+3]=='=' as u8){pos+=4;break}else{}pos+=1}
    i=0; while(i<12){py_civ[i]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; i+=1}
    while(pos<500){if(py_out[pos]=='S' as u8 && py_out[pos+1]=='K' as u8 && py_out[pos+2]=='=' as u8){pos+=3;break}else{}pos+=1}
    i=0; while(i<16){py_sk[i]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; i+=1}
    while(pos<500){if(py_out[pos]=='S' as u8 && py_out[pos+1]=='I' as u8 && py_out[pos+2]=='V' as u8 && py_out[pos+3]=='=' as u8){pos+=4;break}else{}pos+=1}
    i=0; while(i<12){py_siv[i]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; i+=1}

    // Now compute with Chemical HKDF using tls13_derive_handshake_keys
    // We need an SSLContext with a config for tls13_derive_handshake_keys
    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    // cfg needs to be valid; set it up minimally
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut cfg)

    // Derive keys
    var ret = tls13_derive_handshake_keys(&raw mut ctx, &raw shared_secret[0], 32, &raw transcript_hash[0])
    if(ret < 0) { env.error("tls13_derive_handshake_keys failed"); return } else {}

    // Check transform_out (client keys)
    if(ctx.transform_out == null) { env.error("transform_out is null"); return } else {}
    var tr_out = ctx.transform_out
    if(!test_bytes_eq(&raw tr_out.key_enc[0], &raw py_ck[0], 16)) {
        printf("[HKDF_TEST] client_key mismatch: chem[0]=%02x py[0]=%02x\n", tr_out.key_enc[0] as int, py_ck[0] as int)
        env.error("client_key mismatch")
        return
    } else {}
    if(!test_bytes_eq(&raw tr_out.base_iv_enc[0], &raw py_civ[0], 12)) {
        env.error("client_iv mismatch"); return
    } else {}

    // Check transform_in (server keys)
    if(ctx.transform_in == null) { env.error("transform_in is null"); return } else {}
    var tr_in = ctx.transform_in
    if(!test_bytes_eq(&raw tr_in.key_dec[0], &raw py_sk[0], 16)) {
        printf("[HKDF_TEST] server_key mismatch: chem[0]=%02x py[0]=%02x\n", tr_in.key_dec[0] as int, py_sk[0] as int)
        env.error("server_key mismatch")
        return
    } else {}
    if(!test_bytes_eq(&raw tr_in.base_iv_dec[0], &raw py_siv[0], 12)) {
        env.error("server_iv mismatch"); return
    } else {}
}
