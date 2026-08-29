using namespace tls
using namespace crypto
using std::string_view

@test
public func INT_gcm_encrypt_against_python(env : &mut TestEnv) {
    var key : [16]u8; test_random_bytes(&raw mut key[0], 16)
    var iv : [12]u8; test_random_bytes(&raw mut iv[0], 12)
    var aad : [5]u8; test_random_bytes(&raw mut aad[0], 5)
    var pt : [29]u8; test_random_bytes(&raw mut pt[0], 29)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [11]char; test_bytes_to_hex(&raw aad[0], 5, &raw mut aad_hex[0])
    var pt_hex : [59]char; test_bytes_to_hex(&raw pt[0], 29, &raw mut pt_hex[0])

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
    l = "print('CT='+ct_tag[:29].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1
    l = "print('TAG='+ct_tag[29:].hex())" as *char; si=0; while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    script[sp]=10; sp+=1

    var py_out = test_python_run_script(&raw script[0], sp, string_view("gcm_py.py"))

    var py_ct : [29]u8; var py_tag : [16]u8
    var ct_len = test_parse_py_hex_label(&raw mut py_out, string_view("CT="), &raw mut py_ct[0], 29)
    var tag_len = test_parse_py_hex_label(&raw mut py_out, string_view("TAG="), &raw mut py_tag[0], 16)
    if(ct_len != 29 || tag_len != 16) { env.error("failed to parse Python output"); return } else {}

    var gcm : GCMContext
    var ret = gcm_init(unsafe(&raw mut gcm), &raw key[0], 16)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var chem_ct : [64]u8; var chem_tag : [16]u8
    ret = gcm_crypt_and_tag(unsafe(&raw mut gcm), &raw iv[0], 12, &raw aad[0], 5, &raw pt[0], 29, &raw mut chem_ct[0], &raw mut chem_tag[0])
    if(ret < 0) { env.error("gcm_crypt_and_tag failed"); return } else {}

    if(!test_bytes_eq(&raw chem_ct[0], &raw py_ct[0], 29)) {
        printf("[GCM_TEST] ct mismatch: chem[0]=%02x py[0]=%02x\n", chem_ct[0] as int, py_ct[0] as int)
        env.error("GCM ciphertext mismatch")
        return
    } else {}
    if(!test_bytes_eq(&raw chem_tag[0], &raw py_tag[0], 16)) {
        printf("[GCM_TEST] tag mismatch\n")
        env.error("GCM tag mismatch")
        return
    } else {}

    var gcm2 : GCMContext
    gcm_init(unsafe(&raw mut gcm2), &raw key[0], 16)
    var chem_dec : [64]u8
    ret = gcm_auth_decrypt(unsafe(&raw mut gcm2), &raw iv[0], 12, &raw aad[0], 5, &raw py_ct[0], 29, &raw py_tag[0], 16, &raw mut chem_dec[0])
    if(ret < 0) { env.error("gcm_auth_decrypt failed"); return } else {}
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 29)) { env.error("GCM decrypt roundtrip mismatch"); return } else {}
}
