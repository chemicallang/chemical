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
    var full = std::string()
    full.append_view(std::string_view("__query_"))
    full.append_view(&key)
    return page.get_parameter(full.to_view())
}

// Splits a raw query string (`a=1&b=2`) into `out`. Values are NOT
// percent-decoded here — the URL layer decodes with `encoding::url_decode`.
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
