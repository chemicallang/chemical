// Tier 6: the `server` runtime library's file-server path
// (`server::serve_files_async`). Starts the file server on its own thread,
// fetches a static file with the buffered HTTP client driven through the async
// runtime, checks 404 handling and clean shutdown. Run with
// `./scripts/test.sh --tcc --server` / `--llvm --server`.

using std::string;
using std::string_view;
using std::Result;

// The static-file root: `/tmp` on POSIX, the Windows temp dir on Windows (a
// POSIX `/tmp` is not a valid Win32 path).
func server_test_root() : string {
    comptime if(def.windows) {
        var t = environment::get("TEMP")
        if(t is std::Option.Some) {
            var Some(s) = t else unsafe { unreachable }
            return s.copy()
        }
        return string(".")
    } else {
        return string("/tmp")
    }
}

// The fixture served by the file tests, in one place so the writer and the
// content assertion cannot drift apart.
func server_fixture_text() : string {
    return string("hello from the async file server")
}

// Write the fixture served by the tests. Returns false if the write failed.
func write_server_fixture() : bool {
    var content = server_fixture_text()
    var path = server_test_root()
    path.append('/')
    var fixture_name = string_view("chemical_tier6_server_test.txt")
    path.append_view(&fixture_name)
    var r = fs::raw::write_text_file(path.data(), content.data() as *mut u8, content.size())
    return r is std::Result.Ok
}

// `cfg.addr` is the port's only home now (`serve_async` takes no port and
// `serve_files_async` no longer takes one either), while the request URL still
// spells it directly, so build the address from the same constant.
func file_test_addr(port : uint) : string {
    var out = string("127.0.0.1:")
    out.append_uinteger(port as ubigint)
    return out
}

// One GET through the buffered client. The URL is rebuilt here so the blocking
// task captures only a port and a static path, never an owned string.
func http_get_once(port : uint, path_text : *char) : std::Result<http::Response, http::HttpError> {
    var client = http::Client()
    var url = string("http://127.0.0.1:")
    url.append_uinteger(port as ubigint)
    url.append_char_ptr(path_text)
    var url_str = std::str.make_view(url.to_view())
    return client.get(url_str)
}

// The request body is buffered by the client, so the blocking call is handed to
// the runtime's thread pool and the coroutine awaits it. That is the same shape
// the deleted `http::raw::get_async` had — a request that suspends the caller
// instead of parking it — so the async plumbing these tests were written for is
// still exercised.
async func server_get_async(port : uint, path_text : *char) : std::Result<http::Response, http::HttpError> {
    var r = await unsafe(async::spawn_blocking<std::Result<http::Response, http::HttpError>>(
        |port, path_text|() => http_get_once(port, path_text)
    ))
    return r
}

// Issue one async GET, retrying briefly so the accept loop has time to bind.
// A failed attempt still reports its error, so the retry waits for a response
// rather than reporting the bind race as a wrong status.
func server_async_get(port : uint, path_text : *char) : std::Result<http::Response, http::HttpError> {
    var attempt = 0
    var last = async::block_on<std::Result<http::Response, http::HttpError>>(server_get_async(port, path_text))
    while(attempt < 200) {
        if(last is Result.Ok) { return last }
        std::concurrent.sleep_ms(5u)
        attempt = attempt + 1
        last = async::block_on<std::Result<http::Response, http::HttpError>>(server_get_async(port, path_text))
    }
    // Hand back the last failure so the caller reports its reason.
    return last
}

@test
@test.timeout(60000)
public func INT_server_async_file_get(env : &mut TestEnv) {
    const PORT : uint = 19881u

    unsafe {

    if(!write_server_fixture()) {
        env.error("could not write the server fixture file")
        return
    }

    var root = server_test_root()
    var cfg = server::ServerConfig()
    cfg.addr = file_test_addr(PORT)
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19881")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    // `serve_files_async` consumes the server, so the stop handle must be taken
    // first; the accept loop then runs on its own thread.
    var root_view = std::str.make_view(root.to_view())
    var stop = srv.handle()
    var server_thread = server::serve_files_async(srv, root_view)
    std::concurrent.sleep_ms(50u)

    var res = server_async_get(PORT, "/chemical_tier6_server_test.txt")
    if(res is Result.Err) {
        var Err(e) = res else unsafe { unreachable }
        var why = e.message()
        var m = string("async file request failed: ")
        m.append_view(&why)
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    } else {
        var Ok(r) = res else unsafe { unreachable }
        if(r.status != 200u) {
            env.error("async file status mismatch")
        }
        var body = r.body.text()
        if(body is std::Option.None) {
            env.error("async file body was empty")
        } else {
            var Some(bv) = body else unsafe { unreachable }
            if(bv.size() == 0u) {
                env.error("async file body was empty")
            } else {
                var expected = server_fixture_text()
                var expected_view = expected.to_view()
                if(!bv.equals(&expected_view)) {
                    var m = string("async file body mismatch, got: ")
                    m.append_view(&bv)
                    test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
                }
            }
        }
        // The static-file server maps `.txt` to `text/plain`; the type must
        // survive the async path unchanged.
        var ct = r.headers.get("Content-Type")
        if(ct is std::Option.None) {
            env.error("async file response is missing Content-Type")
        } else {
            var Some(cv) = ct else unsafe { unreachable }
            if(!cv.starts_with("text/plain")) {
                env.error("async file content type mismatch")
            }
        }
    }

    // Shutting the server down and joining its thread replaces the old
    // `async::block_on<int>(server_f) == 0` check: the thread only returns once
    // `serve_files_async` has observed the shutdown flag and left its loop.
    stop.shutdown()
    server_thread.join()

}
}

@test
@test.timeout(60000)
public func INT_server_async_404(env : &mut TestEnv) {
    const PORT : uint = 19882u

    unsafe {

    var root = server_test_root()
    var cfg = server::ServerConfig()
    cfg.addr = file_test_addr(PORT)
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19882")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    var root_view = std::str.make_view(root.to_view())
    var stop = srv.handle()
    var server_thread = server::serve_files_async(srv, root_view)
    std::concurrent.sleep_ms(50u)

    var res = server_async_get(PORT, "/definitely_missing_tier6_file.txt")
    if(res is Result.Err) {
        var Err(e) = res else unsafe { unreachable }
        var why = e.message()
        var m = string("404 request failed: ")
        m.append_view(&why)
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    } else {
        var Ok(r) = res else unsafe { unreachable }
        if(r.status != 404u) {
            env.error("missing file did not return 404")
        }
    }

    stop.shutdown()
    server_thread.join()

}
}
