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
