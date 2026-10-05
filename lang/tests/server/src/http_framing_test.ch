// ============================================================================
// http::server request-body framing — regression tests
// ============================================================================
// A request whose body is *already complete on the wire* must be handed to the
// handler immediately. Two defects made the server treat a finished message as
// unfinished and park a worker inside body_recv() for the whole body timeout
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
// These drive raw sockets because http::Client normalises framing (it always
// sends a Content-Length and never uses chunked), so it cannot express the
// request shapes under test. The handler answers "OBSERVED:<bytes>:<body>",
// which tells the test both what the handler read and that it got there at all
// — no shared mutable state between the worker thread and the test thread.
//
// Run with ./scripts/test.sh --tcc --server
// ============================================================================

using std::string;
using std::string_view;

// The derived body timeout is header_timeout_secs * 4; shrink it so a
// regression shows up as an 8s stall rather than 20s, keeping the suite fast
// while still sitting 8x above the 1s latency budget asserted below.
const FRAM_HEADER_TIMEOUT : long = 2
const FRAM_BUDGET_MS : int = 1000

func framing_handler(req : http.Request, res : http::ResponseWriter) {
    var b = req.body.read_to_string()
    if(b is std::Option.None) {
        var m = string("READFAIL")
        res.write_string(&m)
        return
    }
    var Some(s) = b else unreachable
    var out = string("OBSERVED:")
    out.append_uinteger(s.size() as ubigint)
    out.append(':')
    out.append_string(&s)
    res.write_string(&out)
}

// ---------------------------------------------------------------------------
// Harness
// ---------------------------------------------------------------------------

// Send `raw` on a fresh connection to 127.0.0.1:port and read until the peer
// closes. Appends the response text to `resp`. Returns elapsed milliseconds,
// or -1 if no response arrived within the socket budget (which is a failure,
// not something to retry — the whole point is that the answer is instant).
func raw_exchange(port : uint, raw : &string, budget_ms : int, resp : *mut string) : int {
    var s = net::dial("127.0.0.1", port)
    if(s == 0u || (s as longlong) < 0) { return -1 }
    net::set_recv_timeout(s, budget_ms / 1000, 0)

    var t0 = std::chrono::Instant::now()
    net::send_all(s, raw.data(), raw.size() as int)

    var acc = string::empty_str()
    var chunk : [4096]u8
    while(true) {
        var n = net::recv_all(s, &raw mut chunk[0], 4096u)
        if(n <= 0) { break }
        acc.append_view(std::string_view(&chunk[0] as *char, n as size_t))
    }
    var el = t0.elapsed().as_millis() as int
    net::close_socket(s)

    resp.append_string(&acc)
    return el
}

// Same, but writes the request in two pieces with `gap_ms` between them, so a
// chunk terminator split across TCP segments can be exercised. The body_recv
// re-scan loop is exactly what makes that work, so it must survive the fix.
func raw_exchange_split(port : uint, part1 : &string, part2 : &string, gap_ms : int, budget_ms : int, resp : *mut string) : int {
    var s = net::dial("127.0.0.1", port)
    if(s == 0u || (s as longlong) < 0) { return -1 }
    net::set_recv_timeout(s, budget_ms / 1000, 0)

    var t0 = std::chrono::Instant::now()
    net::send_all(s, part1.data(), part1.size() as int)
    std::concurrent.sleep_ms(gap_ms as ulong)
    net::send_all(s, part2.data(), part2.size() as int)

    var acc = string::empty_str()
    var chunk : [4096]u8
    while(true) {
        var n = net::recv_all(s, &raw mut chunk[0], 4096u)
        if(n <= 0) { break }
        acc.append_view(std::string_view(&chunk[0] as *char, n as size_t))
    }
    var el = t0.elapsed().as_millis() as int
    net::close_socket(s)

    resp.append_string(&acc)
    return el
}

// One latency + content assertion. Reports every failure it finds rather than
// bailing on the first, so a single run shows the whole picture.
func framing_check(env : &mut TestEnv, label : *char, resp : &string, expect_marker : *char, elapsed_ms : int) {
    if(elapsed_ms < 0) {
        var m = string::make_no_len(label)
        m.append_view(": no response at all — the server blocked on the request body")
        env.error(m.data())
        return
    }
    if(elapsed_ms > FRAM_BUDGET_MS) {
        var m = string::make_no_len(label)
        m.append_view(": answered in ")
        m.append_uinteger(elapsed_ms as ubigint)
        m.append_view("ms, budget is ")
        m.append_uinteger(FRAM_BUDGET_MS as ubigint)
        m.append_view("ms")
        env.error(m.data())
    }
    if(!resp.starts_with("HTTP/1.1 200")) {
        var m = string::make_no_len(label)
        m.append_view(": expected a 200 response, got: ")
        m.append_view(resp.to_view())
        env.error(m.data())
        return
    }
    if(!resp.contains(string_view(expect_marker))) {
        var m = string::make_no_len(label)
        m.append_view(": handler did not observe ")
        m.append_view(string_view(expect_marker))
        m.append_view(", response was: ")
        m.append_view(resp.to_view())
        env.error(m.data())
    }
}

// ===========================================================================
// Bug 1 — a request with no body must not be treated as read-until-close
// ===========================================================================

// A POST whose handler reads the body must answer immediately when the request
// carries no Content-Length at all. RFC 9112 §6.3: no Content-Length and no
// Transfer-Encoding means the message has no body. The server instead frames
// it as unknown-length, so the handler blocks in body_recv() for the full body
// timeout and only then sees "".
@test
@test.timeout(120000)
public func FRAM_request_without_content_length_answers_immediately(env : &mut TestEnv) {
    const PORT : uint = 19890u

    var cfg = server::ServerConfig()
    cfg.addr = string("127.0.0.1:19890")
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var srv = server::Server(cfg)
    srv.router.add("POST", "/echo", ||(req, res) => { framing_handler(req, res) })
    var thread = srv.serve_async(PORT)
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)

    framing_check(env, "no Content-Length", &resp, "OBSERVED:0:", el)

    srv.shutdown()
    thread.join()
}

// Same framing, but the client states the empty body explicitly — what curl,
// fetch and every other HTTP client send for an empty POST. This also arrives
// as req.body_len == 0, so the two cases are indistinguishable by the time the
// server frames the body; they need separate coverage.
@test
@test.timeout(120000)
public func FRAM_request_with_zero_content_length_answers_immediately(env : &mut TestEnv) {
    const PORT : uint = 19891u

    var cfg = server::ServerConfig()
    cfg.addr = string("127.0.0.1:19891")
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var srv = server::Server(cfg)
    srv.router.add("POST", "/echo", ||(req, res) => { framing_handler(req, res) })
    var thread = srv.serve_async(PORT)
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 0\r\nConnection: close\r\n\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)

    framing_check(env, "Content-Length: 0", &resp, "OBSERVED:0:", el)

    srv.shutdown()
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
    cfg.addr = string("127.0.0.1:19892")
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var srv = server::Server(cfg)
    srv.router.add("POST", "/echo", ||(req, res) => { framing_handler(req, res) })
    var thread = srv.serve_async(PORT)
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 7\r\nConnection: close\r\n\r\n{\"a\":1}")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)

    framing_check(env, "Content-Length: 7", &resp, "OBSERVED:7:{\"a\":1}", el)

    srv.shutdown()
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
    cfg.addr = string("127.0.0.1:19893")
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var srv = server::Server(cfg)
    srv.router.add("POST", "/echo", ||(req, res) => { framing_handler(req, res) })
    var thread = srv.serve_async(PORT)
    std::concurrent.sleep_ms(200u)

    var head = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n")

    // 2 trailing bytes: no trailer section, just the final CRLF — the shortest
    // legal ending, and the one the 4-byte scan could never match.
    var r2 = head.copy()
    r2.append_view(string_view("0\r\n\r\n"))
    var p2 = string::empty_str()
    var e2 = raw_exchange(PORT, &r2, 3000, &raw mut p2)
    framing_check(env, "chunked, 2 trailing bytes", &p2, "OBSERVED:0:", e2)

    // 4 trailing bytes: already handled by the old scan, kept as a guard that
    // the rewrite does not regress it.
    var r4 = head.copy()
    r4.append_view(string_view("0\r\n\r\n\r\n"))
    var p4 = string::empty_str()
    var e4 = raw_exchange(PORT, &r4, 3000, &raw mut p4)
    framing_check(env, "chunked, 4 trailing bytes", &p4, "OBSERVED:0:", e4)

    // 6 trailing bytes.
    var r6 = head.copy()
    r6.append_view(string_view("0\r\n\r\n\r\n\r\n"))
    var p6 = string::empty_str()
    var e6 = raw_exchange(PORT, &r6, 3000, &raw mut p6)
    framing_check(env, "chunked, 6 trailing bytes", &p6, "OBSERVED:0:", e6)

    srv.shutdown()
    thread.join()
}

// A client that stops after the last-chunk line and never sends the mandatory
// final CRLF has sent an incomplete message. The server cannot know that from
// the byte stream alone, so it waits — but the wait must be BOUNDED by the
// body timeout, not unbounded. This pins that: the answer eventually arrives
// with an empty body rather than the connection hanging forever.
@test
@test.timeout(300000)
public func FRAM_truncated_chunked_body_terminates_at_body_timeout(env : &mut TestEnv) {
    const PORT : uint = 19896u
    // header_timeout_secs * 4, plus slack for scheduling.
    const ALLOWED : int = 20000

    var cfg = server::ServerConfig()
    cfg.addr = string("127.0.0.1:19896")
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var srv = server::Server(cfg)
    srv.router.add("POST", "/echo", ||(req, res) => { framing_handler(req, res) })
    var thread = srv.serve_async(PORT)
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n0\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, ALLOWED, &raw mut resp)

    // The socket budget is already ALLOWED, so a response here means the server
    // gave up on its own within the body timeout rather than hanging.
    if(el < 0) {
        env.error("truncated chunked body: server never terminated the read")
    } else if(el > ALLOWED) {
        var m = string("truncated chunked body: took ")
        m.append_uinteger(el as ubigint)
        m.append_view("ms, must be bounded by the body timeout")
        env.error(m.data())
    } else if(!resp.contains("OBSERVED:0:")) {
        var m = string("truncated chunked body: handler should still see an empty body, got: ")
        m.append_view(resp.to_view())
        env.error(m.data())
    }

    srv.shutdown()
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
    cfg.addr = string("127.0.0.1:19894")
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var srv = server::Server(cfg)
    srv.router.add("POST", "/echo", ||(req, res) => { framing_handler(req, res) })
    var thread = srv.serve_async(PORT)
    std::concurrent.sleep_ms(200u)

    var req = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n7\r\nGoodbye\r\n0\r\nX-Trailer: t\r\n\r\n")
    var resp = string::empty_str()
    var el = raw_exchange(PORT, &req, 3000, &raw mut resp)

    framing_check(env, "chunked with trailer", &resp, "OBSERVED:7:Goodbye", el)

    srv.shutdown()
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
    cfg.addr = string("127.0.0.1:19895")
    cfg.header_timeout_secs = FRAM_HEADER_TIMEOUT
    var srv = server::Server(cfg)
    srv.router.add("POST", "/echo", ||(req, res) => { framing_handler(req, res) })
    var thread = srv.serve_async(PORT)
    std::concurrent.sleep_ms(200u)

    var p1 = string("POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n0\r\n")
    var p2 = string("\r\n")
    var resp = string::empty_str()
    var el = raw_exchange_split(PORT, &p1, &p2, 120, 3000, &raw mut resp)

    framing_check(env, "chunked terminator split across sends", &resp, "OBSERVED:0:", el)

    srv.shutdown()
    thread.join()
}