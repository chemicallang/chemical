using namespace tls
using namespace crypto
using std::string_view

@test
public func INT_hkdf_against_python(env : &mut TestEnv) {
    unsafe var shared_secret : [32]u8
    test_random_bytes(&raw mut shared_secret[0], 32)
    unsafe var transcript_hash : [32]u8
    test_random_bytes(&raw mut transcript_hash[0], 32)

    unsafe var ss_hex : [65]char; test_bytes_to_hex(&raw shared_secret[0], 32, &raw mut ss_hex[0])
    unsafe var th_hex : [65]char; test_bytes_to_hex(&raw transcript_hash[0], 32, &raw mut th_hex[0])

    unsafe var script : [2048]u8; var sp : size_t = 0
    var hdr = "import hashlib,hmac\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}

    var l = "def hkdf_expand_label(secret,label,ctx,length):\n hkl=length.to_bytes(2,'big');fl=b'tls13 '+label;hkl+=bytes([len(fl)]);hkl+=fl;hkl+=bytes([len(ctx)]);hkl+=ctx\n o=b'';t=b''\n for i in range(1,(length+31)//32+1):\n  t=hmac.new(secret,t+hkl+bytes([i]),hashlib.sha256).digest();o+=t\n return o[:length]\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "es=hmac.new(bytes(32),bytes(32),hashlib.sha256).digest()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "empty_hash=hashlib.sha256(b'').digest()\nd=hkdf_expand_label(es,b'derived',empty_hash,32)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "hs=hmac.new(d,bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ss_hex[si]!=0){script[sp]=ss_hex[si] as u8; sp+=1; si+=1}
    l = "'),hashlib.sha256).digest()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "chts=hkdf_expand_label(hs,b'c hs traffic',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si] as u8; sp+=1; si+=1}
    l = "'),32)\nshts=hkdf_expand_label(hs,b's hs traffic',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si] as u8; sp+=1; si+=1}
    l = "'),32)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "ck=hkdf_expand_label(chts,b'key',b'',16);civ=hkdf_expand_label(chts,b'iv',b'',12)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "sk=hkdf_expand_label(shts,b'key',b'',16);siv=hkdf_expand_label(shts,b'iv',b'',12)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "print('CK='+ck.hex());print('CIV='+civ.hex());print('SK='+sk.hex());print('SIV='+siv.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("hkdf_py.py"))

    unsafe var py_ck : [16]u8; unsafe var py_civ : [12]u8; unsafe var py_sk : [16]u8; unsafe var py_siv : [12]u8
    var ck_len = test_parse_py_hex_label(&raw mut py_out, string_view("CK="), &raw mut py_ck[0], 16)
    var civ_len = test_parse_py_hex_label(&raw mut py_out, string_view("CIV="), &raw mut py_civ[0], 12)
    var sk_len = test_parse_py_hex_label(&raw mut py_out, string_view("SK="), &raw mut py_sk[0], 16)
    var siv_len = test_parse_py_hex_label(&raw mut py_out, string_view("SIV="), &raw mut py_siv[0], 12)
    if(ck_len != 16 || civ_len != 12 || sk_len != 16 || siv_len != 12) {
        env.error("failed to parse all keys from Python output"); return
    } else {}

    unsafe var ctx : SSLContext; ssl_init(&raw mut ctx)
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut cfg)

    var ret = tls13_derive_handshake_keys(&raw mut ctx, &raw shared_secret[0], 32, &raw transcript_hash[0])
    if(ret < 0) { env.error("tls13_derive_handshake_keys failed"); return } else {}

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
