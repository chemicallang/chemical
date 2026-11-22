// ============================================================================
// Production HTTP behaviour: Date, Range, and framing refusals
// ============================================================================
// Three things every real deployment needs and a render-only test suite misses:
//
//   * a `Date` header (RFC 9110 §6.6.1) — caches and `Age` arithmetic need it, and a
//     response without one looks broken to anyone reading `curl -v`;
//   * `Range` support — a video seek, a resumed download and a browser's partial
//     fetch all send `Range: bytes=…`; without it the server re-sends the whole body;
//   * a refusal with the *right* status — an oversized body is `413`, an oversized
//     header block is `431`, and neither may be answered by silently closing the
//     connection (which is what the server used to do for a bad head).
//
// These drive raw sockets: `http::Client` normalises framing and cannot send a
// deliberately oversized body or a malformed head.
using std::Option;
using std::Result;
using std::string;
using std::string_view;

func prod_handler(req : http.Request) : http.Response {
    var body = string()
    body.append_view(std::string_view("0123456789"))
    var view = body.to_view()
    return http.response_text(200u, &view)
}

// Start a server on `port` with `cfg`, answering every path with `prod_handler`.
func prod_start(port : uint, cfg : server::ServerConfig) : Option<server::Server> {
    var cfg_ = cfg
    var addr = framing_addr(port)
    cfg_.addr = addr
    var bound = server.Server.bind(cfg_)
    if(bound is Result.Err) {
        return Option.None<server::Server>()
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("GET", "/:path*", ||(req) => { return prod_handler(req) })
    return Option.Some<server::Server>(srv)
}

// Position of `needle` in `hay`, or -1.
func prod_find(hay : &string, needle : string_view) : int {
    var view = hay.to_view()
    var got = view.find(&needle)
    if(got is Option.Some) {
        var Some(position) = got else unsafe { unreachable }
        return position as int
    }
    return -1
}

// The value of header `name` in a raw response (`""` when absent).
func prod_header(reply : &string, name : string_view) : string {
    var line = string()
    var lower_name = string()
    lower_name.append_view(&name)
    line.append_view(&lower_name.to_view())
    line.append_view(": ")
    var at = prod_find(reply, line.to_view())
    var out = string()
    if(at < 0) { return out }
    var view = reply.to_view()
    var i = (at as size_t) + line.size()
    while(i < view.size() && view.at(i) != '\r') {
        out.append(view.at(i))
        i = i + 1u
    }
    return out
}

func prod_exchange(port : uint, request : &string, budget_ms : int) : string {
    var out = string()
    var _ = raw_exchange(port, request, budget_ms, &raw mut out)
    return out
}

func prod_get(port : uint, path : string_view) : string {
    var request = string("GET ")
    request.append_view(&path)
    request.append_view(" HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
    return prod_exchange(port, &request, 3000)
}

func prod_get_range(port : uint, path : string_view, range : string_view) : string {
    var request = string("GET ")
    request.append_view(&path)
    request.append_view(" HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\nRange: ")
    request.append_view(&range)
    request.append_view("\r\n\r\n")
    return prod_exchange(port, &request, 3000)
}

// ── Date ───────────────────────────────────────────────────────────────────

// The formatter is calendar arithmetic, so pin values that a locale-dependent
// `strftime` would get wrong ("Sun" vs "dim.") and a leap-year boundary.
@test
public func PROD_date_formatter_matches_known_instants(env : &mut TestEnv) {
    var cases : [4]i64 = [0, 1700000000, 951782400, 1795342022]
    var want = std::vector<string>()
    want.push_back(string("Thu, 01 Jan 1970 00:00:00 GMT"))
    want.push_back(string("Tue, 14 Nov 2023 22:13:20 GMT"))
    want.push_back(string("Tue, 29 Feb 2000 00:00:00 GMT"))
    want.push_back(string("Sun, 22 Nov 2026 10:07:02 GMT"))
    var i : size_t = 0
    while(i < 4u) {
        var got = http::format_http_date(cases[i])
        var got_view = got.to_view()
        var expected = unsafe(&*want.get_ptr(i))
        var expected_view = expected.to_view()
        if(!got_view.equals(&expected_view)) {
            unsafe {
                var m = string("format_http_date mismatch: [")
                m.append_view(&got_view)
                m.append_view("]")
                test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
            }
        }
        if(!http::is_imf_fixdate(got_view)) {
            env.error("format_http_date produced a non-IMF-fixdate")
        }
        i = i + 1u
    }
}

@test
public func PROD_every_response_carries_a_valid_date(env : &mut TestEnv) {
    const PORT : uint = 19951u
    var cfg = server::ServerConfig()
    var started = prod_start(PORT, cfg)
    if(started is Option.None) {
        env.error("could not bind 127.0.0.1:19951")
        return
    }
    var Some(srv) = started else unsafe { unreachable }
    var stop = srv.handle()
    var thread = srv.serve_async()
    std.concurrent.sleep_ms(200u)

    var reply = prod_get(PORT, std::string_view("/"))
    if(!reply.to_view().contains(std::string_view("200 OK"))) {
        env.error("GET / did not answer 200")
    }
    var date = prod_header(&reply, std::string_view("Date"))
    if(date.empty()) {
        env.error("no Date header")
    } else {
        var date_view = date.to_view()
        if(!http::is_imf_fixdate(date_view)) {
            env.error("Date header is not an IMF-fixdate")
        }
    }

    stop.shutdown()
    thread.join()
}

// ── Range ──────────────────────────────────────────────────────────────────

@test
public func PROD_range_answers_partial_content(env : &mut TestEnv) {
    const PORT : uint = 19952u
    var cfg = server::ServerConfig()
    var started = prod_start(PORT, cfg)
    if(started is Option.None) {
        env.error("could not bind 127.0.0.1:19952")
        return
    }
    var Some(srv) = started else unsafe { unreachable }
    var stop = srv.handle()
    var thread = srv.serve_async()
    std.concurrent.sleep_ms(200u)

    // A plain GET advertises ranges so a client knows it can resume.
    var full = prod_get(PORT, std::string_view("/digits"))
    if(!full.to_view().contains(std::string_view("0123456789"))) {
        env.error("plain GET lost the body")
    }
    var accepts = prod_header(&full, std::string_view("Accept-Ranges"))
    if(!accepts.to_view().equals(std::string_view("bytes"))) {
        env.error("a 200 response should advertise Accept-Ranges: bytes")
    }

    // A window in the middle.
    var part = prod_get_range(PORT, std::string_view("/digits"), std::string_view("bytes=2-5"))
    if(!part.to_view().contains(std::string_view("206 Partial Content"))) {
        env.error("Range: bytes=2-5 did not answer 206")
    }
    var content_range = prod_header(&part, std::string_view("Content-Range"))
    if(!content_range.to_view().equals(std::string_view("bytes 2-5/10"))) {
        env.error("Content-Range was not bytes 2-5/10")
    }
    var length = prod_header(&part, std::string_view("Content-Length"))
    if(!length.to_view().equals(std::string_view("4"))) {
        env.error("a partial response advertised the wrong Content-Length")
    }
    if(!part.to_view().contains(std::string_view("\r\n\r\n2345"))) {
        env.error("the partial body was not bytes 2..5")
    }

    // A suffix range resolves against the end.
    var suffix = prod_get_range(PORT, std::string_view("/digits"), std::string_view("bytes=-3"))
    if(!suffix.to_view().contains(std::string_view("\r\n\r\n789"))) {
        env.error("Range: bytes=-3 did not answer the last three bytes")
    }

    // Past the end is a 416 that says how long the representation is.
    var missing = prod_get_range(PORT, std::string_view("/digits"), std::string_view("bytes=99-"))
    if(!missing.to_view().contains(std::string_view("416 Range Not Satisfiable"))) {
        env.error("an unsatisfiable range did not answer 416")
    }
    var unsatisfied = prod_header(&missing, std::string_view("Content-Range"))
    if(!unsatisfied.to_view().equals(std::string_view("bytes */10"))) {
        env.error("416 did not carry Content-Range: bytes */10")
    }

    stop.shutdown()
    thread.join()
}

@test
public func PROD_range_parser_handles_the_forms(env : &mut TestEnv) {
    // first-last, open-ended, suffix, clamped, past-the-end, and multi-range.
    var cases : [6]string_view = [std::string_view("bytes=0-3"), std::string_view("bytes=5-"),
                                  std::string_view("bytes=-2"), std::string_view("bytes=8-100"),
                                  std::string_view("bytes=10-"), std::string_view("bytes=0-1,4-5")]
    var i : size_t = 0
    while(i < 6u) {
        var decision = http::parse_byte_range(cases[i], 10u)
        var ok = false
        if(decision is http::RangeDecision.Slice) {
            var Slice(first, last) = decision else unsafe { unreachable }
            if(i == 0u) { ok = first == 0u && last == 3u }
            else if(i == 1u) { ok = first == 5u && last == 9u }
            else if(i == 2u) { ok = first == 8u && last == 9u }
            else if(i == 3u) { ok = first == 8u && last == 9u }
        } else if(decision is http::RangeDecision.Unsatisfiable) {
            ok = i == 4u
        } else if(decision is http::RangeDecision.None) {
            // A multiple range is not supported and must fall back to the whole body.
            ok = i == 5u
        }
        if(!ok) {
            unsafe {
                var m = string("parse_byte_range case ")
                m.append_integer(i as bigint)
                m.append_view(" was wrong")
                test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
            }
        }
        i = i + 1u
    }
}

// ── Refusals ───────────────────────────────────────────────────────────────

@test
public func PROD_oversized_body_is_413(env : &mut TestEnv) {
    const PORT : uint = 19953u
    var cfg = server::ServerConfig()
    cfg.max_body_bytes = 16u
    var started = prod_start(PORT, cfg)
    if(started is Option.None) {
        env.error("could not bind 127.0.0.1:19953")
        return
    }
    var Some(srv) = started else unsafe { unreachable }
    var stop = srv.handle()
    var thread = srv.serve_async()
    std.concurrent.sleep_ms(200u)

    var request = string("POST /digits HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\nContent-Length: 1000\r\n\r\n")
    var payload = string()
    var i = 0
    while(i < 1000) {
        payload.append('x')
        i = i + 1
    }
    request.append_string(&payload)
    var reply = prod_exchange(PORT, &request, 3000)
    if(!reply.to_view().contains(std::string_view("413"))) {
        env.error("a body over max_body_bytes did not answer 413")
    }

    stop.shutdown()
    thread.join()
}

@test
public func PROD_oversized_head_is_431(env : &mut TestEnv) {
    const PORT : uint = 19954u
    var cfg = server::ServerConfig()
    cfg.max_header_bytes = 256u
    var started = prod_start(PORT, cfg)
    if(started is Option.None) {
        env.error("could not bind 127.0.0.1:19954")
        return
    }
    var Some(srv) = started else unsafe { unreachable }
    var stop = srv.handle()
    var thread = srv.serve_async()
    std.concurrent.sleep_ms(200u)

    var request = string("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n")
    var i = 0
    while(i < 64) {
        request.append_view("X-Pad: ")
        var j = 0
        while(j < 32) {
            request.append('p')
            j = j + 1
        }
        request.append_view("\r\n")
        i = i + 1
    }
    request.append_view("\r\n")
    var reply = prod_exchange(PORT, &request, 3000)
    // The point is that the client is *told*, instead of the socket simply closing.
    if(reply.empty()) {
        env.error("an oversized header block closed the connection with no response")
    } else if(!reply.to_view().contains(std::string_view("431"))) {
        env.error("an oversized header block did not answer 431")
    }

    stop.shutdown()
    thread.join()
}
