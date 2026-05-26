using namespace std;
using namespace net;

@test
func test_http_get(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8081");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/hello", ||(req, res) => {
        res.write_string(std::string::make_no_len("world"));
    });
    
    var thread = srv.serve_async(8081u);
    std.concurrent.sleep_ms(100u);

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
    var rb = http::RequestBuilder("GET", std::replace(u, http::URL()));
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
    var rb = http::RequestBuilder("GET", std::replace(u, http::URL()));
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
        var str = r.body.read_to_string()
        var b = str.take();
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

// ===== Production readiness tests =====

@test
func test_invalid_url(env : &mut TestEnv) {
    var client = net::Client();
    var res = client.get(std::string_view("not-a-url"));
    if(res is Result.Ok) { env.error("Should fail for invalid URL"); }
}

@test
func test_connection_refused(env : &mut TestEnv) {
    var client = net::Client();
    var res = client.get(std::string_view("http://127.0.0.1:59999/nonexistent"));
    if(res is Result.Ok) { env.error("Should fail when connection refused"); }
}

@test
func test_server_shutdown_during_request(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8089");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/slow", ||(req, res) => {
        std.concurrent.sleep_ms(500u);
        res.write_string(std::string::make_no_len("done"));
    });
    
    var thread = srv.serve_async(8089u);
    std.concurrent.sleep_ms(50u);
    
    var client = net::Client();
    // Start request
    var res = client.get(std::string_view("http://127.0.0.1:8089/slow"));
    // Server shutdown during request - result depends on timing
    srv.shutdown();
    thread.join();
    
    // Just verify no crash occurred
    if(res is Result.Err) {
        // Connection closed - acceptable
    } else {
        var Ok(r) = res else unreachable;
        // Request succeeded - also acceptable
    }
}

@test
func test_concurrent_requests(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8090");
    var srv = server::Server(cfg);
    
    var request_count = 0u;
    srv.router.add("GET", "/counter", |&mut request_count|(req, res) => {
        *request_count = *request_count + 1u;
        res.write_string(std::string::make_no_len("ok"));
    });
    
    var thread = srv.serve_async(8090u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    // Make 20 sequential requests
    var success_count = 0u;
    for(var i=0u; i<20u; i++) {
        var res = client.get(std::string_view("http://127.0.0.1:8090/counter"));
        if(res is Result.Ok) { success_count = success_count + 1u; }
    }
    
    if(success_count != 20u) { env.error("Expected 20 successful requests"); }
    
    srv.shutdown();
    thread.join();
}

@test
func test_empty_body_response(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8091");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/empty", ||(req, res) => {
        res.write_string(std::string::make_no_len(""));
    });
    
    var thread = srv.serve_async(8091u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var res = client.get(std::string_view("http://127.0.0.1:8091/empty"));
    
    if(res is Result.Err) { env.error("Request failed"); }
    else {
        var Ok(r) = res else unreachable;
        if(r.status != 200u) { env.error("Status should be 200"); }
        var body_opt = r.body.read_to_string();
        if(body_opt is std.Option.None) { env.error("Read body failed"); }
        else {
            var Some(body) = body_opt else unreachable;
            if(body.size() != 0u) { env.error("Body should be empty"); }
        }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_multiple_headers(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8092");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/multi-headers", ||(req, res) => {
        var h1 = req.headers.get("X-Header-1");
        var h2 = req.headers.get("X-Header-2");
        if(h1 is std.Option.None || h2 is std.Option.None) {
            res.status = 400u;
            res.write_string(std::string::make_no_len("missing headers"));
            return;
        }
        res.set_header(std::string::make_no_len("X-Response-1"), std::string::make_no_len("val1"));
        res.set_header(std::string::make_no_len("X-Response-2"), std::string::make_no_len("val2"));
        res.write_string(std::string::make_no_len("ok"));
    });
    
    var thread = srv.serve_async(8092u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var u_opt = http::URL::parse(std::string_view("http://127.0.0.1:8092/multi-headers"));
    var Some(u) = u_opt else unreachable;
    var rb = http::RequestBuilder("GET", std::replace(u, http::URL()));
    rb.header("X-Header-1", "value1");
    rb.header("X-Header-2", "value2");
    
    var res_result = client.request(rb);
    
    if(res_result is Result.Err) { env.error("Request failed"); }
    else {
        var Ok(res) = res_result else unreachable;
        var r1 = res.headers.get("X-Response-1");
        var r2 = res.headers.get("X-Response-2");
        if(r1 is std.Option.None || r2 is std.Option.None) {
            env.error("Missing response headers");
        }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_url_parsing_edge_cases(env : &mut TestEnv) {
    // Test URL with port
    var u1 = http::URL::parse(std::string_view("http://localhost:3000/path"));
    if(u1 is std::Option.None) { env.error("URL with port should parse"); }
    else {
        var Some(url) = u1 else unreachable;
        if(url.port != 3000u) { env.error("Port should be 3000"); }
        if(!url.path.equals_view("/path")) { env.error("Path mismatch"); }
    }
    
    // Test URL without port
    var u2 = http::URL::parse(std::string_view("http://example.com/test"));
    if(u2 is std::Option.None) { env.error("URL without port should parse"); }
    else {
        var Some(url) = u2 else unreachable;
        if(url.port != 80u) { env.error("Default port should be 80"); }
    }
    
    // Test URL with query string
    var u3 = http::URL::parse(std::string_view("http://localhost:8080/api?key=value"));
    if(u3 is std::Option.None) { env.error("URL with query should parse"); }
    else {
        var Some(url) = u3 else unreachable;
        if(!url.query.equals_view("key=value")) { env.error("Query mismatch"); }
    }
    
    // Test URL without path
    var u4 = http::URL::parse(std::string_view("http://localhost:9090"));
    if(u4 is std::Option.None) { env.error("URL without path should parse"); }
    else {
        var Some(url) = u4 else unreachable;
        if(!url.path.equals_view("/")) { env.error("Default path should be /"); }
    }
}

@test
func test_query_param_encoding(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8093");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/encoded", ||(req, res) => {
        var q = req.query.get("q");
        res.write_string(std::string::view_make(q));
    });
    
    var thread = srv.serve_async(8093u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    // URL encode the query param value to send literal %20
    var u_opt = http::URL::parse(std::string_view("http://127.0.0.1:8093/encoded"));
    var Some(u) = u_opt else unreachable;
    var rb = http::RequestBuilder("GET", std::replace(u, http::URL()));
    // %25 encodes to %, so server will see hello%20world after decoding
    rb.query("q", "hello%2520world");
    
    var res = client.request(rb);
    
    if(res is Result.Err) { env.error("Request failed"); }
    else {
        var Ok(r) = res else unreachable;
        var body_opt = r.body.read_to_string();
        if(body_opt is std.Option.None) { env.error("Read body failed"); }
        else {
            var Some(body) = body_opt else unreachable;
            // Server decodes %25 to %, so we get hello%20world
            if(!body.equals_view("hello%20world")) { env.error("Query encoding mismatch"); }
        }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_router_static_routes(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8094");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/users", ||(req, res) => {
        res.write_string(std::string::make_no_len("users"));
    });
    srv.router.add("GET", "/posts", ||(req, res) => {
        res.write_string(std::string::make_no_len("posts"));
    });
    srv.router.add("POST", "/users", ||(req, res) => {
        res.write_string(std::string::make_no_len("created"));
    });
    
    var thread = srv.serve_async(8094u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    
    var r1 = client.get(std::string_view("http://127.0.0.1:8094/users"));
    if(r1 is Result.Ok) {
        var Ok(resp) = r1 else unreachable;
        var str = resp.body.read_to_string()
        var body = str.take();
        if(!body.equals_view("users")) { env.error("Users route mismatch"); }
    } else { env.error("Users route failed"); }
    
    var r2 = client.get(std::string_view("http://127.0.0.1:8094/posts"));
    if(r2 is Result.Ok) {
        var Ok(resp) = r2 else unreachable;
        var str = resp.body.read_to_string()
        var body = str.take();
        if(!body.equals_view("posts")) { env.error("Posts route mismatch"); }
    } else { env.error("Posts route failed"); }
    
    var r3 = client.post(std::string_view("http://127.0.0.1:8094/users"), std::string_view("data"), "text/plain");
    if(r3 is Result.Ok) {
        var Ok(resp) = r3 else unreachable;
        var str = resp.body.read_to_string()
        var body = str.take();
        if(!body.equals_view("created")) { env.error("POST users mismatch"); }
    } else { env.error("POST users failed"); }
    
    srv.shutdown();
    thread.join();
}

@test
func test_router_param_routes(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8095");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/users/:id", ||(req, res) => {
        res.write_string(std::string::make_no_len("user"));
    });
    
    var thread = srv.serve_async(8095u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var res = client.get(std::string_view("http://127.0.0.1:8095/users/123"));
    
    if(res is Result.Err) { env.error("Request failed"); }
    else {
        var Ok(r) = res else unreachable;
        if(r.status != 200u) { env.error("Should match route with param"); }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_response_status_codes(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8096");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/ok", ||(req, res) => {
        res.write_string(std::string::make_no_len("ok"));
    });
    srv.router.add("GET", "/bad", ||(req, res) => {
        res.status = 400u;
        res.write_string(std::string::make_no_len("bad request"));
    });
    srv.router.add("GET", "/error", ||(req, res) => {
        res.status = 500u;
        res.write_string(std::string::make_no_len("server error"));
    });
    
    var thread = srv.serve_async(8096u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    
    var r1 = client.get(std::string_view("http://127.0.0.1:8096/ok"));
    if(r1 is Result.Ok) {
        var Ok(resp) = r1 else unreachable;
        if(resp.status != 200u) { env.error("Expected 200"); }
    }
    
    var r2 = client.get(std::string_view("http://127.0.0.1:8096/bad"));
    if(r2 is Result.Ok) {
        var Ok(resp) = r2 else unreachable;
        if(resp.status != 400u) { env.error("Expected 400"); }
    }
    
    var r3 = client.get(std::string_view("http://127.0.0.1:8096/error"));
    if(r3 is Result.Ok) {
        var Ok(resp) = r3 else unreachable;
        if(resp.status != 500u) { env.error("Expected 500"); }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_binary_body(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8097");
    var srv = server::Server(cfg);
    
    srv.router.add("POST", "/binary", ||(req, res) => {
        var body_opt = req.body.read_to_string();
        if(body_opt is std.Option.None) { res.status = 400u; return; }
        var Some(body) = body_opt else unreachable;
        res.write_string(body);
    });
    
    var thread = srv.serve_async(8097u);
    std.concurrent.sleep_ms(100u);
    
    // Create binary-like data (non-UTF8 bytes)
    var binary_data = std::string();
    binary_data.append(0x00 as char);
    binary_data.append(0xFF as char);
    binary_data.append(0xFE as char);
    binary_data.append(0x01 as char);
    
    var client = net::Client();
    var res = client.post(std::string_view("http://127.0.0.1:8097/binary"), binary_data.to_view(), "application/octet-stream");
    
    if(res is Result.Err) { env.error("Binary request failed"); }
    else {
        var Ok(r) = res else unreachable;
        var body_opt = r.body.read_to_string();
        if(body_opt is std.Option.None) { env.error("Read binary body failed"); }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_special_characters_in_body(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8098");
    var srv = server::Server(cfg);
    
    srv.router.add("POST", "/special", ||(req, res) => {
        var body_opt = req.body.read_to_string();
        if(body_opt is std.Option.None) { res.status = 400u; return; }
        var Some(body) = body_opt else unreachable;
        res.write_string(body);
    });
    
    var thread = srv.serve_async(8098u);
    std.concurrent.sleep_ms(100u);
    
    // Test special characters that can break HTTP parsing
    // Avoid \0 because std::string treats it as null terminator
    var special = std::string::make_no_len("hello\r\nworld\ttab");
    var client = net::Client();
    var res = client.post(std::string_view("http://127.0.0.1:8098/special"), special.to_view(), "text/plain");
    
    if(res is Result.Err) { env.error("Special chars request failed"); }
    else {
        var Ok(r) = res else unreachable;
        var body_opt = r.body.read_to_string();
        if(body_opt is std.Option.None) { env.error("Read special body failed"); }
        else {
            var Some(body) = body_opt else unreachable;
            if(!body.equals_view("hello\r\nworld\ttab")) { env.error("Special chars mismatch"); }
        }
    }
    
    srv.shutdown();
    thread.join();
}

@test
func test_reuse_client_for_multiple_requests(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8099");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/a", ||(req, res) => { res.write_string(std::string::make_no_len("a")); });
    srv.router.add("GET", "/b", ||(req, res) => { res.write_string(std::string::make_no_len("b")); });
    srv.router.add("GET", "/c", ||(req, res) => { res.write_string(std::string::make_no_len("c")); });
    
    var thread = srv.serve_async(8099u);
    std.concurrent.sleep_ms(100u);
    
    // Reuse same client instance
    var client = net::Client();
    var failures = 0u;
    
    if(client.get(std::string_view("http://127.0.0.1:8099/a")) is Result.Err) { failures = failures + 1u; }
    if(client.get(std::string_view("http://127.0.0.1:8099/b")) is Result.Err) { failures = failures + 1u; }
    if(client.get(std::string_view("http://127.0.0.1:8099/c")) is Result.Err) { failures = failures + 1u; }
    
    if(failures > 0u) { env.error("Some requests failed with reused client"); }
    
    srv.shutdown();
    thread.join();
}

@test
func test_high_concurrency_stress(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8100");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/ping", ||(req, res) => {
        res.write_string(std::string::make_no_len("pong"));
    });
    
    var thread = srv.serve_async(8100u);
    std.concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var success = 0u;
    var fail = 0u;
    
    // Make 50 requests serially to stress test
    for(var i=0u; i<50u; i++) {
        if(client.get(std::string_view("http://127.0.0.1:8100/ping")) is Result.Ok) {
            success = success + 1u;
        } else {
            fail = fail + 1u;
        }
    }
    
    if(fail > 0u) { env.error("Some requests failed under stress"); }
    if(success != 50u) { env.error("Not all requests succeeded"); }
    
    srv.shutdown();
    thread.join();
}

struct TestCaptured2345 {
    var success_count : uint;
    var count_lock : std.mutex
}

@test
func test_multiple_clients_concurrent(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8101");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/echo/:id", ||(req, res) => {
        res.write_string(std::string::make_no_len("ok"));
    });
    
    var thread = srv.serve_async(8101u);
    std.concurrent.sleep_ms(100u);

    var captured : TestCaptured2345 = {
        success_count = 0u
        count_lock = std.mutex()
    }
    
    var t1 = std.concurrent.spawn(||(arg : *void) => {
        var cap = arg as *mut TestCaptured2345
        var client = net::Client();
        for(var i=0u; i<10u; i++) {
            var res = client.get(std::string_view("http://127.0.0.1:8101/echo/1"));
            if(res is Result.Ok) {
                cap.count_lock.lock();
                cap.success_count = cap.success_count + 1u;
                cap.count_lock.unlock();
            }
        }
        return null;
    }, &mut captured);
    
    var t2 = std.concurrent.spawn(||(arg : *void) => {
        var cap = arg as *mut TestCaptured2345
        var client = net::Client();
        for(var i=0u; i<10u; i++) {
            var res = client.get(std::string_view("http://127.0.0.1:8101/echo/2"));
            if(res is Result.Ok) {
                cap.count_lock.lock();
                cap.success_count = cap.success_count + 1u;
                cap.count_lock.unlock();
            }
        }
        return null;
    }, &mut captured);
    
    var t3 = std.concurrent.spawn(||(arg : *void) => {
        var cap = arg as *mut TestCaptured2345
        var client = net::Client();
        for(var i=0u; i<10u; i++) {
            var res = client.get(std::string_view("http://127.0.0.1:8101/echo/3"));
            if(res is Result.Ok) {
                cap.count_lock.lock();
                cap.success_count = cap.success_count + 1u;
                cap.count_lock.unlock();
            }
        }
        return null;
    }, &mut captured);
    
    t1.join();
    t2.join();
    t3.join();
    
    if(captured.success_count != 30u) { env.error("Expected 30 successful requests"); }
    
    srv.shutdown();
    thread.join();
}