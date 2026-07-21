using namespace tls

@test
public func INT_smoke_test(env : &mut TestEnv) {
    if(SSL_VERSION_TLS1_3 != 0x0304) { env.error("TLS 1.3 version wrong") }
    if(SSL_VERSION_TLS1_2 != 0x0303) { env.error("TLS 1.2 version wrong") }
}

@test
public func INT_tls13_client_openssl(env : &mut TestEnv) {
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/tls_key.pem -out /tmp/tls_cert.pem -subj /CN=test.example.com -days 1 -nodes 2>/dev/null")

    system("openssl s_server -cert /tmp/tls_cert.pem -key /tmp/tls_key.pem -tls1_3 -no_anti_replay -accept 19876 -quiet 2>/dev/null &")
    system("sleep 1")

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19876u)
    if(ret < 0) {
        env.error("TLS 1.3 handshake failed against OpenSSL server")
    } else {
        var req = "GET / HTTP/1.0\r\n\r\n"
        ssl_write(&raw mut ctx, req as *u8, 18)
        var buf : [512]u8
        ssl_read(&raw mut ctx, &raw mut buf[0], 512)
        ssl_close_notify(&raw mut ctx)
    }
    ssl_free(&raw mut ctx)
    system("pkill -f 'openssl s_server.*19876' 2>/dev/null")
}

@test
public func INT_x25519_handshake(env : &mut TestEnv) {
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/tls_x25519_key.pem -out /tmp/tls_x25519_cert.pem -subj /CN=test.example.com -days 1 -nodes 2>/dev/null")

    system("openssl s_server -cert /tmp/tls_x25519_cert.pem -key /tmp/tls_x25519_key.pem -groups X25519 -tls1_3 -no_anti_replay -accept 19878 -quiet 2>/dev/null &")
    system("sleep 1")

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19878u)
    if(ret < 0) {
        env.error("x25519 handshake failed against OpenSSL")
    }
    ssl_free(&raw mut ctx)
    system("pkill -f 'openssl s_server.*19878' 2>/dev/null")
}

@test
public func INT_tls12_client(env : &mut TestEnv) {
    system("openssl req -x509 -newkey rsa:2048 -keyout /tmp/tls12_key.pem -out /tmp/tls12_cert.pem -subj /CN=test.example.com -days 1 -nodes 2>/dev/null")

    system("openssl s_server -cert /tmp/tls12_cert.pem -key /tmp/tls12_key.pem -tls1_2 -no_anti_replay -accept 19877 -quiet 2>/dev/null &")
    system("sleep 1")

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.max_tls_version = SSL_VERSION_TLS1_2
    ssl_set_config(&raw mut ctx, &raw mut config)

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19877u)
    if(ret < 0) {
        env.error("TLS 1.2 handshake failed against OpenSSL")
    }
    ssl_free(&raw mut ctx)
    system("pkill -f 'openssl s_server.*19877' 2>/dev/null")
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
    // Generate server certificate
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/srv_key.pem -out /tmp/srv_cert.pem -subj /CN=localhost -days 1 -nodes 2>/dev/null")

    var cert = x509_crt_load_pem_file("/tmp/srv_cert.pem")
    if(cert == null) { env.error("failed to load server cert"); return }

    // Listen on a port
    var server_sock = net::listen_addr("127.0.0.1", 19880u)
    if(server_sock == 0 as net::Socket) { env.error("listen failed"); return }

    // Start OpenSSL s_client in background to connect to us
    system("openssl s_client -connect 127.0.0.1:19880 -tls1_3 -no_anti_replay -quiet 2>/dev/null </dev/null &")
    system("sleep 1")

    // Accept the client connection
    var client_sock = net::accept_socket(server_sock)
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
    system("pkill -f 'openssl s_client.*19880' 2>/dev/null")
}

@test
public func INT_ecdsa_server_client_x25519(env : &mut TestEnv) {
    // ECDSA cert + x25519 key exchange — modern TLS
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/ecdsa_srv_key.pem -out /tmp/ecdsa_srv_cert.pem -subj /CN=localhost -days 1 -nodes 2>/dev/null")

    var cert = x509_crt_load_pem_file("/tmp/ecdsa_srv_cert.pem")
    if(cert == null) { env.error("failed to load ECDSA cert"); return }

    var server_sock = net::listen_addr("127.0.0.1", 19882u)
    if(server_sock == 0 as net::Socket) { env.error("listen failed"); return }

    // Force x25519 on client side
    system("openssl s_client -connect 127.0.0.1:19882 -groups X25519 -tls1_3 -no_anti_replay -quiet 2>/dev/null </dev/null &")
    system("sleep 1")

    var client_sock = net::accept_socket(server_sock)
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
    system("pkill -f 'openssl s_client.*19882' 2>/dev/null")
}

@test
public func INT_ecdsa_client_handshake(env : &mut TestEnv) {
    // Client connects to ECDSA-cert server — tests our ECDSA cert verification
    system("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -keyout /tmp/ecdsa_key.pem -out /tmp/ecdsa_cert.pem -subj /CN=127.0.0.1 -days 1 -nodes 2>/dev/null")

    system("openssl s_server -cert /tmp/ecdsa_cert.pem -key /tmp/ecdsa_key.pem -tls1_3 -no_anti_replay -accept 19883 -quiet 2>/dev/null &")
    system("sleep 1")

    var ctx : SSLContext; ssl_init(&raw mut ctx)
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(&raw mut ctx, &raw mut config)

    // Load the server's cert as trusted CA (self-signed)
    var ca = x509_crt_load_pem_file("/tmp/ecdsa_cert.pem")
    if(ca != null) { ssl_set_ca_chain(&raw mut config, ca) }

    var ret = tls_connect(&raw mut ctx, "127.0.0.1", 19883u)
    if(ret < 0) {
        env.error("ECDSA client handshake failed")
    }
    ssl_free(&raw mut ctx)
    system("pkill -f 'openssl s_server.*19883' 2>/dev/null")
}
