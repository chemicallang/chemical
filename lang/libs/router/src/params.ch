// Router parameter-store extensions (lang/docs/universal-router-design.md §3, §14.5).
//
// `PageParameter` itself lives in the `page` module (the dependency runs
// `router → page`); this module only adds typed retrieval and query helpers.

// Typed retrieval of an `Object` parameter; null on a miss or a type mismatch.
// The cast is intentionally unchecked (the documented fast PATH). The
// type-safe layer is macro-generated accessors; see §3.2's type-safety contract.
public func <T> get_parameter_object(page : &mut HtmlPage, key : std::string_view) : *mut T {
    const p = page.parameters.get_ptr(&key)
    if(p == null) { return null }
    if(p is PageParameter.Object) {
        var Object(ptr) = *p else return null
        return ptr as *mut T
    }
    return null
}

// Reads a server query value stored under the `__query_<key>` namespace (§6.5).
public func (page : &mut HtmlPage) query_param(key : std::string_view) : std::string_view {
    var full = std::string("__query_")
    full.append_view(&key)
    return page.get_owned_parameter(full.to_view())
}

// Stores a request's query string (§6.5). Every `key[=value]` pair becomes a
// decoded `__query_<key>` parameter that `query_param(key)` reads; the raw string
// is kept under `__route_query` for `get_route_query`. Decoding is `%XX`-only and
// never turns `+` into a space, matching the client's `decodeURIComponent`-based
// `$__uni_parse_query` so the two sides cannot diverge. A leading `?` is ignored,
// a bare key maps to `""`, and later keys win (`k=1&k=2` → `2`).
//
// The app calls this next to `set_route_url` (e.g. with `req.query`); matching
// itself only needs the path.
public func (page : &mut HtmlPage) set_route_query(raw : std::string_view) {
    // Copy into a local: a `string_view` parameter is a pointer in the C ABI and
    // cannot be reassigned (same reason `normalize_path_view` copies to `p`).
    var q = raw
    if(q.size() > 0 && q.get(0) == '?') { q = q.subview(1, q.size()) }
    page.add_parameter(std::string_view("__route_query"), q)
    var i : size_t = 0
    while(i < q.size()) {
        const start = i
        while(i < q.size() && q.get(i) != '&') { i = i + 1 }
        const part = q.subview(start, i)
        if(i < q.size()) { i = i + 1 }   // consume '&'
        if(part.size() == 0) { continue }

        var eq = std::NPOS
        for(var j : size_t = 0; j < part.size(); j++) {
            if(part.get(j) == '=') { eq = j; break }
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

// Splits a raw query string (`a=1&b=2`) into `out`, WITHOUT decoding. Kept as a
// pure, testable helper; `set_route_query` performs the decode-and-store step.
// Later keys win (`k=1&k=2` → `2`), matching D-6.5.
public func parse_query(raw : std::string_view, out : &mut std::unordered_map<std::string_view, std::string_view>) {
    var i : size_t = 0
    while(i < raw.size()) {
        var start = i
        while(i < raw.size() && raw.get(i) != '&') { i = i + 1 }
        var part = std::string_view(raw.data() + start, i - start)
        if(i < raw.size()) { i = i + 1 }   // consume '&'
        if(part.size() == 0) { continue }

        var eq = std::NPOS
        for(var j : size_t = 0; j < part.size(); j++) {
            if(part.get(j) == '=') { eq = j; break }
        }
        if(eq == std::NPOS) {
            out.insert(part, std::string_view(""))
        } else {
            out.insert(std::string_view(part.data(), eq),
                       std::string_view(part.data() + eq + 1, part.size() - eq - 1))
        }
    }
}
