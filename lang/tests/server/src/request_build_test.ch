// ============================================================================
// http::request — building a request without a socket
// ============================================================================
// A test, a fixture or a router probe needs a `http.Request` it made itself. The
// server fills one from the request line, so the parts that are easy to get wrong
// are the ones the parser does for it: splitting `?query` off the path, decoding
// it into `query`, and the `Cookie` / `Content-Type` headers a handler reads.
//
// These tests pin that `http::request` behaves exactly like the parser: a handler
// cannot tell a hand-built request from a received one.
//
// Run with ./scripts/test.sh --tcc --server
// ============================================================================

using std::Option;
using std::string;
using std::string_view;

@test
public func HTTPBUILD_method_and_path_are_copied(env : &mut TestEnv) {
    var req = http::request(string_view("POST"), string_view("/todos/7"))
    var method = req.method.to_view()
    var path = req.path.to_view()
    if(!method.equals(&string_view("POST"))) {
        env.error("method was not copied")
        return
    }
    if(!path.equals(&string_view("/todos/7"))) {
        env.error("path was not copied")
        return
    }
    var proto = req.proto.to_view()
    if(!proto.equals(&string_view("HTTP/1.1"))) {
        env.error("proto should default to HTTP/1.1")
    }
}

@test
public func HTTPBUILD_query_is_split_off_the_path(env : &mut TestEnv) {
    var req = http::request(string_view("GET"), string_view("/search?q=two%20words&page=2"))
    var path = req.path.to_view()
    if(!path.equals(&string_view("/search"))) {
        env.error("the query string should not stay in the path")
        return
    }
    var q = req.query_param("q")
    if(q is Option.None) {
        env.error("q should be present")
        return
    }
    var Some(qv) = q else unsafe { unreachable }
    if(!qv.equals(&string_view("two words"))) {
        env.error("q should be percent-decoded")
        return
    }
    var page = req.query_param("page")
    if(page is Option.None) {
        env.error("page should be present")
        return
    }
    var Some(pv) = page else unsafe { unreachable }
    if(!pv.equals(&string_view("2"))) {
        env.error("page should be 2")
    }
}

@test
public func HTTPBUILD_a_path_without_a_query_has_an_empty_query(env : &mut TestEnv) {
    var req = http::request(string_view("GET"), string_view("/plain"))
    var path = req.path.to_view()
    if(!path.equals(&string_view("/plain"))) {
        env.error("path should be unchanged")
        return
    }
    if(req.query.len() != 0u) {
        env.error("a path with no ? should have no query parameters")
        return
    }
    var q = req.query_param("q")
    if(q is Option.Some) {
        env.error("q should be absent")
    }
}

@test
public func HTTPBUILD_empty_query_values_survive(env : &mut TestEnv) {
    // `?a=` is Some("") — distinguishable from a missing parameter, which is what
    // a form validation path relies on.
    var req = http::request(string_view("GET"), string_view("/x?a=&b=1"))
    var a = req.query_param("a")
    if(a is Option.None) {
        env.error("a should be present with an empty value")
        return
    }
    var Some(av) = a else unsafe { unreachable }
    if(!av.empty()) {
        env.error("a should be empty")
    }
}

@test
public func HTTPBUILD_the_helpers_set_what_a_handler_reads(env : &mut TestEnv) {
    var req = http::request(string_view("POST"), string_view("/login"))
    req.with_cookie(string_view("sid=abc"))
    req.with_form(string_view("email=a%40b.c&password=x"))
    var cookie = req.header("Cookie")
    if(cookie is Option.None) {
        env.error("Cookie should be set")
        return
    }
    var Some(seen) = cookie else unsafe { unreachable }
    if(!seen.equals(&string_view("sid=abc"))) {
        env.error("Cookie value was not preserved")
        return
    }
    if(!req.is_content_type("application/x-www-form-urlencoded")) {
        env.error("with_form should set the form content type")
        return
    }
    var body = req.body_text()
    if(!body.equals(&string_view("email=a%40b.c&password=x"))) {
        env.error("with_form should copy the body")
        return
    }
    // `is_form_post()` is the pair a handler actually branches on.
    if(!req.is_form_post()) {
        env.error("a form POST should report is_form_post")
    }
}

@test
public func HTTPBUILD_json_body_is_typed(env : &mut TestEnv) {
    var req = http::request(string_view("POST"), string_view("/api/todos"))
    req.with_json(string_view("{\"title\":\"x\"}"))
    if(!req.is_content_type("application/json")) {
        env.error("with_json should set application/json")
        return
    }
    var body = req.body_text()
    if(!body.equals(&string_view("{\"title\":\"x\"}"))) {
        env.error("with_json should copy the body")
    }
}
