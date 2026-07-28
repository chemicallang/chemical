using namespace tls
using std::string_view

@test
public func INT_x25519_shared_against_python(env : &mut TestEnv) {
    var chem_priv : [32]u8
    var chem_pub : [32]u8
    var ret = x25519_generate_keypair(&raw mut chem_priv[0], &raw mut chem_pub[0])
    if(ret < 0) { env.error("keygen failed"); return } else {}

    var chem_pub_hex : [65]char
    test_bytes_to_hex(&raw chem_pub[0], 32, &raw mut chem_pub_hex[0])

    var script : [1024]u8; var sp : size_t = 0
    var hdr = "import os\nfrom cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey,X25519PublicKey\n" as *char
    var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}

    var l1 = "py_priv=os.urandom(32)\nba=bytearray(py_priv);ba[0]&=0xf8;ba[31]&=0x7f;ba[31]|=0x40;py_priv=bytes(ba)\n" as *char; si=0
    while(l1[si]!=0){script[sp]=l1[si] as u8; sp+=1; si+=1}

    var l2 = "sk=X25519PrivateKey.from_private_bytes(py_priv)\npy_pub=sk.public_key().public_bytes_raw()\n" as *char; si=0
    while(l2[si]!=0){script[sp]=l2[si] as u8; sp+=1; si+=1}

    var l3pre = "shared=sk.exchange(X25519PublicKey.from_public_bytes(bytes.fromhex('" as *char; si=0
    while(l3pre[si]!=0){script[sp]=l3pre[si] as u8; sp+=1; si+=1}
    si=0; while(chem_pub_hex[si]!=0){script[sp]=chem_pub_hex[si] as u8; sp+=1; si+=1}
    var l3post = "')))\n" as *char; si=0
    while(l3post[si]!=0){script[sp]=l3post[si] as u8; sp+=1; si+=1}

    var l4 = "print('PRIV='+py_priv.hex())\nprint('PUB='+py_pub.hex())\nprint('SHARED='+shared.hex())\n" as *char; si=0
    while(l4[si]!=0){script[sp]=l4[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("x25519_py.py"))

    var py_pub : [32]u8; var py_shared : [32]u8
    var pub_len = test_parse_py_hex_label(&raw mut py_out, string_view("PUB="), &raw mut py_pub[0], 32)
    var shared_len = test_parse_py_hex_label(&raw mut py_out, string_view("SHARED="), &raw mut py_shared[0], 32)
    if(pub_len != 32 || shared_len != 32) { env.error("failed to parse Python output"); return } else {}

    var chem_shared : [32]u8
    ret = x25519_compute_shared(&raw chem_priv[0], &raw py_pub[0], &raw mut chem_shared[0])
    if(ret < 0) { env.error("x25519_compute_shared failed"); return } else {}

    if(!test_bytes_eq(&raw chem_shared[0], &raw py_shared[0], 32)) {
        printf("[X25519_TEST] shared[0]: chem=%02x py=%02x\n", chem_shared[0] as int, py_shared[0] as int)
        env.error("x25519 shared secret mismatch against Python")
        return
    } else {}
}
