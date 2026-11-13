// Tier 3: HTTP over a coroutine server. Binds the coroutine server through
// `Server.bind` + `serve_async` (its worker pool), issues a request with the
// buffered `http::Client` and checks the response, then shuts the server down.
//
// Port 19891 is this test's own; it used to share 19878 with INT_x25519_handshake
// in e2e.ch. Tests here run in parallel, so a shared port meant one test's
// `fuser -k` tore down the other's server.
using namespace std;

@test
@test.timeout(180000)
public func INT_http_async_loopback(env : &mut TestEnv) {
    unsafe {

    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:19891");

    var bound = server::Server.bind(cfg);
    if(bound is std::Result.Err) { env.error("bind failed on 19891"); return }
    var Ok(bound_server) = bound else unsafe { unreachable }
    var srv : server::Server = bound_server;

    srv.router.add("GET", "/hello", ||(req) => {
        return http::response_text(200u, "world")
    });

    var stop = srv.handle();
    var thread = srv.serve_async();
    // Let the server start accepting before the client dials.
    std::concurrent::sleep_ms(100u);

    // The async client entry points were removed together with the raw HTTP
    // namespace; `http::Client` already buffers the whole response, so the
    // loopback check is identical while the server keeps serving on its pool.
    var client = http::Client();
    var res = client.get("http://127.0.0.1:19891/hello");
    if(res is std::Result.Err) {
        env.error("async http request failed")
    } else {
        var Ok(r) = res else unsafe { unreachable }
        if(r.status != 200u) {
            env.error("async http status mismatch")
        }
        var body = r.body.text();
        if(body is std::Option.None) {
            env.error("async http body missing")
        } else {
            var Some(bv) = body else unsafe { unreachable }
            if(bv.size() != 5u) {
                env.error("async http body size mismatch")
            }
        }
    }

    stop.shutdown();
    thread.join();

}
}
