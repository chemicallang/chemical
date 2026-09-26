// Pure URL pattern matching (lang/docs/universal-router-design.md §6.1, §15.4).
//
// No `HtmlPage`, no globals: the same rules must be usable by the server matcher,
// by the emitter's validation, and by tests. Precedence is resolved at emission
// time (D-6.9), so `match_route` is a straight first-match scan.

// One `{name}` captured from a matched path. `value` is a view into the input
// `path` (which the caller owns for the lifetime of the match); percent decoding
// is performed by the URL layer, not here.
public variant RouteParam {
    Param(name : std::string_view, value : std::string_view)
}

// One activation step *after* a matched route's root (§6.4 nested URL
// ownership): the router registry `reg` and the route id to activate within it.
// A top-level route has an empty chain; a nested URL route's chain walks from
// the root layout down to the leaf, so the runtime can reproduce the nested
// selection the way the URL spells it (D-2.7 nested params ride along).
public struct RouteChainStep {
    var reg : std::string_view
    var id : std::string_view
}

// A compiled route pattern. `segments` holds literal segments (`"projects"`) and
// brace params (`"{id}"`); `is_fallback` marks `route *` (D-6.1). `prefix` marks
// a nested-fallback entry that matches the pattern plus any remaining segments
// (so `/projects/{id}` catches `/projects/42/unknown`); exact entries always win
// because they are scanned first. `chain` is the nested activation chain (empty
// for a top-level route).
public struct RoutePattern {
    var segments : std::vector<std::string_view>
    var id : std::string_view
    var is_fallback : bool
    var prefix : bool
    var chain : std::vector<RouteChainStep>
}

public struct RouteMatch {
    var matched : bool
    var id : std::string_view
    var params : std::vector<RouteParam>
    var is_fallback : bool
    var chain : std::vector<RouteChainStep>
}

// Splits a declared pattern (`/projects/{id}`) into segments, dropping empty
// pieces produced by the leading slash (matching the client matcher, which skips
// empty segments).
public func pattern_segments(pattern : std::string_view) : std::vector<std::string_view> {
    var out = std::vector<std::string_view>()
    var i : size_t = 0
    while(i < pattern.size()) {
        // skip separators
        while(i < pattern.size() && pattern.get(i) == '/') { i = i + 1 }
        if(i >= pattern.size()) { break }
        const start = i
        while(i < pattern.size() && pattern.get(i) != '/') { i = i + 1 }
        out.push(pattern.subview(start, i))
    }
    return out
}

// A view of `path` normalized for matching: query stripped, `base` prefix
// stripped, one trailing slash removed (except root). Returns a view into `path`.
public func normalize_path_view(path : std::string_view, base : std::string_view) : std::string_view {
    var p = path
    const q = p.find(std::string_view("?"))
    if(q != std::NPOS) { p = p.subview(0, q) }
    if(base.size() > 0 && p.size() >= base.size() && p.starts_with(&base)) {
        p = p.subview(base.size(), p.size())
    }
    if(p.size() == 0) { return std::string_view("/") }
    if(p.size() > 1 && p.get(p.size() - 1) == '/') {
        p = p.subview(0, p.size() - 1)
    }
    return p
}

// Owning variant of `normalize_path_view`, for tests and callers that need to
// store the result (the view variant is what matching uses).
public func normalize_path(path : std::string_view, base : std::string_view) : std::string {
    const v = normalize_path_view(path, base)
    return std::string(v.data(), v.size())
}

func split_path_segments(path : std::string_view, base : std::string_view) : std::vector<std::string_view> {
    var out = std::vector<std::string_view>()
    const p = normalize_path_view(path, base)
    var i : size_t = 0
    while(i < p.size()) {
        while(i < p.size() && p.get(i) == '/') { i = i + 1 }
        if(i >= p.size()) { break }
        const start = i
        while(i < p.size() && p.get(i) != '/') { i = i + 1 }
        out.push(p.subview(start, i))
    }
    return out
}

func is_param_segment(s : std::string_view) : bool {
    return s.size() >= 2 && s.get(0) == '{' && s.get(s.size() - 1) == '}'
}

// Straight first-match scan over `patterns` (already in precedence order,
// D-6.9). A `route *` entry matches any remaining path (D-6.6).
public func match_route(patterns : &std::vector<RoutePattern>, path : std::string_view,
                        base : std::string_view) : RouteMatch {
    var noMatch = RouteMatch {
        matched : false,
        id : std::string_view(),
        params : std::vector<RouteParam>(),
        is_fallback : false,
        chain : std::vector<RouteChainStep>()
    }
    const segs = split_path_segments(path, base)
    for(var i : size_t = 0; i < patterns.size(); i++) {
        const pattern = patterns.get_ptr(i)
        if(pattern.is_fallback) {
            return RouteMatch {
                matched : true,
                id : pattern.id,
                params : std::vector<RouteParam>(),
                is_fallback : true,
                chain : std::vector<RouteChainStep>()
            }
        }
        if(pattern.prefix) {
            if(segs.size() < pattern.segments.size()) { continue }
        } else if(pattern.segments.size() != segs.size()) {
            continue
        }
        var params = std::vector<RouteParam>()
        var ok = true
        for(var j : size_t = 0; j < pattern.segments.size(); j++) {
            const ps = pattern.segments.get(j)
            if(is_param_segment(ps)) {
                params.push(RouteParam.Param(ps.subview(1, ps.size() - 1), segs.get(j)))
            } else {
                const seg = segs.get(j)
                if(!ps.equals(&seg)) {
                    ok = false
                    break
                }
            }
        }
        if(ok) {
            var chain = std::vector<RouteChainStep>()
            for(var c : uint = 0; c < pattern.chain.size(); c++) {
                chain.push(pattern.chain.get(c))
            }
            return RouteMatch {
                matched : true,
                id : pattern.id,
                params : params,
                is_fallback : false,
                chain : chain
            }
        }
    }
    return noMatch
}


