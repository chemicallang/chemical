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

// Where the test's log sink writes. A module-level buffer instead of a captured
// `Arc<Mutex<…>>`: a `Send` callback cannot capture a container the analyzer
// cannot prove `Send`, and a plain byte buffer needs no capture at all.
var PROD_LOG_LINE : [2048]char
var PROD_LOG_LEN : ubigint = 0

func prod_log_store(line : &string) {
    var view = line.to_view()
    var n = view.size()
    if(n > 2047u) { n = 2047u }
    unsafe {
        memcpy(&raw mut PROD_LOG_LINE[0], view.data(), n)
        PROD_LOG_LINE[n] = 0
        PROD_LOG_LEN = n
    }
}

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

// ── Access log ─────────────────────────────────────────────────────────────

// The line format is what an operator's tooling parses, so it is pinned here:
// Common Log Format plus the duration, `-` for a field the entry does not have.
@test
public func PROD_access_line_is_common_log_format(env : &mut TestEnv) {
    var entry = http::AccessEntry {
        remote : std::string_view("10.0.0.7:51234"),
        method : std::string_view("GET"),
        target : std::string_view("/pricing"),
        status : 200u,
        bytes : 2565 as usize,
        duration_us : 38,
        epoch_secs : 1700000000,
        keep_alive : true
    }
    var line = http::format_access_line(&entry)
    var view = line.to_view()
    if(!view.contains(std::string_view("10.0.0.7:51234 - - [14/Nov/2023:22:13:20 +0000] \"GET /pricing HTTP/1.1\" 200 2565 38us"))) {
        unsafe {
            var m = string("unexpected access line: [")
            m.append_view(&view)
            m.append_view("]")
            test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
        }
    }
}

// A bare `format_access_line` proves nothing about the server: this drives a real
// request through the pipeline and checks the entry it produced. The sink writes
// into a module-level buffer (a `Send` callback may not capture a container the
// analyzer cannot prove `Send`), so the check reads the line the server logged.
@test
public func PROD_access_log_records_a_served_request(env : &mut TestEnv) {
    const PORT : uint = 19955u
    unsafe { PROD_LOG_LEN = 0 }
    var cfg = server::ServerConfig()
    cfg.access_log = http::AccessLogger.make(||(entry : &http::AccessEntry) => {
        var line = http::format_access_line(entry)
        prod_log_store(&line)
    })
    var started = prod_start(PORT, cfg)
    if(started is Option.None) {
        env.error("could not bind 127.0.0.1:19955")
        return
    }
    var Some(srv) = started else unsafe { unreachable }
    var stop = srv.handle()
    var thread = srv.serve_async()
    std.concurrent.sleep_ms(200u)
    var reply = prod_get(PORT, string_view("/logged"))
    std.concurrent.sleep_ms(50u)
    stop.shutdown()
    thread.join()

    var logged = string()
    unsafe {
        if(PROD_LOG_LEN == 0u) {
            env.error("no access line was logged for a served request")
            return
        }
        logged.append_with_len(&raw mut PROD_LOG_LINE[0], PROD_LOG_LEN)
    }
    var view = logged.to_view()
    if(!view.contains(std::string_view("\"GET /logged HTTP/1.1\" 200 10"))) {
        unsafe {
            var m = string("access line did not describe the request: [")
            m.append_view(&view)
            m.append_view("]")
            test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
        }
    }
    if(!view.contains(std::string_view("127.0.0.1:"))) {
        env.error("the access line did not name the peer")
    }
    if(!view.contains(std::string_view("us"))) {
        env.error("the access line did not carry a duration")
    }
    if(reply.empty()) {
        env.error("the logged request got no response")
    }
}

// ── Compression negotiation ────────────────────────────────────────────────

// `accepts_gzip` decides whether a rendered body may be compressed. Getting
// `gzip;q=0` or `br, gzip;q=0.5` wrong means sending a body the client refused.
@test
public func PROD_accept_encoding_decides_gzip(env : &mut TestEnv) {
    if(!http::accepts_gzip(std::string_view("gzip"))) { env.error("gzip not accepted") }
    if(!http::accepts_gzip(std::string_view("deflate, gzip;q=0.8"))) { env.error("gzip;q=0.8 not accepted") }
    if(!http::accepts_gzip(std::string_view("deflate, *"))) { env.error("* not accepted") }
    if(http::accepts_gzip(std::string_view("gzip;q=0"))) { env.error("gzip;q=0 accepted") }
    if(http::accepts_gzip(std::string_view("br"))) { env.error("br accepted as gzip") }
    if(http::accepts_gzip(std::string_view(""))) { env.error("empty header accepted") }
    if(http::accepts_gzip(std::string_view("gzip;q=0, *"))) { env.error("explicit refusal outranked by *") }
}

// ── File bodies: validators and ranges ─────────────────────────────────────
//
// `http::FileServer` backs `server.serve`, so this is the static path a real
// deployment uses. Two properties decide whether a download is resumable and
// whether a browser re-downloads a page it already has:
//
//   * an `ETag` + `Last-Modified` on the response, and `304` (no body) when
//     `If-None-Match` names that tag — answered without opening the file;
//   * `Range` served by seeking the file and streaming the slice, not by reading
//     the whole file into memory.
//
// Only a socket proves the second: the response body is a file handle, and the
// point is the bytes the writer pulls out of it.

@never_destructed
var PROD_FILE_ROOT : string

// The body of a raw response: everything after the blank line.
func prod_body_of(reply : &string) : string {
    var out = string()
    var at = prod_find(reply, std::string_view("\r\n\r\n"))
    if(at < 0) { return out }
    var view = reply.to_view()
    var i = (at as size_t) + 4u
    while(i < view.size()) {
        out.append(view.at(i))
        i = i + 1u
    }
    return out
}

func prod_path_of(text : string_view) : path::Path {
    return path::Path.new(unsafe(std::str.make_view(text)))
}

// `<root>/<name>` written with `text`; false when the write failed.
func prod_write_file(root : string_view, name : string_view, text : string_view) : bool {
    var full = string()
    full.append_view(&root)
    full.append_view("/")
    full.append_view(&name)
    var joined = full.to_view()
    var p = prod_path_of(joined)
    var wrote = fs::write(&p, text.as_bytes())
    return wrote is Result.Ok
}

func prod_file_handler(req : http.Request) : http.Response {
    var root_view = unsafe(PROD_FILE_ROOT.to_view())
    var files = http.create_file_server(unsafe(std::str.make_view(root_view)))
    return files.serve_http(&req)
}

@test
public func PROD_file_body_serves_ranges_and_validators(env : &mut TestEnv) {
    const PORT : uint = 19957u
    var root = string("lang/tests/build/prod_file_root")
    var root_view = root.to_view()
    var root_path = prod_path_of(root_view)
    var _ = fs::create_dir_all(&root_path)

    // 20 bytes, so a slice's offsets are checkable by eye.
    if(!prod_write_file(root_view, std::string_view("digits.txt"), std::string_view("0123456789abcdefghij"))) {
        env.error("could not write the fixture file")
        return
    }
    unsafe { PROD_FILE_ROOT = root.copy() }

    var cfg = server::ServerConfig()
    var addr = framing_addr(PORT)
    cfg.addr = addr
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19957")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("GET", "/:path*", ||(req) => { return prod_file_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std.concurrent.sleep_ms(200u)

    // An unqualified GET carries the validators and advertises ranges.
    var full = prod_get(PORT, std::string_view("/digits.txt"))
    if(!full.to_view().contains(std::string_view("200 OK"))) {
        env.error("a file GET was not 200")
    }
    var etag = prod_header(&full, std::string_view("ETag"))
    if(etag.empty()) {
        env.error("a file response carried no ETag")
    }
    var quoted = false
    if(!etag.empty()) { quoted = etag.to_view().at(0u) == '"' }
    if(!quoted) {
        env.error("the ETag was not quoted")
    }
    var modified = prod_header(&full, std::string_view("Last-Modified"))
    if(modified.empty()) {
        env.error("a file response carried no Last-Modified")
    }
    var accepts = prod_header(&full, std::string_view("Accept-Ranges"))
    if(!accepts.to_view().equals(std::string_view("bytes"))) {
        env.error("a file response did not advertise Accept-Ranges: bytes")
    }
    var whole = prod_body_of(&full)
    if(!whole.to_view().equals(std::string_view("0123456789abcdefghij"))) {
        env.error("the whole file body was wrong")
    }

    // A conditional GET is a 304 with the validators and no body.
    var conditional_request = string("GET /digits.txt HTTP/1.1\r\nHost: 127.0.0.1\r\nIf-None-Match: ")
    conditional_request.append_string(&etag)
    conditional_request.append_view("\r\nConnection: close\r\n\r\n")
    var conditional = prod_exchange(PORT, &conditional_request, 3000)
    if(!conditional.to_view().contains(std::string_view("304 Not Modified"))) {
        env.error("If-None-Match did not answer 304")
    }
    var conditional_body = prod_body_of(&conditional)
    if(!conditional_body.empty()) {
        env.error("a 304 carried a body")
    }
    var echoed = prod_header(&conditional, std::string_view("ETag"))
    if(!echoed.to_view().equals(etag.to_view())) {
        env.error("a 304 did not echo the ETag")
    }

    // `*` matches any current representation.
    var star_request = string("GET /digits.txt HTTP/1.1\r\nHost: 127.0.0.1\r\nIf-None-Match: *\r\nConnection: close\r\n\r\n")
    var star = prod_exchange(PORT, &star_request, 3000)
    if(!star.to_view().contains(std::string_view("304 Not Modified"))) {
        env.error("If-None-Match: * did not answer 304")
    }

    // A middle slice: 206, the slice's length, and exactly its bytes.
    var part = prod_get_range(PORT, std::string_view("/digits.txt"), std::string_view("bytes=2-5"))
    if(!part.to_view().contains(std::string_view("206 Partial Content"))) {
        env.error("a file range did not answer 206")
    }
    var content_range = prod_header(&part, std::string_view("Content-Range"))
    if(!content_range.to_view().equals(std::string_view("bytes 2-5/20"))) {
        env.error("a file range carried the wrong Content-Range")
    }
    var part_length = prod_header(&part, std::string_view("Content-Length"))
    if(!part_length.to_view().equals(std::string_view("4"))) {
        env.error("a file range advertised the wrong Content-Length")
    }
    var slice = prod_body_of(&part)
    if(!slice.to_view().equals(std::string_view("2345"))) {
        env.error("a file range streamed the wrong bytes")
    }

    // A suffix range reads from the end of the file.
    var suffix = prod_get_range(PORT, std::string_view("/digits.txt"), std::string_view("bytes=-3"))
    var suffix_body = prod_body_of(&suffix)
    if(!suffix_body.to_view().equals(std::string_view("hij"))) {
        env.error("a file suffix range streamed the wrong bytes")
    }

    // Past the end is 416 and says how long the file is.
    var missing = prod_get_range(PORT, std::string_view("/digits.txt"), std::string_view("bytes=99-"))
    if(!missing.to_view().contains(std::string_view("416 Range Not Satisfiable"))) {
        env.error("an unsatisfiable file range did not answer 416")
    }
    var missing_range = prod_header(&missing, std::string_view("Content-Range"))
    if(!missing_range.to_view().equals(std::string_view("bytes */20"))) {
        env.error("416 for a file did not carry Content-Range: bytes */20")
    }

    // A different file is a different validator.
    if(!prod_write_file(root_view, std::string_view("other.txt"), std::string_view("different"))) {
        env.error("could not write the second fixture file")
    }
    var other = prod_get(PORT, std::string_view("/other.txt"))
    var other_etag = prod_header(&other, std::string_view("ETag"))
    if(other_etag.to_view().equals(etag.to_view())) {
        env.error("two different files shared one ETag")
    }

    stop.shutdown()
    thread.join()
}

// ── Response splitting ─────────────────────────────────────────────────────
//
// A handler that builds a header from request data is one `\r\n` away from
// letting the client read the rest of the head as headers of its own. The
// framework refuses the whole head instead of emitting a split response.

func prod_split_dispatch(req : http.Request) : http.Response {
    var want = req.path.to_view()
    if(want.equals(std::string_view("/value"))) {
        var res = http::response_text(302u, std::string_view(""))
        res.set_header_view("Location", "/next\r\nSet-Cookie: pwned=1")
        return res
    }
    if(want.equals(std::string_view("/status"))) {
        var res = http::response_text(400u, std::string_view(""))
        res.status_text = string("Bad\r\nX-Injected: 1")
        return res
    }
    var res = http::response_text(200u, std::string_view("ok"))
    res.set_header_view("X Bad:Name", "value")
    return res
}

func prod_split_report(env : &mut TestEnv, reply : &string, label : string_view, injected : string_view) {
    if(!reply.to_view().contains(std::string_view("500 Internal Server Error"))) {
        var m = string("an unsafe head was not refused: ")
        m.append_view(&label)
        unsafe(test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0))
    }
    if(reply.to_view().contains(&injected)) {
        var m = string("the injection reached the wire: ")
        m.append_view(&label)
        unsafe(test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0))
    }
}

@test
public func PROD_unsafe_response_head_is_refused(env : &mut TestEnv) {
    const PORT : uint = 19958u
    var cfg = server::ServerConfig()
    var addr = framing_addr(PORT)
    cfg.addr = addr
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19958")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("GET", "/:path*", ||(req) => { return prod_split_dispatch(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std.concurrent.sleep_ms(200u)

    // A header value carrying CRLF (the classic `Location` from a query parameter).
    var by_value = prod_get(PORT, std::string_view("/value"))
    prod_split_report(env, &by_value, std::string_view("header value"), std::string_view("pwned"))
    if(by_value.to_view().contains(std::string_view("Location:"))) {
        env.error("the injected Location header reached the wire")
    }

    // A status text carrying CRLF.
    var by_status = prod_get(PORT, std::string_view("/status"))
    prod_split_report(env, &by_status, std::string_view("status text"), std::string_view("X-Injected"))

    // A header name that is not a token.
    var by_name = prod_get(PORT, std::string_view("/name"))
    prod_split_report(env, &by_name, std::string_view("header name"), std::string_view("X Bad"))

    stop.shutdown()
    thread.join()
}

// ── Request splitting (client side) ────────────────────────────────────────
//
// The mirror of the response guard: a caller that puts a value from the wire into
// the request target or a header must not be able to smuggle a second request.
// The refusal happens before the socket is opened, so a test needs no listener.
@test
public func PROD_unsafe_request_head_is_refused(env : &mut TestEnv) {
    var parsed = http::URL.parse(std::string_view("http://127.0.0.1:1/x"))
    if(parsed is Option.None) {
        env.error("could not parse the test URL")
        return
    }
    var Some(u) = parsed else unsafe { unreachable }
    var client = http::Client()

    // A header value carrying CRLF would start a second request.
    var evil = http::RequestBuilder.new("GET", u)
    evil.header_view(std::string_view("X-Forwarded-For"), std::string_view("127.0.0.1\r\nGET /admin HTTP/1.1\r\nHost: x"))
    if(evil.is_wire_safe()) {
        env.error("a header value with CRLF was considered wire-safe")
    }
    var refused = client.request(&evil)
    // The refusal must be the head check, not a connection error to port 1: that
    // would pass this test while the request was written out in full.
    var refused_as_unsafe = false
    if(refused is Result.Err) {
        var Err(refusal) = refused else unsafe { unreachable }
        refused_as_unsafe = refusal.message().equals(std::string_view("request head is not wire-safe"))
    }
    if(!refused_as_unsafe) {
        env.error("a request with CRLF in a header was not refused as unsafe")
    }

    // A request target carrying CRLF. Parsing may refuse the URL outright, which
    // is an equally good outcome; what must not happen is a "safe" verdict.
    var raw_target = http::URL.parse(std::string_view("http://127.0.0.1:1/a\r\nGET /admin HTTP/1.1\r\nHost: x"))
    var target_safe = true
    if(raw_target is Option.Some) {
        var Some(raw_u) = raw_target else unsafe { unreachable }
        var evil_target = http::RequestBuilder.new("GET", raw_u)
        target_safe = evil_target.is_wire_safe()
    }
    if(target_safe) {
        env.error("a request target with CRLF was considered wire-safe")
    }

    // A header name that is not a token.
    var evil_name = http::RequestBuilder.new("GET", u)
    evil_name.header_view(std::string_view("X Bad:Name"), std::string_view("v"))
    if(evil_name.is_wire_safe()) {
        env.error("a header name that is not a token was considered wire-safe")
    }

    // A normal request is still fine.
    var good = http::RequestBuilder.new("GET", u)
    good.header_view(std::string_view("Accept"), std::string_view("text/html"))
    if(!good.is_wire_safe()) {
        env.error("a normal request was refused as unsafe")
    }
}

// ── Ambiguous framing is refused (request smuggling) ───────────────────────
//
// A message that carries both `Transfer-Encoding` and `Content-Length`, or a
// duplicate `Content-Length`, is framed differently by different HTTP implementations.
// If a front proxy frames it one way and this server the other, the leftover bytes
// are read as the *next* request on the reused connection — the attacker's request,
// sent with the server's trust. Guessing is not an option, so both shapes are 400.
//
// The handler here answers 200 for anything; a 200 in the reply means the ambiguous
// message was accepted.
func PROD_smuggling_request(body : string_view, extra : string_view) : string {
    var out = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\n")
    out.append_view(&extra)
    out.append_view("Connection: close\r\n\r\n")
    out.append_view(&body)
    return out
}

@test
public func PROD_ambiguous_request_framing_is_refused(env : &mut TestEnv) {
    const PORT : uint = 19961u
    var cfg = server::ServerConfig()
    var addr = framing_addr(PORT)
    cfg.addr = addr
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19961")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("POST", "/echo", ||(req) => { return prod_handler(req) })
    srv.router.add("GET", "/echo", ||(req) => { return prod_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std.concurrent.sleep_ms(200u)

    // Both framings: chunked body plus a length that disagrees with it.
    var both = PROD_smuggling_request(
        std::string_view("5\r\nhello\r\n0\r\n\r\n"),
        std::string_view("Transfer-Encoding: chunked\r\nContent-Length: 5\r\n"))
    var answer = prod_exchange(PORT, &both, 3000)
    if(!answer.to_view().contains(std::string_view("400 Bad Request"))) {
        env.error("transfer-encoding with content-length was not refused")
    }
    if(answer.to_view().contains(std::string_view("200 OK"))) {
        env.error("an ambiguous request was answered 200")
    }

    // A coding this server does not implement must not fall back to a length.
    var unknown_coding = PROD_smuggling_request(
        std::string_view("hello"),
        std::string_view("Transfer-Encoding: gzip, chunked\r\n"))
    var answer2 = prod_exchange(PORT, &unknown_coding, 3000)
    if(!answer2.to_view().contains(std::string_view("400 Bad Request"))) {
        env.error("an unsupported transfer-encoding was not refused")
    }

    // Duplicate Content-Length fields: `get` would pick the first one silently.
    var duplicate = PROD_smuggling_request(
        std::string_view("hello"),
        std::string_view("Content-Length: 5\r\nContent-Length: 6\r\n"))
    var answer3 = prod_exchange(PORT, &duplicate, 3000)
    if(!answer3.to_view().contains(std::string_view("400 Bad Request"))) {
        env.error("a duplicate content-length was not refused")
    }

    // A single, unambiguous length still works (the guard must not refuse everything).
    var fine = PROD_smuggling_request(
        std::string_view("hello"),
        std::string_view("Content-Length: 5\r\n"))
    var answer4 = prod_exchange(PORT, &fine, 3000)
    if(!answer4.to_view().contains(std::string_view("200 OK"))) {
        env.error("an ordinary content-length request was refused")
    }

    stop.shutdown()
    thread.join()
}
