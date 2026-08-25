// Shared helpers for TLS integration tests

using std::string;
using std::string_view;
using std::vector;

// -- Cross-platform temp path --
func test_temp_path(name : string_view) : string {
    comptime if(def.windows) {
        var dir_opt = environment_get_temp()
        var result = string()
        if(dir_opt != null) {
            result = string(dir_opt)
        } else {
            result = string("C:\\Windows\\Temp\\")
        }
        result.append_view("\\")
        result.append_view(&name)
        return result
    } else {
        var result = string("/tmp/")
        result.append_view(&name)
        return result
    }
}

func environment_get_temp() : *char {
    comptime if(def.windows) {
        return getenv("TEMP")
    } else {
        return getenv("TMPDIR")
    }
}

// Absolute temp path for a test artifact, normalized to forward slashes so the
// same string can be used in C fopen() calls and embedded in Python string
// literals (both accept '/' as a separator on Windows; raw backslashes would
// be interpreted as Python escapes and corrupt the path).
func test_tmp_file(name : string_view) : string {
    var p = test_temp_path(name)
    var out = string()
    var i : size_t = 0
    while(i < p.size()) {
        var c = p.get(i)
        if(c == '\\') { out.append('/') } else { out.append(c) }
        i += 1
    }
    return out
}

// Append a string_view into a byte buffer — used to splice resolved temp paths
// into the generated Python scripts.
func test_script_append_view(buf : *mut u8, len : *mut size_t, v : string_view) {
    var i : size_t = 0
    while(i < v.size()) {
        buf[*len] = v.get(i) as u8
        *len += 1
        i += 1
    }
}

// -- Cross-platform random bytes --
func test_random_bytes(buf : *mut u8, len : size_t) {
    tls::random_fill(buf, len)
}

// -- Cross-platform shell helpers --
// On Windows, native executables (MSVCRT) and Python both resolve the POSIX
// path "/tmp/x" to "<current-drive>:\tmp\x". Ensure that directory exists so
// the tests' literal "/tmp/..." paths work unchanged on both platforms.
func test_ensure_tmp_dir() {
    comptime if(def.windows) {
        system("if not exist \\tmp mkdir \\tmp")
    }
}

// Python interpreter name (the tests were written against `python3`; Windows
// Python installs only provide `python`).
func test_py_interp() : string {
    comptime if(def.windows) {
        return string("python ")
    } else {
        return string("python3 ")
    }
}

// stderr redirect suffix for shell commands.
func test_redir() : string {
    comptime if(def.windows) {
        return string(" 2>nul")
    } else {
        return string(" 2>/dev/null")
    }
}

// Builds: <interp> /tmp/tls_utils.py <args> <redir>
func test_py_cmd(args : string_view) : string {
    var cmd = test_py_interp()
    cmd.append_view("/tmp/tls_utils.py")
    cmd.append_view(" ")
    cmd.append_view(&args)
    var redir = test_redir()
    cmd.append_view(redir.to_view())
    return cmd
}

// Run a python tls_utils.py command in the foreground. Keeps the command string
// alive in a local before system() reads it (passing a temporary's .data()
// directly dangles the SSO buffer on Windows).
func test_py_run_foreground(args : string_view) {
    var cmd = test_py_cmd(args)
    system(cmd.data())
}

// Run a python tls_utils.py command in the background (same lifetime care).
func test_py_run_background(args : string_view) {
    var cmd = test_py_cmd(args)
    test_run_bg(cmd.data())
}

// Kill whatever is listening on `port` (POSIX only; Windows background python
// servers exit on their own after a short timeout).
func test_kill_port(port : int) {
    comptime if(!def.windows) {
        var cmd = string("fuser -k ")
        cmd.append_integer(port)
        cmd.append_view("/tcp 2>/dev/null")
        system(cmd.data())
    }
}

// Print a file to stdout (used to surface python client stderr in e2e tests).
func test_cat_file(path : string_view) {
    comptime if(def.windows) {
        // cmd's `type` treats '/' in paths as command switches and fails with
        // "The syntax of the command is incorrect." — use backslashes instead.
        var win_path = string()
        var i : size_t = 0
        while(i < path.size()) {
            var c = path.get(i)
            if(c == '/') { win_path.append('\\') } else { win_path.append(c) }
            i += 1
        }
        var cmd = string("type ")
        cmd.append_view(win_path.to_view())
        system(cmd.data())
    } else {
        var cmd = string("cat ")
        cmd.append_view(&path)
        cmd.append_view(" 2>/dev/null")
        system(cmd.data())
    }
}

// Wait ~1s for a background python server to come up.
func test_server_wait() {
    comptime if(def.windows) {
        system("ping -n 2 127.0.0.1 >nul")
    } else {
        system("sleep 1")
    }
}

// Run a command (already fully formed, including redirects) in the background.
func test_run_bg(cmd : *char) {
    comptime if(def.windows) {
        var full = string("start /b ")
        full.append_view(string_view(cmd))
        system(full.data())
    } else {
        var full = string("setsid ")
        full.append_view(string_view(cmd))
        full.append_view(" &")
        system(full.data())
    }
}

// -- Cross-platform Python script runner --
func test_python_run_script(script : *u8, len : size_t, script_name : string_view) : vector<u8> {
    test_ensure_tmp_dir()
    var path = test_temp_path(script_name)
    if(!test_write_file(path.data(), script, len)) { return vector<u8>() } else {}
    return test_python_run_script_file(path.data())
}

func test_python_run_script_file(py_path : *char) : vector<u8> {
    var result = vector<u8>()
    var out_path = string(py_path)
    out_path.append_view(".out")

    comptime if(def.windows) {
        var cmd = test_py_interp()
        cmd.append_view(string_view(py_path))
        cmd.append_view(" > ")
        cmd.append_view(out_path.to_view())
        cmd.append_view(" 2>nul")
        system(cmd.data())
    } else {
        var cmd = test_py_interp()
        cmd.append_view(string_view(py_path))
        cmd.append_view(" > ")
        cmd.append_view(out_path.to_view())
        cmd.append_view(" 2>/dev/null")
        system(cmd.data())
    }

    unsafe var buf : [4096]u8
    var n = test_read_file(out_path.data(), &raw mut buf[0], 4096)
    var i : size_t = 0
    while(i < n) { result.push(buf[i]); i += 1 }
    return result
}

// -- Parse hex output from Python stdout --
func test_parse_py_hex_label(output : *vector<u8>, label : string_view, out : *mut u8, out_len : size_t) : size_t {
    var pos : size_t = 0
    var found = false
    while(pos + label.size() < output.size()) {
        var match = true
        var li : size_t = 0
        while(li < label.size()) {
            if(output.get(pos + li) != (label.get(li) as u8)) { match = false; break } else {}
            li += 1
        }
        if(match) { pos += label.size(); found = true; break } else {}
        pos += 1
    }
    if(!found) { return 0 } else {}
    var written : size_t = 0
    while(written < out_len && pos < output.size()) {
        var hi = output.get(pos) as char
        if(hi == 10 as char || hi == 13 as char || hi == 0 as char) { break } else {}
        if(pos + 1 >= output.size()) {
            out[written] = test_hex_char_val(hi) as u8
            written += 1
            break
        }
        var lo = output.get(pos + 1) as char
        if(lo == 10 as char || lo == 13 as char || lo == 0 as char) {
            out[written] = test_hex_char_val(hi) as u8
            written += 1
            break
        }
        out[written] = test_hex_pair_byte(hi, lo)
        written += 1
        pos += 2
    }
    return written
}

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

// Parse an "N=<lowercase hex>\nD=<lowercase hex>\n" dump (from the tls_utils.py
// privkey command for RSA keys) into big-endian byte buffers. Left-pads N/D with
// zeros so the caller receives exactly the raw modulus/private exponent.
func test_parse_n_d_hex_file(path : *char, n_out : *mut u8, n_max : size_t, n_len : *mut size_t,
                             d_out : *mut u8, d_max : size_t, d_len : *mut size_t) {
    unsafe var buf : [4096]u8
    var total = test_read_file(path, &raw mut buf[0], 4096)
    *n_len = 0
    *d_len = 0
    var pos : size_t = 0
    while(pos < total) {
        if(buf[pos] == 78 && pos + 1 < total && buf[pos + 1] == 61) {
            pos += 2
            var w : size_t = 0
            while(pos < total && buf[pos] != 10 && buf[pos] != 13) {
                if(pos + 1 < total && buf[pos + 1] != 10 && buf[pos + 1] != 13) {
                    if(w < n_max) { n_out[w] = test_hex_pair_byte(buf[pos] as char, buf[pos + 1] as char); w += 1 }
                    pos += 2
                } else {
                    if(w < n_max) { n_out[w] = test_hex_char_val(buf[pos] as char) as u8; w += 1 }
                    pos += 1
                }
            }
            *n_len = w
        } else if(buf[pos] == 68 && pos + 1 < total && buf[pos + 1] == 61) {
            pos += 2
            var w : size_t = 0
            while(pos < total && buf[pos] != 10 && buf[pos] != 13) {
                if(pos + 1 < total && buf[pos + 1] != 10 && buf[pos + 1] != 13) {
                    if(w < d_max) { d_out[w] = test_hex_pair_byte(buf[pos] as char, buf[pos + 1] as char); w += 1 }
                    pos += 2
                } else {
                    if(w < d_max) { d_out[w] = test_hex_char_val(buf[pos] as char) as u8; w += 1 }
                    pos += 1
                }
            }
            *d_len = w
        }
        pos += 1
    }
}

func write_tls_python_utils() {
    test_ensure_tmp_dir()
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
    py.append_view("def tls_srv(cert,key,port,ver='1.3',ciph=None,alpn=None):\n")
    py.append_view("    import os\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    ctx.keylog_filename = '/tmp/tls_keylog_'+port+'.txt'\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_3 if ver=='1.3' else ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    if ver!='1.3':ctx.maximum_version=ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    if ciph:ctx.set_ciphers(ciph)\n")
    py.append_view("    if alpn:ctx.set_alpn_protocols(alpn.split(','))\n")
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
    py.append_view("def tls_msrv(cert,key,port,ver='1.2',nconn=2,ciph=None):\n")
    py.append_view("    import os\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    ctx.keylog_filename = '/tmp/tls_keylog_'+port+'.txt'\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_2 if ver=='1.2' else ssl.TLSVersion.TLSv1_3\n")
    py.append_view("    if ver!='1.3':ctx.maximum_version=ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    if ciph:ctx.set_ciphers(ciph)\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(nconn)\n")
    py.append_view("    s.settimeout(15)\n")
    py.append_view("    for i in range(nconn):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            d=t.recv(4096)\n")
    py.append_view("            size=131072\n")
    py.append_view("            payload=bytes([i%251 for i in range(size)])\n")
    py.append_view("            off=0\n")
    py.append_view("            while off<size:\n")
    py.append_view("                m=min(16384,size-off)\n")
    py.append_view("                t.sendall(payload[off:off+m])\n")
    py.append_view("                off+=m\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception:\n")
    py.append_view("            pass\n")
    py.append_view("    s.close()\n")
    py.append_view("def srv2(cert,key,port):\n")
    py.append_view("    import os\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    ctx.keylog_filename = '/tmp/tls_keylog_'+str(port)+'.txt'\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_3\n")
    py.append_view("    ctx.maximum_version=ssl.TLSVersion.TLSv1_3\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(2)\n")
    py.append_view("    s.settimeout(15)\n")
    py.append_view("    for i in range(2):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            d=t.recv(4096)\n")
    py.append_view("            t.sendall(b'OK')\n")
    py.append_view("            t.close()\n")
    py.append_view("        except socket.timeout:\n")
    py.append_view("            pass\n")
    py.append_view("    s.close()\n")
    py.append_view("def bigsrv(cert,key,port,size=131072):\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_3\n")
    py.append_view("    ctx.maximum_version=ssl.TLSVersion.TLSv1_3\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(1)\n")
    py.append_view("    s.settimeout(10)\n")
    py.append_view("    try:\n")
    py.append_view("        c,a=s.accept()\n")
    py.append_view("        t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("        d=t.recv(4096)\n")
    py.append_view("        size=int(size)\n")
    py.append_view("        payload=bytes([i%251 for i in range(size)])\n")
    py.append_view("        off=0\n")
    py.append_view("        while off<size:\n")
    py.append_view("            m=min(16384,size-off)\n")
    py.append_view("            t.sendall(payload[off:off+m])\n")
    py.append_view("            off+=m\n")
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
    py.append_view("    try:\n")
    py.append_view("        t.sendall(b'GET / HTTP/1.0\\r\\n\\r\\n')\n")
    py.append_view("        d=t.recv(4096)\n")
    py.append_view("        t.close()\n")
    py.append_view("    except Exception:\n")
    py.append_view("        pass\n")
    py.append_view("def gen_ca(prefix,cn):\n")
    py.append_view("    root_key=rsa.generate_private_key(65537,2048)\n")
    py.append_view("    nm=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,cn)])\n")
    py.append_view("    root=(x509.CertificateBuilder().subject_name(nm).issuer_name(nm).public_key(root_key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(datetime.datetime.utcnow()-datetime.timedelta(days=1)).not_valid_after(datetime.datetime.utcnow()+datetime.timedelta(days=365)).add_extension(x509.BasicConstraints(ca=True,path_length=None),critical=True).sign(root_key,hashes.SHA256()))\n")
    py.append_view("    open(prefix+'_root.pem','wb').write(root.public_bytes(serialization.Encoding.PEM))\n")
    py.append_view("    open(prefix+'_root.key','wb').write(root_key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.TraditionalOpenSSL,serialization.NoEncryption()))\n")
    py.append_view("    return root,root_key\n")
    py.append_view("def ca_cmd(prefix,leafcn,with_inter):\n")
    py.append_view("    import os\n")
    py.append_view("    root,root_key=gen_ca(prefix,'Test Root CA')\n")
    py.append_view("    leaf_key=ec.generate_private_key(ec.SECP256R1())\n")
    py.append_view("    if with_inter:\n")
    py.append_view("        int_key=rsa.generate_private_key(65537,2048)\n")
    py.append_view("        intnm=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'Test Intermediate CA')])\n")
    py.append_view("        inter=x509.CertificateBuilder().subject_name(intnm).issuer_name(root.subject).public_key(int_key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(datetime.datetime.utcnow()-datetime.timedelta(days=1)).not_valid_after(datetime.datetime.utcnow()+datetime.timedelta(days=365)).add_extension(x509.BasicConstraints(ca=True,path_length=0),critical=True).sign(root_key,hashes.SHA256())\n")
    py.append_view("        signer=int_key; issuer_cert=inter; inter_pem=inter.public_bytes(serialization.Encoding.PEM)\n")
    py.append_view("    else:\n")
    py.append_view("        signer=root_key; issuer_cert=root; inter_pem=b''\n")
    py.append_view("    leaftm=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,leafcn)])\n")
    py.append_view("    leaf=(x509.CertificateBuilder().subject_name(leaftm).issuer_name(issuer_cert.subject).public_key(leaf_key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(datetime.datetime.utcnow()-datetime.timedelta(days=1)).not_valid_after(datetime.datetime.utcnow()+datetime.timedelta(days=365)).add_extension(x509.SubjectAlternativeName([x509.DNSName(leafcn)]),critical=False).sign(signer,hashes.SHA256()))\n")
    py.append_view("    leaf_pem=leaf.public_bytes(serialization.Encoding.PEM)\n")
    py.append_view("    open(prefix+'_leaf.crt','wb').write(leaf_pem)\n")
    py.append_view("    open(prefix+'_chain.crt','wb').write(leaf_pem+inter_pem)\n")
    py.append_view("    open(prefix+'_leaf.key','wb').write(leaf_key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.TraditionalOpenSSL,serialization.NoEncryption()))\n")
    py.append_view("def mround(cert,key,port,ver='1.3',rounds=5,nconn=1):\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_3 if ver=='1.3' else ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    if ver!='1.3':ctx.maximum_version=ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(int(nconn))\n")
    py.append_view("    s.settimeout(20)\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            for r in range(int(rounds)):\n")
    py.append_view("                d=t.recv(4096)\n")
    py.append_view("                t.sendall(b'OK')\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('MROUND_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")
    py.append_view("def echo_srv(cert,key,port,nconn,size):\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_3\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(int(nconn))\n")
    py.append_view("    s.settimeout(30)\n")
    py.append_view("    size=int(size)\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            buf=b''\n")
    py.append_view("            while len(buf)<size:\n")
    py.append_view("                d=t.recv(16384)\n")
    py.append_view("                if not d:break\n")
    py.append_view("                buf+=d\n")
    py.append_view("            want=bytes([j%251 for j in range(len(buf))])\n")
    py.append_view("            t.sendall(b'OK' if buf==want and len(buf)==size else b'BAD')\n")
    py.append_view("            print('ECHO_GOT:'+str(len(buf)),file=sys.stderr)\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('ECHO_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")
    py.append_view("def plaintcp(port,payload,nconn=1):\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(int(nconn))\n")
    py.append_view("    s.settimeout(10)\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            c.recv(1024)\n")
    py.append_view("            c.sendall(payload.encode() if isinstance(payload,str) else payload)\n")
    py.append_view("            c.close()\n")
    py.append_view("        except Exception:\n")
    py.append_view("            pass\n")
    py.append_view("    s.close()\n")
    py.append_view("def read_http_request(c):\n")
    py.append_view("    data=b''\n")
    py.append_view("    while b'\\r\\n\\r\\n' not in data:\n")
    py.append_view("        d=c.recv(4096)\n")
    py.append_view("        if not d:return None,None,None\n")
    py.append_view("        data+=d\n")
    py.append_view("    head,rest=data.split(b'\\r\\n\\r\\n',1)\n")
    py.append_view("    lines=head.split(b'\\r\\n')\n")
    py.append_view("    parts=lines[0].split(b' ')\n")
    py.append_view("    method=parts[0].decode();path=parts[1].decode() if len(parts)>1 else '/'\n")
    py.append_view("    hdrs={}\n")
    py.append_view("    for h in lines[1:]:\n")
    py.append_view("        if b':' in h:\n")
    py.append_view("            k,v=h.split(b':',1);hdrs[k.strip().lower().decode()]=v.strip().decode()\n")
    py.append_view("    clen=int(hdrs.get('content-length','0'))\n")
    py.append_view("    while len(rest)<clen:\n")
    py.append_view("        d=c.recv(65536)\n")
    py.append_view("        if not d:break\n")
    py.append_view("        rest+=d\n")
    py.append_view("    return (method,path,hdrs,rest[:clen])\n")
    py.append_view("def httpsrv(cert,key,port,nconn=8):\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(int(nconn))\n")
    py.append_view("    s.settimeout(60)\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('HTTPS_SRV_ACCEPT_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("            break\n")
    py.append_view("        try:\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            m,p,h,b=read_http_request(t)\n")
    py.append_view("            if m is None:t.close();continue\n")
    py.append_view("            if p.startswith('/status/'):\n")
    py.append_view("                code=int(p.split('/')[2]);body=b's';hdr={'Content-Length':'1'}\n")
    py.append_view("            elif p=='/hello' and m in ('GET','HEAD'):\n")
    py.append_view("                code=200;body=b'world';hdr={'Content-Length':'5','X-Custom':'chemical-test','Content-Type':'text/plain'}\n")
    py.append_view("            elif p=='/echo' and m=='POST':\n")
    py.append_view("                code=200;body=b;hdr={'Content-Length':str(len(b)),'X-Content-Type':h.get('content-type','')}\n")
    py.append_view("            elif p.startswith('/size/'):\n")
    py.append_view("                n=int(p.split('/')[2]);code=200;body=bytes([j%251 for j in range(n)]);hdr={'Content-Length':str(n),'Content-Type':'application/octet-stream'}\n")
    py.append_view("            elif p.startswith('/query'):\n")
    py.append_view("                q=p.split('?',1)[1] if '?' in p else '';code=200;body=q.encode();hdr={'Content-Length':str(len(body))}\n")
    py.append_view("            elif p=='/hdrs':\n")
    py.append_view("                body=(h.get('x-test','')+'|'+h.get('user-agent','')).encode();code=200\n")
    py.append_view("                hdr={'Content-Length':str(len(body))}\n")
    py.append_view("            elif p=='/chunked':\n")
    py.append_view("                payload=b'chunked-body-data'\n")
    py.append_view("                resp=b'HTTP/1.1 200 OK\\r\\nTransfer-Encoding: chunked\\r\\n\\r\\n'\n")
    py.append_view("                resp+=format(len(payload)//2,'x').encode()+b'\\r\\n'+payload[:len(payload)//2]+b'\\r\\n'\n")
    py.append_view("                resp+=format(len(payload)-len(payload)//2,'x').encode()+b'\\r\\n'+payload[len(payload)//2:]+b'\\r\\n0\\r\\n\\r\\n'\n")
    py.append_view("                t.sendall(resp);t.close();continue\n")
    py.append_view("            elif p=='/put' and m=='PUT':\n")
    py.append_view("                code=200;body=b'put:'+b;hdr={'Content-Length':str(len(body))}\n")
    py.append_view("            elif p=='/patch' and m=='PATCH':\n")
    py.append_view("                code=200;body=b'patched';hdr={'Content-Length':'7'}\n")
    py.append_view("            elif p=='/del' and m=='DELETE':\n")
    py.append_view("                code=200;body=b'deleted';hdr={'Content-Length':'7'}\n")
    py.append_view("            else:\n")
    py.append_view("                code=404;body=b'not found';hdr={'Content-Length':'9'}\n")
    py.append_view("            resp=('HTTP/1.1 '+str(code)+' T\\r\\n').encode()\n")
    py.append_view("            for k,v in hdr.items():resp+=(k+': '+v+'\\r\\n').encode()\n")
    py.append_view("            resp+=b'Connection: close\\r\\n\\r\\n'+body\n")
    py.append_view("            t.sendall(resp)\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('HTTPS_SRV_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")
    py.append_view("def hcli(host,port,outfile,mode,cafile=None,ciphers=None):\n")
    py.append_view("    import http.client,hashlib,time\n")
    py.append_view("    try:\n")
    py.append_view("        if cafile:\n")
    py.append_view("            cc=ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT);cc.load_verify_locations(cafile)\n")
    py.append_view("        else:\n")
    py.append_view("            cc=ssl._create_unverified_context()\n")
    py.append_view("        if ciphers:cc.set_ciphers(ciphers)\n")
    py.append_view("        ok=False;why=''\n")
    py.append_view("        if mode=='get':\n")
    py.append_view("            cn=http.client.HTTPSConnection(host,int(port),context=cc,timeout=15)\n")
    py.append_view("            cn.request('GET','/integrate');r=cn.getresponse();d=r.read()\n")
    py.append_view("            ok=(r.status==200 and d==b'SRV_OK');why='status=%s body=%r'%(r.status,d[:40])\n")
    py.append_view("            cn.close()\n")
    py.append_view("        elif mode=='post':\n")
    py.append_view("            payload=bytes([j%251 for j in range(50000)])\n")
    py.append_view("            cn=http.client.HTTPSConnection(host,int(port),context=cc,timeout=30)\n")
    py.append_view("            cn.request('POST','/integrate',body=payload,headers={'Content-Type':'application/octet-stream'})\n")
    py.append_view("            r=cn.getresponse();d=r.read()\n")
    py.append_view("            ok=(r.status==200 and d==payload);why='status=%s len=%d'%(r.status,len(d))\n")
    py.append_view("            cn.close()\n")
    py.append_view("        elif mode=='slowpost':\n")
    py.append_view("            payload=bytes([(j*7)%251 for j in range(20000)])\n")
    py.append_view("            s=socket.create_connection((host,int(port)),timeout=20)\n")
    py.append_view("            ts=cc.wrap_socket(s,server_hostname=host)\n")
    py.append_view("            head=b'POST /integrate HTTP/1.1\\r\\nHost: '+host.encode()+b'\\r\\nContent-Length: 20000\\r\\nConnection: close\\r\\n\\r\\n'\n")
    py.append_view("            ts.sendall(head)\n")
    py.append_view("            off=0\n")
    py.append_view("            while off<len(payload):\n")
    py.append_view("                ts.sendall(payload[off:off+1024]);time.sleep(0.01);off+=1024\n")
    py.append_view("            buf=b''\n")
    py.append_view("            while True:\n")
    py.append_view("                d=ts.recv(65536)\n")
    py.append_view("                if not d:break\n")
    py.append_view("                buf+=d\n")
    py.append_view("            ts.close()\n")
    py.append_view("            body=buf.split(b'\\r\\n\\r\\n',1)[1] if b'\\r\\n\\r\\n' in buf else b''\n")
    py.append_view("            ok=(body==payload);why='resp_len=%d'%len(body)\n")
    py.append_view("        elif mode=='keepalive':\n")
    py.append_view("            cn=http.client.HTTPSConnection(host,int(port),context=cc,timeout=15)\n")
    py.append_view("            cn.request('GET','/integrate?a=1');r1=cn.getresponse();d1=r1.read()\n")
    py.append_view("            cn.request('GET','/integrate?a=2');r2=cn.getresponse();d2=r2.read()\n")
    py.append_view("            ok=(r1.status==200 and d1==b'SRV_OK:a=1' and r2.status==200 and d2==b'SRV_OK:a=2')\n")
    py.append_view("            why='r1=%s/%r r2=%s/%r'%(r1.status,d1,r2.status,d2)\n")
    py.append_view("            cn.close()\n")
    py.append_view("        elif mode=='bigresp':\n")
    py.append_view("            cn=http.client.HTTPSConnection(host,int(port),context=cc,timeout=60)\n")
    py.append_view("            cn.request('GET','/big');r=cn.getresponse();d=r.read()\n")
    py.append_view("            want=bytes([j%251 for j in range(300000)])\n")
    py.append_view("            ok=(r.status==200 and d==want);why='status=%s len=%d'%(r.status,len(d))\n")
    py.append_view("            cn.close()\n")
    py.append_view("        f=open(outfile,'w')\n")
    py.append_view("        f.write(('RESULT:OK' if ok else 'RESULT:FAIL '+why))\n")
    py.append_view("        f.close()\n")
    py.append_view("    except Exception as e:\n")
    py.append_view("        f=open(outfile,'w');f.write('RESULT:FAIL exception '+str(e));f.close()\n")
    py.append_view("cmd=sys.argv[1]\n")
    py.append_view("if cmd=='cert':\n")
    py.append_view("    gen_cert(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else 'ec')\n")
    py.append_view("elif cmd=='privkey':\n")
    py.append_view("    from cryptography.hazmat.primitives.serialization import load_pem_private_key\n")
    py.append_view("    from cryptography.hazmat.primitives.asymmetric import ec\n")
    py.append_view("    key=load_pem_private_key(open(sys.argv[2],'rb').read(),password=None)\n")
    py.append_view("    if isinstance(key, ec.EllipticCurvePrivateKey):\n")
    py.append_view("        val=key.private_numbers().private_value; open(sys.argv[3],'w').write(format(val,'064x'))\n")
    py.append_view("    else:\n")
    py.append_view("        nums=key.private_numbers()\n")
    py.append_view("        open(sys.argv[3],'w').write('N='+format(nums.public_numbers.n,'0512x')+chr(10)+'D='+format(nums.d,'0512x')+chr(10))\n")
    py.append_view("elif cmd=='srv':\n")
    py.append_view("    tls_srv(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else '1.3',sys.argv[6] if len(sys.argv)>6 else None,sys.argv[7] if len(sys.argv)>7 else None)\n")
    py.append_view("elif cmd=='msrv':\n")
    py.append_view("    tls_msrv(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else '1.2',int(sys.argv[6]) if len(sys.argv)>6 else 2,sys.argv[7] if len(sys.argv)>7 else None)\n")
    py.append_view("elif cmd=='srv2':\n")
    py.append_view("    srv2(sys.argv[2],sys.argv[3],sys.argv[4])\n")
    py.append_view("elif cmd=='bigsrv':\n")
    py.append_view("    bigsrv(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else '131072')\n")
    py.append_view("elif cmd=='cli':\n")
    py.append_view("    tls_cli(sys.argv[2],sys.argv[3],sys.argv[4] if len(sys.argv)>4 else '1.3',sys.argv[5] if len(sys.argv)>5 else None)\n")
    py.append_view("elif cmd=='ca':\n")
    py.append_view("    ca_cmd(sys.argv[2],sys.argv[3],len(sys.argv)>4 and sys.argv[4]=='inter')\n")
    py.append_view("elif cmd=='mround':\n")
    py.append_view("    mround(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else '1.3',sys.argv[6] if len(sys.argv)>6 else 5,sys.argv[7] if len(sys.argv)>7 else 1)\n")
    py.append_view("elif cmd=='echo':\n")
    py.append_view("    echo_srv(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else 1,sys.argv[6] if len(sys.argv)>6 else 16384)\n")
    py.append_view("elif cmd=='plaintcp':\n")
    py.append_view("    plaintcp(sys.argv[2],sys.argv[3],sys.argv[4] if len(sys.argv)>4 else 1)\n")
    py.append_view("elif cmd=='httpsrv':\n")
    py.append_view("    httpsrv(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else 8)\n")
    py.append_view("elif cmd=='hcli':\n")
    py.append_view("    hcli(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5] if len(sys.argv)>5 else 'get',sys.argv[6] if len(sys.argv)>6 and sys.argv[6]!='-' else None,sys.argv[7] if len(sys.argv)>7 and sys.argv[7]!='-' else None)\n")
    test_write_file("/tmp/tls_utils.py\0" as *char, py.data() as *u8, py.size())
}
