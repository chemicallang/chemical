// ============================================================================
// http::server request-body framing — regression tests
// ============================================================================
// A request whose body is *already complete on the wire* must be handed to the
// handler immediately. Two defects once made the server treat a finished message
// as unfinished and park a worker inside body_recv() for the whole body timeout
// (header_timeout_secs * 4) before answering:
//
//   Bug 1 — a request with NO body (no Content-Length, or `Content-Length: 0`)
//           was framed as "body of unknown length" (remaining == -1, i.e.
//           read-until-close). That framing is correct for a *response* and
//           meaningless for a request: RFC 9112 §6.3 says a request's length
//           comes from Transfer-Encoding, else Content-Length, and if neither
//           is present the message simply has no body. A keep-alive peer is
//           never going to close, so the read blocked until the timeout and
//           only then produced the empty body the handler should have had
//           immediately.
//
//   Bug 2 — a chunked request ending in the zero-length chunk blocked on the
//           terminator scan, which looked for a literal 4-byte "\r\n\r\n".
//           RFC 9112 §7.1 says the last chunk `0\r\n` is followed by an
//           optional trailer section and a final CRLF, so the shortest legal
//           ending — a bare "0\r\n\r\n", no trailer — leaves only 2 bytes past
//           the last-chunk line: a perfectly complete message the 4-byte scan
//           could never match.
//
// Both bugs emit the CORRECT response, merely orders of magnitude too late,
// which is exactly why they survive a suite that only asserts on status codes.
// Every test below therefore asserts latency as well as content, and each
// group carries a control case that is already fast and must stay fast.
//
// The body is now framed and fully buffered by the framework *before* dispatch
// (`http::server::Server::serve_ready` → `http::read_request_body`), so a
// handler owns a finished `http.Request` and never touches the connection. That
// makes the two properties above structural, and these tests pin them: a
// complete message must be answered immediately, and a bodyless request must be
// framed as EMPTY (0 bytes) rather than "unknown, read until close".
//
// These drive raw sockets because http::Client normalises framing (it always
// sends a Content-Length and never uses chunked), so it cannot express the
// request shapes under test. The handler answers "OBSERVED:<bytes>:<body>",
// sourced from `req.body.size()` and the buffered body text, which tells the
// test both what the handler read and that it got there at all — no shared
// mutable state between the worker thread and the test thread.
//
// Run with ./scripts/test.sh --tcc --server
// ============================================================================

using std::string;
using std::string_view;
using std::Result;

// The derived body timeout is header_timeout_secs * 4; shrink it so a
// regression shows up as an 8s stall rather than 20s, keeping the suite fast
// while still sitting 8x above the 1s latency budget asserted below.
const FRAM_HEADER_TIMEOUT : long = 2
const FRAM_BUDGET_MS : int = 1000

// `cfg.addr` is the port's only home now (`serve_async` takes no port), while
// the raw harness still dials by port number, so build the address from the
// same constant to keep the two from drifting apart.
func framing_addr(port : uint) : string {
    var out = string("127.0.0.1:")
    out.append_uinteger(port as ubigint)
    return out
}

// The handler owns the request and returns the response: the framework keeps the
// connection, so there is no writer to touch and no lifetime to carry. The body
// is already complete, so `size()` is the number of bytes the peer sent and
// `read_to_string()` returns them without blocking.
func framing_handler(req : http.Request) : http.Response {
    var total = req.body.size()
    var read = req.body.read_to_string()
    if(read is Result.Err) {
        return http.response_text(500u, "READFAIL")
    }
    var Ok(s) = read else unsafe { unreachable }
    var out = string("OBSERVED:")
    out.append_uinteger(total as ubigint)
    out.append(':')
    out.append_string(&s)
    var view = out.to_view()
    return http.response_text(200u, &view)
}

// Reports the framing the server chose for the request body. The white-box
// test below uses this to assert `remaining == 0` (empty) directly, so a
// regression to "unknown / read until close" fails immediately instead of only
// showing up as an 8-second stall. Now that the framework buffers the body
// before dispatch nothing has consumed it yet, so the handler-visible
// `remaining` is exactly the framed length.
func framing_remaining_handler(req : http.Request) : http.Response {
    var out = string("REMAINING:")
    out.append_uinteger(req.body.remaining() as ubigint)
    var view = out.to_view()
    return http.response_text(200u, &view)
}

// ---------------------------------------------------------------------------
// Harness
// ---------------------------------------------------------------------------

// Send `raw` on a fresh connection to 127.0.0.1:port and read until the peer
// closes. Appends the response text to `resp`. Returns elapsed milliseconds,
// or -1 if no response arrived within the socket budget (which is a failure,
// not something to retry — the whole point is that the answer is instant).
func raw_exchange(port : uint, raw : &string, budget_ms : int, resp : *mut string) : int {
    unsafe {

    var s = net::raw::dial("127.0.0.1", port)
    if(s == 0u || (s as longlong) < 0) { return -1 }
    net::raw::set_recv_timeout(s, budget_ms / 1000, 0)

    var t0 = std::chrono::Instant::now()
    net::raw::send_all(s, raw.data(), raw.size() as int)

    var acc = string::empty_str()
    var chunk : [4096]u8
    while(true) {
        var n = net::raw::recv_all(s, &raw mut chunk[0], 4096u)
        if(n <= 0) { break }
        var piece = std::string_view(&chunk[0] as *char, n as size_t)
        acc.append_view(&piece)
    }
    var el = t0.elapsed().as_millis() as int
    net::raw::close_socket(s)

    resp.append_string(&acc)
    return el

}
}

// Same, but writes the request in two pieces with `gap_ms` between them, so a
// chunk terminator split across TCP segments can be exercised. body_recv's
// re-scan loop (now `take_chunked`'s buffered line scan) is exactly what makes
// that work, so it must survive the rewrite.
func raw_exchange_split(port : uint, part1 : &string, part2 : &string, gap_ms : int, budget_ms : int, resp : *mut string) : int {
    unsafe {

    var s = net::raw::dial("127.0.0.1", port)
    if(s == 0u || (s as longlong) < 0) { return -1 }
    net::raw::set_recv_timeout(s, budget_ms / 1000, 0)

    var t0 = std::chrono::Instant::now()
    net::raw::send_all(s, part1.data(), part1.size() as int)
    std::concurrent.sleep_ms(gap_ms as ulong)
    net::raw::send_all(s, part2.data(), part2.size() as int)

    var acc = string::empty_str()
    var chunk : [4096]u8
    while(true) {
        var n = net::raw::recv_all(s, &raw mut chunk[0], 4096u)
        if(n <= 0) { break }
        var piece = std::string_view(&chunk[0] as *char, n as size_t)
        acc.append_view(&piece)
    }
    var el = t0.elapsed().as_millis() as int
    net::raw::close_socket(s)

    resp.append_string(&acc)
    return el

}
}

// One latency + content assertion. Reports every failure it finds rather than
// bailing on the first, so a single run shows the whole picture.
func framing_check(env : &mut TestEnv, label : *char, resp : &string, expect_marker : *char, elapsed_ms : int) {
    unsafe {

    if(elapsed_ms < 0) {
        var m = string::make_no_len(label)
        m.append_view(": no response at all — the server blocked on the request body")
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
        return
    }
    if(elapsed_ms > FRAM_BUDGET_MS) {
        var m = string::make_no_len(label)
        m.append_view(": answered in ")
        m.append_uinteger(elapsed_ms as ubigint)
        m.append_view("ms, budget is ")
        m.append_uinteger(FRAM_BUDGET_MS as ubigint)
        m.append_view("ms")
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    }
    if(!resp.starts_with("HTTP/1.1 200")) {
        var m = string::make_no_len(label)
        m.append_view(": expected a 200 response, got: ")
        var got = resp.to_view()
        m.append_view(&got)
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
        return
    }
    if(!resp.contains(string_view(expect_marker))) {
        var expected = string_view(expect_marker)
        var m = string::make_no_len(label)
        m.append_view(": handler did not observe ")
        m.append_view(&expected)
        m.append_view(", response was: ")
        var got = resp.to_view()
        m.append_view(&got)
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    }

}
}

// ===========================================================================
// Bug 1 — a request with no body must not be treated as read-until-close
// ===========================================================================

// A POST whose handler reads the body must answer immediately when the request
// carries no Content-Length at all. RFC 9112 §6.3: no Content-Length and no
// Transfer-Encoding means the message has no body. The server instead framed
// it as unknown-length, so the handler blocked in body_recv() for the full body
// timeout and only then saw "".
@test
@test.timeout(120000)
public func FRAM_request_without_content_length_answers_immediately(env : &mut TestEnv) {
    const PORT : uint = 19890u

    var cfg = server::ServerConfig()
    cfg.addr = framing_addr(PORT)
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19890")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("POST", "/echo", ||(req) => { return framing_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)

    framing_check(env, "no Content-Length", &resp, "OBSERVED:0:", el)

    stop.shutdown()
    thread.join()
}

// Same framing, but the client states the empty body explicitly — what curl,
// fetch and every other HTTP client send for an empty POST. This also arrives
// as a declared length of 0, so the two cases are indistinguishable by the time
// the server frames the body; they need separate coverage.
@test
@test.timeout(120000)
public func FRAM_request_with_zero_content_length_answers_immediately(env : &mut TestEnv) {
    const PORT : uint = 19891u

    var cfg = server::ServerConfig()
    cfg.addr = framing_addr(PORT)
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19891")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("POST", "/echo", ||(req) => { return framing_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)

    framing_check(env, "Content-Length: 0", &resp, "OBSERVED:0:", el)

    stop.shutdown()
    thread.join()
}

// White-box companion to the latency tests above: assert the framing value
// itself, not just how long it takes. RFC 9112 §6.3 says a request with no
// Transfer-Encoding and no Content-Length has NO body, so the framed body must
// be 0 bytes ("empty"). The bug framed it as "unknown / read until close",
// which makes every handler that reads the body block until the peer closes or
// the body timeout fires. Checking the value makes the regression fail
// deterministically, with no timing dependency — and the latency bound is
// asserted too, because the old framing still produced the right number after
// stalling for the body timeout.
@test
@test.timeout(120000)
public func FRAM_bodyless_request_is_framed_empty_not_unknown(env : &mut TestEnv) {
    const PORT : uint = 19897u

    var cfg = server::ServerConfig()
    cfg.addr = framing_addr(PORT)
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19897")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("GET", "/remaining", ||(req) => { return framing_remaining_handler(req) })
    srv.router.add("POST", "/remaining", ||(req) => { return framing_remaining_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(200u)

    // The production case: a bodyless GET. Before the fix this reported -1.
    var req = string("GET /remaining HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)
    if(el < 0) {
        env.error("bodyless GET: no response")
    } else if(!resp.contains("REMAINING:0")) {
        var m = string("bodyless GET: expected REMAINING:0 (empty body), got: ")
        var got = resp.to_view()
        m.append_view(&got)
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    } else if(el > FRAM_BUDGET_MS) {
        var m = string("bodyless GET: answered in ")
        m.append_uinteger(el as ubigint)
        m.append_view("ms, budget is ")
        m.append_uinteger(FRAM_BUDGET_MS as ubigint)
        m.append_view("ms")
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    }

    // Control: an explicit Content-Length must still be honored, so a "fix"
    // that hard-codes 0 for every request also fails this test.
    var req2 = string("POST /remaining HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 7\r\nConnection: close\r\n\r\n{\"a\":1}")
    var resp2 = string::empty_str()
    var el2 = raw_exchange(PORT, &req2, 3000, &raw mut resp2)
    if(el2 < 0) {
        env.error("Content-Length: 7: no response")
    } else if(!resp2.contains("REMAINING:7")) {
        var m = string("Content-Length: 7: expected REMAINING:7, got: ")
        var got = resp2.to_view()
        m.append_view(&got)
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    }

    stop.shutdown()
    thread.join()
}

// Control: a request that really does carry a body must still answer
// immediately AND still deliver those bytes. This is what catches a "fix"
// that made every request body read as empty — a latency assertion alone would
// sail straight through such a change.
@test
@test.timeout(120000)
public func FRAM_request_with_body_is_unaffected(env : &mut TestEnv) {
    const PORT : uint = 19892u

    var cfg = server::ServerConfig()
    cfg.addr = framing_addr(PORT)
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19892")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("POST", "/echo", ||(req) => { return framing_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 7\r\nConnection: close\r\n\r\n{\"a\":1}")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)

    framing_check(env, "Content-Length: 7", &resp, "OBSERVED:7:{\"a\":1}", el)

    stop.shutdown()
    thread.join()
}

// ===========================================================================
// Bug 2 — the chunked terminator must not require a 4-byte "\r\n\r\n"
// ===========================================================================

// RFC 9112 §7.1: `last-chunk CRLF`, then an optional trailer section, then a
// final CRLF. So a complete chunked body ends with 2 bytes past the `0\r\n`
// (no trailer), or more if a trailer section is present. Every one of those is
// a complete message and must be answered at once; before the fix the 2-byte
// case fell through to body_recv() and stalled for the whole body timeout,
// because the terminator scan looked for a fixed 4-byte "\r\n\r\n".
//
// Deliberately NOT covered here: a `0\r\n` with NOTHING after it. That is a
// truncated message (the final CRLF is mandatory), and TCP carries no message
// boundary, so a server cannot distinguish "terminator still in flight" from
// "client stopped sending" — answering immediately there would break
// FRAM_chunked_terminator_split_across_segments below. The wait is bounded by
// the body timeout instead, which is the correct behaviour; see
// FRAM_truncated_chunked_body_terminates_at_body_timeout.
@test
@test.timeout(300000)
public func FRAM_chunked_terminator_with_fewer_than_four_trailing_bytes(env : &mut TestEnv) {
    const PORT : uint = 19893u

    var cfg = server::ServerConfig()
    cfg.addr = framing_addr(PORT)
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19893")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("POST", "/echo", ||(req) => { return framing_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(200u)

    var head = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n")

    // 2 trailing bytes: no trailer section, just the final CRLF — the shortest
    // legal ending, and the one the 4-byte scan could never match.
    var term2 = string_view("0\r\n\r\n")
    var r2 = head.copy()
    r2.append_view(&term2)
    var p2 = string::empty_str()
    var e2 = raw_exchange(PORT, &r2, 3000, &raw mut p2)
    framing_check(env, "chunked, 2 trailing bytes", &p2, "OBSERVED:0:", e2)

    // 4 trailing bytes: already handled by the old scan, kept as a guard that
    // the rewrite does not regress it.
    var term4 = string_view("0\r\n\r\n\r\n")
    var r4 = head.copy()
    r4.append_view(&term4)
    var p4 = string::empty_str()
    var e4 = raw_exchange(PORT, &r4, 3000, &raw mut p4)
    framing_check(env, "chunked, 4 trailing bytes", &p4, "OBSERVED:0:", e4)

    // 6 trailing bytes.
    var term6 = string_view("0\r\n\r\n\r\n\r\n")
    var r6 = head.copy()
    r6.append_view(&term6)
    var p6 = string::empty_str()
    var e6 = raw_exchange(PORT, &r6, 3000, &raw mut p6)
    framing_check(env, "chunked, 6 trailing bytes", &p6, "OBSERVED:0:", e6)

    stop.shutdown()
    thread.join()
}

// A client that stops after the last-chunk line and never sends the mandatory
// final CRLF has sent an incomplete message. The server cannot know that from
// the byte stream alone, so it waits — but the wait must be BOUNDED by the
// body timeout, not unbounded. This pins that: the answer arrives inside the
// body timeout (header_timeout_secs * 4) instead of the worker parking forever.
//
// The rejection is now the framework's, not the handler's: because the body is
// framed and buffered before dispatch, an incomplete body means the handler is
// never invoked and the peer gets a 400. Before the framework owned framing the
// handler was called anyway and saw an empty body after the stall, so the
// assertion moved from "handler observed 0 bytes" to "the request was rejected
// promptly" — the property under test (bounded, no hang) is unchanged.
@test
@test.timeout(300000)
public func FRAM_truncated_chunked_body_terminates_at_body_timeout(env : &mut TestEnv) {
    const PORT : uint = 19896u
    // header_timeout_secs * 4, plus slack for scheduling.
    const ALLOWED : int = 20000

    var cfg = server::ServerConfig()
    cfg.addr = framing_addr(PORT)
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    // The body deadline is its own setting now: the answer must arrive after it
    // fires, not after the (much longer) default.
    cfg.body_timeout_secs = 2
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19896")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("POST", "/echo", ||(req) => { return framing_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n0\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, ALLOWED, &raw mut resp)

    // The socket budget is already ALLOWED, so a response here means the server
    // gave up on its own within the configured body timeout rather than hanging.
    if(el < 0) {
        env.error("truncated chunked body: server never terminated the read")
    } else if(el > ALLOWED) {
        var m = string("truncated chunked body: took ")
        m.append_uinteger(el as ubigint)
        m.append_view("ms, must be bounded by the body timeout")
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    } else if(!resp.starts_with("HTTP/1.1 400")) {
        var m = string("truncated chunked body: expected a 400 rejection once the body timeout fires, got: ")
        var got = resp.to_view()
        m.append_view(&got)
        test_env::ffi::log_cstr(env, LogType.Error, m.data(), 0, 0)
    }

    stop.shutdown()
    thread.join()
}

// A chunked body with a trailer section must terminate at the trailer's empty
// line and still deliver the real payload. This is the case a naive "consume
// through the first CRLF" fix would get wrong.
@test
@test.timeout(120000)
public func FRAM_chunked_body_with_trailer_delivers_payload(env : &mut TestEnv) {
    const PORT : uint = 19894u

    var cfg = server::ServerConfig()
    cfg.addr = framing_addr(PORT)
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19894")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("POST", "/echo", ||(req) => { return framing_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n7\r\nGoodbye\r\n0\r\nX-Trailer: t\r\n\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)

    framing_check(env, "chunked with trailer", &resp, "OBSERVED:7:Goodbye", el)

    stop.shutdown()
    thread.join()
}

// A terminator split across two TCP segments. body_recv()'s re-scan loop is
// precisely what makes this work, so this test exists to stop a fix from
// "simplifying" the scan into something that only handles bytes already sitting
// in the buffer.
@test
@test.timeout(120000)
public func FRAM_chunked_terminator_split_across_segments(env : &mut TestEnv) {
    const PORT : uint = 19895u

    var cfg = server::ServerConfig()
    cfg.addr = framing_addr(PORT)
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var bound = server.Server.bind(cfg)
    if(bound is Result.Err) {
        env.error("could not bind 127.0.0.1:19895")
        return
    }
    var Ok(srv) = bound else unsafe { unreachable }
    srv.router.add("POST", "/echo", ||(req) => { return framing_handler(req) })
    var stop = srv.handle()
    var thread = srv.serve_async()
    std::concurrent.sleep_ms(200u)

    var p1 = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n0\r\n")
    var p2 = string("\r\n")
    var resp = string::empty_str()
    var el = raw_exchange_split(PORT, &p1, &p2, 120, 3000, &raw mut resp)

    framing_check(env, "chunked terminator split across sends", &resp, "OBSERVED:0:", el)

    stop.shutdown()
    thread.join()
}
