using namespace tls
using namespace crypto

func rand_bytes_gcm(buf : *mut u8, len : size_t) {
    var f = fopen("/dev/urandom", "rb\0" as *char)
    if(f == null) { return } else {}
    fread(buf as *mut void, 1 as size_t, len, f)
    fclose(f)
}

@test
public func INT_gcm_encrypt_against_python(env : &mut TestEnv) {
    var key : [16]u8; rand_bytes_gcm(&raw mut key[0], 16)
    var iv : [12]u8; rand_bytes_gcm(&raw mut iv[0], 12)
    var aad : [5]u8; rand_bytes_gcm(&raw mut aad[0], 5)
    var pt : [29]u8; rand_bytes_gcm(&raw mut pt[0], 29)

    var key_hex : [33]char; test_bytes_to_hex(&raw key[0], 16, &raw mut key_hex[0])
    var iv_hex : [25]char; test_bytes_to_hex(&raw iv[0], 12, &raw mut iv_hex[0])
    var aad_hex : [11]char; test_bytes_to_hex(&raw aad[0], 5, &raw mut aad_hex[0])
    var pt_hex : [59]char; test_bytes_to_hex(&raw pt[0], 29, &raw mut pt_hex[0])

    // Build python script
    var script : [1024]char; var sp : size_t = 0
    var hdr = "#!/usr/bin/python3\nfrom cryptography.hazmat.primitives.ciphers.aead import AESGCM\n\0" as *char
    var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si]; sp+=1; si+=1}

    var l = "aesgcm=AESGCM(bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(key_hex[si]!=0){script[sp]=key_hex[si]; sp+=1; si+=1}
    l="'))\nct_tag=aesgcm.encrypt(bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(iv_hex[si]!=0){script[sp]=iv_hex[si]; sp+=1; si+=1}
    l="'),bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(pt_hex[si]!=0){script[sp]=pt_hex[si]; sp+=1; si+=1}
    l="'),bytes.fromhex('\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    si=0; while(aad_hex[si]!=0){script[sp]=aad_hex[si]; sp+=1; si+=1}
    l="'))\nprint('CT='+ct_tag[:29].hex())\nprint('TAG='+ct_tag[29:].hex())\n\0" as *char; si=0
    while(l[si]!=0){script[sp]=l[si]; sp+=1; si+=1}
    script[sp]=0

    test_write_file("/tmp/gcm_py.py", &raw script[0] as *mut u8, sp)
    system("python3 /tmp/gcm_py.py > /tmp/gcm_py_out.txt 2>/dev/null\0" as *char)

    var py_out : [256]char
    test_read_file("/tmp/gcm_py_out.txt", &raw mut py_out[0] as *mut u8, 256)

    // Parse CT= and TAG=
    var py_ct : [64]u8; var py_tag : [16]u8
    var pos : size_t = 0
    while(pos < 250) {
        if(py_out[pos]=='C' as u8 && py_out[pos+1]=='T' as u8 && py_out[pos+2]=='=' as u8) { pos+=3; break } else {}
        pos+=1
    }
    var ci : size_t = 0
    while(ci<29){py_ct[ci]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; ci+=1}
    while(pos < 250) {
        if(py_out[pos]=='T' as u8 && py_out[pos+1]=='A' as u8 && py_out[pos+2]=='G' as u8 && py_out[pos+3]=='=' as u8) { pos+=4; break } else {}
        pos+=1
    }
    ci=0; while(ci<16){py_tag[ci]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; ci+=1}

    // Chemical GCM encrypt
    var gcm : GCMContext
    var ret = gcm_init(&raw mut gcm, &raw key[0], 16)
    if(ret < 0) { env.error("gcm_init failed"); return } else {}
    var chem_ct : [64]u8; var chem_tag : [16]u8
    ret = gcm_crypt_and_tag(&raw mut gcm, &raw iv[0], 12, &raw aad[0], 5, &raw pt[0], 29, &raw mut chem_ct[0], &raw mut chem_tag[0])
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

    // Chemical GCM decrypt of Python's output
    var gcm2 : GCMContext
    gcm_init(&raw mut gcm2, &raw key[0], 16)
    var chem_dec : [64]u8
    ret = gcm_auth_decrypt(&raw mut gcm2, &raw iv[0], 12, &raw aad[0], 5, &raw py_ct[0], 29, &raw py_tag[0], 16, &raw mut chem_dec[0])
    if(ret < 0) { env.error("gcm_auth_decrypt failed"); return } else {}
    if(!test_bytes_eq(&raw chem_dec[0], &raw pt[0], 29)) { env.error("GCM decrypt roundtrip mismatch"); return } else {}
}
