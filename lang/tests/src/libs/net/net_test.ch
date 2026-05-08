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
    std::concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var res_result = client.get("http://127.0.0.1:8081/hello");
    
    if(res_result is Result.Err) {
        env.error("GET failed");
        srv.run = false;
        net::dial("127.0.0.1", 8081u);
        thread.join();
        return;
    }
    
    var Ok(res) = res_result else unreachable;
    
    if(res.status != 200u) {
        env.error("Status mismatch");
        srv.run = false;
        net::dial("127.0.0.1", 8081u);
        thread.join();
        return;
    }
    
    var body_opt = res.body.read_to_string();
    if(body_opt is std.Result.Err) {
        env.error("Read body failed");
        srv.run = false;
        net::dial("127.0.0.1", 8081u);
        thread.join();
        return;
    }
    var Some(body) = body_opt else unreachable
    
    if(!body.equals_with_len("world", 5)) {
        env.error("Body mismatch");
        srv.run = false;
        net::dial("127.0.0.1", 8081u);
        thread.join();
        return;
    }
    
    srv.run = false;
    net::dial("127.0.0.1", 8081u);
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
        var Some(body) = body_opt else unreachable
        res.write_string(std::replace(body, std::string()));
    });
    
    var thread = srv.serve_async(8082u);
    std::concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var res_result = client.post("http://127.0.0.1:8082/echo", "hello echo", "text/plain");
    
    if(res_result is Result.Err) {
        env.error("POST failed");
        srv.run = false;
        net::dial("127.0.0.1", 8082u);
        thread.join();
        return;
    }
    
    var Ok(res) = res_result else unreachable;
    
    var body_opt = res.body.read_to_string();
    if(body_opt is std.Option.None) {
        env.error("Read body failed");
        srv.run = false;
        net::dial("127.0.0.1", 8082u);
        thread.join();
        return;
    }
    var Some(body) = body_opt else unreachable
    
    if(!body.equals_with_len("hello echo", 10)) {
        env.error("Body mismatch");
        srv.run = false;
        net::dial("127.0.0.1", 8082u);
        thread.join();
        return;
    }
    
    srv.run = false;
    net::dial("127.0.0.1", 8082u);
    thread.join();
}

@test
func test_http_query_params(env : &mut TestEnv) {
    var cfg = server::ServerConfig();
    cfg.addr = std::string::make_no_len("127.0.0.1:8083");
    var srv = server::Server(cfg);
    
    srv.router.add("GET", "/query", ||(req, res) => {
        var name = req.query.get("name");
        var out = std::string::make_no_len("hello ");
        out.append_view(name);
        res.write_string(out);
    });
    
    var thread = srv.serve_async(8083u);
    std::concurrent.sleep_ms(100u);
    
    var client = net::Client();
    var res_result = client.get("http://127.0.0.1:8083/query?name=chemical");
    
    if(res_result is Result.Err) {
        env.error("GET with query failed");
        srv.run = false;
        net::dial("127.0.0.1", 8083u);
        thread.join();
        return;
    }
    
    var Ok(res) = res_result else unreachable;
    
    var body_opt = res.body.read_to_string();
    if(body_opt is std.Option.None) {
        env.error("Read body failed");
        srv.run = false;
        net::dial("127.0.0.1", 8083u);
        thread.join();
        return;
    }
    var Some(body) = body_opt else unreachable
    
    if(!body.equals_with_len("hello chemical", 14)) {
        env.error("Body mismatch");
        srv.run = false;
        net::dial("127.0.0.1", 8083u);
        thread.join();
        return;
    }
    
    srv.run = false;
    net::dial("127.0.0.1", 8083u);
    thread.join();
}
