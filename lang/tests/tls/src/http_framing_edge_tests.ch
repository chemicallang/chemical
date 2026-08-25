// ============================================================================
// HTTP response framing edge cases — full stack (http::Client -> TLS -> net)
// ============================================================================
// Each test starts a scripted python HTTPS server whose raw byte responses are
// crafted to exercise one HTTP framing rule end-to-end. The client must parse
// the response correctly after TLS record reassembly.
//
// Conformance probes (expected-correct behavior is asserted even where the
// current client likely has a bug) are marked with CONFORMANCE and cite the
// RFC section. A failure there indicates a real bug, not an invalid test.
//
// Ports used: 20410-20417.
// ============================================================================

using namespace tls
using namespace http
using std::Result
using std::string
using std::string_view
using std::vector

// Start the scripted seqsrv with the given hex script file.
func frm_start_seqsrv(env : &mut TestEnv, port : uint, tag : string_view,
                      script_path : *char, nconn : uint) : bool {
    write_http_extra_py()
    xpy_kill_and_wait(port)
    xpy_gen_cert(tag)
    var cmd = xpy_server_cmd("seqsrv", tag, port)
    cmd.append_view(" ")
    cmd.append_view(string_view(script_path))
    cmd.append_view(" ")
    cmd.append_uinteger(nconn as ubigint)
    xpy_bg(cmd.to_view())
    test_server_wait()
    return true
}

func frm_url(port : uint, path : string_view) : string {
    var s = string("https://127.0.0.1:")
    s.append_uinteger(port as ubigint)
    s.append_view(&path)
    return s
}

// ─── 1. No Content-Length + Connection: close → read-until-close ────────────
// RFC 9112 §6.3: close-delimited responses are valid; the body is everything
// read until EOF.
@test
@test.timeout(60000)
public func FRAM_close_delimited_no_content_length(env : &mut TestEnv) {
    const PORT : uint = 20410u
    var body = full_pattern(1000u, 0u)

    var resp = string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n")
    resp.append_view(body.to_view())

    var lines = vector<string>()
    lines.push_back(resp)
    var script = test_tmp_file("frm_seq_20410.txt")
    if(!xpy_write_seq_script(script.data(), lines)) { env.error("script write failed"); return }
    frm_start_seqsrv(env, PORT, "frm10", script.data(), 1u)

    var client = Client()
    client.insecure_skip_verify()

    var u = frm_url(PORT, "/anything")
    var res = client.get(u.to_view())
    if(res is Result.Err) { env.error("close-delimited request failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("expected 200") }

    // No Content-Length => remaining must be -1 (unknown / until close).
    if(r.body.content_length() != -1) { env.error("body should be close-delimited (remaining == -1)") }

    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("close-delimited body read failed"); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(v.size() != 1000u) {
        env.error("close-delimited length mismatch")
        var msg = string("got ")
        msg.append_uinteger(v.size() as ubigint)
        env.error(msg.data())
    } else if(!v.equals_view(body.to_view())) {
        env.error("close-delimited bytes mismatch")
    }

    test_kill_port(PORT as int)
}

// ─── 2. 204 and 304 carry no body at all ─────────────────────────────────────
@test
@test.timeout(60000)
public func FRAM_empty_bodies_204_and_304(env : &mut TestEnv) {
    const PORT : uint = 20411u
    var l1 = string("HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n")
    var l2 = string("HTTP/1.1 304 Not Modified\r\nETag: \"abc123\"\r\nConnection: close\r\n\r\n")

    var lines = vector<string>()
    lines.push_back(l1)
    lines.push_back(l2)
    var script = test_tmp_file("frm_seq_20411.txt")
    if(!xpy_write_seq_script(script.data(), lines)) { env.error("script write failed"); return }
    frm_start_seqsrv(env, PORT, "frm11", script.data(), 2u)

    var client = Client()
    client.insecure_skip_verify()

    var u = frm_url(PORT, "/x")
    var r1 = client.get(u.to_view())
    if(r1 is Result.Err) { env.error("204 request failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    if(x1.status != 204u) { env.error("expected status 204") }
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.None) { env.error("204 body read failed") } else {
        var Some(v1) = b1 else unreachable;
        if(v1.size() != 0u) { env.error("204 body should be empty") }
    }

    var r2 = client.get(u.to_view())
    if(r2 is Result.Err) { env.error("304 request failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    if(x2.status != 304u) { env.error("expected status 304") }
    var et = x2.headers.get("etag")
    if(et is std.Option.None) { env.error("ETag header lost on 304") } else {
        var Some(ev) = et else unreachable;
        if(!ev.equals_view("\"abc123\"")) { env.error("ETag value mismatch") }
    }
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("304 body read failed") } else {
        var Some(v2) = b2 else unreachable;
        if(v2.size() != 0u) { env.error("304 body should be empty") }
    }

    test_kill_port(PORT as int)
}

// ─── 3. Zero-length chunked body and chunk trailers ──────────────────────────
// Response A ends immediately ("0\r\n\r\n"). Response B carries a trailer
// section after the terminating chunk, which must not corrupt the body.
@test
@test.timeout(60000)
public func FRAM_chunked_zero_length_and_trailers(env : &mut TestEnv) {
    const PORT : uint = 20412u
    var l1 = string("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n0\r\n\r\n")
    var l2 = string("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n7\r\nGoodbye\r\n0\r\nX-Trailer: t\r\n\r\n")

    var lines = vector<string>()
    lines.push_back(l1)
    lines.push_back(l2)
    var script = test_tmp_file("frm_seq_20412.txt")
    if(!xpy_write_seq_script(script.data(), lines)) { env.error("script write failed"); return }
    frm_start_seqsrv(env, PORT, "frm12", script.data(), 2u)

    var client = Client()
    client.insecure_skip_verify()

    var u = frm_url(PORT, "/c")
    var r1 = client.get(u.to_view())
    if(r1 is Result.Err) { env.error("zero-chunk request failed"); test_kill_port(PORT as int); return }
    var Ok(x1) = r1 else unreachable;
    if(!x1.body.is_chunked()) { env.error("response A should be flagged chunked") }
    var b1 = x1.body.read_to_string()
    if(b1 is std.Option.None) { env.error("zero-chunk body read failed") } else {
        var Some(v1) = b1 else unreachable;
        if(v1.size() != 0u) { env.error("zero-chunk body should be empty") }
    }

    var r2 = client.get(u.to_view())
    if(r2 is Result.Err) { env.error("trailer request failed"); test_kill_port(PORT as int); return }
    var Ok(x2) = r2 else unreachable;
    var b2 = x2.body.read_to_string()
    if(b2 is std.Option.None) { env.error("trailer body read failed"); test_kill_port(PORT as int); return }
    var Some(v2) = b2 else unreachable;
    if(!v2.equals_view("Goodbye")) { env.error("chunk+trailer body mismatch") }

    test_kill_port(PORT as int)
}

// ─── 4. Chunk extensions must be ignored ─────────────────────────────────────
// CONFORMANCE (RFC 9112 §7.1.1): "a recipient MUST ignore unrecognized chunk
// extensions". "5;ext=v\r\nHello\r\n" must deliver "Hello".
@test
@test.timeout(60000)
public func FRAM_chunk_extensions_must_be_ignored(env : &mut TestEnv) {
    const PORT : uint = 20413u
    var l1 = string("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n5;ext=v\r\nHello\r\n0\r\n\r\n")

    var lines = vector<string>()
    lines.push_back(l1)
    var script = test_tmp_file("frm_seq_20413.txt")
    if(!xpy_write_seq_script(script.data(), lines)) { env.error("script write failed"); return }
    frm_start_seqsrv(env, PORT, "frm13", script.data(), 1u)

    var client = Client()
    client.insecure_skip_verify()

    var u = frm_url(PORT, "/e")
    var res = client.get(u.to_view())
    if(res is Result.Err) {
        env.error("[CONFORMANCE] chunk extensions must be ignored (RFC 9112 7.1.1)")
        test_kill_port(PORT as int)
        return
    }
    var Ok(r) = res else unreachable;
    var b = r.body.read_to_string()
    if(b is std.Option.None) {
        env.error("[CONFORMANCE] chunk ext caused body read failure")
        test_kill_port(PORT as int)
        return
    }
    var Some(v) = b else unreachable;
    if(!v.equals_view("Hello")) { env.error("[CONFORMANCE] chunk ext corrupted body") }

    test_kill_port(PORT as int)
}

// ─── 5. Dribbled response across many tiny TLS records ──────────────────────
// The python server writes headers + body in 13-byte pieces with delays, so
// every TLS record lands separately; the client must buffer partial header
// reads then stream the body byte-exactly.
@test
@test.timeout(90000)
public func FRAM_dribbled_response_reassembly(env : &mut TestEnv) {
    const PORT : uint = 20414u
    write_http_extra_py()
    xpy_kill_and_wait(PORT)
    xpy_gen_cert("frm14")
    var cmd = xpy_server_cmd("dribblesrv", "frm14", PORT)
    cmd.append_view(" 2048 13 4")
    xpy_bg(cmd.to_view())
    test_server_wait()

    var client = Client()
    client.insecure_skip_verify()

    var u = frm_url(PORT, "/drip")
    var res = client.get(u.to_view())
    if(res is Result.Err) { env.error("dribbled request failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("dribble expected 200") }
    var dr = r.headers.get("X-Dribble")
    if(dr is std.Option.None) { env.error("X-Dribble header missing") }

    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("dribbled body read failed"); test_kill_port(PORT as int); return }
    var Some(v) = b else unreachable;
    if(v.size() != 2048u) {
        env.error("dribbled length mismatch")
        var msg = string("got ")
        msg.append_uinteger(v.size() as ubigint)
        env.error(msg.data())
    } else {
        var i : size_t = 0
        while(i < v.size()) {
            if((v.get(i) as u8) != ((i % 251) as u8)) { env.error("dribbled pattern mismatch"); break }
            i += 1
        }
    }

    test_kill_port(PORT as int)
}

// ─── 6. HEAD response without any length info → empty body ──────────────────
// Server sends headers only (no CL, no TE) and closes immediately. The client
// must report an empty body rather than erroring or hanging.
@test
@test.timeout(60000)
public func FRAM_head_response_headers_only(env : &mut TestEnv) {
    const PORT : uint = 20415u
    var l1 = string("HTTP/1.1 200 OK\r\nX-A: b\r\nConnection: close\r\n\r\n")

    var lines = vector<string>()
    lines.push_back(l1)
    var script = test_tmp_file("frm_seq_20415.txt")
    if(!xpy_write_seq_script(script.data(), lines)) { env.error("script write failed"); return }
    frm_start_seqsrv(env, PORT, "frm15", script.data(), 1u)

    var client = Client()
    client.insecure_skip_verify()

    var u = frm_url(PORT, "/h")
    var res = client.head(u.to_view())
    if(res is Result.Err) { env.error("HEAD headers-only request failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) { env.error("HEAD expected 200") }
    var xa = r.headers.get("X-A")
    if(xa is std.Option.None) { env.error("HEAD lost X-A header") }
    var b = r.body.read_to_string()
    if(b is std.Option.None) { env.error("HEAD empty-body read should succeed") } else {
        var Some(v) = b else unreachable;
        if(v.size() != 0u) { env.error("HEAD body should be empty") }
    }

    test_kill_port(PORT as int)
}

// ─── 7. HEAD with Content-Length but no body bytes ──────────────────────────
// CONFORMANCE (RFC 9110 §9.3.2 / §8.6): a HEAD response has NO message body;
// Content-Length here describes what a GET would produce and MUST NOT cause
// the client to wait for body bytes. The server closes immediately so this
// cannot hang: a compliant client returns an empty body instantly.
@test
@test.timeout(60000)
public func FRAM_head_with_content_length_has_no_body(env : &mut TestEnv) {
    const PORT : uint = 20416u
    var l1 = string("HTTP/1.1 200 OK\r\nContent-Length: 5\r\nConnection: close\r\n\r\n")

    var lines = vector<string>()
    lines.push_back(l1)
    var script = test_tmp_file("frm_seq_20416.txt")
    if(!xpy_write_seq_script(script.data(), lines)) { env.error("script write failed"); return }
    frm_start_seqsrv(env, PORT, "frm16", script.data(), 1u)

    var client = Client()
    client.insecure_skip_verify()

    var u = frm_url(PORT, "/h")
    var res = client.head(u.to_view())
    if(res is Result.Err) { env.error("HEAD CL request failed"); test_kill_port(PORT as int); return }
    var Ok(r) = res else unreachable;

    var b = r.body.read_to_string()
    if(b is std.Option.None) {
        env.error("[CONFORMANCE] HEAD body must be treated as absent regardless of Content-Length")
    } else {
        var Some(v) = b else unreachable;
        if(v.size() != 0u) { env.error("[CONFORMANCE] HEAD body must be empty") }
    }

    test_kill_port(PORT as int)
}

// ─── 8. Interim 1xx response must be skipped ────────────────────────────────
// CONFORMANCE (RFC 9110 §6.2.1): a client MUST be able to skip any number of
// interim 1xx responses before the final response. The server sends
// "100 Continue" followed by the real final response in one write.
@test
@test.timeout(60000)
public func FRAM_interim_100_continue_is_skipped(env : &mut TestEnv) {
    const PORT : uint = 20417u
    var l1 = string("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK")

    var lines = vector<string>()
    lines.push_back(l1)
    var script = test_tmp_file("frm_seq_20417.txt")
    if(!xpy_write_seq_script(script.data(), lines)) { env.error("script write failed"); return }
    frm_start_seqsrv(env, PORT, "frm17", script.data(), 1u)

    var client = Client()
    client.insecure_skip_verify()

    var u = frm_url(PORT, "/i")
    var res = client.get(u.to_view())
    if(res is Result.Err) {
        env.error("[CONFORMANCE] interim 1xx must be skipped, not fatal")
        test_kill_port(PORT as int)
        return
    }
    var Ok(r) = res else unreachable;
    if(r.status != 200u) {
        env.error("[CONFORMANCE] final status must win over interim 100")
    } else {
        var b = r.body.read_to_string()
        if(b is std.Option.None) {
            env.error("[CONFORMANCE] final body unreadable after interim skip")
        } else {
            var Some(v) = b else unreachable;
            if(!v.equals_view("OK")) { env.error("[CONFORMANCE] body polluted by interim response") }
        }
    }

    test_kill_port(PORT as int)
}
