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
