// Reverse of matching: build a URL path for a route id and params
// (lang/docs/universal-router-design.md §6.6, D-6.4, §15.4). Kept separate from
// `match.ch` so the server matcher stays small.
//
// `build_path` percent-encodes each `{name}` segment with `url_encode` (a `/` in
// a param becomes `%2F` and round-trips under the matcher's split-then-decode
// rule). A missing required param becomes an empty segment; the caller decides
// whether that is an error (a compile diagnostic when the id is literal).

const HEX_UPPER = "0123456789ABCDEF"

func url_encode_view(value : std::string_view, out : &mut std::string) {
    for(var i : size_t = 0; i < value.size(); i++) {
        const c = value.get(i)
        if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '-' || c == '.' || c == '_' || c == '~') {
            out.append(c)
        } else {
            out.append('%')
            const b = c as u8
            out.append(HEX_UPPER[(b >> 4) as size_t] as char)
            out.append(HEX_UPPER[(b & 0x0F) as size_t] as char)
        }
    }
}

public func build_path(patterns : &std::vector<RoutePattern>, id : std::string_view,
                       params : &std::unordered_map<std::string_view, std::string_view>) : std::string {
    for(var i : size_t = 0; i < patterns.size(); i++) {
        const pattern = patterns.get_ptr(i)
        if(!pattern.id.equals(&id)) { continue }
        if(pattern.is_fallback) { return std::string("/") }
        var out = std::string()
        for(var j : size_t = 0; j < pattern.segments.size(); j++) {
            const seg = pattern.segments.get(j)
            out.append('/')
            if(is_param_segment(seg)) {
                const name = seg.subview(1, seg.size() - 1)
                const p = params.get_ptr(&name)
                if(p != null) {
                    const pv = *p
                    if(pv.size() > 0) {
                        url_encode_view(pv, &mut out)
                    }
                }
            } else {
                out.append_view(&seg)
            }
        }
        if(out.size() == 0) { out.append_view(std::string_view("/")) }
        return out
    }
    return std::string()
}
