// ============================================================================
// Extra python helpers for the http/net/tls integration suites.
// ============================================================================
// Writes /tmp/http_extra.py, a second python utility script dedicated to the
// new integration tests. Keeping it separate from write_tls_python_utils()
// avoids disturbing the long-standing TLS suite while giving the http tests
// purpose-built servers:
//
//   cert <c> <k> <cn> [t]            generate a self-signed cert (ec|rsa)
//   seqsrv <cert> <key> <port> <script> <nconn>
//                                    scripted raw responses; each line of the
//                                    script file is hex for ONE connection's
//                                    full response bytes (drains the request
//                                    first, then sends + closes)
//   dribblesrv <cert> <key> <port> <total> <chunk> <delayms>
//                                    sends headers+body in tiny writes with
//                                    delays (many TLS records)
//   hdrsrv <cert> <key> <port> <nconn> <nhdrs> <vlen>
//                                    response with NHDRS X-H-i headers
//   redirsrv <cert> <key> <port> <nconn>
//                                    /r1 -> 301 Location: /r2, else 200
//   pathsrv <cert> <key> <port> <nconn>
//                                    echoes "METHOD path?query" as body
//   slowsrv <cert> <key> <port> <delay> <nconn>
//                                    sleeps DELAY seconds before answering
//   liessrv <cert> <key> <port> <nconn>
//                                    declares Content-Length: 1000 but sends
//                                    400 bytes then closes (truncation)
//
// Client modes (python http.client against a Chemical TLS server):
//   hcli3     host port outfile     three keep-alive GETs on one connection
//   hhead     host port outfile     HEAD must yield no body even with CL
//   hchunkup  host port outfile     chunked upload via generator body
//   hstrict   host port outfile     strict reason-phrase/header validation
// ============================================================================

using std::string;
using std::string_view;
using std::vector;

// Hex-encode a byte view (lowercase), one output char pair per input byte.
func xpy_hex_encode(data : *u8, len : size_t) : string {
    var out = string()
    var i : size_t = 0
    while(i < len) {
        out.append(test_nibble_to_hex(((data[i] as uint) >> 4) & 0xF))
        out.append(test_nibble_to_hex((data[i] as uint) & 0xF))
        i += 1
    }
    return out
}

// Convenience wrapper for strings.
func xpy_hex_encode_str(sv : string_view) : string {
    return xpy_hex_encode(sv.data() as *u8, sv.size())
}

// Write a seqsrv script: one hex-encoded response per line.
func xpy_write_seq_script(path : *char, lines : vector<string>) : bool {
    var content = string()
    var i : size_t = 0
    while(i < lines.size()) {
        var lp = lines.get_ptr(i)
        content.append_string(xpy_hex_encode_str(lp.to_view()))
        content.append('\n')
        i += 1
    }
    return test_write_file(path, content.data() as *u8, content.size())
}

// Deterministic i%256 pattern (full byte cycle — catches signed-char bugs).
func xpy_pattern256(len : usize) : string {
    var s = string()
    var i : usize = 0
    while(i < len) {
        s.append((i % 256) as char)
        i += 1
    }
    return s
}

// Generate a self-signed cert via the extra utils (foreground). Ensures the
// utility script exists in this process before invoking it.
func xpy_gen_cert(tag : string_view) {
    write_http_extra_py()
    var cp = xpy_cert_path(tag)
    var kp = xpy_key_path(tag)
    var gen = string("cert ")
    gen.append_view(cp.to_view())
    gen.append_view(" ")
    gen.append_view(kp.to_view())
    gen.append_view(" localhost")
    xpy_fg(gen.to_view())
}

func xpy_cert_path(tag : string_view) : string {
    var s = string("/tmp/xpy_")
    s.append_view(&tag)
    s.append_view("_cert.pem")
    return s
}

func xpy_key_path(tag : string_view) : string {
    var s = string("/tmp/xpy_")
    s.append_view(&tag)
    s.append_view("_key.pem")
    return s
}

// Launch `python /tmp/http_extra.py <args>` in the background after killing
// whatever occupies the port.
func xpy_bg(args : string_view) {
    test_ensure_tmp_dir()
    var redir = test_redir()
    var cmd = test_py_interp()
    cmd.append_view("/tmp/http_extra.py ")
    cmd.append_view(&args)
    cmd.append_view(redir.to_view())
    test_run_bg(cmd.data())
}

// Run the same in the foreground (cert generation etc).
func xpy_fg(args : string_view) {
    test_ensure_tmp_dir()
    var redir = test_redir()
    var cmd = test_py_interp()
    cmd.append_view("/tmp/http_extra.py ")
    cmd.append_view(&args)
    cmd.append_view(redir.to_view())
    system(cmd.data())
}

// Build "<mode> <cert> <key> <port>" command prefixes for the extra servers.
// Cert/key paths are materialized into locals first so no temporary string
// views escape their statement.
func xpy_server_cmd(mode : string_view, tag : string_view, port : uint) : string {
    var cp = xpy_cert_path(tag)
    var kp = xpy_key_path(tag)
    var cmd = string()
    cmd.append_view(&mode)
    cmd.append_view(" ")
    cmd.append_view(cp.to_view())
    cmd.append_view(" ")
    cmd.append_view(kp.to_view())
    cmd.append_view(" ")
    cmd.append_uinteger(port as ubigint)
    return cmd
}

func xpy_kill_and_wait(port : uint) {
    test_kill_port(port as int)
    test_server_wait()
}

// ----------------------------------------------------------------------------
// The python utility writer.
// ----------------------------------------------------------------------------
func write_http_extra_py() {
    test_ensure_tmp_dir()
    var py = std::string()

    py.append_view("import sys,ssl,socket,time\n")
    py.append_view("from cryptography import x509\n")
    py.append_view("from cryptography.x509.oid import NameOID\n")
    py.append_view("from cryptography.hazmat.primitives import hashes,serialization\n")
    py.append_view("from cryptography.hazmat.primitives.asymmetric import ec\n")

    // ---- cert generation ----
    py.append_view("def gen_cert(c,k,cn):\n")
    py.append_view("    key=ec.generate_private_key(ec.SECP256R1())\n")
    py.append_view("    nm=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,cn)])\n")
    py.append_view("    import datetime\n")
    py.append_view("    cert=(x509.CertificateBuilder().subject_name(nm).issuer_name(nm).public_key(key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(datetime.datetime.utcnow()).not_valid_after(datetime.datetime.utcnow()+datetime.timedelta(days=365)).sign(key,hashes.SHA256()))\n")
    py.append_view("    open(c,'wb').write(cert.public_bytes(serialization.Encoding.PEM))\n")
    py.append_view("    open(k,'wb').write(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.TraditionalOpenSSL,serialization.NoEncryption()))\n")

    // ---- shared server plumbing ----
    py.append_view("def mksrv(cert,key,port,nconn,to=15):\n")
    py.append_view("    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)\n")
    py.append_view("    ctx.load_cert_chain(cert,key)\n")
    py.append_view("    ctx.minimum_version=ssl.TLSVersion.TLSv1_2\n")
    py.append_view("    s=socket.socket()\n")
    py.append_view("    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)\n")
    py.append_view("    s.bind(('127.0.0.1',int(port)))\n")
    py.append_view("    s.listen(int(nconn))\n")
    py.append_view("    s.settimeout(to)\n")
    py.append_view("    return ctx,s\n")

    // Read a full HTTP request (headers + Content-Length body). Best effort.
    // Returns (method, path-and-query) parsed from the request line.
    py.append_view("def drain(t):\n")
    py.append_view("    data=b''\n")
    py.append_view("    try:\n")
    py.append_view("        t.settimeout(5)\n")
    py.append_view("        while b'\\r\\n\\r\\n' not in data:\n")
    py.append_view("            d=t.recv(4096)\n")
    py.append_view("            if not d:return ('GET','/')\n")
    py.append_view("            data+=d\n")
    py.append_view("        head,rest=data.split(b'\\r\\n\\r\\n',1)\n")
    py.append_view("        clen=0\n")
    py.append_view("        for h in head.split(b'\\r\\n'):\n")
    py.append_view("            if h.lower().startswith(b'content-length:'):\n")
    py.append_view("                clen=int(h.split(b':',1)[1].strip() or b'0')\n")
    py.append_view("        while len(rest)<clen:\n")
    py.append_view("            d=t.recv(65536)\n")
    py.append_view("            if not d:break\n")
    py.append_view("            rest+=d\n")
    py.append_view("    except Exception:\n")
    py.append_view("        return ('GET','/')\n")
    py.append_view("    line=data.split(b'\\r\\n',1)[0]\n")
    py.append_view("    parts=line.split(b' ')\n")
    py.append_view("    m=parts[0].decode() if parts else 'GET'\n")
    py.append_view("    p=parts[1].decode() if len(parts)>1 else '/'\n")
    py.append_view("    return (m,p)\n")

    // ---- seqsrv: scripted raw responses ----
    py.append_view("def seqsrv(cert,key,port,script,nconn):\n")
    py.append_view("    lines=[]\n")
    py.append_view("    for l in open(script):\n")
    py.append_view("        l=l.strip()\n")
    py.append_view("        if l:lines.append(bytes.fromhex(l))\n")
    py.append_view("    ctx,s=mksrv(cert,key,port,min(int(nconn),max(1,len(lines))))\n")
    py.append_view("    for i in range(min(int(nconn),len(lines))):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            drain(t)\n")
    py.append_view("            t.sendall(lines[i])\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('SEQSRV_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")

    // ---- dribblesrv: tiny writes with delays ----
    py.append_view("def dribblesrv(cert,key,port,total,chunk,delayms):\n")
    py.append_view("    ctx,s=mksrv(cert,key,port,1)\n")
    py.append_view("    try:\n")
    py.append_view("        c,a=s.accept()\n")
    py.append_view("        t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("        drain(t)\n")
    py.append_view("        n=int(total)\n")
    py.append_view("        payload=bytes([j%251 for j in range(n)])\n")
    py.append_view("        blob=b'HTTP/1.1 200 OK\\r\\nX-Dribble: yes\\r\\nContent-Length: '+str(n).encode()+b'\\r\\nConnection: close\\r\\n\\r\\n'+payload\n")
    py.append_view("        ch=max(1,int(chunk)); dl=float(delayms)/1000.0\n")
    py.append_view("        off=0\n")
    py.append_view("        while off<len(blob):\n")
    py.append_view("            t.sendall(blob[off:off+ch])\n")
    py.append_view("            time.sleep(dl)\n")
    py.append_view("            off+=ch\n")
    py.append_view("        t.close()\n")
    py.append_view("    except Exception as e:\n")
    py.append_view("        print('DRIBBLE_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")

    // ---- hdrsrv: many response headers ----
    py.append_view("def hdrsrv(cert,key,port,nconn,nhdrs,vlen):\n")
    py.append_view("    ctx,s=mksrv(cert,key,port,int(nconn))\n")
    py.append_view("    nh=int(nhdrs); vl=max(1,int(vlen)); val='v'*vl\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            drain(t)\n")
    py.append_view("            resp=b'HTTP/1.1 200 OK\\r\\n'\n")
    py.append_view("            for h in range(nh):\n")
    py.append_view("                resp+=('X-H-%d: %s\\r\\n'%(h,val)).encode()\n")
    py.append_view("            resp+=b'Content-Length: 4\\r\\nConnection: close\\r\\n\\r\\ndone'\n")
    py.append_view("            t.sendall(resp)\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('HDRSRV_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")

    // ---- redirsrv: 301 Location flow ----
    py.append_view("def redirsrv(cert,key,port,nconn):\n")
    py.append_view("    ctx,s=mksrv(cert,key,port,int(nconn))\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            m,p=drain(t)\n")
    py.append_view("            if p.startswith('/r1'):\n")
    py.append_view("                t.sendall(b'HTTP/1.1 301 Moved Permanently\\r\\nLocation: /r2\\r\\nContent-Length: 0\\r\\nConnection: close\\r\\n\\r\\n')\n")
    py.append_view("            else:\n")
    py.append_view("                t.sendall(b'HTTP/1.1 200 OK\\r\\nContent-Type: text/plain\\r\\nContent-Length: 12\\r\\nConnection: close\\r\\n\\r\\nfinal-target')\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('REDIR_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")

    // ---- pathsrv: echo "METHOD path?query" ----
    py.append_view("def pathsrv(cert,key,port,nconn):\n")
    py.append_view("    ctx,s=mksrv(cert,key,port,int(nconn))\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            m,p=drain(t)\n")
    py.append_view("            body=('{} {}'.format(m,p)).encode()\n")
    py.append_view("            t.sendall(b'HTTP/1.1 200 OK\\r\\nX-Echo-Method: '+m.encode()+b'\\r\\nContent-Length: '+str(len(body)).encode()+b'\\r\\nConnection: close\\r\\n\\r\\n'+body)\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('PATH_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")

    // ---- slowsrv: delayed response ----
    py.append_view("def slowsrv(cert,key,port,delay,nconn):\n")
    py.append_view("    ctx,s=mksrv(cert,key,port,int(nconn),to=float(delay)+20)\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            drain(t)\n")
    py.append_view("            time.sleep(float(delay))\n")
    py.append_view("            t.sendall(b'HTTP/1.1 200 OK\\r\\nContent-Length: 7\\r\\nConnection: close\\r\\n\\r\\nlate-ok')\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('SLOW_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")

    // ---- liessrv: truncated body (CL lies) ----
    py.append_view("def liessrv(cert,key,port,nconn):\n")
    py.append_view("    ctx,s=mksrv(cert,key,port,int(nconn))\n")
    py.append_view("    for i in range(int(nconn)):\n")
    py.append_view("        try:\n")
    py.append_view("            c,a=s.accept()\n")
    py.append_view("            t=ctx.wrap_socket(c,server_side=True)\n")
    py.append_view("            drain(t)\n")
    py.append_view("            body=bytes([j%251 for j in range(400)])\n")
    py.append_view("            t.sendall(b'HTTP/1.1 200 OK\\r\\nContent-Length: 1000\\r\\nConnection: close\\r\\n\\r\\n'+body)\n")
    py.append_view("            t.close()\n")
    py.append_view("        except Exception as e:\n")
    py.append_view("            print('LIES_ERR:'+str(e),file=sys.stderr)\n")
    py.append_view("    s.close()\n")

    // ---- client-side modes against a Chemical TLS server ----
    py.append_view("def mkcli():\n")
    py.append_view("    cc=ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)\n")
    py.append_view("    cc.check_hostname=False\n")
    py.append_view("    cc.verify_mode=ssl.CERT_NONE\n")
    py.append_view("    return cc\n")

    py.append_view("def conn(host,port,cc,to=20):\n")
    py.append_view("    import http.client\n")
    py.append_view("    return http.client.HTTPSConnection(host,int(port),context=cc,timeout=to)\n")

    py.append_view("def finish(ok,why,outfile):\n")
    py.append_view("    f=open(outfile,'w')\n")
    py.append_view("    f.write(('RESULT:OK' if ok else 'RESULT:FAIL '+why))\n")
    py.append_view("    f.close()\n")

    // Three sequential keep-alive GETs over ONE connection.
    py.append_view("def hcli3(host,port,outfile):\n")
    py.append_view("    try:\n")
    py.append_view("        cc=mkcli(); cn=conn(host,port,cc)\n")
    py.append_view("        ok=True; why=''\n")
    py.append_view("        for pth in ['/a','/bb','/ccc']:\n")
    py.append_view("            cn.request('GET',pth); r=cn.getresponse(); d=r.read()\n")
    py.append_view("            want=('P:'+pth).encode()\n")
    py.append_view("            if r.status!=200 or d!=want:\n")
    py.append_view("                ok=False; why+=' %s:%s/%r'%(pth,r.status,d[:30])\n")
    py.append_view("        cn.close(); finish(ok,'ka3'+why,outfile)\n")
    py.append_view("    except Exception as e:\n")
    py.append_view("        finish(False,'hcli3 EX '+str(e)[:80],outfile)\n")

    // HEAD: no body expected even when Content-Length is present.
    py.append_view("def hhead(host,port,outfile):\n")
    py.append_view("    try:\n")
    py.append_view("        cc=mkcli(); cn=conn(host,port,cc)\n")
    py.append_view("        cn.request('HEAD','/res'); r=cn.getresponse(); d=r.read()\n")
    py.append_view("        xs=r.getheader('X-Srv') or ''\n")
    py.append_view("        ok=(r.status==200 and d==b'' and r.reason=='OK' and xs=='head')\n")
    py.append_view("        why='status=%s reason=%s body=%r xs=%s'%(r.status,r.reason,d[:20],xs)\n")
    py.append_view("        cn.close(); finish(ok,why,outfile)\n")
    py.append_view("    except Exception as e:\n")
    py.append_view("        finish(False,'hhead EX '+str(e)[:80],outfile)\n")

    // Chunked request upload via a generator body (RFC allows TE: chunked).
    py.append_view("def hchunkup(host,port,outfile):\n")
    py.append_view("    try:\n")
    py.append_view("        cc=mkcli(); cn=conn(host,port,cc)\n")
    py.append_view("        def gen():\n")
    py.append_view("            yield b'chunk-one-'\n")
    py.append_view("            yield b'chunk-two!'\n")
    py.append_view("        cn.request('POST','/up',body=gen(),headers={'Content-Type':'text/plain'})\n")
    py.append_view("        r=cn.getresponse(); d=r.read()\n")
    py.append_view("        want=b'chunk-one-chunk-two!'\n")
    py.append_view("        ok=(r.status==200 and d==want)\n")
    py.append_view("        why='status=%s body=%r'%(r.status,d[:40])\n")
    py.append_view("        cn.close(); finish(ok,why,outfile)\n")
    py.append_view("    except Exception as e:\n")
    py.append_view("        finish(False,'hchunkup EX '+str(e)[:80],outfile)\n")

    // Strict response-shape validation from python's perspective.
    py.append_view("def hstrict(host,port,outfile):\n")
    py.append_view("    try:\n")
    py.append_view("        cc=mkcli(); cn=conn(host,port,cc)\n")
    py.append_view("        cn.request('GET','/strict'); r=cn.getresponse(); d=r.read()\n")
    py.append_view("        a=r.getheader('X-Alpha'); b=r.getheader('x-beta')\n")
    py.append_view("        ok=(r.status==201 and r.reason=='Totally Fine' and d==b'Body' and a=='one' and b=='two')\n")
    py.append_view("        why='st=%s rs=%r body=%r a=%s b=%s'%(r.status,r.reason,d[:20],a,b)\n")
    py.append_view("        cn.close(); finish(ok,why,outfile)\n")
    py.append_view("    except Exception as e:\n")
    py.append_view("        finish(False,'hstrict EX '+str(e)[:80],outfile)\n")

    // ---- dispatch ----
    py.append_view("cmd=sys.argv[1]\n")
    py.append_view("if cmd=='cert':\n")
    py.append_view("    gen_cert(sys.argv[2],sys.argv[3],sys.argv[4])\n")
    py.append_view("elif cmd=='seqsrv':\n")
    py.append_view("    seqsrv(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],int(sys.argv[6]))\n")
    py.append_view("elif cmd=='dribblesrv':\n")
    py.append_view("    dribblesrv(sys.argv[2],sys.argv[3],sys.argv[4],int(sys.argv[5]),int(sys.argv[6]),int(sys.argv[7]))\n")
    py.append_view("elif cmd=='hdrsrv':\n")
    py.append_view("    hdrsrv(sys.argv[2],sys.argv[3],sys.argv[4],int(sys.argv[5]),int(sys.argv[6]),int(sys.argv[7]))\n")
    py.append_view("elif cmd=='redirsrv':\n")
    py.append_view("    redirsrv(sys.argv[2],sys.argv[3],sys.argv[4],int(sys.argv[5]))\n")
    py.append_view("elif cmd=='pathsrv':\n")
    py.append_view("    pathsrv(sys.argv[2],sys.argv[3],sys.argv[4],int(sys.argv[5]))\n")
    py.append_view("elif cmd=='slowsrv':\n")
    py.append_view("    slowsrv(sys.argv[2],sys.argv[3],sys.argv[4],float(sys.argv[5]),int(sys.argv[6]))\n")
    py.append_view("elif cmd=='liessrv':\n")
    py.append_view("    liessrv(sys.argv[2],sys.argv[3],sys.argv[4],int(sys.argv[5]))\n")
    py.append_view("elif cmd=='hcli3':\n")
    py.append_view("    hcli3(sys.argv[2],sys.argv[3],sys.argv[4])\n")
    py.append_view("elif cmd=='hhead':\n")
    py.append_view("    hhead(sys.argv[2],sys.argv[3],sys.argv[4])\n")
    py.append_view("elif cmd=='hchunkup':\n")
    py.append_view("    hchunkup(sys.argv[2],sys.argv[3],sys.argv[4])\n")
    py.append_view("elif cmd=='hstrict':\n")
    py.append_view("    hstrict(sys.argv[2],sys.argv[3],sys.argv[4])\n")

    test_write_file("/tmp/http_extra.py\0" as *char, py.data() as *u8, py.size())
}
