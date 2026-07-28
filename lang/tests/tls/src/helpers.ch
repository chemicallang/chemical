// Shared helpers for TLS integration tests

func test_nibble_to_hex(n : uint) : char {
    if(n < 10) { return (48 as char) + (n as char) }
    else { return (87 as char) + (n as char) }
}

func test_bytes_to_hex(data : *u8, len : size_t, hex_out : *mut char) {
    var i : size_t = 0
    while(i < len) {
        hex_out[i*2] = test_nibble_to_hex(((data[i] as uint)>>4)&0xF)
        hex_out[i*2+1] = test_nibble_to_hex((data[i] as uint)&0xF)
        i += 1
    }
    hex_out[len*2] = 0
}

func test_bytes_eq(a : *u8, b : *u8, len : size_t) : bool {
    var i : size_t = 0
    while(i < len) { if(a[i] != b[i]) { return false }; i += 1 }
    return true
}

func test_write_file(path : *char, data : *u8, len : size_t) : bool {
    var f = fopen(path, "wb\0" as *char)
    if(f == null) { return false } else {}
    fwrite(data as *mut void, 1 as size_t, len, f)
    fclose(f)
    return true
}

func test_read_file(path : *char, buf : *mut u8, max_len : size_t) : size_t {
    var f = fopen(path, "rb\0" as *char)
    if(f == null) { return 0 } else {}
    var total : size_t = 0
    while(total < max_len) {
        var n = fread((buf + total) as *mut void, 1 as size_t, max_len - total, f)
        if(n <= 0) { break } else {}
        total += n
    }
    fclose(f)
    return total
}

func test_hex_char_val(c : char) : uint {
    var v : uint = c as uint
    if(v >= 48u && v <= 57u) { return v - 48u }
    else if(v >= 65u && v <= 70u) { return v - 55u }
    else if(v >= 97u && v <= 102u) { return v - 87u }
    else { return 0 }
}

func test_hex_pair_byte(hi : char, lo : char) : u8 {
    return ((test_hex_char_val(hi) << 4) | test_hex_char_val(lo)) as u8
}

func write_tls_python_utils() {
    var py = std::string()
    py.append_view("import sys,ssl,socket,datetime,time\n")
    py.append_view("from cryptography import x509\n")
    py.append_view("from cryptography.x509.oid import NameOID\n")
    py.append_view("from cryptography.hazmat.primitives import hashes,serialization\n")
    py.append_view("from cryptography.hazmat.primitives.asymmetric import ec,rsa\n")
    py.append_view("def gen_cert(c,k,cn,t='ec'):\n")
    py.append_view("    key=ec.generate_private_key(ec.SECP256R1()) if t=='ec' else rsa.generate_private_key(65537,2048)\n")
    py.append_view("    cert=(x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,cn)])).issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,cn)])).public_key(key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(datetime.datetime.utcnow()).not_valid_after(datetime.datetime.utcnow()+datetime.timedelta(days=365)).sign(key,hashes.SHA256()))\n")
    py.append_view("    open(c,'wb').write(cert.public_bytes(serialization.Encoding.PEM))\n")
    py.append_view("    open(k,'wb').write(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.TraditionalOpenSSL,serialization.NoEncryption()))\n")
    py.append_view("def tls_srv(cert,key,port,ver='1.3',ciph=None):\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_3 if ver=='1.3' else ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    if ver!='1.3':ctx.maximum_version=ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    if ciph:ctx.set_ciphers(ciph)\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(1)\n")
    py.append_view("    s.settimeout(10)\n")
    py.append_view("    try:\n")
    py.append_view("        c,a=s.accept()\n")
    py.append_view("        t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("        d=t.recv(4096)\n")
    py.append_view("        t.sendall(b'OK')\n")
    py.append_view("        t.close()\n")
    py.append_view("    except socket.timeout:\n")
    py.append_view("        pass\n")
    py.append_view("    s.close()\n")
    py.append_view("def tls_cli(host,port,ver='1.3',ciph=None):\n")
    py.append_view("    import os\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)\n")
    py.append_view("    ctx.check_hostname=False\n")
    py.append_view("    ctx.verify_mode=ssl.CERT_NONE\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_3 if ver=='1.3' else ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    if ver!='1.3':ctx.maximum_version=ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    if ciph:ctx.set_ciphers(ciph)\n")
    py.append_view("    keylog = '/tmp/tls_keylog_'+str(port)+'.txt'\n")
    py.append_view("    ctx.keylog_filename = keylog\n")
    py.append_view("    for i in range(50):\n")
    py.append_view("        try:\n")
    py.append_view("            s=socket.create_connection((host,int(port)),timeout=2)\n")
    py.append_view("            break\n")
    py.append_view("        except (ConnectionRefusedError,OSError):\n")
    py.append_view("            time.sleep(0.1)\n")
    py.append_view("    else:\n")
    py.append_view("        sys.exit(1)\n")
    py.append_view("    try:\n")
    py.append_view("        t=ctx.wrap_socket(s,server_hostname=host)\n")
    py.append_view("    except Exception as e:\n")
    py.append_view("        print('WRAP_ERROR:'+str(e),file=sys.stderr)\n")
    py.append_view("        s.close()\n")
    py.append_view("        sys.exit(0)\n")
    py.append_view("    t.sendall(b'GET / HTTP/1.0\\r\\n\\r\\n')\n")
    py.append_view("    d=t.recv(4096)\n")
    py.append_view("    t.close()\n")
    py.append_view("cmd=sys.argv[1]\n")
    py.append_view("if cmd=='cert':\n")
    py.append_view("    gen_cert(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else 'ec')\n")
    py.append_view("elif cmd=='srv':\n")
    py.append_view("    tls_srv(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else '1.3',sys.argv[6] if len(sys.argv)>6 else None)\n")
    py.append_view("elif cmd=='cli':\n")
    py.append_view("    tls_cli(sys.argv[2],sys.argv[3],sys.argv[4] if len(sys.argv)>4 else '1.3',sys.argv[5] if len(sys.argv)>5 else None)\n")
    test_write_file("/tmp/tls_utils.py\0" as *char, py.data() as *u8, py.size())
}
