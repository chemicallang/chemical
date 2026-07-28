using namespace tls
using namespace crypto

@test
public func INT_smoke_test(env : &mut TestEnv) {
    if(SSL_VERSION_TLS1_3 != 0x0304) { env.error("TLS 1.3 version wrong") }
    if(SSL_VERSION_TLS1_2 != 0x0303) { env.error("TLS 1.2 version wrong") }
}

@test
public func INT_tls13_client(env : &mut TestEnv) {
    write_tls_python_utils()
    system("fuser -k 19876/tcp 2>/dev/null; sleep 0.3")
    system("python3 /tmp/tls_utils.py cert /tmp/tls_19876_cert.pem /tmp/tls_19876_key.pem test.example.com ec")
    system("setsid python3 /tmp/tls_utils.py srv /tmp/tls_19876_cert.pem /tmp/tls_19876_key.pem 19876 1.3 &")
    system("sleep 1")

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19876u)
    if(ret < 0) {
        if(ret == ERR_SSL_HANDSHAKE_FAILURE) { env.error("TLS13: ERR_SSL_HANDSHAKE_FAILURE") }
        else if(ret == ERR_SSL_UNEXPECTED_MESSAGE) { env.error("TLS13: ERR_SSL_UNEXPECTED_MESSAGE") }
        else if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("TLS13: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("TLS13: alert = protocol_version(70)") }
            else if(ctx.last_alert_desc == 47) { env.error("TLS13: alert = illegal_parameter(47)") }
            else if(ctx.last_alert_desc == 50) { env.error("TLS13: alert = decode_error(50)") }
            else if(ctx.last_alert_desc == 51) { env.error("TLS13: alert = decrypt_error(51)") }
            else if(ctx.last_alert_desc == 10) { env.error("TLS13: alert = unexpected_message(10)") }
            else if(ctx.last_alert_desc == 86) { env.error("TLS13: alert = inappropriate_fallback(86)") }
            else if(ctx.last_alert_desc == 110) { env.error("TLS13: alert = unsupported_ext(110)") }
            else if(ctx.last_alert_desc == 112) { env.error("TLS13: alert = unrecognized_name(112)") }
            else { env.error("TLS13: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("TLS13: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("TLS13: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("TLS13: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("TLS13: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("TLS13: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("TLS13: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("TLS13: ERR_SSL_INVALID_RECORD") }
        else if(ret == ERR_SSL_NO_RNG) { env.error("TLS13: ERR_SSL_NO_RNG") }
        else { env.error("TLS13: unknown error") }
    } else {
        var req = "GET / HTTP/1.0\r\n\r\n"
        ssl_write(&raw mut ctx, req as *u8, 18)
        var buf : [512]u8
        ssl_read(&raw mut ctx, &raw mut buf[0], 512)
        ssl_close_notify(&raw mut ctx)
    }
    ssl_free(&raw mut ctx)
    system("fuser -k 19876/tcp 2>/dev/null")
}

@test
public func INT_x25519_handshake(env : &mut TestEnv) {
    write_tls_python_utils()
    system("fuser -k 19878/tcp 2>/dev/null; sleep 0.3")
    system("python3 /tmp/tls_utils.py cert /tmp/tls_19878_cert.pem /tmp/tls_19878_key.pem test.example.com ec")
    system("setsid python3 /tmp/tls_utils.py srv /tmp/tls_19878_cert.pem /tmp/tls_19878_key.pem 19878 1.3 &")
    system("sleep 1")

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19878u)
    if(ret < 0) {
        if(ret == ERR_SSL_HANDSHAKE_FAILURE) { env.error("X25519: ERR_SSL_HANDSHAKE_FAILURE") }
        else if(ret == ERR_SSL_UNEXPECTED_MESSAGE) { env.error("X25519: ERR_SSL_UNEXPECTED_MESSAGE") }
        else if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("X25519: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("X25519: alert = protocol_version(70)") }
            else if(ctx.last_alert_desc == 47) { env.error("X25519: alert = illegal_parameter(47)") }
            else if(ctx.last_alert_desc == 50) { env.error("X25519: alert = decode_error(50)") }
            else if(ctx.last_alert_desc == 51) { env.error("X25519: alert = decrypt_error(51)") }
            else if(ctx.last_alert_desc == 110) { env.error("X25519: alert = unsupported_ext(110)") }
            else { env.error("X25519: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("X25519: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("X25519: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("X25519: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("X25519: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("X25519: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("X25519: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("X25519: ERR_SSL_INVALID_RECORD") }
        else if(ret == ERR_SSL_NO_RNG) { env.error("X25519: ERR_SSL_NO_RNG") }
        else { env.error("X25519: unknown error") }
    }
    ssl_free(&raw mut ctx)
    system("fuser -k 19878/tcp 2>/dev/null")
}

@test
public func INT_tls12_client(env : &mut TestEnv) {
    write_tls_python_utils()
    system("fuser -k 19877/tcp 2>/dev/null; sleep 0.3")
    system("python3 /tmp/tls_utils.py cert /tmp/tls_19877_cert.pem /tmp/tls_19877_key.pem test.example.com rsa")
    system("setsid python3 /tmp/tls_utils.py srv /tmp/tls_19877_cert.pem /tmp/tls_19877_key.pem 19877 1.2 &")
    system("sleep 1")

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_2
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19877u)
    if(ret < 0) {
        if(ret == ERR_SSL_HANDSHAKE_FAILURE) { env.error("TLS12: ERR_SSL_HANDSHAKE_FAILURE") }
        else if(ret == ERR_SSL_UNEXPECTED_MESSAGE) { env.error("TLS12: ERR_SSL_UNEXPECTED_MESSAGE") }
        else if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("TLS12: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("TLS12: alert = protocol_version(70)") }
            else if(ctx.last_alert_desc == 47) { env.error("TLS12: alert = illegal_parameter(47)") }
            else if(ctx.last_alert_desc == 50) { env.error("TLS12: alert = decode_error(50)") }
            else if(ctx.last_alert_desc == 51) { env.error("TLS12: alert = decrypt_error(51)") }
            else { env.error("TLS12: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("TLS12: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("TLS12: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("TLS12: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("TLS12: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("TLS12: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("TLS12: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("TLS12: ERR_SSL_INVALID_RECORD") }
        else { env.error("TLS12: unknown error") }
    }
    ssl_free(&raw mut ctx)
    system("fuser -k 19877/tcp 2>/dev/null")
}

@test
public func INT_system_ca_bundle(env : &mut TestEnv) {
    var ca = load_system_ca_bundle()
    if(ca == null) {
        env.error("no system CA bundle found")
    }
}

@test
public func INT_tls13_server_client(env : &mut TestEnv) {
    write_tls_python_utils()
    system("fuser -k 19880/tcp 2>/dev/null; sleep 0.3")
    system("python3 /tmp/tls_utils.py cert /tmp/tls_19880_cert.pem /tmp/tls_19880_key.pem localhost ec")

    var cert = x509_crt_load_pem_file("/tmp/tls_19880_cert.pem")
    if(cert == null) { env.error("failed to load server cert"); return }

    var server_sock = net::listen_addr("127.0.0.1", 19880u)
    if(server_sock == 0 as net::Socket) { env.error("listen failed"); return }

    system("setsid python3 /tmp/tls_utils.py cli 127.0.0.1 19880 1.3 &")
    system("sleep 1")

    net::set_nonblocking(server_sock)
    var client_sock = net::accept_socket(server_sock) as net::Socket
    var accept_attempts = 0
    while(client_sock == 0 as net::Socket && accept_attempts < 50) {
        std::concurrent::sleep_ms(100u)
        client_sock = net::accept_socket(server_sock)
        accept_attempts += 1
    }
    if(client_sock == 0 as net::Socket) {
        env.error("no client connected")
        net::close_socket(server_sock)
        return
    }

    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    ssl_init(ssl_mem)
    ssl_set_socket(ssl_mem, client_sock)

    var cfg = ssl_config_init(SSL_IS_SERVER)
    cfg.own_cert = cert
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(ssl_mem, &raw mut cfg)

    var ret = ssl_handshake(ssl_mem)
    if(ret < 0) {
        env.error("TLS 1.3 server handshake failed against Python client")
    } else {
        var buf : [512]u8
        ssl_read(ssl_mem, &raw mut buf[0], 512)
        ssl_close_notify(ssl_mem)
    }

    ssl_free(ssl_mem)
    unsafe { dealloc ssl_mem }
    net::close_socket(server_sock)
    system("fuser -k 19880/tcp 2>/dev/null")
}

@test
public func INT_ecdsa_server_client_x25519(env : &mut TestEnv) {
    write_tls_python_utils()
    system("fuser -k 19882/tcp 2>/dev/null; sleep 0.3")
    system("python3 /tmp/tls_utils.py cert /tmp/tls_19882_cert.pem /tmp/tls_19882_key.pem localhost ec")

    var cert = x509_crt_load_pem_file("/tmp/tls_19882_cert.pem")
    if(cert == null) { env.error("failed to load ECDSA cert"); return }

    var server_sock = net::listen_addr("127.0.0.1", 19882u)
    if(server_sock == 0 as net::Socket) { env.error("listen failed"); return }

    system("setsid python3 /tmp/tls_utils.py cli 127.0.0.1 19882 1.3 &")
    system("sleep 1")

    net::set_nonblocking(server_sock)
    var client_sock = net::accept_socket(server_sock) as net::Socket
    var accept_attempts = 0
    while(client_sock == 0 as net::Socket && accept_attempts < 50) {
        std::concurrent::sleep_ms(100u)
        client_sock = net::accept_socket(server_sock)
        accept_attempts += 1
    }
    if(client_sock == 0 as net::Socket) { env.error("no client"); net::close_socket(server_sock); return }

    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    ssl_init(ssl_mem)
    ssl_set_socket(ssl_mem, client_sock)
    var cfg = ssl_config_init(SSL_IS_SERVER)
    cfg.own_cert = cert
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(ssl_mem, &raw mut cfg)

    var ret = ssl_handshake(ssl_mem)
    if(ret < 0) {
        env.error("ECDSA cert + x25519 server handshake failed")
    }
    ssl_free(ssl_mem)
    unsafe { dealloc ssl_mem }
    net::close_socket(server_sock)
    system("fuser -k 19882/tcp 2>/dev/null")
}

@test
public func INT_ecdsa_client_handshake(env : &mut TestEnv) {
    write_tls_python_utils()
    system("fuser -k 19883/tcp 2>/dev/null; sleep 0.3")
    system("python3 /tmp/tls_utils.py cert /tmp/tls_19883_cert.pem /tmp/tls_19883_key.pem 127.0.0.1 ec")
    system("setsid python3 /tmp/tls_utils.py srv /tmp/tls_19883_cert.pem /tmp/tls_19883_key.pem 19883 1.3 &")
    system("sleep 1")

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19883u)
    if(ret < 0) {
        if(ret == ERR_SSL_HANDSHAKE_FAILURE) { env.error("ECDSA: ERR_SSL_HANDSHAKE_FAILURE") }
        else if(ret == ERR_SSL_UNEXPECTED_MESSAGE) { env.error("ECDSA: ERR_SSL_UNEXPECTED_MESSAGE") }
        else if(ret == ERR_SSL_FATAL_ALERT_MESSAGE) {
            if(ctx.last_alert_desc == 40) { env.error("ECDSA: alert = handshake_failure(40)") }
            else if(ctx.last_alert_desc == 70) { env.error("ECDSA: alert = protocol_version(70)") }
            else if(ctx.last_alert_desc == 47) { env.error("ECDSA: alert = illegal_parameter(47)") }
            else if(ctx.last_alert_desc == 50) { env.error("ECDSA: alert = decode_error(50)") }
            else if(ctx.last_alert_desc == 51) { env.error("ECDSA: alert = decrypt_error(51)") }
            else if(ctx.last_alert_desc == 110) { env.error("ECDSA: alert = unsupported_ext(110)") }
            else { env.error("ECDSA: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("ECDSA: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("ECDSA: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("ECDSA: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("ECDSA: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("ECDSA: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("ECDSA: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("ECDSA: ERR_SSL_INVALID_RECORD") }
        else if(ret == ERR_SSL_NO_RNG) { env.error("ECDSA: ERR_SSL_NO_RNG") }
        else { env.error("ECDSA: unknown error") }
    }
    ssl_free(&raw mut ctx)
    system("fuser -k 19883/tcp 2>/dev/null")
}
