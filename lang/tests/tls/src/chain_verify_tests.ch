// Regression tests for this session's TLS fixes:
//   1. MAX_LIMBS 256→512 (bignum.ch) — RSA keys up to 4096-bit now verify.
//      The old limit failed on the shift-add path of mpi_mul, which needs
//      2n+1 = 257 limbs for a 128-limb (4096-bit) operand.
//   2. x509_verify_chain rework (ssl.ch) — multi-level chains
//      (leaf -> intermediate -> trusted root) verified by building the chain
//      via subject/issuer DN matching instead of only leaf-vs-CA.
//   3. x509_crt_load_pem_file / x509_parse_server_cert_chain — full
//      multi-cert PEM bundles and handshake cert_list chains are parsed.
//   4. cert_chain_free (x509_crt.ch) — walk-to-and-fro chain cleanup.

using namespace tls
using std::string_view
using std::string

// ─── 4096-bit bignum multiply vs Python ─────────────────────────────
// Direct regression for MAX_LIMBS: two 128-limb (4096-bit) operands need
// 2*128+1 = 257 limbs in the shift-add path of mpi_mul.

@test
public func INT_mpi_mul_4096_vs_python(env : &mut TestEnv) {
    var a : [512]u8; test_random_bytes(&raw mut a[0], 512)
    var b : [512]u8; test_random_bytes(&raw mut b[0], 512)
    a[0] = a[0] | 0x80     // force the top bit so it's a real 4096-bit value
    b[0] = b[0] | 0x80

    var a_hex : [1025]char; test_bytes_to_hex(&raw a[0], 512, &raw mut a_hex[0])
    var b_hex : [1025]char; test_bytes_to_hex(&raw b[0], 512, &raw mut b_hex[0])

    var script : [4096]u8; var sp : size_t = 0
    var hdr = "a=int('" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(a_hex[si]!=0){script[sp]=a_hex[si] as u8; sp+=1; si+=1}
    var l = "',16)\nb=int('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(b_hex[si]!=0){script[sp]=b_hex[si] as u8; sp+=1; si+=1}
    l = "',16)\nprint('MUL='+format(a*b,'2048x'))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_mul4096.py"))
    var py_mul : [1024]u8
    var mul_len = test_parse_py_hex_label(&raw mut py_out, string_view("MUL="), &raw mut py_mul[0], 1024)
    if(mul_len != 1024) { env.error("failed to parse Python MUL output"); return }

    var ma : Mpi; mpi_init(unsafe(&raw mut ma))
    var mb : Mpi; mpi_init(unsafe(&raw mut mb))
    var mx : Mpi; mpi_init(unsafe(&raw mut mx))
    var ret = mpi_read_binary(unsafe(&raw mut ma), &raw a[0], 512)
    if(ret < 0) { env.error("mpi_read_binary failed"); return }
    ret = mpi_read_binary(unsafe(&raw mut mb), &raw b[0], 512)
    if(ret < 0) { env.error("mpi_read_binary(2) failed"); return }
    ret = mpi_mul(unsafe(&raw mut mx), unsafe(&raw mut ma), unsafe(&raw mut mb))
    if(ret < 0) { env.error("mpi_mul failed on 4096-bit operands"); return }

    var chem_mul : [1024]u8
    ret = mpi_write_binary(unsafe(&raw mut mx), &raw mut chem_mul[0], 1024)
    if(ret < 0) { env.error("mpi_write_binary failed"); return }
    if(!test_bytes_eq(&raw chem_mul[0], &raw py_mul[0], 1024)) { env.error("mpi_mul 4096 mismatch vs Python"); return }
}

// ─── 4096-bit modular exponentiation vs Python ──────────────────────
// The RSA signature verify path (WR2 → GTS Root R1) computes S^E mod N
// where N is a 4096-bit modulus. compute_r2 inside mpi_exp_mod calls
// mpi_mul on 128-limb values — the exact path that used to overflow.

@test
public func INT_mpi_exp_mod_4096_vs_python(env : &mut TestEnv) {
    var n : [512]u8; test_random_bytes(&raw mut n[0], 512)
    var a : [512]u8; test_random_bytes(&raw mut a[0], 512)
    n[0] = n[0] | 0x80     // 4096-bit modulus
    n[511] = n[511] | 0x01 // make it odd (valid RSA modulus)
    a[0] = a[0] & 0x7F     // base < modulus

    var n_hex : [1025]char; test_bytes_to_hex(&raw n[0], 512, &raw mut n_hex[0])
    var a_hex : [1025]char; test_bytes_to_hex(&raw a[0], 512, &raw mut a_hex[0])

    var script : [4096]u8; var sp : size_t = 0
    var hdr = "n=int('" as *char; var si : size_t = 0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(n_hex[si]!=0){script[sp]=n_hex[si] as u8; sp+=1; si+=1}
    var l = "',16)\na=int('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    si=0; while(a_hex[si]!=0){script[sp]=a_hex[si] as u8; sp+=1; si+=1}
    l = "',16)\nprint('R='+format(pow(a,65537,n),'1024x'))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_expmod4096.py"))
    var py_r : [512]u8
    var r_len = test_parse_py_hex_label(&raw mut py_out, string_view("R="), &raw mut py_r[0], 512)
    if(r_len != 512) { env.error("failed to parse Python R output"); return }

    var mn : Mpi; mpi_init(unsafe(&raw mut mn))
    var ma : Mpi; mpi_init(unsafe(&raw mut ma))
    var me : Mpi; mpi_init(unsafe(&raw mut me))
    var mr : Mpi; mpi_init(unsafe(&raw mut mr))
    var ret = mpi_read_binary(unsafe(&raw mut mn), &raw n[0], 512)
    if(ret < 0) { env.error("mpi_read_binary(n) failed"); return }
    ret = mpi_read_binary(unsafe(&raw mut ma), &raw a[0], 512)
    if(ret < 0) { env.error("mpi_read_binary(a) failed"); return }
    mpi_lset(unsafe(&raw mut me), 65537)
    ret = mpi_exp_mod(unsafe(&raw mut mr), unsafe(&raw mut ma), unsafe(&raw mut me), unsafe(&raw mut mn))
    if(ret < 0) { env.error("mpi_exp_mod failed on 4096-bit modulus"); return }

    var chem_r : [512]u8
    ret = mpi_write_binary(unsafe(&raw mut mr), &raw mut chem_r[0], 512)
    if(ret < 0) { env.error("mpi_write_binary failed"); return }
    if(!test_bytes_eq(&raw chem_r[0], &raw py_r[0], 512)) { env.error("mpi_exp_mod 4096 mismatch vs Python"); return }
}

// ─── 4096-bit RSA self-signed cert signature verify vs Python ───────
// Python signs a cert with a 4096-bit RSA key; Chemical must verify the
// signature. This is the WR2 (signed by GTS Root R1) overflow scenario.

@test
public func INT_rsa4096_cert_signature_vs_python(env : &mut TestEnv) {
    var cert_path = test_tmp_file(string_view("chem_rsa4096_cert.der"))
    var script : [2048]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import datetime\nfrom cryptography import x509\nfrom cryptography.x509.oid import NameOID\nfrom cryptography.hazmat.primitives import hashes\nfrom cryptography.hazmat.primitives.asymmetric import rsa\nfrom cryptography.hazmat.primitives.serialization import Encoding\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "key=rsa.generate_private_key(65537,4096)\npub=key.public_key()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "now=datetime.datetime.utcnow()\ncert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'test.example.com')])).issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'Test4096CA')])).public_key(pub).serial_number(1).not_valid_before(now-datetime.timedelta(days=1)).not_valid_after(now+datetime.timedelta(days=365)).sign(key,hashes.SHA256()))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "der=cert.public_bytes(Encoding.DER)\nf=open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, cert_path.to_view())
    l = "','wb');f.write(der);f.close()\nprint('LEN='+str(len(der)))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    test_python_run_script(&raw script[0], sp, string_view("cert_rsa4096.py"))

    var cert_file = fopen(cert_path.data() as *char, "rb\0" as *char)
    if(cert_file == null) { env.error("cannot open cert"); return }
    var cert_buf : [2048]u8
    var cert_len = fread(&raw mut cert_buf[0] as *mut void, 1 as size_t, 2048, cert_file)
    fclose(cert_file)
    if(cert_len < 100) { env.error("cert DER too small"); return }

    var crt : X509Cert; x509_cert_init(unsafe(&raw mut crt))
    var ret = parse_cert_der(unsafe(&raw mut crt), &raw cert_buf[0], cert_len)
    if(ret < 0) { env.error("parse_cert_der failed"); return }
    if(unsafe(crt.pk_type) != PK_RSA) { env.error("expected RSA key type"); return }

    var rsa_ctx : RSAContext; rsa_init(unsafe(&raw mut rsa_ctx), RSA_PKCS_V15, 0)
    ret = x509_extract_rsa_pubkey(unsafe(&raw mut crt), unsafe(&raw mut rsa_ctx))
    if(ret < 0) { env.error("x509_extract_rsa_pubkey failed"); return }
    if(rsa_get_len(unsafe(&raw mut rsa_ctx)) != 512) { env.error("RSA modulus len should be 512 bytes"); return }
    if(unsafe(crt.pk_bitlen) != 4096) { env.error("pk_bitlen should report 4096"); return }

    ret = x509_verify_cert_signature(unsafe(&raw mut crt), unsafe(&raw mut rsa_ctx))
    if(ret < 0) { env.error("4096-bit RSA cert signature verification failed"); return }
}

// ─── 2048-bit still verifies (guard against over-tightening) ────────
// Ensures the MAX_LIMBS bump didn't break the common 2048-bit case.

@test
public func INT_rsa2048_cert_signature_still_verifies(env : &mut TestEnv) {
    var cert_path = test_tmp_file(string_view("chem_rsa2048_cert.der"))
    var script : [2048]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import datetime\nfrom cryptography import x509\nfrom cryptography.x509.oid import NameOID\nfrom cryptography.hazmat.primitives import hashes\nfrom cryptography.hazmat.primitives.asymmetric import rsa\nfrom cryptography.hazmat.primitives.serialization import Encoding\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "key=rsa.generate_private_key(65537,2048)\npub=key.public_key()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "now=datetime.datetime.utcnow()\ncert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'test.example.com')])).issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'Test2048CA')])).public_key(pub).serial_number(1).not_valid_before(now-datetime.timedelta(days=1)).not_valid_after(now+datetime.timedelta(days=365)).sign(key,hashes.SHA256()))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "der=cert.public_bytes(Encoding.DER)\nf=open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, cert_path.to_view())
    l = "','wb');f.write(der);f.close()\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    test_python_run_script(&raw script[0], sp, string_view("cert_rsa2048.py"))

    var cert_file = fopen(cert_path.data() as *char, "rb\0" as *char)
    if(cert_file == null) { env.error("cannot open cert"); return }
    var cert_buf : [2048]u8
    var cert_len = fread(&raw mut cert_buf[0] as *mut void, 1 as size_t, 2048, cert_file)
    fclose(cert_file)

    var crt : X509Cert; x509_cert_init(unsafe(&raw mut crt))
    var ret = parse_cert_der(unsafe(&raw mut crt), &raw cert_buf[0], cert_len)
    if(ret < 0) { env.error("parse_cert_der failed"); return }

    var rsa_ctx : RSAContext; rsa_init(unsafe(&raw mut rsa_ctx), RSA_PKCS_V15, 0)
    ret = x509_extract_rsa_pubkey(unsafe(&raw mut crt), unsafe(&raw mut rsa_ctx))
    if(ret < 0) { env.error("x509_extract_rsa_pubkey failed"); return }
    ret = x509_verify_cert_signature(unsafe(&raw mut crt), unsafe(&raw mut rsa_ctx))
    if(ret < 0) { env.error("2048-bit RSA cert signature verification failed"); return }
}

// ─── Full chain: leaf -> intermediate (RSA 2048) -> root (RSA 4096) ──
// Python generates a real three-level PKI. Chemical builds the chain by
// linking leaf.next = intermediate and passes the root as trusted CA.
// Exercises the new chain-walk (root match at step 4a, peer intermediate
// at 4b) AND a 4096-bit RSA signature on the root→intermediate link.

func test_write_chain_python() {
    var root_path = test_tmp_file(string_view("chem_chain_root.der"))
    var inter_path = test_tmp_file(string_view("chem_chain_inter.der"))
    var leaf_path = test_tmp_file(string_view("chem_chain_leaf.der"))
    var script : [8192]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import datetime\nfrom cryptography import x509\nfrom cryptography.x509.oid import NameOID\nfrom cryptography.hazmat.primitives import hashes\nfrom cryptography.hazmat.primitives.asymmetric import rsa,ec\nfrom cryptography.hazmat.primitives.serialization import Encoding\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "def nm(cn):return x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,cn)])\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "def caca(b):return b.add_extension(x509.BasicConstraints(ca=True,path_length=None),critical=True)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "def san(b):return b.add_extension(x509.SubjectAlternativeName([x509.DNSName('test.example.com')]),critical=False)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "na=datetime.datetime.utcnow()-datetime.timedelta(days=1)\nnb=datetime.datetime.utcnow()+datetime.timedelta(days=365)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "rk=rsa.generate_private_key(65537,4096)\nroot=caca(x509.CertificateBuilder().subject_name(nm('RootCA')).issuer_name(nm('RootCA')).public_key(rk.public_key()).serial_number(1).not_valid_before(na).not_valid_after(nb)).sign(rk,hashes.SHA256())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "ik=rsa.generate_private_key(65537,2048)\ninter=caca(x509.CertificateBuilder().subject_name(nm('InterCA')).issuer_name(nm('RootCA')).public_key(ik.public_key()).serial_number(2).not_valid_before(na).not_valid_after(nb)).sign(rk,hashes.SHA256())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "lk=ec.generate_private_key(ec.SECP256R1())\nleaf=san(x509.CertificateBuilder().subject_name(nm('test.example.com')).issuer_name(nm('InterCA')).public_key(lk.public_key()).serial_number(3).not_valid_before(na).not_valid_after(nb)).sign(ik,hashes.SHA256())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, root_path.to_view())
    l = "','wb').write(root.public_bytes(Encoding.DER))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, inter_path.to_view())
    l = "','wb').write(inter.public_bytes(Encoding.DER))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, leaf_path.to_view())
    l = "','wb').write(leaf.public_bytes(Encoding.DER))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "print('OK')\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}

    test_python_run_script(&raw script[0], sp, string_view("chain_gen.py"))
}

func test_read_der_file(path : *char, buf : *mut u8, max_len : size_t) : size_t {
    var f = fopen(path, "rb\0" as *char)
    if(f == null) { return 0 }
    var total = fread(buf as *mut void, 1 as size_t, max_len, f)
    fclose(f)
    return total
}

@test
public func INT_x509_chain_leaf_intermediate_root_vs_python(env : &mut TestEnv) {
    test_write_chain_python()

    var root_path = test_tmp_file(string_view("chem_chain_root.der"))
    var inter_path = test_tmp_file(string_view("chem_chain_inter.der"))
    var leaf_path = test_tmp_file(string_view("chem_chain_leaf.der"))
    var root_buf : [2048]u8; var root_len = test_read_der_file(root_path.data() as *char, &raw mut root_buf[0], 2048)
    var inter_buf : [2048]u8; var inter_len = test_read_der_file(inter_path.data() as *char, &raw mut inter_buf[0], 2048)
    var leaf_buf : [2048]u8; var leaf_len = test_read_der_file(leaf_path.data() as *char, &raw mut leaf_buf[0], 2048)
    if(root_len == 0 || inter_len == 0 || leaf_len == 0) { env.error("failed to read chain DER files"); return }

    var root : X509Cert; x509_cert_init(unsafe(&raw mut root))
    var inter : X509Cert; x509_cert_init(unsafe(&raw mut inter))
    var leaf : X509Cert; x509_cert_init(unsafe(&raw mut leaf))
    var ret = parse_cert_der(unsafe(&raw mut root), &raw root_buf[0], root_len)
    if(ret < 0) { env.error("parse root failed"); return }
    ret = parse_cert_der(unsafe(&raw mut inter), &raw inter_buf[0], inter_len)
    if(ret < 0) { env.error("parse inter failed"); return }
    ret = parse_cert_der(unsafe(&raw mut leaf), &raw leaf_buf[0], leaf_len)
    if(ret < 0) { env.error("parse leaf failed"); return }

    // Build the peer chain: the server sends leaf then intermediate.
    unsafe { leaf.next = &raw mut inter }

    // Verify: leaf -> inter via peer chain (4b), inter -> root via trusted
    // root match (4a). Root is a 4096-bit RSA key — exercises MAX_LIMBS.
    var hostname = "test.example.com\0" as *char
    ret = x509_verify_chain(unsafe(&raw mut leaf), unsafe(&raw mut root), hostname)
    if(ret != 0) { env.error("full chain verify should succeed"); return }
    if(unsafe(leaf.flags) != 0) { env.error("leaf flags should be 0 on success"); return }
}

// ─── Chain fails with an unrelated trusted root ──────────────────────
// Guards the chain-walk: a wrong root must NOT be accepted even though
// leaf -> intermediate verifies.

@test
public func INT_x509_chain_wrong_root_fails_vs_python(env : &mut TestEnv) {
    test_write_chain_python()

    var root_path = test_tmp_file(string_view("chem_chain_root.der"))
    var inter_path = test_tmp_file(string_view("chem_chain_inter.der"))
    var leaf_path = test_tmp_file(string_view("chem_chain_leaf.der"))
    var wrong_path = test_tmp_file(string_view("chem_chain_wrong.der"))
    var root_buf : [2048]u8; var root_len = test_read_der_file(root_path.data() as *char, &raw mut root_buf[0], 2048)
    var inter_buf : [2048]u8; var inter_len = test_read_der_file(inter_path.data() as *char, &raw mut inter_buf[0], 2048)
    var leaf_buf : [2048]u8; var leaf_len = test_read_der_file(leaf_path.data() as *char, &raw mut leaf_buf[0], 2048)
    if(root_len == 0 || inter_len == 0 || leaf_len == 0) { env.error("failed to read chain DER files"); return }

    // Generate an UNRELATED self-signed root.
    var script : [8192]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import datetime\nfrom cryptography import x509\nfrom cryptography.x509.oid import NameOID\nfrom cryptography.hazmat.primitives import hashes\nfrom cryptography.hazmat.primitives.asymmetric import rsa\nfrom cryptography.hazmat.primitives.serialization import Encoding\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "import datetime\nkey=rsa.generate_private_key(65537,2048)\nna=datetime.datetime.utcnow()-datetime.timedelta(days=1)\nnb=datetime.datetime.utcnow()+datetime.timedelta(days=365)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "cert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'WrongCA')])).issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'WrongCA')])).public_key(key.public_key()).serial_number(9).not_valid_before(na).not_valid_after(nb).add_extension(x509.BasicConstraints(ca=True,path_length=None),critical=True).sign(key,hashes.SHA256()))\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, wrong_path.to_view())
    l = "','wb').write(cert.public_bytes(Encoding.DER))\nprint('OK')\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_python_run_script(&raw script[0], sp, string_view("wrong_root.py"))

    var wrong_buf : [2048]u8; var wrong_len = test_read_der_file(wrong_path.data() as *char, &raw mut wrong_buf[0], 2048)
    if(wrong_len == 0) { env.error("failed to read wrong root"); return }

    var root : X509Cert; x509_cert_init(unsafe(&raw mut root))
    var inter : X509Cert; x509_cert_init(unsafe(&raw mut inter))
    var leaf : X509Cert; x509_cert_init(unsafe(&raw mut leaf))
    var wrong : X509Cert; x509_cert_init(unsafe(&raw mut wrong))
    var ret = parse_cert_der(unsafe(&raw mut root), &raw root_buf[0], root_len)
    if(ret < 0) { env.error("parse root failed"); return }
    ret = parse_cert_der(unsafe(&raw mut inter), &raw inter_buf[0], inter_len)
    if(ret < 0) { env.error("parse inter failed"); return }
    ret = parse_cert_der(unsafe(&raw mut leaf), &raw leaf_buf[0], leaf_len)
    if(ret < 0) { env.error("parse leaf failed"); return }
    ret = parse_cert_der(unsafe(&raw mut wrong), &raw wrong_buf[0], wrong_len)
    if(ret < 0) { env.error("parse wrong root failed"); return }

    unsafe { leaf.next = &raw mut inter }
    var hostname = "test.example.com\0" as *char
    ret = x509_verify_chain(unsafe(&raw mut leaf), unsafe(&raw mut wrong), hostname)
    if(ret == 0) { env.error("chain verify should FAIL with unrelated root"); return }
}

// ─── Chain fails when the intermediate is NOT provided ──────────────
// The leaf's issuer (InterCA) is not in the trusted store and no
// intermediate was sent — the chain must not verify.

@test
public func INT_x509_chain_missing_intermediate_fails_vs_python(env : &mut TestEnv) {
    test_write_chain_python()

    var root_path = test_tmp_file(string_view("chem_chain_root.der"))
    var leaf_path = test_tmp_file(string_view("chem_chain_leaf.der"))
    var root_buf : [2048]u8; var root_len = test_read_der_file(root_path.data() as *char, &raw mut root_buf[0], 2048)
    var leaf_buf : [2048]u8; var leaf_len = test_read_der_file(leaf_path.data() as *char, &raw mut leaf_buf[0], 2048)
    if(root_len == 0 || leaf_len == 0) { env.error("failed to read chain DER files"); return }

    var root : X509Cert; x509_cert_init(unsafe(&raw mut root))
    var leaf : X509Cert; x509_cert_init(unsafe(&raw mut leaf))
    var ret = parse_cert_der(unsafe(&raw mut root), &raw root_buf[0], root_len)
    if(ret < 0) { env.error("parse root failed"); return }
    ret = parse_cert_der(unsafe(&raw mut leaf), &raw leaf_buf[0], leaf_len)
    if(ret < 0) { env.error("parse leaf failed"); return }

    // leaf.next stays null — the intermediate was never sent.
    var hostname = "test.example.com\0" as *char
    ret = x509_verify_chain(unsafe(&raw mut leaf), unsafe(&raw mut root), hostname)
    if(ret == 0) { env.error("chain verify should FAIL without the intermediate"); return }
}

// ─── Multi-cert PEM bundle parses into a full chain + free ──────────
// Python writes root + intermediate + leaf as PEMs into ONE file.
// x509_crt_load_pem_file must parse all three into a linked chain, and
// cert_chain_free must release the whole thing (regression for
// single-cert-only parsing and the new chain cleanup).

@test
public func INT_x509_pem_bundle_multicert_vs_python(env : &mut TestEnv) {
    test_write_chain_python()
    var root_path = test_tmp_file(string_view("chem_chain_root.der"))
    var inter_path = test_tmp_file(string_view("chem_chain_inter.der"))
    var leaf_path = test_tmp_file(string_view("chem_chain_leaf.der"))
    var bundle_path = test_tmp_file(string_view("chem_chain_bundle.pem"))

    // Bundle all three PEMs into a single file.
    var script : [8192]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography import x509\nfrom cryptography.x509 import load_pem_x509_certificate\nfrom cryptography.hazmat.primitives.serialization import Encoding\n" as *char; si=0
    while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var l = "root=x509.load_der_x509_certificate(open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, root_path.to_view())
    l = "','rb').read())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "inter=x509.load_der_x509_certificate(open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, inter_path.to_view())
    l = "','rb').read())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "leaf=x509.load_der_x509_certificate(open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, leaf_path.to_view())
    l = "','rb').read())\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "bundle=root.public_bytes(Encoding.PEM)+inter.public_bytes(Encoding.PEM)+leaf.public_bytes(Encoding.PEM)\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    l = "open('" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_script_append_view(&raw mut script[0], &raw mut sp, bundle_path.to_view())
    l = "','wb').write(bundle)\nprint('OK')\n" as *char; si=0
    while(l[si]!=0){script[sp]=l[si] as u8; sp+=1; si+=1}
    test_python_run_script(&raw script[0], sp, string_view("bundle_gen.py"))

    var chain = x509_crt_load_pem_file(bundle_path.data() as *char)
    if(chain == null) { env.error("failed to load multi-cert PEM bundle"); return }

    // Walk the parsed chain: expect exactly 3 certs, root first.
    var count : size_t = 0
    var curr = chain
    while(curr != null) { count += 1; curr = curr.next }
    if(count != 3) { env.error("expected 3 certs in bundle chain"); }

    // chain order from the loader is root -> intermediate -> leaf. Locate the
    // intermediate (index 1) and leaf (index 2) so we can link them as a
    // peer chain for verification, then pass the bundle head as the trust store.
    var inter = chain
    if(inter != null) { inter = inter.next }
    var leaf_cert = inter
    if(leaf_cert != null) { leaf_cert = leaf_cert.next }
    if(leaf_cert == null) { env.error("bundle chain too short"); }

    // Simulate the server sending leaf + intermediate (whose subject is the
    // leaf's issuer), while the bundle head (root) is the trusted anchor.
    // Detach the parse-time links first so the trust-store walk (root->inter)
    // and the peer chain (leaf->inter) don't form a cycle.
    inter.next = null
    leaf_cert.next = inter

    var hostname = "test.example.com\0" as *char
    var ret = x509_verify_chain(leaf_cert, chain, hostname)
    if(ret != 0) { env.error("bundle chain verify should succeed") }

    cert_chain_free(chain)
}

// ─── cert_chain_free on a system CA bundle (regression) ─────────────
// Replacement for the old cert_free + dealloc pattern. Runs the walk in
// both directions to catch double-free/garbage pointer handling.

@test
public func INT_cert_chain_free_system_bundle_stability(env : &mut TestEnv) {
    var ca = load_system_ca_bundle()
    if(ca == null) {
        env.error("no system CA bundle found")
        return
    }
    var count : size_t = 0
    var curr = ca
    while(curr != null) { count += 1; curr = curr.next }
    if(count < 1) { env.error("system CA bundle should contain at least one cert") }
    cert_chain_free(ca)
}