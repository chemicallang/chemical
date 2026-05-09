using namespace std;
using namespace net;

// Waits for the server to be ready on the given port
func wait_for_server_ready(env: &mut TestEnv, port: uint, srv: *mut server::Server, thread: *mut std.concurrent.Thread) {
    var ready = false;
    var attempts = 0u;
    while(!ready && attempts < 50u) {
        var s = net::dial("127.0.0.1", port);
        if(s != 0u && (s as longlong) > 0) {
            net::close_socket(s);
            ready = true;
        } else {
            std.concurrent.sleep_ms(10u);
            attempts = attempts + 1u;
        }
    }
    if(!ready) {
        env.error("Server did not become ready");
        srv.shutdown();
        thread.join();
        return;
    }
}

@test
func test_http_get(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8081");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/hello", ||(req, res) => {
        res.write_string(std::string::make_no_len("world"));
    });
    
    var thread = srv.serve_async(8081u);
    wait_for_server_ready(env, 8081u, &mut srv, &mut thread);
    var client = net::Client();
    var res_result = client.get("http://127.0.0.1:8081/hello");
    
    if(res_result is Result.Err) {
        var Err(e) = res_result else return;
        env.error(e.data());
        srv.shutdown();
        thread.join();
        return;
    }
    
    var Ok(res) = res_result else unreachable;
    if(res.status != 200u) { env.error("Status mismatch"); }
    var body_opt = res.body.read_to_string();
    if(body_opt is std.Option.None) {
        env.error("Read body failed");
    } else {
        var Some(body) = body_opt else unreachable;
        if(!body.equals_view("world")) { env.error("Body mismatch"); }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_http_post(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8082");
    var srv = server::Server(cfg);
    
    srv.router.add("POST", "/echo", ||(req, res) => {
        var body_opt = req.body.read_to_string();
        if(body_opt is std.Option.None) {
            res.status = 400u;
            res.write_string(std::string::make_no_len("no body"));
            return;
        }
        var Some(body) = body_opt else unreachable;
        res.write_string(body);
    });
    
    var thread = srv.serve_async(8082u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var res_result = client.post("http://127.0.0.1:8082/echo", "hello echo", "text/plain");
    
    if(res_result is Result.Err) {
        var Err(e) = res_result else return;
        env.error(e.data());
        srv.shutdown();
        thread.join();
        return;
    }
    
    var Ok(res) = res_result else unreachable;
    var body_opt = res.body.read_to_string();
    if(body_opt is std.Option.None) {
        env.error("Read body failed");
    } else {
        var Some(body) = body_opt else unreachable;
        if(!body.equals_view("hello echo")) { env.error("Body mismatch"); }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_http_404(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8083");
    var srv = server::Server(cfg);
    
    var thread = srv.serve_async(8083u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var res_result = client.get("http://127.0.0.1:8083/notfound");
    
    if(res_result is Result.Err) {
        var Err(e) = res_result else return;
        env.error(e.data());
        srv.shutdown();
        thread.join();
        return;
    }
    
    var Ok(res) = res_result else unreachable;
    if(res.status != 404u) { env.error("Expected 404"); }
    
    srv.shutdown();
    thread.join();
}

@test
func test_http_headers(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8084");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/headers", ||(req, res) => {
        var val = req.headers.get("X-Test-Header");
        if(val is std.Option.None) {
            res.status = 400u;
            res.write_string(std::string::make_no_len("missing header"));
            return;
        }
        var Some(v) = val else unreachable;
        res.set_header(std::string::make_no_len("X-Response-Header"), std::replace(v, std::string()));
        res.write_string(std::string::make_no_len("ok"));
    });
    
    var thread = srv.serve_async(8084u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var u_opt = http::URL::parse("http://127.0.0.1:8084/headers");
    var Some(u) = u_opt else unreachable;
    var rb = http::RequestBuilder("GET", u);
    rb.header("X-Test-Header", "chemical-rocks");
    
    var res_result = client.request(rb);
    
    if(res_result is Result.Err) {
        var Err(e) = res_result else return;
        env.error(e.data());
        srv.shutdown();
        thread.join();
        return;
    }
    
    var Ok(res) = res_result else unreachable;
    var resp_h = res.headers.get("X-Response-Header");
    if(resp_h is std.Option.None) {
        env.error("Missing response header");
    } else {
        var Some(rv) = resp_h else unreachable;
        if(!rv.equals_view("chemical-rocks")) { env.error("Header value mismatch"); }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_http_put_delete(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8085");
    var srv = server::Server(cfg);
    
    srv.router.add("PUT", "/update", ||(req, res) => {
        res.write_string(std::string::make_no_len("updated"));
    });
    srv.router.add("DELETE", "/delete", ||(req, res) => {
        res.write_string(std::string::make_no_len("deleted"));
    });
    
    var thread = srv.serve_async(8085u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    
    var res1 = client.put("http://127.0.0.1:8085/update", "data", "text/plain");
    if(res1 is Result.Err) {
        env.error("PUT failed");
    } else {
        var Ok(r1) = res1 else unreachable;
        var b1_opt = r1.body.read_to_string();
        if(b1_opt is std.Option.None) {
            env.error("PUT read body failed");
        } else {
            var Some(b1) = b1_opt else unreachable;
            if(!b1.equals_view("updated")) { env.error("PUT response mismatch"); }
        }
    }
    
    var res2 = client.delete("http://127.0.0.1:8085/delete");
    if(res2 is Result.Err) {
        env.error("DELETE failed");
    } else {
        var Ok(r2) = res2 else unreachable;
        var b2_opt = r2.body.read_to_string();
        if(b2_opt is std.Option.None) {
            env.error("DELETE read body failed");
        } else {
            var Some(b2) = b2_opt else unreachable;
            if(!b2.equals_view("deleted")) { env.error("DELETE response mismatch"); }
        }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_http_query_params_builder(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8086");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/query", ||(req, res) => {
        var q = req.query.get("q");
        res.write_string(std::string::view_make(q));
    });
    
    var thread = srv.serve_async(8086u);
    std.concurrent.sleep_ms(100u);
    
    var u_opt = http::URL::parse("http://127.0.0.1:8086/query");
    var Some(u) = u_opt else unreachable;
    var rb = http::RequestBuilder("GET", u);
    rb.query("q", "hello-world");
    
    var client = net::Client();
    var res_result = client.request(rb);
    
    if(res_result is Result.Err) {
        env.error("Request failed");
    } else {
        var Ok(res) = res_result else unreachable;
        var body_opt = res.body.read_to_string();
        if(body_opt is std.Option.None) {
            env.error("Read body failed");
        } else {
            var Some(body) = body_opt else unreachable;
            if(!body.equals_view("hello-world")) { env.error("Query param mismatch"); }
        }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_http_large_body(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8087");
    var srv = server::Server(cfg);
    
    srv.router.add("POST", "/large", ||(req, res) => {
        var body_opt = req.body.read_to_string();
        if(body_opt is std.Option.None) {
            res.status = 400u;
            return;
        }
        var Some(body) = body_opt else unreachable;
        res.write_string(body);
    });
    
    var thread = srv.serve_async(8087u);
    std.concurrent.sleep_ms(100u);
    
    var large_data = std::string();
    for(var i=0u; i<10000u; i++) { large_data.append('A'); }
    
    var client = net::Client();
    var res_result = client.post("http://127.0.0.1:8087/large", large_data.to_view(), "text/plain");
    
    if(res_result is Result.Err) {
        env.error("Large POST failed");
    } else {
        var Ok(res) = res_result else unreachable;
        var body_opt = res.body.read_to_string();
        if(body_opt is std.Option.None) {
            env.error("Read body failed");
        } else {
            var Some(body) = body_opt else unreachable;
            if(body.size() != 10000u) { env.error("Large body size mismatch"); }
        }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_http_patch_head(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8088");
    var srv = server::Server(cfg);
    
    srv.router.add("PATCH", "/patch", ||(req, res) => {
        res.write_string(std::string::make_no_len("patched"));
    });
    srv.router.add("HEAD", "/head", ||(req, res) => {
        res.set_header(std::string::make_no_len("X-Head-Ok"), std::string::make_no_len("yes"));
        res.write_string(std::string::make_no_len("")); // HEAD shouldn't have body but server adds Content-Length
    });
    
    var thread = srv.serve_async(8088u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    
    var res_patch = client.patch("http://127.0.0.1:8088/patch", "data", "application/json");
    if(res_patch is Result.Ok) {
        var Ok(r) = res_patch else unreachable;
        var b = r.body.read_to_string().take();
        if(!b.equals_view("patched")) env.error("PATCH mismatch");
    } else {
        env.error("PATCH failed");
    }
    
    var res_head = client.head("http://127.0.0.1:8088/head");
    if(res_head is Result.Ok) {
        var Ok(rh) = res_head else unreachable;
        var h = rh.headers.get("X-Head-Ok");
        if(h is std.Option.None) env.error("HEAD missing header");
    } else {
        env.error("HEAD failed");
    }
    
    srv.shutdown();
    thread.join();
}
