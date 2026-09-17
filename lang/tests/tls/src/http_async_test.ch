// Tier 3: async HTTP over a coroutine server. Starts the coroutine accept loop
// (`server::serve_coro`), issues a request with the async client
// (`http::get_async`) and checks the response, then shuts the server down.
using namespace std;

@test
@test.timeout(60000)
public func INT_http_async_loopback(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:19878");
    var srv = server::Server(cfg);
    srv.router.add("GET", "/hello", ||(req, res) => {
        res.write_string(std::string::make_no_len("world"));
    });

    var server_f = async::spawn<int>(server::serve_coro(&raw mut srv, 19878u));
    // Let the server bind + start accepting before the client dials.
    var unit = async::block_on<core::async::Unit>(async::yield_now());

    var client = http::Client();
    var url = std::string_view("http://127.0.0.1:19878/hello");
    var box = async::block_on<*mut http::HttpResult>(http::get_async(&raw mut client, &raw mut url));
    if(box == null || !box.is_ok()) {
        env.error("async http request failed")
    } else {
        if(box.status_code() != 200u) {
            env.error("async http status mismatch")
        }
        var body = box.body_view();
        if(body.size() != 5u) {
            env.error("async http body size mismatch")
        }
    }
    if(box != null) {
        delete box
    }

    srv.shutdown();
    var r = async::block_on<int>(server_f);
    if(r != 0) {
        env.error("serve_coro returned non-zero")
    }
}
