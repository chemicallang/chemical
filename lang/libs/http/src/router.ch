// ===== Router =====
public namespace web {

using std::Option;
using std::string;
using std::string_view;
using std::vector;

// A stored request handler. The witness type proves the capture environment is
// `Send + Sync + 'static` at registration, so a router can be shared by a
// worker pool and one handler invoked concurrently. A handler owns the request
// it is given and returns the response; the framework keeps the connection.
public type Handler = std.send_sync_function<(req: http.Request) => http.Response>

public struct Route {
    var method: string;
    var pattern: string;
    var handler: Handler;
}

public struct Router {
    var routes: vector<Route>;

    @constructor func constructor() {
        return Router {
            routes : vector<Route>()
        }
    }

    /// Register `h` for `method` + `pattern`. `pattern` segments of the form
    /// `:name` capture one segment, `:name*` capture the rest of the path.
    /// Both arguments are `str`, so string literals convert implicitly.
    public func add(&mut self, method : std::str, pattern : std::str, h: Handler) {
        var m = std::string()
        m.append_view(method.to_view())
        var p = std::string()
        p.append_view(pattern.to_view())
        routes.push_back(Route {
            method: m,
            pattern: p,
            handler: h
        })
    }

    /// The first route matching `method` + `path`, with its captured segments
    /// appended to `params`.
    public func match_route(&self, method: &string, path: &string,
                            params : &mut vector<std::pair<string, string>>) : Option<&'self Route> {
        var i = 0u;
        while(i < routes.size()) {
            var r = routes.at_ref(i);
            if(r.method.equals_with_len(method.data(), method.size())) {
                if(match_pattern(&r.pattern, path, params)) {
                    return Option.Some<&'self Route>(r)
                }
            }
            i = i + 1u;
        }
        return Option.None<&'self Route>()
    }
}

public func match_pattern(pattern: &string, path: &string, params : &mut vector<std::pair<string, string>>) : bool {
    var pseg = split_segments(pattern);
    var tseg = split_segments(path);

    var i = 0u;
    while(i < pseg.size()) {
        var ps = pseg.at_ref(i);

        if(ps.size() > 0u && ps.at(0u) == ':' ) {
            if (ps.at(ps.size() - 1u) == '*') {
                // Wildcard: capture the rest of the path (may be empty).
                var key = ps.substring(1u, ps.size() - 1u);
                var val = std::string();
                var j = i;
                while (j < tseg.size()) {
                    if (j > i) { val.append('/') }
                    var ts = tseg.at_ref(j);
                    val.append_string(ts)
                    j = j + 1u;
                }
                params.push_back(std::pair<string,string>{ first : key, second : val });
                return true;
            }

            if (i >= tseg.size()) { return false }
            var ts = tseg.at_ref(i);
            var key = ps.substring(1u, ps.size());
            params.push_back(std::pair<string,string>{ first : key, second : ts.copy() });
        } else {
            if (i >= tseg.size()) { return false }
            var ts = tseg.at_ref(i);
            if(!ps.equals_with_len(ts.data(), ts.size())) { return false }
        }
        i = i + 1u;
    }

    return i == tseg.size();
}

public func split_segments(s: &string) : vector<string> {
    var out = vector<string>();
    var i = 0u; var start = 0u;
    if(i < s.size() && s.at(i) == '/') { i++; start = i }
    while(i <= s.size()) {
        if(i == s.size() || s.at(i) == '/') {
            out.push_back(s.substring(start, i));
            start = i + 1u;
        }
        i = i + 1u;
    }
    return out;
}

} // namespace web
