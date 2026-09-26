using namespace std;

// ---------------------------------------------------------------------------
// Universal router query-parse tests (Phase 1)
// ---------------------------------------------------------------------------

@test
public func test_router_parse_query_basic(env : &mut TestEnv) {
    var out = std::unordered_map<std::string_view, std::string_view>()
    parse_query("a=1&b=2", &mut out)
    if(out.size() != 2) {
        env.error("parse_query should yield two entries")
        return
    }
    const a = out.get_ptr(&std::string_view("a"))
    const b = out.get_ptr(&std::string_view("b"))
    if(a == null || b == null) {
        env.error("parse_query should contain a and b")
        return
    }
    if(!a.equals(std::string_view("1")) || !b.equals(std::string_view("2"))) {
        env.error("parse_query values should be 1 and 2")
    }
}

@test
public func test_router_parse_query_last_wins(env : &mut TestEnv) {
    var out = std::unordered_map<std::string_view, std::string_view>()
    parse_query("k=1&k=2", &mut out)
    const k = out.get_ptr(&std::string_view("k"))
    if(k == null) {
        env.error("parse_query should contain k")
        return
    }
    if(!k.equals(std::string_view("2"))) {
        env.error("parse_query should keep the last value for a repeated key")
    }
}

@test
public func test_router_parse_query_flag_without_value(env : &mut TestEnv) {
    var out = std::unordered_map<std::string_view, std::string_view>()
    parse_query("flag", &mut out)
    const f = out.get_ptr(&std::string_view("flag"))
    if(f == null) {
        env.error("parse_query should contain a bare flag")
        return
    }
    if(f.size() != 0) {
        env.error("a bare flag should map to an empty value")
    }
}

@test
public func test_router_parse_query_empty(env : &mut TestEnv) {
    var out = std::unordered_map<std::string_view, std::string_view>()
    parse_query("", &mut out)
    if(out.size() != 0) {
        env.error("parse_query of an empty string should yield no entries")
    }
}

// ---------------------------------------------------------------------------
// `set_route_query` — the server half of §6.5 (stores decoded `__query_<k>`).
// ---------------------------------------------------------------------------

@test
public func test_router_set_route_query_stores_decoded_params(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_query("a=1&b=hello%20world")
    if(!page.query_param("a").equals(std::string_view("1"))) {
        env.error("query_param('a') should be 1")
    }
    if(!page.query_param("b").equals(std::string_view("hello world"))) {
        env.error("a query value should be percent-decoded")
        env.info(page.query_param("b").data())
    }
    if(!page.get_route_query().equals(std::string_view("a=1&b=hello%20world"))) {
        env.error("get_route_query should return the raw query")
    }
    if(page.query_param("missing").size() != 0) {
        env.error("a missing query param should read as empty")
    }
}

@test
public func test_router_set_route_query_plus_stays_literal(env : &mut TestEnv) {
    // Client parity: `decodeURIComponent` never turns `+` into a space, so the
    // server must not either (note `encoding::url_decode` WOULD, hence the
    // dedicated decoder).
    var page = HtmlPage()
    page.set_route_query("q=a+b")
    if(!page.query_param("q").equals(std::string_view("a+b"))) {
        env.error("`+` must stay a literal in a query value")
        env.info(page.query_param("q").data())
    }
}

@test
public func test_router_set_route_query_last_wins_and_bare_key(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_query("k=1&k=2&flag")
    if(!page.query_param("k").equals(std::string_view("2"))) {
        env.error("a repeated query key should keep the last value")
    }
    if(page.query_param("flag").size() != 0) {
        env.error("a bare query key should map to an empty value")
    }
}

@test
public func test_router_set_route_query_strips_question_and_decodes_key(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_query("?a%20b=1")
    if(!page.query_param("a b").equals(std::string_view("1"))) {
        env.error("a percent-encoded query key should be decoded")
    }
    if(!page.get_route_query().equals(std::string_view("a%20b=1"))) {
        env.error("the stored raw query should not include the leading '?'")
    }
}

@test
public func test_router_set_route_query_empty(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_query("")
    if(!page.get_route_query().equals(std::string_view(""))) {
        env.error("an empty query should store an empty raw string")
    }
    if(page.query_param("a").size() != 0) {
        env.error("an empty query should define no params")
    }
}
