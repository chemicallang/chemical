using namespace std;

@test
@test.timeout(180000)
public func INT_safe_tls_client(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(20200)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_20200_cert.pem /tmp/tls_20200_key.pem localhost ec"))
    test_py_run_background(string_view("srv /tmp/tls_20200_cert.pem /tmp/tls_20200_key.pem 20200 1.3"))
    test_server_wait()

    var stream = tls::connect_insecure("127.0.0.1", 20200u16)
    if(stream is Result.Err) {
        env.error("safe TLS client connect failed")
    } else {
        var Ok(s) = stream else unsafe { unreachable }
        var req : [18]u8 = ['G','E','T',' ','/',' ','H','T','T','P','/','1','.','0','\r','\n','\r','\n']
        var req_view : [u8] = req
        var wr = s.write_all(req_view)
        if(wr is Result.Err) { env.error("safe TLS client write failed") }
        var buf : [64]u8 = [0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8]
        var buf_view : &mut [u8] = &mut buf
        var rr = s.read(buf_view)
        if(rr is Result.Err) { env.error("safe TLS client read failed") }
    }
    test_kill_port(20200)
}

@test
@test.timeout(180000)
public func INT_safe_tls_server(env : &mut TestEnv) {
    write_tls_python_utils()
    // 20220 is reserved for this test; do not run fuser here. Killing every
    // process that once used the port can also signal this test process when a
    // previous run is still draining, which the runner reports as exit 9.
    test_py_run_foreground(string_view("cert /tmp/tls_20220_cert.pem /tmp/tls_20220_key.pem localhost rsa"))
    test_py_run_foreground(string_view("privkey /tmp/tls_20220_key.pem /tmp/tls_20220_priv.txt"))

    var certs = tls::load_certificate("/tmp/tls_20220_cert.pem")
    if(certs is Result.Err) { env.error("safe TLS cert load failed"); return }
    var Ok(chain) = certs else unsafe { unreachable }

    var n_buf : [512]u8
    var d_buf : [512]u8
    var n_len : size_t = 0
    var d_len : size_t = 0
    test_parse_n_d_hex_file("/tmp/tls_20220_priv.txt\0" as *char,
                            &raw mut n_buf[0], 512, &raw mut n_len,
                            &raw mut d_buf[0], 512, &raw mut d_len)
    if(n_len == 0 || d_len == 0) { env.error("safe TLS server key parse failed"); return }
    var n_view = unsafe(std::span<u8>(&raw mut n_buf[0], n_len))
    var d_view = unsafe(std::span<u8>(&raw mut d_buf[0], d_len))
    var keyr = tls::rsa_private_key_from_components(n_view, d_view)
    if(keyr is Result.Err) { env.error("safe TLS key import failed"); return }
    var Ok(key) = keyr else unsafe { unreachable }

    var listener = net::bind_tcp("127.0.0.1", 20220u16)
    if(listener is Result.Err) { env.error("safe TLS listener failed"); return }
    var Ok(l) = listener else unsafe { unreachable }

    var cli_cmd = test_py_interp()
    cli_cmd.append_view("/tmp/tls_utils.py cli 127.0.0.1 20220 1.2 AES128-GCM-SHA256:@SECLEVEL=0 2>/tmp/tls_safe_cli_err.txt")
    test_run_bg_server(cli_cmd.data(), 20220u16)
    test_server_wait()

    var accepted = l.accept()
    if(accepted is Result.Err) { env.error("safe TCP accept failed"); return }
    var Ok(tcp) = accepted else unsafe { unreachable }
    var cfg = tls::server_config(&chain, &key)
    var tlsr = tls::accept(tcp, &cfg)
    if(tlsr is Result.Err) {
        env.error("safe TLS server accept failed")
    } else {
        var Ok(s) = tlsr else unsafe { unreachable }
        var buf : [18]u8 = [0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8,0u8]
        var view : &mut [u8] = &mut buf
        var rr = s.read_exact(view)
        if(rr is Result.Err) { env.error("safe TLS server read failed") }
    }
    l.close()
}
