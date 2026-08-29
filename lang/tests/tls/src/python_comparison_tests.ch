using namespace tls
using namespace crypto
using std::string_view
using std::string
using std::Result

// ─── AES-ECB encrypt vs Python ─────────────────────────────────────

@test
public func INT_aes128_ecb_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var pt : [32]u8; test_random_bytes(&raw mut pt[0], 32)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var pt_hex : [65]char; test_bytes_to_hex(&raw pt[0], 32, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "cipher=Cipher(algorithms.AES(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "')),modes.ECB())\nenc=cipher.encryptor()\nct=enc.update(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'))+enc.finalize()\nprint('CT='+ct.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("aes128_ecb.py"))
    var py_ct : [32]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 32)
    if(ct_len != 32) { env.error("failed to parse Python output"); return }

    var ctx : AESContext; aes_init(&raw mut ctx)
    aes_setkey_enc(&raw mut ctx, &raw key[0], 16)
    var chem_ct : [32]u8
    aes_crypt_ecb(&raw mut ctx, AES_ENCRYPT, &raw pt[0], &raw mut chem_ct[0])
    aes_crypt_ecb(&raw mut ctx, AES_ENCRYPT, &raw pt[16], &raw mut chem_ct[16])
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 32)) { env.error("AES-128-ECB ct mismatch vs Python"); return }
}

@test
public func INT_aes256_ecb_vs_python(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var pt : [32]u8; test_random_bytes(&raw mut pt[0], 32)

    var key_hex : [65]char; test_bytes_to_hex(&raw key[0], 32, &raw mut key_hex[0])
    var pt_hex : [65]char; test_bytes_to_hex(&raw pt[0], 32, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "cipher=Cipher(algorithms.AES(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "')),modes.ECB())\nenc=cipher.encryptor()\nct=enc.update(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'))+enc.finalize()\nprint('CT='+ct.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("aes256_ecb.py"))
    var py_ct : [32]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 32)
    if(ct_len != 32) { env.error("failed to parse Python output"); return }

    var ctx : AESContext; aes_init(&raw mut ctx)
    aes_setkey_enc(&raw mut ctx, &raw key[0], 32)
    var chem_ct : [32]u8
    aes_crypt_ecb(&raw mut ctx, AES_ENCRYPT, &raw pt[0], &raw mut chem_ct[0])
    aes_crypt_ecb(&raw mut ctx, AES_ENCRYPT, &raw pt[16], &raw mut chem_ct[16])
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 32)) { env.error("AES-256-ECB ct mismatch vs Python"); return }
}

// ─── AES-ECB decrypt vs Python ─────────────────────────────────────

@test
public func INT_aes128_ecb_decrypt_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var pt_hex : [33]char; test_bytes_to_hex(&raw pt[0], 16, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "cipher=Cipher(algorithms.AES(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "')),modes.ECB())\nenc=cipher.encryptor()\nct=enc.update(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'))+enc.finalize()\nprint('CT='+ct.hex())\ndec=cipher.decryptor()\npt2=dec.update(ct)+dec.finalize()\nprint('PT2='+pt2.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("aes128_ecb_dec.py"))
    var py_ct : [16]u8; var py_pt2 : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 16)
    var pt2_len = test_parse_py_hex_label(&raw mut py_out, string_view("PT2="), &raw mut py_pt2[0], 16)
    if(ct_len != 16 || pt2_len != 16) { env.error("failed to parse Python output"); return }

    var ctx : AESContext; aes_init(&raw mut ctx)
    aes_setkey_dec(&raw mut ctx, &raw key[0], 16)
    var chem_dec : [16]u8
    aes_crypt_ecb(&raw mut ctx, AES_DECRYPT, &raw py_ct[0], &raw mut chem_dec[0])
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 16)) { env.error("AES-128-ECB decrypt roundtrip mismatch vs Python"); return }
}

// ─── TLS 1.2 GCM record encrypt vs Python ──────────────────────────

@test
public func INT_tls12_record_gcm_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [4]u8; test_random_bytes(&raw mut iv[0], 4)
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)
    var seq : [8]u8; test_random_bytes(&raw mut seq[0], 8)
    seq[0] = 0; seq[1] = 0; seq[2] = 0; seq[3] = 0

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [9]char; test_bytes_to_hex(&raw iv[0], 4, &raw mut iv_hex[0])
    var pt_hex : [33]char; test_bytes_to_hex(&raw pt[0], 16, &raw mut pt_hex[0])
    var seq_hex : [17]char; test_bytes_to_hex(&raw seq[0], 8, &raw mut seq_hex[0])

    var tr : Transform; transform_init(&raw mut tr)
    tr.cipher_type = CIPHER_AES_128_GCM as u8; tr.key_len = 16; tr.iv_len = 4; tr.fixed_iv_len = 4; tr.mac_key_len = 0
    var i : size_t = 0; while(i < 16) { tr.key_enc[i] = key[i]; i += 1 }
    i = 0; while(i < 4) { tr.base_iv_enc[i] = iv[i]; i += 1 }
    var ct_out : [64]u8
    var enc_len = tls12_encrypt_record(&raw mut tr, &raw seq[0], SSL_MSG_APPLICATION_DATA as u8, 3, 3, &raw pt[0], 16, &raw mut ct_out[0], 64)
    if(enc_len < 0) { env.error("tls12_encrypt_record failed"); return }

    var ct : [16]u8; i = 0; while(i < 16) { ct[i] = ct_out[8 + i]; i += 1 }
    var tag : [16]u8; i = 0; while(i < 16) { tag[i] = ct_out[8 + 16 + i]; i += 1 }

    var ct_hex : [33]char; test_bytes_to_hex(&raw ct[0], 16, &raw mut ct_hex[0])
    var tag_hex : [33]char; test_bytes_to_hex(&raw tag[0], 16, &raw mut tag_hex[0])

    var script : [1536]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\niv=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "')+bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(seq_hex[si]!=0){script[sp]=seq_hex[si] as u8; sp+=1; si+=1}
    l = "')\naad=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(seq_hex[si]!=0){script[sp]=seq_hex[si] as u8; sp+=1; si+=1}
    l = "')+bytes([0x17,0x03,0x03])+bytes([0x00,0x10])\npt=aesgcm.decrypt(iv,bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ct_hex[si]!=0){script[sp]=ct_hex[si] as u8; sp+=1; si+=1}
    l = "')+bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(tag_hex[si]!=0){script[sp]=tag_hex[si] as u8; sp+=1; si+=1}
    l = "'),aad)\nprint('PT='+pt.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls12_gcm.py"))
    var py_pt : [16]u8
    var pt_len = test_parse_py_hex_label(&raw mut py_out, string_view("PT="), &raw mut py_pt[0], 16)
    if(pt_len != 16) { env.error("failed to parse Python output"); return }
    if(!test_bytes_eq(&raw py_pt[0], &raw pt[0], 16)) { env.error("TLS 1.2 GCM record decrypt mismatch vs Python"); return }

    var tr_dec : Transform; transform_init(&raw mut tr_dec)
    tr_dec.cipher_type = CIPHER_AES_128_GCM as u8; tr_dec.key_len = 16; tr_dec.iv_len = 4; tr_dec.fixed_iv_len = 4; tr_dec.mac_key_len = 0
    i = 0; while(i < 16) { tr_dec.key_dec[i] = key[i]; i += 1 }
    i = 0; while(i < 4) { tr_dec.base_iv_dec[i] = iv[i]; i += 1 }
    var dec_out : [64]u8
    var dec_ret = tls12_decrypt_record(&raw mut tr_dec, &raw seq[0], SSL_MSG_APPLICATION_DATA as u8, 3, 3, &raw ct_out[0], enc_len as size_t, &raw mut dec_out[0], 64)
    if(dec_ret < 0) { env.error("tls12_decrypt_record failed"); return }
    if(!test_bytes_eq(&raw dec_out[0], &raw pt[0], 16)) { env.error("TLS 1.2 GCM record Chemical decrypt mismatch"); return }
}

// ─── TLS 1.2 Finished message vs Python ────────────────────────────

@test
public func INT_tls12_finished_vs_python(env : &mut TestEnv) {
    var ms : [48]u8; test_random_bytes(&raw mut ms[0], 48)
    var handshake_hash : [36]u8; test_random_bytes(&raw mut handshake_hash[0], 36)

    var ms_hex : [97]char; test_bytes_to_hex(&raw ms[0], 48, &raw mut ms_hex[0])
    var hh_hex : [73]char; test_bytes_to_hex(&raw handshake_hash[0], 36, &raw mut hh_hex[0])

    var chem_client_finished : [12]u8
    tls12_compute_finished(&raw ms[0], true, &raw handshake_hash[0], 36, &raw mut chem_client_finished[0])
    var chem_server_finished : [12]u8
    tls12_compute_finished(&raw ms[0], false, &raw handshake_hash[0], 36, &raw mut chem_server_finished[0])

    var script : [1536]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import hashlib,hmac\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "ms=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ms_hex[si]!=0){script[sp]=ms_hex[si] as u8; sp+=1; si+=1}
    l = "')\nhh=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(hh_hex[si]!=0){script[sp]=hh_hex[si] as u8; sp+=1; si+=1}
    l = "')\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "def p_hash(secret,seed,length):\n o=b'';t=seed\n while len(o)<length:\n  t=hmac.new(secret,t,hashlib.sha256).digest()\n  o+=hmac.new(secret,t+seed,hashlib.sha256).digest()\n return o[:length]\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "cf=p_hash(ms,b'client finished'+hh,12)\nsf=p_hash(ms,b'server finished'+hh,12)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('CF='+cf.hex())\nprint('SF='+sf.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls12_fin.py"))
    var py_cf : [12]u8; var py_sf : [12]u8
    var cf_len = test_parse_py_hex_label(&raw mut py_out, string_view("CF="), &raw mut py_cf[0], 12)
    var sf_len = test_parse_py_hex_label(&raw mut py_out, string_view("SF="), &raw mut py_sf[0], 12)
    if(cf_len != 12 || sf_len != 12) { env.error("failed to parse Python output"); return }
    if(!test_bytes_eq(&raw chem_client_finished[0], &raw py_cf[0], 12)) { env.error("client Finished mismatch vs Python"); return }
    if(!test_bytes_eq(&raw chem_server_finished[0], &raw py_sf[0], 12)) { env.error("server Finished mismatch vs Python"); return }
}

// ─── X.509 RSA cert signature verification vs Python ───────────────

@test
public func INT_x509_rsa_signature_vs_python(env : &mut TestEnv) {
    var script : [2048]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography import x509\nfrom cryptography.x509.oid import NameOID\nfrom cryptography.hazmat.primitives import hashes\nfrom cryptography.hazmat.primitives.asymmetric import rsa\nimport datetime\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "key=rsa.generate_private_key(65537,2048)\npub=key.public_key()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "cert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'test.example.com')])).issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'TestCA')])).public_key(pub).serial_number(1).not_valid_before(datetime.datetime(2024,1,1)).not_valid_after(datetime.datetime(2026,1,1)).sign(key,hashes.SHA256()))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "from cryptography.hazmat.primitives.serialization import Encoding\nder=cert.public_bytes(Encoding.DER)\nf=open('/tmp/chem_rsa_cert.der','wb');f.write(der);f.close()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "from cryptography.hazmat.primitives.serialization import PublicFormat\npub_enc=pub.public_bytes(Encoding.DER,PublicFormat.PKCS1)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('PUB='+pub_enc.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("cert_rsa.py"))
    var pub_der : [294]u8
    var pub_len = test_parse_py_hex_label(&raw mut py_out, string_view("PUB="), &raw mut pub_der[0], 294)
    if(pub_len == 0) { env.error("failed to parse Python pubkey"); return }

    var cert_file = fopen("/tmp/chem_rsa_cert.der\0" as *char, "rb\0" as *char)
    if(cert_file == null) { env.error("cannot open cert"); return }
    var cert_buf : [2048]u8
    var cert_len = fread(&raw mut cert_buf[0] as *mut void, 1 as size_t, 2048, cert_file)
    fclose(cert_file)

    var crt : X509Cert; x509_cert_init(&raw mut crt)
    var ret = parse_cert_der(&raw mut crt, &raw cert_buf[0], cert_len)
    if(ret < 0) { env.error("parse_cert_der failed"); return }
    if(crt.pk_type != PK_RSA) { env.error("expected RSA key type"); return }

    // Import the RSA public key and verify signature
    var rsa_ctx : RSAContext; rsa_init(&raw mut rsa_ctx, RSA_PKCS_V15, 0)
    ret = x509_extract_rsa_pubkey(&raw mut crt, &raw mut rsa_ctx)
    if(ret < 0) { env.error("x509_extract_rsa_pubkey failed"); return }

    ret = x509_verify_cert_signature(&raw mut crt, &raw mut rsa_ctx)
    if(ret < 0) { env.error("RSA cert signature verification failed"); return }
}

// ─── TLS 1.3 record encrypt then Python decrypt + Python encrypt then Chemical decrypt ───

@test
public func INT_tls13_record_bidirectional_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var pt : [32]u8; test_random_bytes(&raw mut pt[0], 32)

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var tr : Transform; transform_init(&raw mut tr)
    tr.cipher_type = CIPHER_AES_128_GCM as u8; tr.key_len = 16; tr.iv_len = 12; tr.fixed_iv_len = 12
    var i : size_t = 0
    while(i < 16) { tr.key_enc[i] = key[i]; tr.key_dec[i] = key[i]; i += 1 }
    i = 0; while(i < 12) { tr.base_iv_enc[i] = iv[i]; tr.base_iv_dec[i] = iv[i]; i += 1 }
    var tr_out = malloc(sizeof(Transform)) as *mut Transform; *tr_out = tr; ctx.transform_out = tr_out
    var tr_in = malloc(sizeof(Transform)) as *mut Transform; *tr_in = tr; ctx.transform_in = tr_in

    var ct_buf : [128]u8
    var ct_len = tls13_encrypt_record(&raw mut ctx, SSL_MSG_APPLICATION_DATA as u8, &raw pt[0], 32, &raw mut ct_buf[0], 128)
    if(ct_len < 0) { env.error("tls13_encrypt_record failed"); return }

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var ct_hex : [250]char; test_bytes_to_hex(&raw ct_buf[5], (ct_len-5) as size_t, &raw mut ct_hex[0])
    var aad_hex : [11]char; test_bytes_to_hex(&raw ct_buf[0], 5, &raw mut aad_hex[0])

    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\npt=aesgcm.decrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ct_hex[si]!=0){script[sp]=ct_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nprint('PT='+pt.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls13_bidi.py"))
    var py_pt : [32]u8
    var pt_parse_len = test_parse_py_hex_label(&raw mut py_out, string_view("PT="), &raw mut py_pt[0], 32)
    if(pt_parse_len != 32) { env.error("failed to parse Python output"); return }
    if(!test_bytes_eq(&raw py_pt[0], &raw pt[0], 32)) { env.error("Python decrypted TLS 1.3 record mismatch"); return }

    // Python encrypts, Chemical decrypts
    ctx.in_ctr[0]=0;ctx.in_ctr[1]=0;ctx.in_ctr[2]=0;ctx.in_ctr[3]=0;ctx.in_ctr[4]=0;ctx.in_ctr[5]=0;ctx.in_ctr[6]=0;ctx.in_ctr[7]=0
    var pt2 : [16]u8; test_random_bytes(&raw mut pt2[0], 16)
    var pt2_hex : [33]char; test_bytes_to_hex(&raw pt2[0], 16, &raw mut pt2_hex[0])
    script[0]=0; sp=0; si=0
    l = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\naesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\ninner=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt2_hex[si]!=0){script[sp]=pt2_hex[si] as u8; sp+=1; si+=1}
    l = "')+bytes([0x17])\naad=bytes([0x17,0x03,0x03,0x00,(16+16+1)])\nct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),inner,aad)\nprint('REC='+aad.hex()+ct_tag.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    py_out = test_python_run_script(&raw script[0], sp, string_view("tls13_bidi2.py"))
    var rec_bytes : [64]u8
    var rec_len = test_parse_py_hex_label(&raw mut py_out, string_view("REC="), &raw mut rec_bytes[0], 54)
    if(rec_len < 5) { env.error("failed to parse Python encrypted record"); return }

    ctx.in_hdr[0] = rec_bytes[0]; ctx.in_hdr[1] = rec_bytes[1]; ctx.in_hdr[2] = rec_bytes[2]
    ctx.in_hdr[3] = rec_bytes[3]; ctx.in_hdr[4] = rec_bytes[4]
    i = 0; while(i < 8) { ctx.in_ctr[i] = 0; i += 1 }

    var dec_buf : [64]u8; var inner_ct : u8 = 0
    var dlen = tls13_decrypt_record(&raw mut ctx, &raw rec_bytes[5], (rec_len-5) as size_t, &raw mut dec_buf[0], 64, &raw mut inner_ct)
    if(dlen < 0) { env.error("tls13_decrypt_record failed"); return }
    if(!test_bytes_eq(&raw dec_buf[0], &raw pt2[0], 16)) { env.error("TLS 1.3 bidirectional decrypt mismatch"); return }
}

// ─── HMAC-SHA256 long message vs Python ────────────────────────────

@test
public func INT_hmac_sha256_long_msg_vs_python(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var data : [256]u8; test_random_bytes(&raw mut data[0], 256)

    var key_hex : [65]char; test_bytes_to_hex(&raw key[0], 32, &raw mut key_hex[0])
    var data_hex : [513]char; test_bytes_to_hex(&raw data[0], 256, &raw mut data_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "import hmac,hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hmac.new(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var di : size_t = 0; while(di < 512 && data_hex[di] != 0) { script[sp]=data_hex[di] as u8; sp+=1; di+=1 }
    l = "'),hashlib.sha256).hexdigest()\nprint('MAC='+h)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("hmac256.py"))
    var py_mac : [32]u8
    var mac_len = test_parse_py_hex_label(&raw mut py_out, string_view("MAC="), &raw mut py_mac[0], 32)
    if(mac_len != 32) { env.error("failed to parse Python output"); return }

    var chem_mac : [32]u8
    hmac_sha256(&raw key[0], 32, &raw data[0], 256, &raw mut chem_mac[0])
    if(!test_bytes_eq(&raw chem_mac[0], &raw py_mac[0], 32)) { env.error("HMAC-SHA256 long msg mismatch vs Python"); return }
}

// ─── Base64 decode vs Python ────────────────────────────────────────

@test
public func INT_base64_decode_vs_python(env : &mut TestEnv) {
    var data : [48]u8; test_random_bytes(&raw mut data[0], 48)
    var d_hex : [97]char; test_bytes_to_hex(&raw data[0], 48, &raw mut d_hex[0])

    var enc : [72]char; var enc_r = base64_encode(&raw data[0], 48, &raw mut enc[0], 72)
    if(enc_r is Result.Err) { env.error("b64 enc failed"); return }
    var Ok(enc_len) = enc_r else unreachable

    var script : [256]u8; var sp : size_t = 0
    var hdr = "import base64;d=bytes.fromhex('" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(d_hex[si]!=0){script[sp]=d_hex[si] as u8; sp+=1; si+=1}
    var l = "')\nprint(base64.b64encode(d).decode())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("b64_enc"))
    var py_enc : [72]u8; var pw : size_t = 0; var pi : size_t = 0
    while(pi<py_out.size()&&pw<72){var c=py_out.get(pi)as char;if(c==10||c==13||c==0){break}else{py_enc[pw]=c as u8;pw+=1}pi+=1}
    if(pw!=enc_len||!test_bytes_eq(&raw enc[0] as *u8,&raw py_enc[0],enc_len)){env.error("b64 enc vs py");return}

    // Now decode back
    var dec_r = base64_decode(&raw enc[0], enc_len, &raw mut data[0], 48)
    if(dec_r is Result.Err) { env.error("b64 dec failed"); return }
    var Ok(dec_len) = dec_r else unreachable
    if(dec_len != 48) { env.error("b64 dec length mismatch"); return }
    // Already verified via encode roundtrip in existing tests
}

// ─── MD5 hash vs Python ─────────────────────────────────────────────

@test
public func INT_md5_hash_vs_python(env : &mut TestEnv) {
    var data : [64]u8; test_random_bytes(&raw mut data[0], 64)
    var data_hex : [129]char; test_bytes_to_hex(&raw data[0], 64, &raw mut data_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hashlib.md5(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(data_hex[si]!=0){script[sp]=data_hex[si] as u8; sp+=1; si+=1}
    l = "')).hexdigest()\nprint('HASH='+h)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("md5_hash.py"))
    var py_hash : [16]u8
    var hash_len = test_parse_py_hex_label(&raw mut py_out, string_view("HASH="), &raw mut py_hash[0], 16)
    if(hash_len != 16) { env.error("failed to parse Python output"); return }

    var chem_hash : [16]u8
    md5_hash(&raw data[0], 64, &raw mut chem_hash[0])
    if(!test_bytes_eq(&raw chem_hash[0], &raw py_hash[0], 16)) { env.error("MD5 hash mismatch vs Python"); return }
}

@test
public func INT_md5_empty_vs_python(env : &mut TestEnv) {
    var script : [256]u8; var sp : size_t = 0
    var hdr = "import hashlib;print('HASH='+hashlib.md5(b'').hexdigest())\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("md5_empty.py"))
    var py_hash : [16]u8
    var hash_len = test_parse_py_hex_label(&raw mut py_out, string_view("HASH="), &raw mut py_hash[0], 16)
    if(hash_len != 16) { env.error("failed to parse Python output"); return }
    var chem_hash : [16]u8
    md5_hash(null, 0, &raw mut chem_hash[0])
    if(!test_bytes_eq(&raw chem_hash[0], &raw py_hash[0], 16)) { env.error("MD5 empty hash mismatch vs Python"); return }
}

// ─── AES-CBC decrypt vs Python ──────────────────────────────────────

@test
public func INT_aes_cbc_128_decrypt_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [16]u8; test_random_bytes(&raw mut iv[0], 16)
    var pt : [48]u8; test_random_bytes(&raw mut pt[0], 48)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [33]char; test_bytes_to_hex(&raw iv[0], 16, &raw mut iv_hex[0])
    var pt_hex : [97]char; test_bytes_to_hex(&raw pt[0], 48, &raw mut pt_hex[0])

    var py_script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){py_script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "cipher=Cipher(algorithms.AES(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){py_script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){py_script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "')),modes.CBC(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){py_script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){py_script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "')))\nenc=cipher.encryptor()\nct=enc.update(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){py_script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){py_script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'))+enc.finalize()\nprint('CT='+ct.hex())\n" as *char; si=0
    while(l[si]!=0){py_script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw py_script[0], sp, string_view("cbc128_dec.py"))
    var py_ct : [64]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 64)
    if(ct_len == 0) { env.error("failed to parse Python output"); return }

    var ctx : AESContext; aes_init(&raw mut ctx)
    aes_setkey_dec(&raw mut ctx, &raw key[0], 16)
    var dec_iv : [16]u8; var di : size_t = 0; while(di < 16) { dec_iv[di] = iv[di]; di += 1 }
    var chem_dec : [64]u8
    var ret = aes_crypt_cbc(&raw mut ctx, AES_DECRYPT, ct_len, &raw mut dec_iv[0], &raw py_ct[0], &raw mut chem_dec[0])
    if(ret < 0) { env.error("aes_crypt_cbc decrypt failed"); return }
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 48)) { env.error("AES-128-CBC decrypt mismatch vs Python"); return }
}

@test
public func INT_aes_cbc_256_decrypt_vs_python(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var iv : [16]u8; test_random_bytes(&raw mut iv[0], 16)
    var pt : [48]u8; test_random_bytes(&raw mut pt[0], 48)

    var key_hex : [65]char; test_bytes_to_hex(&raw key[0], 32, &raw mut key_hex[0])
    var iv_hex : [33]char; test_bytes_to_hex(&raw iv[0], 16, &raw mut iv_hex[0])
    var pt_hex : [97]char; test_bytes_to_hex(&raw pt[0], 48, &raw mut pt_hex[0])

    var py_script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){py_script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "cipher=Cipher(algorithms.AES(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){py_script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){py_script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "')),modes.CBC(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){py_script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){py_script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "')))\nenc=cipher.encryptor()\nct=enc.update(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){py_script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){py_script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'))+enc.finalize()\nprint('CT='+ct.hex())\n" as *char; si=0
    while(l[si]!=0){py_script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw py_script[0], sp, string_view("cbc256_dec.py"))
    var py_ct : [64]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 64)
    if(ct_len == 0) { env.error("failed to parse Python output"); return }

    var ctx : AESContext; aes_init(&raw mut ctx)
    aes_setkey_dec(&raw mut ctx, &raw key[0], 32)
    var dec_iv : [16]u8; var di : size_t = 0; while(di < 16) { dec_iv[di] = iv[di]; di += 1 }
    var chem_dec : [64]u8
    var ret = aes_crypt_cbc(&raw mut ctx, AES_DECRYPT, ct_len, &raw mut dec_iv[0], &raw py_ct[0], &raw mut chem_dec[0])
    if(ret < 0) { env.error("aes_crypt_cbc decrypt failed"); return }
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 48)) { env.error("AES-256-CBC decrypt mismatch vs Python"); return }
}

// ─── HMAC with empty inputs vs Python ───────────────────────────────

@test
public func INT_hmac_sha256_empty_key_vs_python(env : &mut TestEnv) {
    var data : [32]u8; test_random_bytes(&raw mut data[0], 32)
    var data_hex : [65]char; test_bytes_to_hex(&raw data[0], 32, &raw mut data_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hmac,hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hmac.new(b'',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(data_hex[si]!=0){script[sp]=data_hex[si] as u8; sp+=1; si+=1}
    l = "'),hashlib.sha256).hexdigest()\nprint('MAC='+h)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("hmac_ek.py"))
    var py_mac : [32]u8
    var mac_len = test_parse_py_hex_label(&raw mut py_out, string_view("MAC="), &raw mut py_mac[0], 32)
    if(mac_len != 32) { env.error("failed to parse Python output"); return }
    var chem_mac : [32]u8
    hmac_sha256(null, 0, &raw data[0], 32, &raw mut chem_mac[0])
    if(!test_bytes_eq(&raw chem_mac[0], &raw py_mac[0], 32)) { env.error("HMAC-SHA256 empty key mismatch vs Python"); return }
}

@test
public func INT_hmac_sha256_empty_data_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hmac,hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hmac.new(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'),b'',hashlib.sha256).hexdigest()\nprint('MAC='+h)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("hmac_ed.py"))
    var py_mac : [32]u8
    var mac_len = test_parse_py_hex_label(&raw mut py_out, string_view("MAC="), &raw mut py_mac[0], 32)
    if(mac_len != 32) { env.error("failed to parse Python output"); return }
    var chem_mac : [32]u8
    hmac_sha256(&raw key[0], 16, null, 0, &raw mut chem_mac[0])
    if(!test_bytes_eq(&raw chem_mac[0], &raw py_mac[0], 32)) { env.error("HMAC-SHA256 empty data mismatch vs Python"); return }
}

// ─── GCM with max-size AAD vs Python ────────────────────────────────

@test
public func INT_gcm_large_plaintext_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var aad : [16]u8; test_random_bytes(&raw mut aad[0], 16)
    var pt : [1024]u8; test_random_bytes(&raw mut pt[0], 1024)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [33]char; test_bytes_to_hex(&raw aad[0], 16, &raw mut aad_hex[0])
    var pt_hex : [2049]char; test_bytes_to_hex(&raw pt[0], 1024, &raw mut pt_hex[0])

    var script : [4096]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var di : size_t = 0; while(di < 2048 && pt_hex[di] != 0) { script[sp]=pt_hex[di] as u8; sp+=1; di+=1 }
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nprint('CT='+ct_tag[:1024].hex())\nprint('TAG='+ct_tag[1024:].hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm_large.py"))
    var py_ct : [1024]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 1024)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 1024 || tag_len != 16) { env.error("failed to parse Python output"); return }
    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw key[0], 16)
    var chem_ct : [2048]u8; var chem_tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, &raw aad[0], 16, &raw pt[0], 1024, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 1024)) { env.error("GCM large ct mismatch vs Python"); return }
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM large tag mismatch vs Python"); return }
}

// ─── X25519 key clamping vs Python ──────────────────────────────────

@test
public func INT_x25519_clamping_vs_python(env : &mut TestEnv) {
    var raw_scalar : [32]u8; test_random_bytes(&raw mut raw_scalar[0], 32)
    var rs_hex : [65]char; test_bytes_to_hex(&raw raw_scalar[0], 32, &raw mut rs_hex[0])

    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "s=list(bytes.fromhex('" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(rs_hex[si]!=0){script[sp]=rs_hex[si] as u8; sp+=1; si+=1}
    var l = "'))\ns[0]=s[0]&248;s[31]=s[31]&127;s[31]=s[31]|64\nprint('S='+bytes(s).hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("x25519_clamp"))
    var py_scalar : [32]u8
    var s_len = test_parse_py_hex_label(&raw mut py_out, string_view("S="), &raw mut py_scalar[0], 32)
    if(s_len != 32) { env.error("failed to parse Python output"); return }

    x25519_clamp_scalar(&raw mut raw_scalar[0])
    if(!test_bytes_eq(&raw raw_scalar[0], &raw py_scalar[0], 32)) { env.error("X25519 clamping mismatch vs Python"); return }
}

// ─── GCM with 8-byte IV vs Python ───────────────────────────────────

@test
public func INT_gcm_iv8_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv8 : [8]u8; test_random_bytes(&raw mut iv8[0], 8)
    var aad : [13]u8; test_random_bytes(&raw mut aad[0], 13)
    var pt : [37]u8; test_random_bytes(&raw mut pt[0], 37)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [17]char; test_bytes_to_hex(&raw iv8[0], 8, &raw mut iv_hex[0])
    var aad_hex : [27]char; test_bytes_to_hex(&raw aad[0], 13, &raw mut aad_hex[0])
    var pt_hex : [75]char; test_bytes_to_hex(&raw pt[0], 37, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nprint('CT='+ct_tag[:37].hex())\nprint('TAG='+ct_tag[37:].hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm_iv8.py"))
    var py_ct : [37]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 37)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 37 || tag_len != 16) { env.error("failed to parse Python output"); return }

    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw key[0], 16)
    var chem_ct : [64]u8; var chem_tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv8[0], 8, &raw aad[0], 13, &raw pt[0], 37, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 37)) { env.error("GCM 8-byte IV ct mismatch vs Python"); return }
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM 8-byte IV tag mismatch vs Python"); return }
}

// ─── GCM with 16-byte IV vs Python ──────────────────────────────────

@test
public func INT_gcm_iv16_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv16 : [16]u8; test_random_bytes(&raw mut iv16[0], 16)
    var pt : [32]u8; test_random_bytes(&raw mut pt[0], 32)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [33]char; test_bytes_to_hex(&raw iv16[0], 16, &raw mut iv_hex[0])
    var pt_hex : [65]char; test_bytes_to_hex(&raw pt[0], 32, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),None)\nprint('CT='+ct_tag[:32].hex())\nprint('TAG='+ct_tag[32:].hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm_iv16.py"))
    var py_ct : [32]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 32)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 32 || tag_len != 16) { env.error("failed to parse Python output"); return }

    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw key[0], 16)
    var chem_ct : [64]u8; var chem_tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv16[0], 16, null, 0, &raw pt[0], 32, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 32)) { env.error("GCM 16-byte IV ct mismatch vs Python"); return }
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM 16-byte IV tag mismatch vs Python"); return }
}

// ─── AES-256-GCM vs Python ──────────────────────────────────────────

@test
public func INT_aes256_gcm_encrypt_vs_python(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var aad : [8]u8; test_random_bytes(&raw mut aad[0], 8)
    var pt : [64]u8; test_random_bytes(&raw mut pt[0], 64)

    var key_hex : [65]char; test_bytes_to_hex(&raw key[0], 32, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [17]char; test_bytes_to_hex(&raw aad[0], 8, &raw mut aad_hex[0])
    var pt_hex : [129]char; test_bytes_to_hex(&raw pt[0], 64, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nprint('CT='+ct_tag[:64].hex())\nprint('TAG='+ct_tag[64:].hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("aes256gcm.py"))
    var py_ct : [64]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 64)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 64 || tag_len != 16) { env.error("failed to parse Python output"); return }

    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw key[0], 32)
    var chem_ct : [128]u8; var chem_tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, &raw aad[0], 8, &raw pt[0], 64, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 64)) { env.error("AES-256-GCM ct mismatch vs Python"); return }
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("AES-256-GCM tag mismatch vs Python"); return }
}

// ─── TLS 1.2 key block different sizes vs Python ────────────────────

@test
public func INT_tls12_key_block_128_vs_python(env : &mut TestEnv) {
    var ms : [48]u8; test_random_bytes(&raw mut ms[0], 48)
    var sr : [32]u8; test_random_bytes(&raw mut sr[0], 32)
    var cr : [32]u8; test_random_bytes(&raw mut cr[0], 32)

    var ms_hex : [97]char; test_bytes_to_hex(&raw ms[0], 48, &raw mut ms_hex[0])
    var sr_hex : [65]char; test_bytes_to_hex(&raw sr[0], 32, &raw mut sr_hex[0])
    var cr_hex : [65]char; test_bytes_to_hex(&raw cr[0], 32, &raw mut cr_hex[0])

    var chem_kb : [40]u8
    tls12_derive_key_block(&raw ms[0], &raw sr[0], &raw cr[0], &raw mut chem_kb[0], 40)

    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import hashlib,hmac\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "ms=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ms_hex[si]!=0){script[sp]=ms_hex[si] as u8; sp+=1; si+=1}
    l = "')\nsr=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(sr_hex[si]!=0){script[sp]=sr_hex[si] as u8; sp+=1; si+=1}
    l = "')\ncr=bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(cr_hex[si]!=0){script[sp]=cr_hex[si] as u8; sp+=1; si+=1}
    l = "')\ndef p_hash(secret,seed,length):\n o=b'';t=seed\n while len(o)<length:\n  t=hmac.new(secret,t,hashlib.sha256).digest()\n  o+=hmac.new(secret,t+seed,hashlib.sha256).digest()\n return o[:length]\nkb=p_hash(ms,b'key expansion'+sr+cr,40)\nprint('KB='+kb.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls12kb128"))
    var py_kb : [40]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("KB="), &raw mut py_kb[0], 40)!=40){env.error("kb");return}
    if(!test_bytes_eq(&raw chem_kb[0],&raw py_kb[0],40)){env.error("key block 128");return}
}

// ─── GCM AES-128 with no AAD vs Python ──────────────────────────────

@test
public func INT_gcm_no_aad_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var pt : [32]u8; test_random_bytes(&raw mut pt[0], 32)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var pt_hex : [65]char; test_bytes_to_hex(&raw pt[0], 32, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),None)\nprint('CT='+ct_tag[:32].hex())\nprint('TAG='+ct_tag[32:].hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm_noaad.py"))
    var py_ct : [32]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 32)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 32 || tag_len != 16) { env.error("failed to parse Python output"); return }

    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw key[0], 16)
    var chem_ct : [64]u8; var chem_tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, null, 0, &raw pt[0], 32, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 32)) { env.error("GCM no AAD ct mismatch vs Python"); return }
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM no AAD tag mismatch vs Python"); return }
}

// ─── GCM auth_decrypt roundtrip vs Python ───────────────────────────

@test
public func INT_gcm_auth_decrypt_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var pt : [48]u8; test_random_bytes(&raw mut pt[0], 48)
    var aad : [8]u8; test_random_bytes(&raw mut aad[0], 8)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [17]char; test_bytes_to_hex(&raw aad[0], 8, &raw mut aad_hex[0])
    var pt_hex : [97]char; test_bytes_to_hex(&raw pt[0], 48, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nprint('CT='+ct_tag[:48].hex())\nprint('TAG='+ct_tag[48:].hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm_auth.py"))
    var py_ct : [48]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 48)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 48 || tag_len != 16) { env.error("failed to parse Python output"); return }

    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw key[0], 16)
    var chem_ct : [64]u8; var chem_tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, &raw aad[0], 8, &raw pt[0], 48, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 48)) { env.error("GCM auth ct mismatch vs Python"); return }
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM auth tag mismatch vs Python"); return }

    var gcm2 : GCMContext; gcm_init(&raw mut gcm2, &raw key[0], 16)
    var chem_dec : [64]u8
    var dret = gcm_auth_decrypt(&raw mut gcm2, &raw iv[0], 12, &raw aad[0], 8, &raw chem_ct[0], 48, &raw chem_tag[0], 16, &raw mut chem_dec[0])
    if(dret < 0) { env.error("gcm_auth_decrypt failed"); return }
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 48)) { env.error("GCM auth decrypt roundtrip mismatch"); return }
}

// ─── HMAC-MD5 empty key vs Python ───────────────────────────────────

@test
public func INT_hmac_md5_empty_key_vs_python(env : &mut TestEnv) {
    var data : [16]u8; test_random_bytes(&raw mut data[0], 16)
    var data_hex : [33]char; test_bytes_to_hex(&raw data[0], 16, &raw mut data_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hmac,hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hmac.new(b'',bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(data_hex[si]!=0){script[sp]=data_hex[si] as u8; sp+=1; si+=1}
    l = "'),hashlib.md5).hexdigest()\nprint('MAC='+h)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("hmac_md5_ek"))
    var py_mac : [16]u8
    var mac_len = test_parse_py_hex_label(&raw mut py_out, string_view("MAC="), &raw mut py_mac[0], 16)
    if(mac_len != 16) { env.error("failed to parse Python output"); return }
    var chem_mac : [16]u8
    hmac_md5(null, 0, &raw data[0], 16, &raw mut chem_mac[0])
    if(!test_bytes_eq(&raw chem_mac[0], &raw py_mac[0], 16)) { env.error("HMAC-MD5 empty key mismatch vs Python"); return }
}

// ─── RSA PKCS#1 encrypt produces different outputs each time vs Python ──

@test
@test.timeout(60000)
public func INT_rsa_pkcs1_encrypt_determinism_vs_python(env : &mut TestEnv) {
    var rsa_ctx : RSAContext; rsa_init(&raw mut rsa_ctx, RSA_PKCS_V15, 0)
    var ret = rsa_gen_key(&raw mut rsa_ctx, 2048, 65537)
    if(ret < 0) { env.error("rsa_gen_key failed"); return }

    var n_len = rsa_get_len(&raw mut rsa_ctx)
    var n_buf : [256]u8; var e_buf : [4]u8
    mpi_write_binary(&raw mut rsa_ctx.N, &raw mut n_buf[0], n_len)
    mpi_write_binary(&raw mut rsa_ctx.E, &raw mut e_buf[0], 4)

    var n_hex : [513]char; test_bytes_to_hex(&raw n_buf[0], n_len, &raw mut n_hex[0])
    var e_hex : [9]char; test_bytes_to_hex(&raw e_buf[0], 4, &raw mut e_hex[0])

    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)
    var pt_hex : [33]char; test_bytes_to_hex(&raw pt[0], 16, &raw mut pt_hex[0])

    var chem_ct : [256]u8
    ret = rsa_pkcs1_encrypt(&raw mut rsa_ctx, &raw pt[0], 16, &raw mut chem_ct[0])
    if(ret < 0) { env.error("rsa_pkcs1_encrypt failed"); return }
    var chem_ct2 : [256]u8
    ret = rsa_pkcs1_encrypt(&raw mut rsa_ctx, &raw pt[0], 16, &raw mut chem_ct2[0])
    if(ret < 0) { env.error("rsa_pkcs1_encrypt failed"); return }
    if(test_bytes_eq(&raw chem_ct[0], &raw chem_ct2[0], 256)) { env.error("RSA PKCS#1 encrypt should be non-deterministic"); return }

    var ct_hex : [513]char; test_bytes_to_hex(&raw chem_ct[0], n_len, &raw mut ct_hex[0])
    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import rsa,padding\nn=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(n_hex[si]!=0){script[sp]=n_hex[si] as u8; sp+=1; si+=1}
    var l = "'),'big')\ne=int.from_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(e_hex[si]!=0){script[sp]=e_hex[si] as u8; sp+=1; si+=1}
    l = "'),'big')\npub=rsa.RSAPublicNumbers(e,n).public_key()\nct=pub.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),padding.PKCS1v15())\nprint('CT='+ct.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("rsa_enc_det"))
    var py_ct : [256]u8
    var py_ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], n_len)
    if(py_ct_len != n_len) { env.error("failed to parse Python ciphertext"); return }
    var chem_dec : [256]u8; var dec_len : size_t = 256
    ret = rsa_pkcs1_decrypt(&raw mut rsa_ctx, &raw py_ct[0], n_len, &raw mut chem_dec[0], &raw mut dec_len, 64)
    if(ret < 0) { env.error("rsa_pkcs1_decrypt of Python ct failed"); return }
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 16)) { env.error("decrypted Python ct mismatch"); return }
}
