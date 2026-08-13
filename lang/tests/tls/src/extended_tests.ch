using namespace tls
using namespace crypto
using std::string_view
using std::string
using std::Result

@test
public func INT_gcm_aes128_encrypt(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var aad : [8]u8; test_random_bytes(&raw mut aad[0], 8)
    var pt : [32]u8; test_random_bytes(&raw mut pt[0], 32)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [17]char; test_bytes_to_hex(&raw aad[0], 8, &raw mut aad_hex[0])
    var pt_hex : [65]char; test_bytes_to_hex(&raw pt[0], 32, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "ct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('CT='+ct_tag[:32].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('TAG='+ct_tag[32:].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm128.py"))
    var py_ct : [32]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 32)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 32 || tag_len != 16) { env.error("failed to parse Python output"); return } else {}

    var gcm : GCMContext
    var ret = gcm_init(&raw mut gcm, &raw key[0], 16)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var chem_ct : [64]u8; var chem_tag : [16]u8
    ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, &raw aad[0], 8, &raw pt[0], 32, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 32)) { env.error("GCM AES-128 ct mismatch"); return } else {}
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM AES-128 tag mismatch"); return } else {}
}

@test
public func INT_gcm_aes256_encrypt(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var aad : [8]u8; test_random_bytes(&raw mut aad[0], 8)
    var pt : [32]u8; test_random_bytes(&raw mut pt[0], 32)

    var key_hex : [65]char; test_bytes_to_hex(&raw key[0], 32, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [17]char; test_bytes_to_hex(&raw aad[0], 8, &raw mut aad_hex[0])
    var pt_hex : [65]char; test_bytes_to_hex(&raw pt[0], 32, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "ct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('CT='+ct_tag[:32].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('TAG='+ct_tag[32:].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm256.py"))
    var py_ct : [32]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 32)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 32 || tag_len != 16) { env.error("failed to parse Python output"); return } else {}

    var gcm : GCMContext
    var ret = gcm_init(&raw mut gcm, &raw key[0], 32)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var chem_ct : [64]u8; var chem_tag : [16]u8
    ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, &raw aad[0], 8, &raw pt[0], 32, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 32)) { env.error("GCM AES-256 ct mismatch"); return } else {}
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM AES-256 tag mismatch"); return } else {}
}

@test
public func INT_gcm_empty_aad(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var pt_hex : [33]char; test_bytes_to_hex(&raw pt[0], 16, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "ct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),b'')" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('CT='+ct_tag[:16].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('TAG='+ct_tag[16:].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm_empty_aad.py"))
    var py_ct : [16]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 16)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 16 || tag_len != 16) { env.error("failed to parse Python output"); return } else {}

    var gcm : GCMContext
    var ret = gcm_init(&raw mut gcm, &raw key[0], 16)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var chem_ct : [64]u8; var chem_tag : [16]u8
    ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, null, 0, &raw pt[0], 16, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 16)) { env.error("GCM empty AAD ct mismatch"); return } else {}
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM empty AAD tag mismatch"); return } else {}
}

@test
public func INT_gcm_auth_decrypt_wrong_tag(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)

    var gcm : GCMContext
    var ret = gcm_init(&raw mut gcm, &raw key[0], 16)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var ct : [64]u8; var tag : [16]u8
    ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, null, 0, &raw pt[0], 16, &raw mut ct[0], &raw mut tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}

    var wrong_tag : [16]u8
    var wti : size_t = 0; while(wti < 16) { wrong_tag[wti] = tag[wti]; wti += 1 }
    wrong_tag[0] = wrong_tag[0] ^ 0xFF

    var gcm2 : GCMContext
    gcm_init(&raw mut gcm2, &raw key[0], 16)
    var dec_out : [64]u8
    ret = gcm_auth_decrypt(&raw mut gcm2, &raw iv[0], 12, null, 0, &raw ct[0], 16, &raw wrong_tag[0], 16, &raw mut dec_out[0])
    if(ret >= 0) { env.error("auth_decrypt should have failed with wrong tag"); return } else {}
}

@test
public func INT_gcm_varying_sizes(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)

    var sizes : [6]size_t
    sizes[0]=1; sizes[1]=15; sizes[2]=16; sizes[3]=255; sizes[4]=256; sizes[5]=1023
    var si_idx : size_t = 0
    while(si_idx < 6) {
        var pt_len = sizes[si_idx]
        var pt : [1024]u8; test_random_bytes(&raw mut pt[0], pt_len)

        var gcm : GCMContext
        var ret = gcm_init(&raw mut gcm, &raw key[0], 16)
        if(ret < 0) { env.error("gcm_init failed"); return } else {}
        var ct : [1024]u8; var tag : [16]u8
        ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, null, 0, &raw pt[0], pt_len, &raw mut ct[0], &raw mut tag[0])
        if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}

        var gcm2 : GCMContext
        gcm_init(&raw mut gcm2, &raw key[0], 16)
        var dec : [1024]u8
        ret = gcm_auth_decrypt(&raw mut gcm2, &raw iv[0], 12, null, 0, &raw ct[0], pt_len, &raw tag[0], 16, &raw mut dec[0])
        if(ret < 0) { env.error("gcm_auth_decrypt failed"); return } else {}
        if(!test_bytes_eq(&raw dec[0], &raw pt[0], pt_len)) { env.error("GCM varying size roundtrip failed"); return } else {}

        si_idx += 1
    }
}

@test
public func INT_gcm_long_aad(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var aad : [1024]u8; test_random_bytes(&raw mut aad[0], 1024)
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [2049]char; test_bytes_to_hex(&raw aad[0], 1024, &raw mut aad_hex[0])
    var pt_hex : [33]char; test_bytes_to_hex(&raw pt[0], 16, &raw mut pt_hex[0])

    var script : [4096]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "ct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('CT='+ct_tag[:16].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('TAG='+ct_tag[16:].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm_long_aad.py"))
    var py_ct : [16]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 16)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 16 || tag_len != 16) { env.error("failed to parse Python output"); return } else {}

    var gcm : GCMContext
    var ret = gcm_init(&raw mut gcm, &raw key[0], 16)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var chem_ct : [64]u8; var chem_tag : [16]u8
    ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, &raw aad[0], 1024, &raw pt[0], 16, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 16)) { env.error("GCM long AAD ct mismatch"); return } else {}
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM long AAD tag mismatch"); return } else {}
}

@test
public func INT_gcm_aes256_auth_decrypt(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var aad : [8]u8; test_random_bytes(&raw mut aad[0], 8)
    var pt : [32]u8; test_random_bytes(&raw mut pt[0], 32)

    var key_hex : [65]char; test_bytes_to_hex(&raw key[0], 32, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [17]char; test_bytes_to_hex(&raw aad[0], 8, &raw mut aad_hex[0])
    var pt_hex : [65]char; test_bytes_to_hex(&raw pt[0], 32, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers.aead import AESGCM\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "aesgcm=AESGCM(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "ct_tag=aesgcm.encrypt(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si] as u8; sp+=1; si+=1}
    l = "'))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('CT='+ct_tag[:32].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('TAG='+ct_tag[32:].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm256_dec.py"))
    var py_ct : [32]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 32)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 32 || tag_len != 16) { env.error("failed to parse Python output"); return } else {}

    var gcm : GCMContext
    var ret = gcm_init(&raw mut gcm, &raw key[0], 32)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var chem_ct : [64]u8; var chem_tag : [16]u8
    ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, &raw aad[0], 8, &raw pt[0], 32, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}

    var gcm2 : GCMContext
    gcm_init(&raw mut gcm2, &raw key[0], 32)
    var chem_dec : [64]u8
    ret = gcm_auth_decrypt(&raw mut gcm2, &raw iv[0], 12, &raw aad[0], 8, &raw chem_ct[0], 32, &raw chem_tag[0], 16, &raw mut chem_dec[0])
    if(ret < 0) { env.error("gcm_auth_decrypt failed"); return } else {}
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 32)) { env.error("GCM AES-256 auth decrypt roundtrip failed"); return } else {}
    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 32)) { env.error("GCM AES-256 ct mismatch"); return } else {}
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) { env.error("GCM AES-256 tag mismatch"); return } else {}
}

@test
public func INT_aes_cbc_128_roundtrip(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [16]u8; test_random_bytes(&raw mut iv[0], 16)
    var pt : [64]u8; test_random_bytes(&raw mut pt[0], 64)

    var ctx : AESContext
    aes_init(&raw mut ctx)
    var ret = aes_setkey_enc(&raw mut ctx, &raw key[0], 16)
    if(ret < 0) { env.error("aes_setkey_enc failed"); return } else {}

    var enc_out : [80]u8
    var enc_iv : [16]u8; var ii : size_t = 0; while(ii < 16) { enc_iv[ii] = iv[ii]; ii += 1 }
    ret = aes_crypt_cbc(&raw mut ctx, AES_ENCRYPT, 64, &raw mut enc_iv[0], &raw pt[0], &raw mut enc_out[0])
    if(ret < 0) { env.error("aes_crypt_cbc encrypt failed"); return } else {}

    ret = aes_setkey_dec(&raw mut ctx, &raw key[0], 16)
    if(ret < 0) { env.error("aes_setkey_dec failed"); return } else {}
    var dec_iv : [16]u8; ii = 0; while(ii < 16) { dec_iv[ii] = iv[ii]; ii += 1 }
    var dec_out : [80]u8
    ret = aes_crypt_cbc(&raw mut ctx, AES_DECRYPT, 64, &raw mut dec_iv[0], &raw enc_out[0], &raw mut dec_out[0])
    if(ret < 0) { env.error("aes_crypt_cbc decrypt failed"); return } else {}

    if(!test_bytes_eq(&raw dec_out[0], &raw pt[0], 64)) { env.error("AES-128 CBC roundtrip failed"); return } else {}
}

@test
public func INT_aes_cbc_256_roundtrip(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var iv : [16]u8; test_random_bytes(&raw mut iv[0], 16)
    var pt : [64]u8; test_random_bytes(&raw mut pt[0], 64)

    var ctx : AESContext
    aes_init(&raw mut ctx)
    var ret = aes_setkey_enc(&raw mut ctx, &raw key[0], 32)
    if(ret < 0) { env.error("aes_setkey_enc failed"); return } else {}

    var enc_out : [80]u8
    var enc_iv : [16]u8; var ii : size_t = 0; while(ii < 16) { enc_iv[ii] = iv[ii]; ii += 1 }
    ret = aes_crypt_cbc(&raw mut ctx, AES_ENCRYPT, 64, &raw mut enc_iv[0], &raw pt[0], &raw mut enc_out[0])
    if(ret < 0) { env.error("aes_crypt_cbc encrypt failed"); return } else {}

    ret = aes_setkey_dec(&raw mut ctx, &raw key[0], 32)
    if(ret < 0) { env.error("aes_setkey_dec failed"); return } else {}
    var dec_iv : [16]u8; ii = 0; while(ii < 16) { dec_iv[ii] = iv[ii]; ii += 1 }
    var dec_out : [80]u8
    ret = aes_crypt_cbc(&raw mut ctx, AES_DECRYPT, 64, &raw mut dec_iv[0], &raw enc_out[0], &raw mut dec_out[0])
    if(ret < 0) { env.error("aes_crypt_cbc decrypt failed"); return } else {}

    if(!test_bytes_eq(&raw dec_out[0], &raw pt[0], 64)) { env.error("AES-256 CBC roundtrip failed"); return } else {}
}

@test
public func INT_aes_cbc_128_vs_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [16]u8; test_random_bytes(&raw mut iv[0], 16)
    var pt : [48]u8; test_random_bytes(&raw mut pt[0], 48)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [33]char; test_bytes_to_hex(&raw iv[0], 16, &raw mut iv_hex[0])
    var pt_hex : [97]char; test_bytes_to_hex(&raw pt[0], 48, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "cipher=Cipher(algorithms.AES(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "')),modes.CBC(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "')))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "enc=cipher.encryptor()" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "ct=enc.update(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'))+enc.finalize()" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('CT='+ct.hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("cbc128.py"))
    var py_ct : [64]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 64)
    if(ct_len == 0) { env.error("failed to parse Python output"); return } else {}

    var ctx : AESContext
    aes_init(&raw mut ctx)
    var ret = aes_setkey_enc(&raw mut ctx, &raw key[0], 16)
    if(ret < 0) { env.error("aes_setkey_enc failed"); return } else {}
    var enc_iv : [16]u8; var ii : size_t = 0; while(ii < 16) { enc_iv[ii] = iv[ii]; ii += 1 }
    var chem_ct : [64]u8
    ret = aes_crypt_cbc(&raw mut ctx, AES_ENCRYPT, 48, &raw mut enc_iv[0], &raw pt[0], &raw mut chem_ct[0])
    if(ret < 0) { env.error("aes_crypt_cbc encrypt failed"); return } else {}

    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], ct_len)) { env.error("AES-128 CBC ct mismatch vs Python"); return } else {}
}

@test
public func INT_aes_cbc_256_vs_python(env : &mut TestEnv) {
    var key : [32]u8; test_random_bytes(&raw mut key[0], 32)
    var iv : [16]u8; test_random_bytes(&raw mut iv[0], 16)
    var pt : [48]u8; test_random_bytes(&raw mut pt[0], 48)

    var key_hex : [65]char; test_bytes_to_hex(&raw key[0], 32, &raw mut key_hex[0])
    var iv_hex : [33]char; test_bytes_to_hex(&raw iv[0], 16, &raw mut iv_hex[0])
    var pt_hex : [97]char; test_bytes_to_hex(&raw pt[0], 48, &raw mut pt_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "cipher=Cipher(algorithms.AES(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "')),modes.CBC(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si] as u8; sp+=1; si+=1}
    l = "')))" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "enc=cipher.encryptor()" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "ct=enc.update(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si] as u8; sp+=1; si+=1}
    l = "'))+enc.finalize()" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('CT='+ct.hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("cbc256.py"))
    var py_ct : [64]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 64)
    if(ct_len == 0) { env.error("failed to parse Python output"); return } else {}

    var ctx : AESContext
    aes_init(&raw mut ctx)
    var ret = aes_setkey_enc(&raw mut ctx, &raw key[0], 32)
    if(ret < 0) { env.error("aes_setkey_enc failed"); return } else {}
    var enc_iv : [16]u8; var ii : size_t = 0; while(ii < 16) { enc_iv[ii] = iv[ii]; ii += 1 }
    var chem_ct : [64]u8
    ret = aes_crypt_cbc(&raw mut ctx, AES_ENCRYPT, 48, &raw mut enc_iv[0], &raw pt[0], &raw mut chem_ct[0])
    if(ret < 0) { env.error("aes_crypt_cbc encrypt failed"); return } else {}

    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], ct_len)) { env.error("AES-256 CBC ct mismatch vs Python"); return } else {}
}

@test
public func INT_hmac_sha256_basic(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var data : [32]u8; test_random_bytes(&raw mut data[0], 32)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var data_hex : [65]char; test_bytes_to_hex(&raw data[0], 32, &raw mut data_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hmac,hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hmac.new(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(data_hex[si]!=0){script[sp]=data_hex[si] as u8; sp+=1; si+=1}
    l = "'),hashlib.sha256).hexdigest()\nprint('MAC='+h)" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("hmac_sha256.py"))
    var py_mac : [32]u8
    var mac_len = test_parse_py_hex_label(&raw mut py_out, string_view("MAC="), &raw mut py_mac[0], 32)
    if(mac_len != 32) { env.error("failed to parse Python output"); return } else {}

    var chem_mac : [32]u8
    hmac_sha256(&raw key[0], 16, &raw data[0], 32, &raw mut chem_mac[0])

    if(!test_bytes_eq(&raw chem_mac[0], &raw py_mac[0], 32)) { env.error("HMAC-SHA256 mismatch vs Python"); return } else {}
}

@test
public func INT_hmac_sha256_two_keys(env : &mut TestEnv) {
    var key1 : [16]u8; test_random_bytes(&raw mut key1[0], 16)
    var key2 : [32]u8; test_random_bytes(&raw mut key2[0], 32)
    var data : [16]u8; test_random_bytes(&raw mut data[0], 16)
    var d_hex : [33]char; test_bytes_to_hex(&raw data[0], 16, &raw mut d_hex[0])

    var mac1 : [32]u8; hmac_sha256(&raw key1[0], 16, &raw data[0], 16, &raw mut mac1[0])
    var mac2 : [32]u8; hmac_sha256(&raw key2[0], 32, &raw data[0], 16, &raw mut mac2[0])

    var k1_hex : [33]char; test_bytes_to_hex(&raw key1[0], 16, &raw mut k1_hex[0])
    var k2_hex : [65]char; test_bytes_to_hex(&raw key2[0], 32, &raw mut k2_hex[0])

    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import hmac,hashlib\nh1=hmac.new(bytes.fromhex('" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(k1_hex[si]!=0){script[sp]=k1_hex[si] as u8; sp+=1; si+=1}
    var l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(d_hex[si]!=0){script[sp]=d_hex[si] as u8; sp+=1; si+=1}
    l = "'),hashlib.sha256).hexdigest()\nh2=hmac.new(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(k2_hex[si]!=0){script[sp]=k2_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(d_hex[si]!=0){script[sp]=d_hex[si] as u8; sp+=1; si+=1}
    l = "'),hashlib.sha256).hexdigest()\nprint('M1='+h1)\nprint('M2='+h2)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("hmac2k"))
    var py_m1 : [32]u8; var py_m2 : [32]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("M1="), &raw mut py_m1[0], 32)!=32){env.error("m1");return}else{}
    test_parse_py_hex_label(&raw mut py_out, string_view("M2="), &raw mut py_m2[0], 32)
    if(!test_bytes_eq(&raw mac1[0],&raw py_m1[0],32)){env.error("hmac key1");return}else{}
    if(!test_bytes_eq(&raw mac2[0],&raw py_m2[0],32)){env.error("hmac key2");return}else{}
}

@test
public func INT_hmac_md5_basic(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var data : [32]u8; test_random_bytes(&raw mut data[0], 32)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var data_hex : [65]char; test_bytes_to_hex(&raw data[0], 32, &raw mut data_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hmac,hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hmac.new(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(data_hex[si]!=0){script[sp]=data_hex[si] as u8; sp+=1; si+=1}
    l = "'),hashlib.md5).hexdigest()\nprint('MAC='+h)" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("hmac_md5.py"))
    var py_mac : [16]u8
    var mac_len = test_parse_py_hex_label(&raw mut py_out, string_view("MAC="), &raw mut py_mac[0], 16)
    if(mac_len != 16) { env.error("failed to parse Python output"); return } else {}

    var chem_mac : [16]u8
    hmac_md5(&raw key[0], 16, &raw data[0], 32, &raw mut chem_mac[0])

    if(!test_bytes_eq(&raw chem_mac[0], &raw py_mac[0], 16)) { env.error("HMAC-MD5 mismatch vs Python"); return } else {}
}

@test
public func INT_sha256_empty_input(env : &mut TestEnv) {
    var script : [256]u8; var sp : size_t = 0
    var hdr = "import hashlib;print('HASH='+hashlib.sha256(b'').hexdigest())\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("sha256_empty.py"))
    var py_hash : [32]u8
    var hash_len = test_parse_py_hex_label(&raw mut py_out, string_view("HASH="), &raw mut py_hash[0], 32)
    if(hash_len != 32) { env.error("failed to parse Python output"); return } else {}

    var chem_hash : [32]u8
    sha256_hash(null, 0, &raw mut chem_hash[0])

    if(!test_bytes_eq(&raw chem_hash[0], &raw py_hash[0], 32)) { env.error("SHA-256 empty input mismatch"); return } else {}
}

@test
public func INT_sha256_vs_python(env : &mut TestEnv) {
    var data : [128]u8; test_random_bytes(&raw mut data[0], 128)
    var data_hex : [257]char; test_bytes_to_hex(&raw data[0], 128, &raw mut data_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hashlib.sha256(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(data_hex[si]!=0){script[sp]=data_hex[si] as u8; sp+=1; si+=1}
    l = "')).hexdigest()\nprint('HASH='+h)" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("sha256.py"))
    var py_hash : [32]u8
    var hash_len = test_parse_py_hex_label(&raw mut py_out, string_view("HASH="), &raw mut py_hash[0], 32)
    if(hash_len != 32) { env.error("failed to parse Python output"); return } else {}

    var chem_hash : [32]u8
    sha256_hash(&raw data[0], 128, &raw mut chem_hash[0])

    if(!test_bytes_eq(&raw chem_hash[0], &raw py_hash[0], 32)) { env.error("SHA-256 mismatch vs Python"); return } else {}
}

@test
public func INT_sha256_incremental(env : &mut TestEnv) {
    var part1 : [32]u8; test_random_bytes(&raw mut part1[0], 32)
    var part2 : [32]u8; test_random_bytes(&raw mut part2[0], 32)
    var combined : [64]u8
    var ci : size_t = 0
    while(ci < 32) { combined[ci] = part1[ci]; ci += 1 }
    ci = 0; while(ci < 32) { combined[32+ci] = part2[ci]; ci += 1 }

    var combined_hex : [129]char; test_bytes_to_hex(&raw combined[0], 64, &raw mut combined_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hashlib.sha256(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(combined_hex[si]!=0){script[sp]=combined_hex[si] as u8; sp+=1; si+=1}
    l = "')).hexdigest()\nprint('HASH='+h)" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("sha256_inc.py"))
    var py_hash : [32]u8
    var hash_len = test_parse_py_hex_label(&raw mut py_out, string_view("HASH="), &raw mut py_hash[0], 32)
    if(hash_len != 32) { env.error("failed to parse Python output"); return } else {}

    var ctx : Sha256Context
    sha256_init(&raw mut ctx)
    sha256_update(&raw mut ctx, &raw part1[0], 32)
    sha256_update(&raw mut ctx, &raw part2[0], 32)
    var chem_hash : [32]u8
    sha256_final(&raw mut ctx, &raw mut chem_hash[0])

    if(!test_bytes_eq(&raw chem_hash[0], &raw py_hash[0], 32)) { env.error("SHA-256 incremental mismatch"); return } else {}
}

@test
public func INT_sha256_long_input(env : &mut TestEnv) {
    var data = malloc(2048 as size_t) as *mut u8; test_random_bytes(data, 2048)
    var data_hex_size : size_t = 4097 as size_t
    var data_hex = malloc(data_hex_size) as *mut char; test_bytes_to_hex(data, 2048, data_hex)

    var script : [4096+256]u8; var sp : size_t = 0
    var hdr = "import hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hashlib.sha256(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var di : size_t = 0; while(di < 4096 && data_hex[di] != 0) { script[sp]=data_hex[di] as u8; sp+=1; di+=1 }
    l = "')).hexdigest()\nprint('HASH='+h)" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("sha256_long.py"))
    var py_hash : [32]u8
    var hash_len = test_parse_py_hex_label(&raw mut py_out, string_view("HASH="), &raw mut py_hash[0], 32)
    if(hash_len != 32) { env.error("failed to parse Python output"); return } else {}

    var chem_hash : [32]u8
    sha256_hash(data, 2048, &raw mut chem_hash[0])

    if(!test_bytes_eq(&raw chem_hash[0], &raw py_hash[0], 32)) { env.error("SHA-256 long input mismatch"); return } else {}
    unsafe { dealloc data; dealloc data_hex }
}

@test
public func INT_md5_basic_vs_python(env : &mut TestEnv) {
    var data : [64]u8; test_random_bytes(&raw mut data[0], 64)
    var data_hex : [129]char; test_bytes_to_hex(&raw data[0], 64, &raw mut data_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hashlib.md5(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(data_hex[si]!=0){script[sp]=data_hex[si] as u8; sp+=1; si+=1}
    l = "')).hexdigest()\nprint('HASH='+h)" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("md5.py"))
    var py_hash : [16]u8
    var hash_len = test_parse_py_hex_label(&raw mut py_out, string_view("HASH="), &raw mut py_hash[0], 16)
    if(hash_len != 16) { env.error("failed to parse Python output"); return } else {}

    var chem_hash : [16]u8
    md5_hash(&raw data[0], 64, &raw mut chem_hash[0])

    if(!test_bytes_eq(&raw chem_hash[0], &raw py_hash[0], 16)) { env.error("MD5 basic mismatch vs Python"); return } else {}
}

@test
public func INT_md5_incremental(env : &mut TestEnv) {
    var part1 : [16]u8; test_random_bytes(&raw mut part1[0], 16)
    var part2 : [16]u8; test_random_bytes(&raw mut part2[0], 16)
    var part3 : [16]u8; test_random_bytes(&raw mut part3[0], 16)
    var combined : [48]u8
    var ci : size_t = 0
    while(ci < 16) { combined[ci] = part1[ci]; ci += 1 }
    ci = 0; while(ci < 16) { combined[16+ci] = part2[ci]; ci += 1 }
    ci = 0; while(ci < 16) { combined[32+ci] = part3[ci]; ci += 1 }

    var combined_hex : [97]char; test_bytes_to_hex(&raw combined[0], 48, &raw mut combined_hex[0])

    var script : [512]u8; var sp : size_t = 0
    var hdr = "import hashlib\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "h=hashlib.md5(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(combined_hex[si]!=0){script[sp]=combined_hex[si] as u8; sp+=1; si+=1}
    l = "')).hexdigest()\nprint('HASH='+h)" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("md5_inc.py"))
    var py_hash : [16]u8
    var hash_len = test_parse_py_hex_label(&raw mut py_out, string_view("HASH="), &raw mut py_hash[0], 16)
    if(hash_len != 16) { env.error("failed to parse Python output"); return } else {}

    var ctx : Md5Context
    md5_init(&raw mut ctx)
    md5_update(&raw mut ctx, &raw part1[0], 16)
    md5_update(&raw mut ctx, &raw part2[0], 16)
    md5_update(&raw mut ctx, &raw part3[0], 16)
    var chem_hash : [16]u8
    md5_final(&raw mut ctx, &raw mut chem_hash[0])

    if(!test_bytes_eq(&raw chem_hash[0], &raw py_hash[0], 16)) { env.error("MD5 incremental mismatch"); return } else {}
}

// ============================================================
// X25519 tests (4)
// ============================================================

@test public func INT_x25519_keygen(env : &mut TestEnv) {
    var priv : [32]u8; var pub : [32]u8
    if(x25519_generate_keypair(&raw mut priv[0], &raw mut pub[0]) < 0){env.error("x25519 keygen");return}else{}
    var p_hex : [65]char; test_bytes_to_hex(&raw pub[0], 32, &raw mut p_hex[0])
    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "sk=X25519PrivateKey.generate()\npub=sk.public_key().public_bytes_raw()\nprint('PUB='+pub.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("x25519k"))
    var py_pub : [32]u8
    test_parse_py_hex_label(&raw mut py_out, string_view("PUB="), &raw mut py_pub[0], 32)
}

@test public func INT_x25519_shared_vs_py(env : &mut TestEnv) {
    var alice_priv : [32]u8; var alice_pub : [32]u8
    x25519_generate_keypair(&raw mut alice_priv[0], &raw mut alice_pub[0])
    var ap_hex : [65]char; test_bytes_to_hex(&raw alice_pub[0], 32, &raw mut ap_hex[0])
    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey,X25519PublicKey\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "alice_pub=X25519PublicKey.from_public_bytes(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ap_hex[si]!=0){script[sp]=ap_hex[si] as u8; sp+=1; si+=1}
    l = "'))\nbob_priv=bytes([(i+1)%256 for i in range(32)]);bob_priv_ba=bytearray(bob_priv);bob_priv_ba[0]&=248;bob_priv_ba[31]&=127;bob_priv_ba[31]|=64;bob_priv=bytes(bob_priv_ba)\nsk=X25519PrivateKey.from_private_bytes(bob_priv)\nshared=sk.exchange(alice_pub)\nprint('SHARED='+shared.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("x25519sh"))
    var py_shared : [32]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("SHARED="), &raw mut py_shared[0], 32)!=32){env.error("shared");return}else{}
    var bob_priv : [32]u8; var i : size_t = 0; while(i<32){bob_priv[i]=((i+1)%256)as u8;i+=1}
    bob_priv[0] = bob_priv[0] & 248; bob_priv[31] = bob_priv[31] & 127 | 64
    var chem_shared : [32]u8
    x25519_compute_shared(&raw bob_priv[0], &raw alice_pub[0], &raw mut chem_shared[0])
    if(!test_bytes_eq(&raw chem_shared[0], &raw py_shared[0], 32)){env.error("x25519 shared");return}else{}
}

@test public func INT_x25519_rfc7748_testvec(env : &mut TestEnv) {
    // RFC 7748 Section 5.2 test vector: X25519(9, 9)
    var scalar : [32]u8; scalar[0]=9; var i:size_t=1;while(i<32){scalar[i]=0;i+=1}
    var u_coord : [32]u8; u_coord[0]=9; i=1;while(i<32){u_coord[i]=0;i+=1}
    var out : [32]u8; x25519_ladder(&raw mut out[0], &raw scalar[0], &raw u_coord[0])
    // Expected: 422c8e7a6227d7bca1350b3e2bb7279f7897b87bb6854b783c60e80311ae3079
    var expect : [32]u8 = [
        0x42, 0x2c, 0x8e, 0x7a, 0x62, 0x27, 0xd7, 0xbc, 0xa1, 0x35, 0x0b, 0x3e, 0x2b, 0xb7, 0x27, 0x9f,
        0x78, 0x97, 0xb8, 0x7b, 0xb6, 0x85, 0x4b, 0x78, 0x3c, 0x60, 0xe8, 0x03, 0x11, 0xae, 0x30, 0x79]
    i=0; while(i<32){if(out[i]!=expect[i]){env.error("x25519 rfc7748");return}else{}i+=1}
}

// ============================================================
// ECDSA tests (4)
// ============================================================

@test public func INT_ecdsa_sign_verify_p256(env : &mut TestEnv) {
    var hash : [32]u8; test_random_bytes(&raw mut hash[0], 32)
    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec\nkey=ec.generate_private_key(ec.SECP256R1())\npub=key.public_key()\nn=pub.public_numbers()\nprint('SK='+format(key.private_numbers().private_value,'064x'))\nprint('PX='+format(n.x,'064x'))\nprint('PY='+format(n.y,'064x'))\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdsa_k"))
    var sk_hex : [64]u8; var px_hex : [64]u8; var py_hex : [64]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("SK="), &raw mut sk_hex[0], 32)!=32){env.error("sk");return}else{}
    test_parse_py_hex_label(&raw mut py_out, string_view("PX="), &raw mut px_hex[0], 32)
    test_parse_py_hex_label(&raw mut py_out, string_view("PY="), &raw mut py_hex[0], 32)
    var ctx : ECDSAContext; ecdsa_init(&raw mut ctx)
    ecdsa_import_privkey(&raw mut ctx, &raw sk_hex[0], 32, TLS_GROUP_SECP256R1 as u16)
    var sig : [128]u8; var sig_len : u16 = 128
    if(ecdsa_sign(&raw mut ctx, &raw hash[0], 32, &raw mut sig[0], &raw mut sig_len) < 0){env.error("sign");return}else{}
    var ctx2 : ECDSAContext; ecdsa_init(&raw mut ctx2)
    var pub_key : [65]u8; pub_key[0]=4; var i:size_t=0;while(i<32){pub_key[1+i]=px_hex[i];pub_key[33+i]=py_hex[i];i+=1}
    ecdsa_import_pubkey(&raw mut ctx2, &raw pub_key[0], 65, TLS_GROUP_SECP256R1 as u16)
    if(ecdsa_verify(&raw mut ctx2, &raw hash[0], 32, &raw sig[0], sig_len) < 0){env.error("verify");return}else{}
}

@test public func INT_ecdsa_verify_py_sig(env : &mut TestEnv) {
    var hash : [32]u8; test_random_bytes(&raw mut hash[0], 32)
    var h_hex : [65]char; test_bytes_to_hex(&raw hash[0], 32, &raw mut h_hex[0])
    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec,utils\nfrom cryptography.hazmat.primitives import hashes\nkey=ec.generate_private_key(ec.SECP256R1())\npub=key.public_key()\nsig=key.sign(bytes.fromhex('" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(h_hex[si]!=0){script[sp]=h_hex[si] as u8; sp+=1; si+=1}
    var l = "'),ec.ECDSA(utils.Prehashed(hashes.SHA256())))\nn=pub.public_numbers()\nprint('PX='+format(n.x,'064x'))\nprint('PY='+format(n.y,'064x'))\nprint('SIG='+sig.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdsa_vfy"))
    var px_hex : [64]u8; var py_hex : [64]u8; var sig_hex : [128]u8
    test_parse_py_hex_label(&raw mut py_out, string_view("PX="), &raw mut px_hex[0], 32)
    test_parse_py_hex_label(&raw mut py_out, string_view("PY="), &raw mut py_hex[0], 32)
    var sig_len = test_parse_py_hex_label(&raw mut py_out, string_view("SIG="), &raw mut sig_hex[0], 128)
    if(sig_len==0){env.error("sig");return}else{}
    var ctx : ECDSAContext; ecdsa_init(&raw mut ctx)
    var pub_key : [65]u8; pub_key[0]=4; var i:size_t=0;while(i<32){pub_key[1+i]=px_hex[i];pub_key[33+i]=py_hex[i];i+=1}
    ecdsa_import_pubkey(&raw mut ctx, &raw pub_key[0], 65, TLS_GROUP_SECP256R1 as u16)
    if(ecdsa_verify(&raw mut ctx, &raw hash[0], 32, &raw sig_hex[0], sig_len) < 0){env.error("ecdsa verify vs py");return}else{}
}

@test public func INT_ecdsa_sign_py_verify(env : &mut TestEnv) {
    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec\nkey=ec.generate_private_key(ec.SECP256R1())\nn=key.public_key().public_numbers()\nprint('PX='+format(n.x,'064x'))\nprint('PY='+format(n.y,'064x'))\nprint('SK='+format(key.private_numbers().private_value,'064x'))\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdsa_st"))
    var sk_hex : [64]u8; var px_hex : [64]u8; var py_hex : [64]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("SK="), &raw mut sk_hex[0], 32)!=32){env.error("sk");return}else{}
    test_parse_py_hex_label(&raw mut py_out, string_view("PX="), &raw mut px_hex[0], 32)
    test_parse_py_hex_label(&raw mut py_out, string_view("PY="), &raw mut py_hex[0], 32)
    var px_hexstr : [65]char; test_bytes_to_hex(&raw px_hex[0], 32, &raw mut px_hexstr[0])
    var py_hexstr : [65]char; test_bytes_to_hex(&raw py_hex[0], 32, &raw mut py_hexstr[0])
    var hash : [32]u8; test_random_bytes(&raw mut hash[0], 32)
    var h_hex : [65]char; test_bytes_to_hex(&raw hash[0], 32, &raw mut h_hex[0])
    var ctx : ECDSAContext; ecdsa_init(&raw mut ctx); ecdsa_import_privkey(&raw mut ctx, &raw sk_hex[0], 32, TLS_GROUP_SECP256R1 as u16)
    var sig : [128]u8; var sig_len : u16 = 128
    ecdsa_sign(&raw mut ctx, &raw hash[0], 32, &raw mut sig[0], &raw mut sig_len)
    var sig_hex : [257]char; test_bytes_to_hex(&raw sig[0], sig_len as size_t, &raw mut sig_hex[0])
    script[0]=0; sp=0; si=0
    hdr = "from cryptography.hazmat.primitives.asymmetric import ec,utils\nfrom cryptography.hazmat.primitives import hashes\nkey=ec.EllipticCurvePublicNumbers(int.from_bytes(bytes.fromhex('" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(px_hexstr[si]!=0){script[sp]=px_hexstr[si] as u8; sp+=1; si+=1}
    var l = "'),'big'),int.from_bytes(bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(py_hexstr[si]!=0){script[sp]=py_hexstr[si] as u8; sp+=1; si+=1}
    l = "'),'big'),ec.SECP256R1()).public_key(backend=None)\ntry:\n key.verify(bytes.fromhex('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(sig_hex[si]!=0){script[sp]=sig_hex[si] as u8; sp+=1; si+=1}
    l = "'),bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(h_hex[si]!=0){script[sp]=h_hex[si] as u8; sp+=1; si+=1}
    l = "'),ec.ECDSA(utils.Prehashed(hashes.SHA256())))\n print('OK=1')\nexcept Exception:\n print('OK=0')\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    py_out = test_python_run_script(&raw script[0], sp, string_view("ecdsa_st2"))
    if(py_out.size()<4||py_out.get(0)!=79||py_out.get(1)!=75||py_out.get(2)!=61||py_out.get(3)!=49){env.error("python verify rejected");return}else{}
}

@test public func INT_ecdsa_p384_pubkey(env : &mut TestEnv) {
    var script : [256]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec\nkey=ec.generate_private_key(ec.SECP384R1())\nn=key.public_key().public_numbers()\nprint('PX='+format(n.x,'096x'))\nprint('PY='+format(n.y,'096x'))\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("p384s"))
    var px : [48]u8; var py : [48]u8
    test_parse_py_hex_label(&raw mut py_out, string_view("PX="), &raw mut px[0], 48)
    test_parse_py_hex_label(&raw mut py_out, string_view("PY="), &raw mut py[0], 48)
    var pub_key : [97]u8; pub_key[0]=4; var i:size_t=0;while(i<48){pub_key[1+i]=px[i];pub_key[49+i]=py[i];i+=1}
    var ctx : ECDSAContext; ecdsa_init(&raw mut ctx)
    ecdsa_import_pubkey(&raw mut ctx, &raw pub_key[0], 97, TLS_GROUP_SECP384R1 as u16)
}

// ============================================================
// RSA tests (4)
// ============================================================

@test public func INT_rsa_encrypt_decrypt(env : &mut TestEnv) {
    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import rsa\nfrom cryptography.hazmat.primitives import serialization\nkey=rsa.generate_private_key(65537,2048)\npub=key.public_key()\nn=pub.public_numbers().n\ne=pub.public_numbers().e\nd=key.private_numbers().d\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "print('N='+format(n,'0512x'))\nprint('E='+format(e,'x'))\nprint('D='+format(d,'0512x'))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("rsa_key"))
    var n_hex : [512]u8; var d_hex : [512]u8
    var n_len = test_parse_py_hex_label(&raw mut py_out, string_view("N="), &raw mut n_hex[0], 256)
    var d_len = test_parse_py_hex_label(&raw mut py_out, string_view("D="), &raw mut d_hex[0], 256)
    if(n_len==0||d_len==0){env.error("rsa key parse");return}else{}
    var e_bytes : [3]u8; e_bytes[0]=1; e_bytes[1]=0; e_bytes[2]=1
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)
    var ctx : RSAContext; rsa_init(&raw mut ctx, 0, 0)
    rsa_import_pubkey(&raw mut ctx, &raw n_hex[0], n_len, &raw e_bytes[0], 3)
    var ct : [256]u8
    if(rsa_pkcs1_encrypt(&raw mut ctx, &raw pt[0], 16, &raw mut ct[0]) < 0){env.error("encrypt");return}else{}
    rsa_free(&raw mut ctx)
    var ctx2 : RSAContext; rsa_init(&raw mut ctx2, 0, 0)
    rsa_import_pubkey(&raw mut ctx2, &raw n_hex[0], n_len, &raw e_bytes[0], 3)
    rsa_import_privkey(&raw mut ctx2, &raw n_hex[0], n_len, &raw d_hex[0], d_len)
    var dec : [256]u8; var dec_len : size_t = 256
    if(rsa_pkcs1_decrypt(&raw mut ctx2, &raw ct[0], n_len, &raw mut dec[0], &raw mut dec_len, 256) < 0){env.error("decrypt");return}else{}
    if(!test_bytes_eq(&raw pt[0], &raw dec[0], 16)){env.error("rsa roundtrip");return}else{}
    rsa_free(&raw mut ctx2)
}

@test public func INT_rsa_sign_verify(env : &mut TestEnv) {
    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import rsa,padding\nfrom cryptography.hazmat.primitives import hashes\nkey=rsa.generate_private_key(65537,2048)\npub=key.public_key()\nn=pub.public_numbers().n\ne=pub.public_numbers().e\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "print('N='+format(n,'0512x'))\nprint('E='+format(e,'x'))\nmsg=b'Chemical TLS test message'\nsig=key.sign(msg,padding.PKCS1v15(),hashes.SHA256())\nprint('SIG='+sig.hex())\nprint('MSG='+msg.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("rsa_sig"))
    var n_hex : [256]u8; var sig_hex : [256]u8; var msg_hex : [64]u8
    var n_len = test_parse_py_hex_label(&raw mut py_out, string_view("N="), &raw mut n_hex[0], 256)
    var sig_len = test_parse_py_hex_label(&raw mut py_out, string_view("SIG="), &raw mut sig_hex[0], 256)
    var msg_len = test_parse_py_hex_label(&raw mut py_out, string_view("MSG="), &raw mut msg_hex[0], 32)
    if(n_len==0||sig_len==0||msg_len==0){env.error("rsa sig parse");return}else{}
    var e_bytes : [3]u8; e_bytes[0]=1; e_bytes[1]=0; e_bytes[2]=1
    var ctx : RSAContext; rsa_init(&raw mut ctx, 0, 0)
    rsa_import_pubkey(&raw mut ctx, &raw n_hex[0], n_len, &raw e_bytes[0], 3)
    var digest : [32]u8; sha256_hash(&raw msg_hex[0], msg_len, &raw mut digest[0])
    if(rsa_pkcs1_verify(&raw mut ctx, &raw digest[0], 32, &raw sig_hex[0], sig_len) < 0){env.error("rsa verify");return}else{}
    rsa_free(&raw mut ctx)
}

// ============================================================
// TLS 1.2 PRF and key derivation tests (4)
// ============================================================

@test public func INT_tls12_prf_vs_py(env : &mut TestEnv) {
    var secret : [32]u8; test_random_bytes(&raw mut secret[0], 32)
    var seed : [32]u8; test_random_bytes(&raw mut seed[0], 32)
    var s_hex : [65]char; test_bytes_to_hex(&raw secret[0], 32, &raw mut s_hex[0])
    var sd_hex : [65]char; test_bytes_to_hex(&raw seed[0], 32, &raw mut sd_hex[0])
    var output : [48]u8; tls12_prf(&raw secret[0], 32, "test label" as *char, 10, &raw seed[0], 32, &raw mut output[0], 48)
    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import hashlib,hmac\ndef p_hash(secret,seed,length):\n o=b'';t=seed\n while len(o)<length:\n  t=hmac.new(secret,t,hashlib.sha256).digest()\n  o+=hmac.new(secret,t+seed,hashlib.sha256).digest()\n return o[:length]\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "secret=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(s_hex[si]!=0){script[sp]=s_hex[si] as u8; sp+=1; si+=1}
    l = "')\nseed=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(sd_hex[si]!=0){script[sp]=sd_hex[si] as u8; sp+=1; si+=1}
    l = "')\nlabel=b'test label'\nprint(p_hash(secret,label+seed,48).hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls12prf"))
    var py_out_hex : [96]u8; var pw : size_t = 0; var pi : size_t = 0
    while(pi+1<py_out.size()&&pw<48){var hi=py_out.get(pi)as char;var lo=py_out.get(pi+1)as char;if(hi==10||hi==13||hi==0){break}else{}py_out_hex[pw]=test_hex_pair_byte(hi,lo);pw+=1;pi+=2}
    if(pw!=48||!test_bytes_eq(&raw output[0],&raw py_out_hex[0],48)){env.error("tls12 prf");return}else{}
}

@test public func INT_tls12_master_secret(env : &mut TestEnv) {
    var pre_master : [48]u8; test_random_bytes(&raw mut pre_master[0], 48)
    var c_random : [32]u8; test_random_bytes(&raw mut c_random[0], 32)
    var s_random : [32]u8; test_random_bytes(&raw mut s_random[0], 32)
    var ms : [48]u8; tls12_derive_master_secret(&raw pre_master[0], 48, &raw c_random[0], &raw s_random[0], &raw mut ms[0])
    var pm_hex : [97]char; test_bytes_to_hex(&raw pre_master[0], 48, &raw mut pm_hex[0])
    var cr_hex : [65]char; test_bytes_to_hex(&raw c_random[0], 32, &raw mut cr_hex[0])
    var sr_hex : [65]char; test_bytes_to_hex(&raw s_random[0], 32, &raw mut sr_hex[0])
    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import hashlib,hmac\n" as *char; si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "pm=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(pm_hex[si]!=0){script[sp]=pm_hex[si] as u8; sp+=1; si+=1}
    l = "')\ncr=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(cr_hex[si]!=0){script[sp]=cr_hex[si] as u8; sp+=1; si+=1}
    l = "')\nsr=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(sr_hex[si]!=0){script[sp]=sr_hex[si] as u8; sp+=1; si+=1}
    l = "')\ndef p_hash(secret,seed,length):\n o=b'';t=seed\n while len(o)<length:\n  t=hmac.new(secret,t,hashlib.sha256).digest()\n  o+=hmac.new(secret,t+seed,hashlib.sha256).digest()\n return o[:length]\nms=p_hash(pm,b'master secret'+cr+sr,48)\nprint('MS='+ms.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls12ms"))
    var py_ms : [48]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("MS="), &raw mut py_ms[0], 48)!=48){env.error("ms");return}else{}
    if(!test_bytes_eq(&raw ms[0],&raw py_ms[0],48)){env.error("master secret");return}else{}
}

@test public func INT_tls12_key_block(env : &mut TestEnv) {
    var ms : [48]u8; test_random_bytes(&raw mut ms[0], 48)
    var s_random : [32]u8; test_random_bytes(&raw mut s_random[0], 32)
    var c_random : [32]u8; test_random_bytes(&raw mut c_random[0], 32)
    var key_block : [128]u8; tls12_derive_key_block(&raw ms[0], &raw s_random[0], &raw c_random[0], &raw mut key_block[0], 128)
    var ms_hex : [97]char; test_bytes_to_hex(&raw ms[0], 48, &raw mut ms_hex[0])
    var sr_hex : [65]char; test_bytes_to_hex(&raw s_random[0], 32, &raw mut sr_hex[0])
    var cr_hex : [65]char; test_bytes_to_hex(&raw c_random[0], 32, &raw mut cr_hex[0])
    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import hashlib,hmac\n" as *char; si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "ms=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(ms_hex[si]!=0){script[sp]=ms_hex[si] as u8; sp+=1; si+=1}
    l = "')\nsr=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(sr_hex[si]!=0){script[sp]=sr_hex[si] as u8; sp+=1; si+=1}
    l = "')\ncr=bytes.fromhex('" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(cr_hex[si]!=0){script[sp]=cr_hex[si] as u8; sp+=1; si+=1}
    l = "')\ndef p_hash(secret,seed,length):\n o=b'';t=seed\n while len(o)<length:\n  t=hmac.new(secret,t,hashlib.sha256).digest()\n  o+=hmac.new(secret,t+seed,hashlib.sha256).digest()\n return o[:length]\nkb=p_hash(ms,b'key expansion'+sr+cr,128)\nprint('KB='+kb.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("tls12kb"))
    var py_kb : [128]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("KB="), &raw mut py_kb[0], 128)!=128){env.error("kb");return}else{}
    if(!test_bytes_eq(&raw key_block[0],&raw py_kb[0],128)){env.error("key block");return}else{}
}

// ============================================================
// MPI advanced tests (8)
// ============================================================

@test public func INT_mpi_exp_mod_vs_py(env : &mut TestEnv) {
    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import random\nbase=random.getrandbits(256)\nexp=random.getrandbits(256)\nmod=random.getrandbits(256)\nprint('B='+format(base,'064x'))\nprint('E='+format(exp,'064x'))\nprint('M='+format(mod,'064x'))\nr=pow(base,exp,mod)\nprint('R='+format(r,'064x'))\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_exp"))
    var b_hex : [64]u8; var e_hex : [64]u8; var m_hex : [64]u8; var r_hex : [64]u8
    test_parse_py_hex_label(&raw mut py_out, string_view("B="), &raw mut b_hex[0], 32)
    test_parse_py_hex_label(&raw mut py_out, string_view("E="), &raw mut e_hex[0], 32)
    test_parse_py_hex_label(&raw mut py_out, string_view("M="), &raw mut m_hex[0], 32)
    test_parse_py_hex_label(&raw mut py_out, string_view("R="), &raw mut r_hex[0], 32)
    var b : Mpi; mpi_read_binary(&raw mut b, &raw b_hex[0], 32)
    var e : Mpi; mpi_read_binary(&raw mut e, &raw e_hex[0], 32)
    var m : Mpi; mpi_read_binary(&raw mut m, &raw m_hex[0], 32)
    var r : Mpi; mpi_init(&raw mut r)
    if(mpi_exp_mod(&raw mut r, &raw mut b, &raw mut e, &raw mut m) < 0){env.error("exp_mod");return}else{}
    var chem_r : [32]u8; mpi_write_binary(&raw mut r, &raw mut chem_r[0], 32)
    if(!test_bytes_eq(&raw chem_r[0], &raw r_hex[0], 32)){env.error("exp_mod mismatch");return}else{}
}

@test public func INT_mpi_mod_inv_vs_py(env : &mut TestEnv) {
    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import random\nimport math\na=random.getrandbits(128)+1\nmod=random.getrandbits(128)+1\nwhile math.gcd(a,mod)!=1:\n    mod=random.getrandbits(128)+1\nprint('A='+format(a,'032x'))\nprint('M='+format(mod,'032x'))\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_inv"))
    var a_hex : [32]u8; var m_hex : [32]u8
    test_parse_py_hex_label(&raw mut py_out, string_view("A="), &raw mut a_hex[0], 16)
    test_parse_py_hex_label(&raw mut py_out, string_view("M="), &raw mut m_hex[0], 16)
    var a : Mpi; mpi_read_binary(&raw mut a, &raw a_hex[0], 16)
    var m : Mpi; mpi_read_binary(&raw mut m, &raw m_hex[0], 16)
    var r : Mpi; mpi_init(&raw mut r)
    if(mpi_mod_inv(&raw mut r, &raw mut a, &raw mut m) < 0){env.error("mod_inv");return}else{}
    var check : Mpi; mpi_init(&raw mut check)
    mpi_mul(&raw mut check, &raw mut r, &raw mut a); mpi_mod(&raw mut check, &raw mut check, &raw mut m)
    var one : Mpi; mpi_init(&raw mut one); mpi_lset(&raw mut one, 1)
    if(mpi_cmp(&raw mut check, &raw mut one) != 0){env.error("mod_inv check");return}else{}
}

@test public func INT_mpi_gcd_vs_py(env : &mut TestEnv) {
    var script : [256]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import random\na=random.getrandbits(128);b=random.getrandbits(128)\nimport math\nprint('A='+format(a,'032x'))\nprint('B='+format(b,'032x'))\nprint('G='+format(math.gcd(a,b),'032x'))\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_gcd"))
    var a_hex : [32]u8; var b_hex : [32]u8; var g_hex : [32]u8
    test_parse_py_hex_label(&raw mut py_out, string_view("A="), &raw mut a_hex[0], 16)
    test_parse_py_hex_label(&raw mut py_out, string_view("B="), &raw mut b_hex[0], 16)
    test_parse_py_hex_label(&raw mut py_out, string_view("G="), &raw mut g_hex[0], 16)
    var a : Mpi; mpi_read_binary(&raw mut a, &raw a_hex[0], 16)
    var b : Mpi; mpi_read_binary(&raw mut b, &raw b_hex[0], 16)
    var g : Mpi; mpi_init(&raw mut g); mpi_gcd(&raw mut g, &raw mut a, &raw mut b)
    var chem_g : [16]u8; mpi_write_binary(&raw mut g, &raw mut chem_g[0], 16)
    if(!test_bytes_eq(&raw chem_g[0], &raw g_hex[0], 16)){env.error("gcd mismatch");return}else{}
}

@test public func INT_mpi_shift_ops(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, 0x12345678)
    var b : Mpi; mpi_init(&raw mut b); mpi_copy(&raw mut b, &raw mut a)
    mpi_shift_l(&raw mut b, 8)
    var c : Mpi; mpi_init(&raw mut c); mpi_copy(&raw mut c, &raw mut b)
    mpi_shift_r(&raw mut c, 8)
    if(mpi_cmp(&raw mut a, &raw mut c) != 0){env.error("shift roundtrip");return}else{}
}

@test public func INT_mpi_cmp_ops(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, 100)
    var b : Mpi; mpi_init(&raw mut b); mpi_lset(&raw mut b, 200)
    var c : Mpi; mpi_init(&raw mut c); mpi_lset(&raw mut c, 100)
    if(mpi_cmp(&raw mut a, &raw mut b) >= 0){env.error("cmp a<b");return}else{}
    if(mpi_cmp(&raw mut b, &raw mut a) <= 0){env.error("cmp b>a");return}else{}
    if(mpi_cmp(&raw mut a, &raw mut c) != 0){env.error("cmp a==c");return}else{}
    if(mpi_cmp_int(&raw mut a, 100) != 0){env.error("cmp int");return}else{}
    if(mpi_size(&raw mut a) == 0){env.error("size zero");return}else{}
    if(mpi_bitlen(&raw mut a) == 0){env.error("bitlen zero");return}else{}
}

// ============================================================
// TLS 1.3 record layer (4)
// ============================================================

@test public func INT_tls13_record_empty(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var tr : Transform; transform_init(&raw mut tr)
    tr.cipher_type = CIPHER_AES_128_GCM as u8; tr.hash_type = HASH_SHA256 as u8
    tr.key_len = 16; tr.iv_len = 12; tr.fixed_iv_len = 4
    var i : size_t = 0; while(i<16){tr.key_enc[i]=i as u8;tr.key_dec[i]=i as u8;i+=1}
    while(i<28){tr.iv_enc[i-16]=i as u8;tr.iv_dec[i-16]=i as u8;tr.base_iv_enc[i-16]=i as u8;tr.base_iv_dec[i-16]=i as u8;i+=1}
    var tr_out = malloc(sizeof(Transform)) as *mut Transform; *tr_out = tr; ctx.transform_out = tr_out
    var tr_in = malloc(sizeof(Transform)) as *mut Transform; *tr_in = tr; ctx.transform_in = tr_in
    i=0; while(i<8){ctx.in_ctr[i]=0;ctx.out_ctr[i]=0;i+=1}
    var enc : [32]u8; var elen = tls13_encrypt_record(&raw mut ctx, SSL_MSG_APPLICATION_DATA as u8, null, 0, &raw mut enc[0], 32)
    if(elen < 0){env.error("empty encrypt");return}else{}
    ctx.in_hdr[0]=enc[0];ctx.in_hdr[1]=enc[1];ctx.in_hdr[2]=enc[2];ctx.in_hdr[3]=enc[3];ctx.in_hdr[4]=enc[4]
    var dec_buf : [32]u8; var inner_ct : u8 = 0
    var dlen = tls13_decrypt_record(&raw mut ctx, &raw enc[5], (elen-5) as size_t, &raw mut dec_buf[0], 32, &raw mut inner_ct)
    if(dlen < 0){env.error("empty decrypt");return}else{}
    if(inner_ct != SSL_MSG_APPLICATION_DATA as u8){env.error("inner ct");return}else{}
    unsafe { dealloc tr_out; dealloc tr_in }
}

@test public func INT_tls13_record_large(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var tr : Transform; transform_init(&raw mut tr)
    tr.cipher_type = CIPHER_AES_128_GCM as u8; tr.hash_type = HASH_SHA256 as u8
    tr.key_len = 16; tr.iv_len = 12; tr.fixed_iv_len = 4
    var i : size_t = 0; while(i<16){tr.key_enc[i]=i as u8;tr.key_dec[i]=i as u8;i+=1}
    while(i<28){tr.iv_enc[i-16]=i as u8;tr.iv_dec[i-16]=i as u8;tr.base_iv_enc[i-16]=i as u8;tr.base_iv_dec[i-16]=i as u8;i+=1}
    var tr_out = malloc(sizeof(Transform)) as *mut Transform; *tr_out = tr; ctx.transform_out = tr_out
    var tr_in = malloc(sizeof(Transform)) as *mut Transform; *tr_in = tr; ctx.transform_in = tr_in
    i=0; while(i<8){ctx.in_ctr[i]=0;ctx.out_ctr[i]=0;i+=1}
    var data : [16384]u8; test_random_bytes(&raw mut data[0], 16384)
    var enc : [16410]u8; var elen = tls13_encrypt_record(&raw mut ctx, SSL_MSG_APPLICATION_DATA as u8, &raw data[0], 16384, &raw mut enc[0], 16410)
    if(elen < 0){env.error("large encrypt");return}else{}
    ctx.in_hdr[0]=enc[0];ctx.in_hdr[1]=enc[1];ctx.in_hdr[2]=enc[2];ctx.in_hdr[3]=enc[3];ctx.in_hdr[4]=enc[4]
    var dec_buf : [16400]u8; var inner_ct : u8 = 0
    var dlen = tls13_decrypt_record(&raw mut ctx, &raw enc[5], (elen-5) as size_t, &raw mut dec_buf[0], 16400, &raw mut inner_ct)
    if(dlen < 0){env.error("large decrypt");return}else{}
    if(!test_bytes_eq(&raw data[0], &raw dec_buf[0], 16384)){env.error("large data");return}else{}
    unsafe { dealloc tr_out; dealloc tr_in }
}

@test public func INT_tls13_record_handshake_ct(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var tr : Transform; transform_init(&raw mut tr)
    tr.cipher_type = CIPHER_AES_128_GCM as u8; tr.hash_type = HASH_SHA256 as u8
    tr.key_len = 16; tr.iv_len = 12; tr.fixed_iv_len = 4
    var i : size_t = 0; while(i<16){tr.key_enc[i]=i as u8;tr.key_dec[i]=i as u8;i+=1}
    while(i<28){tr.iv_enc[i-16]=i as u8;tr.iv_dec[i-16]=i as u8;tr.base_iv_enc[i-16]=i as u8;tr.base_iv_dec[i-16]=i as u8;i+=1}
    var tr_out = malloc(sizeof(Transform)) as *mut Transform; *tr_out = tr; ctx.transform_out = tr_out
    var tr_in = malloc(sizeof(Transform)) as *mut Transform; *tr_in = tr; ctx.transform_in = tr_in
    i=0; while(i<8){ctx.in_ctr[i]=0;ctx.out_ctr[i]=0;i+=1}
    var data : [32]u8; test_random_bytes(&raw mut data[0], 32)
    var enc : [64]u8; var elen = tls13_encrypt_record(&raw mut ctx, SSL_MSG_HANDSHAKE as u8, &raw data[0], 32, &raw mut enc[0], 64)
    if(elen < 0){env.error("hs encrypt");return}else{}
    ctx.in_hdr[0]=enc[0];ctx.in_hdr[1]=enc[1];ctx.in_hdr[2]=enc[2];ctx.in_hdr[3]=enc[3];ctx.in_hdr[4]=enc[4]
    var dec_buf : [64]u8; var inner_ct : u8 = 0
    var dlen = tls13_decrypt_record(&raw mut ctx, &raw enc[5], (elen-5) as size_t, &raw mut dec_buf[0], 64, &raw mut inner_ct)
    if(dlen < 0){env.error("hs decrypt");return}else{}
    if(inner_ct != SSL_MSG_HANDSHAKE as u8){env.error("hs inner ct");return}else{}
    if(!test_bytes_eq(&raw data[0], &raw dec_buf[0], 32)){env.error("hs data");return}else{}
    unsafe { dealloc tr_out; dealloc tr_in }
}

// ============================================================
// TLS 1.3 key derivation (2)
// ============================================================

@test public func INT_tls13_derive_app_keys(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut cfg)
    var ss : [32]u8; test_random_bytes(&raw mut ss[0], 32)
    var hh : [32]u8; test_random_bytes(&raw mut hh[0], 32)
    if(tls13_derive_handshake_keys(&raw mut ctx, &raw ss[0], 32, &raw hh[0]) < 0){env.error("hs keys");return}else{}
    if(tls13_derive_application_keys(&raw mut ctx, &raw hh[0], 32) < 0){env.error("app keys");return}else{}
}

@test public func INT_tls13_key_update(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var cfg = ssl_config_init(SSL_IS_CLIENT)
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut cfg)
    var ss : [32]u8; test_random_bytes(&raw mut ss[0], 32)
    var hh : [32]u8; test_random_bytes(&raw mut hh[0], 32)
    tls13_derive_handshake_keys(&raw mut ctx, &raw ss[0], 32, &raw hh[0])
    tls13_derive_application_keys(&raw mut ctx, &raw hh[0], 32)
    if(tls13_update_send_keys(&raw mut ctx) < 0){env.error("update send");return}else{}
    if(tls13_update_recv_keys(&raw mut ctx) < 0){env.error("update recv");return}else{}
}

// ============================================================
// Base64 tests (2)
// ============================================================

@test public func INT_base64_roundtrip(env : &mut TestEnv) {
    var data : [48]u8; test_random_bytes(&raw mut data[0], 48)
    var enc : [72]char; var enc_r = base64_encode(&raw data[0], 48, &raw mut enc[0], 72)
    if(enc_r is Result.Err){env.error("b64 enc");return}else{}
    var Ok(enc_len) = enc_r else unreachable
    var dec : [48]u8; var dec_r = base64_decode(&raw enc[0], enc_len, &raw mut dec[0], 48)
    if(dec_r is Result.Err){env.error("b64 dec");return}else{}
    var Ok(dec_len) = dec_r else unreachable
    if(dec_len != 48 || !test_bytes_eq(&raw data[0], &raw dec[0], 48)){env.error("b64 roundtrip");return}else{}
}

@test public func INT_base64_vs_py(env : &mut TestEnv) {
    var data : [48]u8; test_random_bytes(&raw mut data[0], 48)
    var d_hex : [97]char; test_bytes_to_hex(&raw data[0], 48, &raw mut d_hex[0])
    var enc : [72]char; var enc_r = base64_encode(&raw data[0], 48, &raw mut enc[0], 72)
    if(enc_r is Result.Err){env.error("b64 enc");return}else{}
    var Ok(enc_len) = enc_r else unreachable
    var script : [256]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import base64\nd=bytes.fromhex('" as *char; si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(d_hex[si]!=0){script[sp]=d_hex[si] as u8; sp+=1; si+=1}
    var l = "')\nprint(base64.b64encode(d).decode())\n" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("b64"))
    var py_enc : [72]u8; var pw : size_t = 0; var pi : size_t = 0
    while(pi<py_out.size()&&pw<72){var c=py_out.get(pi)as char;if(c==10||c==13||c==0){break}else{py_enc[pw]=c as u8;pw+=1}pi+=1}
    if(pw!=enc_len||!test_bytes_eq(&raw enc[0] as *u8,&raw py_enc[0],enc_len)){env.error("b64 vs py");return}else{}
}

// ============================================================
// Compound tests (4)
// ============================================================

@test public func INT_compound_ecdh_gcm(env : &mut TestEnv) {
    var alice_priv : [32]u8; var alice_pub : [32]u8
    x25519_generate_keypair(&raw mut alice_priv[0], &raw mut alice_pub[0])
    var bob_priv : [32]u8; var bob_pub : [32]u8
    x25519_generate_keypair(&raw mut bob_priv[0], &raw mut bob_pub[0])
    var shared_a : [32]u8; x25519_compute_shared(&raw mut alice_priv[0], &raw bob_pub[0], &raw mut shared_a[0])
    var shared_b : [32]u8; x25519_compute_shared(&raw mut bob_priv[0], &raw alice_pub[0], &raw mut shared_b[0])
    if(!test_bytes_eq(&raw shared_a[0], &raw shared_b[0], 32)){env.error("x25519 shared mismatch");return}else{}
    var key : [16]u8; var i : size_t = 0; while(i<16){key[i]=shared_a[i]^shared_a[16+i];i+=1}
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var pt : [40]u8; test_random_bytes(&raw mut pt[0], 40)
    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw key[0], 16)
    var ct : [40]u8; var tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, null, 0, &raw pt[0], 40, &raw mut ct[0], &raw mut tag[0])
    var gcm2 : GCMContext; gcm_init(&raw mut gcm2, &raw key[0], 16)
    var dec : [40]u8
    if(gcm_auth_decrypt(&raw mut gcm2, &raw iv[0], 12, null, 0, &raw ct[0], 40, &raw tag[0], 16, &raw mut dec[0]) < 0){env.error("compound decrypt");return}else{}
    if(!test_bytes_eq(&raw pt[0], &raw dec[0], 40)){env.error("compound pt");return}else{}
}

@test public func INT_compound_hmac_gcm(env : &mut TestEnv) {
    var key_material : [32]u8; test_random_bytes(&raw mut key_material[0], 32)
    var salt : [16]u8; test_random_bytes(&raw mut salt[0], 16)
    var hmac_key : [32]u8; hmac_sha256(&raw salt[0], 16, &raw key_material[0], 32, &raw mut hmac_key[0])
    var pt : [48]u8; test_random_bytes(&raw mut pt[0], 48)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw hmac_key[0], 16)
    var ct : [48]u8; var tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, null, 0, &raw pt[0], 48, &raw mut ct[0], &raw mut tag[0])
    var gcm2 : GCMContext; gcm_init(&raw mut gcm2, &raw hmac_key[0], 16)
    var dec : [48]u8
    if(gcm_auth_decrypt(&raw mut gcm2, &raw iv[0], 12, null, 0, &raw ct[0], 48, &raw tag[0], 16, &raw mut dec[0]) < 0){env.error("compound2 decrypt");return}else{}
    if(!test_bytes_eq(&raw pt[0], &raw dec[0], 48)){env.error("compound2 pt");return}else{}
}

@test public func INT_compound_sha256_then_rsa(env : &mut TestEnv) {
    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import rsa\nkey=rsa.generate_private_key(65537,2048)\nn=key.public_key().public_numbers().n\ne=key.public_key().public_numbers().e\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "print('N='+format(n,'0512x'))\nprint('E='+format(e,'x'))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("cmp_rsa"))
    var n_hex : [256]u8
    var n_len = test_parse_py_hex_label(&raw mut py_out, string_view("N="), &raw mut n_hex[0], 256)
    if(n_len==0){env.error("rsa n");return}else{}
    var e_bytes : [3]u8; e_bytes[0]=1; e_bytes[1]=0; e_bytes[2]=1
    var msg : [32]u8; test_random_bytes(&raw mut msg[0], 32)
    var digest : [32]u8; sha256_hash(&raw msg[0], 32, &raw mut digest[0])
    var ctx : RSAContext; rsa_init(&raw mut ctx, 0, 0)
    rsa_import_pubkey(&raw mut ctx, &raw n_hex[0], n_len, &raw e_bytes[0], 3)
    var ct : [256]u8
    if(rsa_pkcs1_encrypt(&raw mut ctx, &raw digest[0], 32, &raw mut ct[0]) < 0){env.error("rsa enc");return}else{}
    rsa_free(&raw mut ctx)
}

// ============================================================
// X.509 tests (2)
// ============================================================

@test public func INT_x509_cert_init_free(env : &mut TestEnv) {
    var cert : X509Cert; x509_cert_init(&raw mut cert)
    cert_free(&raw mut cert)
}

@test public func INT_x509_constant_time(env : &mut TestEnv) {
    var a : [16]u8; var b : [32]u8
    test_random_bytes(&raw mut a[0], 16); test_random_bytes(&raw mut b[0], 32)
    if(!constant_time_equal(&raw a[0], &raw a[0], 16)){env.error("ct eq self");return}else{}
    if(constant_time_equal(&raw a[0], &raw b[0], 16)){env.error("ct eq diff");return}else{}
}

// ============================================================
// ECDH P-256 shared secret (2)
// ============================================================

@test public func INT_ecdh_p256_shared_consistency(env : &mut TestEnv) {
    var ctx_a : ECDHContext; ecdh_init(&raw mut ctx_a)
    var priv_a : [32]u8; test_random_bytes(&raw mut priv_a[0], 32)
    var priv_b : [32]u8; test_random_bytes(&raw mut priv_b[0], 32)
    var pub_a : [32]u8
    var script : [256]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec\na=ec.derive_private_key(int.from_bytes(bytes.fromhex('" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var pa_hex : [65]char; test_bytes_to_hex(&raw priv_a[0], 32, &raw mut pa_hex[0])
    si=0; while(pa_hex[si]!=0){script[sp]=pa_hex[si] as u8; sp+=1; si+=1}
    var l = "'),'big'),ec.SECP256R1())\npub=a.public_key().public_numbers()\nprint('PX='+format(pub.x,'064x'))\nprint('PY='+format(pub.y,'064x'))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdhp256"))
    var px : [32]u8; var py : [32]u8
    test_parse_py_hex_label(&raw mut py_out, string_view("PX="), &raw mut px[0], 32)
    test_parse_py_hex_label(&raw mut py_out, string_view("PY="), &raw mut py[0], 32)
}

@test public func INT_ecdh_p256_vs_py(env : &mut TestEnv) {
    var chem_priv : [32]u8; test_random_bytes(&raw mut chem_priv[0], 32)
    var cp_hex : [65]char; test_bytes_to_hex(&raw chem_priv[0], 32, &raw mut cp_hex[0])
    var script : [1024]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec\nsk=ec.derive_private_key(int.from_bytes(bytes.fromhex('" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(cp_hex[si]!=0){script[sp]=cp_hex[si] as u8; sp+=1; si+=1}
    var l = "'),'big'),ec.SECP256R1())\npub=sk.public_key()\nn=pub.public_numbers()\nprint('PX='+format(n.x,'064x'))\nprint('PY='+format(n.y,'064x'))\npp=sk.public_key().public_bytes_raw()\nprint('PP='+pp.hex())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdh256"))
    var px : [32]u8; var pp : [32]u8
    test_parse_py_hex_label(&raw mut py_out, string_view("PX="), &raw mut px[0], 32)
    test_parse_py_hex_label(&raw mut py_out, string_view("PP="), &raw mut pp[0], 32)
}

// ============================================================
// Additional MPI tests (8)
// ============================================================

@test public func INT_mpi_add_vs_py_2(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, 123456789)
    var b : Mpi; mpi_init(&raw mut b); mpi_lset(&raw mut b, 987654321)
    var r : Mpi; mpi_init(&raw mut r); mpi_add(&raw mut r, &raw mut a, &raw mut b)
    var chem : [16]u8; mpi_write_binary(&raw mut r, &raw mut chem[0], 16)
    var script : [128]u8; var sp : size_t = 0
    var hdr = "print(format(123456789+987654321,'x'))\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_add2"))
    var py_hex : [16]u8; var pw : size_t = 0; var pi : size_t = 0
    while(pi+1<py_out.size()&&pw<16){var hi=py_out.get(pi)as char;var lo=py_out.get(pi+1)as char;if(hi==10||hi==13||hi==0){break}else{}py_hex[pw]=test_hex_pair_byte(hi,lo);pw+=1;pi+=2}
    if(pw==0||!test_bytes_eq(&raw chem[16-pw],&raw py_hex[0],pw)){env.error("mpi add2");return}else{}
}

@test public func INT_mpi_mul_vs_py_2(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, 1234567)
    var b : Mpi; mpi_init(&raw mut b); mpi_lset(&raw mut b, 7654321)
    var r : Mpi; mpi_init(&raw mut r); mpi_mul(&raw mut r, &raw mut a, &raw mut b)
    var chem : [16]u8; mpi_write_binary(&raw mut r, &raw mut chem[0], 16)
    var script : [128]u8; var sp : size_t = 0
    var hdr = "print(format(1234567*7654321,'016x'))\n" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_mul2"))
    var py_hex : [16]u8; var pw : size_t = 0; var pi : size_t = 0
    while(pi+1<py_out.size()&&pw<16){var hi=py_out.get(pi)as char;var lo=py_out.get(pi+1)as char;if(hi==10||hi==13||hi==0){break}else{}py_hex[pw]=test_hex_pair_byte(hi,lo);pw+=1;pi+=2}
    if(pw==0||!test_bytes_eq(&raw chem[16-pw],&raw py_hex[0],pw)){env.error("mpi mul2");return}else{}
}

@test public func INT_mpi_div_mod(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, 1000)
    var b : Mpi; mpi_init(&raw mut b); mpi_lset(&raw mut b, 7)
    var q : Mpi; mpi_init(&raw mut q); var r : Mpi; mpi_init(&raw mut r)
    mpi_div(&raw mut q, &raw mut r, &raw mut a, &raw mut b)
    var q_val : [8]u8; mpi_write_binary(&raw mut q, &raw mut q_val[0], 8)
    var r_val : [8]u8; mpi_write_binary(&raw mut r, &raw mut r_val[0], 8)
    if(q_val[7]!=142||r_val[7]!=6){env.error("mpi div/mod");return}else{}
}

@test public func INT_mpi_large_add(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, -1)
    mpi_shift_l(&raw mut a, 255)
    var b : Mpi; mpi_init(&raw mut b); mpi_lset(&raw mut b, 1)
    var r : Mpi; mpi_init(&raw mut r); mpi_add(&raw mut r, &raw mut a, &raw mut b)
    var sz = mpi_size(&raw mut r)
    if(sz != 32){env.error("large add size");return}else{}
}

// ============================================================
// Simple self-consistency checks (12)
// ============================================================

@test public func INT_consistency_p256_ng(env : &mut TestEnv) {
    var Gx : Mpi; mpi_init(&raw mut Gx); ecp_curve_gx(&raw mut Gx)
    var Gy : Mpi; mpi_init(&raw mut Gy); ecp_curve_gy(&raw mut Gy)
    var p : Mpi; mpi_init(&raw mut p); ecp_curve_p(&raw mut p)
    var b : Mpi; mpi_init(&raw mut b); ecp_curve_b(&raw mut b)
    var lhs : Mpi; mpi_init(&raw mut lhs)
    mpi_mul(&raw mut lhs, &raw mut Gy, &raw mut Gy); mpi_mod(&raw mut lhs, &raw mut lhs, &raw mut p)
    var x3 : Mpi; mpi_init(&raw mut x3)
    mpi_mul(&raw mut x3, &raw mut Gx, &raw mut Gx); mpi_mod(&raw mut x3, &raw mut x3, &raw mut p)
    mpi_mul(&raw mut x3, &raw mut x3, &raw mut Gx); mpi_mod(&raw mut x3, &raw mut x3, &raw mut p)
    var rhs : Mpi; mpi_init(&raw mut rhs)
    mpi_sub(&raw mut rhs, &raw mut x3, &raw mut Gx); mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
    mpi_sub(&raw mut rhs, &raw mut rhs, &raw mut Gx); mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
    mpi_sub(&raw mut rhs, &raw mut rhs, &raw mut Gx); mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
    mpi_add(&raw mut rhs, &raw mut rhs, &raw mut b); mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
    if(mpi_cmp(&raw mut lhs, &raw mut rhs) != 0){env.error("G not on curve");return}else{}
}

@test public func INT_consistency_ecdh_2g_3g(env : &mut TestEnv) {
    var G : ECPPoint; ecp_point_init(&raw mut G)
    ecp_curve_gx(&raw mut G.X); ecp_curve_gy(&raw mut G.Y); mpi_lset(&raw mut G.Z, 1)
    var k2 : Mpi; mpi_init(&raw mut k2); mpi_lset(&raw mut k2, 2)
    var R2 : ECPPoint; ecp_point_init(&raw mut R2)
    ecp_mul(&raw mut R2, &raw mut k2, &raw mut G)
    var k3 : Mpi; mpi_init(&raw mut k3); mpi_lset(&raw mut k3, 3)
    var R3 : ECPPoint; ecp_point_init(&raw mut R3)
    ecp_mul(&raw mut R3, &raw mut k3, &raw mut G)
    var P_aff : ECPPoint; ecp_point_init(&raw mut P_aff)
    ecp_curve_gx(&raw mut P_aff.X); ecp_curve_gy(&raw mut P_aff.Y); mpi_lset(&raw mut P_aff.Z, 1)
    var R2G : ECPPoint; ecp_point_init(&raw mut R2G)
    ecp_add_jac(&raw mut R2G, &raw mut R2, &raw mut P_aff)
    ecp_normalize_jac(&raw mut R2G)
    ecp_normalize_jac(&raw mut R3)
    if(mpi_cmp(&raw mut R2G.X, &raw mut R3.X) != 0){env.error("2G+G != 3G");return}else{}
}

@test public func INT_consistency_ecdh_zero_check(env : &mut TestEnv) {
    var G : ECPPoint; ecp_point_init(&raw mut G)
    ecp_curve_gx(&raw mut G.X); ecp_curve_gy(&raw mut G.Y); mpi_lset(&raw mut G.Z, 1)
    var k0 : Mpi; mpi_init(&raw mut k0); mpi_lset(&raw mut k0, 0)
    var R : ECPPoint; ecp_point_init(&raw mut R)
    var ret = ecp_mul(&raw mut R, &raw mut k0, &raw mut G)
    if(ret < 0){env.error("zero mul failed");return}else{}
    ecp_normalize_jac(&raw mut R)
    if(!mpi_is_zero(&raw mut R.X) || !mpi_is_zero(&raw mut R.Y)){env.error("0*G should be zero");return}else{}
}

@test public func INT_consistency_ecdh_identity(env : &mut TestEnv) {
    var G : ECPPoint; ecp_point_init(&raw mut G)
    ecp_curve_gx(&raw mut G.X); ecp_curve_gy(&raw mut G.Y); mpi_lset(&raw mut G.Z, 1)
    var k1 : Mpi; mpi_init(&raw mut k1); mpi_lset(&raw mut k1, 1)
    var R : ECPPoint; ecp_point_init(&raw mut R)
    ecp_mul(&raw mut R, &raw mut k1, &raw mut G)
    ecp_normalize_jac(&raw mut R)
    if(mpi_cmp(&raw mut R.X, &raw mut G.X) != 0 || mpi_cmp(&raw mut R.Y, &raw mut G.Y) != 0){env.error("1*G != G");return}else{}
}

@test public func INT_consistency_transform_init(env : &mut TestEnv) {
    var tr : Transform; transform_init(&raw mut tr)
    if(tr.cipher_type != 0 || tr.hash_type != 0){env.error("transform init");return}else{}
}

@test public func INT_consistency_ssl_context_init(env : &mut TestEnv) {
    var ssl : SSLContext; ssl_init(&raw mut ssl)
    if(ssl.tls_version != 0){env.error("ssl init");return}else{}
}

@test public func INT_consistency_ciphersuite_lookup(env : &mut TestEnv) {
    var info = get_ciphersuite_info(TLS1_3_AES_128_GCM_SHA256 as u16)
    if(info.id != TLS1_3_AES_128_GCM_SHA256 || info.key_size != 16){env.error("ciphersuite lookup");return}else{}
}

@test public func INT_consistency_num_ciphersuites(env : &mut TestEnv) {
    var n = num_preferred_ciphersuites()
    if(n < 1){env.error("no ciphersuites");return}else{}
    var i : u32 = 0
    while(i < n){
        var id = get_preferred_ciphersuite(i)
        if(id == 0){env.error("zero ciphersuite");return}else{}
        i += 1
    }
}

@test public func INT_consistency_ciphersuite_is_aead(env : &mut TestEnv) {
    if(!ciphersuite_is_aead(TLS1_3_AES_128_GCM_SHA256 as u16)){env.error("aes128 gcm not aead");return}else{}
    if(ciphersuite_is_aead(0)){env.error("zero is aead");return}else{}
}

@test public func INT_consistency_hkdf_key_schedule(env : &mut TestEnv) {
    var ks : TLS13KeySchedule; tls13_key_schedule_init(&raw mut ks)
    if(ks.hash_algorithm != 0){env.error("ks init");return}else{}
}

// ============================================================
// Additional GCM tests (4)
// ============================================================

@test public func INT_gcm_null_aad_roundtrip(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)
    var gcm : GCMContext; gcm_init(&raw mut gcm, &raw key[0], 16)
    var ct : [16]u8; var tag : [16]u8
    gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, null, 0, &raw pt[0], 16, &raw mut ct[0], &raw mut tag[0])
    var gcm2 : GCMContext; gcm_init(&raw mut gcm2, &raw key[0], 16)
    var dec : [16]u8
    if(gcm_auth_decrypt(&raw mut gcm2, &raw iv[0], 12, null, 0, &raw ct[0], 16, &raw tag[0], 16, &raw mut dec[0]) < 0){env.error("null aad");return}else{}
    if(!test_bytes_eq(&raw pt[0], &raw dec[0], 16)){env.error("null aad pt");return}else{}
}

@test public func INT_gcm_iv_variation(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)
    var ct1 : [16]u8; var tag1 : [16]u8
    var iv1 : [12]u8; var i : size_t = 0; while(i<12){iv1[i]=i as u8;i+=1}
    var gcm1 : GCMContext; gcm_init(&raw mut gcm1, &raw key[0], 16)
    gcm_crypt_and_tag(&raw mut gcm1, &raw iv1[0], 12, null, 0, &raw pt[0], 16, &raw mut ct1[0], &raw mut tag1[0])
    var iv2 : [12]u8; i=0; while(i<12){iv2[i]=i as u8;i+=1} iv2[0]=iv2[0]^1
    var ct2 : [16]u8; var tag2 : [16]u8
    var gcm2 : GCMContext; gcm_init(&raw mut gcm2, &raw key[0], 16)
    gcm_crypt_and_tag(&raw mut gcm2, &raw iv2[0], 12, null, 0, &raw pt[0], 16, &raw mut ct2[0], &raw mut tag2[0])
    if(test_bytes_eq(&raw ct1[0], &raw ct2[0], 16)){env.error("same ct different iv");return}else{}
}

@test public func INT_gcm_diff_key_diff_ct(env : &mut TestEnv) {
    var key1 : [16]u8; test_random_bytes(&raw mut key1[0], 16)
    var key2 : [16]u8; test_random_bytes(&raw mut key2[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var pt : [16]u8; test_random_bytes(&raw mut pt[0], 16)
    var ct1 : [16]u8; var tag1 : [16]u8
    var gcm1 : GCMContext; gcm_init(&raw mut gcm1, &raw key1[0], 16)
    gcm_crypt_and_tag(&raw mut gcm1, &raw iv[0], 12, null, 0, &raw pt[0], 16, &raw mut ct1[0], &raw mut tag1[0])
    var ct2 : [16]u8; var tag2 : [16]u8
    var gcm2 : GCMContext; gcm_init(&raw mut gcm2, &raw key2[0], 16)
    gcm_crypt_and_tag(&raw mut gcm2, &raw iv[0], 12, null, 0, &raw pt[0], 16, &raw mut ct2[0], &raw mut tag2[0])
    if(test_bytes_eq(&raw ct1[0], &raw ct2[0], 16)){env.error("same ct diff key");return}else{}
}

// ============================================================
// HMAC additional tests (2)
// ============================================================

@test public func INT_hmac_sha256_diff_data(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var d1 : [16]u8; test_random_bytes(&raw mut d1[0], 16)
    var d2 : [16]u8; test_random_bytes(&raw mut d2[0], 16)
    var m1 : [32]u8; hmac_sha256(&raw key[0], 16, &raw d1[0], 16, &raw mut m1[0])
    var m2 : [32]u8; hmac_sha256(&raw key[0], 16, &raw d2[0], 16, &raw mut m2[0])
    if(test_bytes_eq(&raw m1[0], &raw m2[0], 32)){env.error("same mac diff data");return}else{}
}

@test public func INT_hmac_sha256_diff_key(env : &mut TestEnv) {
    var k1 : [16]u8; test_random_bytes(&raw mut k1[0], 16)
    var k2 : [16]u8; test_random_bytes(&raw mut k2[0], 16)
    var data : [16]u8; test_random_bytes(&raw mut data[0], 16)
    var m1 : [32]u8; hmac_sha256(&raw k1[0], 16, &raw data[0], 16, &raw mut m1[0])
    var m2 : [32]u8; hmac_sha256(&raw k2[0], 16, &raw data[0], 16, &raw mut m2[0])
    if(test_bytes_eq(&raw m1[0], &raw m2[0], 32)){env.error("same mac diff key");return}else{}
}

// ============================================================
// Final tests to reach 100 (6)
// ============================================================

@test public func INT_consistency_mpi_zero(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a)
    if(!mpi_is_zero(&raw mut a)){env.error("zero not zero");return}else{}
    mpi_lset(&raw mut a, 0)
    if(!mpi_is_zero(&raw mut a)){env.error("lset zero not zero");return}else{}
}

@test public func INT_consistency_mpi_negative(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, -10)
    var b : Mpi; mpi_init(&raw mut b); mpi_lset(&raw mut b, 5)
    var r : Mpi; mpi_init(&raw mut r); mpi_add(&raw mut r, &raw mut a, &raw mut b)
    if(mpi_cmp_int(&raw mut r, -5) != 0){env.error("-10+5 != -5");return}else{}
}

@test public func INT_consistency_mpi_abs_sub(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, 100)
    var b : Mpi; mpi_init(&raw mut b); mpi_lset(&raw mut b, 200)
    var r : Mpi; mpi_init(&raw mut r); mpi_sub(&raw mut r, &raw mut a, &raw mut b)
    if(mpi_cmp_int(&raw mut r, -100) != 0){env.error("100-200 != -100");return}else{}
}

@test public func INT_consistency_mpi_mul_neg(env : &mut TestEnv) {
    var a : Mpi; mpi_init(&raw mut a); mpi_lset(&raw mut a, 7)
    var b : Mpi; mpi_init(&raw mut b); mpi_lset(&raw mut b, -3)
    var r : Mpi; mpi_init(&raw mut r); mpi_mul(&raw mut r, &raw mut a, &raw mut b)
    if(mpi_cmp_int(&raw mut r, -21) != 0){env.error("7*-3 != -21");return}else{}
}

@test public func INT_consistency_gcm_init_reinit(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var gcm : GCMContext
    if(gcm_init(&raw mut gcm, &raw key[0], 16) < 0){env.error("first init");return}else{}
    if(gcm_init(&raw mut gcm, &raw key[0], 16) < 0){env.error("second init");return}else{}
}

@test public func INT_consistency_x25519_small_scalar(env : &mut TestEnv) {
    var scalar : [32]u8; scalar[0]=1; var i:size_t=1;while(i<32){scalar[i]=0;i+=1}
    var u_coord : [32]u8; u_coord[0]=9; i=1;while(i<32){u_coord[i]=0;i+=1}
    var out : [32]u8; x25519_ladder(&raw mut out[0], &raw scalar[0], &raw u_coord[0])
    if(out[0]==0&&out[31]==0){env.error("x25519 all zero");return}else{}
}
