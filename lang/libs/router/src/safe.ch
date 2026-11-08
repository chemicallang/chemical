// safe — additive pointer-free request/query façade for the router.
//
// The legacy `set_request`/`get_request` pair stores a caller-owned
// `*mut RouteRequest` in the page parameter map, and `get_parameter_object<T>`
// returns a raw pointer. Those stay for existing callers. The functions here
// are the normal-use surface:
//
//   * `router::safe::set_request(page, method, path, query, body)` copies the
//     request into page-owned storage;
//   * `router::safe::get_request(page)` returns an owned `RequestInfo` (or
//     `None`), never a raw pointer;
//   * `router::safe::query(page, key)` returns the decoded query value as an
//     owned string.

public namespace router {

public namespace safe {

using std::Option;
using std::string;
using std::string_view;

// Owned snapshot of the request context. Copying it is safe: each field owns
// its bytes, independent of the page and of the caller's request buffers.
@direct_init
public struct RequestInfo {
    var method : string
    var path : string
    var query : string
    var body : string

    public func has_body(&self) : bool {
        return body.size() > 0
    }
}

// Store a page-owned copy of the request context. The render can then read it
// through `get_request` without the caller keeping the source buffers alive.
public func set_request(page : &mut HtmlPage, method : &std::str, path : &std::str,
                        query : &std::str, body : &std::str) {
    page.add_parameter_owned(string_view("__request_method"), method.to_view())
    page.add_parameter_owned(string_view("__request_path"), path.to_view())
    page.add_parameter_owned(string_view("__request_query"), query.to_view())
    page.add_parameter_owned(string_view("__request_body"), body.to_view())
}

func request_field(page : &HtmlPage, key : string_view) : string {
    var view = page.get_owned_parameter(key)
    var out = string()
    out.append_view(&view)
    return out
}

// The stored request, or None when `set_request` was never called.
public func get_request(page : &HtmlPage) : Option<RequestInfo> {
    if(!page.has_parameter(string_view("__request_method"))) {
        return Option.None<RequestInfo>()
    }
    var info = RequestInfo {
        method : request_field(page, string_view("__request_method")),
        path : request_field(page, string_view("__request_path")),
        query : request_field(page, string_view("__request_query")),
        body : request_field(page, string_view("__request_body"))
    }
    return Option.Some<RequestInfo>(info)
}

// A decoded query value (`set_route_query` must have run), or None when the
// key is absent. The result owns its bytes.
public func query(html_page : &HtmlPage, key : &std::str) : Option<string> {
    var full = string("__query_")
    var kv = key.to_view()
    full.append_view(&kv)
    // Presence must be checked in the owned-parameter store: `set_route_query`
    // uses `add_owned_parameter` (route_param_owned), not the view-keyed
    // `parameters` map.
    var lookup = string()
    lookup.append_view(full.to_view())
    if(!html_page.has_owned_parameter(lookup.to_view())) {
        return Option.None<string>()
    }
    var view = html_page.get_owned_parameter(full.to_view())
    var out = string()
    out.append_view(&view)
    return Option.Some<string>(out)
}

} // namespace safe

} // namespace router
