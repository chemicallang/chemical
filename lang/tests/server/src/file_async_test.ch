// Tier 6: the `server` runtime library's coroutine file-server path
// (`server::serve_files_async`). Starts the coroutine accept loop, fetches a
// static file with the Tier 3 async client, checks 404 handling and clean
// shutdown. Run with `./scripts/test.sh --tcc --server` / `--llvm --server`.

using std::string;
using std::string_view;

// Write the fixture served by the tests. Returns false if the write failed.
func write_server_fixture() : bool {
    var content = string("hello from the async file server")
    var path = string("/tmp/chemical_tier6_server_test.txt")
    var r = fs::write_text_file(path.data(), content.data() as *mut u8, content.size())
    return r is std::Result.Ok
}

// Issue one async GET, retrying briefly so the accept loop has time to bind.
func server_async_get(url_str : string_view) : *mut http::HttpResult {
    var client = http::Client()
    var n = 0
    var box : *mut http::HttpResult = null
    while(n < 200) {
        var u = url_str
        box = async::block_on<*mut http::HttpResult>(http::get_async(&raw mut client, &raw mut u))
        if(box != null) { return box }
        std::concurrent.sleep_ms(5u)
        n = n + 1
    }
    return box
}

@test
@test.timeout(60000)
public func INT_server_async_file_get(env : &mut TestEnv) {
    if(!write_server_fixture()) {
        env.error("could not write the server fixture file")
        return
    }

    var root = string("/tmp")
    var cfg = server::ServerConfig()
    cfg.addr = string("127.0.0.1:19881")
    var srv = server::Server(cfg)
    var server_f = async::spawn<int>(server::serve_files_async(&raw mut srv, root.data(), 19881u))
    var unit = async::block_on<core::async::Unit>(async::yield_now())

    var box = server_async_get(string_view("http://127.0.0.1:19881/chemical_tier6_server_test.txt"))
    if(box == null) {
        env.error("async file request returned a null box")
    } else if(!box.is_ok()) {
        env.error("async file request failed")
    } else {
        if(box.status_code() != 200u) {
            env.error("async file status mismatch")
        }
        var body = box.body_view()
        if(body.size() == 0u) {
            env.error("async file body was empty")
        }
    }
    if(box != null) { delete box }

    srv.shutdown()
    var r = async::block_on<int>(server_f)
    if(r != 0) {
        env.error("serve_files_async returned non-zero")
    }
}

@test
@test.timeout(60000)
public func INT_server_async_404(env : &mut TestEnv) {
    var root = string("/tmp")
    var cfg = server::ServerConfig()
    cfg.addr = string("127.0.0.1:19882")
    var srv = server::Server(cfg)
    var server_f = async::spawn<int>(server::serve_files_async(&raw mut srv, root.data(), 19882u))
    var unit = async::block_on<core::async::Unit>(async::yield_now())

    var box = server_async_get(string_view("http://127.0.0.1:19882/definitely_missing_tier6_file.txt"))
    if(box == null) {
        env.error("404 request returned a null box")
    } else {
        if(box.status_code() != 404u) {
            env.error("missing file did not return 404")
        }
        delete box
    }

    srv.shutdown()
    var r = async::block_on<int>(server_f)
    if(r != 0) {
        env.error("serve_files_async returned non-zero")
    }
}
