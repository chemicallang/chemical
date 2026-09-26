/**
 * Server-side URL matching, called by the generated router function during
 * render (design §6.1).
 *
 * The generated function owns the compile-time patterns and hands them to this
 * function as a compact, tab/newline-delimited spec:
 *
 *   "<id>\t<pattern>\t<is_fallback>\n"   (one line per URL route / the fallback)
 *
 * Matching uses the same `match_route` the client matcher mirrors, so the server
 * and client cannot diverge on the match rule. On a match the selected route id
 * and its `{param}` values are stored in the page parameter store, which is what
 * `page.route_selected` and the client activation tail read.
 *
 * Only URL routes and the `route *` fallback are listed: id routes have no URL
 * semantics and are selected by the app (`add_parameter`) or the declared
 * default.
 *
 * A free function (not an `HtmlPage` extension) so the compiler plugin can
 * resolve it as a top-level symbol and emit a direct call; extension methods are
 * not reachable through `ASTNode.child` from the plugin.
 */

func router_hex_digit(c : char) : int {
    if(c >= '0' && c <= '9') { return (c as int) - ('0' as int) }
    if(c >= 'a' && c <= 'f') { return (c as int) - ('a' as int) + 10 }
    if(c >= 'A' && c <= 'F') { return (c as int) - ('A' as int) + 10 }
    return -1
}

// Percent-decodes a matched path segment into a fresh `std::string`. Matches the
// client's `decodeURIComponent` semantics: only `%XX` is decoded (never `+`),
// malformed input degrades to the raw text, and decoding happens after segment
// splitting so `%2F` is data (D-6.3).
func router_decode_segment(value : std::string_view) : std::string {
    var out = std::string()
    var i : size_t = 0
    while(i < value.size()) {
        const c = value.get(i)
        if(c == '%' && i + 2 < value.size()) {
            const hi = router_hex_digit(value.get(i + 1))
            const lo = router_hex_digit(value.get(i + 2))
            if(hi >= 0 && lo >= 0) {
                out.append((hi * 16 + lo) as char)
                i = i + 3
                continue
            }
        }
        out.append(c)
        i = i + 1
    }
    return out
}

public func apply_route_url(page : &mut HtmlPage, router : std::string_view, spec : std::string_view) {
    const url = page.get_parameter(std::string_view("__route_url"))
    // No request URL was handed to the page: leave the declared default in place.
    if(url.size() == 0) { return }
    const base = page.get_parameter(std::string_view("__route_base"))

    var patterns = std::vector<RoutePattern>()
    var i : size_t = 0
    while(i < spec.size()) {
        var lineEnd = i
        while(lineEnd < spec.size() && spec.get(lineEnd) != '\n') { lineEnd = lineEnd + 1 }
        const line = spec.subview(i, lineEnd)
        i = lineEnd + 1
        if(line.size() == 0) { continue }

        const t1 = line.find(std::string_view("\t"))
        if(t1 == std::NPOS) { continue }
        const id = line.subview(0, t1)
        const rest = line.subview(t1 + 1, line.size())
        const t2 = rest.find(std::string_view("\t"))
        if(t2 == std::NPOS) { continue }
        const pattern = rest.subview(0, t2)
        const rest2 = rest.subview(t2 + 1, rest.size())
        const t3 = rest2.find(std::string_view("\t"))
        var fb = rest2
        var chainField = std::string_view()
        if(t3 != std::NPOS) {
            fb = rest2.subview(0, t3)
            chainField = rest2.subview(t3 + 1, rest2.size())
        }

        patterns.push(RoutePattern {
            segments : pattern_segments(pattern),
            id : id,
            is_fallback : fb.size() > 0 && fb.get(0) == '1',
            chain : parse_route_chain(chainField)
        })
    }
    if(patterns.size() == 0) { return }

    const m = match_route(&mut patterns, url, base)
    if(m.matched) {
        page.add_parameter(router, m.id)
        for(var k : size_t = 0; k < m.params.size(); k++) {
            const p = m.params.get_ptr(k)
            var Param(name, value) = *p else unreachable
            // Decoded bytes are not page-lifetime: store them in page-owned
            // storage so the route's SSR parameter reads stay valid.
            var decoded = router_decode_segment(value)
            page.add_parameter_owned(name, decoded.to_view())
        }
        // Nested URL ownership (§6.4/§13.3.3): store each level's selected id
        // under its derived registry name so the nested wrappers render visible
        // server-side, mirroring the client activation chain.
        for(var c : size_t = 0; c < m.chain.size(); c++) {
            const step = m.chain.get(c)
            page.add_parameter(step.reg, step.id)
        }
    } else {
        page.mark_route_missing()
    }
}

// Parses the unit/record-separated activation chain emitted by the converter:
// `reg` + US + `id` (RS between steps), e.g. `r#a` US `x` RS ... An empty field
// yields an empty chain (a top-level route).
func parse_route_chain(field : std::string_view) : std::vector<RouteChainStep> {
    var out = std::vector<RouteChainStep>()
    if(field.size() == 0) { return out }
    const rec = std::string_view("\x1e")
    const unit = std::string_view("\x1f")
    var i : size_t = 0
    while(i < field.size()) {
        const rel = field.subview(i, field.size() - i).find(&rec)
        var end = field.size()
        if(rel != std::NPOS) { end = i + rel }
        const step = field.subview(i, end)
        if(step.size() > 0) {
            const sep = step.find(&unit)
            if(sep != std::NPOS) {
                out.push(RouteChainStep { reg : step.subview(0, sep), id : step.subview(sep + 1, step.size()) })
            }
        }
        if(rel == std::NPOS) { break }
        i = end + 1
    }
    return out
}
