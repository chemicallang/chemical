using namespace tls
using namespace crypto
using std::string_view
using std::string

@test
public func INT_tls13_app_keys_vs_py(env : &mut TestEnv) {
    var shared_secret : [32]u8
    test_random_bytes(&raw mut shared_secret[0], 32)
    var transcript_hash : [32]u8
    test_random_bytes(&raw mut transcript_hash[0], 32)

    var ss_hex : [65]char; test_bytes_to_hex(&raw shared_secret[0], 32, &raw mut ss_hex[0])
    var th_hex : [65]char; test_bytes_to_hex(&raw transcript_hash[0], 32, &raw mut th_hex[0])

    var script : [4608]u8; var sp : size_t = 0
    var hdr = "import hashlib,hmac\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}

    var l = "def hkdf_expand_label(secret,label,ctx,length):\n hkl=length.to_bytes(2,'big');fl=b'tls13 '+label;hkl+=bytes([len(fl)]);hkl+=fl;hkl+=bytes([len(ctx)]);hkl+=ctx\n o=b'';t=b''\n for i in range(1,(length+31)//32+1):\n  t=hmac.new(secret,t+hkl+bytes([i]),hashlib.sha256).digest();o+=t\n return o[:length]\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "def hkdf_extract(salt,ikm):\n return hmac.new(salt,ikm,hashlib.sha256).digest()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "es=hkdf_extract(bytes(32),bytes(32))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "empty_hash=hashlib.sha256(b'').digest()\nd=hkdf_expand_label(es,b'derived',empty_hash,32)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "hs=hkdf_extract(d,bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ss_hex[si]!=0){script[sp]=ss_hex[si] as u8; sp+=1; si+=1}
    l = "'))\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "chts=hkdf_expand_label(hs,b'c hs traffic',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si] as u8; sp+=1; si+=1}
    l = "'),32)\nshts=hkdf_expand_label(hs,b's hs traffic',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si] as u8; sp+=1; si+=1}
    l = "'),32)\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "d2=hkdf_expand_label(hs,b'derived',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si] as u8; sp+=1; si+=1}
    l = "'),32)\nms=hkdf_extract(d2,bytes(32))\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "cats=hkdf_expand_label(ms,b'c ap traffic',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si] as u8; sp+=1; si+=1}
    l = "'),32)\nsats=hkdf_expand_label(ms,b's ap traffic',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si] as u8; sp+=1; si+=1}
    l = "'),32)\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "rms=hkdf_expand_label(ms,b'res master',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(th_hex[si]!=0){script[sp]=th_hex[si] as u8; sp+=1; si+=1}
    l = "'),32)\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    l = "print('ES='+es.hex())\nprint('HS='+hs.hex())\nprint('CHTS='+chts.hex())\nprint('SHTS='+shts.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('MS='+ms.hex())\nprint('CATS='+cats.hex())\nprint('SATS='+sats.hex())\nprint('RMS='+rms.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls13_keys_py.py"))

    var py_es : [32]u8; var py_hs : [32]u8; var py_chts : [32]u8; var py_shts : [32]u8
    var py_ms : [32]u8; var py_cats : [32]u8; var py_sats : [32]u8; var py_rms : [32]u8
    var es_ok = test_parse_py_hex_label(&raw mut py_out, string_view("ES="), &raw mut py_es[0], 32) == 32
    var hs_ok = test_parse_py_hex_label(&raw mut py_out, string_view("HS="), &raw mut py_hs[0], 32) == 32
    var chts_ok = test_parse_py_hex_label(&raw mut py_out, string_view("CHTS="), &raw mut py_chts[0], 32) == 32
    var shts_ok = test_parse_py_hex_label(&raw mut py_out, string_view("SHTS="), &raw mut py_shts[0], 32) == 32
    var ms_ok = test_parse_py_hex_label(&raw mut py_out, string_view("MS="), &raw mut py_ms[0], 32) == 32
    var cats_ok = test_parse_py_hex_label(&raw mut py_out, string_view("CATS="), &raw mut py_cats[0], 32) == 32
    var sats_ok = test_parse_py_hex_label(&raw mut py_out, string_view("SATS="), &raw mut py_sats[0], 32) == 32
    var rms_ok = test_parse_py_hex_label(&raw mut py_out, string_view("RMS="), &raw mut py_rms[0], 32) == 32
    if(!es_ok || !hs_ok || !chts_ok || !shts_ok || !ms_ok || !cats_ok || !sats_ok || !rms_ok) {
        env.error("failed to parse Python output"); return
    } else {}

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut cfg)

    var ret = tls13_derive_handshake_keys(&raw mut ctx, &raw shared_secret[0], 32, &raw transcript_hash[0])
    if(ret < 0) { env.error("tls13_derive_handshake_keys failed"); return } else {}

    if(!test_bytes_eq(&raw ctx.tls13_keys.early_secret[0], &raw py_es[0], 32)) { env.error("early_secret mismatch"); return } else {}
    if(!test_bytes_eq(&raw ctx.tls13_keys.handshake_secret[0], &raw py_hs[0], 32)) { env.error("handshake_secret mismatch"); return } else {}
    if(!test_bytes_eq(&raw ctx.tls13_keys.client_handshake_traffic_secret[0], &raw py_chts[0], 32)) { env.error("client_hts mismatch"); return } else {}
    if(!test_bytes_eq(&raw ctx.tls13_keys.server_handshake_traffic_secret[0], &raw py_shts[0], 32)) { env.error("server_hts mismatch"); return } else {}

    ret = tls13_derive_application_keys(&raw mut ctx, &raw transcript_hash[0], 32)
    if(ret < 0) { env.error("tls13_derive_application_keys failed"); return } else {}

    if(!test_bytes_eq(&raw ctx.tls13_keys.master_secret[0], &raw py_ms[0], 32)) { env.error("master_secret mismatch"); return } else {}
    if(!test_bytes_eq(&raw ctx.tls13_keys.client_application_traffic_secret[0], &raw py_cats[0], 32)) { env.error("client_ats mismatch"); return } else {}
    if(!test_bytes_eq(&raw ctx.tls13_keys.server_application_traffic_secret[0], &raw py_sats[0], 32)) { env.error("server_ats mismatch"); return } else {}
    if(!test_bytes_eq(&raw ctx.tls13_keys.resumption_master_secret[0], &raw py_rms[0], 32)) { env.error("resumption_ms mismatch"); return } else {}
}

@test
public func INT_ecdsa_verify_py_signature_tls(env : &mut TestEnv) {
    var hash_val : [32]u8; test_random_bytes(&raw mut hash_val[0], 32)
    var hash_hex : [65]char; test_bytes_to_hex(&raw hash_val[0], 32, &raw mut hash_hex[0])

    var script : [2048]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec,utils\nfrom cryptography.hazmat.primitives import hashes\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "from cryptography.hazmat.primitives.serialization import Encoding,PublicFormat\nkey=ec.generate_private_key(ec.SECP256R1())\npub=key.public_key()\npub_enc=pub.public_bytes(Encoding.X962,PublicFormat.UncompressedPoint)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "hash_data=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(hash_hex[si]!=0){script[sp]=hash_hex[si] as u8; sp+=1; si+=1}
    l = "')\nsig=key.sign(hash_data,ec.ECDSA(utils.Prehashed(hashes.SHA256())))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('PUB='+pub_enc.hex())\nprint('SIG='+sig.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdsa_vfy_keygen.py"))
    var py_pub : [65]u8; var py_sig : [256]u8
    var pub_len = test_parse_py_hex_label(&raw mut py_out, string_view("PUB="), &raw mut py_pub[0], 65)
    var sig_len = test_parse_py_hex_label(&raw mut py_out, string_view("SIG="), &raw mut py_sig[0], 128)
    if(pub_len != 65 || sig_len < 64) { env.error("failed to parse Python key/sig"); return } else {}

    var ecdsa_ctx : ECDSAContext; ecdsa_init(&raw mut ecdsa_ctx)
    var ret = ecdsa_import_pubkey(&raw mut ecdsa_ctx, &raw py_pub[0], 65, TLS_GROUP_SECP256R1 as u16)
    if(ret < 0) { env.error("import pubkey failed"); return } else {}

    ret = ecdsa_verify(&raw mut ecdsa_ctx, &raw hash_val[0], 32, &raw py_sig[0], sig_len)
    if(ret < 0) { env.error("Chemical failed to verify Python's ECDSA signature"); return } else {}
}

@test
public func INT_aes256_gcm_non12iv_vs_py(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var iv8 : [8]u8; test_random_bytes(&raw mut iv8[0], 8)
    var aad : [13]u8; test_random_bytes(&raw mut aad[0], 13)
    var pt : [37]u8; test_random_bytes(&raw mut pt[0], 37)

    var key_hex : [65]char; test_bytes_to_hex(&raw key[0], 32, &raw mut key_hex[0])
    var iv_hex : [17]char; test_bytes_to_hex(&raw iv8[0], 8, &raw mut iv_hex[0])
    var aad_hex : [27]char; test_bytes_to_hex(&raw aad[0], 13, &raw mut aad_hex[0])
    var pt_hex : [75]char; test_bytes_to_hex(&raw pt[0], 37, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "ct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('CT='+ct_tag[:37].hex())\nprint('TAG='+ct_tag[37:].hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("aes256_gcm_py.py"))
    var py_ct : [37]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 37)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 37 || tag_len != 16) { env.error("failed to parse Python output"); return } else {}

    var gcm : GCMContext
    var ret = gcm_init(&raw mut gcm, &raw key[0], 32)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var chem_ct : [64]u8; var chem_tag : [16]u8
    ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv8[0], 8, &raw aad[0], 13, &raw pt[0], 37, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 37)) { env.error("ciphertext mismatch"); return } else {}
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("tag mismatch"); return } else {}

    var gcm2 : GCMContext; gcm_init(&raw mut gcm2, &raw key[0], 32)
    var chem_dec : [64]u8
    ret = gcm_auth_decrypt(&raw mut gcm2, &raw iv8[0], 8, &raw aad[0], 13, &raw py_ct[0], 37, &raw py_tag[0], 16, &raw mut chem_dec[0])
    if(ret < 0) { env.error("gcm_auth_decrypt failed"); return } else {}
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 37)) { env.error("decrypt roundtrip mismatch"); return } else {}
}

@test
public func INT_rsa_cross_encrypt_vs_py(env : &mut TestEnv) {
    var rsa_ctx : RSAContext; rsa_init(&raw mut rsa_ctx, RSA_PKCS_V15, 0)
    var ret = rsa_gen_key(&raw mut rsa_ctx, 2048, 65537)
    if(ret < 0) { env.error("rsa_gen_key failed"); return } else {}

    var n_len = rsa_get_len(&raw mut rsa_ctx)
    var n_buf : [256]u8; var e_buf : [4]u8
    mpi_write_binary(&raw mut rsa_ctx.N, &raw mut n_buf[0], n_len)
    mpi_write_binary(&raw mut rsa_ctx.E, &raw mut e_buf[0], 4)

    var n_hex : [513]char; test_bytes_to_hex(&raw n_buf[0], n_len, &raw mut n_hex[0])
    var e_hex : [9]char; test_bytes_to_hex(&raw e_buf[0], 4, &raw mut e_hex[0])

    var pt_msg : [32]u8; test_random_bytes(&raw mut pt_msg[0], 32)
    var pt_hex : [65]char; test_bytes_to_hex(&raw pt_msg[0], 32, &raw mut pt_hex[0])

    var script : [2048]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import rsa, padding\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "n=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(n_hex[si]!=0){script[sp]=n_hex[si] as u8; sp+=1; si+=1}
    l = "'),'big')\ne=int.from_bytes(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(e_hex[si]!=0){script[sp]=e_hex[si] as u8; sp+=1; si+=1}
    l = "'),'big')\npub=rsa.RSAPublicNumbers(e,n).public_key()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "ct=pub.encrypt(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),padding.PKCS1v15())\nprint('CT='+ct.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("rsa_enc_py.py"))
    var py_ct : [256]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], n_len)
    if(ct_len != n_len) { env.error("failed to parse Python ciphertext"); return } else {}

    var chem_dec : [256]u8; var dec_len : size_t = 256
    ret = rsa_pkcs1_decrypt(&raw mut rsa_ctx, &raw py_ct[0], n_len, &raw mut chem_dec[0], &raw mut dec_len, 64)
    if(ret < 0) { env.error("rsa_pkcs1_decrypt failed"); return } else {}
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt_msg[0], 32)) { env.error("decrypted plaintext mismatch"); return } else {}

    var rsa_ctx2 : RSAContext; rsa_init(&raw mut rsa_ctx2, RSA_PKCS_V15, 0)
    ret = rsa_import_pubkey(&raw mut rsa_ctx2, &raw n_buf[0], n_len, &raw e_buf[0], 4)
    if(ret < 0) { env.error("rsa_import_pubkey failed"); return } else {}

    var chem_ct : [256]u8
    ret = rsa_pkcs1_encrypt(&raw mut rsa_ctx2, &raw pt_msg[0], 32, &raw mut chem_ct[0])
    if(ret < 0) { env.error("rsa_pkcs1_encrypt failed"); return } else {}

    var chem_ct_hex : [513]char; test_bytes_to_hex(&raw chem_ct[0], n_len, &raw mut chem_ct_hex[0])

    // Cross check: Chemical encrypts, then Python DECRYPTS with the private key.
    // (Python re-encrypting the same plaintext cannot match: PKCS#1 v1.5 padding is randomized.)
    var p_buf : [256]u8; var q_buf : [256]u8; var d_buf : [256]u8
    var dp_buf : [256]u8; var dq_buf : [256]u8; var qp_buf : [256]u8
    mpi_write_binary(&raw mut rsa_ctx.P, &raw mut p_buf[0], n_len / 2)
    mpi_write_binary(&raw mut rsa_ctx.Q, &raw mut q_buf[0], n_len / 2)
    mpi_write_binary(&raw mut rsa_ctx.D, &raw mut d_buf[0], n_len)
    mpi_write_binary(&raw mut rsa_ctx.DP, &raw mut dp_buf[0], n_len / 2)
    mpi_write_binary(&raw mut rsa_ctx.DQ, &raw mut dq_buf[0], n_len / 2)
    mpi_write_binary(&raw mut rsa_ctx.QP, &raw mut qp_buf[0], n_len / 2)
    var p_hex : [257]char; test_bytes_to_hex(&raw p_buf[0], n_len / 2, &raw mut p_hex[0])
    var q_hex : [257]char; test_bytes_to_hex(&raw q_buf[0], n_len / 2, &raw mut q_hex[0])
    var d_hex : [513]char; test_bytes_to_hex(&raw d_buf[0], n_len, &raw mut d_hex[0])
    var dp_hex : [257]char; test_bytes_to_hex(&raw dp_buf[0], n_len / 2, &raw mut dp_hex[0])
    var dq_hex : [257]char; test_bytes_to_hex(&raw dq_buf[0], n_len / 2, &raw mut dq_hex[0])
    var qp_hex : [257]char; test_bytes_to_hex(&raw qp_buf[0], n_len / 2, &raw mut qp_hex[0])

    var script2 : [4096]u8; var sp2 : size_t = 0; si = 0
    hdr = "from cryptography.hazmat.primitives.asymmetric import rsa, padding\nfrom cryptography.hazmat.primitives.asymmetric.rsa import RSAPrivateNumbers, RSAPublicNumbers\n" as *char; si=0
    while(hdr[si]!=0){script2[sp2]=hdr[si] as u8; sp2+=1; si+=1}
    l = "n=int.from_bytes(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(n_hex[si]!=0){script2[sp2]=n_hex[si] as u8; sp2+=1; si+=1}
    l = "'),'big')\ne=int.from_bytes(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(e_hex[si]!=0){script2[sp2]=e_hex[si] as u8; sp2+=1; si+=1}
    l = "'),'big')\np=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(p_hex[si]!=0){script2[sp2]=p_hex[si] as u8; sp2+=1; si+=1}
    l = "'),'big')\nq=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(q_hex[si]!=0){script2[sp2]=q_hex[si] as u8; sp2+=1; si+=1}
    l = "'),'big')\nd=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(d_hex[si]!=0){script2[sp2]=d_hex[si] as u8; sp2+=1; si+=1}
    l = "'),'big')\ndmp1=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(dp_hex[si]!=0){script2[sp2]=dp_hex[si] as u8; sp2+=1; si+=1}
    l = "'),'big')\ndmq1=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(dq_hex[si]!=0){script2[sp2]=dq_hex[si] as u8; sp2+=1; si+=1}
    l = "'),'big')\niqmp=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(qp_hex[si]!=0){script2[sp2]=qp_hex[si] as u8; sp2+=1; si+=1}
    l = "'),'big')\nct=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}
    si=0; while(chem_ct_hex[si]!=0){script2[sp2]=chem_ct_hex[si] as u8; sp2+=1; si+=1}
    l = "')\npriv=RSAPrivateNumbers(p,q,d,dmp1,dmq1,iqmp,RSAPublicNumbers(e,n)).private_key()\npt=priv.decrypt(ct,padding.PKCS1v15())\nprint('PT='+pt.hex())\n" as *char; si=0
    while(l[si]!=0){script2[sp2]=l[si] as u8; sp2+=1; si+=1}

    var py_out2 = test_python_run_script(&raw script2[0], sp2, string_view("rsa_enc2_py.py"))
    var py_pt : [64]u8
    var pt_len = test_parse_py_hex_label(&raw mut py_out2, string_view("PT="), &raw mut py_pt[0], 32)
    if(pt_len != 32 || !test_bytes_eq(&raw py_pt[0], &raw pt_msg[0], 32)) {
        env.error("Python cannot decrypt Chemical RSA ciphertext")
    } else {}
}

@test
public func INT_aes_cbc_hmac_tls12_vs_py(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [16]u8; test_random_bytes(&raw mut iv[0], 16)
    var mac_key : [16]u8; test_random_bytes(&raw mut mac_key[0], 16)
    var pt : [48]u8; test_random_bytes(&raw mut pt[0], 48)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [33]char; test_bytes_to_hex(&raw iv[0], 16, &raw mut iv_hex[0])
    var mac_key_hex : [33]char; test_bytes_to_hex(&raw mac_key[0], 16, &raw mut mac_key_hex[0])
    var pt_hex : [97]char; test_bytes_to_hex(&raw pt[0], 48, &raw mut pt_hex[0])

    var aes_ctx : AESContext
    aes_setkey_enc(&raw mut aes_ctx, &raw key[0], 16)
    var chem_iv : [16]u8; var ci : size_t = 0; while(ci < 16) { chem_iv[ci] = iv[ci]; ci += 1 }
    var chem_ct : [64]u8
    var ret = aes_crypt_cbc(&raw mut aes_ctx, AES_ENCRYPT, 48, &raw mut chem_iv[0], &raw pt[0], &raw mut chem_ct[0])
    if(ret < 0) { env.error("aes_crypt_cbc encrypt failed"); return } else {}

    var chem_mac : [32]u8
    hmac_sha256(&raw mac_key[0], 16, &raw chem_ct[0], 48, &raw mut chem_mac[0])

    var script : [1536]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes\nimport hmac,hashlib\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "cipher=Cipher(algorithms.AES(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "')),modes.CBC(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "')))\nenc=cipher.encryptor()\npy_ct=enc.update(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'))+enc.finalize()\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "hm=hmac.new(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(mac_key_hex[si]!=0){script[sp]=mac_key_hex[si] as u8; sp+=1; si+=1}
    l = "'),py_ct,hashlib.sha256).digest()\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('CT='+py_ct.hex())\nprint('MAC='+hm.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("cbc_hmac_py.py"))
    var py_ct : [48]u8; var py_mac : [32]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 48)
    var mac_len = test_parse_py_hex_label(&raw mut py_out, string_view("MAC="), &raw mut py_mac[0], 32)
    if(ct_len != 48 || mac_len != 32) { env.error("failed to parse Python output"); return } else {}

    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 48)) { env.error("CBC ciphertext mismatch"); return } else {}
    if(!test_bytes_eq(&raw chem_mac[0], &raw py_mac[0], 32)) { env.error("HMAC mismatch"); return } else {}

    var aes_dec : AESContext; aes_setkey_enc(&raw mut aes_dec, &raw key[0], 128)
    var dec_iv : [16]u8; ci = 0; while(ci < 16) { dec_iv[ci] = iv[ci]; ci += 1 }
    var chem_dec : [64]u8
    ret = aes_crypt_cbc(&raw mut aes_dec, AES_DECRYPT, 48, &raw mut dec_iv[0], &raw py_ct[0], &raw mut chem_dec[0])
    if(ret < 0) { env.error("aes_crypt_cbc decrypt failed"); return } else {}
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 48)) { env.error("CBC decrypt roundtrip mismatch"); return } else {}
}

@test
public func INT_x509_cert_parse_vs_py(env : &mut TestEnv) {
    var script : [1536]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography import x509\nfrom cryptography.x509.oid import NameOID\nfrom cryptography.hazmat.primitives import hashes\nfrom cryptography.hazmat.primitives.asymmetric import ec\nimport datetime\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "key=ec.generate_private_key(ec.SECP256R1())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "cert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'test.example.com')])).issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'TestCA')])).public_key(key.public_key()).serial_number(12345).not_valid_before(datetime.datetime(2024,1,1)).not_valid_after(datetime.datetime(2026,1,1)).sign(key,hashes.SHA256()))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "from cryptography.hazmat.primitives.serialization import Encoding\nder=cert.public_bytes(Encoding.DER)\nf=open('/tmp/chem_test_cert.der','wb');f.write(der);f.close()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("cert_gen.py"))

    var cert_file = fopen("/tmp/chem_test_cert.der\0" as *char, "rb\0" as *char)
    if(cert_file == null) { env.error("cannot open generated cert"); return } else {}
    var cert_buf : [2048]u8
    var cert_len = fread(&raw mut cert_buf[0] as *mut void, 1 as size_t, 2048, cert_file)
    fclose(cert_file)

    var crt : X509Cert; x509_cert_init(&raw mut crt)
    var ret = parse_cert_der(&raw mut crt, &raw cert_buf[0], cert_len)
    if(ret < 0) { env.error("parse_cert_der failed"); return } else {}
    if(crt.pk_type != PK_ECKEY) { env.error("expected EC key type"); return } else {}
    if(crt.pk_bitlen != 256) { env.error("expected 256-bit EC key"); return } else {}
}

@test
public func INT_x509_hostname_verify_vs_py(env : &mut TestEnv) {
    var script : [1536]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography import x509\nfrom cryptography.x509.oid import NameOID\nfrom cryptography.hazmat.primitives import hashes\nfrom cryptography.hazmat.primitives.asymmetric import ec\nimport datetime\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "key=ec.generate_private_key(ec.SECP256R1())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "cert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'myhost.example.com')])).issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'myhost.example.com')])).public_key(key.public_key()).serial_number(1).not_valid_before(datetime.datetime(2024,1,1)).not_valid_after(datetime.datetime(2026,1,1)).sign(key,hashes.SHA256()))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "from cryptography.hazmat.primitives.serialization import Encoding\nder=cert.public_bytes(Encoding.DER)\nf=open('/tmp/chem_hostname_cert.der','wb');f.write(der);f.close()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("cert_hostname.py"))

    var cert_file = fopen("/tmp/chem_hostname_cert.der\0" as *char, "rb\0" as *char)
    if(cert_file == null) { env.error("cannot open generated cert"); return } else {}
    var cert_buf : [2048]u8
    var cert_len = fread(&raw mut cert_buf[0] as *mut void, 1 as size_t, 2048, cert_file)
    fclose(cert_file)

    var crt : X509Cert; x509_cert_init(&raw mut crt)
    var ret = parse_cert_der(&raw mut crt, &raw cert_buf[0], cert_len)
    if(ret < 0) { env.error("parse_cert_der failed"); return } else {}

    var match = x509_verify_hostname(&raw mut crt, "myhost.example.com" as *char)
    if(match != 0) { env.error("expected hostname to match"); return } else {}

    var nomatch = x509_verify_hostname(&raw mut crt, "wrong.example.com" as *char)
    if(nomatch == 0) { env.error("expected hostname mismatch"); return } else {}
}

@test
public func INT_x509_ecdsa_self_sig_verify(env : &mut TestEnv) {
    var script : [1536]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography import x509\nfrom cryptography.x509.oid import NameOID\nfrom cryptography.hazmat.primitives import hashes\nfrom cryptography.hazmat.primitives.asymmetric import ec\nimport datetime\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "key=ec.generate_private_key(ec.SECP256R1())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "cert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'sigtest')])).issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'sigtest')])).public_key(key.public_key()).serial_number(42).not_valid_before(datetime.datetime(2024,1,1)).not_valid_after(datetime.datetime(2026,1,1)).sign(key,hashes.SHA256()))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "from cryptography.hazmat.primitives.serialization import Encoding,PublicFormat\npub_enc=key.public_key().public_bytes(Encoding.X962,PublicFormat.UncompressedPoint)\nder=cert.public_bytes(Encoding.DER)\nf=open('/tmp/chem_sig_cert.der','wb');f.write(der);f.close()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('PUB='+pub_enc.hex())\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("cert_sig.py"))

    var py_pub : [65]u8
    var pub_len = test_parse_py_hex_label(&raw mut py_out, string_view("PUB="), &raw mut py_pub[0], 65)
    if(pub_len != 65) { env.error("failed to parse pubkey"); return } else {}

    var cert_file = fopen("/tmp/chem_sig_cert.der\0" as *char, "rb\0" as *char)
    if(cert_file == null) { env.error("cannot open cert"); return } else {}
    var cert_buf : [2048]u8
    var cert_len = fread(&raw mut cert_buf[0] as *mut void, 1 as size_t, 2048, cert_file)
    fclose(cert_file)

    var crt : X509Cert; x509_cert_init(&raw mut crt)
    var ret = parse_cert_der(&raw mut crt, &raw cert_buf[0], cert_len)
    if(ret < 0) { env.error("parse_cert_der failed"); return } else {}
    if(crt.pk_type != PK_ECKEY) { env.error("expected EC key type"); return } else {}

    var issuer : ECDSAContext; ecdsa_init(&raw mut issuer)
    ret = ecdsa_import_pubkey(&raw mut issuer, &raw py_pub[0], 65, TLS_GROUP_SECP256R1 as u16)
    if(ret < 0) { env.error("import pubkey failed"); return } else {}

    ret = x509_verify_cert_ecdsa_signature(&raw mut crt, &raw mut issuer)
    if(ret < 0) { env.error("self-signature verification failed"); return } else {}
}

@test
public func INT_hmac_long_key_vs_py(env : &mut TestEnv) {
    var key : [72]u8; test_random_bytes(&raw mut key[0], 72)
    var data : [32]u8; test_random_bytes(&raw mut data[0], 32)

    var key_hex : [145]char; test_bytes_to_hex(&raw key[0], 72, &raw mut key_hex[0])
    var data_hex : [65]char; test_bytes_to_hex(&raw data[0], 32, &raw mut data_hex[0])

    var chem_mac : [32]u8
    hmac_sha256(&raw key[0], 72, &raw data[0], 32, &raw mut chem_mac[0])

    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import hmac,hashlib\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hmac.new(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(data_hex[si]!=0){script[sp]=data_hex[si] as u8; sp+=1; si+=1}
    l = "'),hashlib.sha256).hexdigest()\nprint('MAC='+h)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("hmac_longkey.py"))
    var py_mac : [32]u8
    var mac_len = test_parse_py_hex_label(&raw mut py_out, string_view("MAC="), &raw mut py_mac[0], 32)
    if(mac_len != 32) { env.error("failed to parse Python output"); return } else {}

    if(!test_bytes_eq(&raw chem_mac[0], &raw py_mac[0], 32)) { env.error("HMAC long key mismatch vs Python"); return } else {}
}

@test
public func INT_ecdh_p256_shared_vs_py(env : &mut TestEnv) {
    var chem_ctx : ECDHContext; ecdh_init(&raw mut chem_ctx)
    var chem_priv : [32]u8; test_random_bytes(&raw mut chem_priv[0], 32)
    var chem_pub : [65]u8
    var ret = ecdh_generate_keypair(&raw mut chem_ctx, &raw mut chem_priv[0], 32, &raw mut chem_pub[0], 65)
    if(ret < 0) { env.error("ecdh_generate_keypair failed"); return } else {}

    var pub_hex : [131]char; test_bytes_to_hex(&raw chem_pub[0], 65, &raw mut pub_hex[0])

    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "from cryptography.hazmat.primitives.serialization import Encoding,PublicFormat\npy_key=ec.generate_private_key(ec.SECP256R1())\npy_pub=py_key.public_key()\npy_pub_enc=py_pub.public_bytes(Encoding.X962,PublicFormat.UncompressedPoint)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "chem_pub=ec.EllipticCurvePublicKey.from_encoded_point(ec.SECP256R1(),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pub_hex[si]!=0){script[sp]=pub_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nshared=py_key.exchange(ec.ECDH(),chem_pub)\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('PY_PUB='+py_pub_enc.hex())\nprint('SHARED='+shared.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdh_p256.py"))
    var py_pub : [65]u8; var py_shared : [32]u8
    var pub_len = test_parse_py_hex_label(&raw mut py_out, string_view("PY_PUB="), &raw mut py_pub[0], 65)
    var shared_len = test_parse_py_hex_label(&raw mut py_out, string_view("SHARED="), &raw mut py_shared[0], 32)
    if(pub_len != 65 || shared_len != 32) { env.error("failed to parse Python output"); return } else {}

    var chem_shared : [32]u8
    ret = ecdh_compute_shared(&raw mut chem_ctx, &raw py_pub[0], 65, &raw mut chem_shared[0], 32)
    if(ret < 0) { env.error("ecdh_compute_shared failed"); return } else {}

    if(!test_bytes_eq(&raw chem_shared[0], &raw py_shared[0], 32)) { env.error("ECDH P-256 shared secret mismatch vs Python"); return } else {}
}

@test
public func INT_tls13_max_record_roundtrip(env : &mut TestEnv) {
    var pt_len : size_t = 16384
    var pt_data : [16384]u8; test_random_bytes(&raw mut pt_data[0], pt_len)

    var ssl : SSLContext; ssl_init(&raw mut ssl)
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ssl, &raw mut cfg)

    var ss : [32]u8; test_random_bytes(&raw mut ss[0], 32)
    var hh : [32]u8; test_random_bytes(&raw mut hh[0], 32)
    tls13_derive_handshake_keys(&raw mut ssl, &raw ss[0], 32, &raw hh[0])
    tls13_derive_application_keys(&raw mut ssl, &raw hh[0], 32)

    var ct_buf : [17408]u8
    var ct_len = tls13_encrypt_record(&raw mut ssl, 23 as u8, &raw pt_data[0], pt_len, &raw mut ct_buf[0], 17408)
    if(ct_len < 0) { env.error("encrypt max-size record failed"); return } else {}

    ssl.in_hdr[0] = ct_buf[0]
    ssl.in_hdr[1] = ct_buf[1]
    ssl.in_hdr[2] = ct_buf[2]
    ssl.in_hdr[3] = ct_buf[3]
    ssl.in_hdr[4] = ct_buf[4]

    var ct_payload_len = (ct_len as size_t) - 5
    var inner_ct : u8 = 0
    var dec_buf : [17408]u8
    var dec_len = tls13_decrypt_record(&raw mut ssl, &raw ct_buf[5], ct_payload_len, &raw mut dec_buf[0], 17408, &raw mut inner_ct)
    if(dec_len < 0) { env.error("decrypt max-size record failed"); return } else {}
    if(inner_ct != 23) { env.error("inner content type mismatch"); return } else {}
    if(dec_len as size_t != pt_len) { env.error("decrypted length mismatch"); return } else {}
    if(!test_bytes_eq(&raw dec_buf[0], &raw pt_data[0], pt_len)) { env.error("decrypted data mismatch"); return } else {}
}
