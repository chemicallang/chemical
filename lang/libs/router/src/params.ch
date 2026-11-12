// Router parameter-store extensions (lang/docs/universal-router-design.md §3, §14.5).
//
// `PageParameter` itself lives in the `page` module (the dependency runs
// `router → page`); this module only adds typed retrieval and query helpers.
//
// The page-stored request/URL/query APIs are RAW: they hand the page a view or
// a pointer that must outlive the render. They live under `router::raw`; the
// canonical owned surface is `router::safe` (`RequestInfo`, `query`).
//
// These are extension functions, so `page.set_route_query(...)` still works
// from code that opts into the raw layer.
//
// The nested `raw` namespace is declared once, here, because Chemical does not
// allow reopening a nested namespace across files.

using std::string_view;

public namespace router {

public namespace raw {

// ── request context (§3.3) ───────────────────────────────────────────────

@direct_init
public struct RouteRequest {
    var method : std::string_view       // "GET"
    var path : std::string_view         // "/projects/42"
    var query : std::string_view        // raw query string, "" if none
    var body : std::string_view         // "" unless read by the caller
}

// Store a caller-owned request pointer on the page. The struct must outlive
// the render; use `router::set_request` for page-owned copies.
public func (html_page : &mut HtmlPage) set_request(req : &RouteRequest) {
    html_page.add_parameter_object_raw(std::string_view("__request"), req as *mut void)
}

public func (page : &mut HtmlPage) get_request() : *mut RouteRequest {
    return get_parameter_object<RouteRequest>(page, std::string_view("__request"))
}

// ── typed object parameters ──────────────────────────────────────────────

// Typed retrieval of an `Object` parameter; null on a miss or a type mismatch.
// The cast is intentionally unchecked (the documented fast PATH). The
// type-safe layer is macro-generated accessors; see §3.2's type-safety contract.
public func <T> get_parameter_object(page : &mut HtmlPage, key : std::string_view) : *mut T { unsafe {
    return page.get_parameter_object_ptr(key) as *mut T
 }}

// ── request URL (§6.1, §6.3.1) ───────────────────────────────────────────

public func (page : &mut HtmlPage) set_route_url(path : std::string_view, base : std::string_view = "") {
    // Copy both values: a computed URL/base may die before the render.
    page.add_parameter_owned(std::string_view("__route_url"), path)     // raw; matched at render time
    page.add_parameter_owned(std::string_view("__route_base"), base)
}

public func (page : &mut HtmlPage) get_route_url() : std::string_view {
    return page.get_parameter(std::string_view("__route_url"))
}

public func (page : &mut HtmlPage) get_route_base() : std::string_view {
    return page.get_parameter(std::string_view("__route_base"))
}

// ── query string (§6.5) ──────────────────────────────────────────────────

// Reads a server query value stored under the `__query_<key>` namespace. The
// returned view is valid for the rest of the render only; use
// `router::query` for an owned copy.
public func (page : &mut HtmlPage) query_param(key : std::string_view) : std::string_view {
    var full = std::string("__query_")
    full.append_view(&key)
    return page.get_owned_parameter(full.to_view())
}

// Stores a request's query string. Every `key[=value]` pair becomes a decoded
// `__query_<key>` parameter that `query_param(key)` reads; the raw string is
// kept under `__route_query` for `get_route_query`. Decoding is `%XX`-only and
// never turns `+` into a space, matching the client's `decodeURIComponent`-based
// `$__uni_parse_query` so the two sides cannot diverge. A leading `?` is
// ignored, a bare key maps to `""`, and later keys win (`k=1&k=2` → `2`).
public func (page : &mut HtmlPage) set_route_query(raw : std::string_view) {
    // Copy into a local: a `string_view` parameter is a pointer in the C ABI and
    // cannot be reassigned (same reason `normalize_path_view` copies to `p`).
    var q = raw
    if(q.size() > 0 && q.at(0) == '?') { q = q.subview(1, q.size()) }
    page.add_parameter_view(std::string_view("__route_query"), q)
    var i : size_t = 0
    while(i < q.size()) {
        const start = i
        while(i < q.size() && q.at(i) != '&') { i = i + 1 }
        const part = q.subview(start, i)
        if(i < q.size()) { i = i + 1 }   // consume '&'
        if(part.size() == 0) { continue }

        var eq = std::NPOS
        for(var j : size_t = 0; j < part.size(); j++) {
            if(part.at(j) == '=') { eq = j; break }
        }
        var rawKey = part
        var rawVal = std::string_view()
        if(eq != std::NPOS) {
            rawKey = part.subview(0, eq)
            rawVal = part.subview(eq + 1, part.size())
        }
        const key = router_decode_segment(rawKey)
        const value = router_decode_segment(rawVal)
        var full = std::string("__query_")
        const kv = key.to_view()
        full.append_view(&kv)
        page.add_owned_parameter(full.to_view(), value.to_view())
    }
}

// The raw query string handed to `set_route_query` ("" when none was set).
public func (page : &mut HtmlPage) get_route_query() : std::string_view {
    return page.get_parameter(std::string_view("__route_query"))
}

// ── redirects (§6.7, §6.7-F) ─────────────────────────────────────────────

// Selects `id` on `router`. When `path` is non-empty it is also recorded so the
// matcher follows it and `get_route_redirect()` can feed a `Location` header.
public func (page : &mut HtmlPage) redirect_to(router : std::string_view, id : std::string_view,
                                               path : std::string_view = "") {
    page.add_parameter_view(router, id)
    if(path.size() > 0) {
        // Page-owned copies: `path` may be computed at runtime and the key is
        // read back by the generated matcher via `get_parameter`.
        page.add_parameter_owned(std::string_view("__route_url"), path)
        page.add_parameter_owned(std::string_view("__route_redirect"), path)
    }
}

// The target path handed to `redirect_to` ("" when this is not a redirect).
public func (page : &mut HtmlPage) get_route_redirect() : std::string_view {
    return page.get_parameter(std::string_view("__route_redirect"))
}

} // namespace raw

} // namespace router

// ── pure helpers (not raw storage) ───────────────────────────────────────
//
// Module scope (not `router::raw`) so `parse_query` stays importable like the
// other pure router helpers; it takes no page and no raw pointer.

// Splits a raw query string (`a=1&b=2`) into `out`, WITHOUT decoding. Kept as a
// pure, testable helper; `set_route_query` performs the decode-and-store step.
// Later keys win (`k=1&k=2` → `2`), matching D-6.5.
public func parse_query(raw : std::string_view, out : &mut std::unordered_map<std::string_view, std::string_view>) {
    var i : size_t = 0
    while(i < raw.size()) {
        var start = i
        while(i < raw.size() && raw.at(i) != '&') { i = i + 1 }
        var part = unsafe(std::string_view(raw.data() + start, i - start))
        if(i < raw.size()) { i = i + 1 }   // consume '&'
        if(part.size() == 0) { continue }

        var eq = std::NPOS
        for(var j : size_t = 0; j < part.size(); j++) {
            if(part.at(j) == '=') { eq = j; break }
        }
        if(eq == std::NPOS) {
            out.insert(part, std::string_view(""))
        } else {
            out.insert(unsafe(std::string_view(part.data(), eq)),
                       unsafe(std::string_view(part.data() + eq + 1, part.size() - eq - 1)))
        }
    }
}
