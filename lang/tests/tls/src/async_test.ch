// Tier 2: async TLS wrappers. Drives a real TLS 1.3 handshake + request over
// the async API (`tls_connect_async`, `ssl_write_async`, `ssl_read_async`) and
// checks it produces the same result as the synchronous e2e tests.
using namespace tls
using std::string_view

@test
@test.timeout(60000)
public func INT_tls13_client_async(env : &mut TestEnv) {
    write_tls_python_utils()
    test_kill_port(19877)
    test_server_wait()
    test_py_run_foreground(string_view("cert /tmp/tls_19877_cert.pem /tmp/tls_19877_key.pem test.example.com ec"))
    test_py_run_background(string_view("srv /tmp/tls_19877_cert.pem /tmp/tls_19877_key.pem 19877 1.3"))
    test_server_wait()

    var ctx : SSLContext
    ssl_init(unsafe(&raw mut ctx))
    var config = ssl_config_init(SSL_IS_CLIENT)
    config.authmode = SSL_VERIFY_NONE
    config.max_tls_version = SSL_VERSION_TLS1_3
    ssl_set_config(unsafe(&raw mut ctx), &raw mut config)

    var ret = async::block_on<int>(tls_connect_async(unsafe(&raw mut ctx), "127.0.0.1", 19877u))
    if(ret < 0) {
        env.error("TLS13 async: handshake failed")
        ssl_free(unsafe(&raw mut ctx))
        test_kill_port(19877)
        return
    }

    var req = "GET / HTTP/1.0\r\n\r\n"
    var w = async::block_on<int>(ssl_write_async(unsafe(&raw mut ctx), req as *u8, 18))
    if(w != 18) {
        env.error("TLS13 async: write failed")
    }

    var buf : [512]u8
    var n = async::block_on<int>(ssl_read_async(unsafe(&raw mut ctx), &raw mut buf[0], 512))
    if(n != 2 || buf[0] != 79 || buf[1] != 75) {
        env.error("TLS13 async: app-data response mismatch")
    }

    ssl_close_notify(unsafe(&raw mut ctx))
    ssl_free(unsafe(&raw mut ctx))
    test_kill_port(19877)
}
