// P2.5 — safe router surface: owned request snapshot + query access.
//
// Legacy `get_request(page) : *mut RouteRequest` stays available. These tests
// only use the pointer-free façade.

using namespace std;

func rs_eq(value : &string, expected : string_view) : bool {
    return value.equals_view(&expected)
}

@test
func router_safe_request_round_trip(env : &mut TestEnv) {
    var page = HtmlPage()
    router::set_request(&mut page, "POST", "/projects/42", "tab=files", "hello")

    var got = router::get_request(&page)
    if(got is Option.None) { env.error("stored request missing"); return }
    var Some(info) = got else unsafe { unreachable }

    if(!rs_eq(&info.method, string_view("POST"))) { env.error("method did not round-trip"); return }
    if(!rs_eq(&info.path, string_view("/projects/42"))) { env.error("path did not round-trip"); return }
    if(!rs_eq(&info.query, string_view("tab=files"))) { env.error("query did not round-trip"); return }
    if(!rs_eq(&info.body, string_view("hello"))) { env.error("body did not round-trip"); return }
    if(!info.has_body()) { env.error("has_body should be true"); return }
}

@test
func router_safe_missing_request_is_none(env : &mut TestEnv) {
    var page = HtmlPage()
    var got = router::get_request(&page)
    if(got is Option.Some) { env.error("request should be absent before set_request"); return }
}

@test
func router_safe_query_values_and_absence(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_query(string_view("a=1&b=two&empty=&flag"))

    var a = router::query(&page, "a")
    if(a is Option.None) { env.error("query key 'a' missing"); return }
    var Some(a_text) = a else unsafe { unreachable }
    if(!rs_eq(&a_text, string_view("1"))) { env.error("query 'a' decoded wrong"); return }

    var b = router::query(&page, "b")
    if(b is Option.None) { env.error("query key 'b' missing"); return }
    var Some(b_text) = b else unsafe { unreachable }
    if(!rs_eq(&b_text, string_view("two"))) { env.error("query 'b' decoded wrong"); return }

    // A present-but-empty value is still present.
    var empty = router::query(&page, "empty")
    if(empty is Option.None) { env.error("empty query value should be present"); return }
    var Some(empty_text) = empty else unsafe { unreachable }
    if(empty_text.size() != 0u) { env.error("empty query value should be empty"); return }

    var missing = router::query(&page, "missing")
    if(missing is Option.Some) { env.error("missing query key should be None"); return }
}
