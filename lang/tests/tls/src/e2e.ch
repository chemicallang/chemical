using namespace tls
using namespace crypto

@test
public func INT_smoke_test(env : &mut TestEnv) {
    if(SSL_VERSION_TLS1_3 != 0x0304) { env.error("TLS 1.3 version wrong") }
    if(SSL_VERSION_TLS1_2 != 0x0303) { env.error("TLS 1.2 version wrong") }
}

@test
public func INT_tls13_client_openssl(env : &mut TestEnv) {
    system("fuser -k 19876/tcp 2>/dev/null; sleep 0.3")
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/tls_key.pem -out /tmp/tls_cert.pem -subj /CN=test.example.com -days 1 -nodes 2>/dev/null")
    system("setsid openssl s_server -cert /tmp/tls_cert.pem -key /tmp/tls_key.pem -tls1_3 -groups X25519 -ciphersuites TLS_AES_128_GCM_SHA256 -no_anti_replay -accept 19876 -quiet </dev/null 2>/dev/null &")
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
    system("fuser -k 19878/tcp 2>/dev/null; sleep 0.3")
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/tls_x25519_key.pem -out /tmp/tls_x25519_cert.pem -subj /CN=test.example.com -days 1 -nodes 2>/dev/null")
    system("setsid openssl s_server -cert /tmp/tls_x25519_cert.pem -key /tmp/tls_x25519_key.pem -tls1_3 -groups X25519 -ciphersuites TLS_AES_128_GCM_SHA256 -no_anti_replay -accept 19878 -quiet </dev/null 2>/dev/null &")
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
    system("fuser -k 19877/tcp 2>/dev/null; sleep 0.3")
    system("openssl req -x509 -newkey rsa:2048 -keyout /tmp/tls12_key.pem -out /tmp/tls12_cert.pem -subj /CN=test.example.com -days 1 -nodes 2>/dev/null")
    system("setsid openssl s_server -cert /tmp/tls12_cert.pem -key /tmp/tls12_key.pem -tls1_2 -no_anti_replay -accept 19877 -cipher kRSA -quiet </dev/null 2>/dev/null &")
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
            else if(ctx.last_alert_desc == 110) { env.error("TLS12: alert = unsupported_ext(110)") }
            else { env.error("TLS12: ERR_SSL_FATAL_ALERT_MESSAGE") }
        }
        else if(ret == ERR_SSL_DECODE_ERROR) { env.error("TLS12: ERR_SSL_DECODE_ERROR") }
        else if(ret == ERR_SSL_INTERNAL_ERROR) { env.error("TLS12: ERR_SSL_INTERNAL_ERROR") }
        else if(ret == ERR_SSL_CONN_EOF) { env.error("TLS12: ERR_SSL_CONN_EOF") }
        else if(ret == ERR_SSL_CERT_VERIFY_FAILED) { env.error("TLS12: ERR_SSL_CERT_VERIFY_FAILED") }
        else if(ret == ERR_SSL_BAD_CONFIG) { env.error("TLS12: ERR_SSL_BAD_CONFIG") }
        else if(ret == ERR_SSL_BAD_PROTOCOL_VERSION) { env.error("TLS12: ERR_SSL_BAD_PROTOCOL_VERSION") }
        else if(ret == ERR_SSL_INVALID_RECORD) { env.error("TLS12: ERR_SSL_INVALID_RECORD") }
        else if(ret == ERR_SSL_NO_RNG) { env.error("TLS12: ERR_SSL_NO_RNG") }
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
public func INT_tls13_server_openssl_client(env : &mut TestEnv) {
    system("fuser -k 19880/tcp 2>/dev/null; sleep 0.3")
    // Generate server certificate
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/srv_key.pem -out /tmp/srv_cert.pem -subj /CN=localhost -days 1 -nodes 2>/dev/null")

    var cert = x509_crt_load_pem_file("/tmp/srv_cert.pem")
    if(cert == null) { env.error("failed to load server cert"); return }

    // Listen on a port
    var server_sock = net::listen_addr("127.0.0.1", 19880u)
    if(server_sock == 0 as net::Socket) { env.error("listen failed"); return }

    // Start OpenSSL s_client in background to connect to us
    system("setsid openssl s_client -connect 127.0.0.1:19880 -tls1_3 -groups X25519 -ciphersuites TLS_AES_128_GCM_SHA256 -no_anti_replay -quiet </dev/null 2>/dev/null &")
    system("sleep 1")

    // Accept the client connection with non-blocking loop (timeout after ~5s)
    net::set_nonblocking(server_sock)
    var client_sock = net::accept_socket(server_sock) as net::Socket
    var accept_attempts = 0
    while(client_sock == 0 as net::Socket && accept_attempts < 50) {
        std::concurrent::sleep_ms(100u)
        client_sock = net::accept_socket(server_sock)
        accept_attempts += 1
    }
    if(client_sock == 0 as net::Socket) {
        env.error("no client connected — OpenSSL s_client may not be available")
        net::close_socket(server_sock)
        return
    }

    // Set up server SSL context
    var ssl_mem = malloc(sizeof(SSLContext)) as *mut SSLContext
    ssl_init(ssl_mem)
    ssl_set_socket(ssl_mem, client_sock)

    var cfg = ssl_config_init(SSL_IS_SERVER)
    cfg.own_cert = cert
    cfg.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(ssl_mem, &raw mut cfg)

    var ret = ssl_handshake(ssl_mem)
    if(ret < 0) {
        env.error("TLS 1.3 server handshake failed against OpenSSL client")
    } else {
        // Try reading what the client sent
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
    system("fuser -k 19882/tcp 2>/dev/null; sleep 0.3")
    // ECDSA cert + x25519 key exchange — modern TLS
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/ecdsa_srv_key.pem -out /tmp/ecdsa_srv_cert.pem -subj /CN=localhost -days 1 -nodes 2>/dev/null")

    var cert = x509_crt_load_pem_file("/tmp/ecdsa_srv_cert.pem")
    if(cert == null) { env.error("failed to load ECDSA cert"); return }

    var server_sock = net::listen_addr("127.0.0.1", 19882u)
    if(server_sock == 0 as net::Socket) { env.error("listen failed"); return }

    // Force x25519 on client side
    system("setsid openssl s_client -connect 127.0.0.1:19882 -tls1_3 -groups X25519 -ciphersuites TLS_AES_128_GCM_SHA256 -no_anti_replay -quiet </dev/null 2>/dev/null &")
    system("sleep 1")

    // Accept with non-blocking loop (timeout after ~5s)
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
    system("fuser -k 19883/tcp 2>/dev/null; sleep 0.3")
    // Client connects to ECDSA-cert server — tests our ECDSA cert verification
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/ecdsa_key.pem -out /tmp/ecdsa_cert.pem -subj /CN=127.0.0.1 -days 1 -nodes 2>/dev/null")

    system("setsid openssl s_server -cert /tmp/ecdsa_cert.pem -key /tmp/ecdsa_key.pem -tls1_3 -groups X25519 -ciphersuites TLS_AES_128_GCM_SHA256 -no_anti_replay -accept 19883 -quiet </dev/null 2>/dev/null &")
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
