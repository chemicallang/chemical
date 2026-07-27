using namespace tls

@test
public func INT_x25519_shared_against_python(env : &mut TestEnv) {
    // Generate Chemical keypair
    var chem_priv : [32]u8
    var chem_pub : [32]u8
    var ret = x25519_generate_keypair(&raw mut chem_priv[0], &raw mut chem_pub[0])
    if(ret < 0) { env.error("keygen failed"); return } else {}

    // Python generates its own keypair and computes shared secret
    var chem_pub_hex : [65]char
    test_bytes_to_hex(&raw chem_pub[0], 32, &raw mut chem_pub_hex[0])

    var script : [1024]char; var sp : size_t = 0
    var hdr = "#!/usr/bin/python3\n\0" as *char
    var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si]; sp+=1; si+=1}

    var imp = "import os\nfrom cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey,X25519PublicKey\n\0" as *char; si=0
    while(imp[si]!=0){script[sp]=imp[si]; sp+=1; si+=1}

    var l1 = "py_priv=os.urandom(32)\nba=bytearray(py_priv);ba[0]&=0xf8;ba[31]&=0x7f;ba[31]|=0x40;py_priv=bytes(ba)\n\0" as *char; si=0
    while(l1[si]!=0){script[sp]=l1[si]; sp+=1; si+=1}

    var l2 = "sk=X25519PrivateKey.from_private_bytes(py_priv)\npy_pub=sk.public_key().public_bytes_raw()\n\0" as *char; si=0
    while(l2[si]!=0){script[sp]=l2[si]; sp+=1; si+=1}

    var l3pre = "shared=sk.exchange(X25519PublicKey.from_public_bytes(bytes.fromhex('\0" as *char; si=0
    while(l3pre[si]!=0){script[sp]=l3pre[si]; sp+=1; si+=1}
    si=0; while(chem_pub_hex[si]!=0){script[sp]=chem_pub_hex[si]; sp+=1; si+=1}
    var l3post = "')))\n\0" as *char; si=0
    while(l3post[si]!=0){script[sp]=l3post[si]; sp+=1; si+=1}

    var l4 = "print('PRIV='+py_priv.hex())\nprint('PUB='+py_pub.hex())\nprint('SHARED='+shared.hex())\n\0" as *char; si=0
    while(l4[si]!=0){script[sp]=l4[si]; sp+=1; si+=1}
    script[sp]=0

    test_write_file("/tmp/x25519_py.py", &raw script[0] as *mut u8, sp)
    system("python3 /tmp/x25519_py.py > /tmp/x25519_py_out.txt 2>/dev/null\0" as *char)

    // Parse output
    var py_out : [512]char
    test_read_file("/tmp/x25519_py_out.txt", &raw mut py_out[0] as *mut u8, 512)

    var py_shared : [32]u8; var py_pub : [32]u8
    var pos : size_t = 0

    // Find PUB=
    while(pos < 500) {
        if(py_out[pos]=='P' as u8 && py_out[pos+1]=='U' as u8 && py_out[pos+2]=='B' as u8 && py_out[pos+3]=='=' as u8) { pos+=4; break } else {}
        pos+=1
    }
    var i : size_t = 0
    while(i<32) { py_pub[i]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; i+=1 }

    // Find SHARED=
    while(pos < 500) {
        if(py_out[pos]=='S' as u8 && py_out[pos+1]=='H' as u8 && py_out[pos+2]=='A' as u8 && py_out[pos+3]=='R' as u8 && py_out[pos+4]=='E' as u8 && py_out[pos+5]=='D' as u8 && py_out[pos+6]=='=' as u8) { pos+=7; break } else {}
        pos+=1
    }
    i=0; while(i<32) { py_shared[i]=test_hex_pair_byte(py_out[pos],py_out[pos+1]); pos+=2; i+=1 }

    // Compute Chemical shared: chem_priv * py_pub
    var chem_shared : [32]u8
    ret = x25519_compute_shared(&raw chem_priv[0], &raw py_pub[0], &raw mut chem_shared[0])
    if(ret < 0) { env.error("x25519_compute_shared failed"); return } else {}

    if(!test_bytes_eq(&raw chem_shared[0], &raw py_shared[0], 32)) {
        printf("[X25519_TEST] shared[0]: chem=%02x py=%02x\n", chem_shared[0] as int, py_shared[0] as int)
        env.error("x25519 shared secret mismatch against Python")
        return
    } else {}
}
